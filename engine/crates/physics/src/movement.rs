//! A player's biped as it walks, runs, crouches, jumps and lands: Halo's
//! rules (source/units/bipeds.c, biped_update_physics and the unit's
//! update before it) computing the velocity each tick, and a [`World`]'s
//! character colliding it with the level.
//!
//! Halo runs these rules at 30 Hz in units a tick; the engine runs them at
//! 60 Hz in units a second. Each rate is converted so that the same input
//! gives the same motion (the traces in engine/tests/traces, recorded from
//! the reference game, check it): a velocity's change a tick is the tag's
//! acceleration over the engine's rate, gravity likewise, and every count of
//! ticks is doubled.

use glam::{Vec2, Vec3};
use opence_content::{Biped, PlayerPhysics};

use crate::{CharacterId, CharacterSettings, Ground, Layers, PhysicsError, World};

/// The engine's ticks a second.
pub const TICKS_PER_SECOND: f32 = 60.0;
/// Halo's ticks a second, which its counts of ticks are in.
const HALO_TICKS_PER_SECOND: f32 = 30.0;
/// Engine ticks in one of Halo's.
const TICKS_PER_HALO_TICK: u32 = 2;
/// The engine's tick, in seconds.
const TICK: f32 = 1.0 / TICKS_PER_SECOND;

/// How hard the ground is pressed each tick, into its normal (Halo's 1/128
/// of a unit a tick, as units a second): the collision takes it out again,
/// so it never builds up.
const GROUND_PULL: f32 = HALO_TICKS_PER_SECOND / 128.0;
/// A throttle shorter than this is none (Halo's 0.1).
const THROTTLE_DEAD: f32 = 0.1;
/// Halo's ticks on the ground before a jump: more than 5.
const JUMP_RECOVERY_HALO_TICKS: u32 = 5;
/// How high a character steps onto a ledge, and how far down it follows the
/// floor (a fraction of its standing height).
const STEP_UP: f32 = 0.1;
const STICK_DOWN: f32 = 0.1;

/// A player's controls for one tick.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct MovementInput {
    /// Forward (x) and left (y), each -1 to 1.
    pub throttle: Vec2,
    /// The facing, in radians about z from x.
    pub yaw: f32,
    /// The jump held.
    pub jump: bool,
    /// The crouch held.
    pub crouch: bool,
}

/// A landing that slows the biped as it recovers.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Landing {
    /// It may still move.
    Soft,
    /// It stands where it landed, unable to move or jump.
    Hard,
}

/// The biped's movement settings: its tag's and the globals'.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct MoverSettings {
    /// The biped's tag.
    pub biped: Biped,
    /// The globals' player physics.
    pub player: PlayerPhysics,
    /// Gravity's pull, in units a second squared (positive).
    pub gravity: f32,
}

/// A biped moving through a [`World`]: its state between ticks.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Mover {
    character: CharacterId,
    settings: MoverSettings,
    /// Its feet.
    pub position: Vec3,
    /// Its velocity, in units a second.
    pub velocity: Vec3,
    /// Its ground's normal (up, airborne).
    pub ground_normal: Vec3,
    /// Off the ground.
    pub airborne: bool,
    /// Ticks off the ground.
    pub airborne_ticks: u32,
    /// Sliding with no grip, accelerating past what it may.
    pub slipping: bool,
    /// How far crouched, 0 standing to 1.
    pub crouch: f32,
    /// The landing it is recovering from.
    pub landing: Option<Landing>,
    /// The speed of the last landing, into the ground (units a second).
    pub landing_velocity: f32,
    landing_ticks: u32,
    landing_recovery_ticks: u32,
    jump_recovery_ticks: u32,
    height: f32,
}

impl Mover {
    /// A biped standing at `position`, its pill added to the world.
    pub fn new(
        world: &mut World,
        settings: MoverSettings,
        position: Vec3,
    ) -> Result<Mover, PhysicsError> {
        let biped = &settings.biped;
        let height = biped.collision_height_standing;
        let character = world.add_character(
            &CharacterSettings {
                radius: biped.collision_radius,
                height,
                maximum_slope: libm::acosf(biped.minimum_normal_k.clamp(-1.0, 1.0)),
                step_up: STEP_UP * height,
                stick_down: STICK_DOWN * height,
                layers: Layers::GROUND,
            },
            position,
        )?;
        let state = world.character_state(character)?;
        Ok(Mover {
            character,
            settings,
            position: state.position,
            velocity: Vec3::ZERO,
            ground_normal: Vec3::Z,
            airborne: true,
            airborne_ticks: 0,
            slipping: false,
            crouch: 0.0,
            landing: None,
            landing_velocity: 0.0,
            landing_ticks: 0,
            landing_recovery_ticks: 0,
            jump_recovery_ticks: 0,
            height,
        })
    }

    /// Its pill in the world.
    pub fn character(&self) -> CharacterId {
        self.character
    }

    /// Its pill's radius.
    pub fn radius(&self) -> f32 {
        self.settings.biped.collision_radius
    }

    /// Its eye's height above its feet, as it crouches.
    pub fn camera_height(&self) -> f32 {
        let biped = &self.settings.biped;
        lerp(
            biped.standing_camera_height,
            biped.crouching_camera_height,
            self.crouch,
        )
    }

    /// Puts it at `position`, standing still. Like Halo's, it keeps its
    /// ground until the next step finds none: lifted off the ground, its
    /// first tick still pulls it down onto the ground it left.
    pub fn teleport(&mut self, world: &mut World, position: Vec3) -> Result<(), PhysicsError> {
        world.set_character_position(self.character, position)?;
        self.position = world.character_state(self.character)?.position;
        self.velocity = Vec3::ZERO;
        self.landing = None;
        Ok(())
    }

    /// One tick: the velocity the controls ask for, the collision, then the
    /// landing and the jump.
    pub fn step(&mut self, world: &mut World, input: &MovementInput) -> Result<(), PhysicsError> {
        let biped = self.settings.biped;
        let crouch_velocity = self.update_crouch(world, input.crouch)?;
        let (desired, acceleration, airborne_acceleration, penalty) = self.desired(input);
        let facing = Vec2::new(libm::cosf(input.yaw), libm::sinf(input.yaw));
        let rotated = Vec2::new(
            desired.x * facing.x - facing.y * desired.y,
            desired.y * facing.x + desired.x * facing.y,
        );
        let gravity = self.settings.gravity * TICK;

        let mut slipping = false;
        let mut velocity = self.velocity;
        if self.airborne {
            let wanted = rotated * (1.0 - penalty);
            let change = clamp_length(wanted - velocity.truncate(), airborne_acceleration * TICK);
            velocity.x += change.x;
            velocity.y += change.y;
            velocity.z -= gravity;
            slipping = self.slipping;
        } else {
            let normal = self.ground_normal;
            let direction = if normal.z > f32::EPSILON {
                Vec3::new(
                    rotated.x,
                    rotated.y,
                    -(rotated.y * normal.y + rotated.x * normal.x) / normal.z,
                )
            } else {
                // (a wall's normal: forward and left along it, and up five
                // times as steep, as Halo's)
                let forward = Vec3::new(facing.x, facing.y, 0.0);
                let left = Vec3::new(-facing.y, facing.x, 0.0);
                let forward = forward - normal * forward.dot(normal);
                let left = left - normal * left.dot(normal);
                let mut direction = forward * desired.x + left * desired.y;
                direction.z *= 5.0;
                direction
            }
            .normalize_or_zero();
            let speed = desired.length() * slope_scale(&biped, direction.z);
            let wanted = direction * speed * (1.0 - penalty);
            let maximum = acceleration * TICK;
            let mut change = wanted - velocity;
            if change.length() > maximum {
                slipping = self.slipping;
                change = change.normalize() * maximum;
            }
            change -= normal * GROUND_PULL;
            velocity += change;
            if slipping {
                velocity.z -= gravity;
            }
        }

        let moved = velocity + Vec3::new(0.0, 0.0, crouch_velocity);
        let state = world.move_character(self.character, moved, TICK)?;
        let supported = state.ground == Ground::Standing && !(self.airborne && velocity.z > 0.0);
        // (the character keeps the velocity it was given: what it moved is
        // the velocity the collisions left, Halo's clipped velocity, less
        // the platform's that carried it)
        self.velocity = (state.position - self.position) / TICK
            - state.ground_velocity
            - Vec3::new(0.0, 0.0, crouch_velocity);
        self.position = state.position;
        self.slipping = slipping;
        if supported {
            // (Jolt stands it on its floor but may give the normal of a bank
            // it leans on: it walks along its floor, and the bank stops it
            // as a wall)
            self.ground_normal = if state.ground_normal.z >= biped.minimum_normal_k {
                state.ground_normal
            } else {
                Vec3::Z
            };
            self.landing_velocity = -velocity.dot(self.ground_normal);
            if self.airborne {
                self.start_landing(self.landing_velocity);
            }
            self.airborne = false;
            self.airborne_ticks = 0;
        } else {
            self.ground_normal = Vec3::Z;
            self.landing_velocity = 0.0;
            self.airborne = true;
            self.airborne_ticks = self.airborne_ticks.saturating_add(1);
        }

        self.update_landing();
        self.update_jump(input.jump);
        Ok(())
    }

    /// The velocity the controls ask for (forward, left; units a second),
    /// the accelerations allowed on the ground and in the air (units a
    /// second squared), and the penalty taken off.
    fn desired(&self, input: &MovementInput) -> (Vec2, f32, f32, f32) {
        let player = &self.settings.player;
        if self.landing == Some(Landing::Hard) {
            return (Vec2::ZERO, 0.0, player.airborne_acceleration, 1.0);
        }
        let mut player = *player;
        if self.settings.biped.flags & Biped::USES_OLD_PLAYER_PHYSICS != 0 {
            player.walking_speed = 0.512;
            player.run_forward = 2.25;
            player.run_backward = 2.0;
            player.run_sideways = 2.0;
            player.run_acceleration = 0.32 * HALO_TICKS_PER_SECOND;
        }
        let throttle = if input.throttle.length() < THROTTLE_DEAD {
            Vec2::ZERO
        } else {
            input.throttle
        };
        let crouch = self.crouch;
        let forward = if throttle.x <= 0.0 {
            lerp(player.run_backward, player.sneak_backward, crouch)
        } else {
            lerp(player.run_forward, player.sneak_forward, crouch)
        };
        let sideways = lerp(player.run_sideways, player.sneak_sideways, crouch);
        let acceleration = lerp(player.run_acceleration, player.sneak_acceleration, crouch);
        (
            Vec2::new(throttle.x * forward, throttle.y * sideways),
            acceleration,
            player.airborne_acceleration,
            0.0,
        )
    }

    /// Crouches or stands a tick's way, the pill's height following (it stays
    /// crouched where there is no room to stand). The velocity, upward,
    /// that keeps its head where it was as it crouches in the air.
    fn update_crouch(&mut self, world: &mut World, crouching: bool) -> Result<f32, PhysicsError> {
        let biped = self.settings.biped;
        let rate = if biped.crouch_transition_time > 0.0 {
            TICK / biped.crouch_transition_time
        } else {
            1.0
        };
        let target = if crouching { 1.0 } else { 0.0 };
        let crouch = self.crouch + (target - self.crouch).clamp(-rate, rate);
        let height = lerp(
            biped.collision_height_standing,
            biped.collision_height_crouching,
            crouch,
        );
        if (height - self.height).abs() > 1.0e-5 {
            if !world.set_character_height(self.character, height)? {
                return Ok(0.0);
            }
            self.height = height;
        }
        let delta = crouch - self.crouch;
        self.crouch = crouch;
        // (Halo's 0.01 a 30 Hz tick)
        if self.airborne && delta.abs() > 0.01 / TICKS_PER_HALO_TICK as f32 {
            Ok((biped.collision_height_standing - biped.collision_height_crouching) * delta / TICK)
        } else {
            Ok(0.0)
        }
    }

    /// Starts recovering from a landing at `velocity` into the ground, as
    /// Halo's biped_start_landing.
    fn start_landing(&mut self, velocity: f32) {
        let biped = &self.settings.biped;
        let soft = biped.minimum_soft_landing_velocity;
        let hard = biped.minimum_hard_landing_velocity;
        if velocity < soft {
            return;
        }
        let (landing, fraction, range, time) = if velocity < hard {
            (
                Landing::Soft,
                velocity - soft,
                hard - soft,
                biped.maximum_soft_landing_time,
            )
        } else {
            // (Halo's: the whole velocity over the range above the hard
            // landing's)
            (
                Landing::Hard,
                velocity,
                biped.maximum_hard_landing_velocity - hard,
                biped.maximum_hard_landing_time,
            )
        };
        if range > 0.0 {
            let fraction = (fraction / range).clamp(0.0, 1.0);
            self.landing = Some(landing);
            self.landing_ticks = 0;
            // (Halo truncates its ticks)
            self.landing_recovery_ticks =
                (time * HALO_TICKS_PER_SECOND * fraction) as u32 * TICKS_PER_HALO_TICK;
        }
    }

    fn update_landing(&mut self) {
        if self.landing.is_some() {
            self.landing_ticks += 1;
            if self.landing_ticks >= self.landing_recovery_ticks {
                self.landing = None;
            }
        }
    }

    /// On the ground, recovered from the last jump and not landing hard: a
    /// jump held leaves the ground at the biped's jump velocity.
    fn update_jump(&mut self, jump: bool) {
        if self.airborne || self.landing == Some(Landing::Hard) {
            return;
        }
        self.jump_recovery_ticks = self.jump_recovery_ticks.saturating_add(1);
        if jump && self.jump_recovery_ticks > JUMP_RECOVERY_HALO_TICKS * TICKS_PER_HALO_TICK {
            let jump_velocity = self.settings.biped.jump_velocity;
            if self.velocity.z < jump_velocity {
                self.velocity.z = jump_velocity;
            }
            self.airborne = true;
            self.jump_recovery_ticks = 0;
        }
    }
}

/// Halo's speed on a slope: faster down, slower up, as steep as `k` (the
/// direction's upward part).
fn slope_scale(biped: &Biped, k: f32) -> f32 {
    if k <= biped.downhill_k1 {
        biped.downhill_velocity_scale
    } else if k < biped.downhill_k0 {
        (k - biped.downhill_k0) * (biped.downhill_velocity_scale - 1.0)
            / (biped.downhill_k1 - biped.downhill_k0)
            + 1.0
    } else if k >= biped.uphill_k1 {
        biped.uphill_velocity_scale
    } else if k > biped.uphill_k0 {
        (k - biped.uphill_k0) * (biped.uphill_velocity_scale - 1.0)
            / (biped.uphill_k1 - biped.uphill_k0)
            + 1.0
    } else {
        1.0
    }
}

fn clamp_length(vector: Vec2, maximum: f32) -> Vec2 {
    if vector.length() > maximum {
        vector.normalize() * maximum
    } else {
        vector
    }
}

fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + (b - a) * t
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::PlayerPhysics;

    fn cyborg() -> Biped {
        Biped {
            downhill_velocity_scale: 1.25,
            uphill_velocity_scale: 0.65,
            downhill_k0: -0.342,
            downhill_k1: -0.707,
            uphill_k0: 0.342,
            uphill_k1: 0.707,
            ..Biped::default()
        }
    }

    /// A biped standing in an empty world, with landings, a crouch and a
    /// jump to recover from.
    fn mover(biped: Biped, player: PlayerPhysics) -> Result<(World, Mover), PhysicsError> {
        let biped = Biped {
            collision_radius: 0.2,
            collision_height_standing: 0.7,
            collision_height_crouching: 0.5,
            standing_camera_height: 0.62,
            crouching_camera_height: 0.35,
            crouch_transition_time: 0.25,
            minimum_normal_k: 0.7,
            minimum_soft_landing_velocity: 3.0,
            minimum_hard_landing_velocity: 6.0,
            maximum_hard_landing_velocity: 30.0,
            maximum_soft_landing_time: 1.0,
            maximum_hard_landing_time: 2.0,
            jump_velocity: 2.5,
            ..biped
        };
        let mut world = World::new(crate::WorldSettings::default())?;
        let settings = MoverSettings {
            biped,
            player,
            gravity: 10.0,
        };
        let mover = Mover::new(&mut world, settings, Vec3::new(0.0, 0.0, 1.0))?;
        Ok((world, mover))
    }

    /// A floor 40 units square at z = 0 under the mover's world.
    fn floor(world: &mut World) -> Result<(), PhysicsError> {
        world.set_level(&crate::LevelMesh {
            vertices: vec![
                [-20.0, -20.0, 0.0],
                [20.0, -20.0, 0.0],
                [20.0, 20.0, 0.0],
                [-20.0, 20.0, 0.0],
            ],
            triangles: vec![[0, 1, 2], [0, 2, 3]],
            materials: vec![None, None],
        })
    }

    /// The mover on the floor, settled: grounded and still.
    fn grounded() -> Result<(World, Mover), PhysicsError> {
        let player = PlayerPhysics {
            run_forward: 3.0,
            run_sideways: 2.5,
            run_acceleration: 20.0,
            ..PlayerPhysics::default()
        };
        let (mut world, mut mover) = mover(cyborg(), player)?;
        floor(&mut world)?;
        mover.teleport(&mut world, Vec3::ZERO)?;
        for _ in 0..10 {
            mover.step(&mut world, &MovementInput::default())?;
        }
        assert!(!mover.airborne);
        assert_eq!(mover.landing, None);
        assert!(mover.velocity.length() < 1.0e-3, "{}", mover.velocity);
        Ok((world, mover))
    }

    /// Forward and half left, facing along y.
    fn walking() -> MovementInput {
        MovementInput {
            throttle: Vec2::new(1.0, 0.5),
            yaw: core::f32::consts::FRAC_PI_2,
            ..MovementInput::default()
        }
    }

    fn near_by(a: Vec2, b: Vec2, by: f32) -> bool {
        (a - b).length() < by
    }

    #[test]
    fn on_the_ground_it_gathers_speed_by_its_acceleration_to_its_run() -> Result<(), PhysicsError> {
        let (mut world, mut mover) = grounded()?;
        // (forward is +y, left is -x: a tick's acceleration, 20 / 60,
        // toward (-1.25, 3))
        mover.step(&mut world, &walking())?;
        let first = mover.velocity.truncate();
        assert!(
            near_by(first, Vec2::new(-0.128_205, 0.307_692), 2.0e-3),
            "{first}"
        );
        assert!(mover.velocity.z.abs() < 1.0e-2, "{}", mover.velocity);
        for _ in 0..30 {
            mover.step(&mut world, &walking())?;
        }
        let run = mover.velocity.truncate();
        assert!(near_by(run, Vec2::new(-1.25, 3.0), 1.0e-2), "{run}");
        assert!(!mover.airborne);
        Ok(())
    }

    #[test]
    fn in_the_air_it_falls_and_steers_by_its_airborne_acceleration() -> Result<(), PhysicsError> {
        let (mut world, mut mover) = grounded()?;
        mover.teleport(&mut world, Vec3::new(0.0, 0.0, 3.0))?;
        mover.airborne = true;
        mover.step(&mut world, &walking())?;
        let steer = mover.velocity.truncate();
        assert!(
            near_by(steer, Vec2::new(-0.061_538, 0.147_692), 1.0e-3),
            "{steer}"
        );
        assert!(
            (mover.velocity.z + 10.0 / 60.0).abs() < 1.0e-3,
            "{}",
            mover.velocity
        );
        assert!(mover.airborne);
        // (three units down at 10 a second squared: some 7.7 a second, past
        // the hard landing's 6)
        for _ in 0..120 {
            mover.step(&mut world, &MovementInput::default())?;
            if !mover.airborne {
                break;
            }
        }
        assert!(!mover.airborne);
        assert!(mover.landing_velocity > 6.0, "{}", mover.landing_velocity);
        assert_eq!(mover.landing, Some(Landing::Hard));
        Ok(())
    }

    #[test]
    fn against_a_walls_normal_it_walks_along_the_wall_and_five_times_as_steep(
    ) -> Result<(), PhysicsError> {
        let (mut world, mut mover) = grounded()?;
        // (an overhang's normal, facing down: forward along it and left
        // across it, the climb made five times as steep, and the speed of
        // the slope down it)
        mover.ground_normal = Vec3::new(0.6, 0.0, -0.8);
        mover.step(&mut world, &walking())?;
        let along = mover.velocity.truncate();
        assert!(
            near_by(along, Vec2::new(-0.202_390, 0.231_621), 2.0e-3),
            "{along}"
        );
        Ok(())
    }

    #[test]
    fn a_landing_with_no_range_to_recover_over_is_none() -> Result<(), PhysicsError> {
        let even = Biped {
            maximum_hard_landing_velocity: 6.0,
            ..Biped::default()
        };
        let (_world, mut mover) = mover(even, PlayerPhysics::default())?;
        mover.settings.biped.maximum_hard_landing_velocity = 6.0;
        mover.start_landing(7.0);
        assert_eq!(mover.landing, None);
        Ok(())
    }

    fn near(a: f32, b: f32) -> bool {
        (a - b).abs() < 1.0e-5
    }

    #[test]
    fn slopes_scale_the_speed() {
        let biped = cyborg();
        assert_eq!(slope_scale(&biped, 0.0), 1.0);
        assert_eq!(slope_scale(&biped, -0.9), 1.25);
        assert_eq!(slope_scale(&biped, 0.9), 0.65);
        let halfway = slope_scale(&biped, (0.342 + 0.707) / 2.0);
        assert!((halfway - 0.825).abs() < 1.0e-4, "{halfway}");
        // a quarter of the way up each ramp, and its ends
        let quarter = |from: f32, to: f32| from + (to - from) * 0.25;
        let down = slope_scale(&biped, quarter(-0.342, -0.707));
        assert!((down - 1.0625).abs() < 1.0e-4, "{down}");
        let up = slope_scale(&biped, quarter(0.342, 0.707));
        assert!((up - 0.9125).abs() < 1.0e-4, "{up}");
        assert_eq!(slope_scale(&biped, -0.707), 1.25);
        assert_eq!(slope_scale(&biped, 0.707), 0.65);
        assert_eq!(slope_scale(&biped, -0.342), 1.0);
        assert_eq!(slope_scale(&biped, 0.342), 1.0);
    }

    #[test]
    fn a_long_vector_is_clamped_to_the_longest_and_a_short_one_kept() {
        assert_eq!(clamp_length(Vec2::new(3.0, 4.0), 2.5), Vec2::new(1.5, 2.0));
        assert_eq!(clamp_length(Vec2::new(3.0, 4.0), 10.0), Vec2::new(3.0, 4.0));
        assert_eq!(clamp_length(Vec2::new(3.0, 4.0), 5.0), Vec2::new(3.0, 4.0));
    }

    #[test]
    fn its_radius_and_eye_follow_its_tag_and_its_crouch() -> Result<(), PhysicsError> {
        let (_world, mut mover) = mover(Biped::default(), PlayerPhysics::default())?;
        assert_eq!(mover.radius(), 0.2);
        assert!(near(mover.camera_height(), 0.62));
        mover.crouch = 0.5;
        assert!(near(mover.camera_height(), 0.485));
        Ok(())
    }

    #[test]
    fn a_landing_is_soft_or_hard_by_its_speed_and_recovered_from_in_its_ticks(
    ) -> Result<(), PhysicsError> {
        let (_world, mut mover) = mover(Biped::default(), PlayerPhysics::default())?;
        mover.start_landing(2.9);
        assert_eq!(mover.landing, None);
        // at the softest a landing is soft, and over at once
        mover.start_landing(3.0);
        assert_eq!(mover.landing, Some(Landing::Soft));
        assert_eq!(mover.landing_recovery_ticks, 0);
        mover.update_landing();
        assert_eq!(mover.landing, None);
        // a quarter of the way to hard: a quarter of the soft time, in
        // Halo's ticks (7.5, truncated), doubled
        mover.start_landing(3.75);
        assert_eq!(mover.landing, Some(Landing::Soft));
        assert_eq!(mover.landing_recovery_ticks, 14);
        for _ in 0..13 {
            mover.update_landing();
        }
        assert_eq!(mover.landing, Some(Landing::Soft));
        mover.update_landing();
        assert_eq!(mover.landing, None);
        // hard from the hardest's start, by the whole speed over its range
        mover.start_landing(6.0);
        assert_eq!(mover.landing, Some(Landing::Hard));
        assert_eq!(mover.landing_recovery_ticks, 30);
        mover.start_landing(12.0);
        assert_eq!(mover.landing_recovery_ticks, 60);
        mover.start_landing(100.0);
        assert_eq!(mover.landing_recovery_ticks, 120);
        Ok(())
    }

    #[test]
    fn a_crouch_takes_its_transition_time_and_keeps_the_head_still_in_the_air(
    ) -> Result<(), PhysicsError> {
        let (mut world, mut mover) = mover(Biped::default(), PlayerPhysics::default())?;
        assert!(mover.airborne);
        // (a quarter second: a fifteenth a tick, the pill shrinking 0.2 over
        // it, so its feet rise 0.2 / 15 a tick)
        let lift = mover.update_crouch(&mut world, true)?;
        assert!(near(mover.crouch, 1.0 / 15.0), "{}", mover.crouch);
        assert!(near(mover.height, 0.7 - 0.2 / 15.0), "{}", mover.height);
        assert!(near(lift, 0.2 / 15.0 * TICKS_PER_SECOND), "{lift}");
        let lowered = mover.update_crouch(&mut world, false)?;
        assert!(near(mover.crouch, 0.0), "{}", mover.crouch);
        assert!(near(lowered, -0.2 / 15.0 * TICKS_PER_SECOND), "{lowered}");
        // on the ground the feet stay where they are
        mover.airborne = false;
        assert_eq!(mover.update_crouch(&mut world, true)?, 0.0);
        // a crouch too slow to pass Halo's least change a tick is no lift
        let slow = Biped {
            crouch_transition_time: 10.0,
            ..Biped::default()
        };
        let (mut world, mut mover) = self::mover(slow, PlayerPhysics::default())?;
        mover.settings.biped.crouch_transition_time = 10.0;
        assert_eq!(mover.update_crouch(&mut world, true)?, 0.0);
        assert!(near(mover.crouch, 1.0 / 600.0), "{}", mover.crouch);
        // (two seconds: 1/120 a tick, past Halo's 0.01 a 30 Hz tick)
        mover.settings.biped.crouch_transition_time = 2.0;
        let lift = mover.update_crouch(&mut world, true)?;
        assert!(near(lift, 0.2 / 120.0 * TICKS_PER_SECOND), "{lift}");
        Ok(())
    }

    #[test]
    fn a_jump_waits_for_its_recovery_on_the_ground() -> Result<(), PhysicsError> {
        let (_world, mut mover) = mover(Biped::default(), PlayerPhysics::default())?;
        mover.airborne = false;
        // (more than five of Halo's ticks: the eleventh of the engine's)
        for _ in 0..10 {
            mover.update_jump(true);
            assert!(!mover.airborne);
        }
        mover.update_jump(true);
        assert!(mover.airborne);
        assert_eq!(mover.velocity.z, 2.5);
        // in the air, or landing hard, it cannot jump again
        mover.velocity.z = 0.0;
        mover.jump_recovery_ticks = 100;
        mover.update_jump(true);
        assert_eq!(mover.velocity.z, 0.0);
        mover.airborne = false;
        mover.landing = Some(Landing::Hard);
        mover.update_jump(true);
        assert!(!mover.airborne);
        Ok(())
    }

    #[test]
    fn the_controls_ask_for_the_players_speeds() -> Result<(), PhysicsError> {
        let player = PlayerPhysics {
            run_forward: 3.0,
            run_sideways: 2.5,
            run_acceleration: 20.0,
            ..PlayerPhysics::default()
        };
        let (_world, mut mover) = mover(Biped::default(), player)?;
        let input = |x: f32, y: f32| MovementInput {
            throttle: Vec2::new(x, y),
            ..MovementInput::default()
        };
        let (wanted, acceleration, third, fourth) = mover.desired(&input(1.0, 0.0));
        assert_eq!(
            (wanted, acceleration, fourth),
            (Vec2::new(3.0, 0.0), 20.0, 0.0)
        );
        assert!(near(third, 9.6), "{third}");
        // (at the dead zone's edge a throttle counts)
        assert_eq!(mover.desired(&input(0.0, 0.1)).0, Vec2::new(0.0, 0.25));
        assert_eq!(mover.desired(&input(0.0, 0.09)).0, Vec2::ZERO);
        // a biped on the old physics takes the old speeds
        mover.settings.biped.flags = Biped::USES_OLD_PLAYER_PHYSICS;
        let (wanted, acceleration, _, _) = mover.desired(&input(1.0, 0.0));
        assert_eq!(wanted, Vec2::new(2.25, 0.0));
        assert!(near(acceleration, 9.6), "{acceleration}");
        // and its other flags leave the player's
        mover.settings.biped.flags = !Biped::USES_OLD_PLAYER_PHYSICS;
        assert_eq!(mover.desired(&input(1.0, 0.0)).0, Vec2::new(3.0, 0.0));
        mover.landing = Some(Landing::Hard);
        let (wanted, acceleration, third, fourth) = mover.desired(&input(1.0, 0.0));
        assert_eq!((wanted, acceleration, fourth), (Vec2::ZERO, 0.0, 1.0));
        assert!(near(third, 9.6), "{third}");
        Ok(())
    }
}
