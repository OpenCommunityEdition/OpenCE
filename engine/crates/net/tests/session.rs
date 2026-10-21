//! A host and its clients on Blood Gulch: over UDP on this machine, and
//! over a network with delay and loss. The clients join, move, are
//! predicted within the tolerance, and are refused when the game is full.

use std::error::Error;
use std::net::SocketAddr;
use std::path::Path;

use opence_content::Level;
use opence_import::{import_level, Cache};
use opence_net::client::{Joining, TOLERANCE};
use opence_net::MAX_PLAYERS;
use opence_net::{decode, encode, Client, Host, Link, Packet, Refusal, Transport, Udp};
use opence_sim::clock::TICKS_PER_SECOND;
use opence_sim::game::Variant;
use opence_sim::input::TickInput;
use opence_sim::units::Radians;

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

const LEVEL: &str = "levels\\test\\bloodgulch\\bloodgulch";

fn bloodgulch() -> Result<Option<Level>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

/// Walking forward and turning slowly: a wide circle.
fn walking(tick: u32) -> TickInput {
    TickInput {
        forward: 127,
        yaw: Radians(if tick % 600 < 300 { 0.01 } else { -0.01 }),
        ..TickInput::default()
    }
}

fn address(port: u16) -> SocketAddr {
    SocketAddr::from(([10, 0, 0, 1], port))
}

fn client_position(host: &Host, player: usize) -> Option<[f32; 3]> {
    let player = host.simulation().players().get(player)?;
    Some(player.mover().position.to_array())
}

#[test]
fn a_client_plays_slayer_over_udp() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    // (both on 127.0.0.1, on ports of their own: macOS has no other
    // loopback addresses unless they are added to the system)
    let mut host_socket = Udp::bind(SocketAddr::from(([127, 0, 0, 1], 0)))?;
    let mut client_socket = Udp::bind(SocketAddr::from(([127, 0, 0, 1], 0)))?;
    let mut host = Host::new(&level, LEVEL, Variant::slayer())?;
    let mut client = Client::new(host_socket.address()?, "Spartan", &level)?;

    let mut errors = Vec::new();
    for tick in 0..60 * TICKS_PER_SECOND {
        host.step(&mut host_socket, &level, TickInput::default());
        client.step(&mut client_socket, walking(tick));
        if tick > 2 * TICKS_PER_SECOND {
            errors.extend(client.last_error());
        }
    }
    let Joining::Playing { player } = client.joining() else {
        return Err(format!("not joined: {:?}", client.joining()).into());
    };
    assert_eq!(host.client_players(), vec![player]);
    assert_eq!(host.simulation().players().len(), 2);

    // (it moved on the host as it did in its prediction)
    let on_host = client_position(&host, player).ok_or("no player")?;
    let predicted = client
        .simulation()
        .player()
        .ok_or("no prediction")?
        .mover()
        .position;
    let start = level.scenario.player_starts[1].position;
    let walked = (opence_sim::Vec3::from_array(on_host) - opence_sim::Vec3::from(start)).length();
    assert!(walked > 2.0, "it walked {walked}");
    let apart = (predicted - opence_sim::Vec3::from_array(on_host)).length();
    assert!(apart < 2.0, "prediction {apart} from the host's");

    // (after the first corrections it stays within the tolerance nearly
    // always: a correction is rare, a few in a minute)
    let within = errors.iter().filter(|&&e| e <= TOLERANCE).count();
    assert!(!errors.is_empty());
    assert!(
        within * 100 >= errors.len() * 99 && client.corrections() < 30,
        "{within} of {} within the tolerance, {} corrections",
        errors.len(),
        client.corrections()
    );
    // (and the other player, the host's, is drawn)
    assert_eq!(client.others().len(), 1);
    Ok(())
}

#[test]
fn a_client_plays_capture_the_flag_through_delay_and_loss() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    // (100 ms each way, and one packet in fifty lost)
    let link = Link::new(6, 0.02);
    let mut host_end = link.endpoint(address(1));
    let mut client_end = link.endpoint(address(2));
    let mut host = Host::new(&level, LEVEL, Variant::ctf())?;
    let mut client = Client::new(address(1), "Spartan", &level)?;

    let mut positions = Vec::new();
    for tick in 0..60 * TICKS_PER_SECOND {
        host.step(&mut host_end, &level, TickInput::default());
        client.step(&mut client_end, walking(tick));
        link.tick();
        if let Joining::Playing { player } = client.joining() {
            positions.extend(client_position(&host, player));
        }
    }
    assert!(matches!(client.joining(), Joining::Playing { .. }));
    let (Some(first), Some(last)) = (positions.first(), positions.last()) else {
        return Err("it never played".into());
    };
    let moved =
        (opence_sim::Vec3::from_array(*last) - opence_sim::Vec3::from_array(*first)).length();
    assert!(moved > 2.0, "it moved {moved}");
    assert!(client.last_error().is_some_and(|e| e < 1.0));
    assert_eq!(host.game().rules().flags().len(), 2);
    Ok(())
}

#[test]
fn a_full_game_refuses_and_discovery_is_answered() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let link = Link::new(0, 0.0);
    let mut host_end = link.endpoint(address(1));
    let mut host = Host::new(&level, LEVEL, Variant::team_slayer())?;
    let mut clients = Vec::new();
    for port in 0..MAX_PLAYERS {
        let port = u16::try_from(port)? + 100;
        clients.push((
            link.endpoint(address(port)),
            Client::new(address(1), "Spartan", &level)?,
        ));
    }
    for _ in 0..4 {
        for (end, client) in &mut clients {
            client.step(end, TickInput::default());
        }
        link.tick();
        host.step(&mut host_end, &level, TickInput::default());
        link.tick();
    }
    let playing = clients
        .iter()
        .filter(|(_, c)| matches!(c.joining(), Joining::Playing { .. }))
        .count();
    let refused = clients
        .iter()
        .filter(|(_, c)| c.joining() == Joining::Refused(Refusal::Full))
        .count();
    // (the host is one of the players)
    assert_eq!((playing, refused), (MAX_PLAYERS - 1, 1));
    assert_eq!(host.simulation().players().len(), MAX_PLAYERS);

    let mut seeker = link.endpoint(address(99));
    seeker.send(address(1), &encode(&Packet::Discover)?);
    link.tick();
    host.step(&mut host_end, &level, TickInput::default());
    link.tick();
    let (from, bytes) = seeker.receive().ok_or("no announcement")?;
    assert_eq!(from, address(1));
    let Packet::Announce {
        level: name,
        players,
        capacity,
        ..
    } = decode(&bytes)?
    else {
        return Err("not an announcement".into());
    };
    assert_eq!(name, LEVEL);
    assert_eq!(usize::from(players), MAX_PLAYERS);
    assert_eq!(usize::from(capacity), MAX_PLAYERS);
    Ok(())
}
