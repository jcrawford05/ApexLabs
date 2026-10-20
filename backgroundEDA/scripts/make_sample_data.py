#!/usr/bin/env python3
"""Generate a small synthetic data set in the Kaggle CSV layout.

The drivers, teams, and results are invented; only the circuit list is real.
Use it to preview or test the dashboard without the Kaggle download:

    python scripts/make_sample_data.py --output-dir SampleData
    python scripts/build_data.py --data-dir SampleData --output-dir site/data --demo
"""

from __future__ import annotations

import argparse
import csv
import random
from pathlib import Path

# Default paths are relative to the repository root (the folder above scripts/),
# so the script works no matter which directory it is run from.
REPO_ROOT = Path(__file__).resolve().parent.parent

CIRCUITS = [
    ("Bahrain International Circuit", "Sakhir", "Bahrain", 26.0325, 50.5106, 92_000, "Bahrain Grand Prix"),
    ("Albert Park Grand Prix Circuit", "Melbourne", "Australia", -37.8497, 144.968, 81_000, "Australian Grand Prix"),
    ("Suzuka Circuit", "Suzuka", "Japan", 34.8431, 136.541, 93_000, "Japanese Grand Prix"),
    ("Circuit de Monaco", "Monte-Carlo", "Monaco", 43.7347, 7.42056, 75_000, "Monaco Grand Prix"),
    ("Circuit de Barcelona-Catalunya", "Montmeló", "Spain", 41.57, 2.26111, 79_000, "Spanish Grand Prix"),
    ("Circuit Gilles Villeneuve", "Montreal", "Canada", 45.5, -73.5228, 76_000, "Canadian Grand Prix"),
    ("Silverstone Circuit", "Silverstone", "UK", 52.0786, -1.01694, 90_000, "British Grand Prix"),
    ("Hungaroring", "Budapest", "Hungary", 47.5789, 19.2486, 81_000, "Hungarian Grand Prix"),
    ("Circuit de Spa-Francorchamps", "Spa", "Belgium", 50.4372, 5.97139, 108_000, "Belgian Grand Prix"),
    ("Autodromo Nazionale di Monza", "Monza", "Italy", 45.6156, 9.28111, 84_000, "Italian Grand Prix"),
    ("Marina Bay Street Circuit", "Marina Bay", "Singapore", 1.2914, 103.864, 98_000, "Singapore Grand Prix"),
    ("Circuit of the Americas", "Austin", "USA", 30.1328, -97.6411, 99_000, "United States Grand Prix"),
    ("Autódromo Hermanos Rodríguez", "Mexico City", "Mexico", 19.4042, -99.0907, 81_000, "Mexican Grand Prix"),
    ("Autódromo José Carlos Pace", "São Paulo", "Brazil", -23.7036, -46.6997, 74_000, "Brazilian Grand Prix"),
    ("Yas Marina Circuit", "Abu Dhabi", "UAE", 24.4672, 54.6031, 89_000, "Abu Dhabi Grand Prix"),
]
TEAMS = [("Vanta Racing", "British", 0.95), ("Scuderia Corsa", "Italian", 0.9), ("Nordwerk", "German", 0.88),
         ("Helix GP", "Austrian", 0.75), ("Meridian", "French", 0.65), ("Kestrel F1", "British", 0.6),
         ("Aurora Motorsport", "Swiss", 0.5), ("Tidal Racing", "American", 0.45), ("Sakura Works", "Japanese", 0.4),
         ("Cobalt", "Dutch", 0.35)]
FIRST = ["Luca", "Mara", "Theo", "Ines", "Kai", "Noor", "Elias", "Sasha", "Rafa", "Yuki", "Oskar", "Lena",
         "Mateo", "Ava", "Jonas", "Priya", "Felix", "Zoe", "Nico", "Iris", "Hugo", "Elif", "Tomas", "Rin"]
LAST = ["Varga", "Lindqvist", "Okafor", "Moreau", "Brandt", "Castillo", "Haddad", "Novak", "Ferreira", "Tanaka",
        "Kowalski", "Rossi", "Bauer", "Duarte", "Sorensen", "Mehta", "Klein", "Laurent", "Petrov", "Vidal",
        "Achterberg", "Yilmaz", "Silva", "Mori"]
NATIONALITIES = ["Italian", "Swedish", "Nigerian", "French", "German", "Spanish", "Lebanese", "Czech", "Portuguese",
                 "Japanese", "Polish", "Italian", "Austrian", "Brazilian", "Danish", "Indian", "German", "French",
                 "Bulgarian", "Spanish", "Dutch", "Turkish", "Brazilian", "Japanese"]
STATUSES = [(1, "Finished"), (3, "Accident"), (4, "Collision"), (5, "Engine"), (6, "Gearbox"), (9, "Hydraulics"),
            (11, "+1 Lap"), (12, "+2 Laps"), (20, "Spun off"), (22, "Suspension"), (130, "Collision damage")]
POINTS = [25, 18, 15, 12, 10, 8, 6, 4, 2, 1]


def write(path: Path, header: list[str], rows: list[list]) -> None:
    with path.open("w", encoding="utf-8", newline="") as target:
        writer = csv.writer(target)
        writer.writerow(header)
        writer.writerows(rows)


def lap_text(ms: int) -> str:
    return f"{ms // 60000}:{ms % 60000 / 1000:06.3f}"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=REPO_ROOT / "SampleData")
    parser.add_argument("--first-season", type=int, default=2016)
    parser.add_argument("--last-season", type=int, default=2024)
    parser.add_argument("--seed", type=int, default=7)
    args = parser.parse_args()
    rng = random.Random(args.seed)
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)

    write(out / "circuits.csv", ["circuitId", "circuitRef", "name", "location", "country", "lat", "lng", "alt", "url"],
          [[i, name.lower().split()[0], name, loc, country, lat, lng, 0, ""] for i, (name, loc, country, lat, lng, _, _) in enumerate(CIRCUITS, 1)])
    write(out / "constructors.csv", ["constructorId", "constructorRef", "name", "nationality", "url"],
          [[i, name.lower().replace(" ", "_"), name, nat, ""] for i, (name, nat, _) in enumerate(TEAMS, 1)])
    write(out / "drivers.csv", ["driverId", "driverRef", "number", "code", "forename", "surname", "dob", "nationality", "url"],
          [[i, LAST[i - 1].lower(), i + 1, LAST[i - 1][:3].upper(), FIRST[i - 1], LAST[i - 1], "1995-01-01", NATIONALITIES[i - 1], ""] for i in range(1, len(FIRST) + 1)])
    write(out / "status.csv", ["statusId", "status"], [list(row) for row in STATUSES])

    skill = {driver: rng.uniform(0.3, 1.0) for driver in range(1, len(FIRST) + 1)}
    roster: dict[int, list[int]] = {}
    pool = list(range(1, len(FIRST) + 1))
    races, results, laps, qualifying, sprints, weather = [], [], [], [], [], []
    race_id = result_id = qualify_id = sprint_id = 0
    for year in range(args.first_season, args.last_season + 1):
        if year == args.first_season or rng.random() < 0.6:  # occasional driver market reshuffle
            rng.shuffle(pool)
            roster = {team: pool[2 * (team - 1):2 * team] for team in range(1, len(TEAMS) + 1)}
        drivers = [(driver, team) for team, pair in roster.items() for driver in pair]
        era_pace = 1 - 0.004 * (year - args.first_season)
        for round_number, circuit_index in enumerate(rng.sample(range(len(CIRCUITS)), 12), 1):
            race_id += 1
            name, _, _, _, _, base_lap, gp = CIRCUITS[circuit_index]
            date = f"{year}-{3 + round_number * 8 // 12:02d}-{rng.randint(1, 28):02d}"
            races.append([race_id, year, round_number, circuit_index + 1, gp, date, "14:00:00", ""])
            wet = rng.random() < 0.18
            weather.append([f"{date} 14:00:00", round_number, round(rng.uniform(14, 34), 1), round(rng.uniform(0.5, 6), 1) if wet else 0, round(rng.uniform(4, 22), 1)])
            strength = {driver: TEAMS[team - 1][2] * 0.65 + skill[driver] * 0.35 + rng.gauss(0, 0.08) for driver, team in drivers}
            grid_order = sorted(drivers, key=lambda d: -(strength[d[0]] + rng.gauss(0, 0.05)))
            retirements = {driver for driver, _ in drivers if rng.random() < 0.09}
            # Classified finishers first, then retirements (which keep a positionOrder but no position).
            finish_order = sorted(drivers, key=lambda d: (d[0] in retirements, -(strength[d[0]] + rng.gauss(0, 0.1))))
            total_laps = rng.randint(50, 70)
            fastest_ms = {}
            for position, (driver, team) in enumerate(finish_order, 1):
                retired = driver in retirements
                status = rng.choice(STATUSES[1:6] + STATUSES[8:]) if retired else (STATUSES[0] if position <= 12 else STATUSES[6 + (position > 17)])
                pace = round(base_lap * era_pace * (1.035 - 0.03 * strength[driver]) * (1.08 if wet else 1))
                driver_laps = rng.randint(3, total_laps - 5) if retired else total_laps - (status[0] - 10 if status[0] in (11, 12) else 0)
                for lap in range(1, driver_laps + 1, 3):  # every third lap keeps the sample small
                    laps.append([race_id, driver, lap, position, "", pace + rng.randint(-900, 2600) + (18000 if lap == 1 else 0)])
                fastest_ms[driver] = pace - rng.randint(600, 1500)
                grid = next(i for i, (d, _) in enumerate(grid_order, 1) if d == driver)
                result_id += 1
                results.append([result_id, race_id, driver, team, driver + 1, grid,
                                "\\N" if retired else position, "R" if retired else position, position,
                                0 if retired or position > 10 else POINTS[position - 1], driver_laps, "\\N", "\\N",
                                rng.randint(30, max(31, driver_laps)), 0, lap_text(fastest_ms[driver]), round(rng.uniform(190, 235), 3), status[0]])
            for position, (driver, team) in enumerate(grid_order, 1):
                qualify_id += 1
                q1 = fastest_ms[driver] - rng.randint(0, 800)
                qualifying.append([qualify_id, race_id, driver, team, driver + 1, position, lap_text(q1),
                                   lap_text(q1 - 300) if position <= 15 else "\\N", lap_text(q1 - 600) if position <= 10 else "\\N"])
            if year >= 2021 and round_number % 4 == 0:
                for position, (driver, team) in enumerate(finish_order, 1):
                    sprint_id += 1
                    sprints.append([sprint_id, race_id, driver, team, driver + 1, grid_order.index((driver, team)) + 1, position, max(0, 9 - position), "Finished"])

    write(out / "races.csv", ["raceId", "year", "round", "circuitId", "name", "date", "time", "url"], races)
    write(out / "results.csv", ["resultId", "raceId", "driverId", "constructorId", "number", "grid", "position", "positionText",
                                "positionOrder", "points", "laps", "time", "milliseconds", "fastestLap", "rank", "fastestLapTime",
                                "fastestLapSpeed", "statusId"], results)
    write(out / "lap_times.csv", ["raceId", "driverId", "lap", "position", "time", "milliseconds"], laps)
    write(out / "qualifying.csv", ["qualifyId", "raceId", "driverId", "constructorId", "number", "position", "q1", "q2", "q3"], qualifying)
    write(out / "sprint_results.csv", ["resultId", "raceId", "driverId", "constructorId", "number", "grid", "position", "points", "status"], sprints)
    write(out / "weather_features_v4.csv", ["datetime", "round", "temperature", "precipitation", "windspeed"], weather)
    (out / "README.txt").write_text("Synthetic sample data generated by scripts/make_sample_data.py. Not real Formula 1 results.\n", encoding="utf-8")
    print(f"Wrote {len(races)} synthetic races to {out}")


if __name__ == "__main__":
    main()
