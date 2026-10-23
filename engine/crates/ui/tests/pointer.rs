//! The keyboard and the mouse over the tree tests' widgets: the keys press the
//! pad's buttons, and the pointer focuses, clicks and scrolls by the bounds.

use opence_content::{
    ChildWidget, ConditionalWidget, EventHandler, Group, StringList, TagRef, TextReplacement, Ui,
    Widget, WidgetKind, WidgetList, WidgetText,
};
use opence_ui::{
    key_input, Feedback, Host, Id, Input, Key, MouseButton, Outcome, Shell, TargetKind,
    CONDITIONAL_IF_FUNCTION_FAILS, LIST_ITEMS_FROM_STRING_LIST, NONE,
};

fn tag(path: &str) -> TagRef {
    TagRef {
        group: Group(*b"DeLa"),
        path: path.into(),
    }
}

fn strings_tag(path: &str) -> TagRef {
    TagRef {
        group: Group(*b"ustr"),
        path: path.into(),
    }
}

fn widget(path: &str, kind: WidgetKind) -> Widget {
    Widget {
        tag: tag(path),
        kind,
        controller: NONE,
        name: path.into(),
        bounds: [10, 20, 40, 120],
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
        children: Vec::new(),
    }
}

fn handler(event: i16, flags: u32, function: i16, target: Option<&str>) -> EventHandler {
    EventHandler {
        flags,
        event,
        function,
        widget: target.map(tag),
        sound: None,
        script: String::new(),
    }
}

fn child(path: &str, offset: [i16; 2]) -> ChildWidget {
    ChildWidget {
        widget: Some(tag(path)),
        name: path.into(),
        flags: 0,
        controller: NONE,
        offset,
    }
}

/// The function the recorder fails.
const SET_DIFFICULTY: i16 = 9;
/// The function a closing widget runs.
const RETURN_TO_MAIN_MENU: i16 = 13;
/// The data input "set textbox to build number".
const BUILD_NUMBER: i16 = 8;

fn ui() -> Ui {
    let mut main = widget("main", WidgetKind::Container);
    main.flags =
        Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD | Widget::DPAD_UP_DOWN_TABS_THRU_CHILDREN;
    main.game_data_inputs = vec![BUILD_NUMBER, 0];
    main.children = vec![
        child("play", [0, 0]),
        child("settings", [0, 30]),
        child("swap", [0, 60]),
        child("more", [0, 90]),
    ];

    let mut play = widget("play", WidgetKind::TextBox);
    let mut open = handler(
        EventHandler::A_BUTTON,
        EventHandler::OPEN_WIDGET | EventHandler::RUN_SCENARIO_SCRIPT,
        0,
        Some("level_select"),
    );
    open.script = "intro".into();
    open.sound = Some(TagRef {
        group: Group(*b"snd!"),
        path: "sound\\click".into(),
    });
    play.handlers = vec![open];

    let mut settings = widget("settings", WidgetKind::TextBox);
    settings.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::RUN_FUNCTION | EventHandler::TRY_TO_BRANCH_ON_FAILURE,
        SET_DIFFICULTY,
        None,
    )];
    settings.conditionals = vec![ConditionalWidget {
        widget: Some(tag("error")),
        name: "error".into(),
        flags: CONDITIONAL_IF_FUNCTION_FAILS,
        controller: NONE,
    }];

    let mut swap = widget("swap", WidgetKind::TextBox);
    swap.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::REPLACE_SELF_WITH_WIDGET,
        0,
        Some("swapped"),
    )];
    let mut swapped = widget("swapped", WidgetKind::TextBox);
    swapped.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::CLOSE_CURRENT_WIDGET,
        0,
        None,
    )];

    let mut more = widget("more", WidgetKind::TextBox);
    more.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::OPEN_WIDGET,
        0,
        Some("level_select"),
    )];

    let mut level_select = widget("level_select", WidgetKind::Container);
    level_select.flags = Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD;
    level_select.children = vec![child("levels", [5, 5])];

    let mut levels = widget("levels", WidgetKind::SpinnerList);
    levels.flags = Widget::DPAD_UP_DOWN_TABS_THRU_LIST_ITEMS;
    levels.list.flags = LIST_ITEMS_FROM_STRING_LIST;
    levels.text.string_list = Some(strings_tag("level_names"));

    let mut error = widget("error", WidgetKind::TextBox);
    error.text.string_list = Some(strings_tag("errors"));
    error.replacements = vec![TextReplacement {
        search: "%c".into(),
        function: 1,
    }];

    let mut toast = widget("toast", WidgetKind::Container);
    toast.auto_close_ms = 1000;
    toast.auto_close_fade_ms = 500;

    let mut closing = widget("closing", WidgetKind::Container);
    closing.handlers = vec![handler(
        EventHandler::DELETED,
        EventHandler::RUN_FUNCTION | EventHandler::OPEN_WIDGET,
        RETURN_TO_MAIN_MENU,
        Some("main"),
    )];

    let mut deep = widget("deep", WidgetKind::Container);
    deep.children = vec![child("deep", [1, 1])];

    let mut widgets = vec![
        main,
        play,
        settings,
        swap,
        swapped,
        more,
        level_select,
        levels,
        error,
        toast,
        closing,
        deep,
    ];
    widgets.sort_by(|a, b| a.tag.cmp(&b.tag));
    let mut string_lists = vec![
        StringList {
            tag: strings_tag("level_names"),
            strings: vec!["Pillar of Autumn".into(), "Halo".into(), "".into()],
        },
        StringList {
            tag: strings_tag("errors"),
            strings: vec!["player %c failed".into()],
        },
    ];
    string_lists.sort_by(|a, b| a.tag.cmp(&b.tag));
    Ui {
        widgets,
        string_lists,
        ..Ui::default()
    }
}

/// A host that writes down what the widgets asked of it.
#[derive(Default)]
#[allow(dead_code)]
struct Recorder {
    functions: Vec<i16>,
    scripts: Vec<String>,
    sounds: Vec<String>,
    feedback: Vec<Feedback>,
    inputs: Vec<i16>,
    main_menus: usize,
}

impl Host for Recorder {
    fn run(&mut self, function: i16, _shell: &mut Shell, _ui: &Ui, _widget: Id) -> Outcome {
        self.functions.push(function);
        if function == SET_DIFFICULTY {
            Outcome::Failed
        } else {
            Outcome::Unwired
        }
    }
    fn script(&mut self, name: &str) {
        self.scripts.push(name.into());
    }
    fn sound(&mut self, sound: &TagRef) {
        self.sounds.push(sound.path.clone());
    }
    fn feedback(&mut self, feedback: Feedback) {
        self.feedback.push(feedback);
    }
    fn data_input(&mut self, input: i16, shell: &mut Shell, _ui: &Ui, widget: Id) {
        self.inputs.push(input);
        shell.set_text(widget, "01.01.14.2342");
    }
    fn main_menu(&mut self, _shell: &mut Shell, _ui: &Ui) {
        self.main_menus += 1;
    }
}

fn name(shell: &Shell, ui: &Ui, id: Id) -> String {
    shell
        .definition(ui, id)
        .map_or_else(String::new, |widget| widget.tag.path.clone())
}

/// The names of the widgets with the focus on player `stack`'s screen.
fn focus(shell: &Shell, ui: &Ui, stack: usize) -> Vec<String> {
    shell
        .focus_path(stack)
        .into_iter()
        .map(|id| name(shell, ui, id))
        .collect()
}

fn open_main(ui: &Ui, host: &mut Recorder) -> (Shell, Id) {
    let mut shell = Shell::new();
    let Some(main) = shell.open(ui, host, &tag("main"), 0) else {
        panic!("main did not open");
    };
    (shell, main)
}

#[test]
fn the_keys_press_the_pads_buttons() {
    let button = |index| Input::Button { index, value: 1 };
    assert_eq!(key_input(Key::Up), button(EventHandler::DPAD_UP));
    assert_eq!(key_input(Key::Right), button(EventHandler::DPAD_RIGHT));
    assert_eq!(key_input(Key::Return), button(EventHandler::A_BUTTON));
    assert_eq!(key_input(Key::KeypadEnter), button(EventHandler::A_BUTTON));
    assert_eq!(key_input(Key::Backspace), button(EventHandler::B_BUTTON));
    assert_eq!(key_input(Key::E), button(EventHandler::X_BUTTON));
    assert_eq!(key_input(Key::Tab), button(EventHandler::Y_BUTTON));
    assert_eq!(key_input(Key::F1), button(EventHandler::BACK_BUTTON));
    assert_eq!(key_input(Key::W), Input::LeftStick([0, i16::MAX]));
    assert_eq!(key_input(Key::A), Input::LeftStick([i16::MIN, 0]));

    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_main(&ui, &mut host);
    shell.key(&ui, &mut host, Key::Down);
    assert_eq!(focus(&shell, &ui, 0), ["main", "settings"]);
    shell.key(&ui, &mut host, Key::S);
    assert_eq!(focus(&shell, &ui, 0), ["main", "swap"]);
    shell.key(&ui, &mut host, Key::Up);
    shell.key(&ui, &mut host, Key::Up);
    shell.key(&ui, &mut host, Key::Space);
    assert_eq!(host.scripts, ["intro"]);
    assert_eq!(focus(&shell, &ui, 0), ["level_select", "levels", "levels"]);
    shell.key(&ui, &mut host, Key::Escape);
    assert_eq!(focus(&shell, &ui, 0), ["main", "play"]);
}

#[test]
fn the_item_under_the_pointer_takes_the_focus_once() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, main) = open_main(&ui, &mut host);
    let targets = shell.targets(&ui);
    assert_eq!(targets.len(), 4);
    assert!(targets.iter().all(|target| target.kind == TargetKind::Item));
    let Some(target) = shell.target_at(&ui, [50.0, 50.0]) else {
        panic!("nothing under the pointer");
    };
    assert_eq!(target.bounds, [40, 20, 70, 120]);
    assert_eq!(shell.target_at(&ui, [10.0, 50.0]), None);

    shell.hover(&ui, &mut host, [50.0, 50.0]);
    assert_eq!(focus(&shell, &ui, 0), ["main", "settings"]);
    shell.hover(&ui, &mut host, [60.0, 55.0]);
    assert_eq!(host.feedback, [Feedback::Cursor]);
    // past every item's bounds the focus stays
    shell.hover(&ui, &mut host, [600.0, 400.0]);
    assert_eq!(focus(&shell, &ui, 0), ["main", "settings"]);
    assert_eq!(shell.root(0), Some(main));
}

#[test]
fn a_click_presses_a_on_the_item_and_a_right_click_goes_back() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_main(&ui, &mut host);
    shell.click(&ui, &mut host, [50.0, 115.0], MouseButton::Left);
    assert_eq!(focus(&shell, &ui, 0), ["level_select", "levels", "levels"]);
    assert_eq!(host.scripts, Vec::<String>::new());
    shell.click(&ui, &mut host, [600.0, 400.0], MouseButton::Right);
    assert_eq!(focus(&shell, &ui, 0), ["main", "more"]);
    // a click on nothing does nothing
    shell.click(&ui, &mut host, [600.0, 400.0], MouseButton::Left);
    assert_eq!(focus(&shell, &ui, 0), ["main", "more"]);
}

#[test]
fn a_click_on_a_lists_half_steps_it_that_way() {
    let ui = ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    let Some(screen) = shell.open(&ui, &mut host, &tag("level_select"), 0) else {
        panic!("the level select did not open");
    };
    let Some(&levels) = shell.get(screen).and_then(|screen| screen.children.first()) else {
        panic!("the list was not loaded");
    };
    // the list's generated items are not targets of their own
    let targets = shell.targets(&ui);
    assert_eq!(targets.len(), 1);
    assert_eq!(targets[0].kind, TargetKind::Value);
    assert_eq!(targets[0].bounds, [15, 25, 45, 125]);
    let selected = |shell: &Shell| shell.get(levels).map(|list| list.list.selected);
    shell.click(&ui, &mut host, [50.0, 40.0], MouseButton::Left);
    assert_eq!(selected(&shell), Some(1));
    shell.click(&ui, &mut host, [50.0, 20.0], MouseButton::Left);
    shell.click(&ui, &mut host, [50.0, 20.0], MouseButton::Left);
    assert_eq!(selected(&shell), Some(2));
}

#[test]
fn the_wheel_steps_the_focus_a_few_items_at_most() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_main(&ui, &mut host);
    shell.wheel(&ui, &mut host, -2);
    assert_eq!(focus(&shell, &ui, 0), ["main", "swap"]);
    shell.wheel(&ui, &mut host, 1);
    assert_eq!(focus(&shell, &ui, 0), ["main", "settings"]);
    // ten notches are four presses: settings, play, more, swap, settings
    shell.wheel(&ui, &mut host, 10);
    assert_eq!(focus(&shell, &ui, 0), ["main", "settings"]);
    assert_eq!(host.feedback.len(), 7);
    shell.wheel(&ui, &mut host, 0);
    assert_eq!(host.feedback.len(), 7);
}

/// The function the picker's X runs.
const PICK: i16 = 5;

/// A screen holding a list three items wide and the key's X icon.
fn picker_ui() -> Ui {
    let mut picker = widget("picker", WidgetKind::Container);
    picker.flags = Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD;
    picker.handlers = vec![handler(
        EventHandler::X_BUTTON,
        EventHandler::RUN_FUNCTION,
        PICK,
        None,
    )];
    picker.children = vec![child("wide", [0, 0]), child("ui\\shell\\x_butn", [0, 100])];
    let mut wide = widget("wide", WidgetKind::SpinnerList);
    wide.flags = Widget::DPAD_LEFT_RIGHT_TABS_THRU_LIST_ITEMS;
    wide.children = vec![
        child("cell", [0, 0]),
        child("cell", [100, 0]),
        child("cell", [200, 0]),
    ];
    let mut widgets = vec![
        picker,
        wide,
        widget("cell", WidgetKind::TextBox),
        widget("ui\\shell\\x_butn", WidgetKind::TextBox),
    ];
    widgets.sort_by(|a, b| a.tag.cmp(&b.tag));
    Ui {
        widgets,
        ..Ui::default()
    }
}

#[test]
fn a_lists_items_step_it_to_them_and_a_keys_icon_presses_its_button() {
    let ui = picker_ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    let Some(picker) = shell.open(&ui, &mut host, &tag("picker"), 0) else {
        panic!("the picker did not open");
    };
    let Some(&wide) = shell.get(picker).and_then(|picker| picker.children.first()) else {
        panic!("the list was not loaded");
    };
    shell.set_list_items(wide, 10);
    let cells = shell
        .get(wide)
        .map(|list| list.children.clone())
        .unwrap_or_default();
    let kinds: Vec<TargetKind> = shell
        .targets(&ui)
        .iter()
        .map(|target| target.kind)
        .collect();
    assert_eq!(
        kinds,
        [
            TargetKind::Slot,
            TargetKind::Slot,
            TargetKind::Slot,
            TargetKind::Button(EventHandler::X_BUTTON)
        ]
    );
    let focused = |shell: &Shell| shell.get(wide).and_then(|list| list.focused);

    shell.hover(&ui, &mut host, [250.0, 20.0]);
    assert_eq!(focused(&shell), Some(cells[2]));
    assert_eq!(shell.get(wide).map(|list| list.list.selected), Some(2));
    shell.click(&ui, &mut host, [150.0, 20.0], MouseButton::Left);
    assert_eq!(focused(&shell), Some(cells[1]));
    // the icon is not an item: hovering it moves nothing
    shell.hover(&ui, &mut host, [50.0, 120.0]);
    assert_eq!(focused(&shell), Some(cells[1]));
    shell.click(&ui, &mut host, [50.0, 120.0], MouseButton::Left);
    assert_eq!(host.functions, [PICK]);
    // the wheel steps the list across
    shell.wheel(&ui, &mut host, -1);
    assert_eq!(focused(&shell), Some(cells[2]));
}

/// A 4-way screen: panels that each take the events, and a label.
fn four_way_ui() -> Ui {
    let mut screen = widget("screen", WidgetKind::Container);
    screen.flags = Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD
        | Widget::DONT_FOCUS_A_SPECIFIC_CHILD_WIDGET
        | Widget::PASS_UNHANDLED_EVENTS_TO_ALL_CHILDREN;
    screen.children = vec![
        child("left", [0, 0]),
        child("right", [200, 0]),
        child("label", [0, 100]),
    ];
    let mut left = widget("left", WidgetKind::Container);
    left.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::RUN_FUNCTION,
        41,
        None,
    )];
    let mut right = widget("right", WidgetKind::Container);
    right.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::RUN_FUNCTION,
        42,
        None,
    )];
    let mut widgets = vec![screen, left, right, widget("label", WidgetKind::TextBox)];
    widgets.sort_by(|a, b| a.tag.cmp(&b.tag));
    Ui {
        widgets,
        ..Ui::default()
    }
}

#[test]
fn a_panel_of_a_screen_passing_its_events_to_all_is_clicked() {
    let ui = four_way_ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    if shell.open(&ui, &mut host, &tag("screen"), 0).is_none() {
        panic!("the screen did not open");
    }
    let kinds: Vec<TargetKind> = shell
        .targets(&ui)
        .iter()
        .map(|target| target.kind)
        .collect();
    // the label has no handlers: it is not one
    assert_eq!(kinds, [TargetKind::Item, TargetKind::Item]);
    shell.click(&ui, &mut host, [270.0, 20.0], MouseButton::Left);
    assert!(host.functions.contains(&42), "{:?}", host.functions);
}
