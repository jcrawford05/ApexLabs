# Physics model: where we left off (2026-11-17)

Read `Physics Model Resources/Physics-Model-Integration-Plan.md` (v0.3) first. It
defines the phases named below. The model equations are in
`Physics-Model-Core.md` and `Physics-Model-Second-Tier.md`.

## State

- Branch `phy-model-prelim`, working tree clean. P0, P1 and P2 are each one commit.
- **Done:** Track P phases P0 (scaffold), P1 (straight line), P2 (lap solver and
  golden files). 151 pytest tests pass.
- **Not started:** P3 onward, all of Track D (OpenF1), all of Track C (C++ port
  and desktop). There is no C++ under `physics/` yet.
- **Next:** not chosen. The plan's critical path is C0 then C1 (C++ scaffold,
  port of P1 and P2 against `physics/golden`). The alternative is P3 (load
  transfer, brake bias, ride-height aero, load sensitivity) in Python.

## Run it

From `physics/py` (the virtual environment is git-ignored; recreate it per `README.md`):

```
.venv\Scripts\python -m pytest
.venv\Scripts\python -m apexphys run --car ..\..\database\data\sample_vehicle.json --track ..\..\database\data\sample_track.json
.venv\Scripts\python tools\make_golden.py
```

Sample car on the Demo Circuit: flying lap 1:05.078, top speed 340 km/h (rev limit).

## Decisions made

- D1: default era is "Apex 2026 Default". Its rules in
  `physics/data/regulation_eras.json` were read from FIA Section C Issue 21
  (2026-10-19), each with its article number.
- D2: `m_base` is 540 kg, with a separate 82 kg driver.
- D3: 723 kW peak power and a 3.40 m wheelbase.

## Things to know before changing the model

- **Placeholder data.** The torque curve, gear ratios, part positions,
  driveline efficiency, inertias and rolling resistance are uncalibrated. Each
  data file's `note` says which values are placeholders and how they were chosen.
- **Golden files gate changes.** `tests/regression` fails when the model or data
  no longer matches `physics/golden`. After an intended change, rerun
  `tools/make_golden.py` and say so in the commit.
- **P2 simplifications that P3 replaces.** Braking uses the grip of all four
  tyres with no bias; traction uses the static rear load; grip is a constant
  per compound; aero uses the package's lumped coefficients, so wing angles,
  ride heights and the floor setting have no effect yet.
- **Effective mass in braking.** `solver.max_deceleration` divides by effective
  mass, as the Core page says. Grip-limited braking should arguably use plain
  mass. Revisit in P3.
- **Not modelled.** ERS-K speed taper and energy limits, shift time loss,
  multi-lap runs (`SolverConfig.laps` is ignored), wind, grade.

## Open items for the team

- The stock car is 111 kg over the era minimum (881 kg against 770 kg).
- The UI power unit is 150 kg, below the 185 kg regulation minimum (plan gap 11).
- The Second-Tier page uses a fuel LHV of 43–44 MJ/kg; the 2026 limit is
  38–41 MJ/kg (plan gap 12).
- The Core, Overview and Sources pages still cite FIA Issue 20; the data uses Issue 21.
- Desktop defaults (`vehicle.h`, `sample_vehicle.json`) still say 3.60 m and 745 kW.
- Decisions D4 to D10 in the plan are still open.

## Working preferences

- The user makes all git commits. Do not commit; offer a commit message in the
  repo's "Added ..." style when asked.
