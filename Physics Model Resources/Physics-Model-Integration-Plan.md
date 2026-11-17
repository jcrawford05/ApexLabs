# Physics Model: Integration Plan

**Status:** Draft v0.3 · 2026-11-17 · tracks Physics Model v0.10 · strategy: Python reference first, then C++ port (Option B)

[← Overview](Physics-Model-Overview) · [Core Model](Physics-Model-Core) · [Second-Tier Model](Physics-Model-Second-Tier) · [Sources](Physics-Model-Sources)

This page covers how the physics model (C1–C8, 3A/3B, T1–T8) becomes code and how it plugs into the Apex Labs desktop app (`desktop/`) and storage library (`database/`). It doesn't restate the equations. Each work item links to the module that defines them.

[[_TOC_]]

## 1. Where we start

| Area | Today | What physics needs to change |
|---|---|---|
| `desktop/` (SDL3 + ImGui, C++17) | All screens are migrated. `RUN SIMULATION` ([setup_screen.cpp:1369](../desktop/src/screens/setup_screen.cpp)) only navigates to Results with `MakeSampleRun()` (Run #42). | Call the solver and fill `App::run` with real output. |
| UI state structs | `Vehicle`, `Track`/`Segment`, `Weather`, `SurfaceSettings`, `SimulationSettings`, `Preferences` | These are the solver's inputs. The hidden constants (Overview, "Constants that are not in the UI") are missing. |
| `RunResult` / `Telemetry` | Sections, effects, sector times, and speed/throttle/brake/tire-temp telemetry at `spacing_m` | This is the solver's output shape. Keep it, and add channels only when needed. |
| Setup readouts | Hard-coded heuristics: `Vehicle::DragTendency/AeroBias`, top-gear limit, max decel (an *input* field), brake balance thresholds, rake, grip factor ([setup_screen.cpp:1100](../desktop/src/screens/setup_screen.cpp)), fuel weight cost, `kSampleLapTimeS` | Overview "Display values" says all of these must be model outputs. |
| `database/` (SQLite, C++20, nlohmann_json 3.11.3) | Schema v3. `simulation_runs`, `run_sectors`, `run_segment_results` and `telemetry_samples` already match `RunResult`. | Add the regulation-era and compound tables (3B), and persist solver runs. |
| `backgroundEDA/` (Python) | Historical F1 + weather EDA | Reuse its data-loading patterns. OpenF1 calibration itself lives in `physics/py/` (§5). |

## 2. Architecture

### 2.0 Language strategy: Python reference, C++ engine

Each module is built **twice, on purpose, in this order**:

1. **Python reference (`physics/py/apexphys`)**: the model is written, plotted and debugged here first. It's the executable form of the Core and Second-Tier pages, and it's where calibration happens. It's never shipped with the app.
2. **C++ engine (`physics/src`)**: a port of a module **after** its Python version is stable and tested. The desktop app only ever links C++, so there's no Python at runtime, no bundled interpreter and no per-call startup cost.

Three things keep the two in lock-step:

| Mechanism | What it does |
|---|---|
| **Shared parameter files** (`physics/data/*.json`) | Both implementations read the *same* JSON. Calibration writes it from Python; C++ only reads it. |
| **Golden test vectors** (`physics/golden/*.json`) | Python writes input → expected-output cases (per-module function values plus full-lap solutions). The C++ tests load the same files and must match within tolerance (§5). |
| **Same structure and names** | A Python module maps 1:1 to a C++ file (`aero.py` ↔ `aero.cpp`), with the same function names and SI units, so the port is mechanical and reviewable side by side. |

**Port rule:** a module is ported only when its Python version has passed unit and property tests and its equations are frozen for the current calibration round. When the equations change after porting, the change goes into Python first, golden files are regenerated, and C++ follows in the same MR, so the parity tests stay green on `main`.

**Python environment:** Python ≥ 3.11 with `numpy`, `scipy`, `pandas`, `pyarrow`, `matplotlib`, `requests` and `pytest`, pinned in `physics/py/pyproject.toml`. Plain Python loops are fine for the reference solver; there's no numba or any optimisation, because clarity matters more than speed there.

### 2.1 Repository layout (repo root, mirrors `database/`)

```
physics/
  py/                       # Python reference + calibration (never shipped)
    pyproject.toml
    apexphys/               # one module per model section, same names as the C++ headers
      units.py types.py mass.py aero.py load_transfer.py tire.py powertrain.py
      resistance.py brakes.py solver.py readouts.py atmosphere.py surface.py
      tire_thermal.py tire_wear.py brake_thermal.py balance.py ride_height.py energy.py
      regulation.py gearing.py params.py (JSON loader) cli.py (python -m apexphys ...)
    tests/
      unit/                 # closed-form checks (Core "Sanity checks", §3 hydroplaning)
      property/             # monotonicity and determinism
      validation/           # calibrated model vs held-out OpenF1 laps (pass/fail tolerances)
    openf1/                 # fetch + cache (parquet) + clean + lap/track extraction
    calibration/            # scipy.optimize fits -> physics/data/*.json + reports/
    tools/make_golden.py    # writes physics/golden/*.json
  golden/                   # shared golden vectors (Python writes, C++ reads)
  CMakeLists.txt            # apex_physics (static lib), apexsim (CLI), apex_physics_tests
  include/apex/physics/
    types.h                 # CarParams, TrackModel, Environment, SolverConfig, LapSolution
    units.h                 # psi<->Pa, gauge<->abs, km/h<->m/s, °C<->K, knots
    mass.h        C1        aero.h        C2        load_transfer.h  C3
    tire.h        C4        powertrain.h  C5        resistance.h     C6
    brakes.h      C7        solver.h      C8        readouts.h       (display values)
    atmosphere.h  T7        surface.h     T8 (incl. hydroplaning)
    tire_thermal.h T1       tire_wear.h   T2        brake_thermal.h  T3
    balance.h     T4        ride_height.h T5        energy.h         T6
    regulation.h  3B        gearing.h     3A (force envelope table)
  src/…                     # one .cpp per header
  src/io/params_json.cpp    # JSON -> parameter structs (only file that touches nlohmann_json)
  data/
    power_units.json        # torque curve, gear ratios i1..i8, eta_dl, inertias, r_w, shift loss
    tire_compounds.json     # supersedes database/data/tire_catalog.json; adds Inter/Wet + μ_ref, k_ls, p_opt, k_p, T_opt, w_T, k_wear, k_deg, wet params, groove depth
    aero_packages.json      # per-element A, C_L0, C_D0, ξ, ride-height curve, floor C_L min/max, k_f, stall params
    chassis.json            # m_base, driver mass, component x positions, track width, k_axle, roll split, C_α
    regulation_eras.json    # until the DB tables exist (3B)
    calibration.json        # surface/evolution/wet/brake/rolling-resistance [CAL] numbers in one place
  tests/                    # doctest (single header via FetchContent): unit tests + golden parity
```

The C++ library rules (the Python reference follows the same ones):

- **Pure C++17, no SDL/ImGui/SQLite.** It has to build into the desktop app (C++17) and run headless for tests and Python.
- **SI units inside, with conversions only at the edges.** `units.h` is the only place psi, km/h, knots or °C appear.
- **The solver is deterministic and stateless.** `LapSolution Solve(const CarParams&, const TrackModel&, const Environment&, const SolverConfig&, const RunState* carry_in)`. Multi-lap runs pass `RunState` (tire temp, wear, fuel, brake temp, track evolution) from lap to lap, as C8 describes.
- **Physics parameter structs are not the UI structs.** The UI keeps `Vehicle`/`Track`/…, and a thin adapter maps them. Physics then stays testable without the app, and UI-only fields (names, labels) don't leak in.
- **Every [CAL] number lives in `data/*.json`, never in code.** Calibration then means editing data, not recompiling.

### 2.2 Desktop integration

```
Setup screen state ──► desktop/src/sim_bridge.{h,cpp} ──► apex_physics::Solve ──► LapSolution
 (Vehicle, Track,        BuildCarParams / BuildTrackModel /                          │
  Weather, Surface,      BuildEnvironment / BuildSolverConfig                        ▼
  Simulation, Prefs)                                         sim_bridge: ToRunResult(LapSolution, reference)
                                                                                     │
                                                         App::run (sample_data = false) ──► Results / Compare / Import-Export
```

- `desktop/CMakeLists.txt`: add `add_subdirectory(../physics ${CMAKE_BINARY_DIR}/physics)`, then link `apex_physics`, and copy `physics/data` next to the exe (same pattern as `assets/`).
- `App` gets `PhysicsData physics_data` (loaded once in `App::Init`, with a clear error toast if a file is missing) and `bool RunSimulation()`.
- `RUN SIMULATION` and `Re-run simulation` ([results_screen.cpp:784](../desktop/src/screens/results_screen.cpp)) call `App::RunSimulation()`. That call assigns a new `run_id`, appends a `RunSummary` to `history`, and goes to Results. Remove the "physics engine isn't connected yet" toast.
- **Threading:** a single QSS lap takes milliseconds. Start synchronously. Move it to a `std::thread` worker with a progress state only when multi-lap and effects attribution (§4, phase C5) push the run past ~100 ms. The "about 3 s per run" target leaves plenty of headroom.
- **Live readouts:** the Setup screen calls `apex_physics::ComputeReadouts(car, env)` each frame, or caches it and recomputes only when inputs change. That call replaces every heuristic listed in §1. `brakes_max_decel_g` stops being an editable input and becomes a C7 output.

### 2.3 Mapping UI → physics inputs (bridge responsibilities)

| UI field | Physics input | Notes |
|---|---|---|
| `front_wing_deg`, `rear_wing_deg`, `front/rear_ride_height_mm`, `floor_load_pct`, `drag_area_m2` | C2 aero elements, T5 static ride heights, floor setting `s` | `drag_area_m2` becomes an output (`Σ C_D A`). It can't stay an input. |
| `pressure_front/rear_psi`, `compound` | C4 `g_pressure`, T1 cold gauge pressure, compound row | Convert gauge to absolute using `p_atm` from T7. |
| `power_map`, `final_drive`, `shift_rpm` | C5 `k_map`, `i_fd`, shift rpm; T6 fuel-flow scale | `engine_peak_kw` (745) becomes an output of the torque curve. The doc targets 723 kW; see §6. |
| `ballast_kg`, `ballast_front_pct`, `wheelbase_m`, `cg_height_m`, `*_mass_kg` | C1 parts list, positions, L, h | `chassis_mass_kg` (540) must be split into `m_base`, with the driver as a separate line (C1). |
| `brake_bias_front_pct`, `brake_cooling` | C7 β, T3 `h_cool`, C2 duct ΔC_D A | |
| `Weather` (air, track, humidity, wind, rain) | T7 ρ / p_atm / wind, T1 track temp, T8 water depth | Wind needs track heading. Leave it at 0 until the track has heading (see §6). |
| `SurfaceSettings` | T8 `g_state`, roughness, evolution on/off | |
| `SimulationSettings` (laps, start, fuel) | C8 flying/standing, multi-lap loop, C1/T6 start fuel | |
| `Preferences::time_step` | **C8 Δs** | Per C8, "1/5/10 ms" becomes "Solver resolution 1/2/5 m". Requires a Settings UI change. |
| `Preferences::telemetry_spacing_m` | Telemetry resample step | |
| `Preferences::regulation` | 3B era → C1 minimum mass, limits | |
| `Track.segments` | `TrackModel`: points every Δs with curvature κ = 1/R (0 on straights and braking zones), sector id, segment id | Optional later: smooth κ across segment joins so corner entry isn't a step. |

### 2.4 Mapping physics outputs → `RunResult`

| `RunResult` field | From |
|---|---|
| `lap_time_s`, `sector_times_s[3]` | Σ Δt over all points, and per sector |
| `top_speed_kmh`, `top_speed_at_m`, `top_speed_note` | max v and its distance; note = segment name |
| `sections[i].time_s / min_speed_kmh / peak_brake_g` | per-segment Σ Δt, min v, min a_x / g |
| `*_delta_s`, `telemetry.reference_speed_kmh` | Compared with the reference run (`reference_run_id`, from Settings `reference_lap` / history) |
| `telemetry.speed / throttle / brake / tire_temp` | Resampled at `spacing_m`. Throttle = F_drive used / F_drive available on power-limited points. Brake = \|a_x\| / a_brake,max on braking points. Tire temp comes from T1 (a constant placeholder until T1 lands). |
| `sections[i].effects`, `other_effects_s`, `tradeoff` | **Effects attribution** (phases P5 → C5): re-solve with one setting set back to the reference value and diff per-segment times. The remainder goes in `other_effects_s`. |
| `engine_version` | `apex_physics::kVersion` (feeds `simulation_runs.engine_version`) |

Extra channels the doc asks for (a_x, a_y, gear, rpm, downforce) go in a new `Telemetry` struct inside `LapSolution`. `RunResult::Telemetry` grows only when a screen needs them, which keeps Results/Compare/export unchanged for now.

## 3. Hydroplaning (T8) from Horne & Dreher (NASA TN D-2056)

T8 currently says "use a lookup table". The paper gives enough to make it concrete:

- **Total hydroplaning speed:** `V_p = 9 √p` knots ≈ `4.63 √p_psi` m/s (≈ `10.35 √p` mph), with p = inflation pressure. The paper is based on gauge psi, so use T1 hot gauge pressure.
- **This only applies when water depth exceeds tread groove depth** and a minimum film depth: ≈ 2.5–10 mm (0.1–0.4 in). It's lowest for smooth tires on smooth pavement and highest for grooved tires on open-textured surfaces. Model it as `d_eff = d_water − d_groove(compound)`, and require `d_eff > d_min(roughness)`.
- **Partial hydroplaning:** hydrodynamic lift scales with `v²`, so the supported-load fraction is ≈ `(v / V_p)²`. Grip on the remaining contact falls with it: `g_hydro(v) = max(μ_residual/μ, 1 − (v/V_p)²)`, where `μ_residual ≈ 0.05` (the free-rolling level the paper reports).
- **Thin-film (viscous) loss:** on smooth wet surfaces braking traction can vanish at ≈ 0.65–0.68 `V_p` even without standing water. Apply it as a separate smooth-surface/low-roughness term.
- **Fluid density:** `V_p ∝ ρ^(−1/2)`, so slush (SG 0.85) gives about +8 %. This is optional, for a future "standing water vs slush" option.
- **Solver consequence:** `g_wet` now depends on speed, so the C8 corner-limit fixed-point iteration and the forward/backward passes must evaluate μ(v), not a per-lap constant.
- **Unit tests:** at 22 psi, `V_p` ≈ 42 kn ≈ 78 km/h. That means a slick in standing water is undriveable above ≈ 80 km/h, which is plausible. Inter and wet tires avoid it through `d_groove`, not by changing `V_p`. Also test: at `d_water < d_min`, `g_hydro = 1`; at `v = V_p`, grip = residual.
- **Inputs we lack:** water depth. Map `rain_pct` (plus roughness and drainage) → `d_water` with a [CAL] table in `calibration.json`. A "standing water (mm)" input in the Weather panel can come later if wanted.

Also cite [26] NASA TM X-72650 (already in Sources) when implementing.

## 4. Phased work plan

Each phase is one GitLab feature branch and one MR into `main`, using the current naming (`feature/physics-…`). Each phase ships with its tests. Physics work follows the Overview's "Suggested build order", but runs in three parallel tracks:

- **Track P (Python reference)** builds each module first.
- **Track D (OpenF1 data and calibration)** starts at once, because it only needs data, not the model.
- **Track C (C++ port and desktop)** follows P one module group behind.

### 4.1 Track P: Python reference

| Phase | Branch | Scope | Done when |
|---|---|---|---|
| **P0. Scaffold** | `feature/physics-py-scaffold` | `physics/py` package, `pyproject.toml`, pytest; `units.py`, `types.py` (dataclasses mirroring §2.3/§2.4); `params.py` loader; first `physics/data/*.json` from current UI defaults; `python -m apexphys run --car … --track …` CLI | `pytest` green; CLI loads the Demo Circuit and sample car |
| **P1. Straight line** | `feature/physics-py-straight-line` | C1 mass/CG (bottom-up, era minimum, mandatory ballast), C2 constant coefficients + humid-air ρ, C6, C5 with placeholder gears and torque curve | Unit tests: terminal speed `F_drive = F_drag + F_rr`; 0–300 km/h time; top-gear limit; rake 0.75°; mass totals. Plots of the force envelope and the acceleration run |
| **P2. End-to-end lap** | `feature/physics-py-qss-solver` | C4 constant μ + friction ellipse; C8 corner limit + three-pass solver; flying/standing start; track discretisation; telemetry resample; `tools/make_golden.py` | Tests: `v = √(μgR)` with no aero; aero-unlimited corner; braking `a = μg`; energy check; property tests. Demo Circuit lap + first golden files checked in |
| **P3. Load transfer & braking** | `feature/physics-py-brakes-load` | C3, C7 (β limits, ideal bias, axle-load iteration); C2 ride-height curve and floor scheme; C4 load sensitivity and `g_pressure` | Front/rear lock tests; max decel and brake balance are outputs; golden files regenerated |
| **P4. Second tier** | one branch per module | T6 fuel → T1/T2 tires → T7 atmosphere + T8 surface/evolution/**hydroplaning (§3)** → T3 brakes → T4/T5 balance and dynamic ride height; multi-lap state carry-over | Module tests; multi-lap trends; slick-in-rain vs wet tire emerges without special cases (3B) |
| **P5. Effects attribution** | `feature/physics-py-effects` | Per-setting re-solve and per-segment diff → `effects`, `other_effects_s`, `tradeoff` | Effects sum (plus remainder) matches the total delta to the reference run |

### 4.2 Track D: OpenF1 data and calibration (details in §5)

| Phase | Branch | Scope | Done when |
|---|---|---|---|
| **D0. Data cache** | `feature/physics-openf1-cache` | `openf1/` fetch with on-disk parquet cache, session/lap selection, clean-lap filters | One command builds a local dataset for a chosen season list, and re-runs hit the cache only |
| **D1. Track extraction** | `feature/physics-openf1-tracks` | `location` x/y/z → smoothed curvature, heading, elevation, sector split → track JSON (with `direction`, `elevation_m`) | Extracted track length within 1 % of the official length for each circuit; curvature plots reviewed |
| **D2. Reference laps** | `feature/physics-openf1-laps` | `car_data` + `laps` → speed-vs-distance traces for clean quali/fastest laps, aligned on the extracted track | A reference trace set per circuit per era, with weather attached |
| **D3. Calibration** | `feature/physics-calibration` (after P2) | `calibration/` fits (§5.3) writing `physics/data/*.json` + `calibration/reports/` | Each fitted parameter has a report; held-out circuit error is within the D8 tolerances |
| **D4. Validation gate** | (part of D3) | `tests/validation` runs the calibrated model on held-out circuits | It runs in CI or before each calibration MR, and a failure blocks the merge |

### 4.3 Track C: C++ port and desktop integration

| Phase | Branch | Scope | Done when |
|---|---|---|---|
| **C0. Scaffold** | `feature/physics-cpp-scaffold` | `physics/` CMake (lib + `apexsim` + doctest/CTest); `types.h`, `units.h`; JSON loader; golden-file loader and parity harness | `ctest` passes on Windows/MSVC; the harness reads the P0 golden files |
| **C1. Port P1 + P2** | `feature/physics-cpp-core` | Straight line and the QSS solver | **Golden parity green** (§5.1); `apexsim` Demo Circuit lap matches Python |
| **C2. Desktop hookup** | `feature/physics-desktop-bridge` | `sim_bridge`, `App::RunSimulation`, Results/Compare from real runs, history append, Settings "Solver resolution", Setup readouts from `ComputeReadouts` | RUN SIMULATION shows a non-sample run; a wing change moves lap and section times; no hard-coded readouts left |
| **C3. Port P3** | `feature/physics-cpp-brakes-load` | C3, C7 and calibrated C2/C4 | Golden parity green |
| **C4. Port P4** | one branch per module | Second tier, multi-lap | Golden parity green per module |
| **C5. Port P5 + data** | `feature/physics-effects`, `feature/physics-persistence` | Effects attribution; reference deltas; persist runs through `apex_db`; schema v4 (`regulation_era`, `regulation_rule`, `tire_compound`, track direction/elevation); car/track JSON import/export | Results popups show computed effects; runs survive restart; era switch changes limits |
| **Later** | | 3A Gear Ratio Lab UI; Pacejka behind the C4 interface (Python first, as always) | |

### 4.4 Ordering

```
P0 ─ P1 ─ P2 ─ P3 ─ P4 ─ P5
 │         │    │    │    │
 │         ▼    ▼    ▼    ▼        (each arrow = port after the Python module is frozen)
C0 ─────── C1 ─ C2 ─ C3 ─ C4 ─ C5
D0 ─ D1 ─ D2 ───── D3 (needs P2) ─ D4 ──► feeds physics/data/*.json to both P and C
```

**Critical path to a working app:** P0 → P1 → P2 → C1 → C2. D0–D2 run alongside from day one, so calibration can start the moment P2 produces a lap. After C2, the GUI picks up each new module as soon as its C++ port passes parity.

## 5. Testing, calibration and validation

### 5.1 Test tiers

| Tier | Lives in | Runs | Purpose |
|---|---|---|---|
| **1. Unit (analytic)** | `physics/py/tests/unit` | every change | Closed-form checks from the Core page ("Sanity checks") and §3 hydroplaning, with explicit tolerances |
| **2. Property** | `physics/py/tests/property` | every change | Monotonicity: more rear wing → lower top speed and higher corner speed; more fuel → slower lap; more rain → slower lap; higher pressure → higher `V_p`. Also determinism: same inputs give identical output |
| **3. Golden parity** | `physics/golden/` + `physics/tests` (C++) | every C++ change, and whenever golden files are regenerated | C++ matches Python. Per-function vectors match to relative 1e-9 (double precision, same operation order). Full-lap telemetry matches to 1e-6 relative and lap time to 1e-4 s. Any larger gap means the port diverged and needs investigating, not a loosened tolerance |
| **4. Regression** | `physics/golden/demo_circuit_*.json` | every change | The Demo Circuit lap with `sample_vehicle.json` is stored. A calibration or model change updates it on purpose, and the MR states the delta |
| **5. Validation (data)** | `physics/py/tests/validation` | before every calibration MR | The calibrated model against **held-out** OpenF1 circuits, with pass/fail thresholds (D8). This is the merge gate for `physics/data/*.json` changes |
| **Perf (C++ only)** | `physics/tests` benchmark | release builds | One lap at Δs = 1 m under 10 ms; Python has no perf target |

### 5.2 OpenF1 data pipeline (`physics/py/openf1/`)

- **Fetch and cache:** pull `sessions`, `laps`, `car_data`, `location`, `stints`, `weather` per session into local parquet (`physics/py/.cache/`, git-ignored). Tests and fits read only the cache. API rate limits and outages then never break a test run.
- **Clean-lap filter:** keep qualifying laps and the fastest clean race laps. Drop in/out laps, safety-car/VSC/yellow-flag laps, laps with traffic gaps under ~1.5 s, and laps with deleted times. The goal is laps where the car, not the situation, set the speed.
- **Track extraction (fixes the §6.5 gap):** `location` x/y/z of a clean lap → resample by distance → smooth (e.g. Savitzky–Golay or a spline) → curvature κ(s), heading ψ(s), elevation z(s) → segment into straight/braking/corner with radius and direction. Sectors come from `laps` sector times. Output is the same track JSON the solver reads, so real circuits become selectable tracks in the app.
- **Reference traces:** `car_data` speed (≈ 3.7 Hz) mapped onto the extracted distance axis. Store speed, throttle, brake, gear and rpm per lap, with that session's `weather`.

### 5.3 Calibration (parameter identification, not ML)

The model stays physics. Calibration only picks values for [CAL] numbers so simulated laps match measured ones, which keeps every "why" explanation in the app valid.

- **Fit few, fix many.** Fix what regulations or physics give (mass minimum, g, air properties, wheelbase). Fit a small set of *lumped* parameters per era and compound, starting with: `C_D A`, `C_L A`, aero balance `ξ`, engine power scale, `μ_ref`, `k_ls`, `C_rr`, then `k_deg` / cliff and the T1 thermal constants once those modules exist.
- **Loss function** (weights tuned per stage): distance-aligned speed-trace RMSE + minimum-corner-speed error + top-speed error + sector-time error, plus a prior/regularisation term that pulls each parameter toward its literature value from the Sources page.
- **Staged fits**, so parameters don't trade off against each other:
  1. Straights only: `C_D A`, power scale, `C_rr` from top speed and the acceleration shape.
  2. Corners: `μ_ref`, `C_L A`, `k_ls` from minimum speeds in slow vs fast corners. Slow corners isolate grip and fast corners isolate downforce, which is how the two get separated.
  3. Braking: C7 from deceleration slopes into the heavy-braking zones.
  4. Stints: `k_deg` / cliff from the lap-time trend after removing the fuel-burn effect (T6). Fit fuel and degradation together, or they are confounded.
  5. Weather and wet: T1/T7 sensitivity across sessions at different temperatures, and T8 wet grip from the few wet sessions. Flag wet values as low-confidence.
- **Optimisers:** `scipy.optimize.least_squares` (bounded, smooth stages) and `differential_evolution` where the loss is bumpy (stall, cliffs).
- **Generalisation:** split circuits into fit and held-out sets, such as leave-one-circuit-out or a fixed hold-out of 3–4 varied tracks (one high-speed, one street, one high-altitude). A model that only fits its training tracks has memorised them. Report held-out error first.
- **Eras:** fit each regulation era on its own seasons (2026 cars on 2026 data, 2023–25 data only for the matching older era tables in 3B).
- **Outputs:** fitted values written to `physics/data/*.json` (with a `calibration` block naming the dataset, date and git commit used), plus `calibration/reports/<param>.md` with fit data, residuals, held-out error and plots. Reports feed the verification table on the Sources page.

### 5.4 Known limits of the data

- **Setups are unknown** (wing angles, pressures, fuel load), so fitted values are *effective* values for a typical setup, not the true car. The sensitivities to UI settings still come from the physics, not the fit.
- **Driver and traffic** remain in even the clean laps. Compare trace shapes and deltas, not just absolute lap times (Overview, validation plan).
- **About 3.7 Hz `car_data`** can't pin exact braking points. Use deceleration slopes and minimum speeds instead.
- **Wet data is scarce.** T8 calibration will rest partly on Horne & Dreher (§3) and [26] rather than OpenF1.

## 6. Gaps and conflicts found between the docs and the code

These need a decision or a data fix. None blocks P0–P2 or C0.

1. **Wheelbase:** *Resolved (D3):* `chassis.json` uses the 2026 limit, 3.40 m (C2.3.3). The desktop UI default (3.60 m in `vehicle.h` and `sample_vehicle.json`) still has to follow.
2. **Peak power:** *Resolved (D3):* `power_units.json` uses 723 kW. That is 350 kW from the ERS-K (C5.2.7) plus 373 kW from the engine, which is the 3000 MJ/h fuel energy flow limit (C5.2.3, 833 kW) at 44.8 % thermal efficiency. The desktop UI still shows 745 kW.
3. **Mass breakdown:** *Resolved (D2):* `chassis.json` has `m_base_kg` = 540 and a separate 82 kg driver. The stock car's dry parts then total 881 kg (540 + 82 + 150 + 46 + 40 + 18 + 5 kg ballast), about 111 kg over the era minimum of 724 kg plus the tyre mass. C1 carries that as `overweight`.
4. **Inputs that become outputs:** `drag_area_m2`, `brakes_max_decel_g` and `engine_peak_kw` are editable or stored inputs in `Vehicle` today. Make them read-only readouts in the UI.
5. **Track data is too thin for T7 wind and C6 grade:** no corner direction, heading, elevation profile or `h_track`. Add `direction` (L/R) and optional `elevation_m` per segment and `elevation_m` per course to the track JSON and `course_segments` (schema v4). D1 (§5.2) fills these from OpenF1 `location` for real circuits. The Demo Circuit needs them added by hand, and until then its wind and grade are 0.
6. **Settings "Physics time step"** needs to become "Solver resolution" (C8 decision). The UI label and the meaning of `prefs.time_step` both change.
7. **Floor slider:** the "Floor aggressiveness" rename and stall-margin readout are pending approval (still-open question 3). The solver can implement the physics now and the UI can follow.
8. **Compounds:** the code has only Soft/Medium/Hard (`TireCatalog()` is hard-coded in `vehicle.cpp`). The 3B compound table needs Inter/Wet and has to load from data, so the Tires panel should read it from `PhysicsData`.
9. **License:** OpenLAP [9] is GPL v3. Read it for ideas but copy no code.
10. **Unverified sources:** do not cite Sources entries still marked Partly/Not found in reports until they're checked. The code itself only needs the equation forms, which are standard.
11. **Power unit mass:** the UI power unit is 150 kg, below the 185 kg minimum overall power unit mass (C5.5.2). Found while filling the era table; needs a decision.
12. **Fuel LHV:** T6 quotes 43–44 MJ/kg, but C16.2 limits the 2026 fuel to 38.0–41.0 MJ/kg. Use the era value in T6.

## 7. Decisions the team needs to make

| # | Decision | Needed by |
|---|---|---|
| D1 | ~~Default regulation era and its rule values~~ **Decided:** "Apex 2026 Default", with rule values read from FIA Section C Issue 21 (2026-10-19) into `physics/data/regulation_eras.json` | P1 (C1 minimum mass) |
| D2 | ~~Default `m_base`~~ **Decided:** `m_base` = 540 kg. Still open: "Car mass" shown with or without fuel (the FIA Car Mass is without fuel) | P1 |
| D3 | ~~Peak power 723 vs 745 kW; wheelbase default~~ **Decided:** follow the 2026 regulations: 723 kW and 3.40 m | P1 |
| D4 | C++ test framework (proposed: doctest); whether `physics/` joins a top-level CMake superproject or stays `add_subdirectory` from `desktop/`; Python version and dependency pins | P0 / C0 |
| D5 | Floor-setting scheme approval (still-open 3) | P3 (physics), C3 (UI) |
| D6 | Water-depth input: derived from rain only, or an explicit "standing water" slider | P4 (T8) |
| D7 | Inter/Wet compound parameters (still-open 4); sporting-rule enforcement strict/advisory/off (still-open 5) | P4 / C5 |
| D8 | Validation tolerances (e.g. top speed ±3 %, sector-time shape correlation, held-out circuit set) | D3 |
| D9 | Who owns which track: split Python reference (P), OpenF1 data and calibration (D), and C++ port and desktop (C) across the team | P0 |
| D10 | Which seasons and sessions form the calibration dataset (2026 only for the 2026 era, or include older eras now) | D0 |

## Revision history

- v0.3: recorded decisions D1–D3 (2026 default era with rule values from FIA Section C Issue 21, `m_base` 540 kg, 723 kW and 3.40 m wheelbase); added gaps 11 and 12.
- v0.2: adopted Option B. The Python reference (`physics/py`) is built first and the C++ engine is a parity-tested port. Work splits into three tracks (P, D, C). Added golden-file parity tests, the OpenF1 data pipeline, staged calibration with held-out circuits, and data limits.
- v0.1: initial plan covering architecture, UI/physics mapping, hydroplaning (T8), phases, gaps and decisions.
