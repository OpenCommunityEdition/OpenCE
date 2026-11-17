import pytest
from tools.harness import build, constant, enum_with, function, read, run, structure


def damage_code():
    code = function(read("source/objects/damage.c"), "object_damage_aftermath")
    # Compile the actual dispatch block, retaining its immunity and type gates.
    start = code.index("\tif (object_definition->object.acceleration_scale")
    end = code.index("\n\tif (game_engine_can_score()", start)
    return (function(read("port/linux/game/mcc_damage.c"), "mcc_damage_acceleration") +
            "\nstatic void dispatch(void) {\n" + code[start:end] + "\n}\n")


def executable(code):
    definition = structure(read("source/objects/damage_effect_definitions.h"), "damage_definition")
    config = "#define MAXIMUM_NUMBER_OF_MATERIAL_TYPES %d\n" % constant(
        read("source/game/game_globals.h"), "MAXIMUM_NUMBER_OF_MATERIAL_TYPES")
    config += enum_with(read("source/objects/object_types.h"), "_object_type_biped")
    config += enum_with(read("source/units/units.h"), "_unit_impervious_bit")
    config += enum_with(read("source/objects/damage.c"), "_damage_detonates_explosives_bit")
    return build("mcc_damage", (("definition.inc", config + definition), ("under_test.inc", code)))


@pytest.mark.parametrize("case", ["lift", "negative", "legacy", "unflagged", "immune",
                                  "scalar", "zero", "nan", "overflow", "projectile",
                                  "item", "vehicle", "transition", "reverse", "vector_only", "negative_vector",
                                  "north", "up", "down", "zero_direction", "melee", "shotgun", "pull",
                                  "big_pad", "small_pad", "tiny", "threshold", "scaled"])
def test_damage(case):
    status, output = run(executable(damage_code()), case)
    assert status == 0, output


def test_missing_vector_regression():
    code = damage_code().replace("if (count == 3)", "if (count == 4)")
    status, output = run(executable(code), "lift")
    assert status == 1, output


def test_missing_unit_gate_regression():
    code = damage_code().replace("if (mcc_accelerate_unit &&", "if ((mcc_accelerate_unit && damage_effect->damage.instantaneous_acceleration > _real_epsilon) &&")
    assert code != damage_code()
    status, output = run(executable(code), "lift")
    assert status == 1, output


def test_world_space_regression():
    code = damage_code().replace(" * direction->i", "").replace(" * direction->j", "").replace(" * direction->k", "")
    status, output = run(executable(code), "shotgun")
    assert status == 1, output


def test_positive_component_gate_regression():
    code = damage_code().replace("values[0]*values[0] + values[1]*values[1] + values[2]*values[2] >= _real_epsilon",
                               "values[0] > _real_epsilon || values[1] > _real_epsilon || values[2] > _real_epsilon")
    status, output = run(executable(code), "negative_vector")
    assert status == 1, output
