"""Boat controls must reach the real force consumer independent of map family."""
import pytest
from tools.harness import build, enum_with, function, inline, mutated, read, run, structure


def boat_code():
    return function(read("source/units/vehicles.c"), "update_human_boat_physics")


def executable(code):
    physics = read("source/physics/physics.c")
    definitions = read("source/physics/physics_definitions.h")
    types = "\n".join([
        enum_with(definitions, "MAXIMUM_MASS_POINTS_PER_PHYSICS"),
        *(enum_with(physics, member) for member in (
            "_friction_type_point", "_point_at_rest_bit", "_powered_mass_point_ground_friction_bit")),
        structure(read("source/objects/objects.h"), "location"),
        structure(read("source/physics/powered_mass_point_datum.h"), "powered_mass_point_datum"),
        structure(read("source/physics/friction_datum.h"), "friction_datum"),
        structure(read("source/physics/mass_point_datum.h"), "mass_point_datum"),
        structure(definitions, "mass_point_definition"),
        structure(definitions, "physics_definition"),
        structure(physics, "powered_mass_point_definition"),
        structure(read("source/physics/physics.h"), "physics_instance"),
    ])
    vectors = "\n".join(inline(read("source/math/real_math.h"), name) for name in (
        "set_real_vector2d", "set_real_vector3d", "set_real_point3d", "point_from_line3d",
        "vector_from_points3d", "scale_vector3d", "dot_product3d", "cross_product3d",
        "add_vectors3d", "magnitude_squared3d"))
    consumer = "\n".join(function(physics, name) for name in (
        "pin_fraction", "physics_mass_point_count", "physics_powered_mass_point_count",
        "physics_powered_mass_point_index", "friction_evaluate", "physics_compute_new",
        "physics_update_old", "physics_update_new"))
    # Rename only the entry point so the harness can observe the passed controls.
    dispatch = function(physics, "physics_update").replace(
        "void physics_update(", "void production_physics_update(", 1)
    consumer += dispatch
    return build("mcc_boat", (("types.inc", types), ("vectors.inc", vectors),
                              ("consumer.inc", consumer), ("under_test.inc", code)))


def require_success(code, case):
    status, output = run(executable(code), case)
    assert status == 0, output


@pytest.mark.parametrize("path", ["new", "old"])
@pytest.mark.parametrize("case", [
    f"{family}_{poison}_{flags}"
    for family in ("mcc", "legacy") for poison in ("nan", "finite")
    for flags in ("water", "all", "unpowered")
])
def test_boat_controls_reach_finite_expected_forces(path, case):
    require_success(boat_code(), f"{case}_{path}")


@pytest.mark.parametrize("path", ["new", "old"])
@pytest.mark.parametrize("case", [
    f"{family}_nan_{case}"
    for family in ("mcc", "legacy") for case in ("zero", "reverse", "count0", "count2", "count4", "count32")
])
def test_propeller_values_and_other_point_counts(path, case):
    require_success(boat_code(), f"{case}_{path}")


@pytest.mark.parametrize("path", ["new", "old"])
def test_uninitialized_hydrofoils_regression(path):
    code = mutated(boat_code(), "csmemset(state, 0, 3 * sizeof(*state));", "(void)state;")
    status, output = run(executable(code), f"mcc_nan_water_{path}")
    assert status == 2 and "finite force" in output, output


@pytest.mark.parametrize("path", ["new", "old"])
def test_disabled_powered_flags_do_not_consume_poison(path):
    code = mutated(boat_code(), "csmemset(state, 0, 3 * sizeof(*state));", "(void)state;")
    status, output = run(executable(code), f"mcc_nan_unpowered_consumer_only_{path}")
    assert status == 0, output


@pytest.mark.parametrize("path", ["new", "old"])
@pytest.mark.parametrize("fault", ["propeller", "bounds"])
def test_active_control_and_scratch_bounds_regressions(path, fault):
    if fault == "propeller":
        code = mutated(boat_code(), "state[0].water_friction_velocity = vehicle->vehicle.speed;",
                       "state[0].water_friction_velocity = vehicle->vehicle.speed + 1.0f;")
        case = "legacy_nan_water"
    else:
        code = mutated(boat_code(), "3 * sizeof(*state)", "4 * sizeof(*state)")
        case = "mcc_nan_water"
    status, output = run(executable(code), f"{case}_{path}")
    assert status == 1 and "failed line" in output, output


@pytest.mark.parametrize("path", ["new", "old"])
@pytest.mark.parametrize("poison", ["nan", "finite"])
@pytest.mark.parametrize("flags", ["water", "all"])
def test_initialization_must_not_depend_on_map_family(path, poison, flags):
    # Reintroducing the former MCC-only correction must fail the legacy path.
    code = mutated(boat_code(), "csmemset(state, 0, 3 * sizeof(*state));",
                   "if (mcc_cache_tags_loaded()) csmemset(state, 0, 3 * sizeof(*state));")
    status, output = run(executable(code), f"legacy_{poison}_{flags}_{path}")
    assert status == 2, output
