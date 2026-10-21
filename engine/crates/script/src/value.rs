//! A script's values, the default of each type, and the casts between
//! types Halo makes when a value is taken as another type
//! (`source/hs/hs_runtime.c`, its table of casting procedures).

use opence_content::{Constant, TagRef, ValueType};

use crate::World;

/// A value a script computes.
#[derive(Clone, Debug, PartialEq)]
pub enum Value {
    /// Nothing.
    Void,
    /// A boolean.
    Boolean(bool),
    /// A real number.
    Real(f32),
    /// A short integer.
    Short(i16),
    /// A long integer.
    Long(i32),
    /// A string.
    String(String),
    /// An index: of an object, an object list, a script, or something of
    /// the scenario's or the game's named in the script (`-1` for none).
    Index(i32),
    /// A tag, or none.
    Tag(Option<TagRef>),
}

impl Value {
    /// The value a type starts as, and what a function that is not there
    /// returns.
    pub fn default_of(value_type: ValueType) -> Self {
        match value_type {
            ValueType::Boolean => Self::Boolean(false),
            ValueType::Real => Self::Real(0.0),
            ValueType::Short => Self::Short(0),
            ValueType::Long => Self::Long(0),
            ValueType::String => Self::String(String::new()),
            t if t.is_tag() => Self::Tag(None),
            t if t <= ValueType::Void => Self::Void,
            _ => Self::Index(-1),
        }
    }

    /// The value written in a script.
    pub fn of_constant(constant: &Constant) -> Self {
        match constant {
            Constant::Boolean(value) => Self::Boolean(*value),
            Constant::Real(value) => Self::Real(*value),
            Constant::Short(value) => Self::Short(*value),
            Constant::Long(value) => Self::Long(*value),
            Constant::String(value) => Self::String(value.clone()),
            Constant::Named(named) => Self::Index(named.index),
            Constant::Tag(tag) => Self::Tag(tag.clone()),
        }
    }

    /// It as a boolean (false for anything not one).
    pub fn as_boolean(&self) -> bool {
        matches!(self, Self::Boolean(true))
    }

    /// It as a real number, as Halo's arithmetic takes it.
    pub fn as_real(&self) -> f32 {
        match self {
            Self::Real(value) => *value,
            Self::Short(value) => f32::from(*value),
            // (as Halo's casts do: a long's nearest real)
            #[allow(clippy::cast_precision_loss)]
            Self::Long(value) | Self::Index(value) => *value as f32,
            Self::Boolean(value) => f32::from(u8::from(*value)),
            _ => 0.0,
        }
    }

    /// It as an index (`-1` for anything not one).
    pub fn as_index(&self) -> i32 {
        match self {
            Self::Index(value) | Self::Long(value) => *value,
            Self::Short(value) => i32::from(*value),
            _ => -1,
        }
    }
}

/// A value of one type taken as another, as `hs_cast` takes it: numbers
/// convert, anything becomes void, an object's name becomes its object, and
/// an object, its name or an encounter becomes an object list. A cast Halo
/// has no procedure for gives the type's default.
pub fn cast<W: World + ?Sized>(
    world: &mut W,
    value: Value,
    from: ValueType,
    to: ValueType,
) -> Value {
    if from == to || from == ValueType::Passthrough {
        return value;
    }
    match to {
        ValueType::Void => Value::Void,
        ValueType::Boolean => Value::Boolean(match value {
            Value::Real(v) => v != 0.0,
            Value::Short(v) => v != 0,
            Value::Long(v) => v != 0,
            Value::String(v) => !v.is_empty(),
            Value::Boolean(v) => v,
            _ => false,
        }),
        ValueType::Real => Value::Real(value.as_real()),
        // (Rust's casts saturate where C's would not; scripts' numbers stay
        // well inside a short)
        #[allow(clippy::cast_possible_truncation)]
        ValueType::Short => Value::Short(match value {
            Value::Real(v) => v as i16,
            Value::Long(v) => v as i16,
            Value::Short(v) => v,
            _ => 0,
        }),
        #[allow(clippy::cast_possible_truncation)]
        ValueType::Long => Value::Long(match value {
            Value::Real(v) => v as i32,
            Value::Short(v) => i32::from(v),
            Value::Long(v) => v,
            _ => 0,
        }),
        ValueType::ObjectList => {
            let index = value.as_index();
            if from == ValueType::Ai {
                Value::Index(world.object_list_of_ai(index))
            } else if from.is_object() {
                Value::Index(world.object_list_of(&[index]))
            } else if from.is_object_name() {
                let object = world.object_of_name(index);
                Value::Index(world.object_list_of(&[object]))
            } else {
                Value::default_of(to)
            }
        }
        t if t.is_object() && from.is_object_name() => {
            Value::Index(world.object_of_name(value.as_index()))
        }
        // (an object as a unit, a name as a unit's name: the same index)
        t if (t.is_object() && from.is_object())
            || (t.is_object_name() && from.is_object_name()) =>
        {
            value
        }
        _ => Value::default_of(to),
    }
}
