# Apex Labs F1 Historical Analytics

A static D3 dashboard for the Kaggle **Formula 1 World Championship (1950–2024)** dataset. It needs no database or application server, and it works offline: D3 is vendored and the data build writes a script file the pages can load straight from disk.

## Included pages

- **Race Overview** — race header (round, date, circuit, weather), KPIs (winner, fastest lap, average lap, finishers), grid-vs-finish movement, points by constructor, average lap time by driver, weather panel, and a sortable classification table with CSV export. Driver, constructor, and status filters list only that race's entrants.
- **Race Comparison** — two modes: *Same Grand Prix* (pick an event and two seasons) or *Any two races*. Shows driver pace distributions with medians, a finished / lapped / retired profile, and constructor points side by side.
- **Drivers** — every driver in the data set, career span defaults, eight career measures (points, wins, podiums, starts, average finish, positions gained, DNF rate, best season rank), points trend, finish distribution, and a sortable season table.
- **Circuits** — races hosted, most wins, fastest race lap, wins from pole, average and fastest lap trend, winners chart, and race history.
- **Constructors** — points, wins, podiums, entries, points per entry, DNF rate, best season rank, points trend, reliability chart with the period average, and a sortable season table.
- **Sources** — citations for the core Formula 1 and weather datasets, plus notes on the dashboard’s weather join and historical-data limits.

All pages cover every season in the data, have light and dark themes (following the system setting until toggled), and work down to phone width.

## Add the Kaggle data

Create a `Data` folder at the repository root and copy the Kaggle CSV files into it using their original names. The build script expects at least:

`races.csv`, `results.csv`, `drivers.csv`, `constructors.csv`, `circuits.csv`, and `lap_times.csv`.

It also uses `qualifying.csv`, `sprint_results.csv`, and `status.csv` when available.

For race weather, add `weather_features_v4.csv` to the same `Data` folder. Its records are matched to each race by season and round. It provides temperature, precipitation, and wind speed; the dashboard derives a dry/wet condition from precipitation.

The raw data folder is intentionally ignored by Git and never published with the site.

## Build and view

```bash
python scripts/build_data.py --data-dir Data --output-dir site/data
```

Then use any of these:

1. **Open `site/index.html` directly** (double-click). The build writes `site/data/data.js`, so no server is needed.
2. **Serve it:** `python -m http.server 8000 --directory site`, then open `http://localhost:8000`.
3. **One self-contained file** you can share or submit:
   ```bash
   python scripts/build_standalone.py --site-dir site --output apex-labs-dashboard.html
   ```
   This inlines the CSS, D3, data, and all five pages into a single HTML file (about 6–9 MB with the full Kaggle data).

## Preview without the Kaggle data

`scripts/make_sample_data.py` generates a small **synthetic** data set in the Kaggle CSV layout (invented drivers and teams on real circuits, 2016–2024). Pages built with `--demo` show a banner saying the data is synthetic.

```bash
python scripts/make_sample_data.py --output-dir SampleData
python scripts/build_data.py --data-dir SampleData --output-dir site/data --demo
```

## Notes on the data

- Kaggle stores lap times as text (`1:27.452`); the build converts fastest-lap and qualifying times to milliseconds.
- Lap-by-lap timing (`lap_times.csv`) starts in 1996, so pace charts show a clear message for earlier races.
- Season ranks are by points scored that season. They can differ from official standings in eras with dropped scores.
- Wind speed is shown in the weather source's own unit.

## Scripts

See [SCRIPTS.md](SCRIPTS.md) for what each Python script does, its flags, and troubleshooting.

## Weather

The Kaggle dataset does not include weather, but the optional `weather_features_v4.csv` source is supported. If it is absent, the site shows a clear data-availability message rather than failing to build.
