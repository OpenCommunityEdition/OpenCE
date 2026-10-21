//! Devices and their groups: machines and controls whose power and
//! position the scripts set, each moving toward its position while it has
//! power (`hs_library_devices.c`, `devices.c`).

use opence_content::PlacementDevice;
use opence_script::{Native, Value};
use opence_sim::objects::ObjectHandle;

use crate::{boolean, index, real, Mission};

/// How far a device moves each tick toward its position: a whole opening
/// in a second (the machines' own transition times are not imported).
const STEP: f32 = 1.0 / 30.0;

/// A device's state.
#[derive(Clone, Debug, PartialEq)]
pub struct DeviceState {
    /// Its power, from 0 to 1.
    pub power: f32,
    /// Its position, from closed (0) to open (1).
    pub position: f32,
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
            power,
            position,
            desired: position,
            power_group: device.power_group,
            position_group: device.position_group,
            once: device.flags & 4 != 0,
            changed: false,
        }
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

/// Moves each device on a tick: its power and position follow their
/// groups, and a powered device moves toward its position.
pub(crate) fn advance(mission: &mut Mission) {
    let groups = &mission.groups;
    let group = |g: i16| usize::try_from(g).ok().and_then(|g| groups.get(g)).copied();
    for device in mission.devices.values_mut() {
        if let Some(power) = group(device.power_group) {
            device.power = power;
        }
        if let Some(position) = group(device.position_group) {
            device.set_desired(position);
        }
        if device.power > 0.0 {
            let distance = device.desired - device.position;
            device.position += distance.clamp(-STEP, STEP);
        }
    }
}

fn device<'a>(mission: &'a mut Mission, arguments: &[Value]) -> Option<&'a mut DeviceState> {
    let handle: ObjectHandle = mission.handle(index(arguments, 0))?;
    mission.devices.get_mut(&handle)
}

fn group(arguments: &[Value]) -> Option<usize> {
    usize::try_from(index(arguments, 0)).ok()
}

fn device_get_position(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Real(device(mission, arguments).map_or(0.0, |d| d.position))
}

fn device_get_power(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Real(device(mission, arguments).map_or(0.0, |d| d.power))
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
        device.position = device.desired;
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
        device.power = power;
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
            device.position = value;
        }
        if device.power_group == group {
            device.power = value;
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
    mission.present("device_one_sided_set", arguments);
    Value::Void
}

fn device_operates_automatically_set(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.present("device_operates_automatically_set", arguments);
    Value::Void
}

fn device_set_never_appears_locked(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.present("device_set_never_appears_locked", arguments);
    Value::Void
}
