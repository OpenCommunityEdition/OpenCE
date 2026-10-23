//! The widget tree over a small set of widget definitions made here: focus,
//! lists, opening and going back, replacing, closing, timers and text.

use opence_content::{
    ChildWidget, ConditionalWidget, EventHandler, Group, StringList, TagRef, TextReplacement, Ui,
    Widget, WidgetKind, WidgetList, WidgetText,
};
use opence_ui::{
    Event, Feedback, Host, Id, Input, NoHost, Outcome, Shell, CONDITIONAL_IF_FUNCTION_FAILS,
    LIST_ITEMS_FROM_STRING_LIST, NONE,
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

fn press(shell: &mut Shell, ui: &Ui, host: &mut Recorder, index: i16) {
    shell.handle(ui, host, Event::press(0, index));
}

#[test]
fn a_screen_loads_its_children_and_focuses_the_first_that_takes_events() {
    let ui = ui();
    let mut host = Recorder::default();
    let (shell, main) = open_main(&ui, &mut host);
    assert_eq!(shell.root(0), Some(main));
    assert_eq!(shell.len(), 5);
    assert_eq!(focus(&shell, &ui, 0), ["main", "play"]);
    let Some(instance) = shell.get(main) else {
        panic!("main is not loaded");
    };
    assert_eq!(instance.player, 0);
    let children = instance.children.clone();
    assert_eq!(shell.bounds(&ui, children[1]), Some([40, 20, 70, 120]));
    assert!(shell.can_receive_events(&ui, children[1]));
    assert_eq!(shell.root_of(children[3]), main);
    assert_eq!(shell.find(&ui, main, &tag("more")), Some(children[3]));
    assert_eq!(shell.find(&ui, main, &tag("error")), None);
}

#[test]
fn the_dpad_and_the_stick_tab_through_children_and_wrap() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_main(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    assert_eq!(focus(&shell, &ui, 0), ["main", "settings"]);
    shell.handle(
        &ui,
        &mut host,
        Event {
            controller: 0,
            input: Input::LeftStick([0, i16::MIN]),
        },
    );
    assert_eq!(focus(&shell, &ui, 0), ["main", "swap"]);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_UP);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_UP);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_UP);
    assert_eq!(focus(&shell, &ui, 0), ["main", "more"]);
    assert_eq!(host.feedback, [Feedback::Cursor; 5]);
    // another controller's presses do not reach player 0's screen
    shell.handle(&ui, &mut host, Event::press(1, EventHandler::DPAD_DOWN));
    assert_eq!(focus(&shell, &ui, 0), ["main", "more"]);
}

#[test]
fn a_held_dpad_repeats_after_a_quarter_second() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_main(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    let held = Event {
        controller: 0,
        input: Input::Button {
            index: EventHandler::DPAD_DOWN,
            value: 6,
        },
    };
    shell.update(&ui, &mut host, 100);
    assert!(!shell.dpad_repeats(0, EventHandler::DPAD_DOWN));
    shell.handle(&ui, &mut host, held);
    assert_eq!(focus(&shell, &ui, 0), ["main", "settings"]);
    shell.update(&ui, &mut host, 300);
    assert!(shell.dpad_repeats(0, EventHandler::DPAD_DOWN));
    shell.handle(&ui, &mut host, held);
    assert_eq!(focus(&shell, &ui, 0), ["main", "swap"]);
    // the repeat starts the quarter second again
    shell.update(&ui, &mut host, 400);
    shell.handle(&ui, &mut host, held);
    assert_eq!(focus(&shell, &ui, 0), ["main", "swap"]);
    assert!(!shell.dpad_repeats(NONE, EventHandler::DPAD_DOWN));
    assert!(!shell.dpad_repeats(0, EventHandler::A_BUTTON));
}

#[test]
fn opening_a_screen_remembers_where_it_came_from_and_b_goes_back() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, main) = open_main(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_UP);
    press(&mut shell, &ui, &mut host, EventHandler::A_BUTTON);
    assert!(!shell.alive(main));
    assert_eq!(focus(&shell, &ui, 0)[..2], ["level_select", "levels"]);
    let history = shell.history(0);
    assert_eq!(history.len(), 1);
    assert_eq!(history[0].widget, tag("main"));
    assert_eq!(history[0].focused_parent, Some(tag("main")));
    assert_eq!(history[0].focused_index, 3);
    assert_eq!(history[0].player, 0);

    press(&mut shell, &ui, &mut host, EventHandler::B_BUTTON);
    assert!(shell.history(0).is_empty());
    assert_eq!(focus(&shell, &ui, 0), ["main", "more"]);
    assert_eq!(
        host.feedback,
        [Feedback::Cursor, Feedback::Forward, Feedback::Back]
    );
    // a screen with nowhere to go back to closes on B
    press(&mut shell, &ui, &mut host, EventHandler::B_BUTTON);
    assert!(shell.is_empty());
    assert_eq!(shell.roots().count(), 0);
}

#[test]
fn a_handler_runs_its_script_and_plays_its_sound() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_main(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::A_BUTTON);
    assert_eq!(host.scripts, ["intro"]);
    assert_eq!(host.sounds, ["sound\\click"]);
    assert_eq!(Feedback::Forward.sound(), "sound\\sfx\\ui\\forward");
}

#[test]
fn a_list_made_from_its_string_list_moves_through_its_items_and_wraps() {
    let ui = ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    let Some(screen) = shell.open(&ui, &mut host, &tag("level_select"), 0) else {
        panic!("the level select did not open");
    };
    let Some(&levels) = shell.get(screen).and_then(|screen| screen.children.first()) else {
        panic!("the list was not loaded");
    };
    let Some(list) = shell.get(levels) else {
        panic!("the list is gone");
    };
    assert_eq!(list.children.len(), 3);
    assert_eq!(list.list.generated, 3);
    let items = list.children.clone();
    assert_eq!(shell.text(&ui, items[1]).as_deref(), Some("Halo"));
    assert_eq!(shell.text(&ui, items[2]), None);
    assert_eq!(shell.text(&ui, levels).as_deref(), Some("Pillar of Autumn"));
    assert!(shell.get(items[0]).is_some_and(|item| item.generated));
    // (a list's items are offset with it)
    assert_eq!(shell.bounds(&ui, items[0]), Some([15, 25, 45, 125]));

    let selected = |shell: &Shell| shell.get(levels).map(|list| list.list.selected);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    assert_eq!(selected(&shell), Some(2));
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    assert_eq!(selected(&shell), Some(0));
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_UP);
    assert_eq!(selected(&shell), Some(2));
    assert_eq!(
        shell.get(levels).map(|list| list.list.last_direction),
        Some(-15)
    );
}

#[test]
fn a_list_given_items_by_the_game_moves_through_them() {
    let ui = ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    let Some(screen) = shell.open(&ui, &mut host, &tag("level_select"), 0) else {
        panic!("the level select did not open");
    };
    let Some(&levels) = shell.get(screen).and_then(|screen| screen.children.first()) else {
        panic!("the list was not loaded");
    };
    shell.set_list_items(levels, 5);
    for _ in 0..7 {
        shell.move_item(&ui, levels, true);
    }
    assert_eq!(shell.get(levels).map(|list| list.list.selected), Some(2));
    shell.set_list_items(levels, 2);
    assert_eq!(shell.get(levels).map(|list| list.list.selected), Some(0));
}

#[test]
fn a_function_that_fails_branches_to_its_conditional_widget() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_main(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    press(&mut shell, &ui, &mut host, EventHandler::A_BUTTON);
    assert_eq!(host.functions, [SET_DIFFICULTY]);
    let Some(error) = shell.root(0) else {
        panic!("no screen is open");
    };
    assert_eq!(name(&shell, &ui, error), "error");
    assert_eq!(shell.text(&ui, error).as_deref(), Some("player 1 failed"));
    assert_eq!(shell.history(0).len(), 1);
}

#[test]
fn a_widget_replaces_itself_in_its_place_and_closes() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, main) = open_main(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_DOWN);
    press(&mut shell, &ui, &mut host, EventHandler::A_BUTTON);
    let Some(children) = shell.get(main).map(|main| main.children.clone()) else {
        panic!("main closed");
    };
    let names: Vec<String> = children.iter().map(|&id| name(&shell, &ui, id)).collect();
    assert_eq!(names, ["play", "settings", "swapped", "more"]);
    assert_eq!(focus(&shell, &ui, 0), ["main", "swapped"]);
    assert_eq!(shell.bounds(&ui, children[2]), Some([70, 20, 100, 120]));
    // and the new one closes itself
    press(&mut shell, &ui, &mut host, EventHandler::A_BUTTON);
    assert_eq!(shell.get(main).map(|main| main.children.len()), Some(3));
    assert_eq!(focus(&shell, &ui, 0), ["main"]);
}

#[test]
fn a_screen_closes_itself_after_its_time_fading_out() {
    let ui = ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    let Some(toast) = shell.open(&ui, &mut host, &tag("toast"), 1) else {
        panic!("the toast did not open");
    };
    shell.update(&ui, &mut host, 900);
    assert_eq!(shell.get(toast).map(|toast| toast.alpha), Some(1.0));
    shell.update(&ui, &mut host, 1250);
    assert_eq!(shell.get(toast).map(|toast| toast.alpha), Some(0.5));
    shell.update(&ui, &mut host, 1500);
    assert!(!shell.alive(toast));
    assert_eq!(shell.root(1), None);
    assert_eq!(shell.now_ms(), 1500);
}

#[test]
fn a_widget_closing_runs_its_handlers_and_opens_what_they_open() {
    let ui = ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    let Some(closing) = shell.open(&ui, &mut host, &tag("closing"), 2) else {
        panic!("the widget did not open");
    };
    shell.delete(&ui, &mut host, closing);
    assert_eq!(host.functions, [RETURN_TO_MAIN_MENU]);
    let Some(main) = shell.root(2) else {
        panic!("main did not open");
    };
    assert_eq!(name(&shell, &ui, main), "main");
    assert_eq!(shell.history(2).len(), 1);
    assert_eq!(shell.history(2)[0].widget, tag("closing"));
    shell.close_all(&ui, &mut host);
    assert!(shell.is_empty());
    assert!(shell.history(2).is_empty());
}

#[test]
fn each_frame_gives_the_widgets_their_data() {
    let ui = ui();
    let mut host = Recorder::default();
    let (mut shell, main) = open_main(&ui, &mut host);
    assert_eq!(shell.text(&ui, main), None);
    shell.update(&ui, &mut host, 16);
    assert_eq!(host.inputs, [BUILD_NUMBER]);
    assert_eq!(shell.text(&ui, main).as_deref(), Some("01.01.14.2342"));
}

#[test]
fn a_widget_that_holds_itself_stops_at_the_deepest_tree() {
    let ui = ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    let Some(_) = shell.open(&ui, &mut host, &tag("deep"), NONE) else {
        panic!("the widget did not open");
    };
    assert_eq!(shell.len(), opence_ui::MAXIMUM_DEPTH + 1);
    assert_eq!(
        shell
            .root(0)
            .and_then(|root| shell.get(root))
            .map(|root| root.player),
        Some(NONE)
    );
}

#[test]
fn a_missing_screen_opens_nothing_and_the_names_are_the_games() {
    let ui = ui();
    let mut host = Recorder::default();
    let mut shell = Shell::new();
    assert_eq!(shell.open(&ui, &mut host, &tag("nowhere"), 0), None);
    assert_eq!(opence_ui::function_name(10), Some("start new game"));
    assert_eq!(opence_ui::function_name(102), None);
    assert_eq!(opence_ui::data_input_name(-1), None);
    assert_eq!(opence_ui::replacement_value(1, 3), "4");
    assert_eq!(opence_ui::replacement_value(1, 7), "<invalid>");
    assert_eq!(opence_ui::replacement_value(0, 0), "");
    assert_eq!(opence_ui::replacement_value(5, 0), "<invalid>");
}

/// A screen any controller drives, which passes each event to all its
/// children: a panel of buttons tabbed left and right, and a widget whose
/// handlers close things; a second player's screen; and a column list
/// with a description beside it.
fn hub_ui() -> Ui {
    let mut hub = widget("hub", WidgetKind::Container);
    hub.controller = 4;
    hub.flags =
        Widget::PASS_UNHANDLED_EVENTS_TO_ALL_CHILDREN | Widget::RETURN_TO_MAIN_MENU_IF_NO_HISTORY;
    let mut panel_reference = child("panel", [0, 0]);
    panel_reference.flags = ChildWidget::USE_CUSTOM_CONTROLLER_INDEX;
    panel_reference.controller = 0;
    hub.children = vec![panel_reference, child("closer", [0, 200])];

    let mut panel = widget("panel", WidgetKind::Container);
    panel.flags =
        Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD | Widget::DPAD_LEFT_RIGHT_TABS_THRU_CHILDREN;
    panel.children = vec![
        child("left", [0, 0]),
        child("label", [100, 0]),
        child("middle", [200, 0]),
        child("right", [300, 0]),
    ];
    let mut left = widget("left", WidgetKind::TextBox);
    left.handlers = vec![handler(
        EventHandler::CREATED,
        EventHandler::RUN_FUNCTION,
        0,
        None,
    )];
    let label = widget("label", WidgetKind::TextBox);
    let mut middle = widget("middle", WidgetKind::TextBox);
    middle.handlers = vec![handler(EventHandler::A_BUTTON, 0, 0, None)];
    let mut right = widget("right", WidgetKind::TextBox);
    right.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::RUN_FUNCTION | EventHandler::TRY_TO_BRANCH_ON_FAILURE,
        200,
        None,
    )];

    let mut closer = widget("closer", WidgetKind::TextBox);
    closer.handlers = vec![
        handler(
            EventHandler::X_BUTTON,
            EventHandler::CLOSE_OTHER_WIDGET,
            0,
            Some("hub"),
        ),
        handler(
            EventHandler::Y_BUTTON,
            EventHandler::GO_BACK_TO_PREVIOUS_WIDGET,
            0,
            None,
        ),
        handler(
            EventHandler::LEFT_TRIGGER,
            EventHandler::CLOSE_OTHER_WIDGET,
            0,
            Some("other"),
        ),
        handler(
            EventHandler::WHITE_BUTTON,
            EventHandler::GIVE_FOCUS_TO_WIDGET
                | EventHandler::RELOAD_OTHER_WIDGET
                | EventHandler::OPEN_WIDGET
                | EventHandler::REPLACE_SELF_WITH_WIDGET,
            0,
            Some("nowhere"),
        ),
        handler(
            EventHandler::RIGHT_ANALOG_STICK_UP,
            EventHandler::CLOSE_ALL_WIDGETS,
            0,
            None,
        ),
        handler(
            EventHandler::LEFT_ANALOG_STICK_LEFT,
            EventHandler::GIVE_FOCUS_TO_WIDGET,
            0,
            Some("middle"),
        ),
        handler(
            EventHandler::BLACK_BUTTON,
            EventHandler::OPEN_WIDGET,
            0,
            Some("other"),
        ),
    ];

    let mut other = widget("other", WidgetKind::Container);
    other.controller = 1;
    other.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::REPLACE_SELF_WITH_WIDGET,
        0,
        Some("label"),
    )];

    let mut browser = widget("browser", WidgetKind::ColumnList);
    browser.flags = Widget::DPAD_UP_DOWN_TABS_THRU_LIST_ITEMS;
    browser.extended_description = Some(tag("details"));
    browser.children = vec![
        child("row", [0, 0]),
        child("row", [0, 20]),
        child("row", [0, 40]),
    ];
    let mut row = widget("row", WidgetKind::TextBox);
    row.handlers = vec![handler(
        EventHandler::A_BUTTON,
        EventHandler::RUN_FUNCTION,
        1,
        None,
    )];
    let details = widget("details", WidgetKind::TextBox);

    let mut spinner = widget("spinner", WidgetKind::SpinnerList);
    spinner.children = vec![
        child("row", [0, 0]),
        child("row", [40, 0]),
        child("row", [80, 0]),
    ];

    let mut widgets = vec![
        hub, panel, left, label, middle, right, closer, other, browser, row, details, spinner,
    ];
    widgets.sort_by(|a, b| a.tag.cmp(&b.tag));
    Ui {
        widgets,
        ..Ui::default()
    }
}

fn open_hub(ui: &Ui, host: &mut Recorder) -> (Shell, Id) {
    let mut shell = Shell::new();
    let Some(hub) = shell.open(ui, host, &tag("hub"), 0) else {
        panic!("the hub did not open");
    };
    (shell, hub)
}

fn stick(controller: i16, input: Input) -> Event {
    Event { controller, input }
}

#[test]
fn a_screen_for_any_controller_passes_events_to_all_its_children() {
    let ui = hub_ui();
    let mut host = Recorder::default();
    let (mut shell, hub) = open_hub(&ui, &mut host);
    assert_eq!(focus(&shell, &ui, 0), ["hub", "closer"]);
    let Some(panel) = shell.find(&ui, hub, &tag("panel")) else {
        panic!("no panel");
    };
    let path = |shell: &Shell| {
        shell
            .get(panel)
            .and_then(|panel| panel.focused)
            .map(|id| name(shell, &ui, id))
    };
    assert_eq!(path(&shell).as_deref(), Some("left"));
    // (controller 2 drives a screen that takes any, but not the panel
    // given controller 0)
    shell.handle(&ui, &mut host, Event::press(2, EventHandler::DPAD_RIGHT));
    assert_eq!(path(&shell).as_deref(), Some("left"));
    shell.handle(&ui, &mut host, Event::press(0, EventHandler::DPAD_RIGHT));
    assert_eq!(path(&shell).as_deref(), Some("middle"));
    shell.handle(&ui, &mut host, stick(0, Input::LeftStick([i16::MAX, 0])));
    assert_eq!(path(&shell).as_deref(), Some("right"));
    shell.handle(&ui, &mut host, stick(0, Input::LeftStick([i16::MIN, 0])));
    assert_eq!(path(&shell).as_deref(), Some("middle"));
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_LEFT);
    press(&mut shell, &ui, &mut host, EventHandler::DPAD_LEFT);
    assert_eq!(path(&shell).as_deref(), Some("right"));
    // a function past the last fails, with nothing to branch to
    press(&mut shell, &ui, &mut host, EventHandler::A_BUTTON);
    assert!(shell.alive(hub));
    assert!(host.functions.is_empty());
}

#[test]
fn giving_the_focus_moves_it_down_the_tree_and_past_a_disabled_widget() {
    let ui = hub_ui();
    let mut host = Recorder::default();
    let (mut shell, hub) = open_hub(&ui, &mut host);
    let Some(middle) = shell.find(&ui, hub, &tag("middle")) else {
        panic!("no middle");
    };
    shell.set_disabled(middle, true);
    assert!(!shell.can_receive_events(&ui, middle));
    shell.handle(&ui, &mut host, stick(0, Input::LeftStick([i16::MIN, 0])));
    assert_eq!(focus(&shell, &ui, 0), ["hub", "panel", "right"]);
    assert_eq!(host.feedback.last(), Some(&Feedback::Cursor));
    // a handler whose widgets are missing does nothing
    press(&mut shell, &ui, &mut host, EventHandler::WHITE_BUTTON);
    assert!(shell.alive(hub));
    assert_eq!(shell.len(), 7);
}

#[test]
fn closing_the_screen_with_no_history_opens_the_main_menu() {
    let ui = hub_ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_hub(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::X_BUTTON);
    assert!(shell.is_empty());
    assert_eq!(host.main_menus, 1);

    let (mut shell, _) = open_hub(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::Y_BUTTON);
    assert!(shell.is_empty());
    assert_eq!(host.feedback.last(), Some(&Feedback::Back));
}

#[test]
fn a_handler_closes_another_players_screen_or_every_screen() {
    let ui = hub_ui();
    let mut host = Recorder::default();
    let (mut shell, hub) = open_hub(&ui, &mut host);
    // the left trigger closes the other screen, and with none open does
    // nothing
    press(&mut shell, &ui, &mut host, EventHandler::LEFT_TRIGGER);
    assert!(shell.alive(hub));
    let Some(other) = shell.open(&ui, &mut host, &tag("other"), 1) else {
        panic!("the other screen did not open");
    };
    assert_eq!(shell.root(1), Some(other));
    press(&mut shell, &ui, &mut host, EventHandler::LEFT_TRIGGER);
    assert_eq!(shell.root(1), None);
    assert!(shell.alive(hub));

    shell.open(&ui, &mut host, &tag("other"), 1);
    shell.handle(&ui, &mut host, stick(0, Input::RightStick([0, i16::MAX])));
    assert!(shell.is_empty());
}

#[test]
fn a_screen_opened_for_its_own_player_replaces_itself_at_the_root() {
    let ui = hub_ui();
    let mut host = Recorder::default();
    let (mut shell, _) = open_hub(&ui, &mut host);
    press(&mut shell, &ui, &mut host, EventHandler::BLACK_BUTTON);
    let Some(other) = shell.root(1) else {
        panic!("the other screen did not open for its player");
    };
    assert!(shell.root(0).is_some());
    assert_eq!(shell.history(1).len(), 1);
    shell.handle(&ui, &mut host, Event::press(1, EventHandler::A_BUTTON));
    assert!(!shell.alive(other));
    let Some(replaced) = shell.root(1) else {
        panic!("the replacement is not the screen");
    };
    assert_eq!(name(&shell, &ui, replaced), "label");
}

#[test]
fn a_column_list_moves_its_focus_and_keeps_its_description_beside_it() {
    let ui = hub_ui();
    let mut host = NoHost;
    let mut shell = Shell::new();
    let Some(browser) = shell.open(&ui, &mut host, &tag("browser"), 0) else {
        panic!("the browser did not open");
    };
    let Some(instance) = shell.get(browser) else {
        panic!("the browser is gone");
    };
    let rows = instance.children.clone();
    let Some(details) = instance.extended_description else {
        panic!("no description");
    };
    assert_eq!(name(&shell, &ui, details), "details");
    assert_eq!(
        shell.get(browser).and_then(|list| list.focused),
        Some(rows[0])
    );
    shell.handle(&ui, &mut host, Event::press(0, EventHandler::DPAD_DOWN));
    assert_eq!(
        shell.get(browser).and_then(|list| list.focused),
        Some(rows[1])
    );
    shell.handle(&ui, &mut host, Event::press(0, EventHandler::DPAD_UP));
    shell.handle(&ui, &mut host, Event::press(0, EventHandler::DPAD_UP));
    assert_eq!(
        shell.get(browser).and_then(|list| list.focused),
        Some(rows[2])
    );
    // a row's next item function runs on the row and does nothing
    shell.handle(&ui, &mut host, Event::press(0, EventHandler::A_BUTTON));
    shell.set_list_items(browser, 3);
    shell.move_item(&ui, browser, false);
    assert_eq!(
        shell.get(browser).and_then(|list| list.focused),
        Some(rows[1])
    );
    shell.move_item(&ui, browser, true);
    shell.move_item(&ui, browser, true);
    assert_eq!(
        shell.get(browser).and_then(|list| list.focused),
        Some(rows[0])
    );
    shell.update(&ui, &mut host, 10);
    shell.delete(&ui, &mut host, browser);
    assert!(shell.is_empty());
}

#[test]
fn a_spinner_three_wide_moves_its_focus_with_its_items() {
    let ui = hub_ui();
    let mut host = NoHost;
    let mut shell = Shell::new();
    let Some(spinner) = shell.open(&ui, &mut host, &tag("spinner"), 0) else {
        panic!("the spinner did not open");
    };
    let rows = shell
        .get(spinner)
        .map(|list| list.children.clone())
        .unwrap_or_default();
    shell.set_list_items(spinner, 10);
    shell.move_item(&ui, spinner, true);
    shell.move_item(&ui, spinner, true);
    assert_eq!(
        shell.get(spinner).and_then(|list| list.focused),
        Some(rows[2])
    );
    assert_eq!(shell.get(spinner).map(|list| list.list.selected), Some(2));
    shell.move_item(&ui, spinner, true);
    assert_eq!(
        shell.get(spinner).and_then(|list| list.focused),
        Some(rows[2])
    );
    shell.move_item(&ui, spinner, false);
    assert_eq!(
        shell.get(spinner).and_then(|list| list.focused),
        Some(rows[1])
    );
}

#[test]
fn the_shells_sounds_are_the_games() {
    let sounds = [
        Feedback::Cursor,
        Feedback::Forward,
        Feedback::Back,
        Feedback::FlagFailure,
    ]
    .map(Feedback::sound);
    assert_eq!(sounds[0], "sound\\sfx\\ui\\cursor");
    assert_eq!(sounds[3], "sound\\sfx\\ui\\flag_failure");
    assert_eq!(opence_ui::replacement_value(1, 1), "2");
    assert_eq!(opence_ui::replacement_value(1, 2), "3");
}
