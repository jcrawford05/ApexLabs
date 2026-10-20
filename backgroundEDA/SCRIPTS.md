# Python scripts

All three scripts are in `scripts/` and use only the Python standard library (3.9+). Only `build_data.py` is required.

| Script | Required? | Reads | Writes |
| --- | --- | --- | --- |
| `build_data.py` | Yes | Kaggle CSVs in `Data/` | JSON files and `data.js` in `site/data/` |
| `make_sample_data.py` | No, for previews or testing | Nothing | Synthetic Kaggle-style CSVs in `SampleData/` |
| `build_standalone.py` | No, for a single shareable file | `site/` (after `build_data.py`) | One self-contained `.html` file |

Default paths are relative to the repository root (the folder that holds `scripts/`), not the folder you run from, so `python apex-labs/scripts/build_data.py` works from anywhere, including VS Code's Run button. Paths you pass explicitly, like `--data-dir`, are relative to your current folder.

Order: (optional) `make_sample_data.py` → `build_data.py` → (optional) `build_standalone.py`.

## build_data.py: turns the Kaggle CSVs into dashboard data

It reads the raw CSVs, joins them, pre-computes the statistics the pages show, and writes compact files the browser loads.

- **Required inputs:** `races.csv`, `results.csv`, `drivers.csv`, `constructors.csv`, `circuits.csv`, `lap_times.csv`. It stops with an error if any are missing.
- **Optional inputs:** `qualifying.csv`, `sprint_results.csv`, `status.csv`, `weather_features_v4.csv`. Weather is matched by season and round. Rain above 0 mm is labelled Wet.
- **Steps:**
  1. Build lookups.
  2. Attach circuit and weather to races.
  3. Average lap times per race and per driver.
  4. Enrich results with status, fastest lap (`1:27.452` text converted to ms) and average lap.
  5. Compute per-race metrics: winner, fastest lap, finishers.
  6. Compute per-season driver and constructor stats.
  7. Convert qualifying times to ms.
- **Outputs:** `metadata.json`, `results.json`, `race_metrics.json`, `driver_seasons.json`, `constructor_seasons.json`, `circuit_summary.json`, `qualifying.json`, `sprint_results.json`, and `data.js` (all of them as `window.APEX_DATA`, so the pages open straight from disk).
- **Flags:** `--data-dir` (default `Data`), `--output-dir` (default `site/data`), `--demo` (shows a "Demo data" banner).

## make_sample_data.py: generates fake data for previews and testing

It writes a made-up data set in the Kaggle CSV layout: real circuits, but 24 invented drivers, 10 invented teams, and 12 races a season from 2016 to 2024. It includes laps (every third lap), qualifying, sprints from 2021, weather, and a `README.txt` marking the data as synthetic.

- **Flags:** `--output-dir` (default `SampleData`), `--first-season` (2016), `--last-season` (2024), `--seed` (7; the same seed gives the same data).
- Always build its output with `--demo`:
  ```bash
  python scripts/make_sample_data.py --output-dir SampleData
  python scripts/build_data.py --data-dir SampleData --output-dir site/data --demo
  ```

## build_standalone.py: bundles the site into one HTML file

It inlines the CSS, D3 7.9.0, `site/data/data.js`, `dashboard.js`, and all five pages (as `<template>`s, switched with `#overview`, `#comparison`, and so on) into one file that works with no server or internet. Run it after `build_data.py`.

- **Flags:** `--site-dir` (default `site`), `--output` (default `apex-labs-dashboard.html`).
- **Size:** about 1.2 MB with sample data. Roughly 6–9 MB is expected with the full Kaggle data (an estimate).

## Troubleshooting

| Symptom | Fix |
| --- | --- |
| `Missing required CSV file(s)` | Copy the Kaggle files in with their original names |
| Pages show "Setup needed" | Run `build_data.py`, then reload |
| `site/data/data.js not found` | Run `build_data.py` before `build_standalone.py` |
| Weather "not available" | Add `weather_features_v4.csv` and rebuild |
| Pace charts empty before 1996 | Expected: Kaggle lap timing starts in 1996 |
| "Demo data" banner on real data | Rebuild without `--demo` |
