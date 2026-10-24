//! The states a unit animates in, and the one a biped wants
//! (source/units/units.c `unit_update_animation` and
//! `unit_animation_set_state`, source/units/bipeds.c `biped_update`):
//! each state's animation in the unit's seat and weapon class, which
//! states may interrupt which, how long the change blends, the aiming
//! screen over each, and the frame by frame update of the base, ping,
//! action and overlay animations.
//!
//! What the game does in the world when an animation reaches a key frame
//! or its end (a grenade let go, a melee blow, a body gone limp) is given
//! back as [`UnitEvent`]s for the world to carry out.

use opence_content::AnimationGraph;

use crate::animation::{advance, choose_permutation, AnimationState, Update};
use crate::animation_graph::{seat_and_weapon, SeatAndWeapon};

/// A unit's animation state (`_unit_state_*`, source/units/units.h).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[repr(u8)]
#[allow(missing_docs)]
pub enum UnitState {
    Idle,
    Gesture,
    TurnLeft,
    TurnRight,
    MoveFront,
    MoveBack,
    MoveLeft,
    MoveRight,
    StunnedMoveFront,
    StunnedMoveBack,
    StunnedMoveLeft,
    StunnedMoveRight,
    SlideFront,
    SlideBack,
    SlideLeft,
    SlideRight,
    FlyingFront,
    FlyingBack,
    FlyingLeft,
    FlyingRight,
    Airborne,
    LandSoft,
    LandHard,
    HardPing,
    DyingAirborne,
    Dying,
    EnteringSeat,
    ExitingSeat,
    UserAnimation,
    AiImpulse,
    MeleeAttack,
    MeleeAirborne,
    MeleeContinuous,
    ThrowGrenade,
    ResurrectFront,
    ResurrectBack,
    Feeding,
    Opening,
    Closing,
    LeapStart,
    LeapAirborne,
    LeapMelee,
    ShieldSapping,
    Hovering,
}

use UnitState as S;

impl UnitState {
    /// Every state, in the game's order.
    pub const ALL: [UnitState; 44] = [
        S::Idle,
        S::Gesture,
        S::TurnLeft,
        S::TurnRight,
        S::MoveFront,
        S::MoveBack,
        S::MoveLeft,
        S::MoveRight,
        S::StunnedMoveFront,
        S::StunnedMoveBack,
        S::StunnedMoveLeft,
        S::StunnedMoveRight,
        S::SlideFront,
        S::SlideBack,
        S::SlideLeft,
        S::SlideRight,
        S::FlyingFront,
        S::FlyingBack,
        S::FlyingLeft,
        S::FlyingRight,
        S::Airborne,
        S::LandSoft,
        S::LandHard,
        S::HardPing,
        S::DyingAirborne,
        S::Dying,
        S::EnteringSeat,
        S::ExitingSeat,
        S::UserAnimation,
        S::AiImpulse,
        S::MeleeAttack,
        S::MeleeAirborne,
        S::MeleeContinuous,
        S::ThrowGrenade,
        S::ResurrectFront,
        S::ResurrectBack,
        S::Feeding,
        S::Opening,
        S::Closing,
        S::LeapStart,
        S::LeapAirborne,
        S::LeapMelee,
        S::ShieldSapping,
        S::Hovering,
    ];

    /// The state of the game's number; none past the last.
    pub fn from_index(index: usize) -> Option<Self> {
        Self::ALL.get(index).copied()
    }

    /// Its number in the game.
    pub fn index(self) -> usize {
        self as usize
    }

    /// The state `offset` on in the game's order (a direction of a
    /// movement state's four); none past the last.
    fn offset(self, offset: usize) -> Option<Self> {
        Self::from_index(self.index() + offset)
    }
}

/// Where a state's animation is listed: in the weapon class's animations
/// ([`crate::animation_graph::WEAPON_CLASS`]) or the seat's
/// ([`crate::animation_graph::UNIT_SEAT`]).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum StateAnimation {
    /// The weapon class's animation of this index.
    Class(usize),
    /// The seat's animation of this index.
    Seat(usize),
}

/// The weapon class's `aiming-still`.
pub const AIMING_STILL: usize = 25;
/// The weapon class's `aiming-moving`.
pub const AIMING_MOVING: usize = 26;
/// The seat's `looking-screen`.
pub const LOOKING_SCREEN: usize = 9;

/// The animation a unit plays in `state` (`unit_animation_set_state`'s
/// switch); none for a state played without one of its own (a gesture,
/// a seat's entry and exit, a user animation, which are started
/// elsewhere).
pub fn state_animation(state: UnitState) -> Option<StateAnimation> {
    use StateAnimation::{Class, Seat};
    Some(match state {
        S::Idle => Class(0),
        S::Gesture => Class(1),
        S::TurnLeft => Class(2),
        S::TurnRight => Class(3),
        S::MoveFront => Class(8),
        S::MoveBack => Class(9),
        S::MoveLeft => Class(10),
        S::MoveRight => Class(11),
        S::StunnedMoveFront => Class(35),
        S::StunnedMoveBack => Class(36),
        S::StunnedMoveLeft => Class(37),
        S::StunnedMoveRight => Class(38),
        S::SlideFront => Class(12),
        S::SlideBack => Class(13),
        S::SlideLeft => Class(14),
        S::SlideRight => Class(15),
        S::Airborne => Class(16),
        S::LandSoft => Class(17),
        S::LandHard => Class(18),
        S::ThrowGrenade => Class(20),
        S::MeleeAttack => Class(39),
        S::MeleeAirborne => Class(42),
        S::MeleeContinuous => Class(46),
        S::ResurrectFront => Class(44),
        S::ResurrectBack => Class(45),
        S::Feeding => Class(47),
        S::LeapStart => Class(48),
        S::LeapAirborne => Class(49),
        S::LeapMelee => Class(50),
        S::FlyingFront => Seat(23),
        S::FlyingBack => Seat(24),
        S::FlyingLeft => Seat(25),
        S::FlyingRight => Seat(26),
        S::DyingAirborne => Seat(0),
        S::Dying => Seat(1),
        S::Opening => Seat(27),
        S::Closing => Seat(28),
        S::Hovering => Seat(29),
        _ => return None,
    })
}

/// Whether a unit may be put in `state` when its seat and weapon have no
/// animation for it (`unit_animation_state_can_be_entered_without_animation`):
/// not an attack, which must play to deal its blow.
pub fn entered_without_animation(state: UnitState) -> bool {
    !matches!(
        state,
        S::MeleeAttack
            | S::MeleeAirborne
            | S::MeleeContinuous
            | S::ThrowGrenade
            | S::LeapStart
            | S::LeapMelee
    )
}

/// Whether a unit in `current` (none: its seat just changed) may go to
/// `desired` before its animation ends (`unit_animation_state_interruptable`).
pub fn interruptable(current: Option<UnitState>, desired: UnitState) -> bool {
    let Some(current) = current else {
        return true;
    };
    match current {
        S::AiImpulse
        | S::MeleeAttack
        | S::MeleeAirborne
        | S::ThrowGrenade
        | S::ResurrectFront
        | S::ResurrectBack
        | S::LeapStart
        | S::LeapMelee => desired == S::HardPing,
        S::DyingAirborne | S::Dying => matches!(desired, S::DyingAirborne | S::Dying),
        S::TurnLeft | S::TurnRight | S::Opening | S::Closing => desired != S::Idle,
        S::HardPing | S::EnteringSeat | S::ExitingSeat | S::UserAnimation => false,
        _ => true,
    }
}

/// Whether a unit's animation in `state` plays on past its end
/// (`unit_animation_state_loops`); one that doesn't hands the unit to the
/// state it wants as it ends.
pub fn loops(state: Option<UnitState>) -> bool {
    !matches!(
        state,
        Some(
            S::Gesture
                | S::TurnLeft
                | S::TurnRight
                | S::HardPing
                | S::EnteringSeat
                | S::ExitingSeat
                | S::UserAnimation
                | S::AiImpulse
                | S::MeleeAttack
                | S::MeleeAirborne
                | S::ThrowGrenade
                | S::ResurrectFront
                | S::ResurrectBack
                | S::LeapStart
                | S::LeapMelee
        )
    )
}

/// Whether an overlay action plays on past its end in `state`
/// (`unit_animation_overlay_action_loops`): only in a right turn and
/// moving forward, as the game's range of states is (it likely meant
/// another range; kept as it plays).
pub fn overlay_action_loops(state: Option<UnitState>) -> bool {
    matches!(state, Some(S::TurnRight | S::MoveFront))
}

/// The frames a change from `old` to `new` blends over
/// (`unit_animation_compute_interpolation_frame_count`): one between
/// standing and turning, two into a landing, else six.
pub fn interpolation_frames(new: UnitState, old: Option<UnitState>) -> u16 {
    let standing = |state| matches!(state, Some(S::Idle | S::TurnLeft | S::TurnRight));
    if matches!(new, S::LandHard | S::LandSoft) {
        2
    } else if standing(Some(new)) && standing(old) {
        1
    } else {
        6
    }
}

/// The weapon class's aiming screen over `state`
/// (`unit_animation_state_get_aiming_screen_index`): standing still or
/// moving; none in the others, which aim with the body.
pub fn aiming_screen(state: Option<UnitState>) -> Option<usize> {
    match state? {
        S::Idle
        | S::TurnLeft
        | S::TurnRight
        | S::FlyingFront
        | S::FlyingBack
        | S::FlyingLeft
        | S::FlyingRight
        | S::Airborne
        | S::LandSoft
        | S::LandHard
        | S::Opening
        | S::Closing => Some(AIMING_STILL),
        S::MoveFront
        | S::MoveBack
        | S::MoveLeft
        | S::MoveRight
        | S::StunnedMoveFront
        | S::StunnedMoveBack
        | S::StunnedMoveLeft
        | S::StunnedMoveRight
        | S::SlideFront
        | S::SlideBack
        | S::SlideLeft
        | S::SlideRight => Some(AIMING_MOVING),
        _ => None,
    }
}

/// How a unit holds itself (`_unit_animation_state_*`), which picks the
/// seat it stands in.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[allow(missing_docs)]
pub enum Mood {
    Asleep,
    #[default]
    Alert,
    Suspicious,
    InCombat,
    Wary,
    Flee,
    Flaming,
}

/// The base seats' labels (`base_seat_labels`).
pub const BASE_SEATS: [&str; 6] = ["asleep", "alert", "stand", "crouch", "flee", "flaming"];

/// A seat's animation of entering it (`_unit_seat_animation_seat_enter`).
const SEAT_ENTER: usize = 7;

/// The base seat `asleep`.
pub const SEAT_ASLEEP: usize = 0;
/// The base seat `alert`.
pub const SEAT_ALERT: usize = 1;
/// The base seat `stand`.
pub const SEAT_STAND: usize = 2;
/// The base seat `flee`.
pub const SEAT_FLEE: usize = 4;
/// The base seat `flaming`.
pub const SEAT_FLAMING: usize = 5;

/// What decides a unit's base seat beside its mood.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct SeatChoice {
    /// It crouches (in combat, the crouching seat).
    pub crouching: bool,
    /// The seat a script put it in (`magic_seat_index`).
    pub magic_seat: Option<usize>,
    /// It is told to stay alert (`_unit_control_force_alert_bit`).
    pub force_alert: bool,
    /// It burns, before it dies of it (`flaming_death_delay`).
    pub flaming: bool,
}

/// The base seat a unit in `mood` stands in (`unit_update_animation`):
/// its mood's, unless a script's seat, being told to stay alert or
/// burning (each over the one before) says otherwise.
pub fn base_seat(mood: Mood, choice: SeatChoice) -> usize {
    let mut seat = match mood {
        Mood::Asleep => SEAT_ASLEEP,
        Mood::Alert | Mood::Suspicious => SEAT_ALERT,
        Mood::InCombat => SEAT_STAND + usize::from(choice.crouching),
        Mood::Wary => SEAT_STAND,
        Mood::Flee => SEAT_FLEE,
        Mood::Flaming => SEAT_FLAMING,
    };
    if let Some(magic) = choice.magic_seat {
        seat = magic;
    }
    if choice.force_alert {
        seat = SEAT_ALERT;
    }
    if choice.flaming {
        seat = SEAT_FLAMING;
    }
    seat
}

/// What a unit's animation asks of the world.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum UnitEvent {
    /// Blend from the pose it had over this many frames
    /// (`object_start_interpolation`).
    Interpolate(u16),
    /// Let go of the grenade it throws: at the throw's key frame, or
    /// `early` as the throw is cut off (`unit_throw_grenade_release`).
    ReleaseGrenade {
        /// The throw was cut off.
        early: bool,
    },
    /// Deal its melee blow (`unit_cause_melee_damage`), at the attack's
    /// key frame.
    MeleeDamage,
    /// It finished dying: take it away (`unit_destroy`).
    Destroy,
    /// It finished dying as a biped: let its body go limp
    /// (`biped_start_limp_body_physics`).
    GoLimp,
    /// It finished getting out of its seat: leave it, keeping the exit
    /// animation's speed (`unit_exit_seat_end`).
    ExitedSeat,
    /// It finished getting into its seat: show or hide it as the seat
    /// says, and close the vehicle behind a driver.
    EnteredSeat,
}

/// A unit's animation (`unit.animation` and `object.animation`).
#[derive(Clone, Debug, Default, PartialEq)]
pub struct UnitAnimation {
    /// The state it plays (none: its seat just changed).
    pub state: Option<UnitState>,
    /// How it holds itself.
    pub mood: Mood,
    /// The base seat it stands in ([`BASE_SEATS`]; none for another).
    pub base_seat: Option<usize>,
    /// Its seat, weapon class and weapon type.
    pub seat: Option<SeatAndWeapon>,
    /// Its base animation.
    pub base: Option<AnimationState>,
    /// The aiming screen over it.
    pub aiming_screen: Option<u16>,
    /// The looking screen over it.
    pub looking_screen: Option<u16>,
    /// The soft flinch over it.
    pub soft_ping: Option<AnimationState>,
    /// The action (a replacement: a reload, a melee) over it.
    pub action: Option<AnimationState>,
    /// The overlay action over it.
    pub overlay_action: Option<AnimationState>,
    /// Its animation no longer moves it (dead, it lies where it fell).
    pub ignore_translation: bool,
}

/// What the world knows of a unit as its animation updates.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct UnitTick<'a> {
    /// The state it wants.
    pub desired: Option<UnitState>,
    /// What decides its base seat beside its mood.
    pub seat: SeatChoice,
    /// The label of the weapon it holds (`unarmed` for none).
    pub weapon_label: Option<&'a str>,
    /// It rides something (it keeps the seat it has).
    pub attached: bool,
    /// It is dead.
    pub dead: bool,
    /// It is gone once it finishes dying
    /// (`_unit_is_destroyed_after_dying_bit`).
    pub destroyed_after_dying: bool,
    /// It is at rest.
    pub at_rest: bool,
    /// It is a biped.
    pub biped: bool,
    /// It is a biped in the air.
    pub airborne: bool,
    /// It is a biped with no dying-in-the-air animation
    /// (`_biped_has_no_dying_airborne_bit`).
    pub no_dying_airborne: bool,
}

/// The animation `slot` of a unit's seat and weapon in `graph`.
fn slot_animation(
    graph: &AnimationGraph,
    seat: Option<&SeatAndWeapon>,
    slot: StateAnimation,
) -> Option<u16> {
    let seat = seat?;
    match slot {
        StateAnimation::Class(index) => seat.class_animation(graph, index),
        StateAnimation::Seat(index) => seat.seat_animation(graph, index),
    }
}

impl UnitAnimation {
    /// Puts the unit in `new_state` (`unit_animation_set_state`): its
    /// animation there, a random one of its permutations, blended in; a
    /// new aiming screen as it changes, and a looking screen after its
    /// seat changed. False, changing nothing, for an attack its seat and
    /// weapon have no animation for. `random` draws from 0 to 1 for each
    /// permutation chosen.
    pub fn set_state(
        &mut self,
        graph: &AnimationGraph,
        new_state: UnitState,
        random: &mut impl FnMut() -> f32,
        events: &mut Vec<UnitEvent>,
    ) -> bool {
        let old_state = self.state;
        let mut interpolation = None;
        if old_state != Some(new_state) {
            let animation = state_animation(new_state)
                .and_then(|slot| slot_animation(graph, self.seat.as_ref(), slot));
            if animation.is_none() && !entered_without_animation(new_state) {
                return false;
            }
            if old_state == Some(S::ThrowGrenade) {
                events.push(UnitEvent::ReleaseGrenade { early: true });
            }
            self.base =
                choose_permutation(&graph.animations, animation, random()).map(|animation| {
                    AnimationState {
                        animation,
                        frame: 0,
                    }
                });
            interpolation = Some(interpolation_frames(new_state, old_state));
        }
        let screen = aiming_screen(Some(new_state));
        if old_state.is_none() || screen != aiming_screen(old_state) {
            let animation = screen.and_then(|index| {
                slot_animation(graph, self.seat.as_ref(), StateAnimation::Class(index))
            });
            self.aiming_screen = choose_permutation(&graph.animations, animation, random());
            interpolation = Some(6);
            if old_state.is_none() {
                let animation = slot_animation(
                    graph,
                    self.seat.as_ref(),
                    StateAnimation::Seat(LOOKING_SCREEN),
                );
                self.looking_screen = choose_permutation(&graph.animations, animation, random());
            }
        }
        if let Some(frames) = interpolation {
            events.push(UnitEvent::Interpolate(frames));
        }
        self.state = Some(new_state);
        true
    }

    /// Puts the unit in a vehicle's seat labelled `seat_label`
    /// (`unit_enter_seat`): that seat with `weapon_label`'s class, or any
    /// class, and its animation of entering the seat blended in, if its
    /// graph has one. False, changing nothing, when the graph has no such
    /// seat.
    pub fn enter_seat(
        &mut self,
        graph: &AnimationGraph,
        seat_label: &str,
        weapon_label: Option<&str>,
        random: &mut impl FnMut() -> f32,
        events: &mut Vec<UnitEvent>,
    ) -> bool {
        let Some(found) = seat_and_weapon(graph, Some(seat_label), weapon_label)
            .or_else(|| seat_and_weapon(graph, Some(seat_label), None))
        else {
            return false;
        };
        self.seat = Some(found);
        self.base_seat = None;
        self.state = None;
        let animation = slot_animation(graph, self.seat.as_ref(), StateAnimation::Seat(SEAT_ENTER));
        if let Some(animation) = choose_permutation(&graph.animations, animation, random()) {
            events.push(UnitEvent::Interpolate(6));
            self.base = Some(AnimationState {
                animation,
                frame: 0,
            });
            self.state = Some(S::EnteringSeat);
        }
        true
    }

    /// Puts the unit in base seat `base_seat` with `weapon_label`
    /// (`unit_set_or_test_seat_and_weapon_label`): its state is none
    /// after (but a user animation's), to be chosen again in the new seat.
    /// False, changing nothing, when the graph has no such seat.
    pub fn change_seat(
        &mut self,
        graph: &AnimationGraph,
        base_seat: usize,
        weapon_label: Option<&str>,
    ) -> bool {
        let label = BASE_SEATS.get(base_seat).copied();
        let Some(found) = seat_and_weapon(graph, label, weapon_label) else {
            return false;
        };
        if self.state != Some(S::UserAnimation) {
            self.state = None;
        }
        self.seat = Some(found);
        self.base_seat = label.map(|_| base_seat);
        true
    }

    /// A frame of the unit's animation (`unit_update_animation`): its
    /// seat as its mood says, then its soft flinch, its base animation,
    /// its action and its overlay action played on, and the state it
    /// wants taken when its own may give way. True when a leap's start
    /// ended (the unit leaps).
    pub fn update(
        &mut self,
        graph: &AnimationGraph,
        tick: &UnitTick<'_>,
        random: &mut impl FnMut() -> f32,
        events: &mut Vec<UnitEvent>,
    ) -> bool {
        let mut desired = tick.desired;
        let mut leapt = false;
        let mut apply_state = false;
        if !tick.attached && !tick.dead {
            let seat = base_seat(self.mood, tick.seat);
            if self.base_seat != Some(seat)
                && desired.is_some_and(|desired| interruptable(self.state, desired))
            {
                self.change_seat(graph, seat, tick.weapon_label);
            }
        }
        if let Some(mut soft_ping) = self.soft_ping {
            let update = advance(&graph.animations, &mut soft_ping, &mut *random);
            self.soft_ping = (update != Update::WillRestartOnNextFrame).then_some(soft_ping);
        }
        if let Some(mut base) = self.base {
            let update = advance(&graph.animations, &mut base, &mut *random);
            match update {
                Update::KeyFrame => match self.state {
                    Some(S::ThrowGrenade) => {
                        events.push(UnitEvent::ReleaseGrenade { early: false })
                    }
                    Some(S::MeleeAttack | S::MeleeAirborne | S::LeapMelee) => {
                        events.push(UnitEvent::MeleeDamage)
                    }
                    _ => {}
                },
                Update::WillRestartOnNextFrame => {
                    match self.state {
                        Some(S::LeapStart) => {
                            leapt = true;
                            desired = Some(S::LeapAirborne);
                        }
                        Some(S::Dying) => {
                            if tick.destroyed_after_dying
                                && (tick.at_rest
                                    || (tick.biped && (!tick.airborne || tick.no_dying_airborne)))
                            {
                                events.push(UnitEvent::Destroy);
                            } else {
                                if tick.biped {
                                    events.push(UnitEvent::GoLimp);
                                }
                                self.ignore_translation = true;
                                base.frame = base.frame.wrapping_sub(1);
                            }
                        }
                        Some(S::ExitingSeat) => events.push(UnitEvent::ExitedSeat),
                        Some(S::Opening | S::Closing) => base.frame = base.frame.wrapping_sub(1),
                        Some(S::EnteringSeat) => events.push(UnitEvent::EnteredSeat),
                        _ => {}
                    }
                    apply_state = !loops(self.state);
                }
                _ => {}
            }
            self.base = Some(base);
        }
        if let Some(mut action) = self.action {
            if advance(&graph.animations, &mut action, &mut *random)
                == Update::WillRestartOnNextFrame
            {
                events.push(UnitEvent::Interpolate(6));
                self.action = None;
            } else {
                self.action = Some(action);
            }
        }
        if let Some(mut overlay) = self.overlay_action {
            let update = advance(&graph.animations, &mut overlay, &mut *random);
            let ended = matches!(update, Update::WillRestartOnNextFrame | Update::Looped);
            self.overlay_action = (!ended || overlay_action_loops(self.state)).then_some(overlay);
        }
        if let Some(desired) = desired {
            if apply_state || (Some(desired) != self.state && interruptable(self.state, desired)) {
                self.set_state(graph, desired, random, events);
            }
        }
        leapt
    }
}

/// What a biped is doing, for the state it wants (`biped_update`).
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct BipedTick {
    /// The state it plays now.
    pub state: Option<UnitState>,
    /// It is dead.
    pub dead: bool,
    /// Its body has gone limp and is still settling (it keeps the state
    /// it wanted).
    pub limp: bool,
    /// It is in the air (`_biped_airborne_bit`).
    pub airborne: bool,
    /// The frames it has been in the air.
    pub airborne_ticks: u8,
    /// It has no dying-in-the-air animation.
    pub no_dying_airborne: bool,
    /// It flies (`_biped_flying_bit`).
    pub flying: bool,
    /// Its landing: none, soft (0) or hard (1).
    pub landing: Option<u8>,
    /// The way it is told to move: forward, left, up.
    pub throttle: [f32; 3],
    /// It is stunned (body stun over 0.2, or recent damage over its
    /// stunned movement threshold).
    pub stunned: bool,
    /// The turn it makes standing ([`standing_turn`]).
    pub turn: Option<UnitState>,
}

/// Whether a biped standing must turn to face where it is told, and
/// which way (`biped_update_turning`): when its facing is further than
/// `threshold` (a cosine) from `desired_facing` (none when it may not
/// turn standing: burning, aiming without turning, a special unit, or
/// not standing).
pub fn standing_turn(
    forward: [f32; 3],
    desired_facing: [f32; 3],
    threshold: f32,
) -> Option<UnitState> {
    let length =
        libm::sqrtf(desired_facing[0] * desired_facing[0] + desired_facing[1] * desired_facing[1]);
    let axis = if length == 0.0 {
        [forward[0], forward[1]]
    } else {
        [desired_facing[0] / length, desired_facing[1] / length]
    };
    let turn_error = axis[0] * forward[1] - axis[1] * forward[0];
    let alignment = axis[0] * forward[0] + axis[1] * forward[1];
    (alignment < threshold).then_some(if turn_error > 0.0 {
        S::TurnRight
    } else {
        S::TurnLeft
    })
}

/// The state a biped wants (`biped_update`): standing, turning or
/// moving in one of four directions as it is told; then dying, in the
/// air, leaping or landing over that.
pub fn biped_desired_state(biped: &BipedTick) -> UnitState {
    let mut desired = S::Idle;
    if !biped.dead {
        if let Some(turn) = biped.turn {
            desired = turn;
        }
    }
    if biped.landing != Some(1) {
        if !biped.flying || biped.dead {
            let [x, y, z] = biped.throttle;
            if x != 0.0 || y != 0.0 || z != 0.0 {
                let base = if biped.stunned {
                    S::StunnedMoveFront
                } else {
                    S::MoveFront
                };
                let offset = if libm::fabsf(x) >= libm::fabsf(y) {
                    usize::from(x < 0.0)
                } else {
                    2 + usize::from(y < 0.0)
                };
                desired = base.offset(offset).unwrap_or(base);
            }
        } else {
            desired = S::Idle;
        }
    }
    if biped.dead {
        if biped.limp {
            return desired;
        }
        return if biped.airborne_ticks >= 3 && !biped.no_dying_airborne {
            S::DyingAirborne
        } else {
            S::Dying
        };
    }
    if biped.airborne {
        let flying_through_air = biped.airborne_ticks > 3 && !biped.flying;
        if matches!(biped.state, Some(S::LeapStart | S::LeapAirborne)) {
            desired = S::LeapAirborne;
        } else if biped.state == Some(S::Airborne) || flying_through_air {
            desired = S::Airborne;
        }
    } else if let Some(landing) = biped.landing {
        desired = if landing == 1 {
            S::LandHard
        } else {
            S::LandSoft
        };
    }
    desired
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Animation, AnimationKind, UnitSeat, WeaponClass, WeaponType};

    fn animation(frames: u16) -> Animation {
        let mut animation = crate::animation::tests::animation_named("a", 2);
        animation.kind = AnimationKind::Base;
        animation.frame_count = frames;
        animation.normalized_weight = 1.0;
        animation
    }

    /// A graph with a standing and a crouching seat: 0 idle, 1 move
    /// front, 2 aiming still, 3 aiming moving, 4 throw (key frame 2), 5
    /// looking, 6 dying, 7 the crouching idle, 8 turn left, 9 land hard,
    /// 10 melee (key frame 1), 11 leap start, 12 an action.
    fn graph() -> AnimationGraph {
        let mut animations: Vec<Animation> = (0..13).map(|_| animation(5)).collect();
        animations[4].key_frame = 2;
        animations[10].key_frame = 1;
        let mut graph = crate::animation::tests::graph_of(animations);
        let mut class_animations = vec![None; 51];
        class_animations[0] = Some(0);
        class_animations[2] = Some(8);
        class_animations[8] = Some(1);
        class_animations[18] = Some(9);
        class_animations[20] = Some(4);
        class_animations[AIMING_STILL] = Some(2);
        class_animations[AIMING_MOVING] = Some(3);
        class_animations[39] = Some(10);
        class_animations[48] = Some(11);
        let mut seat_animations = vec![None; 30];
        seat_animations[1] = Some(6);
        seat_animations[LOOKING_SCREEN] = Some(5);
        let class = |animations: Vec<Option<u16>>| WeaponClass {
            label: "rifle".into(),
            animations,
            weapon_types: vec![WeaponType {
                label: "ar".into(),
                animations: vec![],
            }],
            ..WeaponClass::default()
        };
        let mut crouch_animations = class_animations.clone();
        crouch_animations[0] = Some(7);
        graph.unit_seats = vec![
            UnitSeat {
                label: "stand".into(),
                animations: seat_animations.clone(),
                weapon_classes: vec![class(class_animations)],
                ..UnitSeat::default()
            },
            UnitSeat {
                label: "crouch".into(),
                animations: seat_animations,
                weapon_classes: vec![class(crouch_animations)],
                ..UnitSeat::default()
            },
        ];
        graph
    }

    fn standing(graph: &AnimationGraph) -> Result<UnitAnimation, Box<dyn std::error::Error>> {
        let mut unit = UnitAnimation {
            mood: Mood::InCombat,
            ..UnitAnimation::default()
        };
        if !unit.change_seat(graph, SEAT_STAND, None) {
            return Err("no standing seat".into());
        }
        let mut events = Vec::new();
        if !unit.set_state(graph, S::Idle, &mut || 0.5, &mut events) {
            return Err("cannot stand".into());
        }
        Ok(unit)
    }

    fn tick(desired: UnitState) -> UnitTick<'static> {
        UnitTick {
            desired: Some(desired),
            ..UnitTick::default()
        }
    }

    #[test]
    fn every_state_has_its_number() -> Result<(), Box<dyn std::error::Error>> {
        for (index, state) in UnitState::ALL.iter().enumerate() {
            assert_eq!(state.index(), index);
            assert_eq!(UnitState::from_index(index), Some(*state));
        }
        assert_eq!(UnitState::from_index(44), None);
        assert_eq!(S::Hovering.offset(1), None);
        Ok(())
    }

    #[test]
    fn a_seat_change_sets_the_state_none_and_the_looking_screen(
    ) -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let unit = standing(&graph)?;
        assert_eq!(unit.state, Some(S::Idle));
        assert_eq!(unit.base_seat, Some(SEAT_STAND));
        assert_eq!(unit.base.map(|base| base.animation), Some(0));
        assert_eq!(unit.aiming_screen, Some(2));
        assert_eq!(unit.looking_screen, Some(5));
        let mut unit = unit;
        assert!(!unit.change_seat(&graph, SEAT_FLEE, None));
        assert_eq!(unit.state, Some(S::Idle));
        unit.state = Some(S::UserAnimation);
        assert!(unit.change_seat(&graph, SEAT_STAND, None));
        assert_eq!(unit.state, Some(S::UserAnimation));
        Ok(())
    }

    #[test]
    fn moving_changes_the_aiming_screen_and_blends_six_frames(
    ) -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        assert!(unit.set_state(&graph, S::TurnLeft, &mut || 0.5, &mut events));
        assert_eq!(events, [UnitEvent::Interpolate(1)]);
        events.clear();
        assert!(unit.set_state(&graph, S::MoveFront, &mut || 0.5, &mut events));
        assert_eq!(events, [UnitEvent::Interpolate(6)]);
        assert_eq!(unit.aiming_screen, Some(3));
        events.clear();
        // (moving to a landing changes the screen too: six frames, not two)
        assert!(unit.set_state(&graph, S::LandHard, &mut || 0.5, &mut events));
        assert_eq!(events, [UnitEvent::Interpolate(6)]);
        events.clear();
        assert!(unit.set_state(&graph, S::LandHard, &mut || 0.5, &mut events));
        assert!(events.is_empty());
        Ok(())
    }

    #[test]
    fn an_attack_needs_its_animation() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        assert!(!unit.set_state(&graph, S::MeleeAirborne, &mut || 0.5, &mut events));
        assert_eq!(unit.state, Some(S::Idle));
        assert!(unit.set_state(&graph, S::Gesture, &mut || 0.5, &mut events));
        assert_eq!(unit.base, None);
        assert_eq!(unit.aiming_screen, None);
        Ok(())
    }

    #[test]
    fn a_throw_lets_go_at_its_key_frame_and_ends_standing() -> Result<(), Box<dyn std::error::Error>>
    {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        assert!(unit.set_state(&graph, S::ThrowGrenade, &mut || 0.5, &mut events));
        events.clear();
        let mut all = Vec::new();
        for _ in 0..4 {
            // (the unit wants to move, which a throw holds off)
            unit.update(&graph, &tick(S::MoveFront), &mut || 0.5, &mut events);
            all.append(&mut events);
        }
        assert!(all.contains(&UnitEvent::ReleaseGrenade { early: false }));
        assert_eq!(unit.state, Some(S::MoveFront));
        Ok(())
    }

    #[test]
    fn a_cut_off_throw_lets_go_early() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        assert!(unit.set_state(&graph, S::ThrowGrenade, &mut || 0.5, &mut events));
        events.clear();
        unit.update(&graph, &tick(S::HardPing), &mut || 0.5, &mut events);
        assert_eq!(unit.state, Some(S::HardPing));
        assert!(events.contains(&UnitEvent::ReleaseGrenade { early: true }));
        Ok(())
    }

    #[test]
    fn a_melee_deals_its_blow() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        assert!(unit.set_state(&graph, S::MeleeAttack, &mut || 0.5, &mut events));
        events.clear();
        unit.update(&graph, &tick(S::Idle), &mut || 0.5, &mut events);
        assert_eq!(events, [UnitEvent::MeleeDamage]);
        Ok(())
    }

    #[test]
    fn a_leap_start_leaps() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        assert!(unit.set_state(&graph, S::LeapStart, &mut || 0.5, &mut events));
        let mut leapt = false;
        for _ in 0..4 {
            leapt |= unit.update(&graph, &tick(S::Idle), &mut || 0.5, &mut events);
        }
        assert!(leapt);
        assert_eq!(unit.state, Some(S::LeapAirborne));
        Ok(())
    }

    #[test]
    fn dying_ends_in_a_limp_body_or_is_taken_away() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        for (destroyed, airborne, expected) in [
            (false, false, UnitEvent::GoLimp),
            (true, true, UnitEvent::GoLimp),
            (true, false, UnitEvent::Destroy),
        ] {
            let mut unit = standing(&graph)?;
            let mut events = Vec::new();
            assert!(unit.set_state(&graph, S::Dying, &mut || 0.5, &mut events));
            let tick = UnitTick {
                desired: Some(S::Dying),
                dead: true,
                biped: true,
                destroyed_after_dying: destroyed,
                airborne,
                ..UnitTick::default()
            };
            let mut all = Vec::new();
            for _ in 0..6 {
                unit.update(&graph, &tick, &mut || 0.5, &mut events);
                all.append(&mut events);
            }
            assert!(all.contains(&expected));
            if expected == UnitEvent::GoLimp {
                assert!(unit.ignore_translation);
                // (it holds its last frames)
                assert_eq!(unit.base.map(|base| base.frame), Some(3));
            }
        }
        // (a vehicle at rest is taken away)
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        assert!(unit.set_state(&graph, S::Dying, &mut || 0.5, &mut events));
        let tick = UnitTick {
            desired: Some(S::Dying),
            dead: true,
            destroyed_after_dying: true,
            at_rest: true,
            ..UnitTick::default()
        };
        events.clear();
        for _ in 0..4 {
            unit.update(&graph, &tick, &mut || 0.5, &mut events);
        }
        assert_eq!(events, [UnitEvent::Destroy]);
        Ok(())
    }

    #[test]
    fn seats_and_doors_end_as_the_world_says() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        for (state, expected) in [
            (S::ExitingSeat, Some(UnitEvent::ExitedSeat)),
            (S::EnteringSeat, Some(UnitEvent::EnteredSeat)),
            (S::Opening, None),
        ] {
            let mut unit = standing(&graph)?;
            unit.state = Some(state);
            unit.base = Some(AnimationState {
                animation: 1,
                frame: 3,
            });
            let mut events = Vec::new();
            unit.update(&graph, &tick(state), &mut || 0.5, &mut events);
            assert_eq!(events.first().copied(), expected);
            if expected.is_none() {
                assert_eq!(unit.base.map(|base| base.frame), Some(3));
            }
        }
        Ok(())
    }

    #[test]
    fn crouching_in_combat_changes_the_seat() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let mut events = Vec::new();
        let tick = UnitTick {
            desired: Some(S::Idle),
            seat: SeatChoice {
                crouching: true,
                ..SeatChoice::default()
            },
            ..UnitTick::default()
        };
        unit.update(&graph, &tick, &mut || 0.5, &mut events);
        assert_eq!(unit.base_seat, Some(3));
        assert_eq!(unit.state, Some(S::Idle));
        assert_eq!(unit.base.map(|base| base.animation), Some(7));
        assert!(events.contains(&UnitEvent::Interpolate(6)));
        // (one riding keeps its seat)
        let mut unit = standing(&graph)?;
        let tick = UnitTick {
            attached: true,
            ..tick
        };
        unit.update(&graph, &tick, &mut || 0.5, &mut events);
        assert_eq!(unit.base_seat, Some(SEAT_STAND));
        Ok(())
    }

    #[test]
    fn the_flinch_action_and_overlay_end() -> Result<(), Box<dyn std::error::Error>> {
        let graph = graph();
        let mut unit = standing(&graph)?;
        let start = Some(AnimationState {
            animation: 12,
            frame: 0,
        });
        unit.soft_ping = start;
        unit.action = start;
        unit.overlay_action = start;
        let mut events = Vec::new();
        for _ in 0..4 {
            unit.update(&graph, &tick(S::Idle), &mut || 0.5, &mut events);
        }
        assert_eq!(unit.soft_ping, None);
        assert_eq!(unit.action, None);
        assert_eq!(unit.overlay_action, None);
        assert!(events.contains(&UnitEvent::Interpolate(6)));
        // (moving forward, an overlay action plays on)
        let mut events = Vec::new();
        assert!(unit.set_state(&graph, S::MoveFront, &mut || 0.5, &mut events));
        unit.overlay_action = start;
        for _ in 0..6 {
            unit.update(&graph, &tick(S::MoveFront), &mut || 0.5, &mut events);
        }
        assert!(unit.overlay_action.is_some());
        Ok(())
    }

    #[test]
    fn the_base_seat_follows_the_mood() -> Result<(), Box<dyn std::error::Error>> {
        let none = SeatChoice::default();
        assert_eq!(base_seat(Mood::Asleep, none), SEAT_ASLEEP);
        assert_eq!(base_seat(Mood::Suspicious, none), SEAT_ALERT);
        assert_eq!(base_seat(Mood::Wary, none), SEAT_STAND);
        assert_eq!(base_seat(Mood::Flee, none), SEAT_FLEE);
        assert_eq!(base_seat(Mood::Flaming, none), SEAT_FLAMING);
        let magic = SeatChoice {
            magic_seat: Some(4),
            ..none
        };
        assert_eq!(base_seat(Mood::Asleep, magic), 4);
        let alert = SeatChoice {
            force_alert: true,
            ..magic
        };
        assert_eq!(base_seat(Mood::Asleep, alert), SEAT_ALERT);
        let burning = SeatChoice {
            flaming: true,
            ..alert
        };
        assert_eq!(base_seat(Mood::Asleep, burning), SEAT_FLAMING);
        Ok(())
    }

    #[test]
    fn a_biped_moves_in_four_directions() -> Result<(), Box<dyn std::error::Error>> {
        let moving = |throttle, stunned| {
            biped_desired_state(&BipedTick {
                throttle,
                stunned,
                ..BipedTick::default()
            })
        };
        assert_eq!(moving([0.0, 0.0, 0.0], false), S::Idle);
        assert_eq!(moving([1.0, 0.5, 0.0], false), S::MoveFront);
        assert_eq!(moving([-1.0, 1.0, 0.0], false), S::MoveBack);
        assert_eq!(moving([0.2, 0.5, 0.0], false), S::MoveLeft);
        assert_eq!(moving([0.2, -0.5, 0.0], false), S::MoveRight);
        assert_eq!(moving([0.0, 0.0, 1.0], false), S::MoveFront);
        assert_eq!(moving([0.2, -0.5, 0.0], true), S::StunnedMoveRight);
        // (a flier is idle; a hard landing holds it still)
        let flier = BipedTick {
            flying: true,
            throttle: [1.0, 0.0, 0.0],
            ..BipedTick::default()
        };
        assert_eq!(biped_desired_state(&flier), S::Idle);
        let landing = BipedTick {
            landing: Some(1),
            throttle: [1.0, 0.0, 0.0],
            ..BipedTick::default()
        };
        assert_eq!(biped_desired_state(&landing), S::LandHard);
        let soft = BipedTick {
            landing: Some(0),
            ..landing
        };
        assert_eq!(biped_desired_state(&soft), S::LandSoft);
        Ok(())
    }

    #[test]
    fn a_biped_turns_dies_and_flies() -> Result<(), Box<dyn std::error::Error>> {
        let turning = BipedTick {
            turn: Some(S::TurnRight),
            ..BipedTick::default()
        };
        assert_eq!(biped_desired_state(&turning), S::TurnRight);
        let dead = BipedTick {
            dead: true,
            ..turning
        };
        assert_eq!(biped_desired_state(&dead), S::Dying);
        let falling = BipedTick {
            airborne_ticks: 3,
            ..dead
        };
        assert_eq!(biped_desired_state(&falling), S::DyingAirborne);
        let no_air = BipedTick {
            no_dying_airborne: true,
            ..falling
        };
        assert_eq!(biped_desired_state(&no_air), S::Dying);
        let limp = BipedTick {
            limp: true,
            ..falling
        };
        assert_eq!(biped_desired_state(&limp), S::Idle);
        // (in the air: airborne after three frames, a leap goes on)
        let jumping = BipedTick {
            airborne: true,
            airborne_ticks: 3,
            throttle: [1.0, 0.0, 0.0],
            ..BipedTick::default()
        };
        assert_eq!(biped_desired_state(&jumping), S::MoveFront);
        let flying = BipedTick {
            airborne_ticks: 4,
            ..jumping
        };
        assert_eq!(biped_desired_state(&flying), S::Airborne);
        let still = BipedTick {
            state: Some(S::Airborne),
            ..jumping
        };
        assert_eq!(biped_desired_state(&still), S::Airborne);
        let leaping = BipedTick {
            state: Some(S::LeapStart),
            ..jumping
        };
        assert_eq!(biped_desired_state(&leaping), S::LeapAirborne);
        Ok(())
    }

    #[test]
    fn a_biped_turns_toward_where_it_is_told() -> Result<(), Box<dyn std::error::Error>> {
        let forward = [1.0, 0.0, 0.0];
        assert_eq!(standing_turn(forward, [1.0, 0.1, 0.0], 0.9), None);
        assert_eq!(
            standing_turn(forward, [0.0, 1.0, 0.0], 0.9),
            Some(S::TurnLeft)
        );
        assert_eq!(
            standing_turn(forward, [0.0, -1.0, 0.0], 0.9),
            Some(S::TurnRight)
        );
        assert_eq!(standing_turn(forward, [0.0, 0.0, 1.0], 0.9), None);
        Ok(())
    }
}
