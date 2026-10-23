//! The loading screen against progress_bar.c: the light's band, brightness
//! and trail at each progress, the voices' swell, the smoothing at any
//! frame rate, the progress measured from its start, and skipping only once
//! the map is ready, into a fade that ends on the mission.

use opence_ui::loading::{
    color, fade_alpha, mask_intensity, mask_position, volumes, LoadingScreen, Stage,
    CROSSFADE_SECONDS, SETTLE_SECONDS, SILENT_VOLUME,
};

fn close(a: f32, b: f32) -> bool {
    (a - b).abs() < 1e-3
}

/// Runs `screen` for `seconds` in frames of `step`, returning the last.
fn run(screen: &mut LoadingScreen, seconds: f32, step: f32) -> opence_ui::loading::LoadingFrame {
    let mut frame = screen.update(0.0);
    let mut time = 0.0;
    while time < seconds {
        frame = screen.update(step);
        time += step;
    }
    frame
}

#[test]
fn the_band_brightness_and_trail_follow_the_progress() {
    assert_eq!(mask_position(0.0), -128.0, "the band starts off the screen");
    assert_eq!(mask_position(1.0), 640.0, "and ends off its right edge");
    assert!(close(color(0.0), 0.0));
    assert!(close(color(0.5), 1.0), "brightest halfway");
    assert!(close(color(1.0), 0.0));
    assert_eq!(fade_alpha(0.5), 0.9);
    assert!(close(fade_alpha(0.95), 0.45));
    assert!(close(fade_alpha(1.0), 0.0), "the trail is gone at the end");
    assert!(close(mask_intensity(0, 8), 5.0 / 255.0), "dim at its edges");
    assert!(mask_intensity(63, 7) > 0.95, "bright at its middle");
    assert!(close(mask_intensity(64, 0), 5.0 / 255.0));
}

#[test]
fn the_voices_swell_each_in_its_part() {
    let start = volumes(0.0);
    assert!(start.iter().all(|&v| close(v, SILENT_VOLUME)), "{start:?}");
    let early = volumes(0.3);
    assert!(early[0] > -2000.0, "the first voice sings from the start");
    assert!(early[1..].iter().all(|&v| close(v, SILENT_VOLUME)));
    let late = volumes(0.75);
    assert!(late.iter().all(|&v| v > SILENT_VOLUME + 1000.0), "{late:?}");
    let middle = volumes(0.5);
    assert!(close(middle[0], -1500.0), "full at the middle of its part");
}

#[test]
fn the_light_moves_alike_at_any_frame_rate() {
    let mut slow = LoadingScreen::new(true);
    let mut fast = LoadingScreen::new(true);
    slow.set_progress(0.0);
    fast.set_progress(0.0);
    slow.set_progress(0.6);
    fast.set_progress(0.6);
    let at_30 = run(&mut slow, 1.0, 1.0 / 30.0);
    let at_120 = run(&mut fast, 1.0, 1.0 / 120.0);
    assert!(
        (at_30.progress - at_120.progress).abs() < 0.01,
        "{} against {}",
        at_30.progress,
        at_120.progress
    );
    assert!(at_30.progress > 0.55, "it has nearly reached the progress");
}

#[test]
fn the_progress_is_measured_from_its_start() {
    let mut screen = LoadingScreen::new(true);
    screen.set_progress(0.5);
    screen.set_progress(0.75);
    let frame = run(&mut screen, 2.0, 1.0 / 60.0);
    assert!((frame.progress - 0.5).abs() < 0.06, "{}", frame.progress);
    // it never goes back
    screen.set_progress(0.6);
    let frame = run(&mut screen, 2.0, 1.0 / 60.0);
    assert!(frame.progress > 0.44, "{}", frame.progress);
    // nothing to measure: done at once
    let mut whole = LoadingScreen::new(true);
    whole.set_progress(1.0);
    whole.set_progress(f32::NAN);
    assert!(run(&mut whole, 1.0, 1.0 / 30.0).progress > 0.9);
}

#[test]
fn a_skip_works_only_once_the_map_is_ready() {
    let mut screen = LoadingScreen::new(true);
    screen.set_progress(0.0);
    assert!(!screen.update(1.0 / 60.0).skippable);
    assert!(!screen.skip(), "not while loading");
    assert_eq!(screen.stage(), Stage::Loading);
    screen.set_ready();
    let frame = screen.update(1.0 / 60.0);
    assert_eq!(frame.stage, Stage::Settling);
    assert!(frame.skippable);
    assert!(screen.skip());
    assert!(!screen.skip(), "once");
    let frame = screen.update(CROSSFADE_SECONDS / 2.0);
    assert_eq!(frame.stage, Stage::Crossfade);
    assert!(close(frame.screen_alpha, 0.5), "{}", frame.screen_alpha);
    let frame = screen.update(CROSSFADE_SECONDS);
    assert_eq!(frame.stage, Stage::Done);
    assert_eq!(frame.screen_alpha, 0.0);
    screen.set_ready();
    assert_eq!(screen.stage(), Stage::Done, "ready again changes nothing");
}

#[test]
fn unskipped_the_light_finishes_and_fades() {
    let mut screen = LoadingScreen::new(true);
    screen.set_progress(0.0);
    screen.set_ready();
    let mut frame = screen.update(0.0);
    let mut time = 0.0;
    while frame.stage == Stage::Settling {
        frame = screen.update(1.0 / 60.0);
        time += 1.0 / 60.0;
    }
    assert!(time <= SETTLE_SECONDS + 0.02, "{time}");
    assert_eq!(frame.stage, Stage::Crossfade);
    assert!(frame.screen_alpha > 0.9, "the fade has only begun");
    // a hitch is not taken whole
    let frame = screen.update(10.0);
    assert_eq!(frame.stage, Stage::Crossfade);
    let frame = screen.update(f32::INFINITY);
    assert_eq!(frame.stage, Stage::Crossfade);
    assert_eq!(run(&mut screen, 1.0, 0.1).stage, Stage::Done);
}

#[test]
fn a_plain_screen_is_dark_and_silent_and_fades_at_once() {
    let mut screen = LoadingScreen::new(false);
    screen.set_progress(0.0);
    let frame = run(&mut screen, 0.5, 1.0 / 30.0);
    assert_eq!(frame.color, 0.0);
    assert_eq!(frame.fade_alpha, 0.0);
    assert_eq!(frame.volumes, [-10_000.0; 4]);
    assert!(!frame.skippable);
    screen.set_ready();
    assert_eq!(screen.stage(), Stage::Crossfade);
}
