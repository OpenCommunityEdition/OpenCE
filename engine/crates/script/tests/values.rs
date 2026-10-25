//! A script's values: the default of each type, a value taken as a number
//! or an index, and the casts `hs_cast` makes between types
//! (`source/hs/hs_runtime.c`).

use opence_content::ValueType;
use opence_script::{cast, Value, World};

/// A world whose objects and lists are numbered so a cast's path through
/// it can be read from its result.
struct Numbered;

impl World for Numbered {
    fn object_of_name(&mut self, name: i32) -> i32 {
        name + 100
    }

    fn object_list_of(&mut self, objects: &[i32]) -> i32 {
        objects.first().map_or(-1, |object| object + 1000)
    }

    fn object_list_of_ai(&mut self, ai: i32) -> i32 {
        ai + 2000
    }
}

fn cast_of(value: Value, from: ValueType, to: ValueType) -> Value {
    cast(&mut Numbered, value, from, to)
}

#[test]
fn each_type_starts_as_its_default() {
    for (value_type, default) in [
        (ValueType::Unparsed, Value::Void),
        (ValueType::Passthrough, Value::Void),
        (ValueType::Void, Value::Void),
        (ValueType::Boolean, Value::Boolean(false)),
        (ValueType::Real, Value::Real(0.0)),
        (ValueType::Short, Value::Short(0)),
        (ValueType::Long, Value::Long(0)),
        (ValueType::String, Value::String(String::new())),
        (ValueType::Script, Value::Index(-1)),
        (ValueType::ObjectList, Value::Index(-1)),
        (ValueType::Sound, Value::Tag(None)),
        (ValueType::ObjectDefinition, Value::Tag(None)),
        (ValueType::GameDifficulty, Value::Index(-1)),
        (ValueType::Unit, Value::Index(-1)),
        (ValueType::SceneryName, Value::Index(-1)),
    ] {
        assert_eq!(Value::default_of(value_type), default, "{value_type:?}");
    }
}

#[test]
fn a_value_is_taken_as_a_number_or_an_index() {
    for (value, real) in [
        (Value::Real(2.5), 2.5),
        (Value::Short(-3), -3.0),
        (Value::Long(70_000), 70_000.0),
        (Value::Index(-1), -1.0),
        (Value::Boolean(true), 1.0),
        (Value::Boolean(false), 0.0),
        (Value::String("4".into()), 0.0),
        (Value::Void, 0.0),
    ] {
        assert_eq!(value.as_real(), real, "{value:?}");
    }
    for (value, index) in [
        (Value::Index(5), 5),
        (Value::Long(6), 6),
        (Value::Short(-2), -2),
        (Value::Real(3.0), -1),
        (Value::Boolean(true), -1),
    ] {
        assert_eq!(value.as_index(), index, "{value:?}");
    }
}

#[test]
fn numbers_and_booleans_convert() {
    let boolean = ValueType::Boolean;
    for (value, from, expected) in [
        (Value::Real(0.5), ValueType::Real, true),
        (Value::Real(0.0), ValueType::Real, false),
        (Value::Short(3), ValueType::Short, true),
        (Value::Short(0), ValueType::Short, false),
        (Value::Long(-1), ValueType::Long, true),
        (Value::Long(0), ValueType::Long, false),
        (Value::String("x".into()), ValueType::String, true),
        (Value::String(String::new()), ValueType::String, false),
        // (a value whose type is not its node's: taken as it is)
        (Value::Boolean(true), ValueType::Short, true),
        (Value::Index(4), ValueType::Script, false),
    ] {
        assert_eq!(
            cast_of(value.clone(), from, boolean),
            Value::Boolean(expected),
            "{value:?}"
        );
    }
    assert_eq!(
        cast_of(Value::Short(2), ValueType::Short, ValueType::Real),
        Value::Real(2.0)
    );
    for (value, from, expected) in [
        (Value::Real(3.9), ValueType::Real, 3),
        (Value::Real(-3.9), ValueType::Real, -3),
        (Value::Long(-7), ValueType::Long, -7),
        (Value::Short(9), ValueType::Long, 9),
        (Value::Boolean(true), ValueType::Boolean, 0),
    ] {
        assert_eq!(
            cast_of(value.clone(), from, ValueType::Short),
            Value::Short(expected),
            "{value:?}"
        );
    }
    for (value, from, expected) in [
        (Value::Real(-2.7), ValueType::Real, -2),
        (Value::Short(-5), ValueType::Short, -5),
        (Value::Long(8), ValueType::Short, 8),
        (Value::String("1".into()), ValueType::String, 0),
    ] {
        assert_eq!(
            cast_of(value.clone(), from, ValueType::Long),
            Value::Long(expected),
            "{value:?}"
        );
    }
}

#[test]
fn anything_becomes_void_and_the_same_type_or_passthrough_is_kept() {
    assert_eq!(
        cast_of(Value::Real(1.0), ValueType::Real, ValueType::Void),
        Value::Void
    );
    assert_eq!(
        cast_of(Value::Short(1), ValueType::Short, ValueType::Short),
        Value::Short(1)
    );
    assert_eq!(
        cast_of(Value::Short(1), ValueType::Passthrough, ValueType::Real),
        Value::Short(1)
    );
}

#[test]
fn objects_their_names_and_encounters_become_lists() {
    let list = ValueType::ObjectList;
    for (value, from, expected) in [
        (Value::Index(3), ValueType::Ai, 2003),
        (Value::Index(4), ValueType::Unit, 1004),
        (Value::Index(5), ValueType::UnitName, 1105),
        (Value::Index(6), ValueType::Short, -1),
    ] {
        assert_eq!(
            cast_of(value, from, list),
            Value::Index(expected),
            "{from:?}"
        );
    }
}

#[test]
fn a_name_becomes_its_object_and_an_object_stays_itself() {
    for (value, from, to, expected) in [
        // (a name is looked up; an object as another object, or a name as
        // another name, is the same index)
        (
            Value::Index(5),
            ValueType::VehicleName,
            ValueType::Vehicle,
            Value::Index(105),
        ),
        (
            Value::Index(9),
            ValueType::Object,
            ValueType::Unit,
            Value::Index(9),
        ),
        (
            Value::Index(4),
            ValueType::ObjectName,
            ValueType::UnitName,
            Value::Index(4),
        ),
        // (casts Halo has no procedure for: the type's default)
        (
            Value::Long(2),
            ValueType::Long,
            ValueType::Script,
            Value::Index(-1),
        ),
        (
            Value::Long(2),
            ValueType::Long,
            ValueType::Unit,
            Value::Index(-1),
        ),
        (
            Value::Long(2),
            ValueType::Long,
            ValueType::UnitName,
            Value::Index(-1),
        ),
        (
            Value::Index(2),
            ValueType::Unit,
            ValueType::UnitName,
            Value::Index(-1),
        ),
        (
            Value::Index(2),
            ValueType::UnitName,
            ValueType::Sound,
            Value::Tag(None),
        ),
    ] {
        assert_eq!(cast_of(value, from, to), expected, "{from:?} as {to:?}");
    }
}
