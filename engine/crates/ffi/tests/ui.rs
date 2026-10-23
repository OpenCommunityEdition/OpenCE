//! The shell on `ui.map`: the main menu opens and draws its panels and
//! text in the window, quickly enough at 4K. Passes without
//! assets/maps/ui.map.

use std::error::Error;
use std::path::Path;
use std::time::Instant;

use opence_ffi::ui::{main_menu_widget, Menu};
use opence_ffi::LevelGeometry;

type Result = std::result::Result<(), Box<dyn Error>>;

#[test]
fn the_main_menu_draws() -> Result {
    let map = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/ui.map");
    if !map.exists() {
        return Ok(()); // (no game data here)
    }
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("ui_cache");
    std::fs::create_dir_all(&cache)?;
    let geometry = LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )?;
    let menu = Menu::new(geometry);
    assert!(menu.open(main_menu_widget()), "no main menu in ui.map");
    menu.update(0);
    let quads = menu.draw(3840, 2160);
    let glyphs = quads.iter().filter(|quad| quad.glyph).count();
    let panels = quads.len() - glyphs;
    assert!(glyphs > 10, "{glyphs} glyphs");
    assert!(panels > 0, "{panels} panels");
    for quad in &quads {
        assert!(quad.left >= 0.0 && quad.right <= 3840.0, "{quad:?}");
        assert!(
            quad.top >= -64.0 && quad.bottom <= 2160.0 + 64.0,
            "{quad:?}"
        );
    }
    assert!(menu.glyph_atlas().height <= 4096);

    // the draw list, each frame, in well under a millisecond
    let start = Instant::now();
    let frames = 200u32;
    for frame in 0..frames {
        menu.update(u64::from(frame) * 16);
        std::hint::black_box(menu.draw(3840, 2160));
    }
    let each = start.elapsed() / frames;
    if !cfg!(debug_assertions) {
        assert!(each.as_micros() < 1000, "{each:?} a frame");
    }
    Ok(())
}
