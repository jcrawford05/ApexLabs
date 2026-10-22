#!/usr/bin/env python3
"""Build compact dashboard data from Kaggle Formula 1 CSV files.

No third-party packages are required. Source files stay in Data/; this script
writes derived JSON that the static D3 site can safely load.
"""

from __future__ import annotations

import argparse
import csv
import json
from collections import defaultdict
from datetime import datetime, timezone
from math import ceil
from pathlib import Path
from statistics import median

# Every JSON file the dashboard loads. data.js bundles the same payloads so the
# site also works when an HTML file is opened straight from disk.
# Default paths are relative to the repository root (the folder above scripts/),
# so the script works no matter which directory it is run from.
REPO_ROOT = Path(__file__).resolve().parent.parent

OUTPUT_FILES = ["metadata", "results", "race_metrics", "driver_seasons", "constructor_seasons", "circuit_summary", "qualifying", "sprint_results", "weather_impact"]


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open(encoding="utf-8-sig", newline="") as source:
        return list(csv.DictReader(source))


def integer(value: str | None, default: int = 0) -> int:
    try:
        return int(value or default)
    except ValueError:
        return default


def number(value: str | None, default: float = 0.0) -> float:
    try:
        return float(value or default)
    except ValueError:
        return default


def lap_ms(value: str | None) -> int:
    """Convert a Kaggle lap time such as "1:27.452" or "27.452" to milliseconds."""
    text = (value or "").strip()
    if not text or text == "\\N":
        return 0
    try:
        minutes, _, seconds = text.rpartition(":")
        return round((int(minutes or 0) * 60 + float(seconds)) * 1000)
    except ValueError:
        return 0


def column(row: dict[str, str], *names: str) -> str | None:
    """Return the first non-empty value among alternative column names."""
    for name in names:
        value = row.get(name)
        if value not in (None, "", "\\N"):
            return value
    return None


def write_json(directory: Path, filename: str, payload: object) -> None:
    with (directory / filename).open("w", encoding="utf-8") as output:
        json.dump(payload, output, separators=(",", ":"), ensure_ascii=False)


def write_bundle(directory: Path, payloads: dict[str, object]) -> None:
    """Write data.js: every payload assigned to window.APEX_DATA for file:// use."""
    text = json.dumps(payloads, separators=(",", ":"), ensure_ascii=False).replace("</", "<\\/")
    (directory / "data.js").write_text(f"window.APEX_DATA={text};\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", type=Path, default=REPO_ROOT / "Data")
    parser.add_argument("--output-dir", type=Path, default=REPO_ROOT / "site" / "data")
    parser.add_argument("--demo", action="store_true", help="Flag the output as synthetic sample data (shows a banner in the site).")
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    required = ["races.csv", "results.csv", "drivers.csv", "constructors.csv", "circuits.csv", "lap_times.csv"]
    missing = [name for name in required if not (data_dir / name).exists()]
    if missing:
        raise SystemExit(f"Missing required CSV file(s) in {data_dir.resolve()}: {', '.join(missing)}")

    drivers = read_csv(data_dir / "drivers.csv")
    constructors = read_csv(data_dir / "constructors.csv")
    circuits = read_csv(data_dir / "circuits.csv")
    races = read_csv(data_dir / "races.csv")
    results = read_csv(data_dir / "results.csv")
    lap_times = read_csv(data_dir / "lap_times.csv")
    qualifying = read_csv(data_dir / "qualifying.csv")
    sprint_results = read_csv(data_dir / "sprint_results.csv")
    statuses = read_csv(data_dir / "status.csv")
    weather_rows = read_csv(data_dir / "weather_features_v4.csv")

    weather_by_season_round = {
        (integer(row.get("datetime", "")[:4]), integer(row.get("round"))): {
            "available": True,
            "condition": "Wet" if number(row.get("precipitation")) > 0 else "Dry",
            "temperatureC": number(row.get("temperature")),
            "precipitationMm": number(row.get("precipitation")),
            "windSpeed": number(row.get("windspeed")),
            "observedAt": row.get("datetime") or "",
        }
        for row in weather_rows
        if row.get("datetime") and row.get("round")
    }

    driver_lookup = {
        integer(row.get("driverId")): {
            "driverId": integer(row.get("driverId")),
            "name": f"{row.get('forename', '').strip()} {row.get('surname', '').strip()}".strip(),
            "code": row.get("code") or "",
            "nationality": row.get("nationality") or "",
        }
        for row in drivers
    }
    constructor_lookup = {
        integer(row.get("constructorId")): {
            "constructorId": integer(row.get("constructorId")),
            "name": row.get("name") or "Unknown constructor",
            "nationality": row.get("nationality") or "",
        }
        for row in constructors
    }
    circuit_lookup = {
        integer(row.get("circuitId")): {
            "circuitId": integer(row.get("circuitId")),
            "name": row.get("name") or "Unknown circuit",
            "location": row.get("location") or "",
            "country": row.get("country") or "",
            "lat": number(row.get("lat")),
            "lng": number(row.get("lng")),
        }
        for row in circuits
    }
    status_lookup = {integer(row.get("statusId")): row.get("status") or "Unknown" for row in statuses}

    race_lookup: dict[int, dict] = {}
    for row in races:
        race_id = integer(row.get("raceId"))
        circuit = circuit_lookup.get(integer(row.get("circuitId")), {})
        year = integer(row.get("year"))
        round_number = integer(row.get("round"))
        race_lookup[race_id] = {
            "raceId": race_id,
            "year": year,
            "round": round_number,
            "name": row.get("name") or "Unknown Grand Prix",
            "date": row.get("date") or "",
            "time": row.get("time") or "",
            "circuitId": integer(row.get("circuitId")),
            "circuitName": circuit.get("name", "Unknown circuit"),
            "country": circuit.get("country", ""),
            "location": circuit.get("location", ""),
            "weather": weather_by_season_round.get((year, round_number), {
                "available": False,
                "condition": None,
                "temperatureC": None,
                "precipitationMm": None,
                "windSpeed": None,
                "observedAt": None,
            }),
        }

    lap_aggregates: dict[int, dict[str, int]] = defaultdict(lambda: {"totalMs": 0, "count": 0})
    driver_lap_aggregates: dict[tuple[int, int], dict[str, int]] = defaultdict(lambda: {"totalMs": 0, "count": 0})
    driver_lap_values: dict[tuple[int, int], list[int]] = defaultdict(list)
    for row in lap_times:
        race_id = integer(row.get("raceId"))
        milliseconds = integer(row.get("milliseconds"))
        if milliseconds <= 0:
            continue
        lap_aggregates[race_id]["totalMs"] += milliseconds
        lap_aggregates[race_id]["count"] += 1
        driver_id = integer(row.get("driverId"))
        driver_laps = driver_lap_aggregates[(race_id, driver_id)]
        driver_laps["totalMs"] += milliseconds
        driver_laps["count"] += 1
        driver_lap_values[(race_id, driver_id)].append(milliseconds)

    enriched_results: list[dict] = []
    race_results: dict[int, list[dict]] = defaultdict(list)
    for row in results:
        race_id = integer(row.get("raceId"))
        driver_id = integer(row.get("driverId"))
        constructor_id = integer(row.get("constructorId"))
        item = {
            "raceId": race_id,
            "driverId": driver_id,
            "constructorId": constructor_id,
            "position": integer(row.get("position")),
            "positionOrder": integer(row.get("positionOrder")),
            "points": number(row.get("points")),
            "grid": integer(row.get("grid")),
            "laps": integer(row.get("laps")),
            "statusId": integer(row.get("statusId")),
            "status": status_lookup.get(integer(row.get("statusId")), row.get("status") or "Unknown"),
            "fastestLap": integer(row.get("fastestLap")),
            # Kaggle stores "1:27.452" strings; some exports split out a milliseconds column.
            "fastestLapTime": integer(column(row, "fastestLapTime.milliseconds")) or lap_ms(row.get("fastestLapTime")),
            "fastestLapSpeed": number(column(row, "fastestLapSpeed.speed", "fastestLapSpeed")),
            "averageLapMs": None,
            "lapSamples": 0,
        }
        driver_laps = driver_lap_aggregates.get((race_id, driver_id))
        if driver_laps and driver_laps["count"]:
            item["averageLapMs"] = round(driver_laps["totalMs"] / driver_laps["count"])
            item["lapSamples"] = driver_laps["count"]
        enriched_results.append(item)
        race_results[race_id].append(item)

    race_metrics = []
    for race_id, race in race_lookup.items():
        entries = race_results.get(race_id, [])
        lap_data = lap_aggregates.get(race_id, {"totalMs": 0, "count": 0})
        winner = next((entry for entry in entries if entry["positionOrder"] == 1), None)
        fastest = min((entry for entry in entries if entry["fastestLapTime"] > 0), key=lambda entry: entry["fastestLapTime"], default=None)
        race_metrics.append({
            "raceId": race_id,
            "driverCount": len({entry["driverId"] for entry in entries}),
            "classifiedCount": sum(1 for entry in entries if entry["position"] > 0),
            "totalPoints": round(sum(entry["points"] for entry in entries), 1),
            "averageLapTimeMs": round(lap_data["totalMs"] / lap_data["count"]) if lap_data["count"] else None,
            "lapSamples": lap_data["count"],
            "winnerDriverId": winner["driverId"] if winner else None,
            "winnerConstructorId": winner["constructorId"] if winner else None,
            "fastestLapMs": fastest["fastestLapTime"] if fastest else None,
            "fastestLapDriverId": fastest["driverId"] if fastest else None,
        })

    driver_seasons: dict[tuple[int, int], dict] = {}
    constructor_seasons: dict[tuple[int, int], dict] = {}
    circuit_summary: dict[tuple[int, int], dict] = {}
    for entry in enriched_results:
        race = race_lookup.get(entry["raceId"])
        if not race:
            continue
        year = race["year"]
        driver_key = (entry["driverId"], year)
        driver_stats = driver_seasons.setdefault(driver_key, {"driverId": entry["driverId"], "year": year, "starts": 0, "points": 0.0, "wins": 0, "podiums": 0, "dnfs": 0, "finishTotal": 0, "classified": 0, "gridGainTotal": 0, "gridGainCount": 0})
        driver_stats["starts"] += 1
        driver_stats["points"] += entry["points"]
        driver_stats["wins"] += entry["positionOrder"] == 1
        driver_stats["podiums"] += 1 <= entry["positionOrder"] <= 3
        driver_stats["dnfs"] += entry["position"] == 0
        if entry["position"]:
            driver_stats["finishTotal"] += entry["position"]
            driver_stats["classified"] += 1
        if entry["grid"] > 0 and entry["position"] > 0:
            driver_stats["gridGainTotal"] += entry["grid"] - entry["position"]
            driver_stats["gridGainCount"] += 1

        constructor_key = (entry["constructorId"], year)
        constructor_stats = constructor_seasons.setdefault(constructor_key, {"constructorId": entry["constructorId"], "year": year, "starts": 0, "points": 0.0, "wins": 0, "podiums": 0, "dnfs": 0})
        constructor_stats["starts"] += 1
        constructor_stats["points"] += entry["points"]
        constructor_stats["wins"] += entry["positionOrder"] == 1
        constructor_stats["podiums"] += 1 <= entry["positionOrder"] <= 3
        constructor_stats["dnfs"] += entry["position"] == 0

        circuit_key = (race["circuitId"], year)
        circuit_stats = circuit_summary.setdefault(circuit_key, {"circuitId": race["circuitId"], "year": year, "races": 0, "winnerDriverId": None})
        if entry["positionOrder"] == 1:
            circuit_stats["winnerDriverId"] = entry["driverId"]

    for stats in driver_seasons.values():
        stats["points"] = round(stats["points"], 1)
        stats["averageFinish"] = round(stats["finishTotal"] / stats["classified"], 2) if stats["classified"] else None
        stats["averageGridGain"] = round(stats["gridGainTotal"] / stats["gridGainCount"], 2) if stats["gridGainCount"] else None
        del stats["finishTotal"], stats["classified"], stats["gridGainTotal"], stats["gridGainCount"]
    for stats in constructor_seasons.values():
        stats["points"] = round(stats["points"], 1)

    for race in race_lookup.values():
        key = (race["circuitId"], race["year"])
        if key in circuit_summary:
            circuit_summary[key]["races"] += 1

    # Constructor-level pace uses each driver's median among their fastest 25%
    # of recorded laps. This limits the influence of pit, safety-car, and traffic
    # laps while preserving a race-specific pace measure.
    weather_impact = []
    for race_id, entries in race_results.items():
        race = race_lookup.get(race_id)
        if not race or not race["weather"]["available"]:
            continue
        teams: dict[int, list[dict]] = defaultdict(list)
        team_entries: dict[int, list[dict]] = defaultdict(list)
        for entry in entries:
            team_entries[entry["constructorId"]].append(entry)
            lap_values = driver_lap_values.get((race_id, entry["driverId"]), [])
            if not lap_values:
                continue
            fastest_lap_count = max(1, ceil(len(lap_values) * 0.25))
            representative_lap_ms = round(median(sorted(lap_values)[:fastest_lap_count]))
            teams[entry["constructorId"]].append({
                "averageLapMs": entry["averageLapMs"],
                "representativeLapMs": representative_lap_ms,
                "lapSamples": entry["lapSamples"],
                "dnf": entry["position"] == 0,
            })

        team_rows = []
        for constructor_id, driver_rows in teams.items():
            result_entries = team_entries[constructor_id]
            driver_average_laps = [item["averageLapMs"] for item in driver_rows if item["averageLapMs"]]
            team_rows.append({
                "raceId": race_id,
                "year": race["year"],
                "round": race["round"],
                "date": race["date"],
                "raceName": race["name"],
                "circuitId": race["circuitId"],
                "circuitName": race["circuitName"],
                "country": race["country"],
                "constructorId": constructor_id,
                "condition": race["weather"]["condition"],
                "temperatureC": race["weather"]["temperatureC"],
                "precipitationMm": race["weather"]["precipitationMm"],
                "windSpeed": race["weather"]["windSpeed"],
                "observedAt": race["weather"]["observedAt"],
                "teamAverageLapMs": round(sum(driver_average_laps) / len(driver_average_laps)) if driver_average_laps else None,
                "representativeLapMs": round(sum(item["representativeLapMs"] for item in driver_rows) / len(driver_rows)),
                "lapSamples": sum(item["lapSamples"] for item in driver_rows),
                "driverCount": len(driver_rows),
                # Reliability uses every classified result entry, including a
                # car that retired before recording a usable timing lap.
                "dnfCount": sum(item["position"] == 0 for item in result_entries),
                "entryCount": len(result_entries),
            })

        if not team_rows:
            continue
        fastest_team_pace = min(item["representativeLapMs"] for item in team_rows)
        for item in team_rows:
            item["fastestTeamPaceMs"] = fastest_team_pace
            item["paceGapMs"] = item["representativeLapMs"] - fastest_team_pace
            item["paceGapPercent"] = round(100 * item["paceGapMs"] / fastest_team_pace, 3)
            weather_impact.append(item)

    qualifying_entries = [{
        "raceId": integer(row.get("raceId")), "driverId": integer(row.get("driverId")),
        "constructorId": integer(row.get("constructorId")), "position": integer(row.get("position")),
        "q1": lap_ms(row.get("q1")), "q2": lap_ms(row.get("q2")), "q3": lap_ms(row.get("q3")),
    } for row in qualifying]
    sprint_entries = [{
        "raceId": integer(row.get("raceId")), "driverId": integer(row.get("driverId")),
        "constructorId": integer(row.get("constructorId")), "position": integer(row.get("position")),
        "points": number(row.get("points")),
    } for row in sprint_results]

    metadata = {
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "demo": args.demo,
        "races": sorted(race_lookup.values(), key=lambda item: (item["year"], item["round"])),
        "drivers": sorted(driver_lookup.values(), key=lambda item: item["name"]),
        "constructors": sorted(constructor_lookup.values(), key=lambda item: item["name"]),
        "circuits": sorted(circuit_lookup.values(), key=lambda item: item["name"]),
        "seasons": sorted({race["year"] for race in race_lookup.values()}),
    }
    payloads = {
        "metadata": metadata,
        "results": enriched_results,
        "race_metrics": race_metrics,
        "driver_seasons": list(driver_seasons.values()),
        "constructor_seasons": list(constructor_seasons.values()),
        "circuit_summary": list(circuit_summary.values()),
        "qualifying": qualifying_entries,
        "sprint_results": sprint_entries,
        "weather_impact": weather_impact,
    }
    assert list(payloads) == OUTPUT_FILES
    for name, payload in payloads.items():
        write_json(output_dir, f"{name}.json", payload)
    write_bundle(output_dir, payloads)
    print(f"Built dashboard data for {len(metadata['races'])} races in {output_dir}")


if __name__ == "__main__":
    main()
