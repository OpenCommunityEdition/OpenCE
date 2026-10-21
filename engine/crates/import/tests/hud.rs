//! The HUD tags bloodgulch's level imports: the cyborg's unit HUD, a HUD
//! for each weapon, the grenades', the digits and the HUD globals, with
//! the bitmaps they draw, and the first-person models. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{HudAnchor, Level};
use opence_import::{import_level, Cache};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

#[test]
fn bloodgulch_imports_its_hud() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let has_bitmap = |tag: &Option<opence_content::TagRef>| {
        tag.as_ref().is_some_and(|tag| {
            level.bitmaps.iter().any(|bitmap| &bitmap.tag == tag)
                || level.resource_bitmaps.contains(tag)
        })
    };
    let unit = level.unit_huds.first().ok_or("no unit hud")?;
    assert!(has_bitmap(&unit.health_meter.bitmap));
    assert!(has_bitmap(&unit.motion_sensor_background.bitmap));
    assert!(unit.health_min_cutoff < unit.health_max_cutoff);
    assert_eq!(unit.anchor, HudAnchor::TopRight);

    assert!(level.weapon_huds.len() >= 6);
    for hud in &level.weapon_huds {
        for crosshair in &hud.crosshairs {
            assert!(crosshair.bitmap.is_none() || has_bitmap(&crosshair.bitmap));
        }
    }
    assert!(level
        .weapon_huds
        .iter()
        .any(|hud| !hud.crosshairs.is_empty()));
    assert!(level.weapon_huds.iter().any(|hud| !hud.numbers.is_empty()));
    assert_eq!(level.grenade_huds.len(), 2);
    let digits = level.hud_digits.as_ref().ok_or("no digits")?;
    assert!(has_bitmap(&digits.bitmap));
    assert!(digits.screen_width > 0);
    let globals = level.hud_globals.as_ref().ok_or("no hud globals")?;
    assert!(globals.motion_sensor_range > 0.0);

    let hands = level
        .globals
        .first_person_hands
        .as_ref()
        .ok_or("no hands")?;
    assert!(level.models.iter().any(|model| &model.tag == hands));
    for object in &level.objects {
        if let Some(model) = object
            .weapon
            .as_ref()
            .and_then(|weapon| weapon.first_person_model.as_ref())
        {
            assert!(level.models.iter().any(|imported| &imported.tag == model));
        }
    }
    Ok(())
}
