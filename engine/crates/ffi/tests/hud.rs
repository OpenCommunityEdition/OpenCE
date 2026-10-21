//! The HUD's quads on bloodgulch, on windows of three shapes: the shield,
//! health and motion sensor keep to their corners, the crosshair to the
//! centre, and the rounds count what the rifle holds. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::Level;
use opence_ffi::hud::{hud_quads, HudQuad};
use opence_import::{import_level, Cache};
use opence_sim::input::TickInput;
use opence_sim::Simulation;

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

fn quads(level: &Level, width: u32, height: u32) -> Result<Vec<HudQuad>, Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    for _ in 0..120 {
        simulation.step(TickInput::default());
    }
    let state = simulation.hud().ok_or("no HUD")?;
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
