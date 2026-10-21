//! How packets travel: UDP sockets, or an in-memory network with delay
//! and loss for tests.

use std::cell::RefCell;
use std::collections::VecDeque;
use std::io;
use std::net::{SocketAddr, UdpSocket};
use std::rc::Rc;

use crate::MAX_PACKET_BYTES;

/// Something packets are sent and received through.
pub trait Transport {
    /// Sends `bytes` to `to` (lost silently if they cannot go).
    fn send(&mut self, to: SocketAddr, bytes: &[u8]);
    /// The next packet that has arrived, with who sent it.
    fn receive(&mut self) -> Option<(SocketAddr, Vec<u8>)>;
}

/// A UDP socket that never blocks.
#[derive(Debug)]
pub struct Udp {
    socket: UdpSocket,
}

impl Udp {
    /// A socket bound to `address` (port 0: any free one).
    pub fn bind(address: SocketAddr) -> io::Result<Self> {
        let socket = UdpSocket::bind(address)?;
        socket.set_nonblocking(true)?;
        Ok(Self { socket })
    }

    /// The address it is bound to.
    pub fn address(&self) -> io::Result<SocketAddr> {
        self.socket.local_addr()
    }

    /// Lets it send to the broadcast address (for LAN discovery).
    pub fn broadcast(&self) -> io::Result<()> {
        self.socket.set_broadcast(true)
    }
}

impl Transport for Udp {
    fn send(&mut self, to: SocketAddr, bytes: &[u8]) {
        let _ = self.socket.send_to(bytes, to);
    }

    fn receive(&mut self) -> Option<(SocketAddr, Vec<u8>)> {
        let mut buffer = [0; MAX_PACKET_BYTES + 1];
        loop {
            match self.socket.recv_from(&mut buffer) {
                Ok((count, from)) => return Some((from, buffer[..count].to_vec())),
                // (an earlier send's ICMP error on some systems: skip it)
                Err(error) if error.kind() == io::ErrorKind::ConnectionReset => continue,
                Err(_) => return None,
            }
        }
    }
}

#[derive(Debug)]
struct InFlight {
    arrives: u64,
    from: SocketAddr,
    to: SocketAddr,
    bytes: Vec<u8>,
}

#[derive(Debug, Default)]
struct Network {
    now: u64,
    delay_ticks: u64,
    loss: f32,
    seed: u32,
    in_flight: VecDeque<InFlight>,
}

/// An in-memory network that delays packets by whole ticks and loses a
/// share of them, the same way on every run.
#[derive(Clone, Debug, Default)]
pub struct Link {
    network: Rc<RefCell<Network>>,
}

impl Link {
    /// A network that delays every packet `delay_ticks` and loses the
    /// share `loss` of them (from 0 to 1).
    pub fn new(delay_ticks: u64, loss: f32) -> Self {
        Self {
            network: Rc::new(RefCell::new(Network {
                delay_ticks,
                loss,
                seed: 0x5EED,
                ..Network::default()
            })),
        }
    }

    /// A machine on it at `address`.
    pub fn endpoint(&self, address: SocketAddr) -> LinkEndpoint {
        LinkEndpoint {
            address,
            network: Rc::clone(&self.network),
        }
    }

    /// A tick passed.
    pub fn tick(&self) {
        self.network.borrow_mut().now += 1;
    }
}

/// A machine on a [`Link`].
#[derive(Clone, Debug)]
pub struct LinkEndpoint {
    address: SocketAddr,
    network: Rc<RefCell<Network>>,
}

impl Transport for LinkEndpoint {
    fn send(&mut self, to: SocketAddr, bytes: &[u8]) {
        let mut network = self.network.borrow_mut();
        network.seed = network
            .seed
            .wrapping_mul(1_664_525)
            .wrapping_add(1_013_904_223);
        let roll = (network.seed >> 8) as f32 / (1 << 24) as f32;
        if roll < network.loss {
            return;
        }
        let arrives = network.now + network.delay_ticks;
        network.in_flight.push_back(InFlight {
            arrives,
            from: self.address,
            to,
            bytes: bytes.to_vec(),
        });
    }

    fn receive(&mut self) -> Option<(SocketAddr, Vec<u8>)> {
        let mut network = self.network.borrow_mut();
        let now = network.now;
        let at = network
            .in_flight
            .iter()
            .position(|packet| packet.to == self.address && packet.arrives <= now)?;
        let packet = network.in_flight.remove(at)?;
        Some((packet.from, packet.bytes))
    }
}
