//! A client: it sends its inputs, predicts its own player with them, and
//! corrects the prediction by the host's snapshots; the other players are
//! players of its world too, placed between the last two snapshots.

use std::collections::VecDeque;
use std::net::SocketAddr;

use opence_content::Level;
use opence_sim::input::TickInput;
use opence_sim::player::PlayerError;
use opence_sim::units::Radians;
use opence_sim::{Simulation, Vec3};

use crate::host::tick_input;
use crate::packet::{InputFrame, Packet, Refusal, MAX_INPUT_FRAMES};
use crate::snapshot::Snapshot;
use crate::transport::Transport;
use crate::{decode, encode};

/// How far the prediction may stray from the host before it is corrected,
/// in world units.
pub const TOLERANCE: f32 = 0.05;

/// How many snapshots it keeps to apply changes to.
const HISTORY: usize = 32;

/// How often it says hello until welcomed, in ticks.
const HELLO_TICKS: u32 = 30;

/// Where it predicted its player after one of its ticks.
#[derive(Clone, Copy, Debug)]
struct Predicted {
    tick: u32,
    position: Vec3,
    velocity: Vec3,
    yaw: f32,
}

/// The facing of a snapshot's rotation (a turn about Z).
fn yaw_of(rotation: [f32; 4]) -> f32 {
    2.0 * libm::atan2f(rotation[2], rotation[3])
}

/// The turn from `from` to `to`, the short way round.
fn turn_between(from: f32, to: f32) -> f32 {
    libm::remainderf(to - from, std::f32::consts::TAU)
}

/// Where a client is in joining.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Joining {
    /// It waits for the host's welcome.
    Waiting,
    /// It plays as the host's player `player`.
    Playing {
        /// Its player's index on the host.
        player: usize,
    },
    /// The host refused it.
    Refused(Refusal),
}

/// A client's game, predicted in a simulation it is given each tick, its
/// own player the simulation's first.
pub struct Client {
    host: SocketAddr,
    name: String,
    joining: Joining,
    /// The other players: their index on the host, and in the simulation.
    remotes: Vec<(u32, usize)>,
    tick: u32,
    unacked: VecDeque<InputFrame>,
    predicted: VecDeque<Predicted>,
    snapshots: VecDeque<Snapshot>,
    since_snapshot: u32,
    last_error: Option<f32>,
    corrections: u32,
}

impl Client {
    /// A client of the host at `host` on `level`, called `name`, playing
    /// in `simulation`; its own player stands at the first start (unless
    /// it has one) until the host places it.
    pub fn new(
        simulation: &mut Simulation,
        host: SocketAddr,
        name: &str,
        level: &Level,
    ) -> Result<Self, PlayerError> {
        if simulation.players().is_empty() {
            simulation.spawn_player(level, 0)?;
        }
        Ok(Self {
            host,
            name: name.into(),
            joining: Joining::Waiting,
            remotes: Vec::new(),
            tick: 0,
            unacked: VecDeque::new(),
            predicted: VecDeque::new(),
            snapshots: VecDeque::new(),
            since_snapshot: 0,
            last_error: None,
            corrections: 0,
        })
    }

    /// Where it is in joining.
    pub fn joining(&self) -> Joining {
        self.joining
    }

    /// How far its prediction was from the host's at the last snapshot,
    /// before any correction.
    pub fn last_error(&self) -> Option<f32> {
        self.last_error
    }

    /// How many times the host's snapshots corrected its prediction.
    pub fn corrections(&self) -> u32 {
        self.corrections
    }

    /// The other players where it draws them, by their index on the
    /// host: between the last two snapshots, a snapshot's time behind the
    /// host, with their velocity and facing.
    pub fn others(&self) -> Vec<(u32, Vec3, Vec3, f32)> {
        let Joining::Playing { player } = self.joining else {
            return Vec::new();
        };
        let (Some(latest), previous) = (self.snapshots.back(), self.snapshots.iter().rev().nth(1))
        else {
            return Vec::new();
        };
        let span = previous.map_or(1, |previous| {
            latest.tick.saturating_sub(previous.tick).max(1)
        });
        let alpha = (self.since_snapshot as f32 / span as f32).min(1.0);
        latest
            .objects
            .iter()
            .filter(|object| object.id as usize != player)
            .map(|object| {
                let from = previous
                    .and_then(|previous| previous.objects.iter().find(|o| o.id == object.id))
                    .unwrap_or(object);
                let at =
                    Vec3::from_array(from.position).lerp(Vec3::from_array(object.position), alpha);
                let (yaw, to) = (yaw_of(from.rotation), yaw_of(object.rotation));
                let facing = yaw + turn_between(yaw, to) * alpha;
                (object.id, at, Vec3::from_array(object.velocity), facing)
            })
            .collect()
    }

    /// One tick of `simulation` with its player's `input`: it reads the
    /// host's packets, predicts the tick, places the other players and
    /// sends its inputs.
    pub fn step(
        &mut self,
        simulation: &mut Simulation,
        transport: &mut dyn Transport,
        level: &Level,
        input: TickInput,
    ) {
        while let Some((from, bytes)) = transport.receive() {
            if from == self.host {
                self.receive(simulation, &bytes);
            }
        }
        self.since_snapshot += 1;
        match self.joining {
            Joining::Waiting => {
                if self.tick.is_multiple_of(HELLO_TICKS) {
                    let hello = Packet::Hello {
                        name: self.name.clone(),
                    };
                    self.send(transport, &hello);
                }
                self.tick += 1;
            }
            Joining::Refused(_) => {}
            Joining::Playing { .. } => {
                self.tick += 1;
                let frame = InputFrame {
                    tick: self.tick,
                    buttons: input.buttons.0,
                    forward: i16::from(input.forward),
                    strafe: i16::from(input.right),
                    yaw: input.yaw.0,
                    pitch: input.pitch.0,
                };
                self.unacked.push_back(frame);
                while self.unacked.len() > MAX_INPUT_FRAMES {
                    self.unacked.pop_front();
                }
                // (the same input the host will play, as it will read it;
                // the others stand, to be placed)
                simulation.step(tick_input(&frame));
                self.place_others(simulation, level);
                if let Some(player) = simulation.player() {
                    self.predicted.push_back(Predicted {
                        tick: self.tick,
                        position: player.mover().position,
                        velocity: player.mover().velocity,
                        yaw: player.look().0 .0,
                    });
                }
                while self.predicted.len() > 4 * HISTORY {
                    self.predicted.pop_front();
                }
                let inputs = Packet::Inputs {
                    ack: self.snapshots.back().map(|snapshot| snapshot.tick),
                    frames: self.unacked.iter().copied().collect(),
                };
                self.send(transport, &inputs);
            }
        }
    }

    /// Puts the other players where it draws them, adding a player to the
    /// simulation for each it has not seen before.
    fn place_others(&mut self, simulation: &mut Simulation, level: &Level) {
        for (id, position, velocity, yaw) in self.others() {
            let local = match self.remotes.iter().find(|(remote, _)| *remote == id) {
                Some(&(_, local)) => local,
                None => {
                    let Ok((local, _)) = simulation.add_player(level, 0, None) else {
                        continue;
                    };
                    self.remotes.push((id, local));
                    local
                }
            };
            simulation.correct_player(local, position, velocity, Radians(yaw));
        }
    }

    fn receive(&mut self, simulation: &mut Simulation, bytes: &[u8]) {
        let Ok(packet) = decode(bytes) else {
            return;
        };
        match packet {
            Packet::Welcome { player, .. } if self.joining == Joining::Waiting => {
                self.joining = Joining::Playing {
                    player: usize::from(player),
                };
                self.tick = 0;
            }
            Packet::Refused(refusal) if self.joining == Joining::Waiting => {
                self.joining = Joining::Refused(refusal);
            }
            Packet::Snapshot { input_ack, delta } => {
                let Joining::Playing { player } = self.joining else {
                    return;
                };
                let base = delta
                    .base
                    .and_then(|tick| self.snapshots.iter().find(|s| s.tick == tick));
                if delta.base.is_some() && base.is_none() {
                    return; // (its base is gone: wait for one against a newer)
                }
                if self.snapshots.back().is_some_and(|s| s.tick >= delta.tick) {
                    return; // (late)
                }
                let Some(snapshot) = delta.apply(base) else {
                    return;
                };
                if let Some(acked) = input_ack {
                    self.unacked.retain(|frame| frame.tick > acked);
                    self.reconcile(simulation, &snapshot, player, acked);
                }
                self.snapshots.push_back(snapshot);
                while self.snapshots.len() > HISTORY {
                    self.snapshots.pop_front();
                }
                self.since_snapshot = 0;
            }
            _ => {}
        }
    }

    /// Corrects its player by the host's: where the host put it and how
    /// it faced after input `acked`, against what it predicted then. The
    /// errors are moved onto where it is now (the inputs since are kept,
    /// not replayed).
    fn reconcile(
        &mut self,
        simulation: &mut Simulation,
        snapshot: &Snapshot,
        player: usize,
        acked: u32,
    ) {
        let Some(host) = snapshot.objects.iter().find(|o| o.id as usize == player) else {
            return;
        };
        let Some(predicted) = self.predicted.iter().find(|p| p.tick == acked).copied() else {
            return;
        };
        self.predicted.retain(|p| p.tick >= acked);
        let error = Vec3::from_array(host.position) - predicted.position;
        let turn = turn_between(predicted.yaw, yaw_of(host.rotation));
        self.last_error = Some(error.length());
        if error.length() <= TOLERANCE && turn.abs() <= TOLERANCE {
            return;
        }
        let speed = Vec3::from_array(host.velocity) - predicted.velocity;
        let Some((now, velocity, yaw)) = simulation
            .player()
            .map(|p| (p.mover().position, p.mover().velocity, p.look().0 .0))
        else {
            return;
        };
        if simulation.correct_player(0, now + error, velocity + speed, Radians(yaw + turn)) {
            self.corrections += 1;
            for later in &mut self.predicted {
                later.position += error;
                later.velocity += speed;
                later.yaw += turn;
            }
        }
    }

    /// Tells the host it leaves.
    pub fn leave(&mut self, transport: &mut dyn Transport) {
        self.send(transport, &Packet::Leave);
    }

    fn send(&self, transport: &mut dyn Transport, packet: &Packet) {
        if let Ok(bytes) = encode(packet) {
            transport.send(self.host, &bytes);
        }
    }
}
