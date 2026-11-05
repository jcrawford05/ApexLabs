-- Apex Labs database schema (v3): the team spreadsheet "Apex_Labs_Database_Schema.xlsx",
-- written for SQLite (the database ships inside the app as one file).
--
-- Type mapping from the spreadsheet's MySQL notation:
--   BIGINT UNSIGNED AUTO_INCREMENT PK -> INTEGER PRIMARY KEY (SQLite assigns ids automatically)
--   VARCHAR(n), TEXT, ENUM(...)       -> TEXT (ENUMs become CHECK (... IN (...)))
--   DECIMAL(p,s), INT, TINYINT, ...   -> REAL / INTEGER, with CHECKs for "unsigned"
--   BOOLEAN                           -> INTEGER CHECK (x IN (0, 1))
--   JSON                              -> TEXT CHECK (json_valid(x))
--   DATETIME                          -> TEXT 'YYYY-MM-DD HH:MM:SS' (UTC), DEFAULT CURRENT_TIMESTAMP
--
-- Choices the spreadsheet leaves open are marked "-- added:" below.
-- Every connection must run `PRAGMA foreign_keys = ON;` (SQLite leaves it off by default).

PRAGMA user_version = 3;

-- ---------------------------------------------------------------------------
CREATE TABLE regulations (
    regulation_id     INTEGER PRIMARY KEY,
    name              TEXT NOT NULL UNIQUE,                                    -- Apex 2026 Default
    description       TEXT,
    budget_limit_usd  REAL CHECK (budget_limit_usd IS NULL OR budget_limit_usd >= 0),  -- max total component cost
    mass_limit_kg     REAL CHECK (mass_limit_kg IS NULL OR mass_limit_kg >= 0),        -- max vehicle mass
    created_at        TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at        TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ---------------------------------------------------------------------------
CREATE TABLE courses (
    course_id           INTEGER PRIMARY KEY,
    name                TEXT NOT NULL,
    country             TEXT,                                                  -- NULL for fictional circuits
    openf1_circuit_key  INTEGER UNIQUE CHECK (openf1_circuit_key IS NULL OR openf1_circuit_key >= 0),
    source              TEXT,                                                  -- user, openf1, built-in
    is_template         INTEGER NOT NULL DEFAULT 0 CHECK (is_template IN (0, 1)),
    is_read_only        INTEGER NOT NULL DEFAULT 0 CHECK (is_read_only IN (0, 1)),
    created_at          TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at          TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE course_segments (
    segment_id       INTEGER PRIMARY KEY,
    course_id        INTEGER NOT NULL REFERENCES courses(course_id) ON DELETE CASCADE,
    sequence_number  INTEGER NOT NULL CHECK (sequence_number >= 1),
    name             TEXT,                                                     -- T1 Hairpin
    segment_type     TEXT NOT NULL CHECK (segment_type IN ('straight', 'braking', 'corner')),
    sector_number    INTEGER NOT NULL CHECK (sector_number BETWEEN 1 AND 255),
    length_m         REAL NOT NULL CHECK (length_m > 0),
    radius_m         REAL CHECK (radius_m IS NULL OR radius_m > 0),            -- used for corners
    created_at       TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (course_id, sequence_number),
    CHECK (segment_type <> 'corner' OR radius_m IS NOT NULL)                  -- added: a corner needs a radius
);

-- ---------------------------------------------------------------------------
CREATE TABLE component_types (
    component_type_id  INTEGER PRIMARY KEY,
    name               TEXT NOT NULL UNIQUE,                                   -- Aero, Engine, Tire, Chassis, Brakes
    description        TEXT
);

INSERT INTO component_types (name, description) VALUES
    ('Aero',    'Wings and floor package'),
    ('Engine',  'Power unit'),
    ('Tire',    'Tire compound'),
    ('Chassis', 'Monocoque and suspension geometry'),
    ('Brakes',  'Brake discs and calipers');

CREATE TABLE components (
    component_id       INTEGER PRIMARY KEY,
    component_type_id  INTEGER NOT NULL REFERENCES component_types(component_type_id),
    name               TEXT NOT NULL,                                          -- V6 Hybrid Spec B
    cost_usd           REAL NOT NULL DEFAULT 0 CHECK (cost_usd >= 0),
    mass_kg            REAL NOT NULL DEFAULT 0 CHECK (mass_kg >= 0),
    properties_json    TEXT NOT NULL DEFAULT '{}' CHECK (json_valid(properties_json)),
    is_template        INTEGER NOT NULL DEFAULT 0 CHECK (is_template IN (0, 1)),
    is_available       INTEGER NOT NULL DEFAULT 1 CHECK (is_available IN (0, 1)),
    created_at         TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (component_type_id, name)                                           -- added: car files name their parts
);

-- ---------------------------------------------------------------------------
CREATE TABLE cars (
    car_id         INTEGER PRIMARY KEY,
    regulation_id  INTEGER REFERENCES regulations(regulation_id) ON DELETE SET NULL,
    name           TEXT NOT NULL,
    description    TEXT,
    setup_json     TEXT NOT NULL DEFAULT '{}' CHECK (json_valid(setup_json)),  -- added: wing angles, pressures, etc.
    is_template    INTEGER NOT NULL DEFAULT 0 CHECK (is_template IN (0, 1)),
    is_read_only   INTEGER NOT NULL DEFAULT 0 CHECK (is_read_only IN (0, 1)),
    created_at     TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at     TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE car_components (
    car_id             INTEGER NOT NULL REFERENCES cars(car_id) ON DELETE CASCADE,
    component_type_id  INTEGER NOT NULL REFERENCES component_types(component_type_id),
    component_id       INTEGER NOT NULL REFERENCES components(component_id) ON DELETE RESTRICT,
    installed_at       TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (car_id, component_type_id)
);

-- added: the installed part must belong to the slot it is installed in.
CREATE TRIGGER car_components_type_insert BEFORE INSERT ON car_components
FOR EACH ROW WHEN (SELECT component_type_id FROM components WHERE component_id = NEW.component_id)
                  IS NOT NEW.component_type_id
BEGIN
    SELECT RAISE(ABORT, 'component type does not match the car_components slot');
END;

CREATE TRIGGER car_components_type_update BEFORE UPDATE ON car_components
FOR EACH ROW WHEN (SELECT component_type_id FROM components WHERE component_id = NEW.component_id)
                  IS NOT NEW.component_type_id
BEGIN
    SELECT RAISE(ABORT, 'component type does not match the car_components slot');
END;

-- ---------------------------------------------------------------------------
-- A run's car may later be edited or deleted; car_setup_snapshot keeps exactly
-- what it ran with. Courses are not snapshotted, so a course with runs can't be
-- deleted (its segment results point at its segments).
CREATE TABLE simulation_runs (
    run_id              INTEGER PRIMARY KEY,
    car_id              INTEGER REFERENCES cars(car_id) ON DELETE SET NULL,
    course_id           INTEGER NOT NULL REFERENCES courses(course_id) ON DELETE RESTRICT,
    regulation_id       INTEGER REFERENCES regulations(regulation_id) ON DELETE SET NULL,
    engine_version      TEXT NOT NULL,
    weather_preset      TEXT NOT NULL DEFAULT 'dry' CHECK (weather_preset IN ('dry', 'overcast', 'light_rain', 'wet')),
    air_temp_c          REAL,
    track_temp_c        REAL,
    humidity_pct        REAL CHECK (humidity_pct IS NULL OR humidity_pct BETWEEN 0 AND 100),
    wind_kmh            REAL CHECK (wind_kmh IS NULL OR wind_kmh >= 0),
    rain_pct            REAL CHECK (rain_pct IS NULL OR rain_pct BETWEEN 0 AND 100),
    lap_time_s          REAL NOT NULL CHECK (lap_time_s > 0),
    top_speed_kmh       REAL CHECK (top_speed_kmh IS NULL OR top_speed_kmh >= 0),
    top_speed_at_m      REAL CHECK (top_speed_at_m IS NULL OR top_speed_at_m >= 0),
    car_setup_snapshot  TEXT NOT NULL CHECK (json_valid(car_setup_snapshot)),
    created_at          TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_runs_course_time ON simulation_runs (course_id, lap_time_s);
CREATE INDEX idx_runs_created     ON simulation_runs (created_at);

CREATE TABLE run_sectors (
    run_sector_id  INTEGER PRIMARY KEY,
    run_id         INTEGER NOT NULL REFERENCES simulation_runs(run_id) ON DELETE CASCADE,
    sector_number  INTEGER NOT NULL CHECK (sector_number BETWEEN 1 AND 255),
    time_s         REAL NOT NULL CHECK (time_s > 0),
    delta_s        REAL,                                                       -- versus the reference run
    UNIQUE (run_id, sector_number)
);

CREATE TABLE run_segment_results (
    run_segment_result_id  INTEGER PRIMARY KEY,
    run_id                 INTEGER NOT NULL REFERENCES simulation_runs(run_id) ON DELETE CASCADE,
    segment_id             INTEGER NOT NULL REFERENCES course_segments(segment_id) ON DELETE RESTRICT,
    time_s                 REAL NOT NULL CHECK (time_s > 0),
    delta_s                REAL,
    min_speed_kmh          REAL,
    peak_brake_g           REAL,
    tradeoff_note          TEXT,
    effects_json           TEXT CHECK (effects_json IS NULL OR json_valid(effects_json)),
    UNIQUE (run_id, segment_id)                                                -- added
);

CREATE TABLE telemetry_samples (
    telemetry_sample_id  INTEGER PRIMARY KEY,
    run_id               INTEGER NOT NULL REFERENCES simulation_runs(run_id) ON DELETE CASCADE,
    sample_index         INTEGER NOT NULL CHECK (sample_index >= 0),
    distance_m           REAL NOT NULL CHECK (distance_m >= 0),
    speed_kmh            REAL NOT NULL CHECK (speed_kmh >= 0),
    reference_speed_kmh  REAL,
    throttle_pct         REAL CHECK (throttle_pct IS NULL OR throttle_pct BETWEEN 0 AND 100),
    brake_pct            REAL CHECK (brake_pct IS NULL OR brake_pct BETWEEN 0 AND 100),
    tire_temp_c          REAL,
    UNIQUE (run_id, sample_index)
);

-- ---------------------------------------------------------------------------
CREATE TABLE app_settings (
    setting_key    TEXT PRIMARY KEY,                                           -- default_course_id
    setting_value  TEXT NOT NULL CHECK (json_valid(setting_value)),
    updated_at     TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ---------------------------------------------------------------------------
-- updated_at maintenance (MySQL's ON UPDATE CURRENT_TIMESTAMP).
CREATE TRIGGER regulations_touch AFTER UPDATE ON regulations FOR EACH ROW WHEN NEW.updated_at = OLD.updated_at
BEGIN UPDATE regulations SET updated_at = CURRENT_TIMESTAMP WHERE regulation_id = NEW.regulation_id; END;

CREATE TRIGGER courses_touch AFTER UPDATE ON courses FOR EACH ROW WHEN NEW.updated_at = OLD.updated_at
BEGIN UPDATE courses SET updated_at = CURRENT_TIMESTAMP WHERE course_id = NEW.course_id; END;

CREATE TRIGGER cars_touch AFTER UPDATE ON cars FOR EACH ROW WHEN NEW.updated_at = OLD.updated_at
BEGIN UPDATE cars SET updated_at = CURRENT_TIMESTAMP WHERE car_id = NEW.car_id; END;
