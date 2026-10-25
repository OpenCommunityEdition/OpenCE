//! The HUD's state message on bloodgulch: by a Warthog it says, as the
//! original's message does, which key enters which seat of what; and on
//! windows of every common shape it keeps to the top left, within the
//! window and clear of the HUD's own panels, its pieces set left to right. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;

use opence_content::{Font, Level, TagRef};
use opence_ffi::hud::{hud_quads, HudQuad};
use opence_ffi::hud_text::{prompt_text_of, prompt_words};
use opence_ffi::ui::UiQuad;
use opence_import::testing::stock_level;
use opence_sim::hud::ActionPrompt;
use opence_sim::input::TickInput;
use opence_sim::Simulation;

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

const WARTHOG: &str = "vehicles\\warthog\\warthog";

/// The keys of the controls, as the app's default bindings write them.
fn keys() -> Vec<String> {
    [
        "E", "G", "Mouse 1", "Q", "Space", "X", "Tab", "F", "Ctrl", "Z", "Return", "Escape",
        "WASD", "Mouse",
    ]
    .map(String::from)
    .to_vec()
}

fn bloodgulch() -> Result<Option<Level>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

fn tag(level: &Level, path: &str) -> Result<TagRef> {
    level
        .objects
        .iter()
        .find(|object| object.tag.path.eq_ignore_ascii_case(path))
        .map(|object| object.tag.clone())
        .ok_or_else(|| format!("no {path}").into())
}

fn enter(level: &Level) -> Result<ActionPrompt> {
    Ok(ActionPrompt::EnterVehicle {
        vehicle: tag(level, WARTHOG)?,
        seat: 0,
    })
}

/// The HUD's own quads, the player standing at the first start.
fn hud(level: &Level, width: u32, height: u32) -> Result<Vec<HudQuad>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    for _ in 0..30 {
        simulation.step(TickInput::default());
    }
    let state = simulation.hud().ok_or("no HUD")?;
    Ok(hud_quads(level, &state, width, height, 0.0))
}

fn overlaps(a: [f32; 4], b: [f32; 4]) -> bool {
    a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3]
}

/// A glyph's ink: its field less the field's padding, on a window `height`
/// pixels high.
fn rect(glyph: &UiQuad, height: u32) -> [f32; 4] {
    let padding = f32::from(Font::PADDING) * height as f32 / 480.0;
    [
        glyph.left + padding,
        glyph.top + padding,
        glyph.right - padding,
        glyph.bottom - padding,
    ]
}

#[test]
fn by_a_warthog_it_says_which_key_enters_which_seat() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let words = prompt_words(&level, &enter(&level)?, &keys()).ok_or("no message")?;
    println!("{words}");
    assert_eq!(words, "Press E to enter driver seat of Warthog");
    let flip = ActionPrompt::FlipVehicle {
        vehicle: tag(&level, WARTHOG)?,
    };
    let words = prompt_words(&level, &flip, &keys()).ok_or("no message")?;
    assert_eq!(words, "Press E to flip Warthog");
    let text = prompt_text_of(&level, &enter(&level)?, &keys(), [1920, 1080], 0.0);
    let letters = "Press E to enter driver seat of Warthog"
        .chars()
        .filter(|letter| !letter.is_whitespace())
        .count();
    assert_eq!(text.glyphs.len(), letters, "{:?}", text.glyphs.first());
    Ok(())
}

#[test]
fn a_weapons_message_shows_its_picture_or_its_name() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut shown = 0;
    for object in level
        .objects
        .iter()
        .filter(|object| object.weapon.is_some())
    {
        let swap = ActionPrompt::SwapWeapon {
            weapon: object.tag.clone(),
        };
        let Some(words) = prompt_words(&level, &swap, &keys()) else {
            continue;
        };
        println!("{}: {words}", object.tag.path);
        assert!(words.starts_with("Hold E to swap for "), "{words}");
        assert!(!words.ends_with("for "), "{words}");
        let text = prompt_text_of(&level, &swap, &keys(), [1920, 1080], 0.0);
        if words.contains("[picture]") {
            assert_eq!(text.icons.len(), 1, "{}: {:?}", object.tag.path, text.icons);
        }
        shown += 1;
    }
    assert!(shown > 0, "no weapon's message");
    Ok(())
}

#[test]
fn on_every_shape_it_keeps_to_the_top_left_clear_of_the_panels() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let prompt = enter(&level)?;
    for (width, height) in [
        (1024, 768),
        (1280, 720),
        (1920, 1080),
        (2560, 1080),
        (2560, 1440),
        (3440, 1440),
        (3840, 2160),
    ] {
        let text = prompt_text_of(&level, &prompt, &keys(), [width, height], 0.0);
        let rects: Vec<[f32; 4]> = text
            .glyphs
            .iter()
            .map(|glyph| rect(glyph, height))
            .collect();
        let bounds = rects.iter().fold(
            [f32::MAX, f32::MAX, f32::MIN, f32::MIN],
            |[l, t, r, b], g| [l.min(g[0]), t.min(g[1]), r.max(g[2]), b.max(g[3])],
        );
        let panels = hud(&level, width, height)?;
        let crossed: Vec<&HudQuad> = panels
            .iter()
            .filter(|quad| {
                let panel = [quad.left, quad.top, quad.right, quad.bottom];
                rects.iter().any(|&glyph| overlaps(glyph, panel))
            })
            .collect();
        println!(
            "{width}x{height}: text {:.0},{:.0} to {:.0},{:.0} ({:.1}% of the width), {} panels crossed",
            bounds[0],
            bounds[1],
            bounds[2],
            bounds[3],
            (bounds[2] - bounds[0]) / width as f32 * 100.0,
            crossed.len()
        );
        assert!(
            bounds[0] >= 0.0 && bounds[1] >= 0.0,
            "{width}x{height}: {bounds:?}"
        );
        assert!(
            bounds[2] < width as f32 && bounds[3] < height as f32 * 0.5,
            "{width}x{height}: {bounds:?}"
        );
        assert!(
            crossed.is_empty(),
            "{width}x{height}: it crosses {:?}",
            crossed
                .iter()
                .map(|quad| (&quad.bitmap, quad.image))
                .collect::<Vec<_>>()
        );
        // (left to right: each glyph starts right of the one before's start)
        assert!(
            rects.windows(2).all(|pair| pair[1][0] > pair[0][0]),
            "{width}x{height}: out of order"
        );
        // (as tall on the window as the HUD is: 1/480 of it a HUD pixel)
        let scale = height as f32 / 480.0;
        let tallest = rects
            .iter()
            .map(|glyph| (glyph[3] - glyph[1]) / scale)
            .fold(0.0, f32::max);
        assert!(
            tallest > 8.0 && tallest < 40.0,
            "{width}x{height}: {tallest}"
        );
    }
    Ok(())
}
