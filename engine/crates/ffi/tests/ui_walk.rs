//! A walk through every screen of `ui.map` and the missions' pause menus,
//! by the pointer and the back key, as a player would: the campaign
//! reaches its difficulty select and starts a mission; the multiplayer,
//! server list and profile screens open; the pause menus restart, quit and
//! leave. Passes without assets/maps.

use std::collections::{BTreeMap, BTreeSet, VecDeque};
use std::error::Error;
use std::path::{Path, PathBuf};
use std::sync::Arc;

use opence_ffi::ui::{main_menu_widget, pause_menu_widget, Menu, MenuAction, MenuEntry, MenuKey};
use opence_ffi::LevelGeometry;

type Result = std::result::Result<(), Box<dyn Error>>;

const WIDTH: u32 = 1280;
const HEIGHT: u32 = 960;

/// What the player does: clicks the screen's `n`th target, or goes back.
#[derive(Clone, Copy, Debug)]
enum Step {
    Click(usize),
    Back,
}

fn maps() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps")
}

fn load(name: &str) -> std::result::Result<Option<Arc<LevelGeometry>>, Box<dyn Error>> {
    let map = maps().join(format!("{name}.map"));
    if !map.exists() {
        return Ok(None);
    }
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("ui_cache");
    std::fs::create_dir_all(&cache)?;
    Ok(Some(LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )?))
}

fn entries(names: &[&str]) -> Vec<MenuEntry> {
    names
        .iter()
        .map(|&name| MenuEntry {
            map: name.into(),
            name: name.to_uppercase(),
        })
        .collect()
}

/// A menu on `geometry` with `widget` open, the steps `steps` taken: the
/// screen it is on and what it asked for on the way.
fn replay(
    geometry: &Arc<LevelGeometry>,
    widget: &str,
    steps: &[Step],
) -> (Arc<Menu>, Vec<MenuAction>) {
    let menu = Menu::new(geometry.clone());
    menu.set_levels(entries(&["a10", "a30", "a50"]));
    menu.set_maps(entries(&["bloodgulch", "sidewinder"]));
    menu.set_servers(vec!["a server".into()]);
    assert!(menu.open(widget.into()), "no {widget}");
    let mut now = 0;
    let mut actions = Vec::new();
    menu.update(now);
    for &step in steps {
        match step {
            Step::Click(n) => {
                if let Some(point) = menu.targets(WIDTH, HEIGHT).get(n) {
                    menu.pointer_moved(point.x, point.y, WIDTH, HEIGHT);
                    menu.click(point.x, point.y, WIDTH, HEIGHT, false);
                }
            }
            Step::Back => menu.key(MenuKey::Escape),
        }
        // long enough for the screens that close themselves
        for _ in 0..4 {
            now += 2_000;
            menu.update(now);
        }
        actions.extend(menu.take_actions());
    }
    (menu, actions)
}

/// Where the menu is: its screen and what the pointer can act on there
/// (a screen's widgets can be replaced while it stays open).
fn state(menu: &Menu) -> Option<String> {
    let points: Vec<String> = menu
        .targets(WIDTH, HEIGHT)
        .iter()
        .map(|point| format!("{:.0},{:.0}", point.x, point.y))
        .collect();
    Some(format!("{} {}", menu.screen()?, points.join(" ")))
}

/// Every screen reached from `widget`, by the first steps found to it,
/// and every action asked for on the way.
fn walk(
    geometry: &Arc<LevelGeometry>,
    widget: &str,
) -> (BTreeMap<String, Vec<Step>>, Vec<MenuAction>) {
    let mut screens = BTreeMap::new();
    let mut seen = BTreeSet::new();
    let mut actions = Vec::new();
    let mut queue = VecDeque::from([Vec::new()]);
    while let Some(steps) = queue.pop_front() {
        let (menu, _) = replay(geometry, widget, &steps);
        let (Some(screen), Some(here)) = (menu.screen(), state(&menu)) else {
            continue;
        };
        if steps.len() > 12 || seen.len() > 600 || !seen.insert(here) {
            continue;
        }
        screens.entry(screen).or_insert_with(|| steps.clone());
        let count = menu.targets(WIDTH, HEIGHT).len();
        for next in (0..count).map(Step::Click).chain([Step::Back]) {
            let mut longer = steps.clone();
            longer.push(next);
            let (after, asked) = replay(geometry, widget, &longer);
            actions.extend(asked);
            if after.screen().is_some() {
                queue.push_back(longer);
            }
        }
    }
    (screens, actions)
}

fn reached(screens: &BTreeMap<String, Vec<Step>>, leaf: &str) -> bool {
    screens.keys().any(|screen| screen.ends_with(leaf))
}

#[test]
fn every_screen_of_the_shell_is_reached() -> Result {
    let Some(ui) = load("ui")? else {
        return Ok(()); // (no game data here)
    };
    let (screens, actions) = walk(&ui, &main_menu_widget());
    let names: Vec<&String> = screens.keys().collect();
    for leaf in [
        "main_menu",
        "difficulty_select_list_screen",
        "multiplayer_type_select_screen",
        "server_list_screen",
        "player_profile_select_screen",
    ] {
        assert!(reached(&screens, leaf), "{leaf} not reached: {names:#?}");
    }
    assert!(
        actions
            .iter()
            .any(|action| matches!(action, MenuAction::PlayCampaign { .. })),
        "no mission started: {actions:?}"
    );
    assert!(
        actions
            .iter()
            .any(|action| matches!(action, MenuAction::JoinServer { index: 0 })),
        "no server joined: {actions:?}"
    );
    println!("{} screens: {names:#?}", screens.len());
    Ok(())
}

#[test]
fn the_pause_menus_restart_quit_and_leave() -> Result {
    if let Some(a10) = load("a10")? {
        let (screens, actions) = walk(&a10, &pause_menu_widget(false));
        assert!(reached(&screens, "pause_game"), "{screens:?}");
        for wanted in [
            MenuAction::RestartCheckpoint,
            MenuAction::RestartLevel,
            MenuAction::ReturnToMainMenu,
        ] {
            assert!(actions.contains(&wanted), "{wanted:?} not in {actions:?}");
        }
        // resume closes the menu
        let (menu, _) = replay(&a10, &pause_menu_widget(false), &[Step::Back]);
        assert!(!menu.is_open());
    }
    if let Some(bloodgulch) = load("bloodgulch")? {
        let (screens, actions) = walk(&bloodgulch, &pause_menu_widget(true));
        assert!(reached(&screens, "1p_pause_game"), "{screens:?}");
        assert!(actions.contains(&MenuAction::LeaveGame), "{actions:?}");
    }
    Ok(())
}
