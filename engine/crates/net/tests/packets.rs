//! The packets encode and decode, snapshots compress against an
//! acknowledged one, packets out of bounds are refused, and the decoder
//! survives random and mutated bytes.

use opence_net::packet::{MAX_INPUT_FRAMES, MAX_NAME_BYTES};
use opence_net::snapshot::MAX_SNAPSHOT_OBJECTS;
use opence_net::{
    decode, encode, DecodeError, EncodeError, InputFrame, ObjectState, Packet, RateLimit, Refusal,
    Snapshot, MAGIC, MAX_PACKET_BYTES, PROTOCOL_VERSION,
};

fn object(id: u32, x: f32) -> ObjectState {
    ObjectState {
        id,
        definition: 7,
        position: [x, 2.0, 3.0],
        rotation: [0.0, 0.0, 0.0, 1.0],
        velocity: [0.5, 0.0, 0.0],
        body: 1.0,
        shield: 0.5,
    }
}

fn samples() -> Vec<Packet> {
    let snapshot = Snapshot {
        tick: 40,
        objects: vec![object(1, 1.0), object(4, 2.0)],
    };
    vec![
        Packet::Hello {
            name: "Spartan".into(),
        },
        Packet::Welcome {
            player: 3,
            tick: 1000,
            seed: 0xDEAD_BEEF,
            level: "levels\\test\\bloodgulch\\bloodgulch".into(),
            game_type: 1,
        },
        Packet::Refused(Refusal::Full),
        Packet::Inputs {
            ack: Some(38),
            frames: (0..4)
                .map(|tick| InputFrame {
                    tick: 100 + tick,
                    buttons: 0b101,
                    forward: 32767,
                    strafe: -200,
                    yaw: 0.01,
                    pitch: -0.02,
                })
                .collect(),
        },
        Packet::Snapshot {
            input_ack: None,
            delta: snapshot.delta(None),
        },
        Packet::Leave,
        Packet::Discover,
        Packet::Announce {
            name: "host".into(),
            level: "bloodgulch".into(),
            game_type: 2,
            players: 3,
            capacity: 16,
        },
    ]
}

#[test]
fn every_packet_round_trips() -> Result<(), Box<dyn std::error::Error>> {
    for packet in samples() {
        let bytes = encode(&packet)?;
        assert!(bytes.starts_with(&MAGIC));
        assert_eq!(decode(&bytes)?, packet);
    }
    Ok(())
}

#[test]
fn a_snapshot_is_sent_as_its_changes_from_the_acknowledged_one() {
    let base = Snapshot {
        tick: 10,
        objects: vec![object(1, 1.0), object(2, 1.0), object(3, 1.0)],
    };
    let mut moved = object(2, 1.5);
    moved.shield = 0.25;
    let next = Snapshot {
        tick: 12,
        objects: vec![object(1, 1.0), moved, object(9, 4.0)],
    };
    let delta = next.delta(Some(&base));
    assert_eq!(delta.base, Some(10));
    assert_eq!(delta.removed, vec![3]);
    assert_eq!(delta.changed.len(), 2);
    let change = delta.changed[0];
    assert_eq!(change.id, 2);
    assert!(change.position.is_some() && change.shield.is_some());
    assert!(change.rotation.is_none() && change.definition.is_none());
    assert_eq!(delta.apply(Some(&base)), Some(next.clone()));
    // (against another snapshot than its base it is refused)
    assert_eq!(delta.apply(None), None);
    // (and it is smaller than the whole snapshot)
    let whole = encode(&Packet::Snapshot {
        input_ack: None,
        delta: next.delta(None),
    });
    let changes = encode(&Packet::Snapshot {
        input_ack: None,
        delta,
    });
    assert!(matches!((whole, changes), (Ok(a), Ok(b)) if b.len() < a.len()));
}

#[test]
fn an_unchanged_snapshot_sends_nothing_but_its_tick() {
    let base = Snapshot {
        tick: 5,
        objects: vec![object(1, 1.0)],
    };
    let same = Snapshot {
        tick: 6,
        ..base.clone()
    };
    let delta = same.delta(Some(&base));
    assert!(delta.changed.is_empty() && delta.removed.is_empty());
}

#[test]
fn bad_packets_are_refused() -> Result<(), Box<dyn std::error::Error>> {
    let hello = encode(&Packet::Hello { name: "a".into() })?;
    assert_eq!(
        decode(&hello[..hello.len() - 1]),
        Err(DecodeError::Truncated)
    );
    let mut trailing = hello.clone();
    trailing.push(0);
    assert_eq!(decode(&trailing), Err(DecodeError::TrailingBytes));
    let mut magic = hello.clone();
    magic[0] = b'X';
    assert_eq!(decode(&magic), Err(DecodeError::BadMagic));
    let mut version = hello.clone();
    version[4..6].copy_from_slice(&(PROTOCOL_VERSION + 1).to_le_bytes());
    assert_eq!(
        decode(&version),
        Err(DecodeError::WrongVersion(PROTOCOL_VERSION + 1))
    );
    let mut kind = hello.clone();
    kind[6] = 200;
    assert_eq!(decode(&kind), Err(DecodeError::UnknownKind(200)));
    assert_eq!(
        decode(&vec![0; MAX_PACKET_BYTES + 1]),
        Err(DecodeError::TooLarge)
    );

    // (a welcome to a player past the most there are)
    let mut welcome = encode(&Packet::Welcome {
        player: 0,
        tick: 0,
        seed: 0,
        level: String::new(),
        game_type: 2,
    })?;
    welcome[7] = 16;
    assert_eq!(decode(&welcome), Err(DecodeError::BadIndex));

    // (too many inputs, and inputs out of order)
    let frames = |count: u32| Packet::Inputs {
        ack: None,
        frames: (0..count)
            .map(|tick| InputFrame {
                tick,
                ..InputFrame::default()
            })
            .collect(),
    };
    let limit = u32::try_from(MAX_INPUT_FRAMES)?;
    assert_eq!(encode(&frames(limit + 1)), Err(EncodeError::BadCount));
    let mut many = encode(&frames(1))?;
    many[11..13].copy_from_slice(&(limit as u16 + 1).to_le_bytes());
    assert_eq!(decode(&many), Err(DecodeError::BadCount));
    let mut backwards = encode(&frames(2))?;
    backwards[13..17].copy_from_slice(&5u32.to_le_bytes());
    assert_eq!(decode(&backwards), Err(DecodeError::BadIndex));

    // (a name too long, a number not finite)
    let long = "x".repeat(MAX_NAME_BYTES + 1);
    assert_eq!(
        encode(&Packet::Hello { name: long }),
        Err(EncodeError::BadCount)
    );
    let mut nan = encode(&frames(1))?;
    let yaw = nan.len() - 8;
    nan[yaw..yaw + 4].copy_from_slice(&f32::NAN.to_le_bytes());
    assert_eq!(decode(&nan), Err(DecodeError::BadValue));

    // (a snapshot too large for one packet)
    let huge = Snapshot {
        tick: 1,
        objects: (0..u32::try_from(MAX_SNAPSHOT_OBJECTS)?)
            .map(|id| object(id, 0.0))
            .collect(),
    };
    assert_eq!(
        encode(&Packet::Snapshot {
            input_ack: None,
            delta: huge.delta(None)
        }),
        Err(EncodeError::TooLarge)
    );
    Ok(())
}

#[test]
fn a_peer_sending_too_fast_is_held_to_its_rate() {
    let mut limit = RateLimit::new(60, 4, 60);
    let allowed = (0..10).filter(|_| limit.allow()).count();
    assert_eq!(allowed, 4);
    limit.tick();
    assert!(limit.allow());
    assert!(!limit.allow());
}

/// The fuzz target: random bytes, and every sample packet with bytes
/// changed, cut short and lengthened, never panic the decoder, and
/// whatever it accepts encodes back to the same bytes.
#[test]
fn the_decoder_survives_random_and_mutated_bytes() -> Result<(), Box<dyn std::error::Error>> {
    let mut seed: u32 = 0x1234_5678;
    let mut next = move || {
        seed = seed.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
        seed >> 8
    };
    let samples: Vec<Vec<u8>> = samples().iter().map(encode).collect::<Result<_, _>>()?;
    let mut accepted = 0;
    for round in 0..200_000u32 {
        let mut bytes = if round % 4 == 0 {
            let mut bytes = MAGIC.to_vec();
            bytes.extend_from_slice(&PROTOCOL_VERSION.to_le_bytes());
            bytes.extend((0..next() % 64).map(|_| next() as u8));
            bytes
        } else {
            samples[next() as usize % samples.len()].clone()
        };
        for _ in 0..1 + next() % 4 {
            let at = next() as usize % (bytes.len() + 1);
            match next() % 3 {
                0 if at < bytes.len() => bytes[at] = next() as u8,
                1 => bytes.truncate(at),
                _ => bytes.insert(at.min(bytes.len()), next() as u8),
            }
        }
        if let Ok(packet) = decode(&bytes) {
            accepted += 1;
            assert_eq!(encode(&packet)?, bytes);
        }
    }
    assert!(accepted > 0);
    Ok(())
}
