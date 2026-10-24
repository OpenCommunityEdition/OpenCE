//! The engine's network protocol: the packets a host and its clients send
//! each other, their encoding, the snapshots' delta compression and the
//! checks every received packet passes.
//!
//! Only the host decides; a client sends its inputs and predicts its own
//! player, and the host sends snapshots of the world, each against the
//! last one the client acknowledged. Everything received is untrusted:
//! [`decode`] refuses a packet whose sizes, counts or indices are out of
//! bounds, and [`RateLimit`] a peer that sends too much.

pub mod client;
pub mod host;
pub mod lan;
pub mod packet;
pub mod rate;
pub mod snapshot;
pub mod transport;
mod wire;

pub use client::Client;
pub use host::Host;
pub use lan::{Discovery, Server, DEFAULT_PORT};
pub use packet::{decode, encode, InputFrame, Packet, Refusal};
pub use rate::RateLimit;
pub use snapshot::{ObjectState, Snapshot, SnapshotDelta};
pub use transport::{Link, LinkEndpoint, Transport, Udp};

/// The protocol's version: raised with any change to what the machines
/// send each other or must agree on. Machines of different versions do
/// not play together.
pub const PROTOCOL_VERSION: u16 = 2;

/// The bytes every packet starts with.
pub const MAGIC: [u8; 4] = *b"OCEN";

/// The largest packet sent or accepted, in bytes (under a typical path's
/// MTU, so packets are never fragmented).
pub const MAX_PACKET_BYTES: usize = 1200;

/// The most players in a game.
pub const MAX_PLAYERS: usize = 16;

/// Why a packet was refused.
#[derive(Clone, Copy, Debug, PartialEq, Eq, thiserror::Error)]
pub enum DecodeError {
    /// It is larger than [`MAX_PACKET_BYTES`].
    #[error("the packet is too large")]
    TooLarge,
    /// It ends before its contents do.
    #[error("the packet ends early")]
    Truncated,
    /// It does not start with [`MAGIC`].
    #[error("not one of our packets")]
    BadMagic,
    /// It is of another version of the protocol.
    #[error("protocol version {0}, not ours")]
    WrongVersion(u16),
    /// Its kind is unknown.
    #[error("an unknown kind of packet: {0}")]
    UnknownKind(u8),
    /// A count is larger than allowed.
    #[error("a count is out of bounds")]
    BadCount,
    /// An index is out of bounds.
    #[error("an index is out of bounds")]
    BadIndex,
    /// A number is not finite, or text is not UTF-8.
    #[error("a value is malformed")]
    BadValue,
    /// Bytes follow its contents.
    #[error("bytes follow the packet")]
    TrailingBytes,
}

/// Why a packet could not be written.
#[derive(Clone, Copy, Debug, PartialEq, Eq, thiserror::Error)]
pub enum EncodeError {
    /// It would be larger than [`MAX_PACKET_BYTES`].
    #[error("the packet would be too large")]
    TooLarge,
    /// A count is larger than allowed.
    #[error("a count is out of bounds")]
    BadCount,
}
