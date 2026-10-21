//! A scenario's campaign data (`scenario_definitions.h`,
//! `ai_scenario_definitions.h`): the lists its scripts name things in, and
//! its encounters and command lists. Each block's element size is the
//! game's, and each name is read as the game reads it, a string of at most
//! 31 characters.

use opence_content::{
    ActorVariant, CameraPoint, Campaign, Command, CommandList, CutsceneFlag, DeviceGroup,
    Encounter, FiringPosition, Recording, Squad, StartingLocation, TriggerVolume,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_ref, SCENARIO_SIZE, SCNR};
use crate::tags::{Element, GroupTag};

const OBJECT_NAMES: (usize, usize, usize) = (0x204, 0x24, 1024);
const DEVICE_GROUPS: (usize, usize, usize) = (0x288, 0x34, 128);
const STARTING_PROFILES: (usize, usize, usize) = (0x348, 0x68, 256);
const TRIGGER_VOLUMES: (usize, usize, usize) = (0x360, 0x60, 256);
const RECORDINGS: (usize, usize, usize) = (0x36C, 0x40, 1024);
const ACTOR_PALETTE: (usize, usize, usize) = (0x420, 0x10, 64);
const ENCOUNTERS: (usize, usize, usize) = (0x42C, 0xB0, 128);
const COMMAND_LISTS: (usize, usize, usize) = (0x438, 0x60, 256);
const CONVERSATIONS: (usize, usize, usize) = (0x468, 0x74, 128);
const CUTSCENE_FLAGS: (usize, usize, usize) = (0x4E4, 0x5C, 512);
const CAMERA_POINTS: (usize, usize, usize) = (0x4F0, 0x68, 512);
const CUTSCENE_TITLES: (usize, usize, usize) = (0x4FC, 0x60, 64);

// (an encounter's blocks)
const SQUADS: (usize, usize, usize) = (0x80, 0xE8, 64);
const PLATOONS: (usize, usize, usize) = (0x8C, 0xAC, 32);
const FIRING_POSITIONS: (usize, usize, usize) = (0x98, 0x18, 512);
// (a squad's)
const STARTING_LOCATIONS: (usize, usize, usize) = (0xD0, 0x1C, 32);
// (a command list's)
const COMMANDS: (usize, usize, usize) = (0x30, 0x20, 64);
const POINTS: (usize, usize, usize) = (0x3C, 0x14, 64);

const ACTV: GroupTag = GroupTag::new(b"actv");
const ACTR: GroupTag = GroupTag::new(b"actr");
const UNIT: GroupTag = GroupTag::new(b"unit");
const WEAP: GroupTag = GroupTag::new(b"weap");

/// The parts of an actor variant and an actor read (each tag is longer).
const ACTOR_VARIANT_SIZE: usize = 0xF0;
const ACTOR_SIZE: usize = 0x2E8;

/// Each element of the block `(at, size, longest)` of `element`, read.
fn each<T>(
    element: &Element<'_>,
    (at, size, longest): (usize, usize, usize),
    field: &str,
    mut read: impl FnMut(&Element<'_>) -> Result<T, ImportError>,
) -> Result<Vec<T>, ImportError> {
    element
        .block(at, field, size, longest)?
        .iter()
        .map(|element| read(&element))
        .collect()
}

fn name(element: &Element<'_>, at: usize) -> Result<String, ImportError> {
    element.text(at, "name", 32).map(str::to_owned)
}

fn point(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 3], ImportError> {
    Ok([
        element.f32(at, field)?,
        element.f32(at + 4, field)?,
        element.f32(at + 8, field)?,
    ])
}

/// Imports the cache's scenario's campaign data.
pub fn import_campaign(cache: &Cache) -> Result<Campaign, ImportError> {
    let scenario = cache.tag(cache.scenario_index(), SCNR, SCENARIO_SIZE)?;
    scenario_campaign(&scenario)
}

/// A scenario's campaign data.
pub(crate) fn scenario_campaign(scenario: &Element<'_>) -> Result<Campaign, ImportError> {
    let cache = scenario.cache();
    Ok(Campaign {
        object_names: each(scenario, OBJECT_NAMES, "object names", |e| name(e, 0))?,
        trigger_volumes: each(scenario, TRIGGER_VOLUMES, "trigger volumes", |e| {
            Ok(TriggerVolume {
                name: name(e, 4)?,
                oriented: e.i16(0, "trigger volume type")? == 1,
                forward: point(e, 0x30, "forward")?,
                up: point(e, 0x3C, "up")?,
                position: point(e, 0x48, "position")?,
                extents: point(e, 0x54, "extents")?,
            })
        })?,
        cutscene_flags: each(scenario, CUTSCENE_FLAGS, "cutscene flags", |e| {
            Ok(CutsceneFlag {
                name: name(e, 4)?,
                position: point(e, 0x24, "position")?,
                facing: [e.f32(0x30, "facing")?, e.f32(0x34, "facing")?],
            })
        })?,
        camera_points: each(scenario, CAMERA_POINTS, "camera points", |e| {
            Ok(CameraPoint {
                name: name(e, 4)?,
                position: point(e, 0x28, "position")?,
                orientation: point(e, 0x34, "orientation")?,
                field_of_view: e.f32(0x40, "field of view")?,
            })
        })?,
        cutscene_titles: each(scenario, CUTSCENE_TITLES, "cutscene titles", |e| name(e, 4))?,
        device_groups: each(scenario, DEVICE_GROUPS, "device groups", |e| {
            Ok(DeviceGroup {
                name: name(e, 0)?,
                initial_value: e.f32(0x20, "initial value")?,
                flags: e.u32(0x24, "flags")?,
            })
        })?,
        recordings: each(scenario, RECORDINGS, "recorded animations", |e| {
            Ok(Recording {
                name: name(e, 0)?,
                length_in_ticks: e.i16(0x24, "length")?,
            })
        })?,
        actor_palette: each(scenario, ACTOR_PALETTE, "actor palette", |e| {
            Ok(e.reference(0, "actor variant", &[ACTV])?
                .map(|index| tag_ref(cache, index)))
        })?,
        actor_variants: each(scenario, ACTOR_PALETTE, "actor palette", |e| {
            e.reference(0, "actor variant", &[ACTV])?
                .map(|index| import_actor_variant(cache, index))
                .transpose()
        })?,
        encounters: each(scenario, ENCOUNTERS, "encounters", import_encounter)?,
        command_lists: each(scenario, COMMAND_LISTS, "command lists", |e| {
            Ok(CommandList {
                name: name(e, 0)?,
                flags: e.u32(0x20, "flags")?,
                commands: each(e, COMMANDS, "commands", |c| {
                    Ok(Command {
                        atom: c.i16(0, "atom")?,
                        modifier: c.i16(2, "modifier")?,
                        parameters: [c.f32(4, "parameter")?, c.f32(8, "parameter")?],
                        points: [c.i16(0xC, "point")?, c.i16(0xE, "point")?],
                        object_name: c.i16(0x18, "object name")?,
                    })
                })?,
                points: each(e, POINTS, "points", |p| point(p, 0, "point"))?,
            })
        })?,
        conversations: each(scenario, CONVERSATIONS, "conversations", |e| name(e, 0))?,
        starting_profiles: each(scenario, STARTING_PROFILES, "starting profiles", |e| {
            name(e, 0)
        })?,
    })
}

/// An actor variant and the parts of its actor its actors fight by
/// (`actor_variant_definition`, `actor_definition`).
fn import_actor_variant(cache: &Cache, index: usize) -> Result<ActorVariant, ImportError> {
    let variant = cache.tag(index, ACTV, ACTOR_VARIANT_SIZE)?;
    let pair = |at: usize, field: &str| -> Result<[f32; 2], ImportError> {
        Ok([variant.f32(at, field)?, variant.f32(at + 4, field)?])
    };
    let actor = variant.reference(0x04, "actor", &[ACTR])?;
    let unit = variant.reference(0x14, "unit", &[UNIT])?;
    let weapon = variant.reference(0x64, "weapon", &[WEAP])?;
    let mut imported = ActorVariant {
        tag: tag_ref(cache, index),
        unit: unit.map(|index| tag_ref(cache, index)),
        weapon: weapon.map(|index| tag_ref(cache, index)),
        kind: 0,
        vision_distance: 0.0,
        vision_angle: 0.0,
        hearing_distance: 0.0,
        firing_range: variant.f32(0x74, "maximum firing range")?,
        rate_of_fire: variant.f32(0x78, "rate of fire")?,
        error_angle: variant.f32(0x7C, "projectile error angle")?,
        combat_range: pair(0x9C, "combat range")?,
        burst_duration: pair(0xE0, "burst duration")?,
        burst_separation: pair(0xE8, "burst separation")?,
        panic_chance: 0.0,
        shield_fraction_hide: 0.0,
        shield_fraction_emerge: 0.0,
    };
    if let Some(index) = actor {
        let actor = cache.tag(index, ACTR, ACTOR_SIZE)?;
        imported.kind = actor.i16(0x14, "type")?;
        imported.vision_distance = actor.f32(0x18, "maximum vision distance")?;
        imported.vision_angle = actor.f32(0x20, "maximum vision angle")?;
        imported.hearing_distance = actor.f32(0x4C, "hearing distance")?;
        imported.panic_chance = actor.f32(0x2A0, "panic chance")?;
        imported.shield_fraction_hide = actor.f32(0x2DC, "shield fraction hide")?;
        imported.shield_fraction_emerge = actor.f32(0x2E0, "shield fraction emerge")?;
    }
    Ok(imported)
}

fn import_encounter(e: &Element<'_>) -> Result<Encounter, ImportError> {
    Ok(Encounter {
        name: name(e, 0)?,
        flags: e.u32(0x20, "flags")?,
        team: e.i16(0x24, "team")?,
        squads: each(e, SQUADS, "squads", |s| {
            let mut firing_position_groups = [0; 8];
            for (index, groups) in firing_position_groups.iter_mut().enumerate() {
                *groups = s.u32(0x54 + index * 4, "firing position groups")?;
            }
            Ok(Squad {
                name: name(s, 0)?,
                actor: s.i16(0x20, "actor type")?,
                platoon: s.i16(0x22, "platoon")?,
                initial_state: s.i16(0x24, "initial state")?,
                default_state: s.i16(0x26, "default state")?,
                flags: s.u32(0x28, "flags")?,
                firing_position_groups,
                count: [s.i16(0x7C, "count")?, s.i16(0x7E, "count")?],
                starting_locations: each(s, STARTING_LOCATIONS, "starting locations", |l| {
                    Ok(StartingLocation {
                        position: point(l, 0, "position")?,
                        facing: l.f32(0xC, "facing")?,
                        initial_state: l.i16(0x16, "initial state")?,
                        actor: l.i16(0x18, "actor type")?,
                        command_list: l.i16(0x1A, "command list")?,
                    })
                })?,
            })
        })?,
        platoons: each(e, PLATOONS, "platoons", |p| name(p, 0))?,
        firing_positions: each(e, FIRING_POSITIONS, "firing positions", |f| {
            Ok(FiringPosition {
                position: point(f, 0, "position")?,
                group: f.i16(0xC, "group")?,
            })
        })?,
    })
}
