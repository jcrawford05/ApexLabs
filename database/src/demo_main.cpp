// apexdb: command-line demo of the Apex Labs storage layer.
//
//   apexdb demo                  fresh database + a guided tour of every feature
//   apexdb [--db FILE] <command> see Usage() below
//
// Everything here goes through apex::db::Database, the same class the ImGui
// app will call, so what works in this demo works in the app.
#include "apex_db.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using apex::db::Database;
using apex::db::FormatDelta;
using apex::db::FormatLapTime;
using apex::db::json;

namespace {

fs::path g_exe_dir;

// schema.sql and data/ sit next to the executable (copied there by CMake);
// fall back to the source folder when run from somewhere else.
fs::path Resource(const std::string& name) {
    fs::path p = g_exe_dir / name;
    if (fs::exists(p)) return p;
    return fs::path(APEX_SOURCE_DIR) / name;
}

json ReadJson(const fs::path& path) {
    std::ifstream in(path);
    if (!in) throw apex::db::Error("Could not open " + path.string());
    try {
        return json::parse(in);
    } catch (const json::exception& e) {
        throw apex::db::Error(path.string() + " is not valid JSON: " + e.what());
    }
}

void WriteJson(const fs::path& path, const json& j) {
    std::ofstream out(path);
    if (!out) throw apex::db::Error("Could not write " + path.string());
    out << j.dump(2) << "\n";
}

void Heading(const std::string& title) { std::printf("\n\033[1;31m== %s ==\033[0m\n", title.c_str()); }

// printf pads by bytes, so "°" (2 bytes) would throw columns off.
std::string Pad(const std::string& s, size_t width) {
    size_t chars = 0;
    for (unsigned char c : s) chars += (c & 0xC0) != 0x80;
    return s + std::string(chars < width ? width - chars : 0, ' ');
}

// ---- Printing ---------------------------------------------------------------

void PrintCars(const Database& db) {
    std::printf("%-4s %-26s %-8s %10s %12s  %s\n", "ID", "CAR", "TIRE", "MASS", "COST", "REGULATION");
    for (const auto& v : db.ListCars())
        std::printf("%-4lld %-26s %-8s %7.0f kg %10.2fM  %s\n", (long long)v.id, v.name.c_str(), v.tire.c_str(),
                    v.total_mass_kg, v.total_cost_usd / 1e6, v.regulation.c_str());
}

void PrintCourses(const Database& db) {
    std::printf("%-4s %-20s %9s %8s %9s %17s  %s\n", "ID", "COURSE", "LENGTH", "CORNERS", "SEGMENTS",
                "LONGEST STRAIGHT", "SOURCE");
    for (const auto& t : db.ListCourses())
        std::printf("%-4lld %-20s %6.2f km %8d %9d %15.0f m  %s%s\n", (long long)t.id, t.name.c_str(),
                    t.length_m / 1000, t.corners, t.segments, t.longest_straight_m, t.source.c_str(),
                    t.is_template ? " (read-only)" : "");
}

void PrintRuns(const Database& db, const apex::db::RunFilter& f = {}) {
    std::printf("%-5s %-11s %-24s %-13s %-11s %9s %8s %8s %8s %6s  %s\n", "RUN", "DATE", "CAR", "COURSE", "WEATHER",
                "LAP", "S1", "S2", "S3", "TOP", "TELEMETRY");
    for (const auto& r : db.ListRuns(f))
        std::printf("#%-4lld %-11s %-24s %-13s %-11s %9s %8.3f %8.3f %8.3f %6.0f  %s\n", (long long)r.run_id,
                    r.date.c_str(), r.car_name.c_str(), r.course_name.c_str(), r.weather_preset.c_str(),
                    FormatLapTime(r.lap_time_s).c_str(), r.sector_times_s[0], r.sector_times_s[1],
                    r.sector_times_s[2], r.top_speed_kmh, r.has_telemetry ? "yes" : "-");
}

void PrintRun(const Database& db, int64_t id) {
    auto r = db.LoadRun(id);
    if (!r) throw apex::db::Error("No run #" + std::to_string(id));
    const json& j = *r;
    std::printf("Run #%lld  %s on %s  (%s, engine %s)\n", (long long)id, j["vehicle"].get<std::string>().c_str(),
                j["track"].get<std::string>().c_str(), j["date"].get<std::string>().c_str(),
                j["engine_version"].get<std::string>().c_str());
    std::printf("  Lap %s", FormatLapTime(j["lap_time_s"]).c_str());
    if (j["lap_delta_s"].is_number()) std::printf("  (%s vs reference)", FormatDelta(j["lap_delta_s"]).c_str());
    std::printf("   top speed %s km/h\n", j["top_speed_kmh"].dump().c_str());
    std::printf("  Weather %s, air %s C, track %s C\n", j["weather"]["preset"].get<std::string>().c_str(),
                j["weather"]["air_c"].dump().c_str(), j["weather"]["track_c"].dump().c_str());
    for (int i = 0; i < 3; ++i) std::printf("  Sector %d  %.3f s\n", i + 1, j["sector_times_s"][i].get<double>());

    std::map<int64_t, std::string> names;
    if (auto course = db.LoadCourse(j["track"]))
        for (const auto& s : (*course)["segments"]) names[s["seq"]] = s.value("name", "");
    std::printf("  Segments:\n");
    for (const auto& s : j["sections"]) {
        std::printf("    %2lld %-15s %7.3f s", (long long)s["seq"].get<int64_t>(), names[s["seq"]].c_str(),
                    s["time_s"].get<double>());
        if (s["delta_s"].is_number()) std::printf("  %s", FormatDelta(s["delta_s"]).c_str());
        if (!s["effects"].empty()) std::printf("   %zu setup effects", s["effects"].size());
        std::printf("\n");
    }
    if (j["telemetry"].is_object())
        std::printf("  Telemetry: %zu samples every %.0f m%s\n", j["telemetry"]["speed_kmh"].size(),
                    j["telemetry"]["spacing_m"].get<double>(),
                    j["telemetry"].contains("reference_speed_kmh") ? ", with the reference speed trace" : "");
    else
        std::printf("  Telemetry: not kept\n");
    std::printf("  Car snapshot: %s, rear wing %s deg\n", j["car_snapshot"]["name"].get<std::string>().c_str(),
                j["car_snapshot"]["setup"]["rear_wing_angle_deg"].dump().c_str());
}

void PrintCompare(const Database& db, int64_t a, int64_t b) {
    auto c = db.CompareRuns(a, b);
    std::printf("Run #%lld (%s)  vs  run #%lld (%s)\n", (long long)a, c.a.car_name.c_str(), (long long)b,
                c.b.car_name.c_str());
    std::printf("  Lap       %9s  %9s  %s\n", FormatLapTime(c.a.lap_time_s).c_str(),
                FormatLapTime(c.b.lap_time_s).c_str(), FormatDelta(c.lap_delta_s).c_str());
    for (int i = 0; i < 3; ++i)
        std::printf("  Sector %d  %9.3f  %9.3f  %s\n", i + 1, c.a.sector_times_s[i], c.b.sector_times_s[i],
                    FormatDelta(c.sector_delta_s[i]).c_str());
    std::printf("  Biggest segment differences:\n");
    auto segs = c.segments;
    std::sort(segs.begin(), segs.end(), [](auto& x, auto& y) { return std::fabs(x.delta_s) > std::fabs(y.delta_s); });
    for (size_t i = 0; i < segs.size() && i < 3; ++i)
        std::printf("    %2d %-15s %s\n", segs[i].seq, segs[i].name.c_str(), FormatDelta(segs[i].delta_s).c_str());
    std::printf("  Setup changes:\n");
    if (c.setup_changes.empty()) std::printf("    (same setup)\n");
    for (const auto& d : c.setup_changes)
        std::printf("    %s %s -> %s\n", Pad(d.label, 18).c_str(), Pad(d.a, 24).c_str(), d.b.c_str());
}

// ---- Loading the sample data -------------------------------------------------

// The history file only lists each run's setup as display labels
// ("Rear wing angle": "14.0°"). Turn those back into a full car file, starting
// from the sample car, so every run gets a real car snapshot.
json CarFromLabels(const json& base, const json& labels, const json& tires) {
    json car = base;
    car["name"] = labels.value("Car", base["name"].get<std::string>());
    const std::map<std::string, const char*> numeric = {
        {"Front wing angle", "front_wing_angle_deg"}, {"Rear wing angle", "rear_wing_angle_deg"},
        {"Front ride height", "front_ride_height_mm"}, {"Floor load", "floor_load_pct"},
        {"Ballast", "ballast_kg"},                     {"Front pressure", "tire_pressure_front_psi"},
        {"Rear pressure", "tire_pressure_rear_psi"},   {"Brake bias", "brake_bias_front_pct"},
    };
    for (const auto& [label, key] : numeric)
        if (labels.contains(label)) car["setup"][key] = std::stod(labels[label].get<std::string>());
    if (labels.contains("Power map")) car["setup"]["power_map"] = labels["Power map"];
    if (labels.contains("Tire compound")) {
        for (const auto& t : tires["tires"])
            if (t["short"] == labels["Tire compound"])
                car["components"]["tire"] = {{"name", t["name"]}, {"cost_usd", 2500}, {"mass_kg", t["mass_kg"]},
                                             {"properties", t["properties"]}};
    }
    return car;
}

const json kDefaultSettings = {
    {"regulation", "Apex 2026 Default"}, {"default_course", "Demo Circuit"}, {"time_step_ms", 5},
    {"telemetry_spacing_m", 5}, {"default_air_c", 24}, {"default_track_c", 38},
    {"reference_lap", "Previous run, same car and track"}, {"speed_unit", "km/h"}, {"temp_unit", "C"},
    {"autosave_runs", true}, {"keep_telemetry", "Last 50 runs"}, {"telemetry_in_exports", false},
};

void Seed(Database& db, const fs::path& data) {
    const json tires = ReadJson(data / "tire_catalog.json");
    const json course = ReadJson(data / "sample_track.json");
    const json car = ReadJson(data / "sample_vehicle.json");
    json history = ReadJson(data / "sample_history.json")["runs"];
    json run42 = ReadJson(data / "sample_run.json");

    db.Transaction([&] {
        db.SaveTireCatalog(tires);
        db.SaveCourse(course, /*is_template=*/true);  // built in, so read-only
        db.SaveCar(car);
        std::sort(history.begin(), history.end(), [](auto& a, auto& b) { return a["run_id"] < b["run_id"]; });
        for (auto& h : history) {  // oldest first
            if (h["run_id"] == run42["run_id"]) continue;  // the full result is loaded below
            json hcar = CarFromLabels(car, h["setup"], tires);
            db.SaveCar(hcar);
            db.SaveRun(h, hcar, {.keep_run_id = true});
        }
        run42["date"] = "2026-10-27";
        db.SaveRunForSavedCar(run42, {.keep_run_id = true});
        for (const auto& [k, v] : kDefaultSettings.items()) db.SetSetting(k, v);
    });
}

// ---- Commands -----------------------------------------------------------------

void Import(Database& db, const fs::path& file) {
    json j = ReadJson(file);
    if (j.contains("segments")) {
        db.SaveCourse(j);
        std::printf("Saved course \"%s\"\n", j["name"].get<std::string>().c_str());
    } else if (j.contains("tires")) {
        db.SaveTireCatalog(j);
        std::printf("Saved %zu tire compounds\n", j["tires"].size());
    } else if (j.contains("components")) {
        db.SaveCar(j);
        std::printf("Saved car \"%s\"\n", j["name"].get<std::string>().c_str());
    } else if (j.contains("lap_time_s")) {
        // A run exported by `export-run` carries its car snapshot; a fresh engine
        // result names a car that must already be saved.
        int64_t id = j.contains("car_snapshot") ? db.SaveRun(j, j["car_snapshot"]) : db.SaveRunForSavedCar(j);
        std::printf("Saved run #%lld\n", (long long)id);
    } else {
        throw apex::db::Error(file.string() + " doesn't look like a car, course, tire catalog or run file");
    }
}

void Demo(const fs::path& db_path) {
    fs::remove(db_path);
    Heading("1. Create the database");
    Database db(db_path.string(), Resource("schema.sql").string());
    std::printf("Created %s (schema v%d) from schema.sql.\n", db_path.string().c_str(), db.SchemaVersion());

    Heading("2. Load the app's sample data (data/*.json)");
    Seed(db, Resource("data"));
    std::printf("Loaded the tire catalog, Demo Circuit, %zu cars and %zu runs.\n", db.ListCars().size(),
                db.ListRuns().size());

    Heading("3. Saved cars and courses");
    PrintCars(db);
    std::printf("\n");
    PrintCourses(db);

    Heading("4. Run history (what the Compare screen lists), newest first");
    PrintRuns(db);

    Heading("5. Load one full run (what the Results screen needs)");
    PrintRun(db, 42);

    Heading("6. Compare two runs");
    PrintCompare(db, 41, 42);

    Heading("7. Best lap on the Demo Circuit");
    if (auto best = db.BestRun("Demo Circuit"))
        std::printf("Run #%lld, %s, %s\n", (long long)best->run_id, best->car_name.c_str(),
                    FormatLapTime(best->lap_time_s).c_str());

    Heading("8. Runs keep a snapshot of the car");
    json car = *db.LoadCar("Low Drag Monza Setup");
    car["setup"]["rear_wing_angle_deg"] = 10.0;
    db.SaveCar(car);
    std::printf("Changed the saved car's rear wing to 10 deg. Run #42 still shows what it ran with:\n");
    std::printf("  saved car: %s deg   run #42 snapshot: %s deg\n",
                db.LoadCar("Low Drag Monza Setup")->at("setup")["rear_wing_angle_deg"].dump().c_str(),
                db.LoadRun(42)->at("car_snapshot")["setup"]["rear_wing_angle_deg"].dump().c_str());
    db.DeleteCar("Baseline");
    std::printf("Deleted the car \"Baseline\". Run #37 is still in history as: %s\n",
                db.LoadRun(37)->at("vehicle").get<std::string>().c_str());

    Heading("9. Export and import JSON (portable cars and courses)");
    fs::path out = db_path.parent_path() / "exported_car.json";
    WriteJson(out, car);
    std::printf("Wrote %s\n", out.string().c_str());
    json copy = ReadJson(out);
    copy["name"] = "Low Drag Monza (copy)";
    db.SaveCar(copy);
    std::printf("Imported it back as \"Low Drag Monza (copy)\": %zu cars now.\n", db.ListCars().size());

    Heading("10. Settings and telemetry clean-up");
    db.SetSetting("speed_unit", "mph");
    std::printf("speed_unit = %s\n", db.GetSetting("speed_unit")->dump().c_str());
    int trimmed = db.TrimTelemetry(3);
    std::printf("Kept telemetry for the newest 3 runs and dropped it from %d older ones:\n", trimmed);
    PrintRuns(db);

    Heading("11. The database refuses bad data");
    auto refuse = [](const char* what, auto&& fn) {
        try {
            fn();
            std::printf("UNEXPECTED: %s was allowed\n", what);
        } catch (const apex::db::Error& e) {
            std::printf("Refused %s:\n  %s\n", what, e.what());
        }
    };
    refuse("a corner with no radius", [&] {
        json bad = *db.LoadCourse("Demo Circuit");
        bad["name"] = "Broken Circuit";
        bad["segments"][2].erase("radius_m");
        db.SaveCourse(bad);
    });
    refuse("editing the built-in Demo Circuit", [&] { db.SaveCourse(*db.LoadCourse("Demo Circuit")); });
    refuse("deleting a course that has runs", [&] { db.DeleteCourse("Demo Circuit"); });
    std::printf("Courses after all that (nothing half-written): %zu\n", db.ListCourses().size());

    std::printf("\nDone. Open %s in any SQLite viewer, or run `apexdb --db %s runs`.\n", db_path.string().c_str(),
                db_path.string().c_str());
}

int Usage() {
    std::printf(
        "Usage: apexdb [--db FILE] <command>\n"
        "  demo                     fresh database + a guided tour of every feature\n"
        "  init                     create an empty database\n"
        "  seed                     create a database loaded with the sample data\n"
        "  cars | courses | runs    list what is saved   (runs [COURSE])\n"
        "  show-run ID              one run in full\n"
        "  compare ID_A ID_B        lap, sector, segment and setup differences\n"
        "  import FILE.json         a car, course, tire catalog or run (detected from its keys)\n"
        "  export-car NAME FILE     export-course NAME FILE     export-run ID FILE\n"
        "  delete-car NAME          delete-course NAME          delete-run ID\n"
        "  settings                 print app settings\n"
        "Default database: apex_labs.db in the current folder (the demo uses apex_labs_demo.db).\n");
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    g_exe_dir = fs::absolute(argv[0]).parent_path();
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string db_path;
    if (args.size() >= 2 && args[0] == "--db") {
        db_path = args[1];
        args.erase(args.begin(), args.begin() + 2);
    }
    if (args.empty()) return Usage();
    const std::string cmd = args[0];
    auto arg = [&](size_t i) -> const std::string& {
        if (i >= args.size()) throw apex::db::Error("Missing argument for " + cmd + ". Run apexdb for help.");
        return args[i];
    };

    try {
        if (cmd == "demo") {
            Demo(db_path.empty() ? "apex_labs_demo.db" : db_path);
            return 0;
        }
        if (db_path.empty()) db_path = "apex_labs.db";
        if (cmd == "seed") fs::remove(db_path);
        Database db(db_path, Resource("schema.sql").string());

        if (cmd == "init") std::printf("Database ready: %s (schema v%d)\n", db_path.c_str(), db.SchemaVersion());
        else if (cmd == "seed") { Seed(db, Resource("data")); std::printf("Seeded %s\n", db_path.c_str()); }
        else if (cmd == "cars") PrintCars(db);
        else if (cmd == "courses") PrintCourses(db);
        else if (cmd == "runs") PrintRuns(db, {args.size() > 1 ? args[1] : "", "", 0});
        else if (cmd == "show-run") PrintRun(db, std::stoll(arg(1)));
        else if (cmd == "compare") PrintCompare(db, std::stoll(arg(1)), std::stoll(arg(2)));
        else if (cmd == "import") Import(db, arg(1));
        else if (cmd == "export-car") {
            auto v = db.LoadCar(arg(1));
            if (!v) throw apex::db::Error("No saved car named \"" + arg(1) + "\"");
            WriteJson(arg(2), *v);
        } else if (cmd == "export-course") {
            auto t = db.LoadCourse(arg(1));
            if (!t) throw apex::db::Error("No saved course named \"" + arg(1) + "\"");
            WriteJson(arg(2), *t);
        } else if (cmd == "export-run") {
            auto r = db.LoadRun(std::stoll(arg(1)));
            if (!r) throw apex::db::Error("No run #" + arg(1));
            WriteJson(arg(2), *r);
        } else if (cmd == "delete-car") std::printf(db.DeleteCar(arg(1)) ? "Deleted\n" : "No such car\n");
        else if (cmd == "delete-course") std::printf(db.DeleteCourse(arg(1)) ? "Deleted\n" : "No such course\n");
        else if (cmd == "delete-run") std::printf(db.DeleteRun(std::stoll(arg(1))) ? "Deleted\n" : "No such run\n");
        else if (cmd == "settings") std::printf("%s\n", db.AllSettings().dump(2).c_str());
        else return Usage();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
