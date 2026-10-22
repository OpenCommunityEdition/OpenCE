//! Weapons, projectiles, equipment and damage: the item part a weapon and
//! a piece of equipment share, each one's own part, and the damage effects
//! (`jpt!`) they do.
//!
//! A projectile's speeds are a tick's in the tag (the game's 30); the
//! engine keeps them a second. Its guided turn, and every weapon's rates
//! and times, are a second's already.

use opence_content::{
    AimAssist, Ammunition, DamageEffect, Equipment, Item, Magazine, MaterialResponse, Projectile,
    Trigger, Weapon,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::object::{optional_index, reference, vector, ANTR, MOD2, MODE, OBJECT_SIZE};
use crate::tags::{Element, GroupTag};

pub(crate) const WEAP: GroupTag = GroupTag::new(b"weap");
pub(crate) const PROJ: GroupTag = GroupTag::new(b"proj");
pub(crate) const EQIP: GroupTag = GroupTag::new(b"eqip");
pub(crate) const JPT: GroupTag = GroupTag::new(b"jpt!");
const EFFE: GroupTag = GroupTag::new(b"effe");
const SND: GroupTag = GroupTag::new(b"snd!");
const WPHI: GroupTag = GroupTag::new(b"wphi");

/// Where the item part starts (after the object's), and how long it is.
const ITEM_PART: usize = OBJECT_SIZE;
const ITEM_SIZE: usize = 0x18C;
/// Where a weapon's or a piece of equipment's own part starts.
const ITEM_END: usize = ITEM_PART + ITEM_SIZE;
/// Where a weapon's own fields start.
pub(crate) const WEAPON_PART: usize = ITEM_END;
pub(crate) const WEAPON_SIZE: usize = ITEM_END + 0x200;
pub(crate) const EQUIPMENT_SIZE: usize = ITEM_END + 0x18;
const PROJECTILE_PART: usize = OBJECT_SIZE;
pub(crate) const PROJECTILE_SIZE: usize = PROJECTILE_PART + 0xD0;
pub(crate) const DAMAGE_EFFECT_SIZE: usize = 0x2A0;
const DAMAGE_PART: usize = 0x1C4;

pub(crate) const MAGAZINE_SIZE: usize = 0x70;
pub(crate) const AMMUNITION_SIZE: usize = 0x1C;
pub(crate) const TRIGGER_SIZE: usize = 0x114;
/// A trigger's firing effect: its shots, then its effects (firing,
/// misfiring, empty) and their damage effects, references of 0x10 bytes.
pub(crate) const FIRING_EFFECT_SIZE: usize = 0x84;
const MAX_FIRING_EFFECTS: usize = 8;
pub(crate) const MATERIAL_RESPONSE_SIZE: usize = 0xA0;
const MAX_MAGAZINES: usize = 2;
const MAX_AMMUNITION: usize = 8;
const MAX_TRIGGERS: usize = 2;
const MAX_MATERIAL_RESPONSES: usize = 33;

/// The game's ticks a second, which the tag's speeds are of.
const TAG_TICKS_PER_SECOND: f32 = 30.0;

/// The item part of a weapon's or a piece of equipment's definition.
fn import_item(tag: &Element<'_>) -> Result<Item, ImportError> {
    let at = |offset: usize| ITEM_PART + offset;
    Ok(Item {
        flags: tag.u32(at(0x0), "item flags")?,
        hud_message_index: tag.i16(at(0x4), "pickup message index")?,
        sort_order: tag.i16(at(0x6), "sort order")?,
        detonation_delay: [
            tag.f32(at(0x164), "detonation delay")?,
            tag.f32(at(0x168), "detonation delay")?,
        ],
        detonating_effect: reference(tag, at(0x16C), "detonating effect", &[EFFE])?,
        detonation_effect: reference(tag, at(0x17C), "detonation effect", &[EFFE])?,
    })
}

/// A weapon's item part and its own.
pub(crate) fn import_weapon(cache: &Cache, index: usize) -> Result<Weapon, ImportError> {
    let tag = cache.tag(index, WEAP, WEAPON_SIZE)?;
    let at = |offset: usize| ITEM_END + offset;
    let f = |offset: usize, field: &str| tag.f32(at(offset), field);

    let magazines_block = tag.block(at(0x1E8), "magazines", MAGAZINE_SIZE, MAX_MAGAZINES)?;
    let mut magazines = Vec::with_capacity(magazines_block.len());
    for magazine in magazines_block.iter() {
        magazines.push(import_magazine(&magazine)?);
    }
    let triggers_block = tag.block(at(0x1F4), "triggers", TRIGGER_SIZE, MAX_TRIGGERS)?;
    let mut triggers = Vec::with_capacity(triggers_block.len());
    for trigger in triggers_block.iter() {
        triggers.push(import_trigger(&trigger, magazines.len())?);
    }

    Ok(Weapon {
        item: import_item(&tag)?,
        flags: tag.u32(at(0x0), "weapon flags")?,
        secondary_trigger_mode: tag.i16(at(0x24), "secondary trigger mode")?,
        maximum_alternate_shots_loaded: tag.i16(at(0x26), "maximum alternate shots loaded")?,
        ready_time: f(0x30, "ready time")?,
        heat_recovery_threshold: f(0x44, "heat recovery threshold")?,
        heat_overheated_threshold: f(0x48, "overheated threshold")?,
        heat_detonation_threshold: f(0x4C, "heat detonation threshold")?,
        heat_loss_per_second: f(0x54, "heat loss per second")?,
        melee_damage: reference(&tag, at(0x8C), "melee damage", &[JPT])?,
        melee_response: reference(&tag, at(0x9C), "melee response", &[JPT])?,
        zoom_levels: tag.i16(at(0xD2), "magnification levels")?,
        zoom_magnification: vector(&tag, at(0xD4), "magnification range")?,
        aim_assist: AimAssist {
            autoaim_angle: f(0xDC, "autoaim angle")?,
            autoaim_distance: f(0xE0, "autoaim range")?,
            magnetism_angle: f(0xE4, "magnetism angle")?,
            magnetism_distance: f(0xE8, "magnetism range")?,
            deviation_angle: f(0xEC, "deviation angle")?,
        },
        movement_penalty_mode: tag.i16(at(0xF4), "movement penalized")?,
        forward_movement_penalty: f(0xF8, "forward movement penalty")?,
        sideways_movement_penalty: f(0xFC, "sideways movement penalty")?,
        age_rate_of_fire_penalty: f(0x13C, "age rate of fire penalty")?,
        first_person_model: reference(&tag, at(0x154), "first person model", &[MODE, MOD2])?,
        first_person_animations: reference(&tag, at(0x164), "first person animations", &[ANTR])?,
        hud_interface: reference(&tag, at(0x178), "hud interface", &[WPHI])?,
        weapon_type: tag.i16(at(0x1DA), "weapon type")?,
        magazines,
        triggers,
    })
}

fn import_magazine(magazine: &Element<'_>) -> Result<Magazine, ImportError> {
    let ammunition_block =
        magazine.block(0x64, "magazine objects", AMMUNITION_SIZE, MAX_AMMUNITION)?;
    let mut ammunition = Vec::with_capacity(ammunition_block.len());
    for object in ammunition_block.iter() {
        ammunition.push(Ammunition {
            rounds: object.i16(0x0, "rounds")?,
            equipment: reference(&object, 0xC, "equipment", &[EQIP])?,
        });
    }
    Ok(Magazine {
        flags: magazine.u32(0x0, "magazine flags")?,
        rounds_recharged_per_second: magazine.i16(0x4, "rounds recharged")?,
        rounds_total_initial: magazine.i16(0x6, "rounds total initial")?,
        rounds_total_maximum: magazine.i16(0x8, "rounds total maximum")?,
        rounds_loaded_maximum: magazine.i16(0xA, "rounds loaded maximum")?,
        reload_time: magazine.f32(0x14, "reload time")?,
        rounds_reloaded: magazine.i16(0x18, "rounds reloaded")?,
        chamber_time: magazine.f32(0x1C, "chamber time")?,
        ammunition,
    })
}

fn import_trigger(trigger: &Element<'_>, magazines: usize) -> Result<Trigger, ImportError> {
    let f = |at: usize, field: &str| trigger.f32(at, field);
    let i = |at: usize, field: &str| trigger.i16(at, field);
    Ok(Trigger {
        flags: trigger.u32(0x0, "trigger flags")?,
        initial_rate_of_fire: f(0x4, "rounds per second")?,
        final_rate_of_fire: f(0x8, "rounds per second")?,
        rate_of_fire_acceleration_time: f(0xC, "acceleration time")?,
        rate_of_fire_deceleration_time: f(0x10, "deceleration time")?,
        blurred_rate_of_fire: f(0x14, "blurred rate of fire")?,
        magazine: optional_index(trigger, 0x20, "magazine", magazines)?,
        rounds_per_shot: i(0x22, "rounds per shot")?,
        minimum_rounds_loaded: i(0x24, "minimum rounds loaded")?,
        rounds_between_tracers: i(0x26, "rounds between tracers")?,
        initial_error: f(0x30, "error")?,
        final_error: f(0x34, "error")?,
        error_acceleration_time: f(0x38, "error acceleration time")?,
        error_deceleration_time: f(0x3C, "error deceleration time")?,
        charging_time: f(0x48, "charging time")?,
        charged_time: f(0x4C, "charged time")?,
        overcharged_action: i(0x50, "overcharged action")?,
        charged_illumination: f(0x54, "charged illumination")?,
        spew_time: f(0x58, "spew time")?,
        projectile_distribution_function: i(0x6C, "distribution function")?,
        projectiles_per_shot: i(0x6E, "projectiles per shot")?,
        projectile_distribution_angle: f(0x70, "distribution angle")?,
        projectile_error_inner_cone_angle: f(0x78, "minimum error")?,
        projectile_error_angle: vector(trigger, 0x7C, "error angle")?,
        first_person_offset: vector(trigger, 0x84, "first person offset")?,
        projectile: reference(trigger, 0x94, "projectile", &[PROJ])?,
        heat_generated_per_round: f(0xB8, "heat generated per round")?,
        age_generated_per_round: f(0xBC, "age generated per round")?,
        overloading_time: f(0xC4, "overload time")?,
        firing_effect: match trigger
            .block(
                0x108,
                "firing effects",
                FIRING_EFFECT_SIZE,
                MAX_FIRING_EFFECTS,
            )?
            .iter()
            .next()
        {
            Some(effect) => reference(&effect, 0x24, "firing effect", &[EFFE])?,
            None => None,
        },
    })
}

/// A projectile's own part.
pub(crate) fn import_projectile(cache: &Cache, index: usize) -> Result<Projectile, ImportError> {
    let tag = cache.tag(index, PROJ, PROJECTILE_SIZE)?;
    let at = |offset: usize| PROJECTILE_PART + offset;
    let f = |offset: usize, field: &str| tag.f32(at(offset), field);
    let speed =
        |offset: usize, field: &str| Ok::<_, ImportError>(f(offset, field)? * TAG_TICKS_PER_SECOND);

    let responses = tag.block(
        at(0xC4),
        "material responses",
        MATERIAL_RESPONSE_SIZE,
        MAX_MATERIAL_RESPONSES,
    )?;
    let mut material_responses = Vec::with_capacity(responses.len());
    for response in responses.iter() {
        material_responses.push(import_material_response(&response)?);
    }

    Ok(Projectile {
        flags: tag.u32(at(0x0), "projectile flags")?,
        detonation_timer_starts: tag.i16(at(0x4), "detonation timer starts")?,
        super_detonation: reference(&tag, at(0x10), "super detonation", &[EFFE])?,
        collision_radius: f(0x24, "collision radius")?,
        arming_time: f(0x28, "arming time")?,
        danger_radius: f(0x2C, "danger radius")?,
        effect: reference(&tag, at(0x30), "effect", &[EFFE])?,
        timer: vector(&tag, at(0x40), "timer")?,
        minimum_velocity: speed(0x48, "minimum velocity")?,
        maximum_range: f(0x4C, "maximum range")?,
        air_gravity_scale: f(0x50, "air gravity scale")?,
        air_damage_range: vector(&tag, at(0x54), "air damage range")?,
        water_gravity_scale: f(0x5C, "water gravity scale")?,
        water_damage_range: vector(&tag, at(0x60), "water damage range")?,
        initial_velocity: speed(0x68, "initial velocity")?,
        final_velocity: speed(0x6C, "final velocity")?,
        guided_angular_velocity: f(0x70, "guided angular velocity")?,
        detonation_started: reference(&tag, at(0x78), "detonation started", &[EFFE])?,
        attached_detonation_damage: reference(
            &tag,
            at(0x98),
            "attached detonation damage",
            &[JPT],
        )?,
        impact_damage: reference(&tag, at(0xA8), "impact damage", &[JPT])?,
        material_responses,
    })
}

fn import_material_response(response: &Element<'_>) -> Result<MaterialResponse, ImportError> {
    let f = |at: usize, field: &str| response.f32(at, field);
    let potential_velocity: [f32; 2] = vector(response, 0x34, "potential velocity bounds")?;
    Ok(MaterialResponse {
        flags: response.u16(0x0, "material response flags")?,
        default_response: response.i16(0x2, "default response")?,
        default_effect: reference(response, 0x4, "default effect", &[EFFE])?,
        potential_response: response.i16(0x24, "potential response")?,
        potential_flags: response.u16(0x26, "potential flags")?,
        potential_skip_fraction: f(0x28, "skip fraction")?,
        potential_angle: vector(response, 0x2C, "potential angle bounds")?,
        potential_velocity: potential_velocity.map(|speed| speed * TAG_TICKS_PER_SECOND),
        potential_effect: reference(response, 0x3C, "potential effect", &[EFFE])?,
        scale_effects_by: response.i16(0x5C, "scale effects by")?,
        angular_noise: f(0x60, "angular noise")?,
        velocity_noise: f(0x64, "velocity noise")? * TAG_TICKS_PER_SECOND,
        detonation_effect: reference(response, 0x68, "detonation effect", &[EFFE])?,
        initial_friction: f(0x90, "initial friction")?,
        maximum_distance: f(0x94, "maximum distance")?,
        parallel_friction: f(0x98, "parallel friction")?,
        perpendicular_friction: f(0x9C, "perpendicular friction")?,
    })
}

/// A piece of equipment's item part and its own.
pub(crate) fn import_equipment(cache: &Cache, index: usize) -> Result<Equipment, ImportError> {
    let tag = cache.tag(index, EQIP, EQUIPMENT_SIZE)?;
    Ok(Equipment {
        item: import_item(&tag)?,
        powerup_type: tag.i16(ITEM_END, "powerup type")?,
        grenade_type: tag.i16(ITEM_END + 0x2, "grenade type")?,
        powerup_time: tag.f32(ITEM_END + 0x4, "powerup time")?,
        pickup_sound: reference(&tag, ITEM_END + 0x8, "pickup sound", &[SND])?,
    })
}

/// A damage effect.
pub(crate) fn import_damage_effect(
    cache: &Cache,
    index: usize,
) -> Result<DamageEffect, ImportError> {
    let tag = cache.tag(index, JPT, DAMAGE_EFFECT_SIZE)?;
    let at = |offset: usize| DAMAGE_PART + offset;
    let f = |offset: usize, field: &str| tag.f32(at(offset), field);
    let mut material_modifiers = Vec::with_capacity(DamageEffect::MATERIAL_TYPES);
    for material in 0..DamageEffect::MATERIAL_TYPES {
        material_modifiers.push(f(0x3C + material * 4, "damage modifiers")?);
    }
    Ok(DamageEffect {
        tag: tag_ref(cache, index),
        radius: vector(&tag, 0x0, "radius")?,
        cutoff_scale: tag.f32(0x8, "cutoff scale")?,
        flags: tag.u32(0xC, "flags")?,
        side_effect: tag.i16(at(0x0), "side effect")?,
        category: tag.i16(at(0x2), "category")?,
        damage_flags: tag.u32(at(0x4), "damage flags")?,
        area_of_effect_core_radius: f(0x8, "aoe core radius")?,
        damage_minimum: f(0xC, "damage lower bound")?,
        damage: vector(&tag, at(0x10), "damage upper bound")?,
        vehicle_passthrough_penalty: f(0x18, "vehicle passthrough penalty")?,
        active_camouflage_damage: f(0x1C, "active camouflage damage")?,
        stun: f(0x20, "stun")?,
        maximum_stun: f(0x24, "maximum stun")?,
        stun_time: f(0x28, "stun time")?,
        instantaneous_acceleration: f(0x30, "instantaneous acceleration")?,
        material_modifiers,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, empty, put, reference as put_reference};
    use std::error::Error;

    type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

    fn f32s(bytes: &mut [u8], at: usize, values: &[f32]) {
        for (n, value) in values.iter().enumerate() {
            put(bytes, at + n * 4, &value.to_le_bytes());
        }
    }

    /// A weapon of one magazine (of one ammunition, an empty reference)
    /// and one trigger, which fires `projectile` from magazine `magazine`.
    fn weapon_cache(magazine: i16) -> Result<(Cache, usize, usize)> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);

        let mut projectile = vec![0u8; PROJECTILE_SIZE];
        for offset in [0x10, 0x30, 0x78, 0x98, 0xA8] {
            empty(&mut projectile, PROJECTILE_PART + offset);
        }
        f32s(&mut projectile, PROJECTILE_PART + 0x48, &[2.0, 40.0]);
        f32s(&mut projectile, PROJECTILE_PART + 0x68, &[10.8, 9.0]);
        let projectile = fake.add_tag(PROJ, "weapons\\test\\bullet", &projectile);

        let mut ammunition = vec![0u8; AMMUNITION_SIZE];
        put(&mut ammunition, 0, &30i16.to_le_bytes());
        empty(&mut ammunition, 0xC);
        let ammunition = fake.add_data(&ammunition);
        let mut magazine_bytes = vec![0u8; MAGAZINE_SIZE];
        put(&mut magazine_bytes, 0x6, &90i16.to_le_bytes());
        put(&mut magazine_bytes, 0x8, &300i16.to_le_bytes());
        put(&mut magazine_bytes, 0xA, &30i16.to_le_bytes());
        f32s(&mut magazine_bytes, 0x14, &[2.5]);
        block(&mut magazine_bytes, 0x64, 1, ammunition);
        let magazines = fake.add_data(&magazine_bytes);

        let mut trigger = vec![0u8; TRIGGER_SIZE];
        f32s(&mut trigger, 0x4, &[8.0, 12.0]);
        put(&mut trigger, 0x20, &magazine.to_le_bytes());
        put(&mut trigger, 0x6E, &1i16.to_le_bytes());
        put_reference(&mut trigger, 0x94, PROJ, fake.handle(projectile));
        let triggers = fake.add_data(&trigger);

        let mut weapon = vec![0u8; WEAPON_SIZE];
        for at in [ITEM_PART + 0x16C, ITEM_PART + 0x17C] {
            empty(&mut weapon, at);
        }
        for offset in [0x8C, 0x9C, 0x154, 0x164, 0x178] {
            empty(&mut weapon, ITEM_END + offset);
        }
        f32s(&mut weapon, ITEM_END + 0x30, &[1.25]);
        block(&mut weapon, ITEM_END + 0x1E8, 1, magazines);
        block(&mut weapon, ITEM_END + 0x1F4, 1, triggers);
        let weapon = fake.add_tag(WEAP, "weapons\\test\\rifle", &weapon);
        Ok((Cache::open(fake.build())?, weapon, projectile))
    }

    #[test]
    fn a_weapon_imports_with_its_magazine_and_trigger() -> Result {
        let (cache, weapon, _) = weapon_cache(0)?;
        let weapon = import_weapon(&cache, weapon)?;
        assert_eq!(weapon.ready_time, 1.25);
        let [magazine] = weapon.magazines.as_slice() else {
            panic!("{} magazines", weapon.magazines.len());
        };
        assert_eq!(
            (magazine.rounds_total_initial, magazine.rounds_total_maximum),
            (90, 300)
        );
        assert_eq!(magazine.rounds_loaded_maximum, 30);
        assert_eq!(magazine.reload_time, 2.5);
        assert_eq!(magazine.ammunition[0].rounds, 30);
        assert_eq!(magazine.ammunition[0].equipment, None);
        let [trigger] = weapon.triggers.as_slice() else {
            panic!("{} triggers", weapon.triggers.len());
        };
        assert_eq!(trigger.magazine, Some(0));
        assert_eq!(
            (trigger.initial_rate_of_fire, trigger.final_rate_of_fire),
            (8.0, 12.0)
        );
        assert_eq!(trigger.projectiles_per_shot, 1);
        assert_eq!(
            trigger.projectile.as_ref().map(|tag| tag.path.as_str()),
            Some("weapons\\test\\bullet")
        );
        Ok(())
    }

    #[test]
    fn a_trigger_without_a_magazine_imports() -> Result {
        let (cache, weapon, _) = weapon_cache(-1)?;
        assert_eq!(import_weapon(&cache, weapon)?.triggers[0].magazine, None);
        Ok(())
    }

    #[test]
    fn a_trigger_of_a_magazine_past_the_weapons_is_refused() -> Result {
        let (cache, weapon, _) = weapon_cache(1)?;
        assert!(import_weapon(&cache, weapon).is_err());
        Ok(())
    }

    #[test]
    fn a_projectiles_speeds_are_a_seconds() -> Result {
        let (cache, _, projectile) = weapon_cache(0)?;
        let projectile = import_projectile(&cache, projectile)?;
        // (the tag's are a tick's, the game's 30 a second)
        assert_eq!(projectile.minimum_velocity, 60.0);
        assert_eq!(projectile.maximum_range, 40.0);
        assert!((projectile.initial_velocity - 324.0).abs() < 1e-3);
        assert_eq!(projectile.final_velocity, 270.0);
        assert!(projectile.material_responses.is_empty());
        Ok(())
    }

    #[test]
    fn a_damage_effect_imports() -> Result {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let mut effect = vec![0u8; DAMAGE_EFFECT_SIZE];
        f32s(&mut effect, 0x0, &[1.0, 5.0, 0.25]);
        put(&mut effect, DAMAGE_PART + 0x2, &3i16.to_le_bytes());
        put(
            &mut effect,
            DAMAGE_PART + 0x4,
            &DamageEffect::CAN_CAUSE_HEADSHOTS.to_le_bytes(),
        );
        f32s(&mut effect, DAMAGE_PART + 0xC, &[10.0, 20.0, 25.0]);
        f32s(&mut effect, DAMAGE_PART + 0x30, &[4.0]);
        let modifiers: Vec<f32> = (0..DamageEffect::MATERIAL_TYPES)
            .map(|n| n as f32 / 8.0)
            .collect();
        f32s(&mut effect, DAMAGE_PART + 0x3C, &modifiers);
        let index = fake.add_tag(JPT, "weapons\\test\\impact", &effect);
        let cache = Cache::open(fake.build())?;

        let effect = import_damage_effect(&cache, index)?;
        assert_eq!(effect.tag.path, "weapons\\test\\impact");
        assert_eq!(effect.radius, [1.0, 5.0]);
        assert_eq!(effect.cutoff_scale, 0.25);
        assert_eq!(effect.category, 3);
        assert_eq!(effect.damage_flags, DamageEffect::CAN_CAUSE_HEADSHOTS);
        assert_eq!(effect.damage_minimum, 10.0);
        assert_eq!(effect.damage, [20.0, 25.0]);
        assert_eq!(effect.instantaneous_acceleration, 4.0);
        assert_eq!(effect.material_modifiers, modifiers);
        Ok(())
    }
}
