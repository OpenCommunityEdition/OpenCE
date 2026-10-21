//! Objects: the table their handles index, the components each has, the
//! hooks each kind adds its own with, and spawning the scenario's.
//!
//! A handle is a slot and that slot's salt, as the game's datum handles
//! are: removing an object raises its slot's salt, so a handle kept from
//! before is refused rather than finding whatever took the slot next.

use std::collections::BTreeSet;

use bevy_ecs::prelude::*;
use glam::{Mat3, Quat, Vec3};
use opence_content::{ObjectKind, ObjectPlacement, PlacementPermutation, Scenario, TagRef};

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

/// The game's difficulties, which a placement may be left out of.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Difficulty {
    /// Easy.
    Easy,
    /// Normal.
    Normal,
    /// Heroic.
    Hard,
    /// Legendary: every placement is in it.
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

/// Whether `placement` is placed when a level starts at `difficulty`.
pub fn is_placed(placement: &ObjectPlacement, difficulty: Difficulty) -> bool {
    let left_out = match difficulty {
        Difficulty::Easy => 1 << 1,
        Difficulty::Normal => 1 << 2,
        Difficulty::Hard => 1 << 3,
        Difficulty::Legendary => 0,
    };
    placement.is_automatic() && placement.placement_flags & left_out == 0
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

/// Makes every object `scenario` places at `difficulty`, in its order.
pub(crate) fn spawn_scenario(
    world: &mut World,
    scenario: &Scenario,
    difficulty: Difficulty,
) -> Result<Vec<ObjectHandle>, TableFull> {
    scenario
        .placements
        .iter()
        .filter(|placement| is_placed(placement, difficulty))
        .map(|placement| spawn(world, placement_spawn(placement)))
        .collect()
}

#[cfg(test)]
mod tests {
    use std::error::Error;

    use opence_content::Group;

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
        }
    }

    #[test]
    fn the_scenarios_placements_spawn_with_their_tag_position_and_permutation(
    ) -> Result<(), Box<dyn Error>> {
        let mut simulation = Simulation::empty();
        let scenario = scenario();
        let handles = simulation.spawn_scenario(&scenario, Difficulty::Normal)?;
        // (the rock is not placed automatically)
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
        assert_eq!(transform.position, Vec3::new(4.0, 5.0, 6.0));
        // (a quarter turn left: forward is +y)
        assert!((transform.rotation * Vec3::X - Vec3::Y).length() < 1e-6);
        assert_eq!(
            simulation.component::<ObjectName>(hog),
            Some(ObjectName("hog".into()))
        );
        assert_eq!(
            simulation.component::<Permutation>(hog),
            scenario.placements[0].permutation.map(Permutation)
        );
        assert_eq!(simulation.component::<Team>(hog), None);
        // each kind's own components
        assert_eq!(simulation.component::<Unit>(hog), Some(Unit));
        assert_eq!(simulation.component::<Item>(handles[1]), Some(Item));
        assert_eq!(simulation.component::<Device>(handles[2]), Some(Device));
        assert_eq!(simulation.component::<Unit>(handles[2]), None);

        // the pistol is left out of easy
        let mut easy = Simulation::empty();
        assert_eq!(easy.spawn_scenario(&scenario, Difficulty::Easy)?.len(), 2);
        Ok(())
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
