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
                layers: Layers::LEVEL,
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
        // the velocity the collisions left, Halo's clipped velocity)
        self.velocity =
            (state.position - self.position) / TICK - Vec3::new(0.0, 0.0, crouch_velocity);
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

    #[test]
    fn slopes_scale_the_speed() {
        let biped = cyborg();
        assert_eq!(slope_scale(&biped, 0.0), 1.0);
        assert_eq!(slope_scale(&biped, -0.9), 1.25);
        assert_eq!(slope_scale(&biped, 0.9), 0.65);
        let halfway = slope_scale(&biped, (0.342 + 0.707) / 2.0);
        assert!((halfway - 0.825).abs() < 1.0e-4, "{halfway}");
    }
}
