//! A unit's animation state rules against the game's own, run over every
//! pair of states (tools/engine_unit_state_reference.py writes the table
//! from source/units/units.c).

use opence_sim::unit_states::{
    aiming_screen, entered_without_animation, interpolation_frames, interruptable, loops,
    overlay_action_loops, UnitState,
};

const REFERENCE: &str = include_str!("unit_state_reference.txt");

/// The state of the game's number: none for -1.
fn state(field: &str) -> Result<Option<UnitState>, Box<dyn std::error::Error>> {
    let index: i32 = field.parse()?;
    if index < 0 {
        return Ok(None);
    }
    let state = UnitState::from_index(usize::try_from(index)?).ok_or("no such state")?;
    Ok(Some(state))
}

fn flag(field: &str) -> Result<bool, Box<dyn std::error::Error>> {
    Ok(field.parse::<i32>()? != 0)
}

#[test]
fn every_state_and_pair_is_the_games() -> Result<(), Box<dyn std::error::Error>> {
    let (mut states, mut pairs) = (0, 0);
    for line in REFERENCE.lines().filter(|line| !line.starts_with('#')) {
        let fields: Vec<&str> = line.split_whitespace().collect();
        match fields.as_slice() {
            ["state", current, looping, entered, overlay, screen] => {
                let current = state(current)?;
                assert_eq!(loops(current), flag(looping)?, "{line}");
                if let Some(current) = current {
                    assert_eq!(entered_without_animation(current), flag(entered)?, "{line}");
                }
                assert_eq!(overlay_action_loops(current), flag(overlay)?, "{line}");
                let screen: i32 = screen.parse()?;
                let expected = usize::try_from(screen).ok();
                assert_eq!(aiming_screen(current), expected, "{line}");
                states += 1;
            }
            ["pair", current, desired, interrupts, frames] => {
                let current = state(current)?;
                let desired = state(desired)?.ok_or("a pair wants no state")?;
                assert_eq!(interruptable(current, desired), flag(interrupts)?, "{line}");
                assert_eq!(
                    i32::from(interpolation_frames(desired, current)),
                    frames.parse::<i32>()?,
                    "{line}"
                );
                pairs += 1;
            }
            _ => return Err(format!("a line the test does not know: {line}").into()),
        }
    }
    assert_eq!(states, 45);
    assert_eq!(pairs, 45 * 44);
    Ok(())
}
