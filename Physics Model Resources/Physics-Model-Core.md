# Physics Model: Core Model

[← Overview](Physics-Model-Overview) · [Second-Tier Model →](Physics-Model-Second-Tier) · [Sources](Physics-Model-Sources)

Symbols and the UI-to-physics map are on the [Overview page](Physics-Model-Overview). Build these modules first: together they give the end-to-end prototype (configurable car, simplified track, simulation run, telemetry).

[[_TOC_]]

## Core model modules

### C1. Mass, CG and weight distribution

**Mass is built bottom-up from the selected parts**, so changing tires, engine or brakes changes mass and CG automatically:

$`\begin{aligned} m_{\text{dry}} &= m_{\text{base}} + m_{\text{engine}} + m_{\text{tires}} + m_{\text{brakes}} + m_{\text{driver}} + m_{\text{ballast,user}} \\ m &= m_{\text{dry}} + m_{\text{ballast,mand}} + m_{\text{fuel}}(t) \\ x_{cg} &= \frac{\sum_i m_i x_i}{m} \qquad (x \text{ measured from the front axle}) \\ \text{front weight fraction} &= \frac{b}{L} = 1 - \frac{x_{cg}}{L} \end{aligned}`$

| Term | Source |
|---|---|
| `m_base` | chassis, bodywork, suspension, wheel hardware, cooling, electronics (fixed default for v1) |
| `m_engine` | selected power unit (UI: V6 Hybrid Spec B, 150 kg) |
| `m_tires` | compound-dependent nominal mass (UI: 46 kg) |
| `m_brakes` | selected brake set (UI: carbon disc set, 40 kg) |
| `m_driver` | driver + seat + kit; regulation reference-mass floor (82 kg in Issue 20, see below) |
| `m_ballast,user` | Chassis-screen ballast slider (0–20 kg) at the chosen longitudinal position |
| `m_fuel(t)` | starting fuel (5–110 kg), decreasing along the run (T6) |

**Regulatory minimum (FIA Technical Regulations, Article C4).** Checked against the FIA 2026 Section C Technical Regulations, Issue 20 (5 Aug 2026): C4.1 gives a minimum mass of **724 kg plus the Nominal Tyre Mass** (726 kg plus nominal tyre mass in Sprint Qualifying and Qualifying), and C4.5.2 requires the driver's reference mass plus any driver ballast to be **at least 82 kg** at all times. Earlier summaries and secondary sources [21][23] quote 768 kg and 80 kg, which differ from the Issue 20 text, so these values are era-table configuration, never constants. The text I could retrieve does not state whether the car minimum is measured with or without fuel, so that point is **unverified**; check it against the FIA PDF before implementing. Ballast placement rules (C4.3) also apply. Defaults below use the Issue 20 values:

$`\begin{aligned} m_{\text{driver}} &= \max\!\left(m_{\text{driver,actual}},\ m_{\text{drv,ref}}\right) \quad \text{(extra mass is ballast at the driver/seat location)} \\ m_{\text{dry,parts}} &= m_{\text{base}} + m_{\text{engine}} + m_{\text{tires}} + m_{\text{brakes}} + m_{\text{driver}} + m_{\text{ballast,user}} \\ m_{\text{ballast,mand}} &= \max\!\left(0,\ m_{\min} - m_{\text{dry,parts}}\right), \qquad m_{\min} = 724\ \text{kg} + m_{\text{tyre,nom}}\ \text{(from the active regulation era)} \\ \text{overweight} &= \max\!\left(0,\ m_{\text{dry,parts}} - m_{\min}\right) \end{aligned}`$

So a build lighter than the minimum is automatically filled with low, fixed ballast, and a build heavier than the minimum simply carries the extra mass (a real performance cost, which is the intended design trade-off). `overweight` is shown in the UI as a design-cost penalty. The UI's "Car mass" readout is $`m_{\text{dry}} + m_{\text{ballast,mand}}`$ plus the current fuel, labeled clearly (see the Overview page, display values).

*Sources:* FIA 2026 Technical Regulations Art. C4.1, C4.3, C4.5 (Issue 20) [17]; older secondary summaries [21][23] (superseded, see Sources page); mass distribution [1], [6].

Static axle loads (before aero and transfer):

$`F_{z,f,\text{static}} = \frac{m g\, b}{L}, \qquad F_{z,r,\text{static}} = \frac{m g\, a}{L}`$

*Sources:* [1] Milliken & Milliken (weight distribution, CG); [6] Gillespie (static axle loads).

### C2. Aerodynamics

Aerodynamic forces:

$`\begin{aligned} F_{\text{down}} &= \tfrac{1}{2}\,\rho\, v_{\text{air}}^{2}\, C_L\, A \\ F_{\text{drag}} &= \tfrac{1}{2}\,\rho\, v_{\text{air}}^{2}\, C_D\, A \\ C_D A &= \sum_{\text{elements}} (C_D A)_k \quad \text{(front wing, rear wing, floor/body, brake ducts, wheels)} \end{aligned}`$

`v_air` is the speed relative to air (see T7 for wind). Downforce is split to the axles using an aero balance factor `ξ` (fraction on the front axle):

$`F_{\text{down},f} = \xi\, F_{\text{down}}, \qquad F_{\text{down},r} = (1-\xi)\, F_{\text{down}}`$

**Wing contribution to `C_L` and `C_D`.** A finite wing in free air (thin-airfoil theory plus the finite-wing correction):

$`\begin{aligned} C_{L,\text{wing}} &= \frac{a_0\,\alpha}{1 + \dfrac{a_0}{\pi\, AR}}, \qquad a_0 \approx 2\pi\ \text{per radian (2D)} \\ C_{D,\text{induced}} &= \frac{C_L^{2}}{\pi\, e\, AR} \\ C_{D,\text{total}} &= C_{D,0} + C_{D,\text{induced}} \end{aligned}`$

The linear region holds until stall, so clamp `α`. The wing sits in ground effect, so these free-air relations are corrected by a ride-height function (below).

**Ground effect (floor, front wing).** Downforce rises as ride height decreases, down to a limit where the flow chokes or stalls (force reduction). Use a calibrated function, e.g.

$`C_{L,\text{floor}} = C_{L,\text{ref}} \cdot f\!\left(\frac{h_{\text{ride}}}{h_{\text{ref}}}\right) \cdot (\text{floor load setting})`$

with $`f(\cdot)`$ a monotonic-then-saturating curve from published ground-effect data **[CAL]**.

**Air density, including humidity and temperature:**

$`\begin{aligned} e_s &= 6.1094\,\exp\!\left(\frac{17.625\,T_c}{T_c + 243.04}\right) \quad [\text{hPa}] \quad \text{(Magnus form)} \\ e &= \mathrm{RH}\cdot e_s \\ \rho &= \frac{p - e}{R_d\, T} + \frac{e}{R_v\, T} \qquad (p,\ e \text{ in Pa};\ T \text{ in K}) \\ R_d &= 287.05\ \text{J/(kg·K)}, \qquad R_v = 461.495\ \text{J/(kg·K)} \end{aligned}`$

(Humid air is *less* dense than dry air at the same `p` and `T`, which slightly raises top speed and lowers downforce.)

**Floor load = underbody downforce.** The underbody (floor, venturi tunnels, diffuser) is modeled as its own downforce element, separate from the front and rear wings:

$`\begin{aligned} F_{\text{down,floor}} &= \tfrac{1}{2}\,\rho\, v_{\text{air}}^{2}\, C_{L,\text{floor}}(h_{\text{dyn}},\, s_{\text{floor}})\, A \\ F_{\text{down}} &= F_{\text{down,front wing}} + F_{\text{down,rear wing}} + F_{\text{down,floor}} \end{aligned}`$

The UI "Floor load (0–100 %)" is `s_floor`, the setting that scales the floor's downforce between its minimum and maximum achievable `C_L,floor`; the model also reports `F_down,floor` in newtons at a reference speed. Because the floor works as a suction (low-pressure) device, `C_L,floor` is strongly ride-height dependent (T5), and a higher setting carries a drag and stall-sensitivity penalty **[CAL]**. The floor acts mainly near the CG-to-front region, so it is applied through the aero balance `ξ` (front fraction) with its own center of pressure.

**Proposed floor-setting scheme (recommendation, pending approval).** Real floors are not tuned with a "load" knob: floor downforce is an *outcome* of ride height, rake, floor edge and diffuser design. For a teaching tool I recommend:

- **Keep the slider, rename it "Floor aggressiveness" (0–100 %).** It represents how hard the underbody design pushes for suction.
- **Make "Floor load" a computed readout** in newtons (and as a % of total downforce) at a reference speed, e.g. 250 km/h, so users see the *result* of their choices.
- **Model the trade-off physically** so the slider has a cost (with $`s \in [0,1]`$ the slider value):

$`\begin{aligned} C_{L,\text{floor}} &= \Big[\, C_{L,\min} + s\,(C_{L,\max} - C_{L,\min}) \Big] \cdot f\!\left(h_{\text{dyn}};\ h_{\text{stall}}(s)\right) \\ C_{D,\text{floor}} &= C_{D,0,\text{floor}} + k_f\, C_{L,\text{floor}}^{2} \qquad \text{(suction and diffuser expansion add drag)} \\ h_{\text{stall}}(s) &= h_{\text{stall},0} + s\,\Delta h_{\text{stall}} \qquad \text{(a more aggressive floor stalls at a higher ride height)} \end{aligned}`$

  Here $`f(\cdot)`$ is the ground-effect curve (T5): downforce rises as ride height falls toward a peak, then collapses once the floor stalls ($`f \to 0`$ below `h_stall`). Higher `s` gives more peak downforce but a narrower safe ride-height window and more drag.

- **Add a "floor stall margin" readout**, the minimum over the lap of the margin below, in mm:

$`\text{margin} = \min_{\text{lap}} \left( h_{\text{dyn}} - h_{\text{stall}} \right)`$

  If the margin goes negative at some speed the solver applies the downforce collapse there (a stall penalty), which is what teaches users why a "lower is always better" setup fails. QSS cannot reproduce porpoising dynamics; the margin is the QSS stand-in.
- **Regulation link:** `C_L,max` and the stall behavior scale with the era's `floor_cl_scale` (§3B), because ground-effect floors (tunnels) and flat-floor eras differ greatly.

Why this over a plain "% of `C_L`" slider: it ties floor aggressiveness to ride height, rake (Chassis and Aerodynamics screens) and drag, so three UI controls interact the way the physics says they should.

**Outputs for the UI:** "Drag tendency" is a banded display of `C_D A`. "Duct drag" is the `Δ (C_D A)` added by the cooling duct setting.

*Sources:* [4] Anderson (thin-airfoil, finite wing, induced drag); [2] Katz (race car aero, ground effect, wing/floor behavior); [3] Zhang, Toet & Zerihan (ground-effect aerodynamics of race cars); [19] Picard et al. (moist-air density); [20] Alduchov & Eskridge (saturation vapor pressure).
*Numbers [CAL]:* `A`, base `C_L` and `C_D` per element, ride-height curve, aero balance `ξ`. Use FIA dimensional limits [17] for geometry and public estimates plus validation (see the Overview page, validation plan) for coefficients.

### C3. Longitudinal load transfer and axle loads

Under acceleration `a_x` (positive = forward), with aero:

$`\begin{aligned} \Delta F_z &= \frac{m\, a_x\, h}{L} \qquad \text{(transfer to the rear under acceleration)} \\ F_{z,f} &= \frac{m g\, b}{L} + F_{\text{down},f} - \Delta F_z \\ F_{z,r} &= \frac{m g\, a}{L} + F_{\text{down},r} + \Delta F_z \end{aligned}`$

Under braking `a_x` is negative and load moves to the front.

*Sources:* [1] Milliken & Milliken; [6] Gillespie.

### C4. Tires: grip model

Core assumption: **friction-circle (ellipse) limit with load sensitivity.**

Friction limit per axle:

$`\begin{aligned} \mu_{\text{eff}} &= \mu_{\text{compound}} \cdot g_{\text{pressure}} \cdot g_{\text{temperature}}(T_{\text{tire}}) \cdot g_{\text{wear}} \cdot g_{\text{surface}} \cdot g_{\text{wet}} \\ \mu(F_z) &= \mu_{\text{ref}} \left[\, 1 - k_{ls}\,\frac{F_z - F_{z,\text{ref}}}{F_{z,\text{ref}}} \,\right] \qquad (k_{ls} \text{ small and positive}) \ \text{[CAL]} \end{aligned}`$

Combined slip (friction ellipse):

$`\left(\frac{F_x}{\mu_x F_z}\right)^{2} + \left(\frac{F_y}{\mu_y F_z}\right)^{2} \le 1`$

So at a given load, cornering use consumes braking/traction capacity. This is what makes "trail-braking" and corner exit trade-offs appear in the QSS solver.

**Pressure factor.** Contact patch and carcass stiffness change with pressure, so grip peaks near an optimum pressure. Use a smooth peaked function **[CAL]**:

$`g_{\text{pressure}}(p) = 1 - k_p\,(p - p_{\text{opt}})^{2}`$

**Tire radius and rolling resistance** need defaults (not in UI): $`r_w \approx 0.33`$ m **[CAL]**; see C6.

**Upgrade path.** Replace the simple `μ` model with the Pacejka Magic Formula:

$`y = D \sin\!\Big( C \arctan\!\big( B x - E\,(B x - \arctan(B x)) \big) \Big)`$

where `x` is slip, `y` is force, and `B, C, D, E` are fitted. Magic Formula parameters for a racing tire are rarely public. Start with the load-sensitive friction ellipse and keep the interface compatible.

*Sources:* [1] Milliken & Milliken (tire load sensitivity, friction circle); [5] Pacejka (Magic Formula, combined slip); [8] Kelly (tire model use in lap simulation).
*Numbers [CAL]:* `μ_ref` per compound, `k_ls`, `p_opt`, `k_p`.

### C5. Powertrain and longitudinal drive force

**Engine:** torque curve `T_e(ω_e)`, power `P = T_e ω_e`. The power map (Qualifying/Race/Save) is a scaling of the torque curve and fuel flow: $`T_e \to k_{\text{map}}\, T_e`$.

**Wheel force and speed in gear `j`:**

$`F_{\text{drive}} = \frac{T_e\, i_j\, i_{fd}\, \eta_{dl}}{r_w}, \qquad v = \frac{\omega_e\, r_w}{i_j\, i_{fd}}`$

(`i_j` = gear ratio, `i_fd` = final drive, 3.20 default, `η_dl` = driveline efficiency $`\approx 0.93`$–`0.97` **[CAL]**.)

**Shift logic:** upshift when `ω_e` reaches the shift-point rpm (user input, 10,500–15,000). Gear `j` is selected as the highest gear that keeps `ω_e` below that rpm, with a minimum rpm after the shift. The UI exposes only the final drive and the shift rpm, so a **gear ratio table (`i_1 … i_8`)** is a required hidden constant **[CAL]**. These stay hidden from users in v1 and are fixed per power unit; they are also the basis for a future user-facing "Gear Ratio Lab" (§3A).

**Top-gear speed limit (display):**

$`v_{\max,\text{gear}} = \frac{\omega_{\max}\, r_w}{i_{\text{top}}\, i_{fd}}`$

**Traction limit (rear-wheel drive):**

$`F_{x,\text{drive}} \le \mu_x\, F_{z,r} \qquad \text{(then apply the friction ellipse with lateral demand)}`$

**Effective mass for acceleration** (rotating inertia):

$`m_{\text{eff}} = m + \frac{I_{w,\text{total}} + I_e\,(i_j\, i_{fd})^{2}\,\eta_{dl}}{r_w^{2}}`$

*Sources:* [6] Gillespie (driveline, gear ratios, effective mass); [10] Heywood (engine torque/power, efficiency).
*Numbers [CAL]:* torque curve to match 723 kW peak and rpm range; gear ratios; inertias; `r_w`. Use FIA regulations [17] for powertrain limits.

### C6. Resistive forces

$`\begin{aligned} F_{rr} &= C_{rr}\, m g \qquad \text{(use total } F_z \text{ including downforce for loaded rolling resistance)} \\ C_{rr} &= C_0 + C_2\, v^{2} \qquad \text{[CAL]; decreases with higher pressure} \\ F_{\text{grade}} &= m g \sin\theta \qquad (\theta \text{ from track elevation, if supplied}) \\ F_{\text{drag}} &= \text{see C2} \end{aligned}`$

Net longitudinal equation of motion:

$`m_{\text{eff}}\, a_x = F_{\text{drive}} - F_{\text{drag}} - F_{rr} - F_{\text{grade}} - F_{\text{brake}}`$

*Sources:* [6] Gillespie; [22] Wong, *Theory of Ground Vehicles* (rolling resistance).

### C7. Braking

Available friction limits the force at each axle; the brake bias `β` (front fraction, 50–65 %) distributes the brake force:

$`\begin{aligned} F_{b,f} &= \beta\, F_b, \qquad F_{b,r} = (1-\beta)\, F_b \\ \text{Front limit: } F_{b,f} &\le \mu\, F_{z,f}, \qquad \text{Rear limit: } F_{b,r} \le \mu\, F_{z,r} \\ F_{b,\max} &= \min\!\left( \frac{\mu\, F_{z,f}}{\beta},\ \frac{\mu\, F_{z,r}}{1-\beta} \right) \\ a_{\text{brake}} &= \frac{F_{b,\max} + F_{\text{drag}} + F_{rr}}{m_{\text{eff}}} \end{aligned}`$

With load transfer (C3) and aero (C2), `F_z,f` and `F_z,r` depend on the deceleration itself, so solve iteratively or in closed form at each speed.

**Brake balance (display):** compare `β` with the ideal bias that loads both axles to their limit simultaneously:

$`\beta_{\text{ideal}} = \frac{F_{z,f}}{F_{z,f} + F_{z,r}} \qquad \text{(evaluated at max deceleration)}`$

If $`\beta > \beta_{\text{ideal}}`$ the front locks first (understeer on entry); if $`\beta < \beta_{\text{ideal}}`$ the rear locks first (instability). "Balanced" means within a tolerance band.

**Max deceleration (display, UI 5.0 g)** is an *output* of this model, not an input. Check it against validation data [18].

*Sources:* [13] Limpert, *Brake Design and Safety*; [6] Gillespie; [1] Milliken & Milliken.

### C8. Cornering limit and the speed-profile solver

**Cornering speed limit** (point mass with aero, friction limited), on a corner of radius `R = 1/κ`:

$`\begin{aligned} m\, v^{2}\kappa &= \mu_y \left( m g + \tfrac{1}{2}\rho\, C_L A\, v^{2} \right) \\ v_{\text{corner}} &= \sqrt{\frac{\mu_y\, m g}{m\kappa - \tfrac{1}{2}\rho\, C_L A\,\mu_y}} \qquad \text{if } m\kappa > \tfrac{1}{2}\rho\, C_L A\,\mu_y \end{aligned}`$

If the denominator is $`\le 0`$ the aero force alone exceeds the grip needed: the corner is not grip limited (cap at the top speed or power limit). Because `μ` depends on `F_z` (load sensitivity, C4), iterate to convergence (fixed-point, 3–6 iterations).

**Three-pass QSS solver** (distance-domain, step `Δ s`). This is the standard corner-limit / forward / backward scheme described in the QSS lap-time literature [8][24][25]; the specific implementation here is our own design:

1. **Corner pass:** compute `v_corner,i` at every track point from curvature.
2. **Forward pass (acceleration):** from the start,

$`v_{i+1}^{2} = v_i^{2} + 2\, a_{x,i}\, \Delta s`$

   where `a_x,i` is the minimum of the power-limited acceleration (C5, C6), traction-limited acceleration (C4, C5), and friction-ellipse-limited acceleration given the lateral demand $`a_y = v^{2}\kappa`$. Cap at `v_corner,i+1` and the top speed.
3. **Backward pass (braking):** from the end,

$`v_i^{2} = v_{i+1}^{2} + 2\,\lvert a_{\text{brake},i}\rvert\, \Delta s`$

   with the braking limit (C7), capped at the forward-pass result. This produces the braking zones.

Take the pointwise minimum. Time per step with constant acceleration, then sector times and lap time follow by summation:

$`\Delta t = \frac{2\,\Delta s}{v_i + v_{i+1}}, \qquad t_{\text{lap}} = \sum_i \Delta t_i`$

**Flying lap vs. standing start:** flying uses the converged speed at the start/finish line; standing starts at `v = 0` with a launch traction limit (C5) on the first segment.

**Telemetry:** report speed, `a_x`, `a_y`, gear, rpm, brake demand, downforce, tire state over distance (resample the solution to the requested number of points, e.g. 800).

**Decision: the simulator is QSS only.** It is distance-based, with no transient time-domain integrator. The UI's "Physics time step: 5 ms" therefore becomes **solver resolution `Δ s`** (e.g. 1–5 m per step, or "points per lap"), and the telemetry point count is lap length divided by `Δ s`. Slowly varying states (tire temperature, wear, brake temperature, fuel mass, track grip) are updated **segment by segment** using the segment's elapsed time $`\Delta t = 2\Delta s/(v_i + v_{i+1})`$ as the integration step (explicit Euler is adequate at these timescales). Changed states then feed back into the next lap's solve (multi-lap runs iterate the solver per lap with updated states). This keeps the engine simple, deterministic and fast, which matches the "about 3 s per run" target in the UI.

*Sources:* [8] Kelly (lap-time simulation, QSS vs transient); [24] Rossi et al. (QSS apex / forward-backward description); [25] survey of minimum-lap-time optimization and simulation, Vehicle System Dynamics (2021; authors not confirmed); [9] OpenLAP (open-source reference implementation, three-pass structure); [1] Milliken & Milliken (cornering with aero). Note: [7] Brayshaw & Harrison is an optimal-control-style QSS method, not this three-pass scheme, and is cited only as related work.

## Sanity checks with closed-form answers (unit tests)

- **Terminal speed:** at top speed, $`F_{\text{drive}} = F_{\text{drag}} + F_{rr}`$.
- **No-aero corner:** $`v = \sqrt{\mu\, g\, R}`$.
- **Aero-dominant corner:** as `v` grows, the lateral acceleration limit tends to
  $`a_{y,\max} \to \mu\, \dfrac{\tfrac{1}{2}\rho\, C_L A\, v^{2}}{m}`$, which grows as $`v^2`$ while the demand $`v^2 \kappa`$ also grows as $`v^2`$. The corner is therefore unlimited by grip when $`\mu\,\dfrac{\tfrac{1}{2}\rho\, C_L A}{m} \ge \kappa`$ (the same condition as the denominator in C8).
- **Braking without aero:** `a_brake = μ g` (front/rear limits only when bias is mismatched).
- **Energy check:** over a lap, the kinetic energy change is 0; work done by the drive force equals work lost to drag, rolling resistance and brakes.
- **Rake:** $`\arctan\big((78-31)/3600\big) = 0.75^\circ`$ (matches the UI).
