# Physics Model: Second-Tier Model

[← Overview](Physics-Model-Overview) · [← Core Model](Physics-Model-Core) · [Sources](Physics-Model-Sources)

These modules add realism and the "why" explanations on top of the [core model](Physics-Model-Core). Symbols are defined on the [Overview page](Physics-Model-Overview).

[[_TOC_]]

## Second-tier modules

### 3A. Growth feature: Gear Ratio Lab (hidden in v1, planned UI)

In v1 the gear ratios are fixed defaults inside the power-unit definition (JSON). The model already needs them (C5), so exposing them later requires **no new physics**, only UI and data validation. The planned feature is a gearing workbench with four linked plots:

| Plot | X axis | Y axis | What it teaches |
|---|---|---|---|
| **1. Power and torque curve** | engine rpm | power (kW) and torque (N·m), for the selected power map | where the engine makes peak power; effect of Qualifying/Race/Save |
| **2. Tractive-force diagram** (power over speed) | speed (km/h) | wheel force (N), one curve per gear | gear overlap, torque drop at each shift |
| | | + overlays: traction limit `μ F_z,r(v)`, and total resistance `F_drag + F_rr` | where each gear tops out; where top speed is (intersection of force curve and resistance) |
| **3. RPM-vs-speed sawtooth** | speed | engine rpm, one line per gear, shift points marked | how the final drive and shift rpm change the rpm drop after a shift |
| **4. Ratio spacing** | gear number | ratio (and step ratio `i_j / i_j+1`) | whether gearing is "tall" or "short", and whether spacing is progressive |

**Equations behind the plots** (all from C5/C6):

$`\begin{aligned} \textbf{Plot 1:}\quad & P(\omega) = T(\omega)\,\omega \qquad \text{(engine data × map scale)} \\[4pt] \textbf{Plot 2:}\quad & F_j(v) = \frac{T\!\left(\omega_j(v)\right) i_j\, i_{fd}\, \eta_{dl}}{r_w}, \qquad \omega_j(v) = \frac{v\, i_j\, i_{fd}}{r_w} \quad (\text{only where } \omega_j \le \omega_{\max}) \\ & F_{\text{envelope}}(v) = \max_j F_j(v) \qquad \text{(force available with optimal shifting)} \\ & F_{\text{resist}}(v) = \tfrac{1}{2}\rho\, C_D A\, v^{2} + C_{rr}\, m g \\ & v_{\text{top},j}:\ \text{solve } F_j(v) = F_{\text{resist}}(v) \qquad \text{(where gear } j \text{ ends)} \\[4pt] \textbf{Plot 3:}\quad & \omega_j(v) = \frac{v\, i_j\, i_{fd}}{r_w}, \qquad \text{rpm} = \omega_j \cdot \frac{60}{2\pi} \\[4pt] \textbf{Plot 4:}\quad & \text{step}_j = \frac{i_j}{i_{j+1}}, \qquad \text{geometric set: } i_j = i_1\, q^{-(j-1)} \end{aligned}`$

**Rules to teach:** acceleration in gear `j` is proportional to the net force `F_j - F_resist`, so the best shift point is where the next gear's force curve crosses the current one (typically near, not at, the rev limit); a shorter final drive moves every curve left (more force, lower top speed), a longer one moves them right. This turns the single "final drive ratio" and "shift point" sliders into something users can *see*.

**Staged plan**

1. **v1:** fixed, hidden ratio table. Plots 1 and 2 shown as read-only.
2. **v2:** add plots 3 and 4; show the current final-drive and shift-point choices on them.
3. **v3:** user-editable ratio table with validation (monotonic ratios, step-ratio bounds, rpm drop after shifts, maximum gear count, optional gearbox-cost term tied to the budget system), plus saved gearing presets per circuit (long straights vs. twisty).

**Data model:** `gear_ratios: [i_1 … i_N]`, `final_drive`, `eta_dl`, `shift_time_loss_s` (per-shift time/force loss), and `rpm_limit` in the power-unit JSON. The solver takes the force envelope `F_envelope(v)` as a precomputed lookup table, so editing ratios in v3 only re-bakes that table.

*Sources:* [6] Gillespie (tractive-force diagram, gear selection, effective mass); [10] Heywood (torque and power curves); [1] Milliken & Milliken (gear ratio effects on acceleration).

### 3B. Regulation eras and free tire choice

**Regulation table (Settings).** Rules live in data, not code, so users can pick an era in Settings and the whole app adapts (limits, defaults, validators). Suggested SQLite design (extensible without schema changes):

```
regulation_era(era_id, name, year_start, year_end, description, is_sandbox)
regulation_rule(era_id, rule_key, value, unit, severity, source_article)
    severity: 'hard'      → UI blocks the setting
              'warn'      → UI allows it and flags a violation
              'info'      → default or reference value only
```

| Category | Example `rule_key`s (values to be filled from the FIA text for each era) |
|---|---|
| Mass | `min_mass_no_fuel`, `min_driver_mass`, `nominal_tire_mass` |
| Dimensions | `wheelbase_max`, `width_max`, `ride_height_min` |
| Fuel / energy | `fuel_mass_max`, `fuel_flow_max`, `energy_per_lap_max` |
| Power unit | `ice_layout`, `mguk_power_max`, `mguh_allowed`, `rpm_limit`, `engine_mass_min` |
| Aero | `wing_dimension_limits`, `floor_cl_scale` (model scale for the era's floor type), `drs_or_active_aero` |
| Tires | `allowed_compounds`, `mandatory_compound_rule`, `tire_sets_max`, `pressure_min` (if any) |
| Brakes | `brake_disc_diameter_max`, `brake_mass_min` |

The physics engine reads rule values at load (e.g. C1 uses `min_mass_no_fuel` and `min_driver_mass`). A **Sandbox** era with no limits is useful for teaching ("what if there were no rules?"). Settings can offer a **strict/advisory** switch that turns `hard` rules into `warn`. Era values must come from the FIA Technical (and, where relevant, Sporting) Regulations for each season [17]; do not hard-code numbers from press summaries. Data from this table also drives the budget/cost constraints in the Design Document.

**Free tire choice.** Users can choose any compound in any weather unless the active era has a rule against it (`allowed_compounds`, `mandatory_compound_rule`). The physics therefore must handle every compound/weather combination without special-casing:

- Move compounds into a **compound table** (`tire_compound`) instead of hard-coding Soft/Medium/Hard, and add **Intermediate** and **Wet** entries. Each row holds `μ_ref`, `k_ls`, `T_opt`, `w_T`, `k_wear`, nominal mass, and wet-grip parameters.
- **Dry slick in the rain:** $`g_{\text{wet}}(\text{rain}, \text{compound})`$ for slicks falls steeply with rain intensity, with the aquaplaning limit (T8), so lap time degrades realistically. The model never forces a "correct" tire; the telemetry shows the consequence.
- **Wet or intermediate tire on a dry track:** wet compounds have softer rubber and grooves that overheat: their `T_opt` is lower and `k_wear` higher, so on a dry hot track `g_temperature` falls and wear accelerates (T1, T2). This again emerges from the same equations.
- The UI can show a **non-blocking suitability hint** ("Slick in light rain: low grip") computed from the model, which teaches without restricting.

*Sources:* FIA Technical and Sporting Regulations [17]; tire-model forms [1], [5], [14], [16].

### T1. Tire thermal model, warm-up and hot pressure

Lumped thermal energy balance per tire (or per axle):

$`\begin{aligned} C_t\,\frac{dT_t}{dt} &= Q_{\text{friction}} - h_{\text{air}} A_t\,(T_t - T_{\text{air}}) - h_{\text{road}} A_c\,(T_t - T_{\text{track}}) \\ Q_{\text{friction}} &\approx \lvert F_x\, v_{\text{slip},x}\rvert + \lvert F_y\, v_{\text{slip},y}\rvert \qquad \text{(friction power at the contact)} \end{aligned}`$

This produces warm-up (compound- and track-temperature dependent) and overheating. In the QSS solver the left-hand side is integrated segment by segment with $`\Delta t = 2\Delta s/(v_i+v_{i+1})`$.

Grip dependence on temperature (empirical bell curve):

$`g_{\text{temperature}}(T) = \exp\!\left( -\left(\frac{T - T_{\text{opt}}}{w_T}\right)^{2} \right) \qquad \text{[CAL: } T_{\text{opt}},\ w_T \text{ per compound]}`$

**Pressure input and absolute pressure.** The user enters pressure in **psi gauge** (as on a tire gauge, cold set pressure). The model converts it to absolute pressure using the ambient pressure at the track, which depends on **elevation and weather** (see T7):

$`\begin{aligned} p_{\text{atm}} &= p_0 \left(1 - \frac{0.0065\, h_{\text{track}}}{288.15}\right)^{5.25588} \qquad \text{(ISA barometric formula; } h_{\text{track}} \text{ in m, } p_0 = \text{sea-level pressure)} \\ p_{\text{cold,abs}} &= p_{\text{gauge,cold}} + p_{\text{atm}} \qquad (1\ \text{psi} = 6894.757\ \text{Pa; convert first}) \end{aligned}`$

Hot pressure follows from the ideal gas law at constant volume (use absolute pressure and kelvin), then converts back to gauge for display:

$`\begin{aligned} p_{\text{hot,abs}} &= p_{\text{cold,abs}}\,\frac{T_{\text{hot}}}{T_{\text{set}}} \qquad (T_{\text{set}} = \text{temperature when the pressure was set: ambient or tire-warmer temperature, configurable}) \\ p_{\text{hot,gauge}} &= p_{\text{hot,abs}} - p_{\text{atm}} \end{aligned}`$

Note the effect: the same gauge setting is a *slightly different* absolute pressure at altitude, which changes hot pressure and the pressure grip factor by a small amount. The `g_pressure` optimum (C4) should be defined on **absolute** (or hot gauge) pressure, consistent with one reference condition.

Hot pressure then feeds `g_pressure` (C4), which links the pressure slider to grip and temperature. "Peak grip window" in the UI is the period when `g_temperature` is near 1.

*Sources:* [14] Farroni et al. (thermal tire model); [8] Kelly; [12] Çengel & Boles (ideal gas law); [11] Incropera et al. (convective heat transfer).

### T2. Tire wear and degradation

Wear accumulates with frictional work (energy dissipated at the contact), with an additional factor when temperature is out of window:

$`\begin{aligned} \frac{dW}{ds} &= k_{\text{wear}}(\text{compound}) \cdot \frac{Q_{\text{friction}}}{v} \cdot f(T_t) \\ g_{\text{wear}} &= 1 - k_{\text{deg}}\, W \qquad \text{(or a lap-based form; cliff behavior after a threshold)} \end{aligned}`$

A simple lap-indexed form is acceptable for v1, with a threshold ("cliff"):

$`\mu(n) = \mu_0 - k_{\text{deg}}\, n`$

"Estimated life" and "Degradation" in the UI come from this module. Compound ordering: Soft = highest `μ_0` and highest wear rate; Hard = reverse; Medium between.

*Sources:* [15] Heilmeier et al. (tire degradation in race simulation); [14] Farroni et al. (wear/thermal coupling).
*Numbers [CAL]:* `k_wear`, `k_deg`, cliff lap per compound. No first-principles values exist; use stint data from OpenF1 `stints` and `laps` [18].

### T3. Brake thermal model and cooling ducts

Lumped-capacitance model of disc temperature (valid when the Biot number is small, so the disc is nearly isothermal):

$`\begin{aligned} m_d\, c_p\,\frac{dT_d}{dt} &= P_{\text{brake}} - h_{\text{cool}}(\text{duct})\, A_d\,(T_d - T_{\text{air}}) - \varepsilon\,\sigma\, A_d \left(T_d^{4} - T_{\text{air}}^{4}\right) \\ P_{\text{brake}} &= F_b\, v \end{aligned}`$

Cooling ducts raise `h_cool` (Open > Standard > Closed) but add drag `Δ (C_D A)` (C2). Pad/disc friction depends on temperature (fade below and above a window), so `μ_pad(T_d)` modifies C7. "Brake temperature: In window" in the UI is $`T_{\text{low}} < T_d < T_{\text{high}}`$.

*Sources:* [11] Incropera et al. (lumped capacitance, convection, radiation); [13] Limpert (brake thermal behavior, fade).
*Numbers [CAL]:* `c_p` and mass of the carbon disc set (40 kg per UI), `h_cool` per duct setting, temperature window.

### T4. Lateral load transfer and handling balance

Lateral transfer (needs track width `t` and roll-stiffness distribution, added to the car data):

$`\Delta F_{z,\text{lat}} = \frac{m\, a_y\, h}{t} \qquad \text{(split front/rear by roll-stiffness distribution)}`$

Load sensitivity makes lateral transfer *reduce* total axle grip (more load on one side gains less than the other side loses). Axle-level balance indicator via the understeer gradient:

$`K_{us} = \frac{W_f}{C_{\alpha,f}} - \frac{W_r}{C_{\alpha,r}}`$

`K_us > 0` is understeer, `K_us < 0` oversteer, with `W` the axle weights (N) and `C_α` the axle cornering stiffnesses (N/rad), giving `K_us` in rad per g of lateral acceleration (steer angle = L/R + K_us·a_y/g). Do not divide by `g` again if `W` is a weight; only divide if `W` is a mass. The form was confirmed in several vehicle-dynamics course notes and patents quoting Gillespie/Milliken; I could not open Gillespie itself. Ballast position (Chassis screen) moves the CG (C1) and therefore `W_f`, `W_r`, which is the mechanism by which it changes balance.

*Sources:* [1] Milliken & Milliken (lateral load transfer, balance); [6] Gillespie (understeer gradient).

### T5. Ride height, rake and aero coupling

Rake from the UI values:

$`\text{rake} = \arctan\!\left( \frac{h_{\text{rear}} - h_{\text{front}}}{L} \right)`$

(Check: $`\arctan\big((78-31)\ \text{mm} / 3600\ \text{mm}\big) = 0.75^\circ`$, matches the Chassis screen.)

Aero ride height is not static: downforce compresses the suspension. First-order model:

$`h_{\text{dyn}} = h_{\text{static}} - \frac{F_{\text{down,axle}}}{k_{\text{axle}}}`$

where `k_axle` is the axle ride stiffness (spring plus tire) **[CAL]**. Recompute `C_L` from `h_dyn` (C2). This couples speed to downforce nonlinearly, giving a realistic `C_L` rolloff at high speed and a floor-stall limit. Include a **bottoming/stall guard**: below a minimum ride height, downforce drops.

*Sources:* [3] Zhang, Toet & Zerihan; [2] Katz; [1] Milliken & Milliken (ride rates).

### T6. Fuel burn, energy and hybrid deployment

$`\frac{dm_{\text{fuel}}}{dt} = -\dot m_{\text{fuel}} = -\frac{P_{\text{engine}}}{\eta_{th}\, \text{LHV}}`$

with `η_th` (brake thermal efficiency; publicly reported figures for hybrid-era F1 engines are roughly 44% at the 2014 start, above 50% on the dynamometer per Mercedes statements, treat as a **[CAL]** value of about 0.45–0.50 and confirm for the 2026 engines) and LHV (gasoline $`\approx 43`$–`44` MJ/kg). In the QSS solver, burn per segment is $`\dot m_{\text{fuel}}\,\Delta t`$. Regulatory fuel flow limits and total fuel allowances apply [17]. Mass decreases over the run, changing `m`, CG (C1) and lap time (a rule-of-thumb sensitivity of a few hundredths of a second per kg per lap, consistent with the UI's "+0.9 s" at 30 kg; treat as a validation target, not an input).

**Power map effects:** Qualifying = higher power and fuel flow; Race = balanced; Save = reduced power. Implement as scaling of torque curve and fuel flow.

**Hybrid/ERS (if modeled):** track the battery energy state `E_b` with deploy power limits and harvest from braking:

$`\frac{dE_b}{dt} = \eta_h\, P_{\text{harvest}} - \frac{P_{\text{deploy}}}{\eta_d}`$

Limits from [17].

*Sources:* [10] Heywood (thermal efficiency, fuel energy); [17] FIA technical regulations (fuel and energy limits).

### T7. Weather, wind and air properties

- **Ambient pressure and elevation:** the track definition carries an elevation `h_track` (m). Station pressure is

$`p_{\text{atm}} = p_0 \left(1 - \frac{0.0065\, h_{\text{track}}}{288.15}\right)^{5.25588}`$

  with `p_0` the sea-level pressure (default 101,325 Pa; optionally a "barometric pressure" weather input). `p_atm` feeds (i) air density `ρ` (C2), so altitude and weather reduce downforce, drag and naturally aspirated power, (ii) the tire gauge-to-absolute conversion (T1), and (iii) engine power derating below. Use the *station* temperature, not sea-level temperature, in the density formula.
- **Air density:** C2 formula (temperature, humidity, pressure).
- **Wind:** relative air velocity, using the wind speed `w`, wind direction `ψ_wind` (the direction the wind blows **from**) and the track heading `ψ_track` at each point (heading comes from the track definition):

$`v_{\text{air}} = v + w_{\text{head}}, \qquad w_{\text{head}} = w \cos\!\left(\psi_{\text{wind}} - \psi_{\text{track}}\right)`$

  Crosswind side force and yaw effects are out of scope for v1.
- **Track temperature:** drives T1 (heat exchange with the road, warm-up).
- **Air temperature:** affects air density (C2), brake and engine cooling (T3).
- **Engine power vs density:** for a naturally aspirated engine $`P \propto \rho`$; turbocharged engines are largely compensated. Use the following with $`n \approx 0`$–`1` by engine type **[CAL]** [10]:

$`P = P_{\text{ref}} \left(\frac{\rho}{\rho_{\text{ref}}}\right)^{n}`$

*Sources:* [19] Picard et al.; [20] Alduchov & Eskridge; [10] Heywood; [11] Incropera et al.

### T8. Surface, evolution and wet grip

- **Track state:** multiplicative factor on grip: Green (low), Rubbered-in (reference $`\approx 1.0`$), Dusty (low). UI shows ×0.99 for Rubbered-in with a 30 % roughness **[CAL]**:

$`g_{\text{surface}} = g_{\text{state}}\,\big(1 - k_r\, \text{roughness}\big)`$

- **Evolution:** rubber build-up raises grip over laps, saturating **[CAL: `Δ g`, `τ`]**:

$`g_{\text{surface}}(n) = g_0 + \Delta g \left(1 - e^{-n/\tau}\right)`$

- **Roughness:** (i) reduces mean grip directly (above), and (ii) induces load variation. The lateral force a tire can produce is `g(F_z) = μ(F_z) F_z`, which is **concave** in `F_z` (load sensitivity, C4). By Jensen's inequality, average grip falls as load variance rises:

$`\big\langle \mu(F_z)\, F_z \big\rangle \ \le\ \mu\big(\langle F_z\rangle\big)\,\langle F_z\rangle`$

  A simple implementation is a grip penalty proportional to roughness.
- **Wet grip:** `g_wet` decreases with water-film depth, which grows with rain intensity. Rain also adds an aquaplaning speed limit above which grip collapses; a classic empirical estimate scales with the square root of tire pressure. Use a calibrated lookup table of `g_wet` (rain intensity, compound); compound choice interacts with wet performance (see §3B).
- **Wet effects on other modules:** lower track temperature, reduced brake cooling demand, spray (ignored).

*Sources:* [16] Horne & Dreher (NASA, tire hydroplaning); [1] Milliken & Milliken (load sensitivity); [15] Heilmeier et al. (track/tire evolution in race simulation).
*Numbers [CAL]:* nearly all. Use the OpenF1 `weather`, `laps` and `stints` endpoints [18] to calibrate.
