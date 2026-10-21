//! Finding games on the local network: a Discover sent to the broadcast
//! address (and to this machine), and the hosts' announcements gathered
//! as they come, without blocking.

use std::io;
use std::net::{Ipv4Addr, SocketAddr, SocketAddrV4};

use crate::packet::{decode, encode, Packet};
use crate::transport::{Transport, Udp};

/// The port a host listens on unless told otherwise.
pub const DEFAULT_PORT: u16 = 2302;

/// A game a host announced.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Server {
    /// Where it answered from.
    pub address: SocketAddr,
    /// The host's name.
    pub name: String,
    /// The level.
    pub level: String,
    /// The game type (Halo's number).
    pub game_type: u8,
    /// The players in it.
    pub players: u8,
    /// The most it takes.
    pub capacity: u8,
}

/// A search for games: the announcements that have come so far.
#[derive(Debug)]
pub struct Discovery {
    socket: Udp,
    servers: Vec<Server>,
}

impl Discovery {
    /// Asks every host on the network at [`DEFAULT_PORT`], and one on this
    /// machine (a broadcast does not always come back to its sender).
    pub fn start() -> io::Result<Self> {
        let broadcast = SocketAddr::V4(SocketAddrV4::new(Ipv4Addr::BROADCAST, DEFAULT_PORT));
        let here = SocketAddr::V4(SocketAddrV4::new(Ipv4Addr::LOCALHOST, DEFAULT_PORT));
        Self::asking(&[broadcast, here])
    }

    /// Asks the hosts at `addresses`.
    pub fn asking(addresses: &[SocketAddr]) -> io::Result<Self> {
        let mut socket = Udp::bind(SocketAddr::from((Ipv4Addr::UNSPECIFIED, 0)))?;
        socket.broadcast()?;
        let discover = encode(&Packet::Discover)
            .map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;
        for &address in addresses {
            socket.send(address, &discover);
        }
        Ok(Self {
            socket,
            servers: Vec::new(),
        })
    }

    /// The games announced so far, each once (by where it answered from).
    pub fn poll(&mut self) -> &[Server] {
        while let Some((address, bytes)) = self.socket.receive() {
            let Ok(Packet::Announce {
                name,
                level,
                game_type,
                players,
                capacity,
            }) = decode(&bytes)
            else {
                continue;
            };
            let server = Server {
                address,
                name,
                level,
                game_type,
                players,
                capacity,
            };
            match self.servers.iter_mut().find(|s| s.address == address) {
                Some(known) => *known = server,
                None => self.servers.push(server),
            }
        }
        &self.servers
    }
}
