//! Objects and their lists: made and destroyed by name, moved to cutscene
//! flags, attached, and tested against trigger volumes and what can be
//! seen (`hs_library_objects.c`, `hs_library_volumes.c`).

use opence_content::{CutsceneFlag, TriggerVolume};
use opence_script::{Native, Value};
use opence_sim::objects::{placement_spawn, rotation_from_angles, ObjectHandle};
use opence_sim::{Transform, Vec3};

use crate::{element, index, real, short, text, Mission};

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("list_count", list_count),
    ("list_get", list_get),
    ("players", players),
    ("object_create", object_create),
    ("object_create_anew", object_create_anew),
    ("object_create_containing", object_create_containing),
    (
        "object_create_anew_containing",
        object_create_anew_containing,
    ),
    ("object_destroy", object_destroy),
    ("object_destroy_all", object_destroy_all),
    ("object_destroy_containing", object_destroy_containing),
    ("object_teleport", object_teleport),
    ("object_set_facing", object_set_facing),
    ("object_set_scale", object_set_scale),
    ("objects_attach", objects_attach),
    ("objects_detach", objects_detach),
    ("objects_can_see_flag", objects_can_see_flag),
    ("objects_can_see_object", objects_can_see_object),
    ("object_can_take_damage", object_can_take_damage),
    ("object_cannot_take_damage", object_cannot_take_damage),
    ("volume_test_object", volume_test_object),
    ("volume_test_objects", volume_test_objects),
    ("volume_test_objects_all", volume_test_objects_all),
    (
        "volume_teleport_players_not_inside",
        volume_teleport_players_not_inside,
    ),
];

impl Mission {
    /// Makes the object of scenario object name `name` if it is not in
    /// the world (`object_create`); with `anew`, the one there is
    /// destroyed first.
    pub fn create_named(&mut self, name: usize, anew: bool) -> Option<ObjectHandle> {
        if let Some(handle) = self.named(name) {
            if !anew {
                return Some(handle);
            }
            self.sim.remove(handle);
        }
        let placement = self
            .level
            .scenario
            .placements
            .get((*self.placements.get(name)?)?)?;
        let spawn = placement_spawn(placement);
        let device = placement.device;
        let handle = self.sim.spawn(spawn).ok()?;
        self.named[name] = Some(handle);
        if let Some(device) = device {
            self.devices
                .insert(handle, crate::DeviceState::of(&device, &self.groups));
        }
        Some(handle)
    }

    /// The scenario's object names that contain `part`.
    fn names_containing(&self, part: &str) -> Vec<usize> {
        self.level
            .scenario
            .campaign
            .object_names
            .iter()
            .enumerate()
            .filter(|(_, name)| name.contains(part))
            .map(|(index, _)| index)
            .collect()
    }

    fn flag(&self, flag: i32) -> Option<&CutsceneFlag> {
        element(&self.level.scenario.campaign.cutscene_flags, flag)
    }

    fn volume(&self, volume: i32) -> Option<&TriggerVolume> {
        element(&self.level.scenario.campaign.trigger_volumes, volume)
    }

    /// Whether an object stands inside a trigger volume.
    fn inside(&self, volume: &TriggerVolume, handle: ObjectHandle) -> bool {
        self.position(handle)
            .is_some_and(|p| volume.contains(p.to_array()))
    }

    /// Where an object looks from, and which way: a player's unit along its
    /// look, any other object along its facing.
    fn eye(&self, handle: ObjectHandle) -> Option<(Vec3, Vec3)> {
        let transform = self.sim.transform(handle)?;
        let direction = match self.sim.players().iter().find(|p| p.handle() == handle) {
            Some(player) => {
                let (yaw, pitch) = player.look();
                let (sy, cy) = (libm::sinf(yaw.0), libm::cosf(yaw.0));
                let (sp, cp) = (libm::sinf(pitch.0), libm::cosf(pitch.0));
                Vec3::new(cy * cp, sy * cp, sp)
            }
            None => transform.rotation * Vec3::X,
        };
        Some((transform.position, direction))
    }

    /// Whether any of `objects` sees `point` within `degrees` of where it
    /// looks.
    fn can_see(&self, objects: &[ObjectHandle], point: Vec3, degrees: f32) -> bool {
        let cosine = libm::cosf(degrees.to_radians());
        objects.iter().any(|&h| {
            self.eye(h).is_some_and(|(eye, direction)| {
                let toward = (point - eye).normalize_or_zero();
                toward != Vec3::ZERO && direction.dot(toward) >= cosine
            })
        })
    }
}

fn object(mission: &Mission, arguments: &[Value], at: usize) -> Option<ObjectHandle> {
    mission.handle(index(arguments, at))
}

fn list_count(mission: &mut Mission, arguments: &[Value]) -> Value {
    let count = mission.list(index(arguments, 0)).len();
    Value::Short(i16::try_from(count).unwrap_or(i16::MAX))
}

fn list_get(mission: &mut Mission, arguments: &[Value]) -> Value {
    let list = mission.list(index(arguments, 0));
    match usize::try_from(short(arguments, 1))
        .ok()
        .and_then(|at| list.get(at))
    {
        Some(&handle) => Value::Index(mission.index_of(handle)),
        None => Value::Index(-1),
    }
}

fn players(mission: &mut Mission, _: &[Value]) -> Value {
    let units = mission.player_units();
    Value::Index(mission.list_of(units))
}

fn name_of(arguments: &[Value]) -> Option<usize> {
    usize::try_from(index(arguments, 0)).ok()
}

fn object_create(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(name) = name_of(arguments) {
        mission.create_named(name, false);
    }
    Value::Void
}

fn object_create_anew(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(name) = name_of(arguments) {
        mission.create_named(name, true);
    }
    Value::Void
}

fn object_create_containing(mission: &mut Mission, arguments: &[Value]) -> Value {
    for name in mission.names_containing(text(arguments, 0)) {
        mission.create_named(name, false);
    }
    Value::Void
}

fn object_create_anew_containing(mission: &mut Mission, arguments: &[Value]) -> Value {
    for name in mission.names_containing(text(arguments, 0)) {
        mission.create_named(name, true);
    }
    Value::Void
}

fn object_destroy(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(handle) = object(mission, arguments, 0) {
        if mission.player_of(handle).is_none() {
            mission.sim.remove(handle);
        }
    }
    Value::Void
}

fn object_destroy_all(mission: &mut Mission, _: &[Value]) -> Value {
    let players = mission.player_units();
    for handle in mission.sim.objects() {
        if !players.contains(&handle) {
            mission.sim.remove(handle);
        }
    }
    Value::Void
}

fn object_destroy_containing(mission: &mut Mission, arguments: &[Value]) -> Value {
    for name in mission.names_containing(text(arguments, 0)) {
        if let Some(handle) = mission.named(name) {
            mission.sim.remove(handle);
        }
    }
    Value::Void
}

fn object_teleport(mission: &mut Mission, arguments: &[Value]) -> Value {
    let (Some(handle), Some(flag)) = (
        object(mission, arguments, 0),
        mission.flag(index(arguments, 1)).cloned(),
    ) else {
        return Value::Void;
    };
    let scale = mission.sim.transform(handle).map_or(1.0, |t| t.scale);
    let transform = Transform {
        position: Vec3::from_array(flag.position),
        rotation: rotation_from_angles([flag.facing[0], flag.facing[1], 0.0]),
        scale,
    };
    mission.place(handle, transform);
    Value::Void
}

fn object_set_facing(mission: &mut Mission, arguments: &[Value]) -> Value {
    let (Some(handle), Some(flag)) = (
        object(mission, arguments, 0),
        mission.flag(index(arguments, 1)).cloned(),
    ) else {
        return Value::Void;
    };
    if let Some(mut transform) = mission.sim.transform(handle) {
        transform.rotation = rotation_from_angles([flag.facing[0], flag.facing[1], 0.0]);
        mission.sim.insert(handle, transform);
    }
    Value::Void
}

fn object_set_scale(mission: &mut Mission, arguments: &[Value]) -> Value {
    // (the scale is taken at once, not over the frames asked)
    if let Some(handle) = object(mission, arguments, 0) {
        if let Some(mut transform) = mission.sim.transform(handle) {
            transform.scale = real(arguments, 1);
            mission.sim.insert(handle, transform);
        }
    }
    Value::Void
}

fn objects_attach(mission: &mut Mission, arguments: &[Value]) -> Value {
    // (to the parent's origin: its markers are not the simulation's yet)
    if let (Some(parent), Some(child)) =
        (object(mission, arguments, 0), object(mission, arguments, 2))
    {
        mission.attachments.retain(|&(_, c)| c != child);
        mission.attachments.push((parent, child));
    }
    Value::Void
}

fn objects_detach(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let (Some(parent), Some(child)) =
        (object(mission, arguments, 0), object(mission, arguments, 1))
    {
        mission.attachments.retain(|&pair| pair != (parent, child));
    }
    Value::Void
}

fn objects_can_see_flag(mission: &mut Mission, arguments: &[Value]) -> Value {
    let objects = mission.list(index(arguments, 0));
    let Some(flag) = mission.flag(index(arguments, 1)) else {
        return Value::Boolean(false);
    };
    let point = Vec3::from_array(flag.position);
    Value::Boolean(mission.can_see(&objects, point, real(arguments, 2)))
}

fn objects_can_see_object(mission: &mut Mission, arguments: &[Value]) -> Value {
    let objects = mission.list(index(arguments, 0));
    let Some(point) = object(mission, arguments, 1).and_then(|h| mission.position(h)) else {
        return Value::Boolean(false);
    };
    Value::Boolean(mission.can_see(&objects, point, real(arguments, 2)))
}

fn object_can_take_damage(mission: &mut Mission, arguments: &[Value]) -> Value {
    for handle in mission.list(index(arguments, 0)) {
        mission.invulnerable.remove(&handle);
    }
    Value::Void
}

fn object_cannot_take_damage(mission: &mut Mission, arguments: &[Value]) -> Value {
    for handle in mission.list(index(arguments, 0)) {
        mission.invulnerable.insert(handle);
    }
    Value::Void
}

fn volume_test_object(mission: &mut Mission, arguments: &[Value]) -> Value {
    let inside = match (
        mission.volume(index(arguments, 0)),
        object(mission, arguments, 1),
    ) {
        (Some(volume), Some(handle)) => mission.inside(volume, handle),
        _ => false,
    };
    Value::Boolean(inside)
}

/// Whether any of a list's objects is inside a volume, or, with `all`,
/// whether none is outside it (so true of no objects, as Halo's is).
fn test_objects(mission: &Mission, arguments: &[Value], all: bool) -> bool {
    let Some(volume) = mission.volume(index(arguments, 0)) else {
        return false;
    };
    let objects = mission.list(index(arguments, 1));
    if all {
        objects.iter().all(|&h| mission.inside(volume, h))
    } else {
        objects.iter().any(|&h| mission.inside(volume, h))
    }
}

fn volume_test_objects(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Boolean(test_objects(mission, arguments, false))
}

fn volume_test_objects_all(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Boolean(test_objects(mission, arguments, true))
}

fn volume_teleport_players_not_inside(mission: &mut Mission, arguments: &[Value]) -> Value {
    let (Some(volume), Some(flag)) = (
        mission.volume(index(arguments, 0)).cloned(),
        mission.flag(index(arguments, 1)).cloned(),
    ) else {
        return Value::Void;
    };
    for (player, unit) in mission.player_units().into_iter().enumerate() {
        if !mission.inside(&volume, unit) {
            mission
                .sim
                .teleport_player(player, Vec3::from_array(flag.position));
        }
    }
    Value::Void
}
