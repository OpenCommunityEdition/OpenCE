//! Network games for the app: hosting one, joining one by its address,
//! and searching the local network for them.

use std::net::{Ipv4Addr, SocketAddr, ToSocketAddrs};
use std::sync::{Arc, Mutex};

use opence_content::Level;
use opence_net::client::Joining;
use opence_net::{Client, Discovery, Host, Refusal, Udp, DEFAULT_PORT};
use opence_sim::game::Variant;
use opence_sim::input::TickInput;
use opence_sim::objects::Difficulty;
use opence_sim::Simulation;

use crate::{publish, transform, Engine, EngineError, LevelGeometry, Running, Wardrobe};

/// The games the app hosts.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum GameKind {
    /// Every player for themself.
    Slayer,
    /// Red against blue.
    TeamSlayer,
    /// Capture the Flag, red against blue.
    CaptureTheFlag,
}

impl GameKind {
    fn variant(self) -> Variant {
        match self {
            Self::Slayer => Variant::slayer(),
            Self::TeamSlayer => Variant::team_slayer(),
            Self::CaptureTheFlag => Variant::ctf(),
        }
    }
}

/// Where the app is in a network game.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum NetworkStatus {
    /// It plays alone.
    Alone,
    /// It hosts a game.
    Hosting {
        /// The port it listens on.
        port: u16,
        /// The players in it, its own among them.
        players: u32,
    },
    /// It waits for the host's welcome.
    Waiting,
    /// It plays in another's game.
    Playing {
        /// Its player's index on the host.
        player: u32,
    },
    /// The host refused it.
    Refused {
        /// Why.
        reason: String,
    },
}

/// A game a host announced on the local network.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Record)]
pub struct ServerInfo {
    /// Where to join it (address and port).
    pub address: String,
    /// The host's name.
    pub name: String,
    /// The level.
    pub level: String,
    /// The game type (Halo's number: 1 CTF, 2 Slayer).
    pub game_type: u8,
    /// The players in it.
    pub players: u8,
    /// The most it takes.
    pub capacity: u8,
}

/// A network game the engine plays.
pub(crate) enum Network {
    Host { host: Box<Host>, socket: Udp },
    Client { client: Box<Client>, socket: Udp },
}

impl Network {
    pub(crate) fn step(&mut self, simulation: &mut Simulation, level: &Level, input: TickInput) {
        match self {
            Self::Host { host, socket } => {
                // (the game's events go to the HUD and scores in time)
                let _ = host.step(simulation, socket, level, input);
            }
            Self::Client { client, socket } => client.step(simulation, socket, level, input),
        }
    }
}

fn network_error(error: impl std::fmt::Display) -> EngineError {
    EngineError::Network {
        message: error.to_string(),
    }
}

/// The socket address `text` names: `host` or `host:port`, the port 2302
/// unless given; an IPv4 address is preferred.
pub(crate) fn resolve(text: &str) -> Result<SocketAddr, EngineError> {
    let text = text.trim();
    let addresses: Vec<SocketAddr> = match text.parse::<SocketAddr>() {
        Ok(address) => vec![address],
        Err(_) => match text.rsplit_once(':') {
            Some((host, port)) if !host.contains(':') => {
                let port = port.parse::<u16>().map_err(network_error)?;
                (host, port)
                    .to_socket_addrs()
                    .map_err(network_error)?
                    .collect()
            }
            _ => (text.trim_matches(['[', ']']), DEFAULT_PORT)
                .to_socket_addrs()
                .map_err(network_error)?
                .collect(),
        },
    };
    addresses
        .iter()
        .find(|address| address.is_ipv4())
        .or(addresses.first())
        .copied()
        .ok_or_else(|| network_error(format!("{text} has no address")))
}

/// A world of `level` with its scenario's objects (at Normal), dressed.
fn scenario_world(level: &Level) -> Result<Simulation, EngineError> {
    let mut simulation = Simulation::empty();
    let handles = simulation
        .spawn_scenario(&level.scenario, Difficulty::Normal)
        .map_err(|_| EngineError::Load {
            message: "the scenario places more objects than the world holds".into(),
        })?;
    let mut wardrobe = Wardrobe::default();
    for handle in handles {
        wardrobe.dress(&mut simulation, level, handle);
    }
    Ok(simulation)
}

/// Dresses the players added since the last `dressed`; returns how many
/// there are now.
pub(crate) fn dress_players(simulation: &mut Simulation, level: &Level, dressed: usize) -> usize {
    let handles: Vec<_> = simulation
        .players()
        .iter()
        .skip(dressed)
        .map(|player| player.handle())
        .collect();
    let mut wardrobe = Wardrobe::default();
    for handle in handles {
        wardrobe.dress(simulation, level, handle);
    }
    simulation.players().len()
}

#[uniffi::export]
impl Engine {
    /// Hosts a game of `kind` on `geometry`'s level, called `name`, on
    /// UDP `port` (0: any free one); the host's player stands at the first
    /// start, for at most `max_players` (from 1 to 16). Returns the port it
    /// listens on.
    pub fn host_game(
        &self,
        geometry: Arc<LevelGeometry>,
        kind: GameKind,
        port: u16,
        name: String,
        max_players: u8,
    ) -> Result<u16, EngineError> {
        let level = &geometry.content;
        let socket =
            Udp::bind(SocketAddr::from((Ipv4Addr::UNSPECIFIED, port))).map_err(network_error)?;
        let port = socket.address().map_err(network_error)?.port();
        let mut simulation = scenario_world(level)?;
        let mut host = Host::new(&mut simulation, level, &geometry.name, kind.variant())
            .map_err(network_error)?;
        host.set_name(&name);
        host.set_capacity(usize::from(max_players));
        self.run_network(
            geometry.clone(),
            simulation,
            Network::Host {
                host: Box::new(host),
                socket,
            },
        );
        Ok(port)
    }

    /// Joins the game at `address` (`host` or `host:port`) on `geometry`'s
    /// level, as `name`; it waits for the host's welcome (see
    /// `network_status`).
    pub fn join_game(
        &self,
        geometry: Arc<LevelGeometry>,
        address: String,
        name: String,
    ) -> Result<(), EngineError> {
        let level = &geometry.content;
        let host = resolve(&address)?;
        let socket =
            Udp::bind(SocketAddr::from((Ipv4Addr::UNSPECIFIED, 0))).map_err(network_error)?;
        let mut simulation = scenario_world(level)?;
        let name = opence_net::packet::fit(&name, opence_net::packet::MAX_NAME_BYTES);
        let client = Client::new(&mut simulation, host, &name, level).map_err(network_error)?;
        self.run_network(
            geometry.clone(),
            simulation,
            Network::Client {
                client: Box::new(client),
                socket,
            },
        );
        Ok(())
    }

    /// Leaves the network game (telling the host), and plays on alone.
    pub fn leave_game(&self) {
        let mut state = self.lock();
        let Some(running) = state.running.as_mut() else {
            return;
        };
        if let Some(Network::Client { client, socket }) = running.network.as_mut() {
            client.leave(socket);
        }
        running.network = None;
    }

    /// Where the app is in a network game.
    pub fn network_status(&self) -> NetworkStatus {
        let state = self.lock();
        let Some(running) = state.running.as_ref() else {
            return NetworkStatus::Alone;
        };
        match &running.network {
            None => NetworkStatus::Alone,
            Some(Network::Host { socket, .. }) => NetworkStatus::Hosting {
                port: socket.address().map_or(0, |address| address.port()),
                players: running.simulation.players().len() as u32,
            },
            Some(Network::Client { client, .. }) => match client.joining() {
                Joining::Waiting => NetworkStatus::Waiting,
                Joining::Playing { player } => NetworkStatus::Playing {
                    player: player as u32,
                },
                Joining::Refused(refusal) => NetworkStatus::Refused {
                    reason: match refusal {
                        Refusal::Full => "the game is full",
                        Refusal::Over => "the game is over",
                        Refusal::Banned => "the host refused it",
                    }
                    .into(),
                },
            },
        }
    }
}

impl Engine {
    fn run_network(
        &self,
        geometry: Arc<LevelGeometry>,
        mut simulation: Simulation,
        network: Network,
    ) {
        let level = geometry.content.clone();
        let dressed = dress_players(&mut simulation, &level, 0);
        let mut state = self.lock();
        let previous_camera = transform(&simulation.camera().transform);
        state.running = Some(Running {
            simulation,
            previous_camera,
            clock: opence_sim::clock::Clock::default(),
            level: Some(level),
            network: Some(network),
            dressed,
            campaign: None,
        });
        publish(&mut state, &self.snapshots, 0.0);
    }
}

/// A search of the local network for games.
#[derive(uniffi::Object)]
pub struct ServerSearch {
    discovery: Mutex<Discovery>,
}

#[uniffi::export]
impl ServerSearch {
    /// Asks every host on the local network (and this machine) at port
    /// 2302.
    #[uniffi::constructor]
    pub fn start() -> Result<Arc<Self>, EngineError> {
        Ok(Arc::new(Self {
            discovery: Mutex::new(Discovery::start().map_err(network_error)?),
        }))
    }

    /// Asks the hosts at `addresses` (each `host` or `host:port`).
    #[uniffi::constructor]
    pub fn asking(addresses: Vec<String>) -> Result<Arc<Self>, EngineError> {
        let addresses = addresses
            .iter()
            .map(|address| resolve(address))
            .collect::<Result<Vec<_>, _>>()?;
        Ok(Arc::new(Self {
            discovery: Mutex::new(Discovery::asking(&addresses).map_err(network_error)?),
        }))
    }

    /// The games that have answered so far.
    pub fn servers(&self) -> Vec<ServerInfo> {
        let mut discovery = self
            .discovery
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner);
        discovery
            .poll()
            .iter()
            .map(|server| ServerInfo {
                address: server.address.to_string(),
                name: server.name.clone(),
                level: server.level.clone(),
                game_type: server.game_type,
                players: server.players,
                capacity: server.capacity,
            })
            .collect()
    }
}
