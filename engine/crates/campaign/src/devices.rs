//! Devices and their groups: machines and controls whose power and
//! position the scripts set, each accelerating toward its groups' values
//! at its definition's speeds, doors opening for the bipeds near them and
//! gears turning (`hs_library_devices.c`, `devices.c`, `device_machines.c`).

use glam::Vec3;
use opence_content::{Device, Machine, ObjectDefinition, PlacementDevice, TagRef};
use opence_script::{Native, Value};
use opence_sim::device_animation::{locked, DeviceMotion, DevicePose};
use opence_sim::objects::ObjectHandle;

use crate::{boolean, index, real, Mission};

/// A device's speeds where its definition is not in the level: a whole
/// opening in about a second, its power at once.
fn fallback() -> Device {
    Device {
        maximum_power_acceleration: 1.0,
        maximum_power_velocity: 1.0,
        depowered_position_acceleration: 1.0 / 30.0,
        depowered_position_velocity: 1.0 / 30.0,
        powered_position_acceleration: 1.0 / 30.0,
        powered_position_velocity: 1.0 / 30.0,
        ..Device::default()
    }
}

/// The distance under which a door's activation radius is taken as none,
/// so that its bounding radius is used (`k_real_epsilon`).
const EPSILON: f32 = 1e-4;

/// A device's state.
#[derive(Clone, Debug, PartialEq)]
pub struct DeviceState {
    /// Its power and position and how they move.
    pub motion: DeviceMotion,
    /// The power it moves toward without a power group.
    pub power_desired: f32,
    /// The position it moves toward.
    pub desired: f32,
    /// The device group its power follows, or `-1`.
    pub power_group: i16,
    /// The device group its position follows, or `-1`.
    pub position_group: i16,
    /// Whether it changes only once more (then stays).
    pub once: bool,
    /// Whether it has changed since `once` was set.
    pub changed: bool,
    /// Whether its position animation plays from its end.
    pub reversed: bool,
    /// Whether a machine does not operate by itself.
    pub manual: bool,
    /// Whether a machine opens from its back only.
    pub one_sided: bool,
    /// Whether a machine never shows as locked.
    pub never_appears_locked: bool,
    /// Its definition's device part, if the level has it.
    pub definition: Option<Device>,
    /// Its definition's bounding radius.
    pub bounding_radius: f32,
    /// The effects it started on the last tick.
    pub effects: Vec<TagRef>,
}

impl DeviceState {
    /// A placed device as the mission begins, its groups at these values.
    pub fn of(device: &PlacementDevice, groups: &[f32]) -> Self {
        let group = |g: i16| usize::try_from(g).ok().and_then(|g| groups.get(g)).copied();
        let position =
            group(device.position_group).unwrap_or(if device.flags & 1 != 0 { 1.0 } else { 0.0 });
        let power =
            group(device.power_group).unwrap_or(if device.flags & 2 != 0 { 0.0 } else { 1.0 });
        Self {
            motion: DeviceMotion::at(power, position),
            power_desired: power,
            desired: position,
            power_group: device.power_group,
            position_group: device.position_group,
            once: device.flags & 4 != 0,
            changed: false,
            reversed: device.flags & 8 != 0,
            manual: device.kind_flags & 1 != 0,
            one_sided: device.kind_flags & 2 != 0,
            never_appears_locked: device.kind_flags & 4 != 0,
            definition: None,
            bounding_radius: 0.0,
            effects: Vec::new(),
        }
    }

    /// The device with its definition's device part and bounding radius.
    pub fn defined_by(mut self, definition: Option<&ObjectDefinition>) -> Self {
        if let Some(definition) = definition {
            self.definition.clone_from(&definition.device);
            self.bounding_radius = definition.bounding_radius;
        }
        self
    }

    /// Its power, from 0 to 1.
    pub fn power(&self) -> f32 {
        self.motion.power
    }

    /// Its position, from closed (0) to open (1).
    pub fn position(&self) -> f32 {
        self.motion.position
    }

    /// Its machine part, if it is a machine.
    fn machine(&self) -> Option<Machine> {
        self.definition.as_ref().and_then(|device| device.machine)
    }

    /// Whether it shows as locked.
    fn locked(&self) -> bool {
        let grouped = self.position_group >= 0 && self.machine().is_some();
        locked(
            self.power(),
            self.position(),
            grouped.then_some((
                self.manual || self.one_sided,
                self.once && self.changed,
                self.never_appears_locked,
            )),
        )
    }

    /// Sets the position it moves toward; whether it took it.
    fn set_desired(&mut self, desired: f32) -> bool {
        if self.once && self.changed {
            return false;
        }
        let desired = desired.clamp(0.0, 1.0);
        if desired != self.desired {
            self.changed = true;
        }
        self.desired = desired;
        true
    }
}

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("device_get_position", device_get_position),
    ("device_get_power", device_get_power),
    ("device_set_position", device_set_position),
    (
        "device_set_position_immediate",
        device_set_position_immediate,
    ),
    ("device_set_power", device_set_power),
    ("device_group_get", device_group_get),
    ("device_group_set", device_group_set),
    ("device_group_set_immediate", device_group_set_immediate),
    (
        "device_group_change_only_once_more_set",
        device_group_change_only_once_more_set,
    ),
    ("device_one_sided_set", device_one_sided_set),
    (
        "device_operates_automatically_set",
        device_operates_automatically_set,
    ),
    (
        "device_set_never_appears_locked",
        device_set_never_appears_locked,
    ),
];

/// Moves each device on a tick (`device_update`, `machine_update`): its
/// power and position accelerate toward their groups' values, a gear
/// turns, a door opens for the bipeds near it and closes in its time; then
/// each is posed.
pub(crate) fn advance(mission: &mut Mission) {
    let tick = mission.game_tick();
    let bipeds = bipeds(mission);
    let handles: Vec<ObjectHandle> = mission.devices.keys().copied().collect();
    let fallback = fallback();
    for handle in handles {
        let transform = mission.sim.transform(handle);
        let Some(device) = mission.devices.get_mut(&handle) else {
            continue;
        };
        let groups = &mission.groups;
        let group = |g: i16| usize::try_from(g).ok().and_then(|g| groups.get(g)).copied();
        if let Some(position) = group(device.position_group) {
            device.set_desired(position);
        }
        let power = group(device.power_group).unwrap_or(device.power_desired);
        let definition = device
            .definition
            .clone()
            .unwrap_or_else(|| fallback.clone());
        let machine = definition.machine;
        let gear = machine.is_some_and(|m| m.machine_type == Machine::GEAR);
        let position = (!gear).then_some(device.desired);
        let update = device.motion.update(&definition, Some(power), position);
        device.effects = update.effects;
        let mut desired = None;
        if gear {
            desired = Some(device.motion.turn_gear(&definition));
        } else if let Some(machine) = machine.filter(|m| m.machine_type == Machine::DOOR) {
            // (a door looks for bipeds every fourth tick)
            let looks = (tick + u64::from(handle.index)) & 3 == 0 && !device.manual;
            let activated = looks
                && transform.is_some_and(|transform| {
                    let radius = if definition.automatic_activation_radius < EPSILON {
                        device.bounding_radius
                    } else {
                        definition.automatic_activation_radius
                    };
                    let forward = transform.rotation * Vec3::X;
                    let closed_one_side = device.one_sided && device.position() == 0.0;
                    bipeds.iter().any(|&(at, enemy)| {
                        let way = at - transform.position;
                        way.length() <= radius
                            && !(closed_one_side && !enemy && way.dot(forward) > 0.0)
                    })
                });
            desired = device.motion.update_door(&machine, activated);
        }
        if let Some(value) = desired {
            let follows = usize::try_from(device.position_group).ok();
            if let Some(slot) = follows.and_then(|g| mission.groups.get_mut(g)) {
                *slot = value;
            } else if !gear {
                device.set_desired(value);
            }
        }
        let pose = DevicePose {
            position: device.position(),
            power: device.power(),
            reversed: device.reversed,
            flags: definition.flags,
            exports: device.motion.exports(&definition, device.locked()),
        };
        mission.sim.pose_device(handle, pose);
    }
}

/// The living bipeds that may open doors: where each stands, and whether
/// it is the players' enemy.
fn bipeds(mission: &Mission) -> Vec<(Vec3, bool)> {
    crate::actors::others(mission)
        .into_iter()
        .filter(|other| other.alive)
        .filter_map(|other| {
            let at = mission.sim.transform(other.unit)?.position;
            Some((at, mission.enemies(crate::actors::PLAYER_TEAM, other.team)))
        })
        .collect()
}

fn device<'a>(mission: &'a mut Mission, arguments: &[Value]) -> Option<&'a mut DeviceState> {
    let handle: ObjectHandle = mission.handle(index(arguments, 0))?;
    mission.devices.get_mut(&handle)
}

fn group(arguments: &[Value]) -> Option<usize> {
    usize::try_from(index(arguments, 0)).ok()
}

fn device_get_position(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Real(device(mission, arguments).map_or(0.0, |d| d.position()))
}

fn device_get_power(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Real(device(mission, arguments).map_or(0.0, |d| d.power()))
}

/// Sets a device's position to move toward, and its group's; whether it
/// took it.
fn set_position(mission: &mut Mission, arguments: &[Value], immediate: bool) -> bool {
    let position = real(arguments, 1);
    let Some(device) = device(mission, arguments) else {
        return false;
    };
    if !device.set_desired(position) {
        return false;
    }
    if immediate {
        device.motion.position = device.desired;
        device.motion.position_velocity = 0.0;
    }
    let follows = usize::try_from(device.position_group).ok();
    if let Some(value) = follows.and_then(|g| mission.groups.get_mut(g)) {
        *value = position.clamp(0.0, 1.0);
    }
    true
}

fn device_set_position(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Boolean(set_position(mission, arguments, false))
}

fn device_set_position_immediate(mission: &mut Mission, arguments: &[Value]) -> Value {
    set_position(mission, arguments, true);
    Value::Void
}

fn device_set_power(mission: &mut Mission, arguments: &[Value]) -> Value {
    let power = real(arguments, 1).clamp(0.0, 1.0);
    if let Some(device) = device(mission, arguments) {
        device.power_desired = power;
        let follows = usize::try_from(device.power_group).ok();
        if let Some(value) = follows.and_then(|g| mission.groups.get_mut(g)) {
            *value = power;
        }
    }
    Value::Void
}

fn device_group_get(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Real(
        group(arguments)
            .and_then(|g| mission.group(g))
            .unwrap_or(0.0),
    )
}

/// Sets a group's value; its devices follow it on the next tick, or at
/// once.
fn set_group(mission: &mut Mission, arguments: &[Value], immediate: bool) -> bool {
    let Some(group) = group(arguments) else {
        return false;
    };
    let value = real(arguments, 1).clamp(0.0, 1.0);
    let Some(slot) = mission.groups.get_mut(group) else {
        return false;
    };
    *slot = value;
    let group = i16::try_from(group).unwrap_or(-1);
    for device in mission.devices.values_mut() {
        if device.position_group == group && device.set_desired(value) && immediate {
            device.motion.position = value;
            device.motion.position_velocity = 0.0;
        }
        if device.power_group == group && immediate {
            device.motion.power = value;
            device.motion.power_velocity = 0.0;
        }
    }
    true
}

fn device_group_set(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Boolean(set_group(mission, arguments, false))
}

fn device_group_set_immediate(mission: &mut Mission, arguments: &[Value]) -> Value {
    set_group(mission, arguments, true);
    Value::Void
}

fn device_group_change_only_once_more_set(mission: &mut Mission, arguments: &[Value]) -> Value {
    let once = boolean(arguments, 1);
    if let Some(group) = group(arguments).and_then(|g| i16::try_from(g).ok()) {
        for device in mission.devices.values_mut() {
            if device.position_group == group {
                device.once = once;
                device.changed = false;
            }
        }
    }
    Value::Void
}

fn device_one_sided_set(mission: &mut Mission, arguments: &[Value]) -> Value {
    let value = boolean(arguments, 1);
    if let Some(device) = device(mission, arguments) {
        device.one_sided = value;
    }
    Value::Void
}

fn device_operates_automatically_set(mission: &mut Mission, arguments: &[Value]) -> Value {
    let value = !boolean(arguments, 1);
    if let Some(device) = device(mission, arguments) {
        device.manual = value;
    }
    Value::Void
}

fn device_set_never_appears_locked(mission: &mut Mission, arguments: &[Value]) -> Value {
    let value = boolean(arguments, 1);
    if let Some(device) = device(mission, arguments) {
        device.never_appears_locked = value;
    }
    Value::Void
}
