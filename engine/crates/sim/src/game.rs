//! Game types (`game_engine.c`, `game_engine_ctf.c`, `game_engine_slayer.c`):
//! the rules of Slayer and Capture the Flag, the scores, the players'
//! respawn timers, the flags and the scenario's netgame equipment.
//!
//! [`Rules`] are the rules alone, stepped with where each player stands;
//! [`Game`] plays them over a [`Simulation`], reading its kills, standing
//! its players again at the level's starts and placing its flags and
//! equipment.

use opence_content::{GameType, Level, NetgameFlagKind, ObjectKind, TagRef};

use crate::clock::TICKS_PER_SECOND;
use crate::objects::{ObjectHandle, ObjectSpawn};
use crate::random::RandomSeed;
use crate::{Quat, Simulation, Transform, Vec3};

/// Halo's ticks a second, which its game options count in.
const GAME_TICKS_PER_SECOND: u32 = 30;

/// The shortest wait to respawn: 3 seconds (`game_engine.c`).
pub const MINIMUM_RESPAWN_TICKS: u32 = 3 * TICKS_PER_SECOND;

/// How near a player comes to a flag, or a carrier to its own stand, to
/// touch it.
pub const FLAG_TOUCH_RADIUS: f32 = 1.0;

/// How long a dropped flag lies untouched before it goes home: 17 seconds
/// (`CTF_FLAG_RETURN_TIME`).
pub const FLAG_RETURN_TICKS: u32 = 17 * TICKS_PER_SECOND;

/// How long taken equipment waits to come back when neither its placement
/// nor its item collection says.
pub const DEFAULT_EQUIPMENT_SECONDS: u16 = 30;

/// Halo's game ticks as the engine's.
fn engine_ticks(game_ticks: u32) -> u32 {
    game_ticks * TICKS_PER_SECOND / GAME_TICKS_PER_SECOND
}

/// A game variant: the game type and its options.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Variant {
    /// The game type.
    pub game_type: GameType,
    /// Whether players play in teams.
    pub teams: bool,
    /// The score that wins.
    pub score_to_win: i32,
    /// How long the game lasts, if it has a limit.
    pub time_limit_ticks: Option<u32>,
    /// How long a dead player waits to respawn (at least
    /// [`MINIMUM_RESPAWN_TICKS`]).
    pub respawn_ticks: u32,
    /// How much longer after a suicide or a betrayal.
    pub suicide_penalty_ticks: u32,
    /// Whether a team's own flag must be carried home rather than touched.
    pub flag_must_reset: bool,
    /// Whether a team's own flag must be home for it to score.
    pub flag_at_home_to_score: bool,
}

impl Variant {
    /// Slayer, every player for themself, as Halo's default.
    pub fn slayer() -> Self {
        Self {
            game_type: GameType::Slayer,
            teams: false,
            score_to_win: 15,
            time_limit_ticks: None,
            respawn_ticks: 0,
            suicide_penalty_ticks: engine_ticks(300),
            flag_must_reset: false,
            flag_at_home_to_score: false,
        }
    }

    /// Team Slayer.
    pub fn team_slayer() -> Self {
        Self {
            teams: true,
            ..Self::slayer()
        }
    }

    /// Capture the Flag, as Halo's default.
    pub fn ctf() -> Self {
        Self {
            game_type: GameType::CaptureTheFlag,
            teams: true,
            score_to_win: 3,
            time_limit_ticks: None,
            respawn_ticks: engine_ticks(300),
            suicide_penalty_ticks: engine_ticks(150),
            flag_must_reset: false,
            flag_at_home_to_score: false,
        }
    }
}

/// A player in the game: its team, its score and whether it waits to
/// respawn.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Contestant {
    /// Its team (in a game without teams, its own).
    pub team: u16,
    /// Its score.
    pub score: i32,
    /// The players it killed.
    pub kills: u32,
    /// The times it died.
    pub deaths: u32,
    /// The times it killed itself.
    pub suicides: u32,
    /// The times it killed a teammate.
    pub betrayals: u32,
    /// Whether it is alive.
    pub alive: bool,
    /// How long it still waits to respawn.
    pub respawn_ticks: u32,
}

/// Where a flag is.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum FlagState {
    /// On its stand.
    Home,
    /// Carried by a player.
    Carried(usize),
    /// Lying where its carrier fell.
    Dropped {
        /// Where.
        at: [f32; 3],
        /// How long it has lain there untouched.
        untouched_ticks: u32,
    },
}

/// A team's flag.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Flag {
    /// Its team.
    pub team: u16,
    /// Its stand.
    pub stand: [f32; 3],
    /// Where it is.
    pub state: FlagState,
}

impl Flag {
    /// Where it is now, unless a player carries it.
    pub fn position(&self) -> Option<[f32; 3]> {
        match self.state {
            FlagState::Home => Some(self.stand),
            FlagState::Carried(_) => None,
            FlagState::Dropped { at, .. } => Some(at),
        }
    }
}

/// Something that happened in the game.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum GameEvent {
    /// A player died, killed by a player (perhaps itself) or by nothing.
    Killed {
        /// The player who died.
        victim: usize,
        /// The player who killed it.
        killer: Option<usize>,
    },
    /// A player's wait is over: it stands at a start again.
    Respawn(usize),
    /// A player took an enemy's flag.
    FlagTaken {
        /// The flag.
        flag: usize,
        /// The player.
        by: usize,
    },
    /// A flag went home.
    FlagReturned(usize),
    /// A flag's carrier dropped it.
    FlagDropped(usize),
    /// A player carried an enemy's flag to its own stand.
    Captured {
        /// The flag.
        flag: usize,
        /// The player.
        by: usize,
    },
    /// The game is over.
    GameOver,
}

/// The rules of a game.
#[derive(Clone, Debug, PartialEq)]
pub struct Rules {
    variant: Variant,
    contestants: Vec<Contestant>,
    flags: Vec<Flag>,
    ticks: u32,
    over: bool,
    events: Vec<GameEvent>,
}

/// Whether `a` is within `radius` of `b`.
fn within(a: [f32; 3], b: [f32; 3], radius: f32) -> bool {
    let d = [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
    d[0] * d[0] + d[1] * d[1] + d[2] * d[2] <= radius * radius
}

impl Rules {
    /// A game of `variant`, whose flags stand at `stands` (a team and its
    /// stand each), with no players yet.
    pub fn new(variant: Variant, stands: &[(u16, [f32; 3])]) -> Self {
        let flags = if variant.game_type == GameType::CaptureTheFlag {
            stands
                .iter()
                .map(|&(team, stand)| Flag {
                    team,
                    stand,
                    state: FlagState::Home,
                })
                .collect()
        } else {
            Vec::new()
        };
        Self {
            variant,
            contestants: Vec::new(),
            flags,
            ticks: 0,
            over: false,
            events: Vec::new(),
        }
    }

    /// The variant.
    pub fn variant(&self) -> &Variant {
        &self.variant
    }

    /// Adds a player, alive, on `team` (ignored without teams: each
    /// player is its own); its index is returned.
    pub fn join(&mut self, team: u16) -> usize {
        let index = self.contestants.len();
        let team = if self.variant.teams {
            team
        } else {
            u16::try_from(index).unwrap_or(u16::MAX)
        };
        self.contestants.push(Contestant {
            team,
            alive: true,
            ..Contestant::default()
        });
        index
    }

    /// The players.
    pub fn contestants(&self) -> &[Contestant] {
        &self.contestants
    }

    /// The flags.
    pub fn flags(&self) -> &[Flag] {
        &self.flags
    }

    /// How long the game has lasted.
    pub fn ticks(&self) -> u32 {
        self.ticks
    }

    /// Whether the game is over.
    pub fn is_over(&self) -> bool {
        self.over
    }

    /// A team's score: its players' scores in Slayer, its captures in
    /// Capture the Flag. Without teams, a player's.
    pub fn team_score(&self, team: u16) -> i32 {
        self.contestants
            .iter()
            .filter(|contestant| contestant.team == team)
            .map(|contestant| contestant.score)
            .sum()
    }

    /// The leading team (without teams, player), if one leads alone.
    pub fn leader(&self) -> Option<u16> {
        let mut teams: Vec<u16> = self.contestants.iter().map(|c| c.team).collect();
        teams.sort_unstable();
        teams.dedup();
        let mut scores: Vec<(i32, u16)> = teams
            .into_iter()
            .map(|team| (self.team_score(team), team))
            .collect();
        scores.sort_unstable_by_key(|&(score, _)| std::cmp::Reverse(score));
        match scores.as_slice() {
            [(best, team), (next, _), ..] if best > next => Some(*team),
            [(_, team)] => Some(*team),
            _ => None,
        }
    }

    /// Player `victim` died where it stood, at `at`, killed by `killer`
    /// (itself for a suicide; `None` for a fall or the level): its killer
    /// scores, it drops a flag it carried and waits to respawn.
    pub fn player_killed(&mut self, victim: usize, killer: Option<usize>, at: [f32; 3]) {
        let Some(dead) = self.contestants.get(victim).copied() else {
            return;
        };
        if !dead.alive || self.over {
            return;
        }
        let killer = killer.filter(|&killer| killer < self.contestants.len());
        let friendly = match killer {
            Some(killer) => killer == victim || self.contestants[killer].team == dead.team,
            None => false,
        };
        // (a death that is not another player's kill costs the penalty)
        let penalty = killer.is_none() || friendly;
        if let Some(killer) = killer {
            let slayer = self.variant.game_type == GameType::Slayer;
            let scorer = &mut self.contestants[killer];
            if killer == victim {
                scorer.suicides += 1;
            } else if friendly {
                scorer.betrayals += 1;
            } else {
                scorer.kills += 1;
            }
            if slayer {
                scorer.score += if friendly { -1 } else { 1 };
            }
        }
        let wait = self.variant.respawn_ticks
            + if penalty {
                self.variant.suicide_penalty_ticks
            } else {
                0
            };
        let contestant = &mut self.contestants[victim];
        contestant.alive = false;
        contestant.deaths += 1;
        contestant.respawn_ticks = wait.max(MINIMUM_RESPAWN_TICKS);
        self.events.push(GameEvent::Killed { victim, killer });
        for (index, flag) in self.flags.iter_mut().enumerate() {
            if flag.state == FlagState::Carried(victim) {
                flag.state = FlagState::Dropped {
                    at,
                    untouched_ticks: 0,
                };
                self.events.push(GameEvent::FlagDropped(index));
            }
        }
        self.check_score();
    }

    /// One tick, with where each player stands (`None` for the dead and
    /// the absent): the respawn timers count down, players touch flags
    /// and carry them home, and dropped flags go home in time. What
    /// happened since the last step is returned.
    pub fn step(&mut self, positions: &[Option<[f32; 3]>]) -> Vec<GameEvent> {
        if !self.over {
            self.ticks += 1;
            for (index, contestant) in self.contestants.iter_mut().enumerate() {
                if contestant.alive {
                    continue;
                }
                contestant.respawn_ticks = contestant.respawn_ticks.saturating_sub(1);
                if contestant.respawn_ticks == 0 {
                    contestant.alive = true;
                    self.events.push(GameEvent::Respawn(index));
                }
            }
            self.step_flags(positions);
            self.check_score();
            if let Some(limit) = self.variant.time_limit_ticks {
                if self.ticks >= limit && !self.over {
                    self.over = true;
                    self.events.push(GameEvent::GameOver);
                }
            }
        }
        std::mem::take(&mut self.events)
    }

    fn step_flags(&mut self, positions: &[Option<[f32; 3]>]) {
        let living = |index: usize| -> Option<[f32; 3]> {
            match (self.contestants.get(index), positions.get(index)) {
                (Some(contestant), Some(&Some(at))) if contestant.alive => Some(at),
                _ => None,
            }
        };
        let mut changes = Vec::new();
        for (flag_index, flag) in self.flags.iter().enumerate() {
            match flag.state {
                FlagState::Carried(carrier) => {
                    let Some(at) = living(carrier) else {
                        continue;
                    };
                    let team = self.contestants[carrier].team;
                    let home = self.flags.iter().find(|own| own.team == team);
                    let Some(home) = home else {
                        continue;
                    };
                    let at_home = home.state == FlagState::Home;
                    if within(at, home.stand, FLAG_TOUCH_RADIUS)
                        && (at_home || !self.variant.flag_at_home_to_score)
                    {
                        changes.push(GameEvent::Captured {
                            flag: flag_index,
                            by: carrier,
                        });
                    }
                }
                FlagState::Home | FlagState::Dropped { .. } => {
                    let Some(position) = flag.position() else {
                        continue;
                    };
                    let toucher = (0..self.contestants.len()).find_map(|index| {
                        let at = living(index)?;
                        within(at, position, FLAG_TOUCH_RADIUS).then_some(index)
                    });
                    match toucher {
                        Some(index) if self.contestants[index].team != flag.team => {
                            let carrying = self
                                .flags
                                .iter()
                                .any(|other| other.state == FlagState::Carried(index));
                            if !carrying {
                                changes.push(GameEvent::FlagTaken {
                                    flag: flag_index,
                                    by: index,
                                });
                            }
                        }
                        Some(_)
                            if flag.state != FlagState::Home && !self.variant.flag_must_reset =>
                        {
                            changes.push(GameEvent::FlagReturned(flag_index));
                        }
                        _ => {
                            if let FlagState::Dropped {
                                untouched_ticks, ..
                            } = flag.state
                            {
                                if untouched_ticks + 1 >= FLAG_RETURN_TICKS {
                                    changes.push(GameEvent::FlagReturned(flag_index));
                                }
                            }
                        }
                    }
                }
            }
        }
        for flag in &mut self.flags {
            if let FlagState::Dropped {
                untouched_ticks, ..
            } = &mut flag.state
            {
                *untouched_ticks += 1;
            }
        }
        for change in changes {
            match change {
                GameEvent::FlagTaken { flag, by } => {
                    self.flags[flag].state = FlagState::Carried(by);
                }
                GameEvent::FlagReturned(flag) => {
                    self.flags[flag].state = FlagState::Home;
                }
                GameEvent::Captured { flag, by } => {
                    self.flags[flag].state = FlagState::Home;
                    self.contestants[by].score += 1;
                }
                _ => continue,
            }
            self.events.push(change);
        }
    }

    fn check_score(&mut self) {
        if self.over {
            return;
        }
        let won = self
            .contestants
            .iter()
            .any(|contestant| self.team_score(contestant.team) >= self.variant.score_to_win);
        if won {
            self.over = true;
            self.events.push(GameEvent::GameOver);
        }
    }
}

/// A netgame equipment placement in play: what stands there and how long
/// until it comes back.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct EquipmentSpot {
    placement: usize,
    object: Option<ObjectHandle>,
    wait_ticks: u32,
}

/// A game played over a [`Simulation`].
#[derive(Clone, Debug)]
pub struct Game {
    rules: Rules,
    flag_objects: Vec<Option<ObjectHandle>>,
    equipment: Vec<EquipmentSpot>,
    seed: RandomSeed,
}

impl Game {
    /// Starts a game of `variant` on `level` with the simulation's players
    /// (alternating between teams 0 and 1 in a game with teams): its
    /// flags go to their stands and its equipment to its placements.
    pub fn start(simulation: &mut Simulation, level: &Level, variant: Variant) -> Self {
        let stands: Vec<(u16, [f32; 3])> = level
            .scenario
            .netgame_flags
            .iter()
            .filter(|flag| flag.kind == NetgameFlagKind::CtfFlag)
            .map(|flag| (flag.team, flag.position))
            .collect();
        let mut rules = Rules::new(variant, &stands);
        for index in 0..simulation.players().len() {
            rules.join(u16::try_from(index % 2).unwrap_or(0));
        }
        let mut game = Self {
            flag_objects: vec![None; rules.flags().len()],
            rules,
            equipment: Vec::new(),
            seed: RandomSeed::default(),
        };
        for flag in 0..game.flag_objects.len() {
            game.place_flag(simulation, level, flag);
        }
        game.equipment = level
            .scenario
            .netgame_equipment
            .iter()
            .enumerate()
            .filter(|(_, placement)| placement.game_types.matches(variant.game_type))
            .map(|(placement, _)| EquipmentSpot {
                placement,
                object: None,
                wait_ticks: 0,
            })
            .collect();
        for spot in 0..game.equipment.len() {
            game.place_equipment(simulation, level, spot);
        }
        game
    }

    /// Adds a player the simulation added after the game began.
    pub fn join(&mut self, team: u16) -> usize {
        self.rules.join(team)
    }

    /// The rules and the scores.
    pub fn rules(&self) -> &Rules {
        &self.rules
    }

    /// The rules, to kill players by hand.
    pub fn rules_mut(&mut self) -> &mut Rules {
        &mut self.rules
    }

    /// The flags' objects, by flag.
    pub fn flag_objects(&self) -> &[Option<ObjectHandle>] {
        &self.flag_objects
    }

    /// The equipment standing at the placements in play.
    pub fn equipment_objects(&self) -> Vec<ObjectHandle> {
        self.equipment
            .iter()
            .filter_map(|spot| spot.object)
            .collect()
    }

    /// One tick, after the simulation's: the kills it saw are scored, the
    /// rules step, the players whose wait is over stand at a start, and
    /// the flags and the taken equipment are put back.
    pub fn step(&mut self, simulation: &mut Simulation, level: &Level) -> Vec<GameEvent> {
        let handles: Vec<ObjectHandle> = simulation.players().iter().map(|p| p.handle()).collect();
        let player_of = |handle: ObjectHandle| handles.iter().position(|&h| h == handle);
        let kills: Vec<(usize, Option<usize>)> = simulation
            .damage_events()
            .iter()
            .filter(|event| event.outcome.killed)
            .filter_map(|event| {
                let victim = player_of(event.object)?;
                Some((victim, event.owner.and_then(player_of)))
            })
            .collect();
        for (victim, killer) in kills {
            let at = position(simulation, handles[victim]).unwrap_or([0.0; 3]);
            self.rules.player_killed(victim, killer, at);
        }
        let positions: Vec<Option<[f32; 3]>> = handles
            .iter()
            .enumerate()
            .map(|(index, &handle)| {
                let alive = self.rules.contestants().get(index).is_some_and(|c| c.alive);
                alive.then(|| position(simulation, handle)).flatten()
            })
            .collect();
        let events = self.rules.step(&positions);
        for event in &events {
            match *event {
                GameEvent::Respawn(index) => self.respawn(simulation, level, index),
                GameEvent::FlagTaken { flag, .. }
                | GameEvent::FlagDropped(flag)
                | GameEvent::FlagReturned(flag)
                | GameEvent::Captured { flag, .. } => self.place_flag(simulation, level, flag),
                GameEvent::Killed { .. } | GameEvent::GameOver => {}
            }
        }
        for spot in 0..self.equipment.len() {
            let taken = self.equipment[spot]
                .object
                .is_some_and(|handle| simulation.object(handle).is_none());
            if taken {
                let seconds = level
                    .scenario
                    .netgame_equipment
                    .get(self.equipment[spot].placement)
                    .and_then(|placement| placement.spawn_seconds)
                    .unwrap_or(DEFAULT_EQUIPMENT_SECONDS);
                self.equipment[spot].object = None;
                self.equipment[spot].wait_ticks = u32::from(seconds) * TICKS_PER_SECOND;
            } else if self.equipment[spot].object.is_none() {
                let wait = &mut self.equipment[spot].wait_ticks;
                *wait = wait.saturating_sub(1);
                if *wait == 0 {
                    self.place_equipment(simulation, level, spot);
                }
            }
        }
        events
    }

    /// Stands player `index` at the start for the game type and its team
    /// farthest from its living enemies.
    fn respawn(&mut self, simulation: &mut Simulation, level: &Level, index: usize) {
        let Some(contestant) = self.rules.contestants().get(index).copied() else {
            return;
        };
        let variant = *self.rules.variant();
        let enemies: Vec<[f32; 3]> = simulation
            .players()
            .iter()
            .enumerate()
            .filter(|&(other, _)| other != index)
            .filter(|&(other, _)| {
                self.rules
                    .contestants()
                    .get(other)
                    .is_some_and(|c| c.alive && c.team != contestant.team)
            })
            .filter_map(|(_, player)| position(simulation, player.handle()))
            .collect();
        let starts = &level.scenario.player_starts;
        let fits = |team_matters: bool| {
            starts
                .iter()
                .enumerate()
                .filter(move |(_, start)| start.game_types.matches(variant.game_type))
                .filter(move |(_, start)| !team_matters || start.team == contestant.team)
                .map(|(at, _)| at)
        };
        let candidates: Vec<usize> = if variant.teams && fits(true).next().is_some() {
            fits(true).collect()
        } else if fits(false).next().is_some() {
            fits(false).collect()
        } else {
            (0..starts.len()).collect()
        };
        let nearest_enemy = |start: usize| {
            let at = starts[start].position;
            enemies
                .iter()
                .map(|enemy| {
                    let d = [at[0] - enemy[0], at[1] - enemy[1], at[2] - enemy[2]];
                    d[0] * d[0] + d[1] * d[1] + d[2] * d[2]
                })
                .fold(f32::INFINITY, f32::min)
        };
        let chosen = if enemies.is_empty() {
            // (no one to avoid: a random one of them)
            let pick = usize::from(self.seed.next_u16()) % candidates.len().max(1);
            candidates.get(pick).copied()
        } else {
            candidates
                .iter()
                .copied()
                .max_by(|&a, &b| nearest_enemy(a).total_cmp(&nearest_enemy(b)))
        };
        if let Some(start) = chosen {
            let team = variant.teams.then_some(contestant.team);
            let _ = simulation.respawn_player(index, level, start, team);
        }
    }

    /// Puts flag `flag`'s object where the flag is (none while carried).
    fn place_flag(&mut self, simulation: &mut Simulation, level: &Level, flag: usize) {
        if let Some(handle) = self.flag_objects.get_mut(flag).and_then(Option::take) {
            simulation.remove(handle);
        }
        let Some(tag) = level.globals.multiplayer_flag.clone() else {
            return;
        };
        let Some(state) = self.rules.flags().get(flag).copied() else {
            return;
        };
        let Some(at) = state.position() else {
            return;
        };
        let spawned = spawn_item(simulation, level, tag, at, 0.0, Some(state.team));
        if let Some(slot) = self.flag_objects.get_mut(flag) {
            *slot = spawned;
        }
    }

    /// Stands one of placement `spot`'s items there, chosen by weight.
    fn place_equipment(&mut self, simulation: &mut Simulation, level: &Level, spot: usize) {
        let Some(placement) = level
            .scenario
            .netgame_equipment
            .get(self.equipment[spot].placement)
        else {
            return;
        };
        let total: f32 = placement.items.iter().map(|item| item.weight).sum();
        let mut roll = self.seed.next_real() * total;
        let mut chosen = placement.items.first();
        for item in &placement.items {
            chosen = Some(item);
            if roll < item.weight {
                break;
            }
            roll -= item.weight;
        }
        let Some(item) = chosen else {
            return;
        };
        let team = (placement.team != 0).then_some(placement.team);
        self.equipment[spot].object = spawn_item(
            simulation,
            level,
            item.item.clone(),
            placement.position,
            placement.facing,
            team,
        );
    }
}

/// Where an object stands, as an array.
fn position(simulation: &Simulation, handle: ObjectHandle) -> Option<[f32; 3]> {
    simulation
        .transform(handle)
        .map(|transform| transform.position.to_array())
}

/// Spawns an item (a weapon or equipment) at `at`, facing `facing`.
fn spawn_item(
    simulation: &mut Simulation,
    level: &Level,
    tag: TagRef,
    at: [f32; 3],
    facing: f32,
    team: Option<u16>,
) -> Option<ObjectHandle> {
    let kind = level.object(&tag)?.kind;
    if !matches!(kind, ObjectKind::Weapon | ObjectKind::Equipment) {
        return None;
    }
    simulation
        .spawn(ObjectSpawn {
            kind,
            tag,
            transform: Transform {
                position: Vec3::from_array(at),
                rotation: Quat::from_rotation_z(facing),
                scale: 1.0,
            },
            name: None,
            permutation: None,
            team,
        })
        .ok()
}
