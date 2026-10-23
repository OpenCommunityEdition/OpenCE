//! The first-person weapon's states against the game's own, run over every
//! state, request, message, weapon type, heat and shotgun magazine
//! (tools/engine_first_person_reference.py writes the table from
//! source/first_person/first_person_weapons.c).

use opence_sim::first_person_states::{
    FirstPersonMachine, FirstPersonMessage, FirstPersonState, HeldMagazine, HeldWeapon,
    PlayingSound, ShotgunReload, WeaponType,
};

const REFERENCE: &str = include_str!("first_person_reference.txt");

type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;

fn number(field: &str) -> Result<i32> {
    Ok(field.parse()?)
}

/// The state of the game's number: none for -1.
fn state(field: &str) -> Result<Option<FirstPersonState>> {
    let Ok(index) = usize::try_from(number(field)?) else {
        return Ok(None);
    };
    Ok(Some(
        FirstPersonState::from_index(index).ok_or("no such state")?,
    ))
}

/// The graph's first-person block: animation n is 100+n; graph 1 has only
/// the first 17, graph 2 every one but posing.
fn animations(graph: i32) -> Vec<Option<u16>> {
    let count = if graph == 1 { 17 } else { 28 };
    (0..count)
        .map(|slot| (graph != 2 || slot != 1).then_some(100 + slot))
        .collect()
}

/// The weapon as the reference's world sets it up before each case.
fn world(state: Option<FirstPersonState>) -> FirstPersonMachine {
    FirstPersonMachine {
        state,
        frame: 5,
        sound: Some(PlayingSound { state: None }),
        ..FirstPersonMachine::default()
    }
}

/// The weapon after a case, as the reference prints it.
fn printed(machine: &FirstPersonMachine, sounds_stopped: bool) -> [i32; 8] {
    [
        machine.state.map_or(-1, |state| state as i32),
        machine.animation.map_or(-1, i32::from),
        i32::from(machine.frame),
        i32::from(machine.interpolation_count),
        i32::from(sounds_stopped),
        machine.shotgun_reload.map_or(-1, |reload| reload as i32),
        i32::from(machine.shotgun_shells),
        i32::from(machine.shotgun_empty),
    ]
}

fn expected(fields: &[&str]) -> Result<[i32; 8]> {
    let mut weapon = [0; 8];
    for (value, field) in weapon.iter_mut().zip(fields) {
        *value = number(field)?;
    }
    Ok(weapon)
}

#[test]
fn every_state_and_message_is_the_games() -> Result<()> {
    let (mut sets, mut nexts, mut messages) = (0, 0, 0);
    for line in REFERENCE.lines().filter(|line| !line.starts_with('#')) {
        let fields: Vec<&str> = line.split_whitespace().collect();
        let (kind, fields) = fields.split_first().ok_or("an empty line")?;
        let weapon_type =
            WeaponType::from_index(i16::try_from(number(fields.first().ok_or("no type")?)?)?);
        let overheated = number(fields.get(1).ok_or("no heat")?)? != 0;
        match *kind {
            "set" if fields.len() == 13 => {
                let blocks = animations(number(fields[2])?);
                let weapon = HeldWeapon {
                    weapon_type,
                    overheated,
                    animations: &blocks,
                    ..HeldWeapon::default()
                };
                let mut machine = world(state(fields[3])?);
                let new = state(fields[4])?.ok_or("a request of no state")?;
                let stopped = machine.set_state(Some(&weapon), new, true);
                assert_eq!(
                    printed(&machine, stopped),
                    expected(&fields[5..])?,
                    "{line}"
                );
                sets += 1;
            }
            "next" if fields.len() == 13 => {
                let blocks = animations(0);
                let weapon = HeldWeapon {
                    weapon_type,
                    overheated,
                    animations: &blocks,
                    ..HeldWeapon::default()
                };
                let mut machine = world(state(fields[2])?);
                machine.shotgun_reload = ShotgunReload::from_index(number(fields[3])?);
                machine.shotgun_empty = number(fields[4])? != 0;
                machine.next_state(Some(&weapon));
                assert_eq!(printed(&machine, false), expected(&fields[5..])?, "{line}");
                nexts += 1;
            }
            "message" if fields.len() == 20 => {
                let blocks = animations(0);
                let weapon = HeldWeapon {
                    weapon_type,
                    overheated,
                    magazine: Some(HeldMagazine {
                        loaded: i16::try_from(number(fields[2])?)?,
                        total: i16::try_from(number(fields[3])?)?,
                        maximum: i16::try_from(number(fields[4])?)?,
                        busy: number(fields[5])? != 0,
                    }),
                    animations: &blocks,
                    ..HeldWeapon::default()
                };
                let mut machine = world(state(fields[6])?);
                machine.interpolation_count = 4;
                machine.interpolation_index = 2;
                let message = usize::try_from(number(fields[7])?)?;
                let message = FirstPersonMessage::from_index(message).ok_or("no such message")?;
                let effects = machine.message(Some(&weapon), message);
                let side = [
                    i32::from(effects.forget),
                    i32::from(effects.switch),
                    i32::from(effects.unzoom),
                    i32::from(machine.push_back_velocity > 0.0),
                ];
                let mut expected_side = [0; 4];
                for (value, field) in expected_side.iter_mut().zip(&fields[8..12]) {
                    *value = number(field)?;
                }
                assert_eq!(side, expected_side, "{line}");
                assert_eq!(
                    printed(&machine, effects.sound_stopped),
                    expected(&fields[12..])?,
                    "{line}"
                );
                messages += 1;
            }
            _ => return Err(format!("a line the test does not know: {line}").into()),
        }
    }
    // (three weapon types, cool and hot; three graphs; 25 states before, 24 requests)
    assert_eq!(sets, 3 * 2 * 3 * 25 * 24);
    assert_eq!(nexts, 3 * 2 * 24 * 4 * 2);
    // (the shotgun's six magazines, the others' one)
    assert_eq!(messages, (1 + 6 + 1) * 2 * 25 * 18);
    Ok(())
}
