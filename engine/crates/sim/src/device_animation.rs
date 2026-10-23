//! A device's motion and pose (source/devices/devices.c, device_machines.c,
//! source/math/real_math.c): its power and position accelerating toward
//! their groups' values (`device_update`, `accelerate_to_position`), a
//! gear's turning and a door's opening and closing (`machine_update`), what
//! it exports to its functions (`device_export_function_values`), and its
//! position and power overlays (`device_preprocess_node_orientations`).

use bevy_ecs::prelude::Component;
use opence_content::{AnimationGraph, Device, Machine, TagRef};

use crate::animation::{apply_overlay, apply_overlay_continuous, Orientation};

/// The device animations of a graph, by their index in
/// [`AnimationGraph::device`].
const POSITION: usize = 0;
const POWER: usize = 1;

/// The ticks a door that opens by itself waits, open, past its own time
/// (`door_open_ticks = -3`).
const DOOR_OPENING_TICKS: i32 = -3;

/// Moves `position` toward `target` by a speed `velocity` that changes by
/// at most `acceleration` a tick and is at most `maximum_velocity`,
/// slowing to stop there; whether it is there (`accelerate_to_position`).
/// Between `minimum` and `maximum`, the way round if `periodic`. (The
/// game's prototype names the two limits the other way; its callers and
/// body use them as here.)
#[allow(clippy::too_many_arguments)]
pub fn accelerate_to_position(
    position: &mut f32,
    velocity: &mut f32,
    target: f32,
    acceleration: f32,
    maximum_velocity: f32,
    minimum: f32,
    maximum: f32,
    periodic: bool,
) -> bool {
    let mut delta = target - *position;
    if periodic {
        let half = (maximum - minimum) * 0.5;
        if delta > half {
            delta -= half + half;
        } else if delta < -half {
            delta += half + half;
        }
    }
    if (delta - *velocity).abs() <= acceleration.min(maximum_velocity) {
        *velocity = 0.0;
        *position = target.clamp(minimum, maximum);
        return true;
    }
    // (the speed it can still stop from over the distance left)
    let braking = (acceleration + acceleration) * delta.abs();
    let mut speed = if braking >= maximum_velocity * maximum_velocity {
        maximum_velocity
    } else {
        libm::sqrtf(braking)
    };
    if delta < 0.0 {
        speed = -speed;
    }
    let step = (speed - *velocity).clamp(-acceleration, acceleration);
    *velocity += step;
    let mut moved = step * 0.5 + *velocity + *position;
    if periodic {
        if moved < minimum {
            moved += maximum - minimum;
        } else if moved > maximum {
            moved -= maximum - minimum;
        }
    }
    *position = moved.clamp(minimum, maximum);
    false
}

/// A device's power and position as they move (`device_datum.device`).
#[derive(Clone, Debug, Default, PartialEq)]
pub struct DeviceMotion {
    /// Its power, 0 to 1.
    pub power: f32,
    /// How fast its power changes, a tick.
    pub power_velocity: f32,
    /// Its position, closed (0) to open (1).
    pub position: f32,
    /// How fast its position changes, a tick.
    pub position_velocity: f32,
    /// The ticks it has waited, closed, to open.
    pub delay_ticks: i16,
    /// The ticks a door has been open (from [`DOOR_OPENING_TICKS`] when it
    /// opened by itself).
    pub door_open_ticks: i32,
}

/// What a tick of [`DeviceMotion::update`] did.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct DeviceUpdate {
    /// Whether its power or position is still on its way.
    pub changed: bool,
    /// Whether its power or position moved, so that it is posed again.
    pub moved: bool,
    /// The effects it starts, in order.
    pub effects: Vec<TagRef>,
}

impl DeviceMotion {
    /// A device at rest at `power` and `position`.
    pub fn at(power: f32, position: f32) -> Self {
        Self {
            power,
            position,
            ..Self::default()
        }
    }

    /// A tick of a device (`device_update`): its power accelerates toward
    /// its power group's value, then its position toward its position
    /// group's at speeds between its unpowered and powered ones, after its
    /// delay when it opens from closed. Without a group, either stays.
    pub fn update(
        &mut self,
        device: &Device,
        power_target: Option<f32>,
        position_target: Option<f32>,
    ) -> DeviceUpdate {
        let mut update = DeviceUpdate::default();
        if let Some(target) = power_target {
            if target != self.power || self.power_velocity != 0.0 {
                let old = self.power;
                if !accelerate_to_position(
                    &mut self.power,
                    &mut self.power_velocity,
                    target,
                    device.maximum_power_acceleration,
                    device.maximum_power_velocity,
                    0.0,
                    1.0,
                    false,
                ) {
                    update.changed = true;
                }
                update.moved |= old != self.power;
            }
        }
        let Some(target) = position_target else {
            return update;
        };
        if target == self.position && self.position_velocity == 0.0 {
            self.delay_ticks = 0;
            return update;
        }
        let blend =
            |unpowered: f32, powered: f32| (1.0 - self.power) * unpowered + self.power * powered;
        let acceleration = blend(
            device.depowered_position_acceleration,
            device.powered_position_acceleration,
        );
        let maximum_velocity = blend(
            device.depowered_position_velocity,
            device.powered_position_velocity,
        );
        let forward = self.position_velocity > 0.0;
        if f32::from(self.delay_ticks) < device.delay_ticks
            && self.position == 0.0
            && target >= self.position
        {
            self.delay_ticks += 1;
            if self.delay_ticks == 1 {
                update.effects.extend(device.delay_effect.clone());
            }
            return update;
        }
        let (old_velocity, old_position) = (self.position_velocity, self.position);
        if self.position_velocity.abs() > maximum_velocity {
            self.position_velocity = if forward {
                maximum_velocity
            } else {
                -maximum_velocity
            };
        }
        let there = accelerate_to_position(
            &mut self.position,
            &mut self.position_velocity,
            target,
            acceleration,
            maximum_velocity,
            0.0,
            1.0,
            device.flags & Device::POSITION_LOOPS != 0,
        );
        if there {
            let stop = &device.stop_effects[usize::from(!forward)];
            update.effects.extend(stop.clone());
        } else {
            if self.position_velocity != 0.0 && old_velocity * self.position_velocity <= 0.0 {
                let up = self.position_velocity > old_velocity;
                let start = &device.start_effects[usize::from(!up)];
                update.effects.extend(start.clone());
            }
            update.changed = true;
        }
        update.moved |= old_position != self.position;
        update
    }

    /// A gear's turn on a tick (`machine_update`): its position goes round
    /// by its speed between unpowered and powered, and is its group's new
    /// value.
    pub fn turn_gear(&mut self, device: &Device) -> f32 {
        self.position += (1.0 - self.power) * device.depowered_position_velocity
            + self.power * device.powered_position_velocity;
        if self.position >= 1.0 {
            self.position -= 1.0;
        }
        self.position_velocity = 0.0;
        self.position
    }

    /// A door's tick (`machine_update`), `activated` if a biped that opens
    /// it is near on one of the ticks it looks: the value its position
    /// group is to move to, if it changes it (open as it is activated,
    /// closed once it has been open its time).
    pub fn update_door(&mut self, machine: &Machine, activated: bool) -> Option<f32> {
        let mut desired = None;
        if activated {
            desired = Some(1.0);
            self.door_open_ticks = DOOR_OPENING_TICKS;
        }
        if self.position == 1.0 {
            self.door_open_ticks += 1;
            if self.door_open_ticks > machine.door_open_ticks {
                desired = Some(0.0);
            }
        } else {
            self.door_open_ticks = 0;
        }
        desired
    }

    /// The values it exports into its functions, for each of its
    /// definition's modes (`device_export_function_values`): none where
    /// the mode is none.
    pub fn exports(&self, device: &Device, locked: bool) -> [Option<f32>; 4] {
        let mut values = [None; 4];
        for (value, &mode) in values.iter_mut().zip(&device.function_modes) {
            *value = match mode {
                1 => Some(self.power),
                2 => Some(if self.power_velocity != 0.0 {
                    self.power_velocity.abs() / device.maximum_power_velocity
                } else {
                    0.0
                }),
                3 => Some(self.position),
                4 => Some(if self.position_velocity != 0.0 {
                    self.position_velocity.abs() / device.powered_position_velocity
                } else {
                    0.0
                }),
                5 => Some(f32::from(u8::from(locked))),
                6 => Some(
                    if device.delay_ticks <= 0.0
                        || f32::from(self.delay_ticks) == device.delay_ticks
                    {
                        0.0
                    } else {
                        f32::from(self.delay_ticks) / device.delay_ticks
                    },
                ),
                _ => None,
            };
        }
        values
    }
}

/// Whether a device shows as locked (`_device_function_locked`): without
/// power; a machine in a group that does not open by itself, is one sided
/// or has made its one change; but never an open one or one that never
/// appears locked. `machine` is a grouped machine's (manual or one sided,
/// spent, never appears locked).
pub fn locked(power: f32, position: f32, machine: Option<(bool, bool, bool)>) -> bool {
    let mut locked = power == 0.0;
    if let Some((manual, spent, never_appears_locked)) = machine {
        locked |= manual || spent;
        if position == 1.0 || never_appears_locked {
            locked = false;
        }
    }
    locked
}

/// A device's pose: its position and power, whether its position is
/// reversed (a placement's flag), and its definition's flags.
#[derive(Component, Clone, Copy, Debug, Default, PartialEq)]
pub struct DevicePose {
    /// Its position.
    pub position: f32,
    /// Its power.
    pub power: f32,
    /// Whether its position animation plays from its end.
    pub reversed: bool,
    /// Its definition's flags ([`Device::POSITION_LOOPS`] and the other).
    pub flags: u32,
    /// What it exports into its functions.
    pub exports: [Option<f32>; 4],
}

/// The frames a device's two overlays are posed at: its position
/// animation's (and whether by whole frames), then its power animation's.
pub fn device_overlay_frames(
    graph: &AnimationGraph,
    pose: &DevicePose,
) -> [Option<(u16, f32, bool)>; 2] {
    let animation = |slot: usize| {
        let index = (*graph.device.get(slot)?)?;
        Some((index, graph.animations.get(usize::from(index))?))
    };
    let position = animation(POSITION).map(|(index, animation)| {
        let position = if pose.reversed {
            1.0 - pose.position
        } else {
            pose.position
        };
        let frames = f32::from(animation.frame_count as i16);
        let last = if pose.flags & Device::POSITION_LOOPS != 0 {
            frames
        } else {
            frames - 1.0
        };
        (
            index,
            position * last,
            pose.flags & Device::POSITION_NOT_INTERPOLATED != 0,
        )
    });
    let power = animation(POWER).map(|(index, animation)| {
        (
            index,
            f32::from(animation.frame_count as i16) * pose.power,
            false,
        )
    });
    [position, power]
}

/// A device's overlays over `nodes`: its position animation, then its
/// power animation (`device_preprocess_node_orientations`).
pub fn apply_device_overlays(graph: &AnimationGraph, pose: &DevicePose, nodes: &mut [Orientation]) {
    for (index, frame, whole) in device_overlay_frames(graph, pose).into_iter().flatten() {
        let Some(animation) = graph.animations.get(usize::from(index)) else {
            continue;
        };
        if whole {
            apply_overlay(animation, frame as i16, nodes);
        } else {
            apply_overlay_continuous(animation, frame, nodes);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Animation, Group};

    fn effect(name: &str) -> Option<TagRef> {
        Some(TagRef {
            group: Group(*b"effe"),
            path: name.into(),
        })
    }

    /// A door that opens in ten ticks powered, half as fast without power,
    /// after a delay of two.
    fn door() -> Device {
        Device {
            function_modes: [1, 3, 5, 6],
            start_effects: [effect("start up"), effect("start down")],
            stop_effects: [effect("stop up"), effect("stop down")],
            delay_effect: effect("delay"),
            maximum_power_acceleration: 0.5,
            maximum_power_velocity: 0.5,
            depowered_position_acceleration: 0.01,
            depowered_position_velocity: 0.05,
            powered_position_acceleration: 0.02,
            powered_position_velocity: 0.1,
            delay_ticks: 2.0,
            machine: Some(Machine {
                machine_type: Machine::DOOR,
                door_open_ticks: 3,
                ..Machine::default()
            }),
            ..Device::default()
        }
    }

    #[test]
    fn a_position_near_enough_is_reached_at_once() {
        let (mut position, mut velocity) = (0.5, 0.0);
        assert!(accelerate_to_position(
            &mut position,
            &mut velocity,
            0.55,
            0.1,
            0.2,
            0.0,
            1.0,
            false
        ));
        assert_eq!((position, velocity), (0.55, 0.0));
    }

    #[test]
    fn a_position_far_away_speeds_up_then_slows_to_stop_there() {
        let (mut position, mut velocity) = (0.0f32, 0.0f32);
        let mut speeds = Vec::new();
        let mut ticks = 0;
        while !accelerate_to_position(
            &mut position,
            &mut velocity,
            1.0,
            0.02,
            0.1,
            0.0,
            1.0,
            false,
        ) {
            speeds.push(velocity);
            ticks += 1;
            assert!(ticks < 100, "it never arrives");
        }
        assert_eq!(position, 1.0);
        assert!(speeds.iter().all(|&speed| speed <= 0.1 + 1e-6));
        let fastest = speeds.iter().copied().fold(0.0, f32::max);
        assert!((fastest - 0.1).abs() < 1e-6, "{speeds:?}");
        assert!(speeds.last().is_some_and(|&last| last < fastest));
    }

    #[test]
    fn a_looping_position_goes_the_short_way_round() {
        let (mut position, mut velocity) = (0.9, 0.0);
        accelerate_to_position(&mut position, &mut velocity, 0.1, 0.05, 0.1, 0.0, 1.0, true);
        assert!(velocity > 0.0);
        assert!(position > 0.9);
    }

    #[test]
    fn a_door_waits_its_delay_then_opens_with_its_effects() {
        let door = door();
        let mut motion = DeviceMotion::at(1.0, 0.0);
        let first = motion.update(&door, Some(1.0), Some(1.0));
        assert_eq!(
            first.effects,
            vec![effect("delay").unwrap_or_else(|| unreachable!())]
        );
        assert!(!first.moved);
        motion.update(&door, Some(1.0), Some(1.0));
        assert_eq!(motion.position, 0.0);
        let opening = motion.update(&door, Some(1.0), Some(1.0));
        assert!(opening.moved && opening.changed);
        assert_eq!(
            opening.effects,
            vec![effect("start up").unwrap_or_else(|| unreachable!())]
        );
        let mut ticks = 3;
        let mut last = opening;
        while last.changed {
            last = motion.update(&door, Some(1.0), Some(1.0));
            ticks += 1;
            assert!(ticks < 200, "it never opens");
        }
        assert_eq!(motion.position, 1.0);
        assert_eq!(
            last.effects,
            vec![effect("stop up").unwrap_or_else(|| unreachable!())]
        );
        // (at rest where its group is, its delay starts again)
        motion.update(&door, Some(1.0), Some(1.0));
        assert_eq!(motion.delay_ticks, 0);
    }

    #[test]
    fn a_door_without_power_opens_slower() {
        let door = door();
        let opening_time = |power: f32| {
            let mut motion = DeviceMotion::at(power, 0.0);
            let mut ticks = 0;
            while motion.position < 1.0 {
                motion.update(&door, Some(power), Some(1.0));
                ticks += 1;
                if ticks > 1000 {
                    break;
                }
            }
            ticks
        };
        assert!(opening_time(0.0) > opening_time(1.0));
    }

    #[test]
    fn power_accelerates_toward_its_group() {
        let door = door();
        let mut motion = DeviceMotion::at(0.0, 0.0);
        let update = motion.update(&door, Some(1.0), None);
        assert!(update.changed && update.moved);
        assert!(motion.power > 0.0 && motion.power < 1.0);
        while motion.update(&door, Some(1.0), None).changed {}
        assert_eq!(motion.power, 1.0);
    }

    #[test]
    fn a_gear_turns_round_by_its_power() {
        let gear = Device {
            depowered_position_velocity: 0.0,
            powered_position_velocity: 0.4,
            ..Device::default()
        };
        let mut motion = DeviceMotion::at(1.0, 0.0);
        let turns: Vec<f32> = (0..3).map(|_| motion.turn_gear(&gear)).collect();
        assert!((turns[2] - 0.2).abs() < 1e-6, "{turns:?}");
        motion.power = 0.0;
        assert_eq!(motion.turn_gear(&gear), turns[2]);
    }

    #[test]
    fn a_door_closes_after_its_time_open() {
        let machine = Machine {
            door_open_ticks: 3,
            ..Machine::default()
        };
        let mut motion = DeviceMotion::at(1.0, 1.0);
        let closing = |motion: &mut DeviceMotion| {
            (0..8)
                .map(|_| motion.update_door(&machine, false))
                .position(|desired| desired == Some(0.0))
        };
        // (open, it closes once it has been open longer than its time)
        assert_eq!(closing(&mut motion), Some(3));
        // (a biped near it as it is open keeps it open three ticks more)
        motion.door_open_ticks = 0;
        assert_eq!(motion.update_door(&machine, true), Some(1.0));
        assert_eq!(closing(&mut motion), Some(5));
        motion.position = 0.5;
        assert_eq!(motion.update_door(&machine, false), None);
        assert_eq!(motion.door_open_ticks, 0);
    }

    #[test]
    fn a_device_exports_its_state_by_its_modes() {
        let door = door();
        let mut motion = DeviceMotion::at(0.5, 0.25);
        motion.delay_ticks = 1;
        assert_eq!(
            motion.exports(&door, true),
            [Some(0.5), Some(0.25), Some(1.0), Some(0.5)]
        );
        let changes = Device {
            function_modes: [2, 4, 0, 6],
            ..door
        };
        motion.power_velocity = -0.25;
        motion.position_velocity = 0.05;
        motion.delay_ticks = 2;
        assert_eq!(
            motion.exports(&changes, false),
            [Some(0.5), Some(0.5), None, Some(0.0)]
        );
    }

    #[test]
    fn locks_are_the_games() {
        assert!(locked(0.0, 0.0, None));
        assert!(!locked(1.0, 0.0, None));
        assert!(locked(1.0, 0.0, Some((true, false, false))));
        assert!(locked(1.0, 0.5, Some((false, true, false))));
        assert!(!locked(0.0, 1.0, Some((true, true, false))));
        assert!(!locked(0.0, 0.0, Some((false, false, true))));
    }

    fn overlay(frame_count: u16) -> Animation {
        Animation {
            frame_count,
            ..crate::animation::tests::animation_named("overlay", 0)
        }
    }

    #[test]
    fn overlays_are_posed_at_the_games_frames() {
        let graph = AnimationGraph {
            animations: vec![overlay(11), overlay(5)],
            device: vec![Some(0), Some(1)],
            ..crate::animation::tests::graph_of(Vec::new())
        };
        let mut pose = DevicePose {
            position: 0.5,
            power: 0.5,
            ..DevicePose::default()
        };
        assert_eq!(
            device_overlay_frames(&graph, &pose),
            [Some((0, 5.0, false)), Some((1, 2.5, false))]
        );
        pose.reversed = true;
        pose.position = 0.25;
        pose.flags = Device::POSITION_LOOPS | Device::POSITION_NOT_INTERPOLATED;
        assert_eq!(
            device_overlay_frames(&graph, &pose)[0],
            Some((0, 8.25, true))
        );
        let none = AnimationGraph {
            device: vec![None],
            ..graph.clone()
        };
        assert_eq!(device_overlay_frames(&none, &pose), [None, None]);
        let mut nodes = vec![Orientation::REST];
        apply_device_overlays(&graph, &pose, &mut nodes);
    }
}
