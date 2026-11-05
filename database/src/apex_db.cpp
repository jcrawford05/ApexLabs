#include "apex_db.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <set>
#include <sstream>

namespace apex::db {

namespace {

constexpr int kSchemaVersion = 3;  // matches PRAGMA user_version in schema.sql
const char* const kCategories[] = {"aero", "engine", "tire", "chassis", "brakes"};

// Placeholder regulation sets until the team sets real limits.
struct Regulation { const char* name; const char* description; double budget, mass; };
const Regulation kRegulations[] = {
    {"Apex 2026 Default", "Placeholder limits", 20'000'000, 850},
    {"Apex 2026 Low Budget", "Placeholder limits", 12'000'000, 850},
    {"Open (no limits)", "No budget or mass limit", NAN, NAN},
};

// Setup keys a car file can have, with the value used when a file leaves one out.
const json& SetupDefaults() {
    static const json d = {
        {"front_wing_angle_deg", 8.0},     {"rear_wing_angle_deg", 12.0},
        {"front_ride_height_mm", 31.0},    {"floor_load_pct", 62.0},
        {"tire_pressure_front_psi", 22.5}, {"tire_pressure_rear_psi", 21.8},
        {"power_map", "Race"},             {"final_drive_ratio", 3.2},
        {"shift_rpm", 14500.0},            {"rear_ride_height_mm", 78.0},
        {"ballast_kg", 5.0},               {"ballast_front_pct", 50.0},
        {"brake_bias_front_pct", 57.0},    {"brake_cooling", "Standard"},
    };
    return d;
}

// "Light rain" (app) <-> "light_rain" (schema ENUM).
std::string WeatherToDb(std::string s) {
    for (auto& c : s) c = (c == ' ') ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
std::string WeatherFromDb(std::string s) {
    for (auto& c : s) if (c == '_') c = ' ';
    if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

std::string Lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Small RAII wrapper around a prepared statement. Bind indexes start at 1,
// column indexes at 0, like SQLite's own API.
class Stmt {
public:
    Stmt(sqlite3* db, const std::string& sql) : db_(db) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &s_, nullptr) != SQLITE_OK)
            throw Error(std::string("SQL error: ") + sqlite3_errmsg(db) + "\n  in: " + sql);
    }
    ~Stmt() { sqlite3_finalize(s_); }
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;

    Stmt& Null(int i) { Check(sqlite3_bind_null(s_, i)); return *this; }
    Stmt& Int(int i, int64_t v) { Check(sqlite3_bind_int64(s_, i, v)); return *this; }
    Stmt& Real(int i, double v) { Check(sqlite3_bind_double(s_, i, v)); return *this; }
    Stmt& Text(int i, const std::string& v) {
        Check(sqlite3_bind_text(s_, i, v.c_str(), static_cast<int>(v.size()), SQLITE_TRANSIENT));
        return *this;
    }
    Stmt& OptInt(int i, const std::optional<int64_t>& v) { return v ? Int(i, *v) : Null(i); }
    // Binds a JSON scalar as the matching SQL type (objects/arrays as JSON text).
    Stmt& Json(int i, const json& v) {
        if (v.is_null()) return Null(i);
        if (v.is_boolean()) return Int(i, v.get<bool>() ? 1 : 0);
        if (v.is_number_integer()) return Int(i, v.get<int64_t>());
        if (v.is_number()) return Real(i, v.get<double>());
        if (v.is_string()) return Text(i, v.get<std::string>());
        return Text(i, v.dump());
    }

    bool Step() {  // true = a row is ready, false = done
        int rc = sqlite3_step(s_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw Error(sqlite3_errmsg(db_));
    }
    void Run() { while (Step()) {} }
    void Reset() { sqlite3_reset(s_); sqlite3_clear_bindings(s_); }

    bool IsNull(int c) const { return sqlite3_column_type(s_, c) == SQLITE_NULL; }
    int64_t ColInt(int c) const { return sqlite3_column_int64(s_, c); }
    double ColReal(int c) const { return sqlite3_column_double(s_, c); }
    std::string ColText(int c) const {
        auto* p = reinterpret_cast<const char*>(sqlite3_column_text(s_, c));
        return p ? p : "";
    }
    json ColJson(int c) const {  // null, integer, real or string
        switch (sqlite3_column_type(s_, c)) {
            case SQLITE_NULL: return nullptr;
            case SQLITE_INTEGER: return ColInt(c);
            case SQLITE_FLOAT: return ColReal(c);
            default: return ColText(c);
        }
    }

private:
    void Check(int rc) {
        if (rc != SQLITE_OK) throw Error(std::string("bind failed: ") + sqlite3_errmsg(db_));
    }
    sqlite3* db_;
    sqlite3_stmt* s_ = nullptr;
};

// Nestable transaction (SAVEPOINT), so SaveCar works inside a bigger import.
class Savepoint {
public:
    explicit Savepoint(sqlite3* db) : db_(db) { Exec("SAVEPOINT apex"); }
    ~Savepoint() {
        if (!done_) { Exec("ROLLBACK TO apex"); Exec("RELEASE apex"); }
    }
    void Commit() { Exec("RELEASE apex"); done_ = true; }

private:
    void Exec(const char* sql) { sqlite3_exec(db_, sql, nullptr, nullptr, nullptr); }
    sqlite3* db_;
    bool done_ = false;
};

json Require(const json& j, const char* key, const std::string& what) {
    if (!j.is_object() || !j.contains(key) || j[key].is_null())
        throw Error(what + " is missing \"" + key + "\"");
    return j[key];
}

std::string NowUtc() {
    char buf[32];
    std::time_t t = std::time(nullptr);
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::gmtime(&t));
    return buf;
}

std::string Fmt(const char* f, double v) {
    char buf[64];
    std::snprintf(buf, sizeof buf, f, v);
    return buf;
}

// The run list query: one row per run with its sector times.
const char* kRunRowSql =
    "SELECT r.run_id, substr(r.created_at, 1, 10), json_extract(r.car_setup_snapshot, '$.name'), c.name,"
    "       r.weather_preset, r.air_temp_c, r.track_temp_c, r.lap_time_s, r.top_speed_kmh,"
    "       (SELECT time_s FROM run_sectors WHERE run_id = r.run_id AND sector_number = 1),"
    "       (SELECT time_s FROM run_sectors WHERE run_id = r.run_id AND sector_number = 2),"
    "       (SELECT time_s FROM run_sectors WHERE run_id = r.run_id AND sector_number = 3),"
    "       EXISTS (SELECT 1 FROM telemetry_samples WHERE run_id = r.run_id)"
    "  FROM simulation_runs r JOIN courses c ON c.course_id = r.course_id ";

RunRow ReadRunRow(const Stmt& q) {
    RunRow r;
    r.run_id = q.ColInt(0);
    r.date = q.ColText(1);
    r.car_name = q.ColText(2);
    r.course_name = q.ColText(3);
    r.weather_preset = WeatherFromDb(q.ColText(4));
    r.air_c = q.ColReal(5);
    r.track_c = q.ColReal(6);
    r.lap_time_s = q.ColReal(7);
    r.top_speed_kmh = q.ColReal(8);
    for (int i = 0; i < 3; ++i) r.sector_times_s[i] = q.ColReal(9 + i);
    r.has_telemetry = q.ColInt(12) != 0;
    return r;
}

}  // namespace

// ============================================================================
// Opening and schema
// ============================================================================

Database::Database(const std::string& db_path, const std::string& schema_sql_path) : path_(db_path) {
    if (sqlite3_open_v2(db_path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        throw Error("Could not open " + db_path + ": " + msg);
    }
    sqlite3_busy_timeout(db_, 2000);
    try {
        Exec("PRAGMA foreign_keys = ON");  // SQLite leaves this off by default; the cascades need it
        int version = SchemaVersion();
        if (version == 0) {
            CreateSchema(schema_sql_path);
        } else if (version != kSchemaVersion) {
            throw Error(db_path + " uses schema v" + std::to_string(version) + ", this build expects v" +
                        std::to_string(kSchemaVersion) + ". Delete it to start fresh.");
        }
        // Future migrations go here: if (version < 4) { ...; Exec("PRAGMA user_version = 4"); }
        Stmt q(db_, "SELECT component_type_id, name FROM component_types");
        while (q.Step()) type_ids_[Lower(q.ColText(1))] = q.ColInt(0);
    } catch (...) {
        sqlite3_close(db_);
        throw;
    }
}

Database::~Database() { sqlite3_close(db_); }

void Database::Exec(const std::string& sql) const {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw Error("SQL error: " + msg);
    }
}

int Database::SchemaVersion() const {
    Stmt q(db_, "PRAGMA user_version");
    q.Step();
    return static_cast<int>(q.ColInt(0));
}

void Database::CreateSchema(const std::string& schema_sql_path) {
    std::ifstream in(schema_sql_path);
    if (!in) throw Error("Could not read the schema file " + schema_sql_path);
    std::stringstream sql;
    sql << in.rdbuf();

    Exec("BEGIN");
    try {
        Exec(sql.str());
        Stmt ins(db_, "INSERT INTO regulations (name, description, budget_limit_usd, mass_limit_kg) VALUES (?, ?, ?, ?)");
        for (const auto& r : kRegulations) {
            ins.Text(1, r.name).Text(2, r.description);
            std::isnan(r.budget) ? ins.Null(3) : ins.Real(3, r.budget);
            std::isnan(r.mass) ? ins.Null(4) : ins.Real(4, r.mass);
            ins.Run();
            ins.Reset();
        }
        Exec("COMMIT");
    } catch (...) {
        Exec("ROLLBACK");
        throw;
    }
}

std::optional<int64_t> Database::IdByName(const char* table, const char* id_col, const std::string& name) const {
    Stmt q(db_, std::string("SELECT ") + id_col + " FROM " + table + " WHERE name = ? ORDER BY " + id_col + " LIMIT 1");
    q.Text(1, name);
    if (q.Step()) return q.ColInt(0);
    return std::nullopt;
}

int64_t Database::TypeId(const std::string& category) const {
    auto it = type_ids_.find(Lower(category));
    if (it == type_ids_.end()) throw Error("Unknown component type \"" + category + "\"");
    return it->second;
}

std::optional<int64_t> Database::RegulationId(const std::string& name) const {
    if (name.empty()) return std::nullopt;
    auto id = IdByName("regulations", "regulation_id", name);
    if (!id) throw Error("Unknown regulation set \"" + name + "\"");
    return id;
}

int64_t Database::UpsertComponent(int64_t type_id, const json& part, const json& extra_properties, bool is_template) {
    const std::string name = Require(part, "name", "A car part").get<std::string>();
    json props = part.value("properties", json::object());
    props.update(extra_properties);

    Stmt find(db_, "SELECT component_id, properties_json FROM components WHERE component_type_id = ? AND name = ?");
    find.Int(1, type_id).Text(2, name);
    if (find.Step()) {
        const int64_t id = find.ColInt(0);
        json merged = json::parse(find.ColText(1));  // keep keys the file doesn't mention (e.g. tire labels)
        merged.update(props);
        Stmt up(db_, "UPDATE components SET cost_usd = COALESCE(?, cost_usd), mass_kg = COALESCE(?, mass_kg),"
                     " properties_json = ? WHERE component_id = ?");
        up.Json(1, part.value("cost_usd", json())).Json(2, part.value("mass_kg", json()))
            .Text(3, merged.dump()).Int(4, id).Run();
        return id;
    }
    Stmt ins(db_, "INSERT INTO components (component_type_id, name, cost_usd, mass_kg, properties_json, is_template)"
                  " VALUES (?, ?, COALESCE(?, 0), COALESCE(?, 0), ?, ?)");
    ins.Int(1, type_id).Text(2, name).Json(3, part.value("cost_usd", json())).Json(4, part.value("mass_kg", json()))
        .Text(5, props.dump()).Int(6, is_template ? 1 : 0).Run();
    return sqlite3_last_insert_rowid(db_);
}

// ============================================================================
// Cars
// ============================================================================

int64_t Database::SaveCar(const json& car, bool is_template) {
    const std::string name = Require(car, "name", "The car file").get<std::string>();
    const json& parts = Require(car, "components", "Car \"" + name + "\"");
    for (const char* cat : kCategories)
        if (!parts.contains(cat)) throw Error("Car \"" + name + "\" has no " + cat + " part");

    json setup = SetupDefaults();  // fill in anything the file leaves out
    setup.update(car.value("setup", json::object()));
    json description = car.contains("description") ? car["description"] : car.value("notes", json());

    Savepoint sp(db_);
    const auto reg = RegulationId(car.value("regulation", std::string()));
    std::optional<int64_t> id = IdByName("cars", "car_id", name);
    if (id) {
        Stmt ro(db_, "SELECT is_read_only FROM cars WHERE car_id = ?");
        ro.Int(1, *id).Step();
        if (ro.ColInt(0)) throw Error("Car \"" + name + "\" is read-only. Save it under a new name.");
        Stmt up(db_, "UPDATE cars SET regulation_id = ?, description = ?, setup_json = ? WHERE car_id = ?");
        up.OptInt(1, reg).Json(2, description).Text(3, setup.dump()).Int(4, *id).Run();
        Stmt del(db_, "DELETE FROM car_components WHERE car_id = ?");
        del.Int(1, *id).Run();
    } else {
        Stmt ins(db_, "INSERT INTO cars (regulation_id, name, description, setup_json, is_template, is_read_only)"
                      " VALUES (?, ?, ?, ?, ?, 0)");
        ins.OptInt(1, reg).Text(2, name).Json(3, description).Text(4, setup.dump()).Int(5, is_template ? 1 : 0).Run();
        id = sqlite3_last_insert_rowid(db_);
    }

    Stmt fit(db_, "INSERT INTO car_components (car_id, component_type_id, component_id) VALUES (?, ?, ?)");
    for (const char* cat : kCategories) {
        const int64_t type = TypeId(cat);
        fit.Int(1, *id).Int(2, type).Int(3, UpsertComponent(type, parts[cat])).Run();
        fit.Reset();
    }
    sp.Commit();
    return *id;
}

std::optional<json> Database::LoadCar(const std::string& name) const {
    Stmt q(db_, "SELECT c.car_id, r.name, c.description, c.setup_json FROM cars c"
                " LEFT JOIN regulations r ON r.regulation_id = c.regulation_id"
                " WHERE c.name = ? ORDER BY c.car_id LIMIT 1");
    q.Text(1, name);
    if (!q.Step()) return std::nullopt;

    json car = {{"schema_version", 1}, {"name", name}, {"regulation", q.ColJson(1)},
                {"setup", json::parse(q.ColText(3))}};
    if (!q.IsNull(2)) car["description"] = q.ColText(2);

    Stmt p(db_, "SELECT t.name, k.name, k.cost_usd, k.mass_kg, k.properties_json FROM car_components cc"
                " JOIN components k ON k.component_id = cc.component_id"
                " JOIN component_types t ON t.component_type_id = cc.component_type_id WHERE cc.car_id = ?");
    p.Int(1, q.ColInt(0));
    json parts = json::object();
    while (p.Step()) {
        json props = json::parse(p.ColText(4));
        props.erase("short_name");  // catalog-only display fields
        props.erase("labels");
        parts[Lower(p.ColText(0))] = {{"name", p.ColText(1)}, {"cost_usd", p.ColJson(2)},
                                      {"mass_kg", p.ColJson(3)}, {"properties", props}};
    }
    car["components"] = parts;
    return car;
}

std::vector<CarRow> Database::ListCars() const {
    Stmt q(db_,
           "SELECT c.car_id, c.name, COALESCE(r.name, ''), c.updated_at,"
           "       COALESCE(SUM(k.mass_kg), 0) + COALESCE(json_extract(c.setup_json, '$.ballast_kg'), 0),"
           "       COALESCE(SUM(k.cost_usd), 0),"
           "       COALESCE(MAX(CASE WHEN cc.component_type_id = ?1"
           "                    THEN COALESCE(json_extract(k.properties_json, '$.short_name'), k.name) END), '')"
           "  FROM cars c"
           "  LEFT JOIN regulations r ON r.regulation_id = c.regulation_id"
           "  LEFT JOIN car_components cc ON cc.car_id = c.car_id"
           "  LEFT JOIN components k ON k.component_id = cc.component_id"
           " GROUP BY c.car_id ORDER BY c.name");
    q.Int(1, TypeId("tire"));
    std::vector<CarRow> out;
    while (q.Step()) {
        CarRow r;
        r.id = q.ColInt(0);
        r.name = q.ColText(1);
        r.regulation = q.ColText(2);
        r.updated_at = q.ColText(3);
        r.total_mass_kg = q.ColReal(4);
        r.total_cost_usd = q.ColReal(5);
        r.tire = q.ColText(6);
        out.push_back(r);
    }
    return out;
}

bool Database::DeleteCar(const std::string& name) {
    Stmt q(db_, "DELETE FROM cars WHERE name = ?");  // runs keep car_setup_snapshot; car_id becomes NULL
    q.Text(1, name).Run();
    return sqlite3_changes(db_) > 0;
}

// ============================================================================
// Courses and the parts catalog
// ============================================================================

int64_t Database::SaveCourse(const json& course, bool is_template) {
    const std::string name = Require(course, "name", "The track file").get<std::string>();
    const json& segs = Require(course, "segments", "Course \"" + name + "\"");
    if (!segs.is_array() || segs.empty()) throw Error("Course \"" + name + "\" has no segments");

    Savepoint sp(db_);
    std::optional<int64_t> id = IdByName("courses", "course_id", name);
    std::string source = is_template ? "built-in" : course.value("source", std::string("user"));
    if (!is_template && source == "built-in") source = "user";  // an exported built-in course is a user copy
    if (id) {
        Stmt ro(db_, "SELECT is_read_only FROM courses WHERE course_id = ?");
        ro.Int(1, *id).Step();
        if (ro.ColInt(0)) throw Error("Course \"" + name + "\" is built in and read-only. Save it under a new name.");
        Stmt up(db_, "UPDATE courses SET country = ?, openf1_circuit_key = ?, source = ? WHERE course_id = ?");
        up.Json(1, course.value("country", json())).Json(2, course.value("openf1_circuit_key", json()))
            .Text(3, source).Int(4, *id).Run();
    } else {
        Stmt ins(db_, "INSERT INTO courses (name, country, openf1_circuit_key, source, is_template, is_read_only)"
                      " VALUES (?, ?, ?, ?, ?, ?)");
        ins.Text(1, name).Json(2, course.value("country", json())).Json(3, course.value("openf1_circuit_key", json()))
            .Text(4, source).Int(5, is_template ? 1 : 0).Int(6, is_template ? 1 : 0).Run();
        id = sqlite3_last_insert_rowid(db_);
    }

    // Update segments in place, keyed by sequence number, so segment_ids that
    // run results point at survive an edit.
    Stmt seg(db_,
             "INSERT INTO course_segments (course_id, sequence_number, name, segment_type, sector_number, length_m, radius_m)"
             " VALUES (?, ?, ?, ?, ?, ?, ?)"
             " ON CONFLICT (course_id, sequence_number) DO UPDATE SET name = excluded.name,"
             "   segment_type = excluded.segment_type, sector_number = excluded.sector_number,"
             "   length_m = excluded.length_m, radius_m = excluded.radius_m");
    std::set<int64_t> seqs;
    for (const auto& s : segs) {
        const json& seq = Require(s, "seq", "A segment of \"" + name + "\"");
        seqs.insert(seq.get<int64_t>());
        seg.Int(1, *id).Json(2, seq).Json(3, s.value("name", json()))
            .Json(4, Require(s, "type", "Segment " + seq.dump()))
            .Json(5, Require(s, "sector", "Segment " + seq.dump()))
            .Json(6, Require(s, "length_m", "Segment " + seq.dump()))
            .Json(7, s.value("radius_m", json()));
        try {
            seg.Run();
        } catch (const Error& e) {
            throw Error("Course \"" + name + "\" segment " + seq.dump() + ": " + e.what());
        }
        seg.Reset();
    }
    Stmt old(db_, "SELECT segment_id, sequence_number FROM course_segments WHERE course_id = ?");
    old.Int(1, *id);
    Stmt del(db_, "DELETE FROM course_segments WHERE segment_id = ?");
    while (old.Step()) {
        if (seqs.count(old.ColInt(1))) continue;
        try {
            del.Int(1, old.ColInt(0)).Run();
        } catch (const Error&) {
            throw Error("Course \"" + name + "\" has runs that use segment " + std::to_string(old.ColInt(1)) +
                        ", so it can't be removed. Save the edited course under a new name.");
        }
        del.Reset();
    }
    sp.Commit();
    return *id;
}

std::optional<json> Database::LoadCourse(const std::string& name) const {
    Stmt q(db_, "SELECT course_id, country, openf1_circuit_key, source FROM courses WHERE name = ?"
                " ORDER BY course_id LIMIT 1");
    q.Text(1, name);
    if (!q.Step()) return std::nullopt;
    json c = {{"schema_version", 1}, {"name", name}, {"country", q.ColJson(1)},
              {"openf1_circuit_key", q.ColJson(2)}, {"source", q.ColJson(3)}};
    Stmt s(db_, "SELECT sequence_number, name, segment_type, sector_number, length_m, radius_m"
                " FROM course_segments WHERE course_id = ? ORDER BY sequence_number");
    s.Int(1, q.ColInt(0));
    json segs = json::array();
    while (s.Step()) {
        json seg = {{"seq", s.ColInt(0)}, {"name", s.ColJson(1)}, {"type", s.ColText(2)},
                    {"sector", s.ColInt(3)}, {"length_m", s.ColReal(4)}};
        if (!s.IsNull(5)) seg["radius_m"] = s.ColReal(5);
        segs.push_back(seg);
    }
    c["segments"] = segs;
    return c;
}

std::vector<CourseRow> Database::ListCourses() const {
    Stmt q(db_,
           "SELECT c.course_id, c.name, COALESCE(c.country, ''), COALESCE(c.source, ''), c.is_template,"
           "       SUM(s.length_m), SUM(s.segment_type = 'corner'),"
           "       MAX(CASE WHEN s.segment_type = 'straight' THEN s.length_m END), COUNT(s.segment_id)"
           "  FROM courses c LEFT JOIN course_segments s ON s.course_id = c.course_id"
           " GROUP BY c.course_id ORDER BY c.name");
    std::vector<CourseRow> out;
    while (q.Step()) {
        CourseRow r;
        r.id = q.ColInt(0);
        r.name = q.ColText(1);
        r.country = q.ColText(2);
        r.source = q.ColText(3);
        r.is_template = q.ColInt(4) != 0;
        r.length_m = q.ColReal(5);
        r.corners = static_cast<int>(q.ColInt(6));
        r.longest_straight_m = q.ColReal(7);
        r.segments = static_cast<int>(q.ColInt(8));
        out.push_back(r);
    }
    return out;
}

bool Database::DeleteCourse(const std::string& name) {
    auto id = IdByName("courses", "course_id", name);
    if (!id) return false;
    Stmt runs(db_, "SELECT COUNT(*) FROM simulation_runs WHERE course_id = ?");
    runs.Int(1, *id).Step();
    if (runs.ColInt(0) > 0)
        throw Error("Course \"" + name + "\" has " + std::to_string(runs.ColInt(0)) +
                    " saved runs. Delete those runs first.");
    Stmt q(db_, "DELETE FROM courses WHERE course_id = ?");
    q.Int(1, *id).Run();
    return true;
}

void Database::SaveTireCatalog(const json& catalog) {
    const json& tires = Require(catalog, "tires", "The tire catalog");
    Savepoint sp(db_);
    const int64_t tire = TypeId("tire");
    for (const auto& t : tires) {
        // The spreadsheet has no columns for display text, so it rides along in properties_json.
        json extra = json::object();
        if (t.contains("short")) extra["short_name"] = t["short"];
        if (t.contains("labels")) extra["labels"] = t["labels"];
        UpsertComponent(tire, t, extra, true);
    }
    sp.Commit();
}

// ============================================================================
// Runs
// ============================================================================

int64_t Database::SaveRun(const json& run, const json& car, SaveRunOptions opts) {
    const double lap = Require(run, "lap_time_s", "The run").get<double>();
    const json& sectors = Require(run, "sector_times_s", "The run");
    if (!sectors.is_array() || sectors.size() != 3) throw Error("A run needs exactly three sector times");
    const std::string car_name = Require(car, "name", "The car snapshot").get<std::string>();
    const std::string course_name = Require(run, "track", "The run").get<std::string>();
    auto course_id = IdByName("courses", "course_id", course_name);
    if (!course_id) throw Error("Course \"" + course_name + "\" isn't saved in the database. Import it first.");

    std::string created = NowUtc();
    if (run.contains("date") && run["date"].is_string()) created = run["date"].get<std::string>() + " 12:00:00";

    // Segment sequence number -> segment_id for this course.
    std::map<int64_t, int64_t> seg_ids;
    {
        Stmt q(db_, "SELECT sequence_number, segment_id FROM course_segments WHERE course_id = ?");
        q.Int(1, *course_id);
        while (q.Step()) seg_ids[q.ColInt(0)] = q.ColInt(1);
    }
    auto segment_id = [&](int64_t seq) {
        auto it = seg_ids.find(seq);
        if (it == seg_ids.end())
            throw Error("Run has a result for segment " + std::to_string(seq) + ", which \"" + course_name +
                        "\" doesn't have");
        return it->second;
    };

    Savepoint sp(db_);
    const json weather = run.value("weather", json::object());
    std::optional<int64_t> reg;
    if (car.contains("regulation") && car["regulation"].is_string())
        reg = IdByName("regulations", "regulation_id", car["regulation"].get<std::string>());

    Stmt ins(db_,
             "INSERT INTO simulation_runs (run_id, car_id, course_id, regulation_id, engine_version, weather_preset,"
             "   air_temp_c, track_temp_c, humidity_pct, wind_kmh, rain_pct, lap_time_s, top_speed_kmh,"
             "   top_speed_at_m, car_setup_snapshot, created_at)"
             " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    opts.keep_run_id ? ins.Json(1, Require(run, "run_id", "The run")) : ins.Null(1);
    ins.OptInt(2, IdByName("cars", "car_id", car_name))
        .Int(3, *course_id)
        .OptInt(4, reg)
        .Text(5, run.value("engine_version", std::string("unknown")))
        .Text(6, WeatherToDb(weather.value("preset", std::string("Dry"))))
        .Json(7, weather.value("air_c", json()))
        .Json(8, weather.value("track_c", json()))
        .Json(9, weather.value("humidity_pct", json()))
        .Json(10, weather.value("wind_kmh", json()))
        .Json(11, weather.value("rain_pct", json()))
        .Real(12, lap)
        .Json(13, run.value("top_speed_kmh", json()))
        .Json(14, run.value("top_speed_at_m", json()))
        .Text(15, car.dump())
        .Text(16, created);
    try {
        ins.Run();
    } catch (const Error& e) {
        throw Error(std::string("Could not save the run: ") + e.what());
    }
    const int64_t run_id = sqlite3_last_insert_rowid(db_);

    const json deltas = run.value("sector_deltas_s", json::array());
    Stmt sec(db_, "INSERT INTO run_sectors (run_id, sector_number, time_s, delta_s) VALUES (?, ?, ?, ?)");
    for (int i = 0; i < 3; ++i) {
        sec.Int(1, run_id).Int(2, i + 1).Json(3, sectors[i]).Json(4, i < (int)deltas.size() ? deltas[i] : json()).Run();
        sec.Reset();
    }

    Stmt seg(db_, "INSERT INTO run_segment_results (run_id, segment_id, time_s, delta_s, min_speed_kmh, peak_brake_g,"
                  " tradeoff_note, effects_json) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    if (run.contains("sections")) {  // full result from the engine
        for (const auto& s : run["sections"]) {
            json effects = nullptr;
            if (s.contains("effects") || s.contains("other_effects_s"))
                effects = {{"effects", s.value("effects", json::array())},
                           {"other_effects_s", s.value("other_effects_s", json())}};
            seg.Int(1, run_id).Int(2, segment_id(s["seq"].get<int64_t>())).Json(3, s["time_s"])
                .Json(4, s.value("delta_s", json())).Json(5, s.value("min_speed_kmh", json()))
                .Json(6, s.value("peak_brake_g", json())).Json(7, s.value("tradeoff", json()))
                .Json(8, effects.is_null() ? json() : json(effects.dump()))
                .Run();
            seg.Reset();
        }
    } else if (run.contains("section_times_s")) {  // history summary: times only
        int64_t seq = 1;
        for (const auto& t : run["section_times_s"]) {
            seg.Int(1, run_id).Int(2, segment_id(seq++)).Json(3, t).Run();
            seg.Reset();
        }
    }

    const json tel = run.value("telemetry", json());
    if (tel.is_object() && tel.contains("speed_kmh")) {
        const double spacing = tel.value("spacing_m", 5.0);
        const json& speed = tel["speed_kmh"];
        auto pick = [&](const char* key, size_t i) {
            return tel.contains(key) && i < tel[key].size() ? tel[key][i] : json();
        };
        Stmt t(db_, "INSERT INTO telemetry_samples (run_id, sample_index, distance_m, speed_kmh, reference_speed_kmh,"
                    " throttle_pct, brake_pct, tire_temp_c) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
        for (size_t i = 0; i < speed.size(); ++i) {
            t.Int(1, run_id).Int(2, (int64_t)i).Real(3, i * spacing).Json(4, speed[i])
                .Json(5, pick("reference_speed_kmh", i)).Json(6, pick("throttle_pct", i))
                .Json(7, pick("brake_pct", i)).Json(8, pick("tire_temp_c", i)).Run();
            t.Reset();
        }
    }
    sp.Commit();
    return run_id;
}

int64_t Database::SaveRunForSavedCar(const json& run, SaveRunOptions opts) {
    const std::string name = Require(run, "vehicle", "The run").get<std::string>();
    auto car = LoadCar(name);
    if (!car) throw Error("Car \"" + name + "\" isn't saved in the database. Import the car first.");
    return SaveRun(run, *car, opts);
}

std::optional<json> Database::LoadRun(int64_t run_id) const {
    Stmt q(db_,
           "SELECT c.name, r.engine_version, substr(r.created_at, 1, 10), r.weather_preset, r.air_temp_c,"
           "       r.track_temp_c, r.humidity_pct, r.wind_kmh, r.rain_pct, r.lap_time_s, r.top_speed_kmh,"
           "       r.top_speed_at_m, r.car_setup_snapshot, reg.name"
           "  FROM simulation_runs r JOIN courses c ON c.course_id = r.course_id"
           "  LEFT JOIN regulations reg ON reg.regulation_id = r.regulation_id"
           " WHERE r.run_id = ?");
    q.Int(1, run_id);
    if (!q.Step()) return std::nullopt;

    json snapshot = json::parse(q.ColText(12));
    json weather = {{"preset", WeatherFromDb(q.ColText(3))}, {"air_c", q.ColJson(4)}, {"track_c", q.ColJson(5)}};
    if (!q.IsNull(6)) weather["humidity_pct"] = q.ColJson(6);
    if (!q.IsNull(7)) weather["wind_kmh"] = q.ColJson(7);
    if (!q.IsNull(8)) weather["rain_pct"] = q.ColJson(8);
    json r = {{"schema_version", 1},
              {"run_id", run_id},
              {"vehicle", snapshot.value("name", "")},
              {"track", q.ColText(0)},
              {"engine_version", q.ColText(1)},
              {"date", q.ColText(2)},
              {"regulation", q.ColJson(13)},
              {"weather", weather},
              {"lap_time_s", q.ColReal(9)},
              {"top_speed_kmh", q.ColJson(10)},
              {"top_speed_at_m", q.ColJson(11)},
              {"car_snapshot", snapshot}};

    Stmt s(db_, "SELECT time_s, delta_s FROM run_sectors WHERE run_id = ? ORDER BY sector_number");
    s.Int(1, run_id);
    json times = json::array(), deltas = json::array();
    bool all_deltas = true;
    double lap_delta = 0;
    while (s.Step()) {
        times.push_back(s.ColReal(0));
        deltas.push_back(s.ColJson(1));
        if (s.IsNull(1)) all_deltas = false; else lap_delta += s.ColReal(1);
    }
    r["sector_times_s"] = times;
    r["sector_deltas_s"] = deltas;
    // Not a column in the schema: the lap delta is the sum of the sector deltas.
    r["lap_delta_s"] = (all_deltas && !times.empty()) ? json(std::round(lap_delta * 1000) / 1000) : json();

    Stmt sec(db_, "SELECT g.sequence_number, x.time_s, x.delta_s, x.min_speed_kmh, x.peak_brake_g, x.tradeoff_note,"
                  "       x.effects_json"
                  "  FROM run_segment_results x JOIN course_segments g ON g.segment_id = x.segment_id"
                  " WHERE x.run_id = ? ORDER BY g.sequence_number");
    sec.Int(1, run_id);
    json sections = json::array();
    while (sec.Step()) {
        json fx = sec.IsNull(6) ? json::object() : json::parse(sec.ColText(6));
        sections.push_back({{"seq", sec.ColInt(0)}, {"time_s", sec.ColReal(1)}, {"delta_s", sec.ColJson(2)},
                            {"min_speed_kmh", sec.ColJson(3)}, {"peak_brake_g", sec.ColJson(4)},
                            {"effects", fx.value("effects", json::array())},
                            {"other_effects_s", fx.value("other_effects_s", json())},
                            {"tradeoff", sec.ColJson(5)}});
    }
    r["sections"] = sections;

    Stmt t(db_, "SELECT distance_m, speed_kmh, reference_speed_kmh, throttle_pct, brake_pct, tire_temp_c"
                "  FROM telemetry_samples WHERE run_id = ? ORDER BY sample_index");
    t.Int(1, run_id);
    const char* keys[] = {"speed_kmh", "reference_speed_kmh", "throttle_pct", "brake_pct", "tire_temp_c"};
    json arrays[5];
    double d0 = 0, d1 = -1;
    int n = 0;
    while (t.Step()) {
        if (n == 0) d0 = t.ColReal(0);
        if (n == 1) d1 = t.ColReal(0);
        for (int k = 0; k < 5; ++k) arrays[k].push_back(t.ColJson(k + 1));
        ++n;
    }
    if (n == 0) {
        r["telemetry"] = nullptr;  // never kept, or trimmed by TrimTelemetry
    } else {
        json tel = {{"spacing_m", d1 >= 0 ? d1 - d0 : 5.0}};
        for (int k = 0; k < 5; ++k)
            if (!arrays[k][0].is_null()) tel[keys[k]] = arrays[k];
        r["telemetry"] = tel;
    }
    return r;
}

std::vector<RunRow> Database::ListRuns(const RunFilter& f) const {
    Stmt q(db_, std::string(kRunRowSql) +
                    "WHERE (?1 = '' OR c.name = ?1) AND (?2 = '' OR json_extract(r.car_setup_snapshot, '$.name') = ?2)"
                    " ORDER BY r.created_at DESC, r.run_id DESC LIMIT ?3");
    q.Text(1, f.course_name).Text(2, f.car_name).Int(3, f.limit > 0 ? f.limit : -1);
    std::vector<RunRow> out;
    while (q.Step()) out.push_back(ReadRunRow(q));
    return out;
}

std::optional<RunRow> Database::BestRun(const std::string& course_name, const std::string& car_name) const {
    Stmt q(db_, std::string(kRunRowSql) +
                    "WHERE c.name = ?1 AND (?2 = '' OR json_extract(r.car_setup_snapshot, '$.name') = ?2)"
                    " ORDER BY r.lap_time_s LIMIT 1");
    q.Text(1, course_name).Text(2, car_name);
    if (q.Step()) return ReadRunRow(q);
    return std::nullopt;
}

bool Database::DeleteRun(int64_t run_id) {
    Stmt q(db_, "DELETE FROM simulation_runs WHERE run_id = ?");  // sectors, segments and telemetry cascade
    q.Int(1, run_id).Run();
    return sqlite3_changes(db_) > 0;
}

RunComparison Database::CompareRuns(int64_t run_a, int64_t run_b) const {
    auto row = [&](int64_t id) {
        Stmt q(db_, std::string(kRunRowSql) + "WHERE r.run_id = ?");
        q.Int(1, id);
        if (!q.Step()) throw Error("No run #" + std::to_string(id));
        return ReadRunRow(q);
    };
    RunComparison c;
    c.a = row(run_a);
    c.b = row(run_b);
    c.lap_delta_s = c.b.lap_time_s - c.a.lap_time_s;
    for (int i = 0; i < 3; ++i) c.sector_delta_s[i] = c.b.sector_times_s[i] - c.a.sector_times_s[i];

    Stmt seg(db_, "SELECT g.sequence_number, COALESCE(g.name, ''), a.time_s, b.time_s"
                  "  FROM run_segment_results a"
                  "  JOIN run_segment_results b ON b.segment_id = a.segment_id AND b.run_id = ?2"
                  "  JOIN course_segments g ON g.segment_id = a.segment_id"
                  " WHERE a.run_id = ?1 ORDER BY g.sequence_number");
    seg.Int(1, run_a).Int(2, run_b);
    while (seg.Step())
        c.segments.push_back({(int)seg.ColInt(0), seg.ColText(1), seg.ColReal(2), seg.ColReal(3),
                              seg.ColReal(3) - seg.ColReal(2)});

    auto snapshot = [&](int64_t id) {
        Stmt q(db_, "SELECT car_setup_snapshot FROM simulation_runs WHERE run_id = ?");
        q.Int(1, id);
        q.Step();
        return json::parse(q.ColText(0));
    };
    auto la = SetupLabels(snapshot(run_a)), lb = SetupLabels(snapshot(run_b));
    std::map<std::string, std::string> mb(lb.begin(), lb.end());
    for (const auto& [label, va] : la) {
        auto it = mb.find(label);
        std::string vb = it == mb.end() ? "" : it->second;
        if (va != vb) c.setup_changes.push_back({label, va, vb});
    }
    return c;
}

int Database::TrimTelemetry(int keep_newest_runs) {
    const std::string doomed =
        " FROM telemetry_samples WHERE run_id NOT IN"
        " (SELECT run_id FROM simulation_runs ORDER BY created_at DESC, run_id DESC LIMIT ?1)";
    Stmt count(db_, "SELECT COUNT(DISTINCT run_id)" + doomed);
    count.Int(1, keep_newest_runs);
    count.Step();
    const int runs = static_cast<int>(count.ColInt(0));
    Stmt del(db_, "DELETE" + doomed);
    del.Int(1, keep_newest_runs).Run();
    return runs;
}

// ============================================================================
// Settings
// ============================================================================

void Database::SetSetting(const std::string& key, const json& value) {
    Stmt q(db_, "INSERT INTO app_settings (setting_key, setting_value) VALUES (?, ?) ON CONFLICT (setting_key)"
                " DO UPDATE SET setting_value = excluded.setting_value, updated_at = CURRENT_TIMESTAMP");
    q.Text(1, key).Text(2, value.dump()).Run();
}

std::optional<json> Database::GetSetting(const std::string& key) const {
    Stmt q(db_, "SELECT setting_value FROM app_settings WHERE setting_key = ?");
    q.Text(1, key);
    if (q.Step()) return json::parse(q.ColText(0));
    return std::nullopt;
}

json Database::AllSettings() const {
    Stmt q(db_, "SELECT setting_key, setting_value FROM app_settings ORDER BY setting_key");
    json out = json::object();
    while (q.Step()) out[q.ColText(0)] = json::parse(q.ColText(1));
    return out;
}

// ============================================================================
// Helpers
// ============================================================================

std::vector<std::pair<std::string, std::string>> Database::SetupLabels(const json& car) {
    json s = SetupDefaults();
    s.update(car.value("setup", json::object()));
    auto num = [&](const char* key) { return s[key].is_number() ? s[key].get<double>() : 0.0; };
    auto text = [&](const char* key) { return s[key].is_string() ? s[key].get<std::string>() : s[key].dump(); };
    std::string tire;
    if (car.contains("components") && car["components"].contains("tire")) {
        tire = car["components"]["tire"].value("name", "");
        auto pos = tire.find(" Compound");
        if (pos != std::string::npos) tire.erase(pos);
    }
    return {
        {"Car", car.value("name", "")},
        {"Front wing angle", Fmt("%.1f°", num("front_wing_angle_deg"))},
        {"Rear wing angle", Fmt("%.1f°", num("rear_wing_angle_deg"))},
        {"Front ride height", Fmt("%.0f mm", num("front_ride_height_mm"))},
        {"Rear ride height", Fmt("%.0f mm", num("rear_ride_height_mm"))},
        {"Floor load", Fmt("%.0f%%", num("floor_load_pct"))},
        {"Ballast", Fmt("%.0f kg", num("ballast_kg"))},
        {"Ballast position", Fmt("%.0f%% front", num("ballast_front_pct"))},
        {"Tire compound", tire},
        {"Front pressure", Fmt("%.1f psi", num("tire_pressure_front_psi"))},
        {"Rear pressure", Fmt("%.1f psi", num("tire_pressure_rear_psi"))},
        {"Brake bias", Fmt("%.0f%% front", num("brake_bias_front_pct"))},
        {"Brake cooling", text("brake_cooling")},
        {"Power map", text("power_map")},
        {"Final drive", Fmt("%.2f", num("final_drive_ratio"))},
        {"Shift RPM", Fmt("%.0f", num("shift_rpm"))},
    };
}

std::string FormatLapTime(double seconds) {
    int m = static_cast<int>(seconds / 60);
    return std::to_string(m) + ":" + Fmt("%06.3f", seconds - m * 60);
}

std::string FormatDelta(double seconds) {
    if (std::fabs(seconds) < 0.0005) return "±0.000 s";
    return Fmt("%+.3f s", seconds);
}

}  // namespace apex::db
