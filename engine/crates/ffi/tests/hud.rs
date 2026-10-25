//! The HUD's quads on bloodgulch, on windows of every common shape: the
//! shield, health and motion sensor keep to their corners and clear of one
//! another, the crosshair to the centre, the rounds count what the rifle
//! holds, and the meters and counters follow the rounds fired, the
//! reserve, the shield and the grenades. Passes without assets/maps/bloodgulch.map.

use std::error::Error;

use opence_content::Level;
use opence_ffi::hud::{hud_quads, HudQuad};
use opence_import::testing::stock_level;
use opence_sim::hud::HudState;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::Simulation;

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

/// The player at the first start, two seconds on.
fn spawned(level: &Level) -> Result<Simulation, Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    for _ in 0..120 {
        simulation.step(TickInput::default());
    }
    Ok(simulation)
}

fn quads(level: &Level, width: u32, height: u32) -> Result<Vec<HudQuad>, Box<dyn Error>> {
    let state = spawned(level)?.hud().ok_or("no HUD")?;
    Ok(hud_quads(level, &state, width, height, 0.0))
}

fn find<'a>(quads: &'a [HudQuad], bitmap: &str, image: u32) -> Result<&'a HudQuad, String> {
    quads
        .iter()
        .find(|quad| quad.bitmap.ends_with(bitmap) && quad.image == image)
        .ok_or_else(|| format!("no {bitmap} {image}"))
}

const UNIT_BACKGROUND: &str = "hud_unit_backgrounds";
const WEAPON_BACKGROUND: &str = "hud_weapon_backgrounds";
const RETICLES: &str = "hud_reticles";
const DIGITS: &str = "hud_counter_numbers";

#[test]
fn the_crosshair_is_at_the_centre_of_every_shape() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    for (width, height) in [(1920, 1080), (1920, 1200), (2560, 1080)] {
        let quads = quads(&level, width, height)?;
        let crosshair = find(&quads, RETICLES, 0)?;
        let centre = [
            (crosshair.left + crosshair.right) * 0.5,
            (crosshair.top + crosshair.bottom) * 0.5,
        ];
        assert!(
            (centre[0] - width as f32 * 0.5).abs() < 0.01,
            "{width}x{height}: {centre:?}"
        );
        assert!(
            (centre[1] - height as f32 * 0.5).abs() < 0.01,
            "{width}x{height}: {centre:?}"
        );
        for quad in &quads {
            assert!(
                quad.top > -32.0 && quad.bottom < height as f32 + 32.0,
                "{quad:?}"
            );
        }
    }
    Ok(())
}

#[test]
fn a_wider_window_moves_only_the_right_corners() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let narrow = quads(&level, 1920, 1080)?;
    let wide = quads(&level, 2560, 1080)?;
    // (the shield keeps to the top right; the rounds to the top left)
    let (shield, wider_shield) = (
        find(&narrow, UNIT_BACKGROUND, 0)?,
        find(&wide, UNIT_BACKGROUND, 0)?,
    );
    assert!((wider_shield.left - shield.left - 640.0).abs() < 0.01);
    assert!(
        (wider_shield.right - 2560.0).abs() < 0.01,
        "{wider_shield:?}"
    );
    let (rounds, wider_rounds) = (
        find(&narrow, WEAPON_BACKGROUND, 1)?,
        find(&wide, WEAPON_BACKGROUND, 1)?,
    );
    assert_eq!(rounds, wider_rounds);
    Ok(())
}

#[test]
fn a_taller_window_draws_it_larger() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let short = quads(&level, 1920, 1080)?;
    let tall = quads(&level, 1920, 1200)?;
    let (panel, taller) = (
        find(&short, UNIT_BACKGROUND, 0)?,
        find(&tall, UNIT_BACKGROUND, 0)?,
    );
    let ratio = (taller.bottom - taller.top) / (panel.bottom - panel.top);
    assert!((ratio - 1200.0 / 1080.0).abs() < 1e-4, "{ratio}");
    // (the panel's 128 HUD pixels, 2.25 of the window's each at 1080 lines)
    assert!((panel.right - panel.left - 288.0).abs() < 0.01, "{panel:?}");
    Ok(())
}

#[test]
fn the_rounds_and_meters_count_the_rifle() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let quads = quads(&level, 1920, 1080)?;
    let digits = quads
        .iter()
        .filter(|quad| quad.bitmap.ends_with(DIGITS))
        .count();
    // (the rifle's rounds carried, three digits, and the grenades, one)
    assert!(digits >= 3, "{digits} digits");
    let meters: Vec<_> = quads.iter().filter_map(|quad| quad.meter).collect();
    // (the shield, the health and the rifle's magazine, all full)
    assert_eq!(meters.len(), 3);
    assert!(
        meters.iter().all(|meter| meter.threshold > 0.9),
        "{meters:?}"
    );
    Ok(())
}

const UNIT_METERS: &str = "hud_unit_meters";
const AMMO_METERS: &str = "hud_ammo_meters";

/// The windows of every common shape, 4:3 to 21:9.
const SHAPES: [(u32, u32); 7] = [
    (1024, 768),
    (1280, 720),
    (1920, 1080),
    (2560, 1080),
    (2560, 1440),
    (3440, 1440),
    (3840, 2160),
];

/// The HUD's parts, by where they are drawn: the crosshair, and the top
/// left (the weapon's rounds and the grenades), the top right (the shield
/// and health) and the bottom (the motion sensor).
fn part(quad: &HudQuad, width: u32, height: u32) -> &'static str {
    let centre = [
        (quad.left + quad.right) * 0.5,
        (quad.top + quad.bottom) * 0.5,
    ];
    if quad.bitmap.ends_with(RETICLES) {
        "crosshair"
    } else if centre[1] >= height as f32 * 0.5 {
        "sensor"
    } else if centre[0] < width as f32 * 0.5 {
        "weapon"
    } else {
        "unit"
    }
}

#[test]
fn on_every_shape_its_parts_keep_clear_of_one_another() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let state = spawned(&level)?.hud().ok_or("no HUD")?;
    for (width, height) in SHAPES {
        let mut parts: Vec<(&str, [f32; 4])> = Vec::new();
        for quad in hud_quads(&level, &state, width, height, 0.0) {
            let name = part(&quad, width, height);
            let rect = [quad.left, quad.top, quad.right, quad.bottom];
            match parts.iter_mut().find(|(other, _)| *other == name) {
                Some((_, bounds)) => {
                    *bounds = [
                        bounds[0].min(rect[0]),
                        bounds[1].min(rect[1]),
                        bounds[2].max(rect[2]),
                        bounds[3].max(rect[3]),
                    ];
                }
                None => parts.push((name, rect)),
            }
        }
        println!("{width}x{height}: {parts:?}");
        assert_eq!(parts.len(), 4, "{width}x{height}: {parts:?}");
        // (within the window, but for the art's own clear margins: a few
        // HUD pixels, as the original draws them)
        let margin = 8.0 * height as f32 / 480.0;
        for (name, [left, top, right, bottom]) in &parts {
            assert!(
                *left > -margin
                    && *top > -margin
                    && *right < width as f32 + margin
                    && *bottom < height as f32 + margin,
                "{width}x{height}: the {name} runs off the window"
            );
        }
        for (at, (name, a)) in parts.iter().enumerate() {
            for (other, b) in &parts[at + 1..] {
                let crossed = a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3];
                assert!(!crossed, "{width}x{height}: the {name} crosses the {other}");
            }
        }
    }
    Ok(())
}

/// The fill of the meters of `bitmap`, least first.
fn fills(quads: &[HudQuad], bitmap: &str) -> Vec<f32> {
    let mut fills: Vec<f32> = quads
        .iter()
        .filter(|quad| quad.bitmap.ends_with(bitmap))
        .filter_map(|quad| quad.meter.map(|meter| meter.threshold))
        .collect();
    fills.sort_by(f32::total_cmp);
    fills
}

/// The digits drawn: where each is and which it is.
fn digits(quads: &[HudQuad]) -> Vec<[f32; 3]> {
    quads
        .iter()
        .filter(|quad| quad.bitmap.ends_with(DIGITS))
        .map(|quad| [quad.left, quad.top, quad.u0])
        .collect()
}

fn drawn(level: &Level, state: &HudState) -> Vec<HudQuad> {
    hud_quads(level, state, 1920, 1080, 0.0)
}

#[test]
fn it_follows_the_rounds_the_shield_and_the_grenades() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = spawned(&level)?;
    let full = simulation.hud().ok_or("no HUD")?;
    let before = drawn(&level, &full);

    // (rounds fired: the magazine's meter and its count fall)
    for _ in 0..15 {
        simulation.step(TickInput {
            buttons: Buttons::FIRE,
            ..TickInput::default()
        });
    }
    let fired = simulation.hud().ok_or("no HUD")?;
    let loaded = |state: &HudState| {
        state
            .weapon
            .as_ref()
            .and_then(|weapon| weapon.magazines.first())
            .map(|magazine| magazine.loaded)
    };
    assert!(loaded(&fired) < loaded(&full), "{:?}", loaded(&fired));
    let after = drawn(&level, &fired);
    let (was, now) = (fills(&before, AMMO_METERS), fills(&after, AMMO_METERS));
    println!(
        "rounds {:?} to {:?}: magazine {was:?} to {now:?}",
        loaded(&full),
        loaded(&fired)
    );
    assert!(now.first() < was.first(), "the magazine's meter held");
    // (the rifle's counter is of the reserve, as the original's: it holds
    // while the magazine empties, and falls with the reserve)
    assert_eq!(
        digits(&after),
        digits(&before),
        "the counter moved with the magazine"
    );
    let mut spent = fired.clone();
    if let Some(magazine) = spent
        .weapon
        .as_mut()
        .and_then(|weapon| weapon.magazines.first_mut())
    {
        magazine.total -= 60;
    }
    assert_ne!(
        digits(&drawn(&level, &spent)),
        digits(&before),
        "the counter held as the reserve fell"
    );

    // (the shield down to a quarter: its meter falls, the health's holds)
    let hurt = HudState {
        shield: 0.25,
        ..full.clone()
    };
    let (was, now) = (
        fills(&before, UNIT_METERS),
        fills(&drawn(&level, &hurt), UNIT_METERS),
    );
    println!("shield 1 to 0.25: unit meters {was:?} to {now:?}");
    assert_eq!(was.len(), now.len());
    assert!(now.first() < was.first(), "the shield's meter held");
    assert_eq!(now.last(), was.last(), "the health's meter moved");

    // (a grenade thrown: the grenades' count changes)
    let mut thrown = full.clone();
    thrown.grenades[thrown.grenade] -= 1;
    assert_ne!(
        digits(&drawn(&level, &thrown)),
        digits(&before),
        "the grenades' count held"
    );
    Ok(())
}
