# Physics Model: Overview

**Status:** Draft v0.10 · 2026-11-16

[[_TOC_]]

## Purpose and method

The Apex Labs physics model turns the Car Setup and Track Setup inputs into speed, braking, sector-time, lap-time and telemetry outputs. It is a **quasi-steady-state (QSS) lap-time simulation** (distance-based, no transient time-domain integrator), as described in the Design Document: the track is a sequence of straights, braking zones and corners; the engine is C++ with Python used for validation.

Every quantity exposed in the UI (Aerodynamics, Tires, Powertrain, Chassis, Brakes, Weather, Surface, Simulation) is covered. Track geometry (segments, radii, elevation) is specified separately.

> **Citation note.** Citations are numbered [n] and listed on the [Sources page](Physics-Model-Sources). They are textbooks, journal papers and public sources. Chapter and page numbers are intentionally omitted until the team verifies each source; treat every entry as **to be verified** before it goes into a report (see the verification table on the Sources page).

> **Honesty note.** Some quantities (aero coefficients vs. wing angle, compound grip, wear rate) cannot be derived from first principles. The *form* of each equation is physically grounded and cited, but the *numbers* must be calibrated or taken from regulations and validation data. These are flagged **[CAL]**.

> **Rendering note.** Math is written in GitLab's inline form (dollar sign, backtick, formula, backtick, dollar sign). GitLab limits math rendering per page (1000 characters per math block, 2000 ms total), so the model is split across pages and tiny prose symbols are plain text. Edit in the plain text (Markdown) editor, not the rich text editor, which escapes backslashes and underscores.

## Where things live

| Page | Contents |
|---|---|
| **Overview** (this page) | scope, symbols, UI-to-physics map, module index, constants, display values, build order, validation plan, open decisions |
| [Core Model](Physics-Model-Core) | C1 to C8 and unit-test checks: everything needed for the first end-to-end prototype |
| [Second-Tier Model](Physics-Model-Second-Tier) | Gear Ratio Lab, regulation eras, T1 to T8: the realism and "why" layer |
| [Sources](Physics-Model-Sources) | references [1] to [27] and the citation verification table |

## Module index

| Module | What it covers |
|---|---|
| [C1. Mass, CG and weight distribution](Physics-Model-Core#c1-mass-cg-and-weight-distribution) | Bottom-up mass and CG from the selected parts, ballast, regulatory minimum mass, static axle loads |
| [C2. Aerodynamics](Physics-Model-Core#c2-aerodynamics) | Downforce and drag from wings and the underbody floor, ground effect, humid-air density, proposed floor setting |
| [C3. Longitudinal load transfer and axle loads](Physics-Model-Core#c3-longitudinal-load-transfer-and-axle-loads) | Longitudinal load transfer and axle loads with aero |
| [C4. Tires: grip model](Physics-Model-Core#c4-tires-grip-model) | Tire grip: friction ellipse, load sensitivity, pressure factor, Magic Formula upgrade path |
| [C5. Powertrain and longitudinal drive force](Physics-Model-Core#c5-powertrain-and-longitudinal-drive-force) | Engine torque and power, gearing, shift logic, traction limit, effective mass |
| [C6. Resistive forces](Physics-Model-Core#c6-resistive-forces) | Rolling resistance, grade, and the longitudinal equation of motion |
| [C7. Braking](Physics-Model-Core#c7-braking) | Brake bias, axle limits, brake balance, maximum deceleration |
| [C8. Cornering limit and the speed-profile solver](Physics-Model-Core#c8-cornering-limit-and-the-speed-profile-solver) | Cornering speed limit and the three-pass QSS speed-profile solver (QSS only) |
| [3A. Growth feature: Gear Ratio Lab (hidden in v1, planned UI)](Physics-Model-Second-Tier#3a-growth-feature-gear-ratio-lab-hidden-in-v1-planned-ui) | Gear Ratio Lab: planned power, tractive-force, rpm and ratio-spacing plots |
| [3B. Regulation eras and free tire choice](Physics-Model-Second-Tier#3b-regulation-eras-and-free-tire-choice) | Regulation eras (Settings table) and free tire choice in any weather |
| [T1. Tire thermal model, warm-up and hot pressure](Physics-Model-Second-Tier#t1-tire-thermal-model-warm-up-and-hot-pressure) | Tire thermal model, warm-up, psi gauge to absolute and hot pressure |
| [T2. Tire wear and degradation](Physics-Model-Second-Tier#t2-tire-wear-and-degradation) | Tire wear and degradation |
| [T3. Brake thermal model and cooling ducts](Physics-Model-Second-Tier#t3-brake-thermal-model-and-cooling-ducts) | Brake thermal model and cooling ducts |
| [T4. Lateral load transfer and handling balance](Physics-Model-Second-Tier#t4-lateral-load-transfer-and-handling-balance) | Lateral load transfer and handling balance |
| [T5. Ride height, rake and aero coupling](Physics-Model-Second-Tier#t5-ride-height-rake-and-aero-coupling) | Ride height, rake and aero coupling (floor stall) |
| [T6. Fuel burn, energy and hybrid deployment](Physics-Model-Second-Tier#t6-fuel-burn-energy-and-hybrid-deployment) | Fuel burn, energy and hybrid deployment |
| [T7. Weather, wind and air properties](Physics-Model-Second-Tier#t7-weather-wind-and-air-properties) | Weather, wind, elevation and air properties |
| [T8. Surface, evolution and wet grip](Physics-Model-Second-Tier#t8-surface-evolution-and-wet-grip) | Track surface, evolution, roughness and wet grip |

## Conventions and symbols

| Symbol | Meaning | Unit |
|---|---|---|
| $`m`$ | total mass (chassis + engine + tires + brakes + ballast + fuel + driver) | kg |
| $`g`$ | gravitational acceleration, 9.80665 | m/s² |
| $`v`$ | vehicle speed (ground) | m/s |
| $`\rho`$ | air density | kg/m³ |
| $`A`$ | aero reference area | m² |
| $`C_L,\ C_D`$ | downforce and drag coefficients ($`C_L`$ positive = downforce) | – |
| $`L`$ | wheelbase (3.60 m in UI) | m |
| $`a,\ b`$ | CG distance to front / rear axle ($`a+b=L`$) | m |
| $`h`$ | CG height (0.30 m in UI) | m |
| $`r_w`$ | effective tire rolling radius | m |
| $`\mu`$ | tire friction coefficient | – |
| $`F_z`$ | normal load on a tire/axle | N |
| $`\kappa`$ | path curvature ($`1/R`$) | 1/m |
| $`\Delta s`$ | distance step along the track | m |

Unit rules: the engine works in SI internally. Pressures are converted from psi to Pa and gauge to **absolute** before any gas-law use. Temperatures are converted to kelvin for any thermodynamic use.

## UI inputs to physics map

| Screen | UI input | Physics module(s) |
|---|---|---|
| Aerodynamics | Front wing angle (2–15°) | C2 Aero; T5 ride-height/rake coupling |
| | Front ride height (22–45 mm) | C2 Aero; T5 |
| | Rear wing angle (4–20°) | C2 Aero |
| | Floor load (0–100 %) | C2 Aero: underbody (floor) downforce setting |
| | Drag tendency (display) | C2: output of `C_D A` |
| Tires | Compound (Soft/Med/Hard now; Intermediate/Wet added via the compound table, §3B) | C4 Tires; T2 wear; T1 thermal; T8 wet grip |
| | Front/rear pressure (20–26 psi) | C4 (pressure factor); T1 (hot pressure) |
| | Grip window, life, degradation (display) | T1, T2 |
| | Mass contribution (46 kg) | C1 Mass |
| Powertrain | Power map (Qualifying/Race/Save) | C5 Powertrain; T6 energy/fuel |
| | Final drive ratio (3.20) | C5 |
| | Shift point (10,500–15,000 rpm) | C5 |
| | Peak power, top-gear limit, fuel use (display) | C5, T6 |
| | Engine mass (150 kg) | C1 |
| Chassis | Ballast (0–20 kg), position | C1 Mass/CG; T4 balance |
| | Rear ride height (60–100 mm) | C2; T5 |
| | Mass, weight distribution, rake, wheelbase, CG height (display) | C1, C3 |
| Brakes | Front bias (50–65 %) | C7 Braking |
| | Cooling ducts (Closed/Standard/Open) | T3 brake thermal; C2 (duct drag) |
| | Max decel, balance, temperature, duct drag (display) | C7, T3 |
| Weather | Preset, air temp, track temp, humidity, wind, rain | C2 (`ρ`), C4/T1 (tire temp), T7 weather, T8 wet grip |
| Surface | Track state, roughness, static/evolving | T8 Surface |
| Simulation | Laps, flying/standing start | C8 Solver |
| | Starting fuel (5–110 kg) | C1, T6 fuel burn |
| | Solver resolution (replaces "Physics time step 5 ms"), telemetry points | C8 Solver (distance step `Δ s`) |

## Constants that are not in the UI

These are needed by the equations above and must be stored in the car/track JSON (§ Design Document):

| Constant | Used in | Notes |
|---|---|---|
| Tire rolling radius `r_w`, track width `t` | C5, T4 | defaults ≈ 0.33 m, ≈ 1.6–1.8 m |
| Gear ratios `i_1 … i_8`, driveline efficiency, shift time loss | C5, §3A | hidden in v1; later user-editable in the Gear Ratio Lab |
| `m_base`, driver mass, component positions `x_i` | C1 | bottom-up mass; minimums come from the active regulation era (§3B) |
| Track elevation `h_track`, sea-level pressure `p_0` | T1, T7, C2 | in the track definition / weather inputs |
| Regulation era tables, tire compound table | §3B | data-driven, selectable in Settings |
| Engine torque curve | C5 | fitted to 723 kW at the rpm range |
| `A`, `C_L,0` and `C_D,0` per aero element, `ξ` | C2 | calibrate |
| Ride-height response `f(h)` | C2, T5 | calibrate |
| `μ_ref`, `k_ls`, `p_opt`, `T_opt`, `w_T`, `k_wear` per compound | C4, T1, T2 | calibrate |
| `C_0`, `C_2` rolling resistance | C6 | calibrate |
| Brake disc `c_p`, `h_cool` per duct, temperature window | T3 | calibrate |
| Axle ride stiffness `k_axle` | T5 | calibrate |
| Roll stiffness distribution, `C_α` per axle | T4 | calibrate |
| Static mass breakdown and positions | C1 | needed for CG |

## Display values: where they come from

These UI readouts should be **outputs of the model**, never hard-coded:

| UI readout | Computed from |
|---|---|
| Drag tendency | `Σ C_D A` (C2) |
| Peak power, top-gear speed | torque curve × map; `ω_max r_w / (i_top i_fd)` (C5) |
| Fuel use | T6 (`ṁ_fuel` vs power map) |
| Car mass | C1: `m_dry + m_ballast,mand` (+ current fuel when shown "with fuel"); also "over/under minimum" |
| Weight distribution, rake, CG | C1, T5 |
| Max deceleration, balance, brake temp | C7, T3 |
| Estimated grip / durability / warm-up | C4, T1, T2 |
| Grip factor, lap time vs ideal | T8; full solver delta (C8) |
| Fuel weight cost (+0.9 s lap 1) | solver sensitivity to `Δ m` (T6, C8) |

## Suggested build order

1. C1 mass/CG → C2 aero (constant coefficients first) → C6 resistances
2. C5 powertrain with a placeholder gear table → straight-line acceleration test (0–300 km/h, top speed)
3. C4 tires (constant $`\mu`$ with a friction ellipse) → C8 corner limit → **three-pass solver on a simple test track** (the end-to-end prototype)
4. C3 load transfer + C7 braking
5. Replace constants with calibrated functions (C2 ride height, C4 load sensitivity)
6. Second tier: T6 fuel burn → T1/T2 tires → T7/T8 weather and surface → T3 brakes → T4/T5 balance and ride height

Each step should ship with unit tests against analytic cases (e.g., a constant-radius corner with and without downforce; a straight-line top speed where $`F_{\text{drive}} = F_{\text{drag}} + F_{rr}`$).

## Validation plan (OpenF1)

OpenF1 provides historical Formula 1 data from 2023 onward [18]. Mapping:

| Data endpoint | Validates |
|---|---|
| `car_data` (speed, rpm, throttle, brake, gear) | top speed, gearing/shift model (C5), braking points (C7), speed trace shape (C8) |
| `laps` (sector and lap times) | overall solver output, sector times |
| `stints` | tire degradation (T2), fuel burn trend (T6) |
| `weather` | air/track temperature inputs (T7, T1) |
| `location` | track geometry extraction (separate track spec) |

Telemetry from real cars is a *reference*, not ground truth for every parameter (it includes driver, traffic and unknown setup). Compare shapes and deltas (e.g., `Δ` lap time for ±1 °C track temperature) rather than absolute lap times only.


## Open questions and decisions

**Resolved in v0.2**

1. QSS only (§C8). The UI "physics time step" becomes a distance-resolution setting.
2. "Floor load" = underbody downforce (§C2).
3. Mass is bottom-up with a regulatory minimum and driver reference-mass floor, enforced by ballast (§C1). FIA Issue 20 gives 724 kg + nominal tyre mass and 82 kg; earlier figures (768 / 80) are superseded and live in the era table.
4. Gear ratios stay hidden in v1; Gear Ratio Lab planned (§3A).

**Resolved in v0.3**

5. Pressure input is psi gauge; absolute pressure comes from track elevation and weather (§T1, §T7).
6. Regulation era is a Settings-selectable regulation table (§3B). The UI's mixed-era defaults (3.60 m wheelbase, 110 kg fuel, 799 kg mass) go away once each era supplies its own defaults.
7. Tire choice is free in any weather unless an era rule restricts it; intermediate and wet compounds are added to the compound table (§3B).

**Still open**

1. **Default era and its rule values.** Choose the default era and fill its rule rows from the FIA regulations (mass, wheelbase, fuel, power unit, tires). Include a Sandbox era?
2. **Default mass.** Tune default `m_base` so a stock car in the default era lands just above that era's minimum. Decide whether "Car mass" shows without fuel (regulatory) or with the current fuel.
3. **Floor setting scheme:** approve or modify the proposed "Floor aggressiveness" slider, "Floor load" readout and stall-margin design (§C2).
4. **Intermediate and wet compound data:** these need parameter sets (grip, wet response, thermal window) to be calibrated.
5. **Sporting rules:** should rules like mandatory compound usage be enforced (strict), advisory, or left out of the first release?

## Revision history

- v0.10: source verification pass: FIA Issue 20 mass values (724 kg + tyres, 82 kg driver), corrected references [7] [9] [14], solver attribution, understeer-gradient units, wind convention, thermal efficiency; Sources page now has a status table.
- v0.9: restructured into four pages (Overview, Core, Second Tier, Sources) to stay within GitLab's math rendering limits.
- v0.2 to v0.8: QSS-only solver; floor load defined as underbody downforce; bottom-up mass with regulatory minimum; Gear Ratio Lab; psi-gauge pressure input; regulation-era table; free tire choice; math syntax changes for GitLab.
