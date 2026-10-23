//! The AI's actors: the squads the mission and its scripts place, each
//! actor a biped steered as a player's is, by the input the AI gives it each
//! tick (`ai.c`, `actor_perception.c`, `actor_firing_position.c`,
//! `actor_combat.c`). An actor sees what its variant lets it see, keeps to
//! its squad's firing positions at its combat range, fires bursts at what
//! it sees within its range, hides while its shield is low, runs when a
//! friend dies near it, and follows the command list it is given.

use std::sync::Arc;

use glam::Vec2;
use opence_content::{ActorVariant, Encounter, Level};
use opence_sim::ai_impulse::{choose_dive, choose_surprise, AnimationImpulse, Escape};
use opence_sim::damage::Vitality;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{Difficulty, ObjectHandle, Team};
use opence_sim::player::MELEE_RANGE;
use opence_sim::units::Radians;
use opence_sim::Vec3;

use crate::ai::AiReference;
use crate::{element, Mission};

/// Ticks in a second.
const TICKS: f32 = 30.0;
/// The most an actor turns in a tick.
const TURN: f32 = 0.2;
/// How near its goal an actor stops.
const ARRIVED: f32 = 0.5;
/// How near its aim must be to its target before it fires.
const ON_TARGET: f32 = 0.15;
/// How much nearer than its target another must be for an actor to turn
/// to it.
const NEARER: f32 = 2.0;

/// How near the line of fire a friend's eyes must be for an actor to
/// hold its fire.
const FRIEND_WIDTH: f32 = 0.5;

/// How near its target's eyes an actor's must be for it to strike (its
/// reach, and a little for the ray to find the body).
const MELEE: f32 = MELEE_RANGE + 0.4;
/// The ticks an actor runs for when it panics.
const PANIC_TICKS: u16 = 90;
/// How near a friend's death is felt.
const PANIC_DISTANCE: f32 = 10.0;
/// How far below a starting location the level must be for an actor to be
/// placed there (one in a structure not loaded has nothing under it).
const GROUND: f32 = 2.0;
/// The ticks before an actor looks again for where to stand.
const REPICK_TICKS: u16 = 90;
/// How much farther than it sees an actor keeps a target it has seen.
const AWARENESS: f32 = 2.0;
/// The encounter flag for one placed only when the scripts place it.
const NOT_INITIALLY_CREATED: u32 = 1;

/// The game's teams (`game_team`).
pub(crate) const PLAYER_TEAM: u16 = 1;
const HUMAN_TEAM: u16 = 2;
const COVENANT_TEAM: u16 = 3;
const FLOOD_TEAM: u16 = 4;
const SENTINEL_TEAM: u16 = 5;

// (a command list's atoms that the actors follow)
const PAUSE: i16 = 0;
const GO_TO: i16 = 1;
const GO_TO_AND_FACE: i16 = 2;
/// A command step's wait when it has just begun.
const FRESH: u16 = u16::MAX;
/// How near (in world units, across the ground) an enemy's grenade lands
/// before an actor dives from it.
const DIVE_RADIUS: f32 = 3.0;

/// What an actor is doing.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ActorState {
    /// Nothing seen: it stands, or follows its command list.
    Idle,
    /// Fighting its target.
    Fighting,
    /// Hiding from its target while its shield recharges.
    Hiding,
    /// Running from its target.
    Fleeing,
}

/// An actor of the AI.
#[derive(Clone, Debug, PartialEq)]
pub struct Actor {
    /// Its unit.
    pub unit: ObjectHandle,
    /// The squad it was placed in.
    pub squad: AiReference,
    /// Its variant, by its place in the scenario's actor palette.
    pub variant: usize,
    /// Its team.
    pub team: u16,
    /// What it is doing.
    pub state: ActorState,
    /// What it fights.
    pub target: Option<ObjectHandle>,
    /// Where it is going.
    pub goal: Option<Vec3>,
    /// The command list it follows, its step in it and the ticks it waits.
    pub command: Option<(i32, usize, u16)>,
    alive: bool,
    burst: u16,
    wait: u16,
    panic: u16,
    repick: u16,
    /// The ticks left of the impulse it plays, and the way it faces.
    impulse: Option<(u16, Vec2)>,
    /// It has gone berserk, and charges until it dies.
    berserk: bool,
    /// Its body when it last thought.
    body: f32,
}

/// What startles an actor as it thinks.
#[derive(Clone, Copy, Debug, PartialEq)]
enum Reaction {
    /// It saw an enemy near, the way to which is given.
    Surprise(Vec2),
    /// It goes berserk.
    Berserk,
    /// It dives from a grenade the way given.
    Dive(Vec2),
}

/// What an actor senses as it thinks: its state before and now, how far
/// its target is, its body before and now, and the way across the ground
/// to an enemy's grenade near it.
#[derive(Clone, Copy, Debug)]
struct Senses {
    was: ActorState,
    now: ActorState,
    berserk: bool,
    target: Option<Vec2>,
    body: [f32; 2],
    grenade: Option<Vec2>,
}

/// How an actor of `definition` reacts to what it senses: it dives from a
/// grenade first, goes berserk (always while it fights if its variant
/// says so, else when its target comes within its berserk distance or a
/// wound costs it its berserk damage and leaves it below the threshold),
/// else starts at an enemy that surprises it within its surprise distance
/// as it first sees one.
fn react(definition: &ActorVariant, senses: Senses) -> Option<Reaction> {
    if let Some(way) = senses.grenade.filter(|_| !senses.berserk) {
        return Some(Reaction::Dive(-way.normalize_or(Vec2::X)));
    }
    let distance = senses.target.map_or(f32::INFINITY, Vec2::length);
    let [amount, threshold] = definition.berserk_damage;
    let wound = senses.body[0] - senses.body[1];
    let berserk = senses.now == ActorState::Fighting
        && !senses.berserk
        && (definition.actor_flags & ActorVariant::ALWAYS_BERSERK_IN_ATTACKING_MODE != 0
            || distance < definition.berserk_distance
            || (amount > 0.0 && wound >= amount && senses.body[1] < threshold));
    if berserk {
        return Some(Reaction::Berserk);
    }
    let seen = senses.was == ActorState::Idle
        && matches!(senses.now, ActorState::Fighting | ActorState::Hiding);
    senses
        .target
        .filter(|_| seen && distance < definition.surprise_distance)
        .map(Reaction::Surprise)
}

/// What an actor knows of the others: their units, teams, eyes and
/// whether they live.
pub(crate) struct Other {
    pub(crate) unit: ObjectHandle,
    pub(crate) team: u16,
    pub(crate) eyes: Vec3,
    pub(crate) alive: bool,
}

/// The team an encounter's actors are on: its own, or the one its actors'
/// type belongs to.
pub(crate) fn team_of(encounter_team: i16, kind: i16) -> u16 {
    match u16::try_from(encounter_team) {
        Ok(team) if team > 0 => team,
        _ => match kind {
            // (player, marine, crew)
            6..=8 => HUMAN_TEAM,
            // (the flood's forms)
            9..=11 => FLOOD_TEAM,
            // (monitor, sentinel)
            12 | 13 => SENTINEL_TEAM,
            _ => COVENANT_TEAM,
        },
    }
}

/// How many of a squad's actors are placed at `difficulty`: its normal
/// count on easy and normal, its insane count on legendary, between them
/// on heroic; all its starting locations if it gives none.
pub(crate) fn squad_count(count: [i16; 2], locations: usize, difficulty: Difficulty) -> usize {
    let [normal, insane] = count.map(|c| usize::try_from(c).unwrap_or(0));
    let count = match difficulty {
        Difficulty::Easy | Difficulty::Normal => normal,
        Difficulty::Hard => (normal + insane).div_ceil(2),
        Difficulty::Legendary => insane,
    };
    if count == 0 {
        locations
    } else {
        count.min(locations)
    }
}

impl Mission {
    /// The AI's actors, in the simulation's order of them.
    pub fn actors(&self) -> &[Actor] {
        &self.actors
    }

    /// Places the squads `reference` names, as `ai_place` does.
    pub fn place_ai(&mut self, reference: AiReference) {
        place(self, reference);
    }

    /// Whether teams `a` and `b` fight: they differ, and neither the game
    /// (the player and the humans) nor the scripts made them allies.
    pub(crate) fn enemies(&self, a: u16, b: u16) -> bool {
        let allied = |a: u16, b: u16| {
            matches!((a, b), (PLAYER_TEAM | HUMAN_TEAM, PLAYER_TEAM | HUMAN_TEAM))
                || self
                    .ai
                    .allegiances
                    .contains(&(i32::from(a.min(b)), i32::from(a.max(b))))
        };
        a != b && !allied(a, b)
    }

    pub(crate) fn living(&self, unit: ObjectHandle) -> bool {
        self.sim.object(unit).is_some()
            && !self
                .sim
                .component::<Vitality>(unit)
                .is_some_and(|vitality| vitality.dead)
    }

    /// A random real between `bounds`.
    fn uniform(&mut self, [low, high]: [f32; 2]) -> f32 {
        #[allow(clippy::cast_precision_loss)] // (24 bits, exact in a real)
        let along = self.next_random() as f32 / (1u32 << 24) as f32;
        low + (high - low) * along
    }

    /// A random number of ticks between `bounds` seconds, at least one.
    fn ticks(&mut self, bounds: [f32; 2]) -> u16 {
        #[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
        // (clamped to a u16's range first)
        let ticks = (self.uniform(bounds) * TICKS).clamp(1.0, f32::from(u16::MAX)) as u16;
        ticks
    }
}

/// Places the encounters made when the mission begins.
pub(crate) fn begin(mission: &mut Mission) {
    let level = Arc::clone(&mission.level);
    for (index, encounter) in level.scenario.campaign.encounters.iter().enumerate() {
        if encounter.flags & NOT_INITIALLY_CREATED == 0 {
            if let Ok(index) = u16::try_from(index) {
                place(mission, AiReference::Encounter(index));
            }
        }
    }
}

/// Places the squads `reference` names (an encounter's, a platoon's or
/// one), each that has no actor living.
pub(crate) fn place(mission: &mut Mission, reference: AiReference) {
    let level = Arc::clone(&mission.level);
    let Some(e) = reference.encounter() else {
        return;
    };
    let Some(encounter) = level.scenario.campaign.encounters.get(usize::from(e)) else {
        return;
    };
    for (s, squad) in encounter.squads.iter().enumerate() {
        let named = match reference {
            AiReference::None => false,
            AiReference::Encounter(_) => true,
            AiReference::Platoon(_, p) => squad.platoon == i16::from(p),
            AiReference::Squad(_, q) => s == usize::from(q),
        };
        let Ok(s) = u8::try_from(s) else {
            break;
        };
        let squad_reference = AiReference::Squad(e, s);
        let placed = mission.ai.units(squad_reference);
        if named && !placed.iter().any(|&unit| mission.living(unit)) {
            place_squad(mission, &level, encounter, (e, s));
        }
    }
}

/// Places squad `s` of encounter `e`, an actor at each of its starting
/// locations its count reaches.
fn place_squad(mission: &mut Mission, level: &Level, encounter: &Encounter, (e, s): (u16, u8)) {
    let campaign = &level.scenario.campaign;
    let Some(squad) = encounter.squads.get(usize::from(s)) else {
        return;
    };
    let count = squad_count(
        squad.count,
        squad.starting_locations.len(),
        mission.difficulty,
    );
    let reference = AiReference::Squad(e, s);
    for location in squad.starting_locations.iter().take(count) {
        let palette = if location.actor >= 0 {
            location.actor
        } else {
            squad.actor
        };
        let Some((variant, definition)) = usize::try_from(palette).ok().and_then(|index| {
            let definition = campaign.actor_variants.get(index)?.as_ref()?;
            Some((index, definition))
        }) else {
            continue;
        };
        let Some(unit) = &definition.unit else {
            continue;
        };
        let position = Vec3::from_array(location.position);
        if mission
            .sim
            .level_clear(position + Vec3::Z * 0.5, position - Vec3::Z * GROUND)
        {
            continue;
        }
        let team = team_of(encounter.team, definition.kind);
        let placed = mission.sim.add_actor(
            level,
            unit,
            definition.weapon.as_ref(),
            (position, location.facing),
            Some(team),
        );
        let Ok((_, handle)) = placed else {
            break; // (the table is full)
        };
        mission.actors.push(Actor {
            unit: handle,
            squad: reference,
            variant,
            team,
            state: ActorState::Idle,
            target: None,
            goal: None,
            command: None,
            alive: true,
            burst: 0,
            wait: 0,
            panic: 0,
            repick: 0,
            impulse: None,
            berserk: false,
            body: 1.0,
        });
        mission.ai.actors.entry(reference).or_default().push(handle);
        if let Ok(platoon) = u8::try_from(squad.platoon) {
            let platoon = AiReference::Platoon(e, platoon);
            mission.ai.actors.entry(platoon).or_default().push(handle);
        }
        if element(&campaign.command_lists, i32::from(location.command_list)).is_some() {
            mission
                .ai
                .command_lists
                .insert(handle, i32::from(location.command_list));
        }
    }
}

/// Takes actor `unit` out of the world, with its unit; false if it is no
/// actor.
pub(crate) fn erase(mission: &mut Mission, unit: ObjectHandle) -> bool {
    let Some(index) = mission.actors.iter().position(|a| a.unit == unit) else {
        return false;
    };
    mission.actors.remove(index);
    mission.ai.command_lists.remove(&unit);
    mission.sim.remove_actor(index)
}

/// Kills actor `unit` where it stands.
pub(crate) fn kill(mission: &mut Mission, unit: ObjectHandle) {
    if let Some(mut vitality) = mission.sim.component::<Vitality>(unit) {
        vitality.body = -1.0;
        vitality.shield = 0.0;
        vitality.dead = true;
        mission.sim.insert(unit, vitality);
    }
}

/// Decides what each actor does this tick and gives the simulation their
/// inputs.
pub(crate) fn think(mission: &mut Mission) {
    if !mission.ai.enabled {
        mission.sim.steer_actors(&[]);
        return;
    }
    let level = Arc::clone(&mission.level);
    let others = others(mission);
    panic_at_deaths(mission, &level, &others);
    let mut inputs = Vec::with_capacity(mission.actors.len());
    for index in 0..mission.actors.len() {
        inputs.push(think_one(mission, &level, &others, index));
    }
    mission.sim.steer_actors(&inputs);
}

/// The players and the actors, as each actor sees them.
pub(crate) fn others(mission: &Mission) -> Vec<Other> {
    let players = mission.sim.players().iter().map(|player| {
        let team = mission
            .sim
            .component::<Team>(player.handle())
            .map_or(PLAYER_TEAM, |team| team.0);
        (player.handle(), team, player.eyes())
    });
    let actors = mission
        .sim
        .actors()
        .iter()
        .zip(&mission.actors)
        .map(|(player, actor)| (player.handle(), actor.team, player.eyes()));
    players
        .chain(actors)
        .map(|(unit, team, eyes)| Other {
            unit,
            team,
            eyes,
            alive: mission.living(unit),
        })
        .collect()
}

/// Each actor that died since the last tick: its friends near it may
/// panic, by their variants' chances.
fn panic_at_deaths(mission: &mut Mission, level: &Level, others: &[Other]) {
    let died: Vec<(u16, Vec3)> = mission
        .actors
        .iter()
        .filter(|actor| actor.alive)
        .filter_map(|actor| {
            let other = others.iter().find(|o| o.unit == actor.unit)?;
            (!other.alive).then_some((actor.team, other.eyes))
        })
        .collect();
    for actor in &mut mission.actors {
        actor.alive = others.iter().any(|o| o.unit == actor.unit && o.alive);
    }
    for (team, at) in died {
        for index in 0..mission.actors.len() {
            let actor = &mission.actors[index];
            let near = others
                .iter()
                .find(|o| o.unit == actor.unit)
                .is_some_and(|o| o.alive && o.eyes.distance(at) < PANIC_DISTANCE);
            if actor.team != team || !near || actor.panic > 0 {
                continue;
            }
            let chance = variant(level, actor.variant).map_or(0.0, |v| v.panic_chance);
            if mission.uniform([0.0, 1.0]) < chance {
                mission.actors[index].panic = PANIC_TICKS;
            }
        }
    }
}

fn variant(level: &Level, index: usize) -> Option<&ActorVariant> {
    level.scenario.campaign.actor_variants.get(index)?.as_ref()
}

/// The turn from `from` to `to`, the shorter way round.
fn turn(from: f32, to: f32) -> f32 {
    libm::remainderf(to - from, std::f32::consts::TAU)
}

/// The facing and look toward `to` from `from`.
fn aim(from: Vec3, to: Vec3) -> (f32, f32) {
    let d = to - from;
    (
        libm::atan2f(d.y, d.x),
        libm::atan2f(d.z, libm::hypotf(d.x, d.y)),
    )
}

/// The throttle that moves a biped facing `yaw` toward `direction` (on
/// the ground), at full speed.
fn throttle(yaw: f32, direction: Vec3) -> (i8, i8) {
    let flat = Vec3::new(direction.x, direction.y, 0.0).normalize_or_zero();
    let (sin, cos) = (libm::sinf(yaw), libm::cosf(yaw));
    let forward = flat.x * cos + flat.y * sin;
    let left = -flat.x * sin + flat.y * cos;
    #[allow(clippy::cast_possible_truncation)] // (within -127 to 127)
    let axis = |v: f32| (v.clamp(-1.0, 1.0) * 127.0) as i8;
    (axis(forward), axis(-left))
}

/// The nearest enemy `index` sees: within its vision's distance and angle,
/// or near enough to hear, with nothing of the level between.
fn perceive(
    mission: &Mission,
    definition: &ActorVariant,
    others: &[Other],
    (index, eyes, yaw): (usize, Vec3, f32),
) -> Option<ObjectHandle> {
    let actor = &mission.actors[index];
    let mut nearest: Option<(f32, ObjectHandle)> = None;
    for other in others.iter().filter(|o| o.alive && o.unit != actor.unit) {
        if !mission.enemies(actor.team, other.team) {
            continue;
        }
        let distance = eyes.distance(other.eyes);
        let (facing, _) = aim(eyes, other.eyes);
        let seen = distance < definition.vision_distance
            && turn(yaw, facing).abs() < definition.vision_angle.max(0.1);
        let heard = distance < definition.hearing_distance;
        if !(seen || heard) || nearest.is_some_and(|(d, _)| d <= distance) {
            continue;
        }
        if mission.sim.level_clear(eyes, other.eyes) {
            nearest = Some((distance, other.unit));
        }
    }
    nearest.map(|(_, unit)| unit)
}

/// Where actor `index` stands to fight or hide from `target`: among its
/// squad's firing positions, the one nearest its combat range from the
/// target (fighting) or the nearest the target cannot see (hiding).
fn pick_goal(
    mission: &Mission,
    level: &Level,
    definition: &ActorVariant,
    (index, here): (usize, Vec3),
    target: Vec3,
) -> Option<Vec3> {
    let actor = &mission.actors[index];
    let AiReference::Squad(e, s) = actor.squad else {
        return None;
    };
    let encounter = level.scenario.campaign.encounters.get(usize::from(e))?;
    let squad = encounter.squads.get(usize::from(s))?;
    let groups = squad.firing_position_groups.iter().fold(0, |a, &g| a | g);
    let positions = encounter.firing_positions.iter().filter(|p| {
        u32::try_from(p.group)
            .ok()
            .and_then(|g| 1u32.checked_shl(g))
            .is_some_and(|bit| groups & bit != 0)
    });
    if definition.firing_range <= 0.0 && actor.state == ActorState::Fighting {
        // (with nothing to fire, it runs at what it fights, stopping
        // within its reach)
        let to = (target - here) * Vec3::new(1.0, 1.0, 0.0);
        let short = to.length() - MELEE_RANGE / 2.0;
        return Some(here + to.normalize_or_zero() * short.max(0.0));
    }
    let [near, far] = definition.combat_range;
    let middle = (near + far) / 2.0;
    let eye = Vec3::Z * (here.z - here.z.floor()).max(0.6);
    let mut best: Option<(f32, Vec3)> = None;
    for position in positions {
        let at = Vec3::from_array(position.position);
        let score = if actor.state == ActorState::Hiding {
            if mission.sim.level_clear(at + eye, target) {
                continue;
            }
            at.distance(here)
        } else {
            (at.distance(target) - middle).abs() + at.distance(here) / 2.0
        };
        if best.is_none_or(|(b, _)| score < b) {
            best = Some((score, at));
        }
    }
    best.map(|(_, at)| at).or_else(|| {
        // (no firing position: keep the range from where it stands)
        let away = (here - target).normalize_or_zero();
        let distance = here.distance(target);
        match actor.state {
            ActorState::Hiding => Some(here + away * 4.0),
            _ if distance > far || distance < near => Some(target + away * middle),
            _ => None,
        }
    })
}

/// Actor `index`'s input this tick.
fn think_one(mission: &mut Mission, level: &Level, others: &[Other], index: usize) -> TickInput {
    let Some(player) = mission.sim.actors().get(index) else {
        return TickInput::default();
    };
    let (eyes, feet) = (player.eyes(), player.mover().position);
    let (Radians(yaw), Radians(pitch)) = player.look();
    let (unit, team) = (mission.actors[index].unit, mission.actors[index].team);
    let Some(definition) = variant(level, mission.actors[index].variant) else {
        return TickInput::default();
    };
    if !mission.living(unit) {
        mission.actors[index].state = ActorState::Idle;
        return TickInput::default();
    }

    // (what it fights: the one it fights while it lives and is near, or
    // the nearest it now sees)
    let kept = mission.actors[index].target.and_then(|target| {
        let other = others.iter().find(|o| o.unit == target && o.alive)?;
        (eyes.distance(other.eyes) < definition.vision_distance * AWARENESS).then_some(target)
    });
    let seen = perceive(mission, definition, others, (index, eyes, yaw));
    let far = |unit: ObjectHandle| {
        others
            .iter()
            .find(|o| o.unit == unit)
            .map_or(f32::INFINITY, |o| eyes.distance(o.eyes))
    };
    // (it turns to one it sees much nearer than what it fights)
    let target = match (kept, seen) {
        (Some(kept), Some(seen)) if far(seen) * NEARER < far(kept) => Some(seen),
        (Some(kept), _) => Some(kept),
        (None, seen) => seen,
    };
    let target_eyes = target.and_then(|t| others.iter().find(|o| o.unit == t).map(|o| o.eyes));
    let (shield, body) = mission
        .sim
        .component::<Vitality>(unit)
        .map_or((1.0, 1.0), |vitality| (vitality.shield, vitality.body));
    let grenade = grenade_near(mission, others, team, feet);
    let actor = &mut mission.actors[index];
    actor.target = target;
    let hide = definition.shield_fraction_hide;
    let emerge = if definition.shield_fraction_emerge > 0.0 {
        definition.shield_fraction_emerge
    } else {
        1.0
    };
    let state = if actor.panic > 0 && target.is_some() {
        actor.panic -= 1;
        ActorState::Fleeing
    } else if target.is_none() {
        ActorState::Idle
    } else if shield < hide || (actor.state == ActorState::Hiding && shield < emerge) {
        ActorState::Hiding
    } else {
        ActorState::Fighting
    };
    if state != actor.state {
        actor.goal = None;
        actor.repick = 0;
    }
    let was = actor.state;
    actor.state = state;

    // (an impulse it plays holds it, turning only the way it plays to)
    if let Some((ticks, way)) = actor.impulse {
        actor.impulse = ticks.checked_sub(1).filter(|&t| t > 0).map(|t| (t, way));
        actor.body = body;
        let d_yaw = turn(yaw, libm::atan2f(way.y, way.x));
        return TickInput {
            yaw: Radians(d_yaw.clamp(-TURN, TURN)),
            ..TickInput::default()
        };
    }
    let senses = Senses {
        was,
        now: state,
        berserk: actor.berserk,
        target: target_eyes.map(|at| (at - eyes).truncate()),
        body: [actor.body, body],
        grenade,
    };
    actor.body = body;
    if let Some(reaction) = react(definition, senses) {
        if start_reaction(mission, definition, index, yaw, reaction) {
            return TickInput::default();
        }
    }
    let berserk = mission.actors[index].berserk;

    let mut input = TickInput::default();
    let mut facing = None;
    match (state, target_eyes) {
        (ActorState::Fleeing, Some(from)) => {
            (input.forward, input.right) = throttle(yaw, eyes - from);
            facing = Some(aim(from, eyes));
        }
        (ActorState::Fighting | ActorState::Hiding, Some(at)) => {
            let actor = &mut mission.actors[index];
            actor.repick = actor.repick.saturating_sub(1);
            // (one that charges follows its target every tick)
            let charging =
                (definition.firing_range <= 0.0 || berserk) && state == ActorState::Fighting;
            if actor.goal.is_none() || actor.repick == 0 || charging {
                let goal = pick_goal(mission, level, definition, (index, feet), at);
                let actor = &mut mission.actors[index];
                actor.goal = goal;
                actor.repick = REPICK_TICKS;
            }
            facing = Some(aim(eyes, at));
        }
        _ => {
            facing = follow_commands(mission, level, index, feet).or(facing);
        }
    }
    let actor = &mission.actors[index];
    if let Some(goal) = actor.goal.filter(|_| state != ActorState::Fleeing) {
        let to = goal - feet;
        if libm::hypotf(to.x, to.y) > ARRIVED {
            (input.forward, input.right) = throttle(yaw, to);
            if state == ActorState::Idle {
                facing = facing.or(Some(aim(feet, goal)));
            }
        }
    }

    // (turn toward what it faces, a little off while it fires)
    let firing = state == ActorState::Fighting
        && target_eyes.is_some_and(|at| {
            let distance = eyes.distance(at);
            (distance < definition.firing_range && !berserk || distance < MELEE)
                && mission.sim.level_clear(eyes, at)
                && !friend_between(mission, others, (unit, team), eyes, at)
        });
    if let Some((to_yaw, to_pitch)) = facing {
        let error = if firing {
            let e = definition.error_angle;
            mission.uniform([-e, e])
        } else {
            0.0
        };
        let (d_yaw, d_pitch) = (turn(yaw, to_yaw + error), to_pitch - pitch);
        input.yaw = Radians(d_yaw.clamp(-TURN, TURN));
        input.pitch = Radians(d_pitch.clamp(-TURN, TURN));
        if firing && d_yaw.abs() < ON_TARGET && d_pitch.abs() < ON_TARGET {
            input.buttons = fire(mission, definition, index, eyes, target_eyes);
        }
    }
    input
}

/// Starts the impulse `reaction` asks of actor `index`, facing `yaw`:
/// whether it plays one (a unit whose graph has none of it carries on,
/// though one surprised may still panic and one berserk still charges).
fn start_reaction(
    mission: &mut Mission,
    definition: &ActorVariant,
    index: usize,
    yaw: f32,
    reaction: Reaction,
) -> bool {
    let unit = mission.actors[index].unit;
    let facing = Vec2::new(libm::cosf(yaw), libm::sinf(yaw));
    let (impulse, way) = match reaction {
        Reaction::Surprise(way) => {
            if definition.actor_flags & ActorVariant::PANIC_ON_SURPRISE != 0 {
                mission.actors[index].panic = PANIC_TICKS;
            }
            choose_surprise(Some(way), facing)
        }
        Reaction::Berserk => {
            mission.actors[index].berserk = true;
            (AnimationImpulse::Berserk, facing)
        }
        Reaction::Dive(away) => {
            let sim = &mission.sim;
            let dive = choose_dive(facing, Escape::Forward, away, |impulse| {
                sim.has_animation_impulse(unit, impulse)
            });
            let Some(dive) = dive else {
                return false;
            };
            dive
        }
    };
    match mission.sim.start_animation_impulse(unit, impulse) {
        Some(frames) if frames > 0 => {
            mission.actors[index].impulse = Some((frames, way));
            true
        }
        _ => false,
    }
}

/// The way across the ground from `feet` to the nearest grenade an enemy
/// of `team` threw that flies within the dive radius of it.
fn grenade_near(mission: &Mission, others: &[Other], team: u16, feet: Vec3) -> Option<Vec2> {
    mission
        .sim
        .players()
        .iter()
        .chain(mission.sim.actors())
        .flat_map(|player| player.flights())
        .filter(|flight| flight.fuse.is_some())
        .filter(|flight| {
            others
                .iter()
                .find(|other| other.unit == flight.owner)
                .is_some_and(|other| mission.enemies(team, other.team))
        })
        .map(|flight| (flight.position - feet).truncate())
        .filter(|way| way.length() < DIVE_RADIUS)
        .min_by(|a, b| a.length().total_cmp(&b.length()))
}

/// Whether a living friend of `team` (other than `unit`) stands within
/// a body's width of the line from `eyes` to `at`, nearer than `at`: an
/// actor holds its fire then.
fn friend_between(
    mission: &Mission,
    others: &[Other],
    (unit, team): (ObjectHandle, u16),
    eyes: Vec3,
    at: Vec3,
) -> bool {
    let line = at - eyes;
    let length = line.length();
    if length == 0.0 {
        return false;
    }
    let along = line / length;
    others
        .iter()
        .filter(|o| o.alive && o.unit != unit && !mission.enemies(team, o.team))
        .any(|o| {
            let to = o.eyes - eyes;
            let ahead = to.dot(along);
            ahead > 0.0 && ahead < length && (to - along * ahead).length() < FRIEND_WIDTH
        })
}

/// The buttons actor `index` presses at its target: a strike when it is
/// near and has no gun, else its trigger through each burst and not
/// between them.
fn fire(
    mission: &mut Mission,
    definition: &ActorVariant,
    index: usize,
    eyes: Vec3,
    target: Option<Vec3>,
) -> Buttons {
    let near = target.is_some_and(|at| eyes.distance(at) < MELEE);
    if definition.firing_range <= 0.0 {
        // (a strike is a press: the button is let go between)
        let press = near && mission.sim.tick().0.is_multiple_of(2);
        return if press {
            Buttons::MELEE
        } else {
            Buttons::default()
        };
    }
    let actor = &mission.actors[index];
    if actor.wait > 0 {
        mission.actors[index].wait -= 1;
        return Buttons::default();
    }
    if actor.burst == 0 {
        let burst = mission.ticks(definition.burst_duration);
        mission.actors[index].burst = burst;
    }
    let actor = &mut mission.actors[index];
    actor.burst -= 1;
    if actor.burst == 0 {
        let wait = mission.ticks(definition.burst_separation);
        mission.actors[index].wait = wait;
    }
    Buttons::FIRE
}

/// Moves actor `index` along its command list (pauses and walks to
/// points; the other atoms are passed over); the facing it asks for.
fn follow_commands(
    mission: &mut Mission,
    level: &Level,
    index: usize,
    feet: Vec3,
) -> Option<(f32, f32)> {
    let unit = mission.actors[index].unit;
    let Some(&list) = mission.ai.command_lists.get(&unit) else {
        mission.actors[index].command = None;
        return None;
    };
    let actor = &mut mission.actors[index];
    let (step, wait) = match actor.command {
        Some((current, step, wait)) if current == list => (step, wait),
        _ => (0, FRESH),
    };
    let Some(commands) = element(&level.scenario.campaign.command_lists, list) else {
        mission.ai.command_lists.remove(&unit);
        return None;
    };
    let Some(command) = commands.commands.get(step) else {
        // (the list is done)
        actor.command = None;
        actor.goal = None;
        mission.ai.command_lists.remove(&unit);
        return None;
    };
    let point = |slot: usize| {
        element(&commands.points, i32::from(command.points[slot])).map(|p| Vec3::from_array(*p))
    };
    let mut facing = None;
    let next = match command.atom {
        PAUSE => {
            let wait = if wait == FRESH {
                #[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
                // (clamped below a u16's greatest first)
                let ticks = (command.parameters[0] * TICKS).clamp(0.0, f32::from(FRESH - 1)) as u16;
                ticks
            } else {
                wait
            };
            if wait > 0 {
                actor.command = Some((list, step, wait - 1));
                return None;
            }
            true
        }
        GO_TO | GO_TO_AND_FACE => match point(0) {
            Some(goal) => {
                actor.goal = Some(goal);
                if command.atom == GO_TO_AND_FACE {
                    facing = point(1).map(|p| aim(goal, p));
                }
                libm::hypotf(goal.x - feet.x, goal.y - feet.y) <= ARRIVED
            }
            None => true,
        },
        _ => true,
    };
    let step = if next { step + 1 } else { step };
    actor.command = Some((list, step, if next { FRESH } else { 0 }));
    if next {
        actor.goal = None;
    }
    facing
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn an_encounters_team_is_its_own_or_its_actors() {
        assert_eq!(team_of(4, 0), 4);
        assert_eq!(team_of(0, 7), HUMAN_TEAM);
        assert_eq!(team_of(0, 10), FLOOD_TEAM);
        assert_eq!(team_of(-1, 13), SENTINEL_TEAM);
        assert_eq!(team_of(0, 2), COVENANT_TEAM);
    }

    #[test]
    fn a_squads_count_rises_with_the_difficulty() {
        assert_eq!(squad_count([2, 6], 8, Difficulty::Normal), 2);
        assert_eq!(squad_count([2, 6], 8, Difficulty::Hard), 4);
        assert_eq!(squad_count([2, 6], 8, Difficulty::Legendary), 6);
        assert_eq!(squad_count([2, 6], 3, Difficulty::Legendary), 3);
        assert_eq!(squad_count([0, 0], 5, Difficulty::Easy), 5);
    }

    #[test]
    fn the_throttle_moves_toward_the_direction() {
        // (facing along x: ahead is forward, along y is to the left)
        assert_eq!(throttle(0.0, Vec3::X), (127, 0));
        assert_eq!(throttle(0.0, Vec3::Y), (0, -127));
        let (forward, right) = throttle(std::f32::consts::FRAC_PI_2, Vec3::Y);
        assert_eq!((forward, right), (127, 0));
    }

    /// An actor variant with the given flags, surprise distance, berserk
    /// damage and berserk distance (a grunt, an elite and so on).
    fn variant_of(flags: u32, surprise: f32, damage: [f32; 2], distance: f32) -> ActorVariant {
        ActorVariant {
            tag: opence_content::TagRef {
                group: opence_content::Group(*b"actv"),
                path: String::new(),
            },
            unit: None,
            weapon: None,
            kind: 0,
            vision_distance: 20.0,
            vision_angle: 1.0,
            hearing_distance: 10.0,
            firing_range: 15.0,
            rate_of_fire: 1.0,
            error_angle: 0.0,
            combat_range: [2.0, 10.0],
            burst_duration: [1.0, 1.0],
            burst_separation: [1.0, 1.0],
            panic_chance: 0.0,
            shield_fraction_hide: 0.0,
            shield_fraction_emerge: 0.0,
            actor_flags: flags,
            surprise_distance: surprise,
            berserk_damage: damage,
            berserk_distance: distance,
        }
    }

    fn senses(was: ActorState, now: ActorState, target: Option<Vec2>) -> Senses {
        Senses {
            was,
            now,
            berserk: false,
            target,
            body: [1.0, 1.0],
            grenade: None,
        }
    }

    #[test]
    fn a_grunt_is_surprised_by_an_enemy_near_as_it_first_sees_one() {
        let grunt = variant_of(ActorVariant::PANIC_ON_SURPRISE, 4.0, [0.0, 0.0], 0.0);
        let near = Some(Vec2::new(2.0, 0.0));
        let sees = senses(ActorState::Idle, ActorState::Fighting, near);
        assert_eq!(
            react(&grunt, sees),
            Some(Reaction::Surprise(Vec2::new(2.0, 0.0)))
        );
        // (one far, or one it already fought, does not surprise it)
        let far = senses(
            ActorState::Idle,
            ActorState::Fighting,
            Some(Vec2::new(9.0, 0.0)),
        );
        assert_eq!(react(&grunt, far), None);
        let fought = senses(ActorState::Fighting, ActorState::Fighting, near);
        assert_eq!(react(&grunt, fought), None);
        // (one seen from cover surprises it too)
        let hiding = senses(ActorState::Idle, ActorState::Hiding, near);
        assert!(matches!(react(&grunt, hiding), Some(Reaction::Surprise(_))));
    }

    #[test]
    fn an_elite_goes_berserk_when_wounded_or_near() {
        let elite = variant_of(0, 0.0, [0.3, 0.5], 1.5);
        let mut wounded = senses(
            ActorState::Fighting,
            ActorState::Fighting,
            Some(Vec2::X * 8.0),
        );
        wounded.body = [0.8, 0.4];
        assert_eq!(react(&elite, wounded), Some(Reaction::Berserk));
        // (a small wound, or one that leaves it above the threshold, does not)
        wounded.body = [0.6, 0.4];
        assert_eq!(react(&elite, wounded), None);
        wounded.body = [1.0, 0.6];
        assert_eq!(react(&elite, wounded), None);
        let near = senses(ActorState::Fighting, ActorState::Fighting, Some(Vec2::X));
        assert_eq!(react(&elite, near), Some(Reaction::Berserk));
        // (it goes berserk once, and only as it fights)
        let mut again = near;
        again.berserk = true;
        assert_eq!(react(&elite, again), None);
        let fleeing = senses(ActorState::Fighting, ActorState::Fleeing, Some(Vec2::X));
        assert_eq!(react(&elite, fleeing), None);
    }

    #[test]
    fn a_flood_form_is_always_berserk_as_it_fights() {
        let flood = variant_of(
            ActorVariant::ALWAYS_BERSERK_IN_ATTACKING_MODE,
            0.0,
            [0.0, 0.0],
            0.0,
        );
        let far = senses(
            ActorState::Fighting,
            ActorState::Fighting,
            Some(Vec2::X * 30.0),
        );
        assert_eq!(react(&flood, far), Some(Reaction::Berserk));
        let idle = senses(ActorState::Idle, ActorState::Idle, None);
        assert_eq!(react(&flood, idle), None);
    }

    #[test]
    fn a_marine_dives_away_from_a_grenade_before_anything_else() {
        let marine = variant_of(ActorVariant::PANIC_ON_SURPRISE, 4.0, [0.0, 0.0], 5.0);
        let mut grenade = senses(ActorState::Idle, ActorState::Fighting, Some(Vec2::X));
        grenade.grenade = Some(Vec2::new(0.0, 2.0));
        assert_eq!(
            react(&marine, grenade),
            Some(Reaction::Dive(Vec2::new(0.0, -1.0)))
        );
        // (one berserk does not dive)
        grenade.berserk = true;
        assert!(matches!(
            react(&marine, grenade),
            Some(Reaction::Surprise(_))
        ));
    }

    #[test]
    fn a_turn_goes_the_shorter_way() {
        let t = turn(3.0, -3.0);
        assert!(t > 0.0 && t < 0.3, "{t}");
    }
}
