//! The first-person weapon's states (`first_person_weapons.c`): what each
//! message the weapon sends starts, which state follows each at its
//! animation's end, the shotgun's reloads a shell at a time, and the sway,
//! push-back and idle posing of each tick.
//!
//! The machine knows the weapon only through [`HeldWeapon`]: its type, its
//! heat, its first magazine and its graph's first-person animations, so
//! tests/first_person_reference.rs can run it over the game's own table.

use libm::{fabsf, sqrtf};

use crate::animation::{advance, AnimationState, Update};
use crate::random::RandomSeed;

/// The first-person animations, in the graph's order (the game's
/// `_first_person_weapon_animation_*`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FirstPersonAnimation {
    /// Held.
    Idle = 0,
    /// Shown off while held still.
    Posing,
    /// Fired.
    PrimaryFire,
    /// Carried at a run.
    Moving,
    /// The sway overlays.
    Overlays,
    /// The flashlight turned on.
    LightOn,
    /// The flashlight turned off.
    LightOff,
    /// Reloaded, empty.
    ReloadWhileEmpty,
    /// Reloaded, not empty.
    ReloadWhileFull,
    /// Cooling.
    Overheated,
    /// Drawn.
    Ready,
    /// Put away.
    PutAway,
    /// Charged.
    Overcharged,
    /// Swung.
    Melee,
    /// Fired with the second trigger.
    SecondaryFire,
    /// Shaking while charged.
    OverchargedJitter,
    /// A grenade thrown.
    ThrowGrenade,
    /// The rounds shown, a frame for each round loaded.
    Ammunition,
    /// Fired with no round.
    PrimaryMisfire,
    /// Fired with the second trigger and no round.
    SecondaryMisfire,
    /// A grenade thrown while cooling.
    ThrowGrenadeOverheated,
    /// Starting to cool.
    Overheating,
    /// Drawn while cooling.
    OverheatingAgain,
    /// The shotgun opened for shells.
    ShotgunEnter,
    /// The shotgun closed, pumped.
    ShotgunExitEmpty,
    /// The shotgun closed.
    ShotgunExitFull,
    /// Cooled.
    OverheatedExit,
    /// Overcharged to a supercharge.
    OverheatedSuperchargeEnter,
}

/// What the first-person weapon is doing (the game's
/// `_first_person_weapon_state_*`, in its order).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum FirstPersonState {
    /// Held still.
    #[default]
    Idle = 0,
    /// Starting to cool.
    Overheating,
    /// Drawn while cooling.
    OverheatingAgain,
    /// Cooling.
    Overheated,
    /// Charged.
    Charged,
    /// Shown off while held still.
    Posing,
    /// Firing.
    PrimaryFire,
    /// Firing with the second trigger.
    SecondaryFire,
    /// Firing with no round.
    PrimaryMisfire,
    /// Firing with the second trigger and no round.
    SecondaryMisfire,
    /// Swinging.
    Melee,
    /// Turning the flashlight on.
    LightOn,
    /// Turning the flashlight off.
    LightOff,
    /// Reloading an empty magazine.
    ReloadWhileEmpty,
    /// Reloading one with rounds still in it.
    ReloadWhileFull,
    /// Opening the shotgun for shells.
    ShotgunEnterReload,
    /// Closing an emptied shotgun.
    ShotgunExitReloadEmpty,
    /// Closing the shotgun.
    ShotgunExitReloadFull,
    /// Being put away.
    PutAway,
    /// Being drawn.
    Ready,
    /// Throwing a grenade.
    ThrowGrenade,
    /// Throwing a grenade while cooling.
    ThrowGrenadeOverheated,
    /// Cooled.
    OverheatedExit,
    /// Overcharged to a supercharge.
    OverheatingSuperRecoil,
}

impl FirstPersonState {
    /// Every state, in the game's order.
    pub const ALL: [FirstPersonState; 24] = [
        FirstPersonState::Idle,
        FirstPersonState::Overheating,
        FirstPersonState::OverheatingAgain,
        FirstPersonState::Overheated,
        FirstPersonState::Charged,
        FirstPersonState::Posing,
        FirstPersonState::PrimaryFire,
        FirstPersonState::SecondaryFire,
        FirstPersonState::PrimaryMisfire,
        FirstPersonState::SecondaryMisfire,
        FirstPersonState::Melee,
        FirstPersonState::LightOn,
        FirstPersonState::LightOff,
        FirstPersonState::ReloadWhileEmpty,
        FirstPersonState::ReloadWhileFull,
        FirstPersonState::ShotgunEnterReload,
        FirstPersonState::ShotgunExitReloadEmpty,
        FirstPersonState::ShotgunExitReloadFull,
        FirstPersonState::PutAway,
        FirstPersonState::Ready,
        FirstPersonState::ThrowGrenade,
        FirstPersonState::ThrowGrenadeOverheated,
        FirstPersonState::OverheatedExit,
        FirstPersonState::OverheatingSuperRecoil,
    ];

    /// The state of the game's number.
    pub fn from_index(index: usize) -> Option<Self> {
        Self::ALL.get(index).copied()
    }

    /// Its animation (`first_person_animation_type_from_weapon_state`).
    pub fn animation(self) -> FirstPersonAnimation {
        use FirstPersonAnimation as A;
        match self {
            FirstPersonState::Idle => A::Idle,
            FirstPersonState::Overheating => A::Overheating,
            FirstPersonState::OverheatingAgain => A::OverheatingAgain,
            FirstPersonState::Overheated => A::Overheated,
            FirstPersonState::Charged => A::Overcharged,
            FirstPersonState::Posing => A::Posing,
            FirstPersonState::PrimaryFire => A::PrimaryFire,
            FirstPersonState::SecondaryFire => A::SecondaryFire,
            FirstPersonState::PrimaryMisfire => A::PrimaryMisfire,
            FirstPersonState::SecondaryMisfire => A::SecondaryMisfire,
            FirstPersonState::Melee => A::Melee,
            FirstPersonState::LightOn => A::LightOn,
            FirstPersonState::LightOff => A::LightOff,
            FirstPersonState::ReloadWhileEmpty => A::ReloadWhileEmpty,
            FirstPersonState::ReloadWhileFull => A::ReloadWhileFull,
            FirstPersonState::ShotgunEnterReload => A::ShotgunEnter,
            FirstPersonState::ShotgunExitReloadEmpty => A::ShotgunExitEmpty,
            FirstPersonState::ShotgunExitReloadFull => A::ShotgunExitFull,
            FirstPersonState::PutAway => A::PutAway,
            FirstPersonState::Ready => A::Ready,
            FirstPersonState::ThrowGrenade => A::ThrowGrenade,
            FirstPersonState::ThrowGrenadeOverheated => A::ThrowGrenadeOverheated,
            FirstPersonState::OverheatedExit => A::OverheatedExit,
            FirstPersonState::OverheatingSuperRecoil => A::OverheatedSuperchargeEnter,
        }
    }

    /// Whether firing may start over it (`first_person_weapon_set_state`):
    /// from holding, posing, firing, charging, or the ends and middles of
    /// reloads.
    fn fires_over(self) -> bool {
        matches!(
            self,
            FirstPersonState::Idle
                | FirstPersonState::Posing
                | FirstPersonState::PrimaryFire
                | FirstPersonState::Charged
                | FirstPersonState::ShotgunEnterReload
                | FirstPersonState::OverheatedExit
                | FirstPersonState::ShotgunExitReloadEmpty
                | FirstPersonState::ShotgunExitReloadFull
                | FirstPersonState::ReloadWhileEmpty
                | FirstPersonState::ReloadWhileFull
        )
    }

    /// Whether it is a reload, which a shotgun's next reload message adds a
    /// shell to rather than starts over.
    fn reloading(self) -> bool {
        matches!(
            self,
            FirstPersonState::ShotgunEnterReload
                | FirstPersonState::OverheatedExit
                | FirstPersonState::ShotgunExitReloadEmpty
                | FirstPersonState::ShotgunExitReloadFull
                | FirstPersonState::ReloadWhileEmpty
                | FirstPersonState::ReloadWhileFull
        )
    }

    /// The interpolation frames into it (`first_person_weapon_set_state`):
    /// none into cooling, a swing or a draw, three into firing, six
    /// otherwise.
    fn interpolation_frames(self) -> i16 {
        match self {
            FirstPersonState::Overheated | FirstPersonState::Melee | FirstPersonState::Ready => 0,
            FirstPersonState::PrimaryFire
            | FirstPersonState::SecondaryFire
            | FirstPersonState::PrimaryMisfire
            | FirstPersonState::SecondaryMisfire => 3,
            _ => 6,
        }
    }
}

/// What the weapon tells its first-person view (the game's
/// `_first_person_weapon_message_*`, in its order).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FirstPersonMessage {
    /// Fired.
    PrimaryFire = 0,
    /// Fired with the second trigger.
    SecondaryFire,
    /// Fired with no round.
    PrimaryMisfire,
    /// Fired with the second trigger and no round.
    SecondaryMisfire,
    /// Swung.
    Melee,
    /// The flashlight turned on.
    LightOn,
    /// The flashlight turned off.
    LightOff,
    /// (sent by nothing)
    ShotgunEnterReload,
    /// (sent by nothing)
    ShotgunExitReload,
    /// A reload of an empty magazine begun.
    ReloadWhileEmpty,
    /// A reload begun.
    ReloadWhileFull,
    /// Being put away.
    PutAway,
    /// A weapon drawn.
    Ready,
    /// Dropped.
    Drop,
    /// Charged.
    Charged,
    /// Overheated.
    Overheating,
    /// Overcharged to a supercharge.
    OverheatingSuperRecoil,
    /// A grenade thrown.
    ThrowGrenade,
}

impl FirstPersonMessage {
    /// Every message, in the game's order.
    pub const ALL: [FirstPersonMessage; 18] = [
        FirstPersonMessage::PrimaryFire,
        FirstPersonMessage::SecondaryFire,
        FirstPersonMessage::PrimaryMisfire,
        FirstPersonMessage::SecondaryMisfire,
        FirstPersonMessage::Melee,
        FirstPersonMessage::LightOn,
        FirstPersonMessage::LightOff,
        FirstPersonMessage::ShotgunEnterReload,
        FirstPersonMessage::ShotgunExitReload,
        FirstPersonMessage::ReloadWhileEmpty,
        FirstPersonMessage::ReloadWhileFull,
        FirstPersonMessage::PutAway,
        FirstPersonMessage::Ready,
        FirstPersonMessage::Drop,
        FirstPersonMessage::Charged,
        FirstPersonMessage::Overheating,
        FirstPersonMessage::OverheatingSuperRecoil,
        FirstPersonMessage::ThrowGrenade,
    ];

    /// The message of the game's number.
    pub fn from_index(index: usize) -> Option<Self> {
        Self::ALL.get(index).copied()
    }

    /// The state it starts (`first_person_weapon_state_from_weapon_message`).
    pub fn state(self) -> Option<FirstPersonState> {
        use FirstPersonState as S;
        Some(match self {
            FirstPersonMessage::PrimaryFire => S::PrimaryFire,
            FirstPersonMessage::SecondaryFire => S::SecondaryFire,
            FirstPersonMessage::PrimaryMisfire => S::PrimaryMisfire,
            FirstPersonMessage::SecondaryMisfire => S::SecondaryMisfire,
            FirstPersonMessage::Melee => S::Melee,
            FirstPersonMessage::LightOn => S::LightOn,
            FirstPersonMessage::LightOff => S::LightOff,
            FirstPersonMessage::ReloadWhileEmpty => S::ReloadWhileEmpty,
            FirstPersonMessage::ReloadWhileFull => S::ReloadWhileFull,
            FirstPersonMessage::PutAway => S::PutAway,
            FirstPersonMessage::Ready => S::Ready,
            FirstPersonMessage::Charged => S::Charged,
            FirstPersonMessage::Overheating => S::Overheating,
            FirstPersonMessage::ThrowGrenade => S::ThrowGrenade,
            FirstPersonMessage::OverheatingSuperRecoil => S::OverheatingSuperRecoil,
            FirstPersonMessage::ShotgunEnterReload
            | FirstPersonMessage::ShotgunExitReload
            | FirstPersonMessage::Drop => return None,
        })
    }
}

/// The weapon types the first person tells apart (`_weapon_type_*`).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum WeaponType {
    /// Any other.
    #[default]
    Undefined = 0,
    /// Reloaded a shell at a time.
    Shotgun,
    /// Its rounds shown as it reloads.
    Needler,
    /// Overheated only when it is.
    PlasmaPistol,
    /// The plasma rifle.
    PlasmaRifle,
}

impl WeaponType {
    /// The type of the tag's number; an unknown one is undefined.
    pub fn from_index(index: i16) -> Self {
        match index {
            1 => WeaponType::Shotgun,
            2 => WeaponType::Needler,
            3 => WeaponType::PlasmaPistol,
            4 => WeaponType::PlasmaRifle,
            _ => WeaponType::Undefined,
        }
    }
}

/// How a shotgun's reload goes (`_shotgun_reload_type_*`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ShotgunReload {
    /// Opened, with more shells to come.
    FirstRound = 0,
    /// The last shell, then closed.
    LastRound,
    /// Opened for one shell, then closed.
    FirstAndLastRound,
}

impl ShotgunReload {
    /// The type of the game's number.
    pub fn from_index(index: i32) -> Option<Self> {
        match index {
            0 => Some(ShotgunReload::FirstRound),
            1 => Some(ShotgunReload::LastRound),
            2 => Some(ShotgunReload::FirstAndLastRound),
            _ => None,
        }
    }
}

/// A weapon's first magazine, as a shotgun's reload counts its shells.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct HeldMagazine {
    /// The rounds in it.
    pub loaded: i16,
    /// The rounds carried for it.
    pub total: i16,
    /// The most it holds.
    pub maximum: i16,
    /// Whether it is reloading or chambering.
    pub busy: bool,
}

/// What the machine knows of the weapon it shows.
#[derive(Clone, Copy, Debug, Default)]
pub struct HeldWeapon<'a> {
    /// Its type.
    pub weapon_type: WeaponType,
    /// Whether it is cooling.
    pub overheated: bool,
    /// Whether it has just cooled.
    pub overheated_exit: bool,
    /// Its first magazine.
    pub magazine: Option<HeldMagazine>,
    /// Its graph's first-person block: the graph's animation for each
    /// [`FirstPersonAnimation`], as many as the block has.
    pub animations: &'a [Option<u16>],
}

impl HeldWeapon<'_> {
    /// The graph's animation for `animation`, if the block has it.
    pub fn animation(&self, animation: FirstPersonAnimation) -> Option<u16> {
        self.animations.get(animation as usize).copied().flatten()
    }
}

/// The push-back of a shot (`FIRST_PERSON_WEAPON_FIRING_PUSH_BACK_VELOCITY`).
pub const FIRING_PUSH_BACK_VELOCITY: f32 = 0.05;

/// What a message asks of the view beyond its states.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct MessageEffects {
    /// The weapon dropped: forget it.
    pub forget: bool,
    /// A weapon drawn: the caller switched to it first
    /// ([`FirstPersonMachine::switch_weapons`]).
    pub switch: bool,
    /// A reload: the holder's zoom ends.
    pub unzoom: bool,
    /// The sound the last state started was stopped.
    pub sound_stopped: bool,
}

/// A sound a state started, playing still.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlayingSound {
    /// The state that started it.
    pub state: Option<FirstPersonState>,
}

/// The idle posing's times (the globals' player information).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct IdleTimes {
    /// The fewest seconds held still before a pose.
    pub lower: f32,
    /// The most.
    pub upper: f32,
    /// The part of the poses skipped.
    pub skip_fraction: f32,
}

impl Default for IdleTimes {
    fn default() -> Self {
        // (never shown off)
        IdleTimes {
            lower: 0.0,
            upper: 0.0,
            skip_fraction: 1.0,
        }
    }
}

/// What the holder did this tick, as the view's sway reads it.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Surroundings {
    /// The holder's throttle: forward, left.
    pub throttle: [f32; 2],
    /// The throttle's length, which with the holder on the ground moves
    /// the weapon.
    pub moving: bool,
    /// Whether the view was drawn since the last tick.
    pub rendered: bool,
    /// The facing's change since the last tick: yaw, pitch, in radians.
    pub turned: [f32; 2],
    /// Whether the holder is aiming at something or zoomed, which ends a
    /// pose.
    pub aiming: bool,
    /// The weapon's charge, 0 to 1.
    pub overcharged: f32,
}

/// A first-person weapon's state machine (`struct first_person_weapon`,
/// the parts the states are made of).
#[derive(Clone, Debug, Default, PartialEq)]
pub struct FirstPersonMachine {
    /// What it is doing; none before the first state.
    pub state: Option<FirstPersonState>,
    /// The state's animation in the graph.
    pub animation: Option<u16>,
    /// Its frame.
    pub frame: i16,
    /// The tick of the blend from the last pose into the state's.
    pub interpolation_index: i16,
    /// The blend's ticks; none blends.
    pub interpolation_count: i16,
    /// Whether a blend began since the caller last looked: the caller keeps
    /// the pose it blends from.
    pub interpolation_started: bool,
    /// Whether the shotgun was empty when its reload began.
    pub shotgun_empty: bool,
    /// The shells the shotgun's reload loads.
    pub shotgun_shells: i16,
    /// How the shotgun's reload goes.
    pub shotgun_reload: Option<ShotgunReload>,
    /// The sound the last state started.
    pub sound: Option<PlayingSound>,
    /// How far a shot pushed the weapon back, 0 to 1.
    pub push_back: f32,
    /// Its velocity.
    pub push_back_velocity: f32,
    /// The weapon's lag behind the holder's movement: forward, left.
    pub position: [f32; 2],
    /// Its velocity.
    pub position_velocity: [f32; 2],
    /// The weapon's lag behind the holder's turning: yaw, pitch.
    pub turning: [f32; 2],
    /// Its velocity.
    pub turning_velocity: [f32; 2],
    /// The ticks held still.
    pub ticks_idle: i16,
    /// The ticks still before the next pose; none chosen yet.
    pub ticks_until_pose: i16,
    /// The animation of carrying it at a run.
    pub moving: Option<AnimationState>,
    /// The animation of its charged shaking, and its frame.
    pub jitter: Option<(u16, f32)>,
}

impl FirstPersonMachine {
    /// Begins a blend of `frames` ticks from the pose now
    /// (`first_person_weapon_start_interpolation`), unless a longer one has
    /// further to go.
    pub fn start_interpolation(&mut self, frames: i16) {
        self.interpolation_started = true;
        if frames >= self.interpolation_count - self.interpolation_index {
            self.interpolation_index = 0;
            self.interpolation_count = frames;
        }
    }

    /// Starts `new` if it may start and the weapon has its animation
    /// (`first_person_weapon_set_state`); stopping the last state's sound
    /// if `reset_sounds`. Returns whether a sound was stopped.
    pub fn set_state(
        &mut self,
        weapon: Option<&HeldWeapon>,
        new: FirstPersonState,
        reset_sounds: bool,
    ) -> bool {
        use FirstPersonState as S;
        let mut new = new;
        if weapon.is_some_and(|weapon| weapon.overheated) {
            new = match new {
                S::Ready => S::OverheatingAgain,
                S::ThrowGrenade => S::ThrowGrenadeOverheated,
                other => other,
            };
        }
        let allowed = match new {
            S::Ready => self.state != Some(S::Ready),
            S::LightOff | S::LightOn => matches!(self.state, Some(S::Idle | S::Posing)),
            S::PrimaryFire | S::SecondaryFire | S::PrimaryMisfire | S::SecondaryMisfire => {
                self.state.is_some_and(S::fires_over)
            }
            _ => true,
        };
        let Some(weapon) = weapon.filter(|_| allowed) else {
            return false;
        };
        if weapon.weapon_type == WeaponType::PlasmaPistol
            && new == S::Overheated
            && !weapon.overheated
        {
            new = S::Idle;
        }
        let frames = if weapon.weapon_type == WeaponType::Shotgun
            && self.state == Some(S::ShotgunExitReloadEmpty)
        {
            0
        } else {
            new.interpolation_frames()
        };
        let Some(animation) = weapon.animation(new.animation()) else {
            return false;
        };
        let mut stopped = false;
        if reset_sounds {
            if let Some(sound) = self.sound {
                if sound.state != Some(S::Overheating) {
                    self.sound = None;
                    stopped = true;
                }
            }
        }
        if frames > 0 {
            self.start_interpolation(frames);
        }
        self.state = Some(new);
        self.animation = Some(animation);
        self.frame = 0;
        stopped
    }

    /// Takes up a newly drawn weapon (`first_person_weapon_switch_weapons`,
    /// once the weapon's rig is known): its state from nothing, held.
    /// Returns whether a sound was stopped.
    pub fn switch_weapons(&mut self, weapon: &HeldWeapon) -> bool {
        self.state = None;
        self.animation = None;
        self.moving = None;
        self.jitter = None;
        self.push_back = 0.0;
        self.push_back_velocity = 0.0;
        self.ticks_idle = 0;
        self.sound = None;
        let stopped = self.set_state(Some(weapon), FirstPersonState::Idle, true);
        self.interpolation_count = 0;
        stopped
    }

    /// The weapon's `message` (`first_person_weapon_message`): the state it
    /// starts, and for a shotgun's reload, the shells it loads.
    pub fn message(
        &mut self,
        weapon: Option<&HeldWeapon>,
        message: FirstPersonMessage,
    ) -> MessageEffects {
        use FirstPersonMessage as M;
        let mut effects = MessageEffects::default();
        match message {
            M::Drop => effects.forget = true,
            M::Ready => effects.switch = true,
            M::PrimaryFire => self.push_back_velocity += FIRING_PUSH_BACK_VELOCITY,
            M::ReloadWhileEmpty | M::ReloadWhileFull => effects.unzoom = true,
            _ => {}
        }
        let mut state = None;
        if let Some(magazine) = weapon
            .filter(|weapon| weapon.weapon_type == WeaponType::Shotgun)
            .and_then(|weapon| weapon.magazine)
        {
            if matches!(message, M::ReloadWhileEmpty | M::ReloadWhileFull) {
                let shells = (magazine.maximum - magazine.loaded).min(magazine.total);
                if self.state.is_some_and(FirstPersonState::reloading) || magazine.busy {
                    self.shotgun_reload = (shells == 1).then_some(ShotgunReload::LastRound);
                } else {
                    self.shotgun_shells = shells;
                    self.shotgun_empty = magazine.loaded == 0;
                    self.shotgun_reload = Some(if shells != 1 {
                        ShotgunReload::FirstRound
                    } else {
                        ShotgunReload::FirstAndLastRound
                    });
                }
                state = match self.shotgun_reload {
                    Some(ShotgunReload::FirstRound | ShotgunReload::FirstAndLastRound) => {
                        Some(FirstPersonState::ShotgunEnterReload)
                    }
                    None => Some(FirstPersonState::ReloadWhileEmpty),
                    Some(ShotgunReload::LastRound) => None,
                };
            }
        }
        if let Some(state) = state.or_else(|| message.state()) {
            effects.sound_stopped = self.set_state(weapon, state, true);
        }
        if message == M::Ready {
            self.interpolation_count = 0;
        }
        effects
    }

    /// The state after the last one's animation ends
    /// (`first_person_weapon_next_state`): holding, cooling, the shotgun's
    /// closing, or for a weapon put away, its last frame held.
    pub fn next_state(&mut self, weapon: Option<&HeldWeapon>) {
        use FirstPersonState as S;
        let shotgun = weapon.is_some_and(|weapon| weapon.weapon_type == WeaponType::Shotgun);
        let closed = if self.shotgun_empty {
            S::ShotgunExitReloadEmpty
        } else {
            S::ShotgunExitReloadFull
        };
        let new = match self.state {
            Some(
                S::Idle
                | S::Posing
                | S::PrimaryFire
                | S::SecondaryFire
                | S::PrimaryMisfire
                | S::SecondaryMisfire
                | S::Melee
                | S::LightOff
                | S::LightOn
                | S::ShotgunExitReloadEmpty
                | S::ShotgunExitReloadFull
                | S::Ready
                | S::ThrowGrenade
                | S::OverheatedExit,
            ) => Some(S::Idle),
            Some(
                S::Overheating
                | S::OverheatingAgain
                | S::ThrowGrenadeOverheated
                | S::OverheatingSuperRecoil,
            ) => Some(S::Overheated),
            Some(S::PutAway) => {
                self.frame -= 1;
                None
            }
            Some(S::ShotgunEnterReload) => Some(
                if shotgun && self.shotgun_reload == Some(ShotgunReload::FirstAndLastRound) {
                    closed
                } else {
                    S::Idle
                },
            ),
            Some(S::ReloadWhileEmpty | S::ReloadWhileFull) => {
                Some(match self.shotgun_reload.filter(|_| shotgun) {
                    None | Some(ShotgunReload::FirstRound) => S::Idle,
                    Some(_) => closed,
                })
            }
            Some(S::Overheated | S::Charged) | None => None,
        };
        if let Some(new) = new {
            self.set_state(weapon, new, false);
        }
    }

    /// The animation of the state, at its frame, to pose.
    pub fn animation_state(&self) -> Option<AnimationState> {
        Some(AnimationState {
            animation: self.animation?,
            frame: u16::try_from(self.frame).unwrap_or(0),
        })
    }

    /// One tick (`first_person_weapon_update`): cooling ends, the state's
    /// animation plays on (and at its end, the next state starts), the run
    /// and the charge play theirs, the weapon sways after the holder, a
    /// shot's push-back settles, the blend goes on, and a weapon held still
    /// long enough is shown off.
    pub fn update(
        &mut self,
        weapon: &HeldWeapon,
        animations: &[opence_content::Animation],
        surroundings: &Surroundings,
        idle: &IdleTimes,
        random: &mut RandomSeed,
    ) {
        use FirstPersonState as S;
        if matches!(self.state, Some(S::Overheated | S::Overheating)) {
            if weapon.overheated_exit {
                self.set_state(Some(weapon), S::OverheatedExit, true);
            }
            if !weapon.overheated {
                self.set_state(Some(weapon), S::Idle, true);
            }
        }

        if let Some(mut state) = self.animation_state() {
            let update = advance(animations, &mut state, || random.next_real());
            self.animation = Some(state.animation);
            self.frame = i16::try_from(state.frame).unwrap_or(i16::MAX);
            if update == Update::WillRestartOnNextFrame {
                self.next_state(Some(weapon));
            }
        }

        if let Some(mut moving) = self.moving {
            advance(animations, &mut moving, || random.next_real());
            self.moving = Some(moving);
            if !surroundings.moving {
                if self.state == Some(S::Idle) {
                    self.start_interpolation(6);
                }
                self.moving = None;
            }
        } else if surroundings.moving {
            self.moving = weapon
                .animation(FirstPersonAnimation::Moving)
                .map(|animation| AnimationState {
                    animation,
                    frame: 0,
                });
        }

        if self.state == Some(S::Charged) {
            self.jitter = match self.jitter {
                None => weapon
                    .animation(FirstPersonAnimation::OverchargedJitter)
                    .map(|animation| (animation, 0.0)),
                Some((animation, frame)) => {
                    let count = animations
                        .get(usize::from(animation))
                        .map_or(1.0, |animation| f32::from(animation.frame_count));
                    let frame = libm::fmodf((surroundings.overcharged + 1.0) * 2.0 + frame, count);
                    Some((animation, frame))
                }
            };
        } else if self.jitter.is_some() {
            self.jitter = None;
        }

        if surroundings.rendered {
            for axis in 0..2 {
                accelerate_to_position(
                    &mut self.position[axis],
                    &mut self.position_velocity[axis],
                    surroundings.throttle[axis],
                    0.08,
                    0.5,
                    -1.0,
                    1.0,
                );
            }
            let turning = [
                (surroundings.turned[0] * 30.0).clamp(-1.0, 1.0),
                (surroundings.turned[1] * -30.0).clamp(-1.0, 1.0),
            ];
            for (axis, target) in turning.into_iter().enumerate() {
                accelerate_to_position(
                    &mut self.turning[axis],
                    &mut self.turning_velocity[axis],
                    target,
                    0.03,
                    0.2,
                    -1.0,
                    1.0,
                );
            }
        }

        accelerate_to_position(
            &mut self.push_back,
            &mut self.push_back_velocity,
            0.0,
            0.01,
            0.2,
            0.0,
            1.0,
        );
        if self.push_back == 1.0 {
            self.push_back_velocity = 0.0;
        }

        if self.interpolation_count > 0 {
            self.interpolation_index += 1;
            if self.interpolation_index >= self.interpolation_count {
                self.interpolation_count = 0;
            }
        }

        let swaying = self.push_back != 0.0
            || self.position.iter().any(|&lag| lag != 0.0)
            || self.turning.iter().any(|&lag| lag != 0.0);
        if surroundings.aiming || swaying {
            self.ticks_idle = 0;
            if self.state == Some(S::Posing) {
                self.set_state(Some(weapon), S::Idle, true);
            }
        } else if self.state == Some(S::Idle) {
            if self.ticks_until_pose == 0 {
                let seconds = idle.lower + random.next_real() * (idle.upper - idle.lower);
                self.ticks_until_pose = (seconds * crate::clock::TICKS_PER_SECOND as f32) as i16;
            }
            self.ticks_idle = self.ticks_idle.saturating_add(1);
            if self.ticks_idle > self.ticks_until_pose {
                self.ticks_until_pose = 0;
                if random.next_real() >= idle.skip_fraction {
                    self.set_state(Some(weapon), S::Posing, true);
                }
            }
        } else {
            self.ticks_idle = 0;
        }
    }
}

/// Moves `position` toward `target` at no more than `maximum_velocity` a
/// tick, speeding up or braking by `acceleration`, within `minimum` and
/// `maximum` (`accelerate_to_position`, not periodic). Returns whether it
/// arrived.
pub fn accelerate_to_position(
    position: &mut f32,
    velocity: &mut f32,
    target: f32,
    maximum_velocity: f32,
    acceleration: f32,
    minimum: f32,
    maximum: f32,
) -> bool {
    let delta = target - *position;
    let limit = maximum_velocity.min(acceleration);
    if fabsf(delta - *velocity) <= limit {
        *velocity = 0.0;
        *position = target.clamp(minimum, maximum);
        return true;
    }
    let braking = (maximum_velocity + maximum_velocity) * fabsf(delta);
    let mut speed = if braking >= acceleration * acceleration {
        acceleration
    } else {
        sqrtf(braking)
    };
    if delta < 0.0 {
        speed = -speed;
    }
    let step = (speed - *velocity).clamp(-maximum_velocity, maximum_velocity);
    *velocity += step;
    *position = (step * 0.5 + *velocity + *position).clamp(minimum, maximum);
    false
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_push_back_of_a_shot_settles_to_nothing() {
        let (mut position, mut velocity) = (0.0, FIRING_PUSH_BACK_VELOCITY);
        let mut ticks = 0;
        while !accelerate_to_position(&mut position, &mut velocity, 0.0, 0.01, 0.2, 0.0, 1.0) {
            ticks += 1;
            assert!(ticks < 100);
        }
        assert_eq!((position, velocity), (0.0, 0.0));
    }

    #[test]
    fn an_arrival_is_clamped_to_the_range() {
        let (mut position, mut velocity) = (0.9, 0.0);
        assert!(accelerate_to_position(
            &mut position,
            &mut velocity,
            1.2,
            0.5,
            0.5,
            -1.0,
            1.0
        ));
        assert_eq!(position, 1.0);
    }

    #[test]
    fn a_weapon_held_still_poses_and_a_turn_ends_it() {
        let animations: Vec<Option<u16>> = (0..28).map(Some).collect();
        let weapon = HeldWeapon {
            animations: &animations,
            ..HeldWeapon::default()
        };
        let mut machine = FirstPersonMachine::default();
        machine.switch_weapons(&weapon);
        assert_eq!(machine.state, Some(FirstPersonState::Idle));
        let idle = IdleTimes {
            lower: 0.1,
            upper: 0.2,
            skip_fraction: 0.0,
        };
        let mut random = RandomSeed(1);
        let still = Surroundings {
            rendered: true,
            ..Surroundings::default()
        };
        for _ in 0..30 {
            machine.update(&weapon, &[], &still, &idle, &mut random);
        }
        assert_eq!(machine.state, Some(FirstPersonState::Posing));
        let turned = Surroundings {
            turned: [0.1, 0.0],
            ..still
        };
        machine.update(&weapon, &[], &turned, &idle, &mut random);
        assert_eq!(machine.state, Some(FirstPersonState::Idle));
        assert_eq!(machine.ticks_idle, 0);
    }

    #[test]
    fn a_charge_shakes_and_cooling_ends_when_the_weapon_cools() {
        let animations: Vec<Option<u16>> = (0..28).map(Some).collect();
        let mut weapon = HeldWeapon {
            weapon_type: WeaponType::PlasmaRifle,
            overheated: true,
            animations: &animations,
            ..HeldWeapon::default()
        };
        let idle = IdleTimes {
            lower: 1.0,
            upper: 2.0,
            skip_fraction: 1.0,
        };
        let mut random = RandomSeed(1);
        let mut machine = FirstPersonMachine::default();
        machine.switch_weapons(&weapon);
        machine.message(Some(&weapon), FirstPersonMessage::Charged);
        let charged = Surroundings {
            overcharged: 0.5,
            ..Surroundings::default()
        };
        machine.update(&weapon, &[], &charged, &idle, &mut random);
        assert_eq!(machine.jitter, Some((15, 0.0)));
        machine.update(&weapon, &[], &charged, &idle, &mut random);
        assert_eq!(machine.jitter, Some((15, 0.0)));
        machine.message(Some(&weapon), FirstPersonMessage::Overheating);
        machine.update(&weapon, &[], &charged, &idle, &mut random);
        assert_eq!(machine.state, Some(FirstPersonState::Overheating));
        assert_eq!(machine.jitter, None);
        weapon.overheated = false;
        machine.update(&weapon, &[], &charged, &idle, &mut random);
        assert_eq!(machine.state, Some(FirstPersonState::Idle));
        // (a run plays the moving animation, and stopping ends it)
        let running = Surroundings {
            moving: true,
            ..Surroundings::default()
        };
        machine.update(&weapon, &[], &running, &idle, &mut random);
        assert_eq!(machine.moving.map(|moving| moving.animation), Some(3));
        machine.update(&weapon, &[], &running, &idle, &mut random);
        machine.update(&weapon, &[], &Surroundings::default(), &idle, &mut random);
        assert_eq!(machine.moving, None);
    }
}
