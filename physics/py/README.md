# apexphys: Python reference for the physics model

The executable form of the Physics Model pages, built first and then ported to
C++ (see `Physics Model Resources/Physics-Model-Integration-Plan.md`). It is
never shipped with the desktop app.

## Setup (Python 3.11 or newer)

Run from `physics/py`:

```
py -3.12 -m venv .venv
.venv\Scripts\python -m pip install -e ".[dev]"
```

## Use

```
.venv\Scripts\python -m pytest
.venv\Scripts\python -m apexphys run --car ..\..\database\data\sample_vehicle.json --track ..\..\database\data\sample_track.json
```

Parameter files live in `physics/data/`. Car and track files use the app's
existing JSON formats (`database/data/sample_vehicle.json`, `sample_track.json`).
