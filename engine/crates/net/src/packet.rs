//! The packets: each starts with [`MAGIC`], the protocol's version and
//! its kind.

use crate::snapshot::SnapshotDelta;
use crate::wire::{Reader, Writer};
use crate::{DecodeError, EncodeError, MAGIC, MAX_PACKET_BYTES, MAX_PLAYERS, PROTOCOL_VERSION};

/// The most input frames in one packet (a client resends the ones the
/// host has not acknowledged).
pub const MAX_INPUT_FRAMES: usize = 16;

/// The longest name, in bytes.
pub const MAX_NAME_BYTES: usize = 32;

/// The longest level name, in bytes.
pub const MAX_LEVEL_BYTES: usize = 64;

/// A tick's input from a client.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct InputFrame {
    /// The tick it is for.
    pub tick: u32,
    /// The buttons held.
    pub buttons: u32,
    /// Forward and back, from -32767 to 32767.
    pub forward: i16,
    /// Left and right, from -32767 to 32767.
    pub strafe: i16,
    /// How far it turned, in radians.
    pub yaw: f32,
    /// How far it looked up, in radians.
    pub pitch: f32,
}

/// Why the host refused a client.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Refusal {
    /// The game is full.
    Full,
    /// The game has ended.
    Over,
    /// The client is not welcome.
    Banned,
}

/// A packet.
#[derive(Clone, Debug, PartialEq)]
pub enum Packet {
    /// A client asks to join.
    Hello {
        /// Its player's name.
        name: String,
    },
    /// The host lets a client join.
    Welcome {
        /// Its player's index.
        player: u8,
        /// The host's tick now.
        tick: u32,
        /// The game's random seed.
        seed: u32,
        /// The level.
        level: String,
        /// The game type (Halo's number).
        game_type: u8,
    },
    /// The host refuses a client.
    Refused(Refusal),
    /// A client's inputs.
    Inputs {
        /// The last snapshot it has, for the host to send changes from.
        ack: Option<u32>,
        /// Its inputs, oldest first, in order of tick.
        frames: Vec<InputFrame>,
    },
    /// The host's world.
    Snapshot {
        /// The last of the client's inputs the host has played.
        input_ack: Option<u32>,
        /// The world, as the changes from the client's last snapshot.
        delta: SnapshotDelta,
    },
    /// A machine leaves the game.
    Leave,
    /// A client looks for games on its network.
    Discover,
    /// A host answers: its game.
    Announce {
        /// The host's name.
        name: String,
        /// The level.
        level: String,
        /// The game type (Halo's number).
        game_type: u8,
        /// The players in it.
        players: u8,
        /// The most it takes.
        capacity: u8,
    },
}

const HELLO: u8 = 1;
const WELCOME: u8 = 2;
const REFUSED: u8 = 3;
const INPUTS: u8 = 4;
const SNAPSHOT: u8 = 5;
const LEAVE: u8 = 6;
const DISCOVER: u8 = 7;
const ANNOUNCE: u8 = 8;

fn tick_or_none(tick: Option<u32>) -> u32 {
    tick.unwrap_or(u32::MAX)
}

fn none_or_tick(tick: u32) -> Option<u32> {
    (tick != u32::MAX).then_some(tick)
}

/// A packet's bytes.
pub fn encode(packet: &Packet) -> Result<Vec<u8>, EncodeError> {
    let mut out = Writer::default();
    out.put(&MAGIC);
    out.u16(PROTOCOL_VERSION);
    match packet {
        Packet::Hello { name } => {
            out.u8(HELLO);
            out.text(name, MAX_NAME_BYTES)?;
        }
        Packet::Welcome {
            player,
            tick,
            seed,
            level,
            game_type,
        } => {
            if usize::from(*player) >= MAX_PLAYERS {
                return Err(EncodeError::BadCount);
            }
            out.u8(WELCOME);
            out.u8(*player);
            out.u32(*tick);
            out.u32(*seed);
            out.text(level, MAX_LEVEL_BYTES)?;
            out.u8(*game_type);
        }
        Packet::Refused(refusal) => {
            out.u8(REFUSED);
            out.u8(match refusal {
                Refusal::Full => 0,
                Refusal::Over => 1,
                Refusal::Banned => 2,
            });
        }
        Packet::Inputs { ack, frames } => {
            out.u8(INPUTS);
            out.u32(tick_or_none(*ack));
            out.count(frames.len(), MAX_INPUT_FRAMES)?;
            for frame in frames {
                out.u32(frame.tick);
                out.u32(frame.buttons);
                out.i16(frame.forward);
                out.i16(frame.strafe);
                out.f32(frame.yaw);
                out.f32(frame.pitch);
            }
        }
        Packet::Snapshot { input_ack, delta } => {
            out.u8(SNAPSHOT);
            out.u32(tick_or_none(*input_ack));
            delta.write(&mut out)?;
        }
        Packet::Leave => out.u8(LEAVE),
        Packet::Discover => out.u8(DISCOVER),
        Packet::Announce {
            name,
            level,
            game_type,
            players,
            capacity,
        } => {
            out.u8(ANNOUNCE);
            out.text(name, MAX_NAME_BYTES)?;
            out.text(level, MAX_LEVEL_BYTES)?;
            out.u8(*game_type);
            out.u8(*players);
            out.u8(*capacity);
        }
    }
    out.finish()
}

/// A packet from its bytes, refused if anything in it is out of bounds.
pub fn decode(bytes: &[u8]) -> Result<Packet, DecodeError> {
    if bytes.len() > MAX_PACKET_BYTES {
        return Err(DecodeError::TooLarge);
    }
    let mut input = Reader::new(bytes);
    if input.take(MAGIC.len())? != MAGIC {
        return Err(DecodeError::BadMagic);
    }
    let version = input.u16()?;
    if version != PROTOCOL_VERSION {
        return Err(DecodeError::WrongVersion(version));
    }
    let packet = match input.u8()? {
        HELLO => Packet::Hello {
            name: input.text(MAX_NAME_BYTES)?,
        },
        WELCOME => {
            let player = input.u8()?;
            if usize::from(player) >= MAX_PLAYERS {
                return Err(DecodeError::BadIndex);
            }
            Packet::Welcome {
                player,
                tick: input.u32()?,
                seed: input.u32()?,
                level: input.text(MAX_LEVEL_BYTES)?,
                game_type: input.u8()?,
            }
        }
        REFUSED => Packet::Refused(match input.u8()? {
            0 => Refusal::Full,
            1 => Refusal::Over,
            2 => Refusal::Banned,
            _ => return Err(DecodeError::BadValue),
        }),
        INPUTS => {
            let ack = none_or_tick(input.u32()?);
            let count = input.count(MAX_INPUT_FRAMES)?;
            let mut frames: Vec<InputFrame> = Vec::with_capacity(count);
            for _ in 0..count {
                let frame = InputFrame {
                    tick: input.u32()?,
                    buttons: input.u32()?,
                    forward: input.i16()?,
                    strafe: input.i16()?,
                    yaw: input.f32()?,
                    pitch: input.f32()?,
                };
                if frames.last().is_some_and(|last| frame.tick <= last.tick) {
                    return Err(DecodeError::BadIndex);
                }
                frames.push(frame);
            }
            Packet::Inputs { ack, frames }
        }
        SNAPSHOT => Packet::Snapshot {
            input_ack: none_or_tick(input.u32()?),
            delta: SnapshotDelta::read(&mut input)?,
        },
        LEAVE => Packet::Leave,
        DISCOVER => Packet::Discover,
        ANNOUNCE => {
            let name = input.text(MAX_NAME_BYTES)?;
            let level = input.text(MAX_LEVEL_BYTES)?;
            let game_type = input.u8()?;
            let players = input.u8()?;
            let capacity = input.u8()?;
            if usize::from(capacity) > MAX_PLAYERS || players > capacity {
                return Err(DecodeError::BadCount);
            }
            Packet::Announce {
                name,
                level,
                game_type,
                players,
                capacity,
            }
        }
        kind => return Err(DecodeError::UnknownKind(kind)),
    };
    input.finish()?;
    Ok(packet)
}
