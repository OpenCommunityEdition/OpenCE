//! What the game does for the shell's widgets in the app: the functions
//! their handlers run, kept to what one player at a keyboard can do, and
//! the data their lists and text boxes show. What a function asks of the
//! engine (a mission to start, a game to host or join, a pause menu's
//! choice) is queued as a [`MenuAction`] for the app to take.

use opence_content::{TagRef, Ui, WidgetKind};
use opence_ui::{Feedback, Host, Id, Outcome, Shell};

/// A level or map the menus list: its name in the maps folder and the
/// name they show.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Record)]
pub struct MenuEntry {
    /// Its map's name, as the app loads it (`a10`, `bloodgulch`).
    pub map: String,
    /// What the menus call it.
    pub name: String,
}

/// What the menus ask of the engine.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum MenuAction {
    /// Start the campaign at `map` on `difficulty` (0 easy to 3
    /// legendary).
    PlayCampaign {
        /// The mission's map.
        map: String,
        /// Its difficulty.
        difficulty: u8,
    },
    /// Host a game on `map`.
    HostGame {
        /// The map.
        map: String,
    },
    /// Join the server at `index` in the list the app gave.
    JoinServer {
        /// Its index.
        index: u32,
    },
    /// Go back to the last checkpoint.
    RestartCheckpoint,
    /// Start the mission again.
    RestartLevel,
    /// Leave the mission for the main menu.
    ReturnToMainMenu,
    /// Leave the network game.
    LeaveGame,
    /// Quit the app.
    Quit,
}

/// The multiplayer game types, as the stock profiles name them.
pub const GAME_TYPES: [&str; 5] = [
    "Slayer",
    "Capture the Flag",
    "King of the Hill",
    "Oddball",
    "Race",
];

/// Screen items one player at a keyboard can't use: split screen, two
/// players' co-op and the Xbox's demos.
const UNUSABLE_ITEMS: [&str; 3] = [
    "multiplayer_type_split_item",
    "multiplayer_type_coop_item",
    "main_menu_item_game_demos",
];

/// The shell's host in the app.
#[derive(Debug)]
pub(crate) struct MenuHost {
    /// The sounds the widgets played, by their tag paths.
    pub(crate) sounds: Vec<String>,
    /// What the menus asked for.
    pub(crate) actions: Vec<MenuAction>,
    /// The campaign's missions, in order.
    pub(crate) levels: Vec<MenuEntry>,
    /// The multiplayer maps.
    pub(crate) maps: Vec<MenuEntry>,
    /// The servers found, by name.
    pub(crate) servers: Vec<String>,
    /// The player's profile name.
    pub(crate) profile: String,
    /// The build the version text box shows.
    pub(crate) version: String,
    /// The mission chosen.
    level: usize,
    /// The difficulty chosen.
    difficulty: u8,
    /// The multiplayer map chosen.
    map: usize,
    /// Whether the multiplayer screens are setting up a game to host.
    hosting: bool,
}

impl Default for MenuHost {
    fn default() -> Self {
        Self {
            sounds: Vec::new(),
            actions: Vec::new(),
            levels: Vec::new(),
            maps: Vec::new(),
            servers: Vec::new(),
            profile: "Player".into(),
            version: crate::engine_version(),
            level: 0,
            difficulty: 1,
            map: 0,
            hosting: false,
        }
    }
}

/// `widget` and every widget below it, in order.
fn descendants(shell: &Shell, widget: Id) -> Vec<Id> {
    let mut found = vec![widget];
    let mut next = 0;
    while let Some(&id) = found.get(next) {
        if let Some(instance) = shell.get(id) {
            found.extend(instance.children.iter().copied());
        }
        next += 1;
    }
    found
}

/// Whether `id`'s definition's tag path ends with `name`.
fn named(shell: &Shell, ui: &Ui, id: Id, name: &str) -> bool {
    shell
        .definition(ui, id)
        .is_some_and(|definition| definition.tag.path.ends_with(name))
}

/// The item chosen in the list `id` is, or is in: the game's index for a
/// list of the game's items, its focused child's place otherwise.
fn chosen_in(shell: &Shell, id: Id) -> Option<usize> {
    let instance = shell.get(id)?;
    if !matches!(
        instance.kind,
        WidgetKind::SpinnerList | WidgetKind::ColumnList
    ) {
        return None;
    }
    if instance.list.from_game {
        return usize::try_from(instance.list.selected).ok();
    }
    let focused = instance.focused?;
    instance.children.iter().position(|&child| child == focused)
}

/// The item chosen in the list nearest `widget`: itself, then the lists
/// it is in, then the lists on the focus path of its screen.
fn chosen(shell: &Shell, widget: Id) -> Option<usize> {
    let mut node = Some(widget);
    while let Some(id) = node {
        if let Some(index) = chosen_in(shell, id) {
            return Some(index);
        }
        node = shell.get(id).and_then(|instance| instance.parent);
    }
    let root = shell.root_of(widget);
    let mut path = vec![root];
    while let Some(next) = path
        .last()
        .and_then(|&id| shell.get(id))
        .and_then(|instance| instance.focused)
    {
        if path.contains(&next) {
            break;
        }
        path.push(next);
    }
    path.into_iter().find_map(|id| chosen_in(shell, id))
}

/// Gives the list `list` the game's items `names`: a spinner shows the
/// one selected, or its item widgets the ones around it; a column list's
/// rows show one each, those past the last disabled.
fn fill_list(shell: &mut Shell, ui: &Ui, list: Id, names: &[String]) {
    let count = i16::try_from(names.len()).unwrap_or(i16::MAX);
    shell.set_list_items(list, count);
    let Some(instance) = shell.get(list) else {
        return;
    };
    let (kind, selected, children, focused) = (
        instance.kind,
        usize::try_from(instance.list.selected).unwrap_or(0),
        instance.children.clone(),
        instance.focused,
    );
    let has_strings = |shell: &Shell, id: Id| {
        shell
            .definition(ui, id)
            .is_some_and(|definition| definition.text.string_list.is_some())
    };
    if children.is_empty() {
        if kind == WidgetKind::SpinnerList && !has_strings(shell, list) {
            shell.set_text(list, names.get(selected).cloned().unwrap_or_default());
        }
        return;
    }
    let here = focused
        .and_then(|focused| children.iter().position(|&child| child == focused))
        .unwrap_or(0);
    for (place, &child) in children.iter().enumerate() {
        let entry = if kind == WidgetKind::ColumnList {
            Some(place)
        } else {
            (selected + place).checked_sub(here)
        };
        let name = entry.and_then(|entry| names.get(entry));
        if kind == WidgetKind::ColumnList {
            shell.set_disabled(child, name.is_none());
        }
        let text_box = descendants(shell, child).into_iter().find(|&id| {
            shell
                .get(id)
                .is_some_and(|instance| instance.kind == WidgetKind::TextBox)
                && !has_strings(shell, id)
        });
        if let Some(text_box) = text_box {
            shell.set_text(text_box, name.cloned().unwrap_or_default());
        }
    }
}

impl MenuHost {
    fn names(entries: &[MenuEntry]) -> Vec<String> {
        entries.iter().map(|entry| entry.name.clone()).collect()
    }

    /// Disables the items of `widget`'s screen one player at a keyboard
    /// can't use.
    fn disable_unusable(shell: &mut Shell, ui: &Ui, widget: Id) {
        let root = shell.root_of(widget);
        for id in descendants(shell, root) {
            if UNUSABLE_ITEMS.iter().any(|name| named(shell, ui, id, name)) {
                shell.set_disabled(id, true);
            }
        }
    }
}

impl Host for MenuHost {
    fn run(&mut self, function: i16, shell: &mut Shell, ui: &Ui, widget: Id) -> Outcome {
        match function {
            // the campaign's missions, the one chosen, a new campaign
            5 | 6 => {
                fill_list(shell, ui, widget, &Self::names(&self.levels));
                Outcome::Done
            }
            8 => {
                self.level = chosen(shell, widget).unwrap_or(0);
                Outcome::Done
            }
            96 => {
                self.level = 0;
                Outcome::Done
            }
            // the difficulty, and the mission started on it
            9 => {
                self.difficulty = chosen(shell, widget)
                    .and_then(|index| u8::try_from(index.min(3)).ok())
                    .unwrap_or(1);
                Outcome::Done
            }
            10 | 73 => match self.levels.get(self.level) {
                Some(level) => {
                    self.actions.push(MenuAction::PlayCampaign {
                        map: level.map.clone(),
                        difficulty: self.difficulty,
                    });
                    Outcome::Done
                }
                None => Outcome::Failed,
            },
            // the pause menus
            11 => {
                self.actions.push(MenuAction::RestartCheckpoint);
                Outcome::Done
            }
            12 => {
                self.actions.push(MenuAction::RestartLevel);
                Outcome::Done
            }
            13 => {
                self.actions.push(MenuAction::ReturnToMainMenu);
                Outcome::Done
            }
            19 | 72 => {
                self.actions.push(MenuAction::LeaveGame);
                Outcome::Done
            }
            // the network game's screens
            16 => {
                self.hosting = false;
                fill_list(shell, ui, widget, &self.servers);
                Outcome::Done
            }
            17 | 92 => {
                self.hosting = true;
                Outcome::Done
            }
            20 => match chosen(shell, widget) {
                Some(index) if index < self.servers.len() => {
                    self.actions.push(MenuAction::JoinServer {
                        index: u32::try_from(index).unwrap_or(u32::MAX),
                    });
                    Outcome::Done
                }
                _ => Outcome::Failed,
            },
            26 => {
                fill_list(shell, ui, widget, &Self::names(&self.maps));
                Outcome::Done
            }
            28 => {
                self.map = chosen(shell, widget).unwrap_or(0);
                Outcome::Done
            }
            29 => {
                let types: Vec<String> = GAME_TYPES.iter().map(|&name| name.into()).collect();
                fill_list(shell, ui, widget, &types);
                Outcome::Done
            }
            81 | 85 if self.hosting => match self.maps.get(self.map) {
                Some(map) => {
                    if !self
                        .actions
                        .iter()
                        .any(|action| matches!(action, MenuAction::HostGame { .. }))
                    {
                        self.actions.push(MenuAction::HostGame {
                            map: map.map.clone(),
                        });
                    }
                    Outcome::Done
                }
                None => Outcome::Failed,
            },
            // the profiles: there is one, the player's
            34 => {
                fill_list(shell, ui, widget, std::slice::from_ref(&self.profile));
                Outcome::Done
            }
            // the screens' first frames
            23 | 24 => {
                Self::disable_unusable(shell, ui, widget);
                Outcome::Done
            }
            // split screen, co-op and the demos need more players or an Xbox
            21 | 22 | 86 | 87 => Outcome::Failed,
            95 => {
                self.actions.push(MenuAction::Quit);
                Outcome::Done
            }
            7 | 14 | 15 | 18 | 27 | 30 | 31 | 35 | 36 | 83 | 84 | 91 | 97 | 99 | 101 => {
                Outcome::Done
            }
            _ => Outcome::Unwired,
        }
    }

    fn sound(&mut self, sound: &TagRef) {
        self.sounds.push(sound.path.clone());
    }

    fn feedback(&mut self, feedback: Feedback) {
        self.sounds.push(feedback.sound().into());
    }

    fn data_input(&mut self, input: i16, shell: &mut Shell, ui: &Ui, widget: Id) {
        match input {
            6 => fill_list(shell, ui, widget, &Self::names(&self.levels)),
            8 => shell.set_text(widget, format!("OpenCE {}", self.version)),
            9 => fill_list(shell, ui, widget, &self.servers),
            14 | 16 => fill_list(shell, ui, widget, std::slice::from_ref(&self.profile)),
            22 => fill_list(shell, ui, widget, &Self::names(&self.maps)),
            23 => shell.set_text(widget, self.profile.clone()),
            27 => {
                let name = self.maps.get(self.map).map(|map| map.name.clone());
                shell.set_text(widget, name.unwrap_or_default());
            }
            _ => {}
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{ChildWidget, Group, Widget, WidgetList, WidgetText};
    use opence_ui::NONE;

    fn tag(path: &str) -> TagRef {
        TagRef {
            group: Group(*b"DeLa"),
            path: path.into(),
        }
    }

    fn widget(path: &str, kind: WidgetKind, children: &[&str]) -> Widget {
        Widget {
            tag: tag(path),
            kind,
            controller: NONE,
            name: path.into(),
            bounds: [0, 0, 480, 640],
            flags: 0,
            auto_close_ms: 0,
            auto_close_fade_ms: 0,
            background: None,
            game_data_inputs: Vec::new(),
            handlers: Vec::new(),
            replacements: Vec::new(),
            text: WidgetText::default(),
            list: WidgetList::default(),
            extended_description: None,
            conditionals: Vec::new(),
            children: children
                .iter()
                .map(|&child| ChildWidget {
                    widget: Some(tag(child)),
                    name: child.into(),
                    flags: 0,
                    controller: NONE,
                    offset: [0, 0],
                })
                .collect(),
        }
    }

    /// A screen holding a column list of two rows, each a text box, a
    /// spinner with no items of its own and the split screen item.
    fn ui() -> Ui {
        let split = "multiplayer_type_split_item";
        let mut widgets = vec![
            widget("screen", WidgetKind::Container, &["list", "spinner", split]),
            widget("list", WidgetKind::ColumnList, &["row0", "row1"]),
            widget("row0", WidgetKind::TextBox, &[]),
            widget("row1", WidgetKind::TextBox, &[]),
            widget("spinner", WidgetKind::SpinnerList, &[]),
            widget(split, WidgetKind::TextBox, &[]),
        ];
        widgets.sort_by(|a, b| a.tag.cmp(&b.tag));
        Ui {
            widgets,
            ..Ui::default()
        }
    }

    fn entries(names: &[&str]) -> Vec<MenuEntry> {
        names
            .iter()
            .map(|&name| MenuEntry {
                map: name.to_lowercase(),
                name: name.into(),
            })
            .collect()
    }

    struct Opened {
        ui: Ui,
        shell: Shell,
        host: MenuHost,
        screen: Id,
    }

    fn opened() -> Opened {
        let ui = ui();
        let mut shell = Shell::new();
        let mut host = MenuHost {
            levels: entries(&["A10", "A30"]),
            maps: entries(&["Bloodgulch"]),
            ..MenuHost::default()
        };
        let Some(screen) = shell.open(&ui, &mut host, &tag("screen"), NONE) else {
            panic!("the screen did not open");
        };
        Opened {
            ui,
            shell,
            host,
            screen,
        }
    }

    fn find(opened: &Opened, path: &str) -> Id {
        let Some(id) = opened.shell.find(&opened.ui, opened.screen, &tag(path)) else {
            panic!("no {path}");
        };
        id
    }

    #[test]
    fn a_column_list_shows_the_games_items_and_disables_the_rest() {
        let mut opened = opened();
        let list = find(&opened, "list");
        let (row0, row1) = (find(&opened, "row0"), find(&opened, "row1"));
        let (ui, shell, host) = (&opened.ui, &mut opened.shell, &mut opened.host);
        assert_eq!(host.run(16, shell, ui, list), Outcome::Done);
        assert!(shell.get(row0).is_some_and(|row| row.disabled));
        host.servers = vec!["Lan party".into()];
        host.data_input(9, shell, ui, list);
        assert_eq!(shell.text(ui, row0).as_deref(), Some("Lan party"));
        assert!(shell.get(row1).is_some_and(|row| row.disabled));
        assert_eq!(chosen(shell, list), Some(0));
        assert_eq!(host.run(20, shell, ui, list), Outcome::Done);
        assert_eq!(host.actions, [MenuAction::JoinServer { index: 0 }]);
        host.servers.clear();
        assert_eq!(host.run(20, shell, ui, list), Outcome::Failed);
    }

    #[test]
    fn the_campaign_starts_on_the_mission_and_difficulty_chosen() {
        let mut opened = opened();
        let spinner = find(&opened, "spinner");
        let (ui, shell, host) = (&opened.ui, &mut opened.shell, &mut opened.host);
        assert_eq!(host.run(5, shell, ui, spinner), Outcome::Done);
        assert_eq!(
            shell.get(spinner).and_then(|s| s.text.clone()).as_deref(),
            Some("A10")
        );
        shell.move_item(ui, spinner, true);
        host.data_input(6, shell, ui, spinner);
        assert_eq!(
            shell.get(spinner).and_then(|s| s.text.clone()).as_deref(),
            Some("A30")
        );
        assert_eq!(host.run(8, shell, ui, spinner), Outcome::Done);
        assert_eq!(host.run(9, shell, ui, spinner), Outcome::Done);
        assert_eq!(host.run(73, shell, ui, spinner), Outcome::Done);
        assert_eq!(
            host.actions,
            [MenuAction::PlayCampaign {
                map: "a30".into(),
                difficulty: 1
            }]
        );
        // a new campaign starts at the first mission
        host.actions.clear();
        host.run(96, shell, ui, spinner);
        host.run(10, shell, ui, spinner);
        assert_eq!(
            host.actions,
            [MenuAction::PlayCampaign {
                map: "a10".into(),
                difficulty: 1
            }]
        );
        host.levels.clear();
        assert_eq!(host.run(73, shell, ui, spinner), Outcome::Failed);
    }

    #[test]
    fn hosting_asks_for_the_map_chosen_once() {
        let mut opened = opened();
        let spinner = find(&opened, "spinner");
        let (ui, shell, host) = (&opened.ui, &mut opened.shell, &mut opened.host);
        assert_eq!(host.run(81, shell, ui, spinner), Outcome::Unwired);
        host.run(17, shell, ui, spinner);
        host.run(26, shell, ui, spinner);
        host.run(28, shell, ui, spinner);
        host.data_input(27, shell, ui, spinner);
        assert_eq!(
            shell.get(spinner).and_then(|s| s.text.clone()).as_deref(),
            Some("Bloodgulch")
        );
        assert_eq!(host.run(81, shell, ui, spinner), Outcome::Done);
        assert_eq!(host.run(85, shell, ui, spinner), Outcome::Done);
        assert_eq!(
            host.actions,
            [MenuAction::HostGame {
                map: "bloodgulch".into()
            }]
        );
        host.maps.clear();
        host.actions.clear();
        assert_eq!(host.run(81, shell, ui, spinner), Outcome::Failed);
        assert_eq!(host.run(29, shell, ui, spinner), Outcome::Done);
        assert_eq!(
            shell.get(spinner).and_then(|s| s.text.clone()).as_deref(),
            Some(GAME_TYPES[0])
        );
    }

    #[test]
    fn the_pause_menus_and_the_rest() {
        let mut opened = opened();
        let screen = opened.screen;
        let split = find(&opened, "multiplayer_type_split_item");
        let (ui, shell, host) = (&opened.ui, &mut opened.shell, &mut opened.host);
        for function in [11, 12, 13, 72, 95] {
            assert_eq!(host.run(function, shell, ui, screen), Outcome::Done);
        }
        assert_eq!(
            host.actions,
            [
                MenuAction::RestartCheckpoint,
                MenuAction::RestartLevel,
                MenuAction::ReturnToMainMenu,
                MenuAction::LeaveGame,
                MenuAction::Quit
            ]
        );
        assert_eq!(host.run(24, shell, ui, screen), Outcome::Done);
        assert!(shell.get(split).is_some_and(|item| item.disabled));
        assert_eq!(host.run(22, shell, ui, screen), Outcome::Failed);
        assert_eq!(host.run(101, shell, ui, screen), Outcome::Done);
        assert_eq!(host.run(64, shell, ui, screen), Outcome::Unwired);
        assert_eq!(host.run(34, shell, ui, screen), Outcome::Done);
        assert_eq!(chosen(shell, screen), Some(0), "the screen's first list");
        host.data_input(8, shell, ui, split);
        assert_eq!(
            shell.text(ui, split),
            Some(format!("OpenCE {}", crate::engine_version()))
        );
        host.data_input(23, shell, ui, split);
        assert_eq!(shell.text(ui, split).as_deref(), Some("Player"));
        host.data_input(14, shell, ui, screen);
        host.data_input(22, shell, ui, screen);
        host.data_input(0, shell, ui, screen);
        host.feedback(Feedback::Cursor);
        host.sound(&tag("sound\\click"));
        assert_eq!(host.sounds, ["sound\\sfx\\ui\\cursor", "sound\\click"]);
    }
}
