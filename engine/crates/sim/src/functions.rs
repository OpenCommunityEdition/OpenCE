//! Objects' functions (`object_compute_function_values`): each tick, an
//! object's four incoming values are exported from its state (its
//! vitality, a unit's flashlight, a weapon's heat and rounds), and its
//! functions turn them and time into four outgoing values, which scale its
//! lights and attachments.

use std::collections::BTreeMap;

use bevy_ecs::prelude::*;
use opence_content::{FunctionModes, Level, ObjectFunction, ObjectKind, TagRef};

use crate::armory::WeaponItem;
use crate::damage::{transition, Vitality};
use crate::objects::Object;
use crate::periodic::evaluate;
use crate::{ObjectId, Tick};

/// An object's values: what it exports into its functions and what they
/// make of them.
#[derive(Component, Clone, Copy, Debug, Default, PartialEq)]
pub struct FunctionValues {
    /// Its incoming values, A to D.
    pub incoming: [f32; 4],
    /// Its functions' values.
    pub outgoing: [f32; 4],
    /// Which of its functions are on.
    pub active: [bool; 4],
}

impl FunctionValues {
    /// The value an index names: 1 to 4 an incoming value, 5 to 8 an
    /// outgoing one, anything else 0.
    pub fn value(&self, index: i16) -> f32 {
        match index {
            1..=4 => self.incoming[(index - 1) as usize],
            5..=8 => self.outgoing[(index - 5) as usize],
            _ => 0.0,
        }
    }
}

/// A unit's flashlight (`units.c`): on or off, how bright it is now, and
/// the battery it runs on.
#[derive(Component, Clone, Copy, Debug, PartialEq)]
pub struct IntegratedLight {
    /// It has been switched on.
    pub on: bool,
    /// How bright it is, 0 to 1 (it brightens and dims over a few ticks).
    pub power: f32,
    /// What is left of its battery, 0 to 1.
    pub battery: f32,
}

impl Default for IntegratedLight {
    fn default() -> Self {
        Self {
            on: false,
            power: 0.0,
            battery: 1.0,
        }
    }
}

/// The ticks a full battery lasts.
const BATTERY_TICKS: f32 = 2.0 * 1800.0;
/// The ticks an empty battery takes to fill.
const RECHARGE_TICKS: f32 = 900.0;
/// The battery a flashlight needs to be switched on.
const BATTERY_TO_SWITCH_ON: f32 = 0.2;

impl IntegratedLight {
    /// One tick: switched when `pressed` (or when its battery runs out)
    /// unless its unit is `seated`; off once its unit is seated or dead.
    pub fn step(&mut self, pressed: bool, seated: bool, dead: bool) {
        if (pressed || self.battery <= 0.0)
            && (self.on || self.battery > BATTERY_TO_SWITCH_ON)
            && !seated
        {
            self.on = !self.on;
        }
        if self.on {
            self.battery = (self.battery - 1.0 / BATTERY_TICKS).max(0.0);
            if seated || dead {
                self.on = false;
            }
            self.power = (self.power + 1.0 / 6.0).min(1.0);
        } else {
            self.battery = (self.battery + 1.0 / RECHARGE_TICKS).min(1.0);
            self.power = (self.power - 1.0 / 24.0).max(0.0);
        }
    }
}

/// An object definition's functions and where their values come from.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Functions {
    /// Where its incoming values come from.
    pub modes: FunctionModes,
    /// Its functions.
    pub functions: Vec<ObjectFunction>,
    /// A weapon's first magazine's most rounds loaded (its ammunition's
    /// value is the rounds loaded over this).
    pub rounds_loaded_maximum: i16,
}

/// The level's objects' functions, by their tags (only those that have
/// some).
#[derive(Resource, Clone, Debug, Default)]
pub struct FunctionTable(pub BTreeMap<TagRef, Functions>);

impl FunctionTable {
    /// Every object definition of `level` that exports or computes
    /// anything.
    pub fn new(level: &Level) -> Self {
        let mut table = BTreeMap::new();
        for object in &level.objects {
            let functions = Functions {
                modes: object.function_modes,
                functions: object.functions.clone(),
                rounds_loaded_maximum: object
                    .weapon
                    .as_ref()
                    .and_then(|weapon| weapon.magazines.first())
                    .map_or(0, |magazine| magazine.rounds_loaded_maximum),
            };
            if functions.functions.is_empty() && functions.modes == FunctionModes::default() {
                continue;
            }
            table.insert(object.tag.clone(), functions);
        }
        Self(table)
    }
}

/// What an object's state exports into its functions.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Exported {
    /// Its body and shield vitality, if it can be damaged.
    pub vitality: Option<(f32, f32, bool)>,
    /// A constant of its own, 0 to 1.
    pub random: f32,
    /// A unit's flashlight's power.
    pub integrated_light: f32,
    /// A weapon's heat, rounds loaded over its maximum, and whether it is
    /// cooling from overheating.
    pub weapon: Option<(f32, f32, bool)>,
}

/// The value an object's part exports by `mode` (none: it leaves the
/// value as it is), for the object's own modes, a unit's, a biped's and a
/// weapon's (`object_export_function_values` and the rest).
fn export(part: usize, mode: i16, exported: &Exported) -> Option<f32> {
    if mode == 0 {
        return None;
    }
    let (body, shield, dead) = exported.vitality.unwrap_or((1.0, 0.0, false));
    let value = match (part, mode) {
        (0, 1) => body.clamp(0.0, 1.0),
        (0, 2) => shield.clamp(0.0, 1.0),
        (0, 5) => exported.random,
        (0, 18) => f32::from(u8::from(!dead)),
        (1, 5) => exported.integrated_light,
        (1, 6) => f32::from(u8::from(!dead)),
        (3, 1) => exported.weapon.map_or(0.0, |weapon| weapon.0),
        (3, 2 | 3) => exported.weapon.map_or(1.0, |weapon| weapon.1),
        (3, 6) => 1.0,
        (3, 9) => exported
            .weapon
            .map_or(0.0, |weapon| f32::from(u8::from(weapon.2))),
        // (what the port does not keep yet: recent damage, the compass, a
        // region's damage, seat power, aiming, flying speed, rate of fire)
        _ => 0.0,
    };
    Some(value)
}

/// The incoming values `modes` export from `exported`: the object's
/// first, then its parts', which override where they export anything.
pub fn incoming(modes: &FunctionModes, exported: &Exported) -> [f32; 4] {
    let mut values = [0.0; 4];
    for (part, modes) in [modes.object, modes.unit, modes.biped, modes.weapon]
        .iter()
        .enumerate()
    {
        for (value, &mode) in values.iter_mut().zip(modes) {
            if let Some(exported) = export(part, mode, exported) {
                *value = exported;
            }
        }
    }
    values
}

/// A value below a bound by this little is at it.
const EPSILON: f32 = 0.0001;

/// `functions`' values and which are on, for an object whose index is
/// `index` at tick `tick`, with incoming values `incoming` and the last
/// tick's values `previous` (which additive functions add to).
pub fn compute(
    functions: &[ObjectFunction],
    incoming: [f32; 4],
    previous: [f32; 4],
    index: u32,
    tick: u64,
) -> ([f32; 4], [bool; 4]) {
    let mut values = FunctionValues {
        incoming,
        ..FunctionValues::default()
    };
    // (each object's functions start from its own place in their period)
    let seconds = (57.0 * index as f32 + tick as f32) * (1.0 / 30.0);
    for (at, function) in functions.iter().enumerate().take(4) {
        let mut period = function.one_over_period;
        if function.scale_period_by != 0 {
            let scale = values.value(function.scale_period_by);
            if scale > 0.0 {
                period /= scale;
            }
        }
        let mut value = evaluate(function.function, seconds * period);
        if function.scale_function_by != 0 {
            value *= values.value(function.scale_function_by);
        }
        if function.flags & ObjectFunction::INVERT != 0 {
            value = 1.0 - value;
        }
        if function.wobble_magnitude != 0.0 {
            let wobble = evaluate(function.wobble_function, seconds * function.wobble_period);
            value += 2.0 * function.wobble_magnitude * (wobble - 0.5);
        }
        if function.square_wave_threshold != 0.0 {
            value = if value > function.square_wave_threshold {
                1.0
            } else {
                0.0
            };
        }
        if function.step_count > 1 {
            value = libm::floorf(f32::from(function.step_count) * value)
                * function.reciprocal_step_count;
        }
        if function.reciprocal_sawtooth_count > 0.0 {
            value = libm::fmodf(value, function.reciprocal_sawtooth_count);
        }
        if function.add != 0 {
            value = (value + values.value(function.add)).min(1.0);
        }
        if function.scale_result_by != 0 {
            value *= values.value(function.scale_result_by);
        }
        let mut out = transition(function.map_to, value);
        if function.scale_by > 0.0 {
            out *= function.scale_by;
        }
        let always = function.flags & ObjectFunction::ALWAYS_ACTIVE != 0;
        let [lower, upper] = function.bounds;
        let mut active = true;
        if function.bounds_mode == 2 {
            out = out * (upper - lower) + lower;
            if lower + EPSILON >= out && !always {
                active = false;
            }
        } else {
            if lower + EPSILON >= out {
                out = lower;
                active = always;
            }
            out = out.min(upper);
            if function.bounds_mode == 1 {
                out = (out - lower) * function.reciprocal_bounds_range;
            }
        }
        if let Ok(other) = usize::try_from(function.turn_off_with) {
            if other < at && !values.active[other] {
                active = false;
            }
        }
        if function.flags & ObjectFunction::ADDITIVE != 0 {
            out = libm::fmodf(out + previous[at], 1.0);
        }
        values.outgoing[at] = out;
        values.active[at] = active;
    }
    (values.outgoing, values.active)
}

/// A constant of the object `id`'s own, 0 to 1 (the same each run).
fn random_constant(id: u32) -> f32 {
    let mixed = id.wrapping_mul(0x9E37_79B9) ^ (id >> 16);
    (mixed >> 8) as f32 / (1u32 << 24) as f32
}

/// Every object's functions for this tick (an object the table does not
/// name keeps none).
pub fn update(world: &mut World) {
    let Some(table) = world.remove_resource::<FunctionTable>() else {
        return;
    };
    let tick = world.get_resource::<Tick>().map_or(0, |tick| tick.0);
    let mut missing = Vec::new();
    let mut objects = world.query::<(
        Entity,
        &ObjectId,
        &Object,
        Option<&Vitality>,
        Option<&IntegratedLight>,
        Option<&WeaponItem>,
        Option<&mut FunctionValues>,
    )>();
    for (entity, id, object, vitality, light, item, values) in objects.iter_mut(world) {
        let Some(functions) = table.0.get(&object.tag) else {
            continue;
        };
        let exported = Exported {
            vitality: vitality.map(|v| (v.body, v.shield, v.dead)),
            random: random_constant(id.0),
            integrated_light: light.map_or(0.0, |light| light.power),
            weapon: (object.kind == ObjectKind::Weapon).then(|| {
                item.map_or((0.0, 1.0, false), |item| {
                    let loaded = item.0.magazines.first().map_or(0, |m| m.loaded);
                    let full = if functions.rounds_loaded_maximum > 0 {
                        f32::from(loaded) / f32::from(functions.rounds_loaded_maximum)
                    } else {
                        1.0
                    };
                    (item.0.heat, full.clamp(0.0, 1.0), item.0.overheated)
                })
            }),
        };
        let incoming = incoming(&functions.modes, &exported);
        let previous = values.as_ref().map_or([0.0; 4], |values| values.outgoing);
        let (outgoing, active) = compute(&functions.functions, incoming, previous, id.0, tick);
        let computed = FunctionValues {
            incoming,
            outgoing,
            active,
        };
        match values {
            Some(mut values) => *values = computed,
            None => missing.push((entity, computed)),
        }
    }
    for (entity, values) in missing {
        world.entity_mut(entity).insert(values);
    }
    world.insert_resource(table);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::periodic::function;

    fn constant() -> ObjectFunction {
        ObjectFunction {
            function: function::ONE,
            bounds: [0.0, 1.0],
            turn_off_with: -1,
            reciprocal_bounds_range: 1.0,
            ..ObjectFunction::default()
        }
    }

    #[test]
    fn a_function_of_one_is_one_and_on() {
        let (out, active) = compute(&[constant()], [0.0; 4], [0.0; 4], 0, 0);
        assert_eq!(out[0], 1.0);
        assert!(active[0]);
        assert!(!active[1]);
    }

    #[test]
    fn a_function_scaled_by_an_incoming_value_follows_it() {
        let function = ObjectFunction {
            scale_function_by: 2,
            ..constant()
        };
        let (out, _) = compute(
            std::slice::from_ref(&function),
            [0.0, 0.25, 0.0, 0.0],
            [0.0; 4],
            0,
            0,
        );
        assert_eq!(out[0], 0.25);
        // (below its lower bound it is off, at the bound)
        let (out, active) = compute(std::slice::from_ref(&function), [0.0; 4], [0.0; 4], 0, 0);
        assert_eq!((out[0], active[0]), (0.0, false));
        let always = ObjectFunction {
            flags: ObjectFunction::ALWAYS_ACTIVE,
            ..function
        };
        let (_, active) = compute(&[always], [0.0; 4], [0.0; 4], 0, 0);
        assert!(active[0]);
    }

    #[test]
    fn inverted_stepped_scaled_and_bounded_functions() {
        let inverted = ObjectFunction {
            flags: ObjectFunction::INVERT,
            scale_function_by: 1,
            ..constant()
        };
        let (out, _) = compute(&[inverted], [0.25, 0.0, 0.0, 0.0], [0.0; 4], 0, 0);
        assert_eq!(out[0], 0.75);
        let stepped = ObjectFunction {
            scale_function_by: 1,
            step_count: 4,
            reciprocal_step_count: 0.25,
            ..constant()
        };
        let (out, _) = compute(&[stepped], [0.6, 0.0, 0.0, 0.0], [0.0; 4], 0, 0);
        assert_eq!(out[0], 0.5);
        let squared = ObjectFunction {
            scale_function_by: 1,
            square_wave_threshold: 0.5,
            ..constant()
        };
        let (out, _) = compute(&[squared], [0.6, 0.0, 0.0, 0.0], [0.0; 4], 0, 0);
        assert_eq!(out[0], 1.0);
        let fitted = ObjectFunction {
            bounds_mode: 2,
            bounds: [0.5, 0.75],
            scale_by: 0.5,
            ..constant()
        };
        let (out, _) = compute(&[fitted], [0.0; 4], [0.0; 4], 0, 0);
        assert_eq!(out[0], 0.625);
        let normalized = ObjectFunction {
            bounds_mode: 1,
            bounds: [0.5, 1.0],
            reciprocal_bounds_range: 2.0,
            scale_function_by: 1,
            ..constant()
        };
        let (out, _) = compute(&[normalized], [0.75, 0.0, 0.0, 0.0], [0.0; 4], 0, 0);
        assert_eq!(out[0], 0.5);
    }

    #[test]
    fn functions_read_each_other_and_turn_each_other_off() {
        let off = ObjectFunction {
            scale_function_by: 1,
            ..constant()
        };
        let follower = ObjectFunction {
            turn_off_with: 0,
            add: 5,
            scale_function_by: 2,
            ..constant()
        };
        let (out, active) = compute(&[off, follower], [0.0, 0.5, 0.0, 0.0], [0.0; 4], 0, 0);
        assert_eq!(out[1], 0.5);
        assert!(!active[1]);
        let additive = ObjectFunction {
            flags: ObjectFunction::ADDITIVE,
            scale_function_by: 1,
            ..constant()
        };
        let (out, _) = compute(
            &[additive],
            [0.5, 0.0, 0.0, 0.0],
            [0.75, 0.0, 0.0, 0.0],
            0,
            0,
        );
        assert_eq!(out[0], 0.25);
    }

    #[test]
    fn a_wobble_and_a_sawtooth_stay_near_the_value() {
        let wobbled = ObjectFunction {
            wobble_function: function::COSINE,
            wobble_period: 1.0,
            wobble_magnitude: 0.1,
            scale_function_by: 1,
            reciprocal_sawtooth_count: 0.5,
            ..constant()
        };
        for tick in 0..60 {
            let (out, _) = compute(
                std::slice::from_ref(&wobbled),
                [0.3, 0.0, 0.0, 0.0],
                [0.0; 4],
                3,
                tick,
            );
            assert!((0.0..0.5).contains(&out[0]), "{}", out[0]);
        }
    }

    #[test]
    fn exports_fill_the_incoming_values_the_parts_override() {
        let modes = FunctionModes {
            object: [1, 2, 18, 5],
            unit: [0, 0, 5, 0],
            biped: [0; 4],
            weapon: [0; 4],
        };
        let exported = Exported {
            vitality: Some((0.5, 1.5, false)),
            random: 0.25,
            integrated_light: 0.75,
            weapon: None,
        };
        assert_eq!(incoming(&modes, &exported), [0.5, 1.0, 0.75, 0.25]);
        let weapon = FunctionModes {
            weapon: [1, 2, 9, 6],
            ..FunctionModes::default()
        };
        let exported = Exported {
            weapon: Some((0.4, 0.5, true)),
            ..Exported::default()
        };
        assert_eq!(incoming(&weapon, &exported), [0.4, 0.5, 1.0, 1.0]);
        assert_eq!(incoming(&FunctionModes::default(), &exported), [0.0; 4]);
        assert!((0.0..1.0).contains(&random_constant(12345)));
    }

    #[test]
    fn a_flashlight_brightens_runs_down_and_recharges() {
        let mut light = IntegratedLight::default();
        light.step(true, false, false);
        assert!(light.on);
        for _ in 0..5 {
            light.step(false, false, false);
        }
        assert_eq!(light.power, 1.0);
        assert!(light.battery < 1.0);
        light.step(true, false, false);
        assert!(!light.on);
        for _ in 0..24 {
            light.step(false, false, false);
        }
        assert_eq!(light.power, 0.0);
        // (a seat neither switches it on nor lets it stay on)
        light.step(true, true, false);
        assert!(!light.on);
        light.step(true, false, false);
        light.step(false, true, false);
        assert!(!light.on);
        // (an empty battery switches it off, and it will not come on
        // until the battery has some charge)
        light.battery = 0.0;
        light.on = true;
        light.step(false, false, false);
        assert!(!light.on);
        light.battery = 0.1;
        light.step(true, false, false);
        assert!(!light.on);
    }
}
