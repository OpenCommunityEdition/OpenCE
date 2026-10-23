//! The loading screen's model as Swift sees it: gains that fall with the
//! fade into the mission, the voices' rates, the mask, and a skip only once
//! the map is ready.

use opence_ffi::loading::{mask_pixels, LoadingScreenModel, LoadingStage, MASK_HEIGHT, MASK_WIDTH};

#[test]
fn the_model_fades_its_voices_with_the_screen() {
    let screen = LoadingScreenModel::new(true);
    screen.set_progress(0.0);
    screen.set_progress(0.5);
    let mut view = screen.update(0.0);
    for _ in 0..60 {
        view = screen.update(1.0 / 30.0);
    }
    assert_eq!(view.stage, LoadingStage::Loading);
    assert_eq!(view.gains.len(), 4);
    assert_eq!(view.rates.len(), 4);
    assert!(view.gains[0] > 0.1, "{:?}", view.gains);
    assert!(view.rates.iter().all(|&r| r > 0.2 && r < 0.6));
    assert!(!view.skippable && !screen.skip());
    screen.set_ready();
    assert!(screen.update(1.0 / 60.0).skippable);
    assert!(screen.skip());
    let halfway = screen.update(0.25);
    assert_eq!(halfway.stage, LoadingStage::Crossfade);
    assert!(halfway.gains[0] < view.gains[0]);
    let done = screen.update(1.0);
    assert_eq!(done.stage, LoadingStage::Done);
    assert!(done.gains.iter().all(|&g| g == 0.0));
}

#[test]
fn a_plain_screen_settles_into_the_fade_at_once() {
    let screen = LoadingScreenModel::new(false);
    screen.set_ready();
    let view = screen.update(0.1);
    assert_eq!(view.stage, LoadingStage::Crossfade);
    assert!(view.gains.iter().all(|&g| g == 0.0));
    let mut stage = view.stage;
    for _ in 0..30 {
        stage = screen.update(0.1).stage;
    }
    assert_eq!(stage, LoadingStage::Done);
    let mut settling = LoadingScreenModel::new(true);
    settling.set_ready();
    for _ in 0..200 {
        settling.update(1.0 / 30.0);
    }
    assert_eq!(settling.update(0.0).stage, LoadingStage::Done);
    settling = LoadingScreenModel::new(true);
    assert_eq!(settling.mask_pixels().len(), 128 * 16);
}

#[test]
fn the_mask_is_bright_in_its_middle() {
    let mask = mask_pixels();
    assert_eq!(mask.len(), (MASK_WIDTH * MASK_HEIGHT) as usize);
    assert_eq!(mask[0], 5);
    assert!(mask[(7 * MASK_WIDTH + 63) as usize] > 240);
}
