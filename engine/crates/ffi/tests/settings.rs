//! The settings through TOML: they come back as they went, a missing or
//! out-of-range setting is the stock one, and a missing file gives the
//! stock settings.

use opence_ffi::settings::{
    load_settings, save_settings, settings_from_toml, settings_to_toml, stock_settings, KeyBinding,
};

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

#[test]
fn settings_round_trip_through_toml() -> Result {
    let mut settings = stock_settings();
    settings.game.player_name = "Chief".into();
    settings.game.game_type = "ctf".into();
    settings.game.max_players = 8;
    settings.controls.mouse_sensitivity = 0.002;
    settings.controls.invert_mouse = true;
    settings.controls.bindings = vec![KeyBinding {
        action: "jump".into(),
        keys: vec![49, 0x101],
    }];
    settings.video.anti_aliasing = "native".into();
    settings.video.render_scale = 0.75;
    settings.audio.music = 0.25;
    let text = settings_to_toml(settings.clone())?;
    assert!(
        text.contains("[controls]") && text.contains("player_name = \"Chief\""),
        "{text}"
    );
    assert_eq!(settings_from_toml(text)?, settings);
    Ok(())
}

#[test]
fn missing_and_out_of_range_settings_are_the_stock_ones() -> Result {
    let stock = stock_settings();
    let read = settings_from_toml(
        "[game]\nmax_players = 40\nplayer_name = \"\"\nmap = \"../etc\"\n\
         [video]\nrender_scale = 3.0\nanti_aliasing = \"fxaa\"\n\
         [audio]\nmaster = -1.0\neffects = 0.5\n[unknown]\nkey = 1\n"
            .into(),
    )?;
    assert_eq!(read.game.max_players, stock.game.max_players);
    assert_eq!(read.game.player_name, stock.game.player_name);
    assert_eq!(read.game.map, stock.game.map);
    assert_eq!(read.video, stock.video);
    assert_eq!((read.audio.master, read.audio.effects), (1.0, 0.5));
    assert_eq!(read.controls, stock.controls);
    assert!(settings_from_toml("not = [toml".into()).is_err());
    Ok(())
}

#[test]
fn settings_are_saved_and_loaded() -> Result {
    let folder = std::path::Path::new(env!("CARGO_TARGET_TMPDIR")).join("settings_test");
    let _ = std::fs::remove_dir_all(&folder);
    let path = folder
        .join("OpenCE/settings.toml")
        .to_string_lossy()
        .into_owned();
    assert_eq!(load_settings(path.clone()), stock_settings());
    let mut settings = stock_settings();
    settings.game.server_name = "Gulch".into();
    save_settings(settings.clone(), path.clone())?;
    assert_eq!(load_settings(path), settings);
    Ok(())
}

#[test]
fn the_launchers_settings_are_kept() -> Result {
    let stock = stock_settings();
    assert_eq!(stock.data.maps_folder, "", "the checkout's assets/maps");
    assert_eq!(stock.video.display, 0);
    assert!(stock.ui.loading_screen, "on by default");
    let mut settings = stock.clone();
    settings.data.maps_folder = "/Volumes/Halo/maps".into();
    settings.data.custom_maps_folder = "/Volumes/Halo/custom_maps".into();
    settings.video.display = 1;
    settings.video.full_screen = true;
    settings.ui.loading_screen = false;
    assert_eq!(
        settings_from_toml(settings_to_toml(settings.clone())?)?,
        settings
    );
    let read = settings_from_toml(
        "[data]\nmaps_folder = \"relative/maps\"\n[video]\ndisplay = 99\n".into(),
    )?;
    assert_eq!(read.data, stock.data, "a folder must be absolute");
    assert_eq!(
        read.video.display, 99,
        "a display not there is the app's to replace"
    );
    Ok(())
}
