//! A vehicle's own animations and its riders' (source/units/vehicles.c,
//! units.c): the steering, roll, throttle, velocity and wheel overlays and
//! the suspensions over its pose (`vehicle_preprocess_node_orientations`),
//! the suspensions' travel (`update_suspension`), and a rider's
//! acceleration overlays (`unit_seat_update`).

use glam::Vec3;
use opence_content::{Animation, AnimationGraph, Physics, VehicleAnimations};

use crate::animation::{apply_aiming_screen, apply_overlay_continuous, NodeMatrix, Orientation};
use crate::animation_graph::fits;

/// The suspensions a vehicle keeps (`vehicle->vehicle.suspension`).
pub const SUSPENSIONS: usize = 8;

/// The overlays of a vehicle's animations, by their index in
/// [`VehicleAnimations::animations`].
const STEERING: usize = 0;
const ROLL: usize = 1;
const THROTTLE: usize = 2;
const VELOCITY: usize = 3;
const WHEELS: usize = 5;

/// What a vehicle's overlays are posed by: its movement, as the last game
/// tick left it, and the speeds and wheel of its definition.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct VehicleMotion {
    /// Its wheels' turn (`vehicle.turn`).
    pub turn: f32,
    /// Its speed (`vehicle.speed`).
    pub speed: f32,
    /// How far its wheels have rolled (`vehicle.wheel`).
    pub wheel: f32,
    /// Its velocity.
    pub velocity: Vec3,
    /// Its forward axis.
    pub forward: Vec3,
    /// Its up axis.
    pub up: Vec3,
    /// Each suspension's travel, 0 (extended) to 255 (compressed).
    pub suspension: [u8; SUSPENSIONS],
    /// Its definition's top speed forward.
    pub maximum_forward_speed: f32,
    /// Its definition's top speed in reverse.
    pub maximum_reverse_speed: f32,
    /// Its definition's wheels' circumference.
    pub wheel_circumference: f32,
}

/// Animation `index` of `graph`, if the graph has it and it fits a model of
/// `model_nodes` nodes (`unit_animation_get_fitting`).
fn fitting(graph: &AnimationGraph, index: Option<u16>, model_nodes: usize) -> Option<&Animation> {
    graph
        .animations
        .get(usize::from(index?))
        .filter(|animation| fits(animation, None, model_nodes))
}

/// The last frame of `animation` as a real.
fn last_frame(animation: &Animation) -> f32 {
    f32::from(animation.frame_count as i16) - 1.0
}

/// A suspension's travel as a fraction: 255 is all of it.
pub fn suspension_fraction(travel: u8) -> f32 {
    if travel == 0xff {
        1.0
    } else {
        f32::from(travel) * (1.0 / 255.0)
    }
}

/// The game's `triple_product3d`: `(a × b) · n`.
fn triple_product(a: Vec3, b: Vec3, n: Vec3) -> f32 {
    a.cross(b).dot(n)
}

/// A vehicle's overlays over `nodes`, in the game's order: steering (an
/// aiming screen by its wheels' turn), then each of
/// [`vehicle_overlay_frames`].
pub fn apply_vehicle_overlays(
    graph: &AnimationGraph,
    model_nodes: usize,
    motion: &VehicleMotion,
    nodes: &mut [Orientation],
) {
    let Some(animations) = graph.vehicle.as_ref() else {
        return;
    };
    let steering = animations
        .animations
        .get(STEERING)
        .and_then(|&index| fitting(graph, index, model_nodes));
    if let Some(steering) = steering {
        apply_aiming_screen(
            steering,
            &animations.steering_bounds,
            motion.turn,
            0.0,
            nodes,
        );
    }
    for (overlay, frame) in vehicle_overlay_frames(graph, model_nodes, motion) {
        apply_overlay_continuous(overlay, frame, nodes);
    }
}

/// A vehicle's continuous overlays and the frame each is at, in the game's
/// order: roll (its sideways velocity), throttle (its speed, forward above
/// the middle frame), velocity (its speed forward), wheels (how far they
/// have rolled) and each suspension (its travel).
pub fn vehicle_overlay_frames<'a>(
    graph: &'a AnimationGraph,
    model_nodes: usize,
    motion: &VehicleMotion,
) -> Vec<(&'a Animation, f32)> {
    let mut frames = Vec::new();
    let Some(animations) = graph.vehicle.as_ref() else {
        return frames;
    };
    let overlay = |slot: usize| {
        animations
            .animations
            .get(slot)
            .and_then(|&index| fitting(graph, index, model_nodes))
    };
    if let Some(roll) = overlay(ROLL) {
        let value = (triple_product(motion.up, motion.forward, motion.velocity)
            / motion.maximum_forward_speed
            + 1.0)
            * 0.5;
        frames.push((roll, value.clamp(0.0, 1.0) * last_frame(roll)));
    }
    if let Some(throttle) = overlay(THROTTLE) {
        // (not pinned: the game reads past the last frame as the last)
        let value = if motion.speed < 0.0 {
            0.5 - motion.speed / motion.maximum_reverse_speed * 0.5
        } else {
            (motion.speed / motion.maximum_forward_speed + 1.0) * 0.5
        };
        frames.push((throttle, value * last_frame(throttle)));
    }
    if let Some(velocity) = overlay(VELOCITY) {
        let value = motion.velocity.dot(motion.forward).clamp(0.0, 1.0)
            / motion.maximum_forward_speed.abs();
        frames.push((velocity, value.clamp(0.0, 1.0) * last_frame(velocity)));
    }
    if let Some(wheels) = overlay(WHEELS) {
        let value = if motion.wheel_circumference > 0.0 {
            motion.wheel / motion.wheel_circumference
        } else {
            0.0
        };
        frames.push((wheels, value * f32::from(wheels.frame_count as i16)));
    }
    for (suspension, &travel) in animations.suspensions.iter().zip(&motion.suspension) {
        if let Some(overlay) = fitting(graph, suspension.animation, model_nodes) {
            frames.push((overlay, suspension_fraction(travel) * last_frame(overlay)));
        }
    }
    frames
}

/// The game's `quantize_real_to_byte_lower_bound(0, 1, value)`.
fn quantize(value: f32) -> u8 {
    (value.clamp(0.0, 1.0) * 255.0) as u8
}

/// Moves each suspension halfway toward where the ground below its mass
/// point holds it (`update_suspension`): `cast(start, vector)` is how far
/// along the vector the ground is, 1 for none. The vehicle's origin is at
/// `origin`, facing `forward` with `up` up. Returns how far the most
/// compressed suspension moved, whose sound plays above 0.3.
pub fn update_suspension(
    animations: &VehicleAnimations,
    physics: &Physics,
    origin: Vec3,
    forward: Vec3,
    up: Vec3,
    suspension: &mut [u8; SUSPENSIONS],
    mut cast: impl FnMut(Vec3, Vec3) -> f32,
) -> f32 {
    let matrix =
        NodeMatrix::from_point_and_vectors(origin.to_array(), forward.to_array(), up.to_array());
    let mut maximum_shift = 0.0_f32;
    for (definition, travel) in animations.suspensions.iter().zip(suspension.iter_mut()) {
        let Some(mass_point) = usize::try_from(definition.mass_point)
            .ok()
            .and_then(|index| physics.mass_points.get(index))
        else {
            continue;
        };
        if definition.animation.is_none() {
            continue;
        }
        let current = suspension_fraction(*travel);
        let point = Vec3::from_array(matrix.transform_point(mass_point.position));
        let normal = Vec3::from_array(matrix.transform_vector(mass_point.up));
        let extent = definition.full_extension - definition.full_compression;
        let offset = (definition.full_compression - physics.center_of_mass[2]) - extent;
        let start = normal * offset + point;
        let vector = normal * (extent + extent);
        let t = cast(start, vector);
        let shift = ((1.0 - t) + (1.0 - t)).clamp(0.0, 1.0);
        if shift - current > maximum_shift {
            maximum_shift = shift - current;
        }
        *travel = quantize((shift + current) * 0.5);
    }
    maximum_shift
}

/// The gain of the suspension's sound for `maximum_shift`, if it plays.
pub fn suspension_sound_gain(maximum_shift: f32) -> Option<f32> {
    (maximum_shift > 0.3).then(|| ((maximum_shift - 0.3) * (1.0 / (0.9 - 0.3))).clamp(0.0, 1.0))
}

/// The seat animations of a unit's graph a rider shows its vehicle's
/// acceleration by (`_unit_seat_animation_acceleration_*`): front-back,
/// left-right and up-down.
pub const SEAT_ACCELERATION: [usize; 3] = [2, 3, 4];
/// A seat's entering animation (`_unit_seat_animation_enter`).
pub const SEAT_ENTER: usize = 7;
/// A seat's exiting animation (`_unit_seat_animation_exit`).
pub const SEAT_EXIT: usize = 8;

/// A rider's felt acceleration, smoothed over the ticks
/// (`unit->unit.seat_acceleration`).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SeatAcceleration {
    /// Where the seat was at the last tick.
    pub last_position: Vec3,
    /// How far it moved over the last tick.
    pub last_velocity: Vec3,
    /// The acceleration each overlay shows, 0 to 1 with 0.5 for none:
    /// forward, left and up.
    pub acceleration: Vec3,
}

impl SeatAcceleration {
    /// At rest at `position`.
    pub fn at(position: Vec3) -> Self {
        Self {
            last_position: position,
            last_velocity: Vec3::ZERO,
            acceleration: Vec3::splat(0.5),
        }
    }

    /// One tick of a seat at `position` facing `forward` with `up` up,
    /// scaled by the seat's `scale` (`unit_seat_update`): the change in its
    /// velocity along each axis, from the middle, blended in by 0.3.
    pub fn update(&mut self, position: Vec3, forward: Vec3, up: Vec3, scale: Vec3) {
        let velocity = position - self.last_position;
        let change = velocity - self.last_velocity;
        let left = up.cross(forward);
        let desired = Vec3::new(
            (change.dot(forward) * scale.x + 0.5).clamp(0.0, 1.0),
            (change.dot(left) * scale.y + 0.5).clamp(0.0, 1.0),
            (change.dot(up) * scale.z + 0.5).clamp(0.0, 1.0),
        );
        self.acceleration = desired * 0.3 + self.acceleration * 0.7;
        self.last_position = position;
        self.last_velocity = velocity;
    }

    /// The acceleration overlays of a seat's `animations` over a rider's
    /// `nodes`, each at its share of the acceleration along its axis.
    pub fn apply(
        &self,
        graph: &AnimationGraph,
        animations: &[Option<u16>],
        model_nodes: usize,
        nodes: &mut [Orientation],
    ) {
        for (slot, value) in SEAT_ACCELERATION.iter().zip(self.acceleration.to_array()) {
            let index = animations.get(*slot).copied().flatten();
            if let Some(overlay) = fitting(graph, index, model_nodes) {
                apply_overlay_continuous(overlay, last_frame(overlay) * value, nodes);
            }
        }
    }
}

/// Whether a seat's `animations` show its rider's acceleration
/// (`showing_acceleration`): any of the three.
pub fn shows_acceleration(animations: &[Option<u16>]) -> bool {
    SEAT_ACCELERATION
        .iter()
        .any(|&slot| animations.get(slot).copied().flatten().is_some())
}

/// The animation a rider enters (or exits) a seat by, if the seat's
/// `animations` have it: `slot` is [`SEAT_ENTER`] or [`SEAT_EXIT`].
pub fn seat_animation(
    graph: &AnimationGraph,
    animations: &[Option<u16>],
    slot: usize,
) -> Option<u16> {
    animations
        .get(slot)
        .copied()
        .flatten()
        .filter(|&index| usize::from(index) < graph.animations.len())
}

/// Where a unit that leaves its vehicle without an animation is put
/// (`unit_detach_from_parent`): 0.02 away from the vehicle's origin, along
/// its own forward if it is at the origin.
pub fn detached_position(unit: Vec3, vehicle: Vec3, forward: Vec3) -> Vec3 {
    let away = unit - vehicle;
    let direction = if away.length_squared() > 0.0 {
        away.normalize()
    } else {
        forward
    };
    unit + direction * 0.02
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::animation::tests::graph_of;
    use opence_content::{
        AnimationKind, FrictionType, Group, MassPoint, ScreenBounds, Suspension, TagRef,
    };

    /// An overlay of `frames` frames that moves no node.
    fn overlay(frames: u16) -> Animation {
        let mut animation = crate::animation::tests::animation_named("o", 1);
        animation.kind = AnimationKind::Overlay;
        animation.frame_count = frames;
        animation
    }

    fn vehicle_graph() -> AnimationGraph {
        let mut graph = graph_of((0..8).map(|_| overlay(11)).collect());
        graph.vehicle = Some(VehicleAnimations {
            steering_bounds: ScreenBounds::default(),
            animations: (0..6).map(Some).collect(),
            suspensions: vec![Suspension {
                mass_point: 0,
                animation: Some(6),
                full_extension: 0.2,
                full_compression: 0.0,
            }],
        });
        graph
    }

    fn physics() -> Physics {
        let point = MassPoint {
            name: "wheel".into(),
            powered_mass_point: None,
            model_node: None,
            flags: 0,
            relative_mass: 1.0,
            mass: 1.0,
            relative_density: 1.0,
            density: 1.0,
            position: [1.0, 0.0, 0.0],
            forward: [1.0, 0.0, 0.0],
            up: [0.0, 0.0, 1.0],
            friction_type: FrictionType::Point,
            friction_parallel_scale: 1.0,
            friction_perpendicular_scale: 1.0,
            radius: 0.1,
        };
        Physics {
            tag: TagRef {
                group: Group(*b"phys"),
                path: "test".into(),
            },
            radius: 1.0,
            moment_scale: 1.0,
            mass: 1.0,
            center_of_mass: [0.0; 3],
            density: 1.0,
            gravity_scale: 1.0,
            ground_friction: 0.0,
            ground_depth: 0.0,
            ground_damp_fraction: 0.0,
            ground_normal_k1: 0.0,
            ground_normal_k0: 0.0,
            water_friction: 0.0,
            water_depth: 0.0,
            water_density: 0.0,
            air_friction: 0.0,
            xx_moment: 1.0,
            yy_moment: 1.0,
            zz_moment: 1.0,
            inertial_matrix: None,
            powered_mass_points: Vec::new(),
            mass_points: vec![point],
        }
    }

    #[test]
    fn a_suspension_moves_halfway_to_the_ground() -> Result<(), &'static str> {
        let graph = vehicle_graph();
        let animations = graph.vehicle.clone().unwrap_or_default();
        let mut suspension = [0; SUSPENSIONS];
        let mut casts = Vec::new();
        let shift = update_suspension(
            &animations,
            &physics(),
            Vec3::new(10.0, 0.0, 0.0),
            Vec3::X,
            Vec3::Z,
            &mut suspension,
            |start, vector| {
                casts.push((start, vector));
                0.75
            },
        );
        // (the ground a quarter of the way short of full extension: half
        // compressed, of which the suspension moves half)
        assert!((shift - 0.5).abs() < 1e-6);
        assert_eq!(suspension[0], quantize(0.25));
        let (start, vector) = casts[0];
        assert!((start - Vec3::new(11.0, 0.0, -0.2)).length() < 1e-6);
        assert!((vector - Vec3::new(0.0, 0.0, 0.4)).length() < 1e-6);
        let gain = suspension_sound_gain(shift).ok_or("no sound")?;
        assert!((gain - 1.0 / 3.0).abs() < 1e-6);
        assert_eq!(suspension_sound_gain(0.2), None);
        Ok(())
    }

    #[test]
    fn a_suspension_without_a_mass_point_or_animation_stays() {
        let mut animations = vehicle_graph().vehicle.unwrap_or_default();
        animations.suspensions.push(Suspension {
            mass_point: 5,
            animation: Some(6),
            full_extension: 0.0,
            full_compression: 0.0,
        });
        animations.suspensions[0].animation = None;
        let mut suspension = [9; SUSPENSIONS];
        let shift = update_suspension(
            &animations,
            &physics(),
            Vec3::ZERO,
            Vec3::X,
            Vec3::Z,
            &mut suspension,
            |_, _| 0.0,
        );
        assert_eq!(shift, 0.0);
        assert_eq!(suspension, [9; SUSPENSIONS]);
    }

    #[test]
    fn the_fractions_are_the_games() {
        assert_eq!(suspension_fraction(0xff), 1.0);
        assert_eq!(suspension_fraction(51), 51.0 * (1.0 / 255.0));
        assert_eq!(quantize(1.5), 255);
        assert_eq!(quantize(-1.0), 0);
        assert_eq!(triple_product(Vec3::Z, Vec3::X, Vec3::Y), 1.0);
    }

    #[test]
    fn overlays_of_no_nodes_leave_the_pose() {
        let graph = vehicle_graph();
        let motion = VehicleMotion {
            turn: 0.2,
            speed: -0.5,
            wheel: 1.0,
            velocity: Vec3::new(0.5, 0.1, 0.0),
            forward: Vec3::X,
            up: Vec3::Z,
            suspension: [0xff; SUSPENSIONS],
            maximum_forward_speed: 1.0,
            maximum_reverse_speed: 1.0,
            wheel_circumference: 2.0,
        };
        let mut nodes = vec![Orientation::REST; 1];
        apply_vehicle_overlays(&graph, 1, &motion, &mut nodes);
        assert_eq!(nodes, vec![Orientation::REST; 1]);
        // (roll: 0.1 to the left of 1 is 0.55 of the way; throttle: half
        // of reverse is 0.75; velocity: 0.5 forward; wheels: half round;
        // the suspension: compressed)
        let frames: Vec<f32> = vehicle_overlay_frames(&graph, 1, &motion)
            .into_iter()
            .map(|(_, frame)| frame)
            .collect();
        let expected = [5.5, 7.5, 5.0, 5.5, 10.0];
        assert_eq!(frames.len(), expected.len());
        for (frame, expected) in frames.iter().zip(expected) {
            assert!((frame - expected).abs() < 1e-5, "{frames:?}");
        }
        let forward = VehicleMotion {
            speed: 0.5,
            velocity: Vec3::new(2.0, 0.0, 0.0),
            wheel_circumference: 0.0,
            ..motion
        };
        let frames: Vec<f32> = vehicle_overlay_frames(&graph, 1, &forward)
            .into_iter()
            .map(|(_, frame)| frame)
            .collect();
        assert_eq!(frames, vec![5.0, 7.5, 10.0, 0.0, 10.0]);
        // (too few nodes in the model: none fit)
        assert!(vehicle_overlay_frames(&graph, 0, &motion).is_empty());
        // (no vehicle block: nothing)
        let plain = graph_of(Vec::new());
        apply_vehicle_overlays(&plain, 1, &motion, &mut nodes);
        assert_eq!(nodes, vec![Orientation::REST; 1]);
    }

    #[test]
    fn a_seat_feels_the_change_in_its_velocity() {
        let mut seat = SeatAcceleration::at(Vec3::ZERO);
        seat.update(
            Vec3::new(1.0, 0.0, 0.0),
            Vec3::X,
            Vec3::Z,
            Vec3::splat(0.25),
        );
        // (from rest a unit forward: 0.25 above the middle, blended by 0.3)
        assert!((seat.acceleration.x - (0.75 * 0.3 + 0.5 * 0.7)).abs() < 1e-6);
        assert!((seat.acceleration.y - 0.5).abs() < 1e-6);
        seat.update(
            Vec3::new(2.0, 0.0, 0.0),
            Vec3::X,
            Vec3::Z,
            Vec3::splat(0.25),
        );
        // (at a steady velocity it settles back toward the middle)
        assert!(seat.acceleration.x < 0.75 * 0.3 + 0.5 * 0.7);
        let graph = graph_of((0..5).map(|_| overlay(3)).collect());
        let animations = [None, None, Some(2), None, Some(4)];
        assert!(shows_acceleration(&animations));
        assert!(!shows_acceleration(&[Some(0)]));
        let mut nodes = vec![Orientation::REST; 1];
        seat.apply(&graph, &animations, 1, &mut nodes);
        assert_eq!(nodes, vec![Orientation::REST; 1]);
    }

    #[test]
    fn seats_enter_and_exit_by_their_animations() {
        let graph = graph_of((0..9).map(|_| overlay(1)).collect());
        let mut animations = vec![None; 9];
        animations[SEAT_ENTER] = Some(7);
        animations[SEAT_EXIT] = Some(20);
        assert_eq!(seat_animation(&graph, &animations, SEAT_ENTER), Some(7));
        assert_eq!(seat_animation(&graph, &animations, SEAT_EXIT), None);
        let pushed = detached_position(Vec3::new(1.0, 0.0, 0.0), Vec3::ZERO, Vec3::Y);
        assert!((pushed - Vec3::new(1.02, 0.0, 0.0)).length() < 1e-6);
        let pushed = detached_position(Vec3::ZERO, Vec3::ZERO, Vec3::Y);
        assert!((pushed - Vec3::new(0.0, 0.02, 0.0)).length() < 1e-6);
    }
}
