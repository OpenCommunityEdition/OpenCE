//! The host: it plays the game, takes its clients' inputs, and sends each
//! client the world as the changes since the snapshot it acknowledged.

use std::collections::{BTreeMap, VecDeque};
use std::net::SocketAddr;

use opence_content::Level;
use opence_sim::clock::TICKS_PER_SECOND;
use opence_sim::game::{Game, GameEvent, Variant};
use opence_sim::input::TickInput;
use opence_sim::player::PlayerError;
use opence_sim::Simulation;

use crate::packet::{InputFrame, Packet, Refusal};
use crate::snapshot::{ObjectState, Snapshot};
use crate::transport::Transport;
use crate::{decode, encode, RateLimit, MAX_PLAYERS};

/// How many snapshots the host keeps to send changes from.
const HISTORY: usize = 64;

/// How many of a client's inputs the host holds ahead of the one it plays.
const MAX_QUEUED_INPUTS: usize = 64;

/// How long a silent client stays in the game: 10 seconds.
const TIMEOUT_TICKS: u32 = 10 * TICKS_PER_SECOND;

/// The packets a client may send a second (inputs every tick, and room
/// to spare).
const PACKETS_PER_SECOND: u32 = 2 * TICKS_PER_SECOND;

#[derive(Debug)]
struct Peer {
    address: SocketAddr,
    player: usize,
    rate: RateLimit,
    queued: BTreeMap<u32, InputFrame>,
    last: InputFrame,
    next_input: Option<u32>,
    input_ack: Option<u32>,
    snapshot_ack: Option<u32>,
    silent_ticks: u32,
}

/// A frame of input as the simulation's.
pub fn tick_input(frame: &InputFrame) -> TickInput {
    let axis = |value: i16| i8::try_from(value.clamp(-127, 127)).unwrap_or(0);
    TickInput {
        buttons: opence_sim::input::Buttons(frame.buttons),
        forward: axis(frame.forward),
        right: axis(frame.strafe),
        up: 0,
        yaw: opence_sim::units::Radians(frame.yaw.clamp(-1.0, 1.0)),
        pitch: opence_sim::units::Radians(frame.pitch.clamp(-1.0, 1.0)),
    }
}

/// The players' state, by player index, as a snapshot at `tick`.
pub fn players_snapshot(simulation: &Simulation, tick: u32) -> Snapshot {
    let objects = simulation
        .players()
        .iter()
        .enumerate()
        .filter_map(|(index, player)| {
            // (a player's rotation is its facing, seated or not)
            simulation.transform(player.handle())?;
            Some(ObjectState {
                id: u32::try_from(index).ok()?,
                definition: 0,
                position: player.mover().position.to_array(),
                rotation: opence_sim::Quat::from_rotation_z(player.look().0 .0).to_array(),
                velocity: player.mover().velocity.to_array(),
                body: 1.0,
                shield: 1.0,
            })
        })
        .collect();
    Snapshot { tick, objects }
}

/// A host's game.
pub struct Host {
    simulation: Simulation,
    game: Game,
    level_name: String,
    game_type: u8,
    peers: Vec<Peer>,
    history: VecDeque<Snapshot>,
    tick: u32,
    seed: u32,
}

impl Host {
    /// Starts a game of `variant` on `level` (called `level_name` to the
    /// clients), its own player at the first start.
    pub fn new(level: &Level, level_name: &str, variant: Variant) -> Result<Self, PlayerError> {
        let mut simulation = Simulation::empty();
        simulation.spawn_player(level, 0)?;
        let game = Game::start(&mut simulation, level, variant);
        Ok(Self {
            simulation,
            game,
            level_name: level_name.into(),
            game_type: variant.game_type as u8,
            peers: Vec::new(),
            history: VecDeque::new(),
            tick: 0,
            seed: opence_sim::random::DEFAULT_SEED,
        })
    }

    /// The game's world.
    pub fn simulation(&self) -> &Simulation {
        &self.simulation
    }

    /// The game's rules and scores.
    pub fn game(&self) -> &Game {
        &self.game
    }

    /// The clients' players' indices.
    pub fn client_players(&self) -> Vec<usize> {
        self.peers.iter().map(|peer| peer.player).collect()
    }

    /// The host's tick.
    pub fn tick(&self) -> u32 {
        self.tick
    }

    /// One tick: it reads its clients' packets, plays the tick with their
    /// inputs and its own player's `input`, and sends them its world.
    pub fn step(
        &mut self,
        transport: &mut dyn Transport,
        level: &Level,
        input: TickInput,
    ) -> Vec<GameEvent> {
        for peer in &mut self.peers {
            peer.rate.tick();
            peer.silent_ticks += 1;
        }
        while let Some((from, bytes)) = transport.receive() {
            self.receive(transport, level, from, &bytes);
        }
        self.peers.retain(|peer| peer.silent_ticks < TIMEOUT_TICKS);

        let mut inputs = vec![TickInput::default(); self.simulation.players().len()];
        if let Some(first) = inputs.first_mut() {
            *first = input;
        }
        for peer in &mut self.peers {
            // (one of its ticks for each of the host's, from its first
            // input on; when that tick's input has not come the last is
            // played again, and it is not played when it comes late)
            if let Some(next) = peer.next_input {
                if let Some(frame) = peer.queued.remove(&next) {
                    peer.last = frame;
                }
                peer.input_ack = Some(next);
                peer.next_input = next.checked_add(1);
                peer.queued.retain(|&tick, _| tick > next);
            }
            if let Some(slot) = inputs.get_mut(peer.player) {
                *slot = tick_input(&peer.last);
            }
        }
        self.simulation.step_all(&inputs);
        let events = self.game.step(&mut self.simulation, level);
        self.tick += 1;

        let snapshot = players_snapshot(&self.simulation, self.tick);
        for peer in &self.peers {
            let base = peer
                .snapshot_ack
                .and_then(|ack| self.history.iter().find(|old| old.tick == ack));
            let packet = Packet::Snapshot {
                input_ack: peer.input_ack,
                delta: snapshot.delta(base),
            };
            if let Ok(bytes) = encode(&packet) {
                transport.send(peer.address, &bytes);
            }
        }
        self.history.push_back(snapshot);
        if self.history.len() > HISTORY {
            self.history.pop_front();
        }
        events
    }

    fn receive(
        &mut self,
        transport: &mut dyn Transport,
        level: &Level,
        from: SocketAddr,
        bytes: &[u8],
    ) {
        let Ok(packet) = decode(bytes) else {
            return;
        };
        let known = self.peers.iter().position(|peer| peer.address == from);
        if let Some(at) = known {
            let peer = &mut self.peers[at];
            if !peer.rate.allow() {
                return;
            }
            peer.silent_ticks = 0;
        }
        match (packet, known) {
            (Packet::Discover, _) => {
                let announce = Packet::Announce {
                    name: "host".into(),
                    level: self.level_name.clone(),
                    game_type: self.game_type,
                    players: u8::try_from(self.simulation.players().len()).unwrap_or(u8::MAX),
                    capacity: u8::try_from(MAX_PLAYERS).unwrap_or(u8::MAX),
                };
                self.send(transport, from, &announce);
            }
            (Packet::Hello { .. }, Some(at)) => {
                let player = self.peers[at].player;
                self.welcome(transport, from, player);
            }
            (Packet::Hello { .. }, None) => {
                if self.game.rules().is_over() {
                    self.send(transport, from, &Packet::Refused(Refusal::Over));
                    return;
                }
                if self.simulation.players().len() >= MAX_PLAYERS {
                    self.send(transport, from, &Packet::Refused(Refusal::Full));
                    return;
                }
                let index = self.simulation.players().len();
                let team = u16::try_from(index % 2).unwrap_or(0);
                let start = index % level.scenario.player_starts.len().max(1);
                let teams = self.game.rules().variant().teams;
                let Ok((player, _)) =
                    self.simulation
                        .add_player(level, start, teams.then_some(team))
                else {
                    return;
                };
                self.game.join(team);
                self.peers.push(Peer {
                    address: from,
                    player,
                    rate: RateLimit::new(PACKETS_PER_SECOND, TICKS_PER_SECOND, TICKS_PER_SECOND),
                    queued: BTreeMap::new(),
                    last: InputFrame::default(),
                    next_input: None,
                    input_ack: None,
                    snapshot_ack: None,
                    silent_ticks: 0,
                });
                self.welcome(transport, from, player);
            }
            (Packet::Inputs { ack, frames }, Some(at)) => {
                let tick = self.tick;
                let peer = &mut self.peers[at];
                // (a snapshot it says it has must be one that was sent)
                if ack.is_some_and(|ack| ack <= tick) {
                    peer.snapshot_ack = ack.max(peer.snapshot_ack);
                }
                if peer.next_input.is_none() {
                    peer.next_input = frames.first().map(|frame| frame.tick);
                }
                for frame in frames {
                    let new = peer.input_ack.is_none_or(|played| frame.tick > played);
                    if new && peer.queued.len() < MAX_QUEUED_INPUTS {
                        peer.queued.insert(frame.tick, frame);
                    }
                }
            }
            (Packet::Leave, Some(at)) => {
                // (its biped stays, still, until the game ends)
                self.peers.remove(at);
            }
            _ => {}
        }
    }

    fn welcome(&self, transport: &mut dyn Transport, to: SocketAddr, player: usize) {
        let welcome = Packet::Welcome {
            player: u8::try_from(player).unwrap_or(u8::MAX),
            tick: self.tick,
            seed: self.seed,
            level: self.level_name.clone(),
            game_type: self.game_type,
        };
        self.send(transport, to, &welcome);
    }

    fn send(&self, transport: &mut dyn Transport, to: SocketAddr, packet: &Packet) {
        if let Ok(bytes) = encode(packet) {
            transport.send(to, &bytes);
        }
    }
}
