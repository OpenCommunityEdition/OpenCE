//! A weapon's triggers, magazines and heat, stepped a tick at a time as the
//! original's `weapon_update` steps them (source/items/weapons.c), at the
//! engine's 60 ticks a second.
//!
//! The tags give rates and times per second, and the original turned them
//! into its own 30 ticks; here they become 60, so a weapon fires as many
//! rounds a second, reloads in as many seconds and cools as fast as it did.
//! This is the logic only: what a shot makes (its projectiles, its effects)
//! is for whoever steps the weapon, from the [`Shot`]s it returns.

use opence_content::{Magazine, Trigger, Weapon};

use crate::clock::TICKS_PER_SECOND;
use crate::hash::WorldHasher;

const TPS: f32 = TICKS_PER_SECOND as f32;

/// The most triggers a weapon has.
pub const MAXIMUM_TRIGGERS: usize = 2;
/// The most magazines a weapon has.
pub const MAXIMUM_MAGAZINES: usize = 2;

/// Seconds as whole ticks, cut short as the original's `(short)` cut them.
fn ticks(seconds: f32) -> i16 {
    // (`as` saturates, and takes what is not a number to 0)
    (seconds * TPS) as i16
}

/// What a tick adds to a fraction that goes from 0 to 1 in `seconds`: all
/// of it at once for none.
fn per_tick(seconds: f32) -> f32 {
    if seconds > 0.0 {
        1.0 / (seconds * TPS)
    } else {
        1.0
    }
}

/// The secondary trigger's modes (`secondary_trigger_mode`).
pub mod secondary {
    /// Fires only when the primary does not (fully fires at its rate).
    pub const SLAVED_TO_PRIMARY: i16 = 1;
    /// Held, it stops the primary.
    pub const INHIBITS_PRIMARY: i16 = 2;
}

/// What a trigger is doing.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum TriggerPhase {
    /// Ready to fire, when its rate allows.
    #[default]
    Idle,
    /// Firing every tick it can, for its spew time.
    Spewing,
    /// Held, charging.
    Charging,
    /// Charged, waiting to be let go.
    Charged,
    /// Empty, or latched: waiting to be let go.
    Locked,
}

/// A trigger's state.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct TriggerState {
    /// What it is doing.
    pub phase: TriggerPhase,
    /// The ticks left in its phase.
    pub timer: i16,
    /// The ticks since it last fired (to 127).
    pub idle_ticks: i16,
    /// How far from its initial rate of fire to its final, 0 to 1.
    pub rate_of_fire: f32,
    /// How far from its initial error to its final, 0 to 1.
    pub error: f32,
    /// Whether it has been let go since it last fired.
    pub released: bool,
}

/// What a magazine is doing.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum MagazinePhase {
    /// Ready.
    #[default]
    Idle,
    /// Reloading.
    Reloading,
    /// Reloaded or fired, its next round not yet chambered.
    Unchambered,
    /// Chambering a round.
    Chambering,
}

/// A magazine's state.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct MagazineState {
    /// What it is doing.
    pub phase: MagazinePhase,
    /// The ticks left in its phase.
    pub timer: i16,
    /// The rounds in it.
    pub loaded: i16,
    /// The rounds carried for it, not in it.
    pub total: i16,
    /// The part of a round recharged so far.
    pub recharge: f32,
}

/// What the weapon's holder does with it this tick.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct WeaponControls {
    /// Each trigger held.
    pub triggers: [bool; MAXIMUM_TRIGGERS],
    /// The reload asked for.
    pub reload: bool,
}

/// A round fired.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Shot {
    /// The trigger that fired it.
    pub trigger: usize,
    /// Its trigger's error then, 0 to 1: the spread is the trigger's
    /// projectile error angle from its first to its second at this.
    pub error: f32,
}

/// Why a weapon's state is not its tag's.
#[derive(Clone, Copy, Debug, PartialEq, Eq, thiserror::Error)]
pub enum WeaponError {
    /// The tag has more triggers than a weapon has.
    #[error("{0} triggers, more than {MAXIMUM_TRIGGERS}")]
    TooManyTriggers(usize),
}

/// The effects of a weapon's handling (`weapon_effect_new`) due since
/// they were last taken: its readying, and each magazine's reloading (each
/// round of a one-at-a-time reload) and chambering.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Cues {
    /// It was readied.
    pub ready: bool,
    /// Each magazine began a reload.
    pub reloading: [bool; MAXIMUM_MAGAZINES],
    /// Each magazine began chambering.
    pub chambering: [bool; MAXIMUM_MAGAZINES],
}

/// A weapon's state.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct WeaponState {
    /// Its heat, 0 to 1.
    pub heat: f32,
    /// Whether it is cooling from overheating.
    pub overheated: bool,
    /// Its age (a plasma weapon's spent battery), 0 to 1.
    pub age: f32,
    /// The ticks until it is ready (drawn).
    pub ready: i16,
    /// Each trigger's state.
    pub triggers: Vec<TriggerState>,
    /// Each magazine's state.
    pub magazines: Vec<MagazineState>,
    /// The handling effects due (not part of its hash: they are taken the
    /// tick they are made).
    pub cues: Cues,
}

impl WeaponState {
    /// A weapon as it is made (`weapon_new`): full magazines, the rest
    /// carried, cool, new, and readying.
    pub fn new(weapon: &Weapon) -> Result<WeaponState, WeaponError> {
        if weapon.triggers.len() > MAXIMUM_TRIGGERS {
            return Err(WeaponError::TooManyTriggers(weapon.triggers.len()));
        }
        Ok(WeaponState {
            heat: 0.0,
            overheated: false,
            age: 0.0,
            ready: ticks(weapon.ready_time),
            cues: Cues::default(),
            triggers: vec![
                TriggerState {
                    released: true,
                    ..TriggerState::default()
                };
                weapon.triggers.len()
            ],
            magazines: weapon
                .magazines
                .iter()
                .map(|magazine| {
                    let loaded = magazine
                        .rounds_total_initial
                        .min(magazine.rounds_loaded_maximum)
                        .max(0);
                    MagazineState {
                        loaded,
                        total: (magazine.rounds_total_initial - loaded).max(0),
                        ..MagazineState::default()
                    }
                })
                .collect(),
        })
    }

    /// Readies it again (drawn after being put away).
    pub fn draw(&mut self, weapon: &Weapon) {
        self.ready = ticks(weapon.ready_time);
        self.cues.ready = true;
    }

    /// The handling effects due, which are then no longer due.
    pub fn take_cues(&mut self) -> Cues {
        std::mem::take(&mut self.cues)
    }

    /// Whether anything stops it firing now (it is still being drawn).
    pub fn busy(&self) -> bool {
        self.ready > 0
    }

    /// Starts reloading every magazine that can (`weapon_magazine_start_reload`).
    pub fn reload(&mut self, weapon: &Weapon) {
        for index in 0..self.magazines.len() {
            self.start_reload(weapon, index);
        }
    }

    fn start_reload(&mut self, weapon: &Weapon, index: usize) {
        let (Some(state), Some(magazine)) =
            (self.magazines.get_mut(index), weapon.magazines.get(index))
        else {
            return;
        };
        if matches!(
            state.phase,
            MagazinePhase::Idle | MagazinePhase::Unchambered
        ) && state.total > 0
            && state.loaded < magazine.rounds_loaded_maximum
        {
            state.phase = MagazinePhase::Reloading;
            state.timer = ticks(magazine.reload_time);
            cue(&mut self.cues.reloading, index);
        }
    }

    /// One tick (`weapon_update`): the heat, the magazines, then the
    /// triggers; the rounds it fired.
    pub fn step(&mut self, weapon: &Weapon, controls: WeaponControls) -> Vec<Shot> {
        // the heat
        if weapon.heat_overheated_threshold > 0.0 && self.heat >= weapon.heat_overheated_threshold {
            self.overheated = true;
        }
        self.heat = (self.heat - weapon.heat_loss_per_second / TPS).max(0.0);
        if self.overheated && self.heat < weapon.heat_recovery_threshold {
            self.overheated = false;
        }
        if self.ready > 0 {
            self.ready -= 1;
        }
        let ready = !self.busy();
        let mut down = [false; MAXIMUM_TRIGGERS];
        for (index, held) in down.iter_mut().enumerate() {
            *held = ready && controls.triggers[index] && index < self.triggers.len();
        }
        // the secondary trigger's mode
        if self.triggers.len() > 1 {
            match weapon.secondary_trigger_mode {
                secondary::SLAVED_TO_PRIMARY if self.triggers[0].rate_of_fire < 1.0 => {
                    down[1] = false;
                }
                secondary::INHIBITS_PRIMARY if down[1] => down[0] = false,
                _ => {}
            }
        }
        if controls.reload && ready {
            self.reload(weapon);
        }
        for index in 0..self.magazines.len() {
            self.step_magazine(weapon, index);
        }
        let mut shots = Vec::new();
        for (index, down) in down.into_iter().enumerate().take(self.triggers.len()) {
            self.step_trigger(weapon, index, down, &mut shots);
        }
        shots
    }

    fn step_magazine(&mut self, weapon: &Weapon, index: usize) {
        let (Some(state), Some(magazine)) =
            (self.magazines.get_mut(index), weapon.magazines.get(index))
        else {
            return;
        };
        if magazine.rounds_recharged_per_second > 0 {
            state.recharge += f32::from(magazine.rounds_recharged_per_second) / TPS;
            let whole = libm::floorf(state.recharge);
            state.recharge -= whole;
            state.loaded = state
                .loaded
                .saturating_add(whole as i16)
                .min(magazine.rounds_loaded_maximum);
        }
        if state.timer > 0 {
            state.timer -= 1;
        }
        match state.phase {
            MagazinePhase::Reloading if state.timer <= 0 => {
                if finish_reload(state, magazine) {
                    cue(&mut self.cues.reloading, index);
                }
            }
            MagazinePhase::Unchambered => {
                state.timer = ticks(magazine.chamber_time);
                state.phase = if state.timer > 0 {
                    cue(&mut self.cues.chambering, index);
                    MagazinePhase::Chambering
                } else {
                    MagazinePhase::Idle
                };
            }
            MagazinePhase::Chambering if state.timer <= 0 => state.phase = MagazinePhase::Idle,
            _ => {}
        }
    }

    fn step_trigger(&mut self, weapon: &Weapon, index: usize, down: bool, shots: &mut Vec<Shot>) {
        let Some(trigger) = weapon.triggers.get(index) else {
            return;
        };
        let magazine = trigger
            .magazine
            .map(usize::from)
            .filter(|&magazine| magazine < self.magazines.len());
        if let Some(state) = self.triggers.get_mut(index) {
            if state.timer > 0 {
                state.timer -= 1;
            }
            if !down {
                state.released = true;
            }
        }
        let phase = self.triggers[index].phase;
        match phase {
            TriggerPhase::Idle => {
                if let Some(magazine) = magazine {
                    let loaded = self.magazines[magazine].loaded;
                    let partial = trigger.flags & Trigger::CAN_FIRE_WITH_PARTIAL_AMMUNITION != 0;
                    if (loaded < trigger.rounds_per_shot && !partial)
                        || loaded < trigger.minimum_rounds_loaded
                        || loaded == 0
                    {
                        self.start_reload(weapon, magazine);
                    }
                }
                if down && self.can_fire_again(weapon, index) {
                    self.begin_firing(weapon, index, magazine, shots);
                } else {
                    let state = &mut self.triggers[index];
                    state.idle_ticks = (state.idle_ticks + 1).min(127);
                }
            }
            TriggerPhase::Spewing => {
                if self.triggers[index].timer > 0 {
                    if self.can_fire_again(weapon, index) {
                        self.fire(weapon, index, magazine, shots);
                    } else {
                        let state = &mut self.triggers[index];
                        state.idle_ticks = (state.idle_ticks + 1).min(127);
                    }
                } else {
                    self.recover(index);
                }
            }
            TriggerPhase::Charging => {
                if self.triggers[index].timer > 0 {
                    if !down {
                        self.triggers[index].phase = TriggerPhase::Idle;
                    }
                } else {
                    let state = &mut self.triggers[index];
                    state.phase = TriggerPhase::Charged;
                    state.timer = ticks(trigger.charged_time);
                }
            }
            TriggerPhase::Charged => {
                let empty = magazine.is_some_and(|magazine| {
                    self.magazines[magazine].loaded < trigger.rounds_per_shot
                        && trigger.flags & Trigger::CAN_FIRE_WITH_PARTIAL_AMMUNITION == 0
                });
                // (a charge held past its time is let go too: the
                // overcharged actions are not modelled)
                if !down || empty || self.triggers[index].timer <= 0 {
                    self.release_charge(weapon, index, magazine, shots);
                }
            }
            TriggerPhase::Locked => {
                if !down {
                    self.triggers[index].phase = TriggerPhase::Idle;
                }
            }
        }
        // the rate of fire and the error
        let Some(state) = self.triggers.get_mut(index) else {
            return;
        };
        if down {
            state.rate_of_fire =
                (state.rate_of_fire + per_tick(trigger.rate_of_fire_acceleration_time)).min(1.0);
        } else {
            state.rate_of_fire =
                (state.rate_of_fire - per_tick(trigger.rate_of_fire_deceleration_time)).max(0.0);
        }
        if state.phase == TriggerPhase::Spewing || down {
            state.error = (state.error + per_tick(trigger.error_acceleration_time)).min(1.0);
        } else {
            state.error = (state.error - per_tick(trigger.error_deceleration_time)).max(0.0);
        }
    }

    /// Whether its rate of fire lets a trigger fire again
    /// (`weapon_trigger_can_fire_again`).
    fn can_fire_again(&self, weapon: &Weapon, index: usize) -> bool {
        let (Some(state), Some(trigger)) = (self.triggers.get(index), weapon.triggers.get(index))
        else {
            return false;
        };
        if trigger.flags & Trigger::LATCHED != 0 && !state.released {
            return false;
        }
        let rate = trigger.initial_rate_of_fire
            + (trigger.final_rate_of_fire - trigger.initial_rate_of_fire) * state.rate_of_fire;
        let mut required = if rate > 0.0001 { TPS / rate } else { 0.0 };
        required *= self.age * weapon.age_rate_of_fire_penalty + 1.0;
        f32::from(state.idle_ticks) + 1.0 >= required
    }

    /// Pulls a trigger (`weapon_trigger_begin_firing`): it charges, or
    /// fires.
    fn begin_firing(
        &mut self,
        weapon: &Weapon,
        index: usize,
        magazine: Option<usize>,
        shots: &mut Vec<Shot>,
    ) {
        let Some(trigger) = weapon.triggers.get(index) else {
            return;
        };
        if magazine.is_some_and(|magazine| self.magazines[magazine].phase != MagazinePhase::Idle)
            || self.overheated
        {
            return;
        }
        if trigger.charging_time > 0.0 {
            let state = &mut self.triggers[index];
            state.phase = TriggerPhase::Charging;
            state.timer = ticks(trigger.charging_time);
        } else {
            self.fire(weapon, index, magazine, shots);
        }
    }

    /// Lets go of a charge (`weapon_trigger_release_charge`): it spews, or
    /// fires once.
    fn release_charge(
        &mut self,
        weapon: &Weapon,
        index: usize,
        magazine: Option<usize>,
        shots: &mut Vec<Shot>,
    ) {
        let Some(trigger) = weapon.triggers.get(index) else {
            return;
        };
        if trigger.spew_time > 0.0 {
            let state = &mut self.triggers[index];
            state.phase = TriggerPhase::Spewing;
            state.timer = ticks(trigger.spew_time);
        } else {
            self.fire(weapon, index, magazine, shots);
        }
        self.triggers[index].rate_of_fire = 0.0;
    }

    /// Back to idle, its count of ticks since firing started again
    /// (`weapon_trigger_recover`).
    fn recover(&mut self, index: usize) {
        let state = &mut self.triggers[index];
        state.idle_ticks = 0;
        state.phase = TriggerPhase::Idle;
    }

    /// Fires a trigger (`weapon_trigger_fire`): takes its rounds, heats
    /// and ages the weapon, and recovers; an empty one clicks and locks.
    fn fire(
        &mut self,
        weapon: &Weapon,
        index: usize,
        magazine: Option<usize>,
        shots: &mut Vec<Shot>,
    ) {
        let Some(trigger) = weapon.triggers.get(index) else {
            return;
        };
        let too_old = weapon.flags & Weapon::CANNOT_FIRE_AT_MAXIMUM_AGE != 0 && self.age >= 1.0;
        let fired = match magazine {
            Some(magazine) => {
                let released = self.triggers[index].released;
                let partial = trigger.flags & Trigger::CAN_FIRE_WITH_PARTIAL_AMMUNITION != 0;
                let state = &mut self.magazines[magazine];
                let fires = (state.loaded >= trigger.rounds_per_shot || partial)
                    && !too_old
                    && (state.loaded >= trigger.minimum_rounds_loaded || !released);
                if fires {
                    state.loaded -= trigger.rounds_per_shot;
                    if state.loaded <= 0 {
                        state.loaded = 0;
                    } else if weapon.magazines[magazine].flags
                        & Magazine::MUST_BE_CHAMBERED_EVERY_SHOT
                        != 0
                    {
                        state.phase = MagazinePhase::Unchambered;
                        state.timer = 0;
                    }
                }
                fires
            }
            None => !too_old,
        };
        if fired {
            // (the player's weapon: heat to 1, and it ages)
            self.heat = (self.heat + trigger.heat_generated_per_round).min(1.0);
            self.age = (self.age + trigger.age_generated_per_round).min(1.0);
            shots.push(Shot {
                trigger: index,
                error: self.triggers[index].error,
            });
        }
        let state = &mut self.triggers[index];
        if !fired {
            state.phase = TriggerPhase::Locked;
            state.timer = 0;
        } else if state.phase != TriggerPhase::Spewing {
            self.recover(index);
        }
        self.triggers[index].released = false;
    }

    /// Its state, for the world's hash.
    pub(crate) fn hash(&self, hasher: &mut WorldHasher) {
        for value in [self.heat, self.age] {
            hasher.f32(value);
        }
        hasher.u64(u64::from(self.overheated));
        hasher.u64(self.ready as u64);
        for trigger in &self.triggers {
            hasher.u64(trigger.phase as u64);
            hasher.u64(trigger.timer as u64);
            hasher.u64(trigger.idle_ticks as u64);
            hasher.f32(trigger.rate_of_fire);
            hasher.f32(trigger.error);
            hasher.u64(u64::from(trigger.released));
        }
        for magazine in &self.magazines {
            hasher.u64(magazine.phase as u64);
            hasher.u64(magazine.timer as u64);
            hasher.u64(magazine.loaded as u64);
            hasher.u64(magazine.total as u64);
            hasher.f32(magazine.recharge);
        }
    }
}

/// Marks magazine `index`'s cue in `cues`.
fn cue(cues: &mut [bool; MAXIMUM_MAGAZINES], index: usize) {
    if let Some(cue) = cues.get_mut(index) {
        *cue = true;
    }
}

/// A reload done (`weapon_magazine_finish_reload`): the rounds move from
/// those carried into the magazine, and one at a time (a shotgun) it goes
/// on; whether it went on (a new reload, with its own effect).
fn finish_reload(state: &mut MagazineState, magazine: &Magazine) -> bool {
    if magazine.flags & Magazine::WASTES_ROUNDS_WHEN_RELOADED != 0 {
        state.loaded = 0;
    }
    let load = magazine.rounds_reloaded.min(state.total).max(0);
    let before = state.loaded;
    state.loaded = (state.loaded + load).min(magazine.rounds_loaded_maximum);
    state.total -= state.loaded - before;
    state.phase = MagazinePhase::Unchambered;
    state.timer = 0;
    if state.total > 0
        && state.loaded < magazine.rounds_loaded_maximum
        && magazine.flags & Magazine::WASTES_ROUNDS_WHEN_RELOADED == 0
    {
        state.phase = MagazinePhase::Reloading;
        state.timer = ticks(magazine.reload_time);
        return true;
    }
    false
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A weapon like the assault rifle: 15 rounds a second, 60 in a
    /// magazine and 240 carried, reloaded in 2 seconds.
    fn rifle() -> Weapon {
        Weapon {
            heat_recovery_threshold: 0.0,
            heat_overheated_threshold: 1.0,
            ready_time: 0.5,
            magazines: vec![Magazine {
                rounds_total_initial: 300,
                rounds_total_maximum: 600,
                rounds_loaded_maximum: 60,
                reload_time: 2.0,
                rounds_reloaded: 60,
                ..Magazine::default()
            }],
            triggers: vec![Trigger {
                initial_rate_of_fire: 15.0,
                final_rate_of_fire: 15.0,
                magazine: Some(0),
                rounds_per_shot: 1,
                minimum_rounds_loaded: 1,
                initial_error: 0.0,
                final_error: 1.0,
                error_acceleration_time: 1.0,
                error_deceleration_time: 0.5,
                projectiles_per_shot: 1,
                ..Trigger::default()
            }],
            ..Weapon::default()
        }
    }

    fn fire(held: bool) -> WeaponControls {
        WeaponControls {
            triggers: [held, false],
            reload: false,
        }
    }

    fn state(weapon: &Weapon) -> WeaponState {
        match WeaponState::new(weapon) {
            Ok(state) => state,
            Err(error) => panic!("{error}"),
        }
    }

    /// Steps `ticks` ticks of `controls`, counting the shots.
    fn run(
        state: &mut WeaponState,
        weapon: &Weapon,
        controls: WeaponControls,
        ticks: u32,
    ) -> usize {
        (0..ticks).map(|_| state.step(weapon, controls).len()).sum()
    }

    #[test]
    fn a_new_weapon_is_loaded_and_readies_first() {
        let weapon = rifle();
        let mut state = state(&weapon);
        assert_eq!(
            (state.magazines[0].loaded, state.magazines[0].total),
            (60, 240)
        );
        // half a second to draw: nothing fires before it
        assert_eq!(run(&mut state, &weapon, fire(true), 29), 0);
        assert!(run(&mut state, &weapon, fire(true), 2) > 0);
    }

    #[test]
    fn it_fires_at_its_rate_of_fire() {
        let weapon = rifle();
        let mut state = state(&weapon);
        run(&mut state, &weapon, fire(false), 30);
        // 15 rounds a second, for two seconds' worth of a 60-round magazine
        assert_eq!(run(&mut state, &weapon, fire(true), 60), 15);
        assert_eq!(state.magazines[0].loaded, 45);
        // its rate is the same at any tick rate: a round every 4 ticks
        let mut gaps = Vec::new();
        let mut last = None;
        for tick in 0..20 {
            if !state.step(&weapon, fire(true)).is_empty() {
                if let Some(last) = last {
                    gaps.push(tick - last);
                }
                last = Some(tick);
            }
        }
        assert!(gaps.iter().all(|&gap| gap == 4), "{gaps:?}");
    }

    #[test]
    fn an_empty_magazine_reloads_in_its_reload_time() {
        let weapon = rifle();
        let mut state = state(&weapon);
        state.ready = 0;
        // 60 rounds: four seconds at 15
        assert_eq!(run(&mut state, &weapon, fire(true), 240), 60);
        assert_eq!(state.magazines[0].loaded, 0);
        // reloading for two seconds (it began the tick it ran dry)
        let mut reloaded_at = None;
        for tick in 0..200 {
            state.step(&weapon, fire(false));
            if state.magazines[0].phase == MagazinePhase::Idle && state.magazines[0].loaded > 0 {
                reloaded_at = Some(tick);
                break;
            }
        }
        assert!(
            matches!(reloaded_at, Some(tick) if (118..=121).contains(&tick)),
            "{reloaded_at:?}"
        );
        assert_eq!(
            (state.magazines[0].loaded, state.magazines[0].total),
            (60, 180)
        );
    }

    #[test]
    fn a_reload_is_asked_for_and_only_tops_up() {
        let weapon = rifle();
        let mut state = state(&weapon);
        state.ready = 0;
        run(&mut state, &weapon, fire(true), 40);
        let loaded = state.magazines[0].loaded;
        assert!(loaded < 60);
        state.step(
            &weapon,
            WeaponControls {
                reload: true,
                ..WeaponControls::default()
            },
        );
        assert_eq!(state.magazines[0].phase, MagazinePhase::Reloading);
        // it cannot fire while reloading
        assert_eq!(run(&mut state, &weapon, fire(true), 60), 0);
        run(&mut state, &weapon, fire(false), 120);
        assert_eq!(state.magazines[0].loaded, 60);
        assert_eq!(state.magazines[0].total, 240 - (60 - loaded));
    }

    #[test]
    fn rounds_carried_run_out() {
        let mut weapon = rifle();
        weapon.magazines[0].rounds_total_initial = 70;
        let mut state = state(&weapon);
        state.ready = 0;
        assert_eq!(run(&mut state, &weapon, fire(true), 240), 60);
        run(&mut state, &weapon, fire(false), 150);
        assert_eq!(
            (state.magazines[0].loaded, state.magazines[0].total),
            (10, 0)
        );
        assert_eq!(run(&mut state, &weapon, fire(true), 240), 10);
        // and then it only clicks
        assert_eq!(state.triggers[0].phase, TriggerPhase::Locked);
        assert_eq!(run(&mut state, &weapon, fire(true), 60), 0);
    }

    #[test]
    fn a_shotgun_reloads_a_round_at_a_time() {
        let mut weapon = rifle();
        weapon.magazines[0] = Magazine {
            rounds_total_initial: 24,
            rounds_loaded_maximum: 12,
            reload_time: 0.5,
            rounds_reloaded: 1,
            ..Magazine::default()
        };
        let mut state = state(&weapon);
        state.ready = 0;
        state.magazines[0].loaded = 9;
        state.magazines[0].total = 15;
        state.reload(&weapon);
        // three rounds, half a second each
        run(&mut state, &weapon, fire(false), 3 * 30 + 1);
        assert_eq!(
            (state.magazines[0].loaded, state.magazines[0].total),
            (12, 12)
        );
        assert_eq!(state.magazines[0].phase, MagazinePhase::Idle);
    }

    #[test]
    fn heat_overheats_and_cools_at_its_rates() {
        let weapon = Weapon {
            heat_recovery_threshold: 0.2,
            heat_overheated_threshold: 1.0,
            heat_loss_per_second: 0.5,
            triggers: vec![Trigger {
                initial_rate_of_fire: 10.0,
                final_rate_of_fire: 10.0,
                magazine: None,
                rounds_per_shot: 1,
                heat_generated_per_round: 0.1,
                ..Trigger::default()
            }],
            ..Weapon::default()
        };
        let mut state = state(&weapon);
        let mut shots = 0;
        for _ in 0..240 {
            if state.overheated {
                break;
            }
            shots += state.step(&weapon, fire(true)).len();
        }
        assert!(state.overheated);
        // each round's 0.1 less the 0.05 that cools between rounds
        assert!((18..=20).contains(&shots), "{shots}");
        // held, it does not fire until it cools from 1 to under 0.2 at 0.5
        // a second: 1.6 seconds
        let cooled_at = (0..200).find(|_| !state.step(&weapon, fire(true)).is_empty());
        assert!(
            matches!(cooled_at, Some(tick) if (93..=98).contains(&tick)),
            "{cooled_at:?}"
        );
        assert!(!state.overheated);
    }

    #[test]
    fn the_error_grows_while_held_and_falls_when_let_go() {
        let weapon = rifle();
        let mut state = state(&weapon);
        state.ready = 0;
        run(&mut state, &weapon, fire(true), 30);
        assert!((state.triggers[0].error - 0.5).abs() < 0.02);
        run(&mut state, &weapon, fire(true), 30);
        assert!(state.triggers[0].error > 0.9999);
        // half a second to fall from 1
        run(&mut state, &weapon, fire(false), 15);
        assert!((state.triggers[0].error - 0.5).abs() < 0.02);
        run(&mut state, &weapon, fire(false), 15);
        assert!(state.triggers[0].error < 0.0001);
    }

    #[test]
    fn a_latched_trigger_fires_once_a_pull() {
        let mut weapon = rifle();
        weapon.triggers[0].flags |= Trigger::LATCHED;
        let mut state = state(&weapon);
        state.ready = 0;
        assert_eq!(run(&mut state, &weapon, fire(true), 60), 1);
        run(&mut state, &weapon, fire(false), 1);
        assert_eq!(run(&mut state, &weapon, fire(true), 60), 1);
    }

    #[test]
    fn a_charged_trigger_fires_when_let_go() {
        let mut weapon = rifle();
        weapon.triggers[0].charging_time = 0.5;
        weapon.triggers[0].charged_time = 5.0;
        let mut state = state(&weapon);
        state.ready = 0;
        // let go before it charges: nothing
        run(&mut state, &weapon, fire(true), 10);
        assert_eq!(run(&mut state, &weapon, fire(false), 2), 0);
        // held to its charge, then let go: one round
        assert_eq!(run(&mut state, &weapon, fire(true), 40), 0);
        assert_eq!(state.triggers[0].phase, TriggerPhase::Charged);
        assert_eq!(run(&mut state, &weapon, fire(false), 1), 1);
    }

    #[test]
    fn a_weapon_of_too_many_triggers_is_refused() {
        let mut weapon = rifle();
        weapon.triggers = vec![Trigger::default(); 3];
        assert_eq!(
            WeaponState::new(&weapon),
            Err(WeaponError::TooManyTriggers(3))
        );
    }
}
