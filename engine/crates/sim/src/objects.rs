//! Objects: the table their handles index, the components each has, the
//! hooks each kind adds its own with, and spawning the scenario's.
//!
//! A handle is a slot and that slot's salt, as the game's datum handles
//! are: removing an object raises its slot's salt, so a handle kept from
//! before is refused rather than finding whatever took the slot next.

use std::collections::BTreeSet;

use bevy_ecs::prelude::*;
use glam::{Mat3, Quat, Vec3};
use opence_content::{
    CollisionBsp, Level, ObjectKind, ObjectPlacement, PlacementPermutation, TagRef,
};

use crate::{ObjectId, Transform};

/// The most objects a world holds, as the game's object table does.
pub const MAX_OBJECTS: usize = 2048;

/// An object, by its slot and the slot's salt when it was made.
#[derive(Component, Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct ObjectHandle {
    /// Its slot in the table.
    pub index: u32,
    /// The slot's salt.
    pub salt: u32,
}

/// What an object is: its kind and its definition's tag.
#[derive(Component, Clone, Debug, PartialEq, Eq)]
pub struct Object {
    /// Its kind.
    pub kind: ObjectKind,
    /// Its object tag.
    pub tag: TagRef,
}

/// An object's name, from the scenario's names.
#[derive(Component, Clone, Debug, PartialEq, Eq)]
pub struct ObjectName(pub String);

/// An object's colors and regions' permutations.
#[derive(Component, Clone, Copy, Debug, PartialEq, Eq)]
pub struct Permutation(pub PlacementPermutation);

/// How an object is drawn: its model, the radius its level of detail is
/// chosen by, and the permutation each of its model's regions is drawn
/// with. The simulation keeps it for the renderer and never reads it.
#[derive(Component, Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Appearance {
    /// Its model's index in the level's models.
    pub model: u32,
    /// Its bounding sphere's radius, as bits (so that the component is
    /// `Eq`, as the others are).
    pub radius_bits: u32,
    /// The permutation of each region.
    pub regions: [u8; 8],
    /// Its four change colours, as `0xFFRRGGBB`.
    pub change_colors: [u32; 4],
}

/// The team an object is on (multiplayer).
#[derive(Component, Clone, Copy, Debug, PartialEq, Eq)]
pub struct Team(pub u16);

/// A unit: a biped or a vehicle.
#[derive(Component, Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Unit;

/// An item: a weapon, equipment or garbage.
#[derive(Component, Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Item;

/// A device: a machine, a control or a light fixture.
#[derive(Component, Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Device;

/// An object to make.
#[derive(Clone, Debug, PartialEq)]
pub struct ObjectSpawn {
    /// Its kind.
    pub kind: ObjectKind,
    /// Its object tag.
    pub tag: TagRef,
    /// Where it is.
    pub transform: Transform,
    /// Its name.
    pub name: Option<String>,
    /// Its colors and regions' permutations.
    pub permutation: Option<PlacementPermutation>,
    /// Its team.
    pub team: Option<u16>,
}

/// The game's difficulties.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Difficulty {
    /// Easy.
    Easy,
    /// Normal.
    Normal,
    /// Heroic.
    Hard,
    /// Legendary.
    Legendary,
}

/// An object table that cannot be added to.
#[derive(Clone, Copy, Debug, PartialEq, Eq, thiserror::Error)]
#[error("the world holds {MAX_OBJECTS} objects already")]
pub struct TableFull;

#[derive(Clone, Copy, Debug, Default)]
struct Slot {
    salt: u32,
    entity: Option<Entity>,
}

/// Every object's slot: its entity and its salt.
#[derive(Resource, Debug, Default)]
pub struct ObjectTable {
    slots: Vec<Slot>,
    /// The empty slots, the lowest taken first.
    free: BTreeSet<u32>,
}

impl ObjectTable {
    /// The entity `handle` is, if it is still in the world.
    pub fn entity(&self, handle: ObjectHandle) -> Option<Entity> {
        let slot = self.slots.get(handle.index as usize)?;
        if slot.salt == handle.salt {
            slot.entity
        } else {
            None
        }
    }

    /// How many objects there are.
    pub fn len(&self) -> usize {
        self.slots.len() - self.free.len()
    }

    /// Whether there are none.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    fn take(&mut self, entity: Entity) -> Result<ObjectHandle, TableFull> {
        let index = match self.free.pop_first() {
            Some(index) => index,
            None if self.slots.len() < MAX_OBJECTS => {
                self.slots.push(Slot::default());
                (self.slots.len() - 1) as u32
            }
            None => return Err(TableFull),
        };
        let slot = &mut self.slots[index as usize];
        slot.entity = Some(entity);
        Ok(ObjectHandle {
            index,
            salt: slot.salt,
        })
    }

    fn release(&mut self, handle: ObjectHandle) -> Option<Entity> {
        let entity = self.entity(handle)?;
        let slot = &mut self.slots[handle.index as usize];
        slot.entity = None;
        slot.salt = slot.salt.wrapping_add(1);
        self.free.insert(handle.index);
        Some(entity)
    }
}

/// What a kind adds to each of its objects as it is made.
pub type KindHook = fn(&mut EntityWorldMut<'_>);

/// Each kind's hook: the components its objects have beyond every
/// object's. Later systems (physics, weapons) register their own.
#[derive(Resource, Clone, Debug)]
pub struct ObjectKinds {
    hooks: [KindHook; ObjectKind::ALL.len()],
}

impl Default for ObjectKinds {
    fn default() -> Self {
        let mut kinds = Self {
            hooks: [|_| {}; ObjectKind::ALL.len()],
        };
        for kind in [ObjectKind::Biped, ObjectKind::Vehicle] {
            kinds.register(kind, |entity| {
                entity.insert(Unit);
            });
        }
        for kind in [
            ObjectKind::Weapon,
            ObjectKind::Equipment,
            ObjectKind::Garbage,
        ] {
            kinds.register(kind, |entity| {
                entity.insert(Item);
            });
        }
        for kind in [
            ObjectKind::Machine,
            ObjectKind::Control,
            ObjectKind::LightFixture,
        ] {
            kinds.register(kind, |entity| {
                entity.insert(Device);
            });
        }
        kinds
    }
}

impl ObjectKinds {
    /// Makes `hook` the one `kind`'s objects are made with.
    pub fn register(&mut self, kind: ObjectKind, hook: KindHook) {
        self.hooks[kind_index(kind)] = hook;
    }

    /// The hook `kind`'s objects are made with.
    pub fn hook(&self, kind: ObjectKind) -> KindHook {
        self.hooks[kind_index(kind)]
    }
}

/// A kind's place in [`ObjectKind::ALL`]: the game's type field.
pub fn kind_index(kind: ObjectKind) -> usize {
    ObjectKind::ALL
        .iter()
        .position(|other| *other == kind)
        .unwrap_or_default()
}

/// Makes an object in `world`: its components, then its kind's.
pub(crate) fn spawn(world: &mut World, spawn: ObjectSpawn) -> Result<ObjectHandle, TableFull> {
    let hook = world.get_resource_or_init::<ObjectKinds>().hook(spawn.kind);
    let entity = world.spawn_empty().id();
    let handle = match world.get_resource_or_init::<ObjectTable>().take(entity) {
        Ok(handle) => handle,
        Err(full) => {
            world.despawn(entity);
            return Err(full);
        }
    };
    let mut object = world.entity_mut(entity);
    object.insert((
        handle,
        ObjectId(handle.index),
        spawn.transform,
        crate::PreviousTransform(spawn.transform),
        Object {
            kind: spawn.kind,
            tag: spawn.tag,
        },
    ));
    if let Some(name) = spawn.name {
        object.insert(ObjectName(name));
    }
    if let Some(permutation) = spawn.permutation {
        object.insert(Permutation(permutation));
    }
    if let Some(team) = spawn.team {
        object.insert(Team(team));
    }
    hook(&mut object);
    Ok(handle)
}

/// Removes the object `handle` is; false if it was gone already.
pub(crate) fn remove(world: &mut World, handle: ObjectHandle) -> bool {
    let Some(entity) = world
        .get_resource_mut::<ObjectTable>()
        .and_then(|mut table| table.release(handle))
    else {
        return false;
    };
    world.despawn(entity)
}

/// Whether `placement` is placed when a level starts: it is placed
/// automatically. (The game reads none of the flags that would leave it
/// out of a difficulty.)
pub fn is_placed(placement: &ObjectPlacement) -> bool {
    placement.is_automatic()
}

/// Whether the game places `kind` as each structure BSP is loaded, only if
/// it is in that BSP, and removes it (unless it is named) as the BSP is
/// let go of (`_object_mask_remove_on_bsp_switch`).
pub fn placed_by_structure_bsp(kind: ObjectKind) -> bool {
    matches!(kind, ObjectKind::Scenery | ObjectKind::LightFixture)
}

/// Whether `placement` is in the structure BSP `bsp`, as the game tests it
/// (`object_types_place_objects`): its origin, or its bounding sphere's
/// centre, is in one of the BSP's leaves.
fn in_structure_bsp(level: &Level, bsp: &CollisionBsp, placement: &ObjectPlacement) -> bool {
    let offset = level
        .object(&placement.tag)
        .map_or([0.0; 3], |object| object.bounding_offset);
    let centre = rotation_from_angles(placement.rotation) * Vec3::from_array(offset)
        + Vec3::from_array(placement.position);
    bsp.leaf_at(placement.position).is_some() || bsp.leaf_at(centre.to_array()).is_some()
}

/// The scenery and light fixtures the game places as `level`'s structure
/// BSP `structure_bsp` is loaded, each with its placement's index, in the
/// scenario's order: those placed automatically that are in it (none if
/// the level has no such BSP).
fn structure_bsp_placements(level: &Level, structure_bsp: usize) -> Vec<usize> {
    let Some(bsp) = level.structure_bsps.get(structure_bsp) else {
        return Vec::new();
    };
    let placements = &level.scenario.placements;
    (0..placements.len())
        .filter(|&index| {
            let placement = &placements[index];
            placed_by_structure_bsp(placement.kind)
                && is_placed(placement)
                && in_structure_bsp(level, &bsp.collision, placement)
        })
        .collect()
}

/// The placement `index` of `level` made an object to spawn: a vehicle
/// with physics is raised half its bounding radius (`vehicle_new`; the
/// importer has raised weapons and equipment).
fn level_spawn(level: &Level, index: usize) -> ObjectSpawn {
    let placement = &level.scenario.placements[index];
    let mut spawn = placement_spawn(placement);
    if placement.kind == ObjectKind::Vehicle {
        if let Some(object) = level.object(&placement.tag) {
            if object.physics.is_some() {
                spawn.transform.position.z += object.bounding_radius * 0.5;
            }
        }
    }
    spawn
}

/// The placements in `order` whose names are not in `names` (the names of
/// objects there are, which those placed are added to), each made an
/// object to spawn (`object_new_from_scenario`).
fn unnamed_yet<'a>(
    level: &'a Level,
    order: impl IntoIterator<Item = usize>,
    names: &mut BTreeSet<&'a str>,
) -> Vec<(usize, ObjectSpawn)> {
    order
        .into_iter()
        .filter(|&index| match &level.scenario.placements[index].name {
            Some(name) => names.insert(name.as_str()),
            None => true,
        })
        .map(|index| (index, level_spawn(level, index)))
        .collect()
}

/// The objects the game places as `level` starts in its structure BSP
/// `structure_bsp`, each with its placement's index, in the order it places
/// them: the placements of each kind in the game's order
/// (`object_types_place_all`), then the scenery and light fixtures in the
/// BSP (`object_types_place_objects`). A placement not placed
/// automatically, or named as one already placed, is left out.
pub fn level_placements(level: &Level, structure_bsp: usize) -> Vec<(usize, ObjectSpawn)> {
    let placements = &level.scenario.placements;
    let mut order: Vec<usize> = (0..placements.len())
        .filter(|&index| {
            !placed_by_structure_bsp(placements[index].kind) && is_placed(&placements[index])
        })
        .collect();
    order.sort_by_key(|&index| {
        let kind = placements[index].kind;
        ObjectKind::ALL.iter().position(|&each| each == kind)
    });
    order.extend(structure_bsp_placements(level, structure_bsp));
    unnamed_yet(level, order, &mut BTreeSet::new())
}

/// The orientation of a yaw, pitch and roll, as the game builds it
/// (`matrix4x3_rotation_from_angles`): forward, left and up are the
/// matrix's rows.
pub fn rotation_from_angles([yaw, pitch, roll]: [f32; 3]) -> Quat {
    let (sine_yaw, cosine_yaw) = (libm::sinf(yaw), libm::cosf(yaw));
    let (sine_pitch, cosine_pitch) = (libm::sinf(pitch), libm::cosf(pitch));
    let (sine_roll, cosine_roll) = (libm::sinf(roll), libm::cosf(roll));
    let sine_pitch_cosine_roll = sine_pitch * cosine_roll;
    let sine_pitch_sine_roll = sine_pitch * sine_roll;
    let forward = Vec3::new(
        cosine_yaw * cosine_pitch,
        sine_yaw * cosine_roll - cosine_yaw * sine_pitch_sine_roll,
        cosine_yaw * sine_pitch_cosine_roll + sine_yaw * sine_roll,
    );
    let left = Vec3::new(
        -(sine_yaw * cosine_pitch),
        sine_yaw * sine_pitch_sine_roll + cosine_yaw * cosine_roll,
        cosine_yaw * sine_roll - sine_yaw * sine_pitch_cosine_roll,
    );
    let up = Vec3::new(
        -sine_pitch,
        -(cosine_pitch * sine_roll),
        cosine_pitch * cosine_roll,
    );
    Quat::from_mat3(&Mat3::from_cols(forward, left, up)).normalize()
}

/// What a placement makes.
pub fn placement_spawn(placement: &ObjectPlacement) -> ObjectSpawn {
    ObjectSpawn {
        kind: placement.kind,
        tag: placement.tag.clone(),
        transform: Transform {
            position: Vec3::from_array(placement.position),
            rotation: rotation_from_angles(placement.rotation),
            scale: 1.0,
        },
        name: placement.name.clone(),
        permutation: placement.permutation,
        team: None,
    }
}

/// Makes every object the game places as `level` starts in its structure
/// BSP `structure_bsp` ([`level_placements`]).
pub(crate) fn spawn_scenario(
    world: &mut World,
    level: &Level,
    structure_bsp: usize,
) -> Result<Vec<ObjectHandle>, TableFull> {
    level_placements(level, structure_bsp)
        .into_iter()
        .map(|(_, made)| spawn(world, made))
        .collect()
}

/// Moves the objects to `level`'s structure BSP `structure_bsp`, as the
/// game does as it switches to it: every scenery and light fixture not
/// named is removed (`object_types_disconnect_from_structure_bsp`), then
/// those in the new BSP are placed, but for those named as an object there
/// is. The objects made, each with its placement's index.
pub(crate) fn switch_structure_bsp(
    world: &mut World,
    level: &Level,
    structure_bsp: usize,
) -> Result<Vec<(usize, ObjectHandle)>, TableFull> {
    let mut removed = Vec::new();
    let mut named = BTreeSet::new();
    let mut objects = world.query::<(&ObjectHandle, &Object, Option<&ObjectName>)>();
    for (&handle, object, name) in objects.iter(world) {
        match name {
            Some(name) => {
                named.insert(name.0.clone());
            }
            None if placed_by_structure_bsp(object.kind) => removed.push(handle),
            None => {}
        }
    }
    for handle in removed {
        remove(world, handle);
    }
    let mut names = named.iter().map(String::as_str).collect();
    unnamed_yet(
        level,
        structure_bsp_placements(level, structure_bsp),
        &mut names,
    )
    .into_iter()
    .map(|(index, made)| Ok((index, spawn(world, made)?)))
    .collect()
}

#[cfg(test)]
mod tests {
    use std::error::Error;

    use opence_content::{Group, ObjectDefinition, Scenario};

    use super::*;
    use crate::Simulation;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn placement(kind: ObjectKind, path: &str, placement_flags: u16) -> ObjectPlacement {
        ObjectPlacement {
            kind,
            tag: tag(&kind.group(), path),
            name: None,
            placement_flags,
            position: [1.0, 2.0, 3.0],
            rotation: [0.0; 3],
            structure_bsps: 1,
            permutation: None,
            device: None,
        }
    }

    fn scenario() -> Scenario {
        let permutation = PlacementPermutation {
            change_colors: [0xFF00_00FF, 0, 0, 0],
            region_permutations: [2, 0, 0, 0, 0, 0, 0, 0],
        };
        Scenario {
            path: "levels\\test\\box\\box".into(),
            structure_bsps: Vec::new(),
            player_starts: Vec::new(),
            placements: vec![
                ObjectPlacement {
                    name: Some("hog".into()),
                    position: [4.0, 5.0, 6.0],
                    rotation: [core::f32::consts::FRAC_PI_2, 0.0, 0.0],
                    permutation: Some(permutation),
                    ..placement(ObjectKind::Vehicle, "vehicles\\warthog\\warthog", 0)
                },
                placement(ObjectKind::Scenery, "scenery\\rock", 1),
                placement(ObjectKind::Weapon, "weapons\\pistol\\pistol", 1 << 1),
                placement(ObjectKind::Machine, "levels\\test\\door", 0),
            ],
            netgame_flags: Vec::new(),
            netgame_equipment: Vec::new(),
            scripts: Default::default(),
            campaign: Default::default(),
            decal_palette: Vec::new(),
        }
    }

    /// A definition of `kind` at `path`, its bounding sphere of `radius`
    /// at `offset`, with physics if `physics`.
    fn definition(
        kind: ObjectKind,
        path: &str,
        radius: f32,
        offset: [f32; 3],
        physics: bool,
    ) -> ObjectDefinition {
        ObjectDefinition {
            widgets: Vec::new(),
            tag: tag(&kind.group(), path),
            kind,
            flags: 0,
            bounding_radius: radius,
            bounding_offset: offset,
            acceleration_scale: 1.0,
            render_bounding_radius: radius,
            model: None,
            animation_graph: None,
            collision_model: None,
            physics: physics.then(|| tag(b"phys", path)),
            modifier_shader: None,
            creation_effect: None,
            function_modes: Default::default(),
            functions: Vec::new(),
            attachments: Vec::new(),
            change_colors: Vec::new(),
            unit_hud: None,
            unit_melee_damage: None,
            biped: None,
            weapon: None,
            projectile: None,
            equipment: None,
            vehicle: None,
        }
    }

    /// The devmap's plane with `scenario`, the warthog's definition (a
    /// vehicle with physics, its bounding radius 2) and the rock's (its
    /// bounding sphere 6 above its origin).
    fn level(scenario: Scenario) -> Level {
        let mut level = opence_content::devmap::level(None);
        level.scenario = scenario;
        level.objects.push(definition(
            ObjectKind::Vehicle,
            "vehicles\\warthog\\warthog",
            2.0,
            [0.0; 3],
            true,
        ));
        level.objects.push(definition(
            ObjectKind::Scenery,
            "scenery\\rock",
            1.0,
            [0.0, 0.0, 6.0],
            false,
        ));
        level
    }

    #[test]
    fn the_scenarios_placements_spawn_with_their_tag_position_and_permutation(
    ) -> Result<(), Box<dyn Error>> {
        let mut simulation = Simulation::empty();
        let level = level(scenario());
        let handles = simulation.spawn_scenario(&level, 0)?;
        // (the rock is not placed automatically; the pistol is placed at
        // every difficulty, as the game reads none of those flags)
        assert_eq!(handles.len(), 3);
        assert_eq!(simulation.objects(), handles);
        let hog = handles[0];
        assert_eq!(
            simulation.object(hog),
            Some(Object {
                kind: ObjectKind::Vehicle,
                tag: tag(b"vehi", "vehicles\\warthog\\warthog"),
            })
        );
        let transform = simulation.transform(hog).ok_or("no transform")?;
        // (raised half its bounding radius, as it has physics)
        assert_eq!(transform.position, Vec3::new(4.0, 5.0, 7.0));
        // (a quarter turn left: forward is +y)
        assert!((transform.rotation * Vec3::X - Vec3::Y).length() < 1e-6);
        assert_eq!(
            simulation.component::<ObjectName>(hog),
            Some(ObjectName("hog".into()))
        );
        assert_eq!(
            simulation.component::<Permutation>(hog),
            level.scenario.placements[0].permutation.map(Permutation)
        );
        assert_eq!(simulation.component::<Team>(hog), None);
        // each kind's own components
        assert_eq!(simulation.component::<Unit>(hog), Some(Unit));
        assert_eq!(simulation.component::<Item>(handles[1]), Some(Item));
        assert_eq!(simulation.component::<Device>(handles[2]), Some(Device));
        assert_eq!(simulation.component::<Unit>(handles[2]), None);
        Ok(())
    }

    #[test]
    fn scenery_is_placed_after_the_rest_and_only_in_the_bsp() {
        let rock = |z: f32| ObjectPlacement {
            position: [1.0, 2.0, z],
            ..placement(ObjectKind::Scenery, "scenery\\rock", 0)
        };
        let lamp = |z: f32| ObjectPlacement {
            position: [1.0, 2.0, z],
            ..placement(ObjectKind::LightFixture, "scenery\\lamp", 0)
        };
        let mut scenario = scenario();
        scenario.placements = vec![
            rock(3.0),
            // (under the plane, in no leaf, its bounding sphere too)
            rock(-30.0),
            // (under the plane, its bounding sphere above it)
            rock(-3.0),
            // (under the plane, with no bounding offset)
            lamp(-3.0),
            lamp(3.0),
            placement(ObjectKind::Biped, "characters\\cyborg\\cyborg", 0),
        ];
        let level = level(scenario);
        let placed: Vec<usize> = level_placements(&level, 0)
            .into_iter()
            .map(|(index, _)| index)
            .collect();
        assert_eq!(placed, [5, 0, 2, 4]);
        // with no such structure BSP, none of them
        let placed: Vec<usize> = level_placements(&level, 1)
            .into_iter()
            .map(|(index, _)| index)
            .collect();
        assert_eq!(placed, [5]);
    }

    #[test]
    fn a_switch_moves_the_scenery_to_the_new_bsp() -> Result<(), Box<dyn Error>> {
        let rock = |z: f32, name: Option<&str>| ObjectPlacement {
            position: [0.0, 0.0, z],
            name: name.map(Into::into),
            ..placement(ObjectKind::Scenery, "scenery\\rock", 0)
        };
        let mut scenario = scenario();
        scenario.placements = vec![
            // (above the plane: in the first BSP only)
            rock(3.0, None),
            rock(3.0, Some("kept")),
            // (well under it: in the second only)
            rock(-30.0, None),
            rock(-30.0, Some("kept")),
            rock(-30.0, Some("new")),
            placement(ObjectKind::Weapon, "weapons\\pistol\\pistol", 0),
        ];
        let mut level = level(scenario);
        // (the second: the plane turned over, its leaf below it)
        let mut under = opence_content::devmap::plane(10.0, 1);
        under.collision.planes[0] = [0.0, 0.0, -1.0, 0.0];
        level.structure_bsps.push(under);

        let mut simulation = Simulation::empty();
        let first = simulation.spawn_scenario(&level, 0)?;
        assert_eq!(first.len(), 3);
        let made = simulation.place_structure_bsp(&level, 1)?;
        let indices: Vec<usize> = made.iter().map(|&(index, _)| index).collect();
        // (the unnamed rock of the first is gone; "kept" stays as it was)
        assert_eq!(indices, [2, 4]);
        assert_eq!(simulation.objects().len(), 4);
        assert!(simulation.object(first[1]).is_none());
        assert!(simulation.object(first[0]).is_some());
        // no such BSP: the unnamed scenery goes, none is made
        assert!(simulation.place_structure_bsp(&level, 9)?.is_empty());
        assert_eq!(simulation.objects().len(), 3);
        Ok(())
    }

    #[test]
    fn a_name_is_placed_once() {
        let named = |kind, path| ObjectPlacement {
            name: Some("thing".into()),
            ..placement(kind, path, 0)
        };
        let mut scenario = scenario();
        scenario.placements = vec![
            named(ObjectKind::Scenery, "scenery\\rock"),
            named(ObjectKind::Weapon, "weapons\\pistol\\pistol"),
            named(ObjectKind::Weapon, "weapons\\rifle\\rifle"),
            placement(ObjectKind::Weapon, "weapons\\rifle\\rifle", 0),
        ];
        let level = level(scenario);
        let placed: Vec<usize> = level_placements(&level, 0)
            .into_iter()
            .map(|(index, _)| index)
            .collect();
        // (the weapons first: the rock's name is taken)
        assert_eq!(placed, [1, 3]);
    }

    #[test]
    fn a_spawn_has_its_team() -> Result<(), Box<dyn Error>> {
        let mut simulation = Simulation::empty();
        let spawn = ObjectSpawn {
            team: Some(1),
            ..placement_spawn(&placement(
                ObjectKind::Biped,
                "characters\\cyborg\\cyborg",
                0,
            ))
        };
        let cyborg = simulation.spawn(spawn)?;
        assert_eq!(simulation.component::<Team>(cyborg), Some(Team(1)));
        Ok(())
    }

    #[test]
    fn a_handle_cannot_be_used_after_its_object_is_removed() -> Result<(), Box<dyn Error>> {
        let mut simulation = Simulation::empty();
        let rock = || placement_spawn(&placement(ObjectKind::Scenery, "scenery\\rock", 0));
        let first = simulation.spawn(rock())?;
        let second = simulation.spawn(rock())?;
        assert!(simulation.remove(first));
        assert_eq!(simulation.object(first), None);
        assert_eq!(simulation.transform(first), None);
        assert!(!simulation.remove(first));
        // the slot is taken again, with another salt: the old handle still
        // finds nothing
        let third = simulation.spawn(rock())?;
        assert_eq!(third.index, first.index);
        assert_ne!(third.salt, first.salt);
        assert_eq!(simulation.object(first), None);
        assert!(simulation.object(third).is_some());
        assert_eq!(simulation.objects(), [third, second]);
        // a handle from another table's slots is nothing here
        let past = ObjectHandle { index: 99, salt: 0 };
        assert_eq!(simulation.object(past), None);
        assert!(!simulation.remove(past));
        Ok(())
    }

    #[test]
    fn the_table_holds_its_most_and_no_more() -> Result<(), Box<dyn Error>> {
        let mut simulation = Simulation::empty();
        let rock = placement_spawn(&placement(ObjectKind::Scenery, "scenery\\rock", 0));
        let mut handles = Vec::new();
        for _ in 0..MAX_OBJECTS {
            handles.push(simulation.spawn(rock.clone())?);
        }
        assert_eq!(simulation.spawn(rock.clone()), Err(TableFull));
        assert_eq!(simulation.objects().len(), MAX_OBJECTS);
        assert!(simulation.remove(handles[7]));
        assert_eq!(simulation.spawn(rock)?.index, 7);
        Ok(())
    }

    #[test]
    fn a_registered_hook_adds_its_components() -> Result<(), Box<dyn Error>> {
        #[derive(Component, Clone, Debug, PartialEq)]
        struct Glows;
        let mut simulation = Simulation::empty();
        simulation
            .kinds_mut()
            .register(ObjectKind::Scenery, |entity| {
                entity.insert(Glows);
            });
        let rock = simulation.spawn(placement_spawn(&placement(
            ObjectKind::Scenery,
            "scenery\\rock",
            0,
        )))?;
        assert_eq!(simulation.component::<Glows>(rock), Some(Glows));
        Ok(())
    }

    #[test]
    fn angles_turn_as_the_games_matrix_does() {
        // yaw left, pitch up, roll: each about its own axis
        let close = |a: Vec3, b: Vec3| (a - b).length() < 1e-6;
        let yaw = rotation_from_angles([core::f32::consts::FRAC_PI_2, 0.0, 0.0]);
        assert!(close(yaw * Vec3::X, Vec3::Y));
        let pitch = rotation_from_angles([0.0, core::f32::consts::FRAC_PI_2, 0.0]);
        assert!(close(pitch * Vec3::X, Vec3::Z));
        assert!(close(pitch * Vec3::Z, -Vec3::X));
        let roll = rotation_from_angles([0.0, 0.0, core::f32::consts::FRAC_PI_2]);
        assert!(close(roll * Vec3::X, Vec3::X));
        assert!(close(roll * Vec3::Z, -Vec3::Y));
        assert!(close(rotation_from_angles([0.0; 3]) * Vec3::Y, Vec3::Y));
    }
}
