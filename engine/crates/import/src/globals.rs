//! The game's globals (`matg`): the player's speeds, what falling does,
//! the player's bipeds, the grenades and the weapons.
//!
//! The tag gives speeds in world units a second and accelerations in world
//! units a second each tick (the game's 30); the engine keeps both a
//! second. Its falling velocities, worked out by the tools that built the
//! map, are a tick's; the engine keeps them a second.

use opence_content::{FallingDamage, Globals, Grenade, PlayerPhysics, TagRef};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::tags::{Element, GroupTag};

const MATG: GroupTag = GroupTag::new(b"matg");
const GLOBALS_PATH: &str = "globals\\globals";
const GLOBALS_SIZE: usize = 0x1AC;
const MULTIPLAYER_INFORMATION_OFFSET: usize = 0x164;
const MULTIPLAYER_INFORMATION_SIZE: usize = 0xA0;
const PLAYER_INFORMATION_OFFSET: usize = 0x170;
const PLAYER_INFORMATION_SIZE: usize = 0xF4;
const GRENADES_OFFSET: usize = 0x128;
const GRENADE_SIZE: usize = 0x44;
const MAX_GRENADES: usize = 4;
const WEAPON_LIST_OFFSET: usize = 0x14C;
const WEAPON_SIZE: usize = 0x10;
const MAX_WEAPONS: usize = 20;
const FALLING_DAMAGE_OFFSET: usize = 0x188;
const FALLING_DAMAGE_SIZE: usize = 0x98;
/// The game's ticks a second, which the tag's rates are of.
const TAG_TICKS_PER_SECOND: f32 = 30.0;

fn reference(element: &Element<'_>, at: usize, field: &str) -> Result<Option<TagRef>, ImportError> {
    Ok(element
        .reference(at, field, &[])?
        .map(|index| tag_ref(element.cache(), index)))
}

/// The cache's globals (`globals\globals`, or its first `matg`), or the
/// game's defaults for a cache without them.
pub fn import_globals(cache: &Cache) -> Result<Globals, ImportError> {
    let index = cache
        .find(MATG, GLOBALS_PATH)
        .or_else(|| cache.tags().iter().position(|instance| instance.is(MATG)));
    let Some(index) = index else {
        return Ok(Globals::default());
    };
    let globals = cache.tag(index, MATG, GLOBALS_SIZE)?;
    let mut imported = Globals::default();

    let player = globals.block(
        PLAYER_INFORMATION_OFFSET,
        "player information",
        PLAYER_INFORMATION_SIZE,
        1,
    )?;
    if let Some(player) = player.get(0) {
        let f = |at: usize, field: &str| player.f32(at, field);
        let rate =
            |at: usize, field: &str| Ok::<_, ImportError>(f(at, field)? * TAG_TICKS_PER_SECOND);
        imported.player_unit = reference(&player, 0x00, "unit")?;
        imported.player = PlayerPhysics {
            walking_speed: f(0x2C, "walking speed")?,
            run_forward: f(0x34, "run forward")?,
            run_backward: f(0x38, "run backward")?,
            run_sideways: f(0x3C, "run sideways")?,
            run_acceleration: rate(0x40, "run acceleration")?,
            sneak_forward: f(0x44, "sneak forward")?,
            sneak_backward: f(0x48, "sneak backward")?,
            sneak_sideways: f(0x4C, "sneak sideways")?,
            sneak_acceleration: rate(0x50, "sneak acceleration")?,
            airborne_acceleration: rate(0x54, "airborne acceleration")?,
            stun_movement_penalty: f(0x80, "stun movement penalty")?,
            stun_turning_penalty: f(0x84, "stun turning penalty")?,
            stun_jumping_penalty: f(0x88, "stun jumping penalty")?,
        };
    }

    let multiplayer = globals.block(
        MULTIPLAYER_INFORMATION_OFFSET,
        "multiplayer information",
        MULTIPLAYER_INFORMATION_SIZE,
        1,
    )?;
    if let Some(multiplayer) = multiplayer.get(0) {
        imported.multiplayer_unit = reference(&multiplayer, 0x10, "unit")?;
    }

    let falling = globals.block(
        FALLING_DAMAGE_OFFSET,
        "falling damage",
        FALLING_DAMAGE_SIZE,
        1,
    )?;
    if let Some(falling) = falling.get(0) {
        let speed = |at: usize, field: &str| {
            Ok::<_, ImportError>(falling.f32(at, field)? * TAG_TICKS_PER_SECOND)
        };
        imported.falling = FallingDamage {
            minimum_damage_velocity: speed(0x90, "minimum damage velocity")?,
            maximum_damage_velocity: speed(0x94, "maximum damage velocity")?,
            maximum_falling_velocity: speed(0x8C, "maximum falling velocity")?,
            falling_damage: reference(&falling, 0x10, "falling damage")?,
            maximum_distance_damage: reference(&falling, 0x2C, "maximum falling damage")?,
        };
    }

    let grenades = globals.block(GRENADES_OFFSET, "grenades", GRENADE_SIZE, MAX_GRENADES)?;
    for grenade in grenades.iter() {
        imported.grenades.push(Grenade {
            maximum_count: grenade.i16(0x0, "maximum count")?,
            multiplayer_spawn_default: grenade.i16(0x2, "mp spawn default")?,
            throwing_effect: reference(&grenade, 0x4, "throwing effect")?,
            hud_interface: reference(&grenade, 0x14, "hud interface")?,
            item: reference(&grenade, 0x24, "equipment")?,
            projectile: reference(&grenade, 0x34, "projectile")?,
        });
    }

    let weapons = globals.block(WEAPON_LIST_OFFSET, "weapon list", WEAPON_SIZE, MAX_WEAPONS)?;
    for weapon in weapons.iter() {
        if let Some(tag) = reference(&weapon, 0x0, "weapon")? {
            imported.weapons.push(tag);
        }
    }
    Ok(imported)
}
