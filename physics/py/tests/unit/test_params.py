import json
import shutil

import pytest

from apexphys import units
from apexphys.params import (
    DEFAULT_DATA_DIR,
    ParamError,
    default_environment,
    default_solver_config,
    load_car,
    load_physics_data,
    load_track,
)
from conftest import SAMPLE_CAR, SAMPLE_TRACK


def write_json(path, doc):
    path.write_text(json.dumps(doc), encoding="utf-8")
    return path


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


# ----------------------------------------------------------- parameter files


def test_physics_data_loads_every_table(data):
    assert set(data.power_units) == {"V6 Hybrid Spec B"}
    assert set(data.tire_compounds) == {
        "Soft Compound",
        "Medium Compound",
        "Hard Compound",
    }
    assert set(data.aero_packages) == {"Low Downforce Package"}
    assert set(data.chassis) == {"Standard Monocoque"}
    assert set(data.brake_sets) == {"Carbon Disc Set"}
    assert list(data.eras) == [
        "Apex 2026 Default",
        "Apex 2026 Low Budget",
        "Open (no limits)",
    ]
    assert data.calibration.sea_level_pressure_pa == 101325.0


def test_parameter_values_are_converted_to_si(data):
    engine = data.power_units["V6 Hybrid Spec B"]
    assert len(engine.gear_ratios) == 8
    assert engine.map_scales == {"Qualifying": 1.0, "Race": 0.97, "Save": 0.92}
    first_omega, first_torque = engine.torque_curve[0]
    assert units.rad_s_to_rpm(first_omega) == pytest.approx(4000.0)
    assert first_torque == 869.5
    assert engine.torque_curve[-1][0] == pytest.approx(engine.max_omega_rad_s)
    assert units.rad_s_to_rpm(engine.max_omega_rad_s) == pytest.approx(15000.0)
    medium = data.tire_compounds["Medium Compound"]
    assert medium.t_opt_k == pytest.approx(373.15)
    assert medium.wear_rate_per_m == pytest.approx(2.5e-6)


def test_compound_grip_ordering(data):
    mu = [data.tire_compounds[f"{n} Compound"].mu_ref for n in ("Soft", "Medium", "Hard")]
    assert mu == sorted(mu, reverse=True)


def test_default_era_rules(data):
    era = data.default_era
    assert era.name == "Apex 2026 Default"
    assert "Issue 21" in era.source
    expected = {
        "min_mass_no_fuel": (724.0, "kg"),
        "min_driver_mass": (82.0, "kg"),
        "wheelbase_max": (3.40, "m"),
        "width_max": (1.90, "m"),
        "fuel_energy_flow_max": (3000.0, "MJ/h"),
        "mguk_power_max": (350.0, "kW"),
        "engine_mass_min": (185.0, "kg"),
        "forward_gears": (8.0, "-"),
    }
    for key, (value, unit) in expected.items():
        assert (era.rules[key].value, era.rules[key].unit) == (value, unit)
    assert era.rules["min_mass_no_fuel"].severity == "hard"
    assert era.rules["mguh_allowed"].value is False
    assert all(rule.source_article.startswith("C") for rule in era.rules.values())
    assert data.eras["Open (no limits)"].is_sandbox
    assert data.eras["Open (no limits)"].rules == {}


def test_missing_data_directory(tmp_path):
    with pytest.raises(ParamError, match="cannot read"):
        load_physics_data(tmp_path / "nowhere")


def test_wrong_schema_version(tmp_path):
    shutil.copytree(DEFAULT_DATA_DIR, tmp_path / "data")
    path = tmp_path / "data" / "chassis.json"
    doc = read_json(path)
    doc["schema_version"] = 99
    write_json(path, doc)
    with pytest.raises(ParamError, match="schema_version is 99"):
        load_physics_data(tmp_path / "data")


def test_unknown_default_era_is_named(tmp_path):
    shutil.copytree(DEFAULT_DATA_DIR, tmp_path / "data")
    path = tmp_path / "data" / "regulation_eras.json"
    doc = read_json(path)
    doc["default_era"] = "Group B"
    write_json(path, doc)
    with pytest.raises(ParamError, match="unknown regulation era 'Group B'"):
        load_physics_data(tmp_path / "data")


def test_default_chassis_respects_the_default_era(data):
    # Decision D3: stock parts follow the 2026 rules.
    rules = data.default_era.rules
    chassis = data.chassis["Standard Monocoque"]
    assert chassis.wheelbase_m <= rules["wheelbase_max"].value
    assert chassis.driver_mass_kg >= rules["min_driver_mass"].value


def test_missing_parameter_is_named(tmp_path):
    shutil.copytree(DEFAULT_DATA_DIR, tmp_path / "data")
    path = tmp_path / "data" / "power_units.json"
    doc = read_json(path)
    del doc["power_units"][0]["max_rpm"]
    write_json(path, doc)
    with pytest.raises(ParamError, match=r"power_units\[0\]: missing 'max_rpm'"):
        load_physics_data(tmp_path / "data")


def edited_power_unit(tmp_path, edit):
    shutil.copytree(DEFAULT_DATA_DIR, tmp_path / "data")
    path = tmp_path / "data" / "power_units.json"
    doc = read_json(path)
    edit(doc["power_units"][0])
    write_json(path, doc)
    return tmp_path / "data"


def test_gear_ratios_must_decrease(tmp_path):
    def edit(pu):
        pu["gear_ratios"][3] = 9.0

    with pytest.raises(ParamError, match="'gear_ratios' must be positive and decreasing"):
        load_physics_data(edited_power_unit(tmp_path, edit))


def test_torque_curve_must_reach_the_rev_limit(tmp_path):
    def edit(pu):
        pu["torque_curve_rpm_nm"] = pu["torque_curve_rpm_nm"][:-2]

    with pytest.raises(ParamError, match="must reach max_rpm"):
        load_physics_data(edited_power_unit(tmp_path, edit))


def test_torque_curve_rpm_must_increase(tmp_path):
    def edit(pu):
        pu["torque_curve_rpm_nm"][5][0] = 4000

    with pytest.raises(ParamError, match="rpm values must increase"):
        load_physics_data(edited_power_unit(tmp_path, edit))


def test_missing_part_position_is_named(tmp_path):
    shutil.copytree(DEFAULT_DATA_DIR, tmp_path / "data")
    path = tmp_path / "data" / "chassis.json"
    doc = read_json(path)
    del doc["chassis"][0]["position_frac"]["fuel"]
    write_json(path, doc)
    with pytest.raises(ParamError, match="position_frac: missing 'fuel'"):
        load_physics_data(tmp_path / "data")


# ----------------------------------------------------------------- car files


def test_sample_car(data):
    car = load_car(SAMPLE_CAR, data)
    assert car.name == "Low Drag Monza Setup"
    assert car.era.name == "Apex 2026 Default"
    assert car.power_unit.name == "V6 Hybrid Spec B"
    assert car.tire.short_name == "Medium"
    assert car.aero.drag_area_m2 == 1.10
    assert car.chassis.wheelbase_m == 3.40
    assert car.chassis.m_base_kg == 540.0
    assert car.chassis.driver_mass_kg == 82.0
    assert car.brakes.mass_kg == 40.0


def test_sample_car_setup_is_si(data):
    setup = load_car(SAMPLE_CAR, data).setup
    assert units.rad_to_deg(setup.front_wing_rad) == pytest.approx(8.0)
    assert units.rad_to_deg(setup.rear_wing_rad) == pytest.approx(12.0)
    assert setup.front_ride_height_m == pytest.approx(0.031)
    assert setup.floor_setting == pytest.approx(0.62)
    assert setup.pressure_front_pa == pytest.approx(22.5 * 6894.757)
    assert setup.pressure_rear_pa == pytest.approx(21.8 * 6894.757)
    assert setup.ballast_kg == 5.0


def test_setup_fields_missing_from_the_file_use_ui_defaults(data):
    setup = load_car(SAMPLE_CAR, data).setup
    assert setup.rear_ride_height_m == pytest.approx(0.078)
    assert setup.power_map == "Race"
    assert setup.final_drive == 3.2
    assert units.rad_s_to_rpm(setup.shift_omega_rad_s) == pytest.approx(14500.0)
    assert setup.ballast_front_frac == 0.5
    assert setup.brake_bias_front == pytest.approx(0.57)
    assert setup.brake_cooling == "Standard"


def edited_car(tmp_path, edit):
    doc = read_json(SAMPLE_CAR)
    edit(doc)
    return write_json(tmp_path / "car.json", doc)


def test_unknown_component_is_named(tmp_path, data):
    def edit(doc):
        doc["components"]["engine"]["name"] = "V12 Screamer"

    with pytest.raises(ParamError, match="unknown engine 'V12 Screamer'"):
        load_car(edited_car(tmp_path, edit), data)


def test_unknown_era_is_named(tmp_path, data):
    def edit(doc):
        doc["regulation"] = "Group B"

    with pytest.raises(ParamError, match="unknown regulation era 'Group B'"):
        load_car(edited_car(tmp_path, edit), data)


def test_misspelled_setting_is_rejected(tmp_path, data):
    def edit(doc):
        doc["setup"]["front_wing_deg"] = 9.0

    with pytest.raises(ParamError, match=r"unknown setting\(s\) front_wing_deg"):
        load_car(edited_car(tmp_path, edit), data)


def test_invalid_power_map_is_rejected(tmp_path, data):
    def edit(doc):
        doc["setup"]["power_map"] = "Party"

    with pytest.raises(ParamError, match="'power_map' is 'Party'"):
        load_car(edited_car(tmp_path, edit), data)


def test_non_numeric_setting_is_rejected(tmp_path, data):
    def edit(doc):
        doc["setup"]["ballast_kg"] = "five"

    with pytest.raises(ParamError, match="'ballast_kg' must be a number"):
        load_car(edited_car(tmp_path, edit), data)


def test_missing_car_file(tmp_path, data):
    with pytest.raises(ParamError, match="cannot read"):
        load_car(tmp_path / "missing.json", data)


def test_invalid_json(tmp_path, data):
    path = tmp_path / "car.json"
    path.write_text("{ not json", encoding="utf-8")
    with pytest.raises(ParamError, match="invalid JSON"):
        load_car(path, data)


# --------------------------------------------------------------- track files


def test_demo_circuit():
    track = load_track(SAMPLE_TRACK)
    assert track.name == "Demo Circuit"
    assert len(track.segments) == 12
    assert track.length_m == 4000.0
    assert track.corner_count == 4
    assert track.sector_count == 3


def test_curvature_is_zero_except_in_corners():
    track = load_track(SAMPLE_TRACK)
    hairpin = track.segments[2]
    assert hairpin.name == "T1 hairpin"
    assert hairpin.curvature_per_m == pytest.approx(1.0 / 25.0)
    for seg in track.segments:
        if seg.type != "corner":
            assert seg.curvature_per_m == 0.0
            assert seg.radius_m == 0.0


def edited_track(tmp_path, edit):
    doc = read_json(SAMPLE_TRACK)
    edit(doc)
    return write_json(tmp_path / "track.json", doc)


def test_corner_without_radius_is_rejected(tmp_path):
    def edit(doc):
        del doc["segments"][2]["radius_m"]

    with pytest.raises(ParamError, match=r"segments\[2\]: missing 'radius_m'"):
        load_track(edited_track(tmp_path, edit))


def test_unknown_segment_type_is_rejected(tmp_path):
    def edit(doc):
        doc["segments"][0]["type"] = "chicane"

    with pytest.raises(ParamError, match="'type' is 'chicane'"):
        load_track(edited_track(tmp_path, edit))


def test_non_positive_length_is_rejected(tmp_path):
    def edit(doc):
        doc["segments"][0]["length_m"] = 0

    with pytest.raises(ParamError, match="'length_m' must be positive"):
        load_track(edited_track(tmp_path, edit))


def test_out_of_order_segments_are_rejected(tmp_path):
    def edit(doc):
        doc["segments"][0], doc["segments"][1] = doc["segments"][1], doc["segments"][0]

    with pytest.raises(ParamError, match="'seq' is 2, expected 1"):
        load_track(edited_track(tmp_path, edit))


def test_empty_track_is_rejected(tmp_path):
    def edit(doc):
        doc["segments"] = []

    with pytest.raises(ParamError, match="no segments"):
        load_track(edited_track(tmp_path, edit))


# ------------------------------------------------- environment and run setup


def test_default_environment_matches_ui_defaults(data):
    env = default_environment(data)
    assert env.preset == "Dry"
    assert units.k_to_c(env.air_temp_k) == pytest.approx(24.0)
    assert units.k_to_c(env.track_temp_k) == pytest.approx(38.0)
    assert env.relative_humidity == pytest.approx(0.42)
    assert units.ms_to_kmh(env.wind_speed_m_s) == pytest.approx(12.0)
    assert env.rain_frac == 0.0
    assert env.grip_state == "Rubbered-in"
    assert env.roughness_frac == pytest.approx(0.30)
    assert env.track_evolution is True
    assert env.sea_level_pressure_pa == 101325.0


def test_default_solver_config_matches_ui_defaults():
    config = default_solver_config()
    assert config.laps == 1
    assert config.standing_start is False
    assert config.start_fuel_kg == 30.0
    assert config.telemetry_spacing_m == 5.0
    assert config.ds_m == 2.0
