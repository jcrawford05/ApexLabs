// Apex Labs storage layer: run history in SQLite, cars and courses as JSON.
//
// One Database object wraps one SQLite file built from schema.sql (the team
// spreadsheet schema, v3). Cars, courses and runs go in and come out as
// nlohmann::json in the same shapes as the app's files in data/
// (sample_vehicle.json, sample_track.json, sample_run.json), so the ImGui app
// can keep its existing JSON parsing and only swap where the data comes from.
//
// Every method throws apex::db::Error on failure (bad JSON, a broken rule in
// the schema, a missing car or course). Nothing here depends on ImGui or SDL.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

struct sqlite3;

namespace apex::db {

using json = nlohmann::json;

struct Error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// ---- Rows for list screens (cheap to load, no telemetry) --------------------

struct CarRow {
    int64_t id = 0;
    std::string name, regulation, tire, updated_at;
    double total_mass_kg = 0, total_cost_usd = 0;
};

struct CourseRow {
    int64_t id = 0;
    std::string name, country, source;
    double length_m = 0, longest_straight_m = 0;
    int corners = 0, segments = 0;
    bool is_template = false;
};

// One run, as the Compare screen and history list show it.
struct RunRow {
    int64_t run_id = 0;
    std::string date, car_name, course_name, weather_preset;  // weather as shown: "Light rain"
    double air_c = 0, track_c = 0;
    double lap_time_s = 0, top_speed_kmh = 0;
    double sector_times_s[3] = {};
    bool has_telemetry = false;
};

struct RunFilter {
    std::string course_name;  // empty = any course
    std::string car_name;     // empty = any car (matched on the run's car snapshot)
    int limit = 0;            // 0 = no limit
};

// Run B measured against run A. Positive delta = B is slower.
struct RunComparison {
    RunRow a, b;
    double lap_delta_s = 0;
    double sector_delta_s[3] = {};
    struct Segment { int seq; std::string name; double a_s, b_s, delta_s; };
    std::vector<Segment> segments;
    struct SetupDiff { std::string label, a, b; };
    std::vector<SetupDiff> setup_changes;  // only settings that differ
};

struct SaveRunOptions {
    bool keep_run_id = false;  // use run["run_id"] instead of a new id (for importing history)
};

class Database {
public:
    // Opens (or creates) the database file. A new file gets schema.sql plus the
    // placeholder regulation sets.
    Database(const std::string& db_path, const std::string& schema_sql_path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    int SchemaVersion() const;  // PRAGMA user_version
    const std::string& Path() const { return path_; }

    // ---- Cars (vehicle JSON file shape) -------------------------------------
    // Inserts the car, or replaces the saved car with the same name (its id is
    // kept, so past runs stay linked). Setup values go in cars.setup_json.
    int64_t SaveCar(const json& car, bool is_template = false);
    std::optional<json> LoadCar(const std::string& name) const;
    std::vector<CarRow> ListCars() const;
    bool DeleteCar(const std::string& name);  // past runs keep their car_setup_snapshot

    // ---- Courses (track JSON file shape) ------------------------------------
    // Inserts or updates by name. Segments are updated in place by sequence
    // number, so runs that point at them stay valid. Built-in (template)
    // courses are read-only.
    int64_t SaveCourse(const json& course, bool is_template = false);
    std::optional<json> LoadCourse(const std::string& name) const;
    std::vector<CourseRow> ListCourses() const;
    bool DeleteCourse(const std::string& name);  // refused while runs use it

    // ---- Parts catalog (tire_catalog.json) ----------------------------------
    void SaveTireCatalog(const json& catalog);

    // ---- Runs ---------------------------------------------------------------
    // Saves a finished run (sample_run.json shape, or a history entry with only
    // section_times_s). run["track"] must name a saved course. The car JSON is
    // stored with the run as car_setup_snapshot.
    int64_t SaveRun(const json& run, const json& car, SaveRunOptions opts = {});
    // Same, snapshotting the saved car named in run["vehicle"].
    int64_t SaveRunForSavedCar(const json& run, SaveRunOptions opts = {});
    // Full run in sample_run.json shape, plus "car_snapshot".
    std::optional<json> LoadRun(int64_t run_id) const;
    std::vector<RunRow> ListRuns(const RunFilter& filter = {}) const;  // newest first
    bool DeleteRun(int64_t run_id);
    RunComparison CompareRuns(int64_t run_a, int64_t run_b) const;
    std::optional<RunRow> BestRun(const std::string& course_name, const std::string& car_name = "") const;
    // Settings > "Keep telemetry for": drops telemetry of all but the newest N runs.
    int TrimTelemetry(int keep_newest_runs);

    // ---- Settings (app_settings) ----------------------------------------------
    void SetSetting(const std::string& key, const json& value);
    std::optional<json> GetSetting(const std::string& key) const;
    json AllSettings() const;

    // Setup shown as label/value pairs ("Rear wing angle", "12.0°"), in the
    // order the Compare screen lists them. Works on any car JSON.
    static std::vector<std::pair<std::string, std::string>> SetupLabels(const json& car);

    // Runs fn inside one transaction; rolls back if it throws.
    template <class Fn> void Transaction(Fn&& fn) {
        Exec("BEGIN");
        try { fn(); } catch (...) { Exec("ROLLBACK"); throw; }
        Exec("COMMIT");
    }

private:
    void Exec(const std::string& sql) const;
    void CreateSchema(const std::string& schema_sql_path);
    int64_t TypeId(const std::string& category) const;  // "aero" -> component_types id
    std::optional<int64_t> RegulationId(const std::string& name) const;
    int64_t UpsertComponent(int64_t type_id, const json& part, const json& extra_properties = json::object(),
                            bool is_template = false);
    std::optional<int64_t> IdByName(const char* table, const char* id_col, const std::string& name) const;

    sqlite3* db_ = nullptr;
    std::string path_;
    std::map<std::string, int64_t> type_ids_;  // lowercase type name -> id
};

// Display helpers shared with the CLI (same formats as the app).
std::string FormatLapTime(double seconds);  // 1:18.432
std::string FormatDelta(double seconds);    // +0.220 s

}  // namespace apex::db
