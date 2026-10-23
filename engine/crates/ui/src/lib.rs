//! The shell's widgets (conductor/tracks/halo_ui_20261021): the tree of
//! widgets loaded from a map's widget definitions (`ui.map`'s, and the
//! pause menu every map carries), which of them has the focus, what each
//! does with an event, and the text each shows.
//!
//! It does what the game's widget code does (source/interface/ui_widget.c):
//! one tree of widgets for each local player, a history of the screens
//! each came from, focus moved by the d-pad and the stick through children
//! and list items, and the functions a handler runs. The functions that
//! reach the game (start a level, set the difficulty) are the [`Host`]'s;
//! the ones the engine has no use for do nothing ([`Outcome::Unwired`]).
//!
//! The widgets live in an arena and are named by [`Id`]; a widget closed is
//! gone, and its id names nothing again.

pub mod chant;
mod draw;
mod names;
mod pointer;

use std::collections::BTreeMap;

use opence_content::{ChildWidget, EventHandler, TagRef, Ui, Widget, WidgetKind};

pub use draw::{
    frame_count, layout_text, sequence_image, Bitmaps, DrawItem, GlyphQuad, Layout, Quad,
    FLAT_FILL_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH, TEXT_FLASHING, UI_WHITE,
};
pub use names::{
    data_input_name, function_name, DATA_INPUT_NAMES, FUNCTION_NAMES, LIST_GOTO_NEXT_ITEM,
    LIST_GOTO_PREVIOUS_ITEM, REPLACEMENT_NAMES,
};
pub use pointer::{
    key_button, key_input, Key, MouseButton, Target, TargetKind, MAXIMUM_WHEEL_STEPS,
};

/// No player, no controller, no index.
pub const NONE: i16 = -1;
/// How many players play on one machine, each with a tree of widgets.
pub const LOCAL_PLAYERS: usize = 4;
/// After how long a held d-pad direction counts as pressed again.
pub const DPAD_REPEAT_MS: u64 = 250;
/// The most widgets loaded at once (a map's widgets are untrusted).
pub const MAXIMUM_WIDGETS: usize = 4096;
/// How deep a tree of widgets may go before children are no longer loaded
/// (a widget that is its own child would otherwise never end).
pub const MAXIMUM_DEPTH: usize = 16;
/// The most items a list makes from its string list.
pub const MAXIMUM_LIST_ITEMS: usize = 256;

/// The list flag "items generated in code".
pub const LIST_ITEMS_FROM_CODE: u32 = 1 << 0;
/// The list flag "items generated from the string list tag".
pub const LIST_ITEMS_FROM_STRING_LIST: u32 = 1 << 1;
/// The conditional widget flag "load if the event handler's function fails".
pub const CONDITIONAL_IF_FUNCTION_FAILS: u32 = 1 << 0;
/// The direction a list last moved in, as the game keeps it.
const LIST_TAB_DIRECTION: i16 = 15;

/// The screen a screen was opened from, the widget whose child had the
/// focus there, and that child's index.
type Invoking = (TagRef, Option<TagRef>, i16);

/// A widget loaded: an index into the shell's arena.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Id(usize);

/// The sound the shell plays for what a press did.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Feedback {
    /// The focus moved.
    Cursor,
    /// A screen opened.
    Forward,
    /// A screen closed, back to the one before.
    Back,
    /// A press did nothing it could.
    FlagFailure,
}

impl Feedback {
    /// The sound's tag path.
    pub fn sound(self) -> &'static str {
        match self {
            Self::Cursor => "sound\\sfx\\ui\\cursor",
            Self::Forward => "sound\\sfx\\ui\\forward",
            Self::Back => "sound\\sfx\\ui\\back",
            Self::FlagFailure => "sound\\sfx\\ui\\flag_failure",
        }
    }
}

/// What a function a handler ran did.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Outcome {
    /// It did what it does.
    Done,
    /// It could not (the handler then branches to its conditional widgets).
    Failed,
    /// The engine does not do it: it counts as done and does nothing.
    Unwired,
}

/// An input the widgets take.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Input {
    /// Nothing: the widgets' timers and fades still run.
    Tick,
    /// A button ([`EventHandler::A_BUTTON`] to
    /// [`EventHandler::RIGHT_THUMB`]): 1 when pressed, more while it is
    /// held.
    Button {
        /// The button.
        index: i16,
        /// 1 pressed; more for each frame it has been held.
        value: u16,
    },
    /// The left stick, across and up, each from `i16::MIN` to `i16::MAX`.
    LeftStick([i16; 2]),
    /// The right stick.
    RightStick([i16; 2]),
}

/// An input from a controller.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Event {
    /// The controller (0 to 3).
    pub controller: i16,
    /// What it did.
    pub input: Input,
}

impl Event {
    /// A button pressed on `controller`.
    pub fn press(controller: i16, index: i16) -> Self {
        Self {
            controller,
            input: Input::Button { index, value: 1 },
        }
    }
}

/// A list widget's place among its items.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct ListState {
    /// The item selected.
    pub selected: i16,
    /// How many items the game gave it ([`Shell::set_list_items`]).
    pub count: i16,
    /// Whether its items come from the game rather than its children.
    pub from_game: bool,
    /// How many items it made from its string list.
    pub generated: i16,
    /// The way it last moved: 15 down the list, −15 up it.
    pub last_direction: i16,
}

/// A widget loaded.
#[derive(Clone, Debug, PartialEq)]
pub struct Instance {
    /// Its definition, an index into [`Ui::widgets`].
    pub definition: usize,
    /// What it is.
    pub kind: WidgetKind,
    /// Its parent (none for a screen).
    pub parent: Option<Id>,
    /// Its children, in order.
    pub children: Vec<Id>,
    /// The child that has its focus.
    pub focused: Option<Id>,
    /// Its offset from its definition's bounds, across and down.
    pub offset: [i16; 2],
    /// The player it listens to, or [`NONE`] for any.
    pub player: i16,
    /// Whether it takes no input and the focus passes over it.
    pub disabled: bool,
    /// Whether it is drawn.
    pub visible: bool,
    /// When it was loaded, in the shell's milliseconds.
    pub created_ms: u64,
    /// After how long it closes (0: never).
    pub close_ms: i32,
    /// How long it fades out over before it does.
    pub fade_ms: i32,
    /// Its opacity, from 1 down as it fades.
    pub alpha: f32,
    /// The text the game gave it, for a text box without a string list.
    pub text: Option<String>,
    /// The string of its definition's list it shows, or [`NONE`] for the
    /// definition's.
    pub string_index: i16,
    /// Its list's place.
    pub list: ListState,
    /// Whether it is an item its parent made from its string list.
    pub generated: bool,
    /// The widget that describes its focused item, loaded beside it.
    pub extended_description: Option<Id>,
}

/// A screen a player came from, to go back to.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct HistoryEntry {
    /// The screen.
    pub widget: TagRef,
    /// The widget whose child had the focus.
    pub focused_parent: Option<TagRef>,
    /// That child's index, or [`NONE`].
    pub focused_index: i16,
    /// The player it was open for.
    pub player: i16,
}

/// What the game does for the widgets: the functions their handlers run,
/// their scripts, their sounds and their data. Each does nothing unless
/// the game says otherwise.
pub trait Host {
    /// Runs the function `function` (an index into [`FUNCTION_NAMES`]) for
    /// `widget`. The list functions and the null ones never reach it.
    fn run(&mut self, _function: i16, _shell: &mut Shell, _ui: &Ui, _widget: Id) -> Outcome {
        Outcome::Unwired
    }
    /// Runs the scenario's script `name`.
    fn script(&mut self, _name: &str) {}
    /// Plays the sound `sound`.
    fn sound(&mut self, _sound: &TagRef) {}
    /// Plays the shell's sound for `feedback`.
    fn feedback(&mut self, _feedback: Feedback) {}
    /// Gives `widget` its data (an index into [`DATA_INPUT_NAMES`]), each
    /// frame before it is drawn.
    fn data_input(&mut self, _input: i16, _shell: &mut Shell, _ui: &Ui, _widget: Id) {}
    /// Opens the main menu, when a screen that asks for it closed with
    /// nothing to go back to.
    fn main_menu(&mut self, _shell: &mut Shell, _ui: &Ui) {}
}

/// A host that does nothing.
#[derive(Clone, Copy, Debug, Default)]
pub struct NoHost;

impl Host for NoHost {}

/// The widgets loaded, each player's screen and history.
#[derive(Clone, Debug, Default)]
pub struct Shell {
    widgets: BTreeMap<Id, Instance>,
    next: usize,
    roots: [Option<Id>; LOCAL_PLAYERS],
    history: [Vec<HistoryEntry>; LOCAL_PLAYERS],
    now_ms: u64,
    dpad_ms: [[u64; 4]; LOCAL_PLAYERS],
}

/// The stack a player's screens are on.
fn stack_of(player: i16) -> usize {
    usize::try_from(player)
        .ok()
        .filter(|&stack| stack < LOCAL_PLAYERS)
        .unwrap_or(0)
}

/// Whether `kind` is a list.
fn is_list(kind: WidgetKind) -> bool {
    matches!(kind, WidgetKind::SpinnerList | WidgetKind::ColumnList)
}

/// The definition index of `tag`.
fn definition_index(ui: &Ui, tag: &TagRef) -> Option<usize> {
    ui.widgets
        .binary_search_by(|widget| widget.tag.cmp(tag))
        .ok()
}

/// Whether `input` is the event a handler for `event` takes.
fn matches_event(event: i16, input: &Input) -> bool {
    match *input {
        Input::Button { index, value } => {
            (EventHandler::A_BUTTON..=EventHandler::RIGHT_THUMB).contains(&event)
                && index == event
                && value == 1
        }
        Input::LeftStick(stick) => stick_matches(event - EventHandler::LEFT_ANALOG_STICK_UP, stick),
        Input::RightStick(stick) => {
            stick_matches(event - EventHandler::RIGHT_ANALOG_STICK_UP, stick)
        }
        Input::Tick => false,
    }
}

/// Whether the stick at `stick` is pushed all the way the way `direction`
/// (0 up, 1 down, 2 left, 3 right) names.
fn stick_matches(direction: i16, stick: [i16; 2]) -> bool {
    match direction {
        0 => stick[1] == i16::MAX,
        1 => stick[1] == i16::MIN,
        2 => stick[0] == i16::MIN,
        3 => stick[0] == i16::MAX,
        _ => false,
    }
}

/// Which way `input` moves the focus: −1 back, 1 on, along the vertical
/// or the horizontal.
fn step(input: &Input, vertical: bool) -> Option<i8> {
    match (*input, vertical) {
        (Input::Button { index, value: 1 }, true) => match index {
            EventHandler::DPAD_UP => Some(-1),
            EventHandler::DPAD_DOWN => Some(1),
            _ => None,
        },
        (Input::Button { index, value: 1 }, false) => match index {
            EventHandler::DPAD_LEFT => Some(-1),
            EventHandler::DPAD_RIGHT => Some(1),
            _ => None,
        },
        (Input::LeftStick([_, y]), true) => match y {
            i16::MIN => Some(1),
            i16::MAX => Some(-1),
            _ => None,
        },
        (Input::LeftStick([x, _]), false) => match x {
            i16::MIN => Some(-1),
            i16::MAX => Some(1),
            _ => None,
        },
        _ => None,
    }
}

/// The value of the text replacement function `function` for a widget
/// of `player`.
pub fn replacement_value(function: i16, player: i16) -> &'static str {
    match function {
        0 => "",
        1 => match player {
            NONE | 0 => "1",
            1 => "2",
            2 => "3",
            3 => "4",
            _ => "<invalid>",
        },
        _ => "<invalid>",
    }
}

impl Shell {
    /// No widgets.
    pub fn new() -> Self {
        Self::default()
    }

    /// The widget `id`, if it is loaded.
    pub fn get(&self, id: Id) -> Option<&Instance> {
        self.widgets.get(&id)
    }

    /// The widget `id`, to change.
    pub fn get_mut(&mut self, id: Id) -> Option<&mut Instance> {
        self.widgets.get_mut(&id)
    }

    /// Whether `id` is loaded.
    pub fn alive(&self, id: Id) -> bool {
        self.widgets.contains_key(&id)
    }

    /// How many widgets are loaded.
    pub fn len(&self) -> usize {
        self.widgets.len()
    }

    /// Whether none are.
    pub fn is_empty(&self) -> bool {
        self.widgets.is_empty()
    }

    /// The definition of `id`.
    pub fn definition<'a>(&self, ui: &'a Ui, id: Id) -> Option<&'a Widget> {
        self.get(id)
            .and_then(|instance| ui.widgets.get(instance.definition))
    }

    /// The screen open for player `stack`'s widgets.
    pub fn root(&self, stack: usize) -> Option<Id> {
        self.roots.get(stack).copied().flatten()
    }

    /// The screens open, one for each player that has one.
    pub fn roots(&self) -> impl Iterator<Item = Id> + '_ {
        self.roots.iter().filter_map(|root| *root)
    }

    /// The screens player `stack` can go back to, the last last.
    pub fn history(&self, stack: usize) -> &[HistoryEntry] {
        self.history.get(stack).map_or(&[], Vec::as_slice)
    }

    /// The shell's clock, in milliseconds.
    pub fn now_ms(&self) -> u64 {
        self.now_ms
    }

    /// The screen `id` is on.
    pub fn root_of(&self, id: Id) -> Id {
        let mut node = id;
        while let Some(parent) = self.get(node).and_then(|instance| instance.parent) {
            node = parent;
        }
        node
    }

    /// The widgets with the focus from player `stack`'s screen down, the
    /// screen first.
    pub fn focus_path(&self, stack: usize) -> Vec<Id> {
        let mut path = Vec::new();
        let mut node = self.root(stack);
        while let Some(id) = node {
            if path.contains(&id) {
                break;
            }
            path.push(id);
            node = self.get(id).and_then(|instance| instance.focused);
        }
        path
    }

    /// The widget below `from` (or `from`) whose definition is `tag`, the
    /// first in order.
    pub fn find(&self, ui: &Ui, from: Id, tag: &TagRef) -> Option<Id> {
        let instance = self.get(from)?;
        if ui
            .widgets
            .get(instance.definition)
            .is_some_and(|widget| &widget.tag == tag)
        {
            return Some(from);
        }
        instance
            .children
            .iter()
            .find_map(|&child| self.find(ui, child, tag))
    }

    /// The bounds of `id` on the 640×480 screen: top, left, bottom, right.
    pub fn bounds(&self, ui: &Ui, id: Id) -> Option<[i16; 4]> {
        let instance = self.get(id)?;
        let [top, left, bottom, right] = ui.widgets.get(instance.definition)?.bounds;
        let [across, down] = instance.offset;
        Some([
            top.saturating_add(down),
            left.saturating_add(across),
            bottom.saturating_add(down),
            right.saturating_add(across),
        ])
    }

    /// Gives `id` the text `text` (a text box without a string list shows
    /// it).
    pub fn set_text(&mut self, id: Id, text: impl Into<String>) {
        if let Some(instance) = self.get_mut(id) {
            instance.text = Some(text.into());
        }
    }

    /// Gives the list `id` `count` items from the game, which its next
    /// and previous item functions then move through.
    pub fn set_list_items(&mut self, id: Id, count: i16) {
        if let Some(instance) = self.get_mut(id) {
            instance.list.from_game = true;
            instance.list.count = count.max(0);
            if instance.list.selected >= instance.list.count {
                instance.list.selected = 0;
            }
        }
    }

    /// Disables `id` or enables it again.
    pub fn set_disabled(&mut self, id: Id, disabled: bool) {
        if let Some(instance) = self.get_mut(id) {
            instance.disabled = disabled;
        }
    }

    /// The text `id` shows: its string list's string, or the game's text,
    /// with its replacements made; `None` when it is empty.
    pub fn text(&self, ui: &Ui, id: Id) -> Option<String> {
        let instance = self.get(id)?;
        let definition = ui.widgets.get(instance.definition)?;
        let mut text = match &definition.text.string_list {
            Some(tag) => {
                let index = if instance.string_index != NONE {
                    instance.string_index
                } else {
                    definition.text.string_index
                };
                ui.string_list(tag)?
                    .strings
                    .get(usize::try_from(index).ok()?)?
                    .clone()
            }
            None => instance.text.clone()?,
        };
        if text.is_empty() {
            return None;
        }
        for replacement in &definition.replacements {
            if !replacement.search.is_empty() {
                text = text.replace(
                    &replacement.search,
                    replacement_value(replacement.function, instance.player),
                );
            }
        }
        Some(text)
    }

    /// Opens the screen `tag` for `player` (or [`NONE`] for its
    /// definition's), closing the one that player had open; it is not
    /// added to the history.
    pub fn open(&mut self, ui: &Ui, host: &mut dyn Host, tag: &TagRef, player: i16) -> Option<Id> {
        self.load_root(ui, host, tag, player, None)
    }

    /// Closes every screen and forgets every history.
    pub fn close_all(&mut self, ui: &Ui, host: &mut dyn Host) {
        for stack in 0..LOCAL_PLAYERS {
            if let Some(root) = self.roots[stack] {
                self.delete(ui, host, root);
            }
            self.history[stack].clear();
        }
    }

    /// Gives `event` to each screen that takes its controller.
    pub fn handle(&mut self, ui: &Ui, host: &mut dyn Host, event: Event) {
        let roots = self.roots;
        for (stack, root) in roots.into_iter().enumerate() {
            let Some(root) = root else { continue };
            if self.roots[stack] != Some(root) || !self.takes(ui, root, event.controller) {
                continue;
            }
            let mut event = event;
            self.process(ui, host, root, &mut event);
            self.restore(ui, host, stack);
        }
    }

    /// Moves the clock to `now_ms`, runs each screen's timers and fades,
    /// and gives each widget its data for the frame.
    pub fn update(&mut self, ui: &Ui, host: &mut dyn Host, now_ms: u64) {
        self.now_ms = now_ms;
        for stack in 0..LOCAL_PLAYERS {
            if let Some(root) = self.roots[stack] {
                let controller = self.get(root).map_or(NONE, |instance| instance.player);
                let mut event = Event {
                    controller,
                    input: Input::Tick,
                };
                self.process(ui, host, root, &mut event);
            }
            self.restore(ui, host, stack);
        }
        let ids: Vec<Id> = self.widgets.keys().copied().collect();
        for id in ids {
            let Some(definition) = self.definition(ui, id) else {
                continue;
            };
            for &input in &definition.game_data_inputs {
                if !self.alive(id) {
                    break;
                }
                if data_input_name(input).is_some_and(|name| name != "null") {
                    host.data_input(input, self, ui, id);
                }
            }
        }
    }

    /// Whether a d-pad direction of `controller`, held since it was last
    /// pressed, counts as pressed again now.
    pub fn dpad_repeats(&self, controller: i16, index: i16) -> bool {
        let (Ok(controller), Ok(direction)) = (
            usize::try_from(controller),
            usize::try_from(index - EventHandler::DPAD_UP),
        ) else {
            return false;
        };
        self.dpad_ms
            .get(controller)
            .and_then(|times| times.get(direction))
            .is_some_and(|&last| self.now_ms.saturating_sub(last) >= DPAD_REPEAT_MS)
    }

    // loading and closing

    fn load_root(
        &mut self,
        ui: &Ui,
        host: &mut dyn Host,
        tag: &TagRef,
        player: i16,
        invoking: Option<Invoking>,
    ) -> Option<Id> {
        let definition = definition_index(ui, tag)?;
        let stack = stack_of(player);
        let mut previous_player = NONE;
        // (closing a screen can open another, which then closes too)
        for _ in 0..LOCAL_PLAYERS {
            let Some(active) = self.roots[stack] else {
                break;
            };
            previous_player = self.get(active).map_or(NONE, |instance| instance.player);
            self.delete(ui, host, active);
        }
        if let Some(active) = self.roots[stack] {
            self.remove_quietly(active);
        }
        if let Some((widget, focused_parent, focused_index)) = invoking {
            if ui
                .widget(&widget)
                .is_some_and(|invoker| invoker.flags & Widget::DONT_PUSH_HISTORY == 0)
            {
                self.history[stack].push(HistoryEntry {
                    widget,
                    focused_parent,
                    focused_index,
                    player: previous_player,
                });
            }
        }
        let id = self.create(ui, definition, None, player, false, [0, 0])?;
        self.roots[stack] = Some(id);
        self.initialize(ui, host, id);
        self.alive(id).then_some(id)
    }

    fn create(
        &mut self,
        ui: &Ui,
        definition: usize,
        parent: Option<Id>,
        player: i16,
        generated: bool,
        offset: [i16; 2],
    ) -> Option<Id> {
        if self.widgets.len() >= MAXIMUM_WIDGETS {
            return None;
        }
        let widget = ui.widgets.get(definition)?;
        let player = if player == NONE && (0..4).contains(&widget.controller) {
            widget.controller
        } else {
            player
        };
        let id = Id(self.next);
        self.next += 1;
        self.widgets.insert(
            id,
            Instance {
                definition,
                kind: widget.kind,
                parent,
                children: Vec::new(),
                focused: None,
                offset,
                player,
                disabled: false,
                visible: true,
                created_ms: self.now_ms,
                close_ms: widget.auto_close_ms.max(0),
                fade_ms: widget.auto_close_fade_ms.max(0),
                alpha: 1.0,
                text: None,
                string_index: NONE,
                list: ListState::default(),
                generated,
                extended_description: None,
            },
        );
        if let Some(parent) = parent.and_then(|parent| self.widgets.get_mut(&parent)) {
            parent.children.push(id);
        }
        Some(id)
    }

    fn initialize(&mut self, ui: &Ui, host: &mut dyn Host, id: Id) {
        let Some(instance) = self.get(id) else { return };
        let definition = instance.definition;
        if !instance.generated {
            self.load_children(ui, host, id);
        }
        for handler in ui.widgets[definition].handlers_for(EventHandler::CREATED) {
            if !self.alive(id) {
                return;
            }
            self.dispatch(ui, host, id, handler);
        }
        if self
            .get(id)
            .is_some_and(|instance| instance.focused.is_none())
        {
            let first = self.get(id).and_then(|instance| {
                instance
                    .children
                    .iter()
                    .copied()
                    .find(|&child| self.can_handle_events(ui, child))
            });
            if let Some(instance) = self.widgets.get_mut(&id) {
                instance.focused = first;
            }
        }
    }

    fn depth(&self, id: Id) -> usize {
        let mut depth = 0;
        let mut node = id;
        while let Some(parent) = self.get(node).and_then(|instance| instance.parent) {
            depth += 1;
            node = parent;
        }
        depth
    }

    fn load_children(&mut self, ui: &Ui, host: &mut dyn Host, id: Id) {
        let Some(instance) = self.get(id) else { return };
        if self.depth(id) >= MAXIMUM_DEPTH {
            return;
        }
        let (index, player, offset) = (instance.definition, instance.player, instance.offset);
        let definition = &ui.widgets[index];
        if definition.list.flags & LIST_ITEMS_FROM_STRING_LIST != 0 {
            let count = definition
                .text
                .string_list
                .as_ref()
                .and_then(|tag| ui.string_list(tag))
                .map_or(0, |list| list.strings.len().min(MAXIMUM_LIST_ITEMS));
            for item in 0..count {
                let Some(child) = self.create(ui, index, Some(id), player, true, offset) else {
                    return;
                };
                if let Some(child) = self.widgets.get_mut(&child) {
                    child.string_index = i16::try_from(item).unwrap_or(NONE);
                }
                if let Some(instance) = self.widgets.get_mut(&id) {
                    instance.list.generated += 1;
                }
                self.initialize(ui, host, child);
                if !self.alive(id) {
                    return;
                }
            }
        }
        for reference in &definition.children {
            let Some(child) = reference
                .widget
                .as_ref()
                .and_then(|tag| definition_index(ui, tag))
            else {
                continue;
            };
            let player = if reference.flags & ChildWidget::USE_CUSTOM_CONTROLLER_INDEX != 0
                && (0..4).contains(&reference.controller)
            {
                reference.controller
            } else {
                player
            };
            let offset = [
                offset[0].saturating_add(reference.offset[0]),
                offset[1].saturating_add(reference.offset[1]),
            ];
            let Some(child) = self.create(ui, child, Some(id), player, false, offset) else {
                return;
            };
            self.initialize(ui, host, child);
            if !self.alive(id) {
                return;
            }
        }
        if definition.kind == WidgetKind::ColumnList {
            if let Some(extended) = definition
                .extended_description
                .as_ref()
                .and_then(|tag| definition_index(ui, tag))
            {
                if let Some(extended) = self.create(ui, extended, None, player, false, [0, 0]) {
                    if let Some(instance) = self.widgets.get_mut(&id) {
                        instance.extended_description = Some(extended);
                    }
                    self.initialize(ui, host, extended);
                }
            }
        }
        if definition.flags & Widget::DONT_FOCUS_A_SPECIFIC_CHILD_WIDGET == 0 {
            let Some(instance) = self.get(id) else { return };
            let focused = if is_list(instance.kind) {
                instance.children.first().copied()
            } else if definition.flags & Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD != 0 {
                instance
                    .children
                    .iter()
                    .copied()
                    .find(|&child| self.can_handle_events(ui, child))
            } else {
                None
            };
            if let Some(instance) = self.widgets.get_mut(&id) {
                if is_list(instance.kind) {
                    instance.list.selected = 0;
                    instance.list.last_direction = 0;
                }
                if focused.is_some() {
                    instance.focused = focused;
                }
            }
        }
    }

    /// Closes `id`: its handlers for being closed run, then its children
    /// close, and it leaves its parent.
    pub fn delete(&mut self, ui: &Ui, host: &mut dyn Host, id: Id) {
        let Some(instance) = self.get(id) else { return };
        let definition = instance.definition;
        let mut launches = Vec::new();
        for handler in ui.widgets[definition].handlers_for(EventHandler::DELETED) {
            if handler.flags & EventHandler::RUN_FUNCTION != 0
                && self.invoke(ui, host, handler.function, id)
                && handler.flags & EventHandler::OPEN_WIDGET != 0
            {
                if let Some(tag) = &handler.widget {
                    if let Some(launch) = self.launch_arguments(ui, id, tag) {
                        launches.push(launch);
                    }
                }
            }
            if !self.alive(id) {
                return;
            }
        }
        let children = self.get(id).map(|instance| instance.children.clone());
        for child in children.into_iter().flatten() {
            self.delete(ui, host, child);
        }
        if let Some(extended) = self
            .get(id)
            .and_then(|instance| instance.extended_description)
        {
            self.delete(ui, host, extended);
        }
        self.unlink(id);
        for (tag, player, invoking) in launches {
            self.load_root(ui, host, &tag, player, Some(invoking));
        }
    }

    /// Removes `id` and everything below it without running a handler.
    fn remove_quietly(&mut self, id: Id) {
        let Some(instance) = self.get(id) else { return };
        let mut below = instance.children.clone();
        below.extend(instance.extended_description);
        for child in below {
            self.remove_quietly(child);
        }
        self.unlink(id);
    }

    fn unlink(&mut self, id: Id) {
        let Some(instance) = self.widgets.remove(&id) else {
            return;
        };
        if let Some(parent) = instance
            .parent
            .and_then(|parent| self.widgets.get_mut(&parent))
        {
            parent.children.retain(|&child| child != id);
            if parent.focused == Some(id) {
                parent.focused = None;
            }
        }
        for root in &mut self.roots {
            if *root == Some(id) {
                *root = None;
            }
        }
    }

    /// When player `stack`'s screen closed and it has somewhere to go back
    /// to, opens that.
    fn restore(&mut self, ui: &Ui, host: &mut dyn Host, stack: usize) {
        if self.roots[stack].is_none() {
            if let Some(entry) = self.history[stack].pop() {
                self.reopen(ui, host, entry);
            }
        }
    }

    fn reopen(&mut self, ui: &Ui, host: &mut dyn Host, entry: HistoryEntry) {
        if let Some(screen) = self.load_root(ui, host, &entry.widget, entry.player, None) {
            self.set_focused_child_by_index(
                ui,
                entry.focused_parent.as_ref(),
                screen,
                entry.focused_index,
            );
        }
    }

    // what can take events and the focus

    fn can_handle_events(&self, ui: &Ui, id: Id) -> bool {
        let Some(instance) = self.get(id) else {
            return false;
        };
        !instance.disabled
            && (is_list(instance.kind)
                || ui
                    .widgets
                    .get(instance.definition)
                    .is_some_and(|widget| !widget.handlers.is_empty()))
    }

    /// Whether events reach `id`: it is enabled, and each widget above it
    /// passes events to its focused child or is a list.
    pub fn can_receive_events(&self, ui: &Ui, id: Id) -> bool {
        let Some(instance) = self.get(id) else {
            return false;
        };
        if instance.disabled {
            return false;
        }
        let mut node = instance.parent;
        while let Some(parent) = node.and_then(|parent| self.get(parent)) {
            let passes = ui.widgets.get(parent.definition).is_some_and(|widget| {
                widget.flags & Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD != 0
            });
            if !passes && !is_list(parent.kind) {
                return false;
            }
            node = parent.parent;
        }
        true
    }

    fn takes(&self, ui: &Ui, id: Id, controller: i16) -> bool {
        let Some(instance) = self.get(id) else {
            return false;
        };
        instance.player == NONE
            || instance.player == controller
            || ((1..4).contains(&controller)
                && ui
                    .widgets
                    .get(instance.definition)
                    .is_some_and(|widget| widget.controller == 4))
    }

    fn is_multi_spinner(&self, ui: &Ui, id: Id) -> bool {
        self.get(id).is_some_and(|instance| {
            instance.kind == WidgetKind::SpinnerList
                && ui
                    .widgets
                    .get(instance.definition)
                    .is_some_and(|widget| widget.children.len() > 1)
        })
    }

    /// Gives the focus to `target`, on the screen `widget` is on; a
    /// disabled target passes it to a sibling that can take it.
    pub fn give_focus_directly(&mut self, ui: &Ui, widget: Id, target: Id) {
        if !self.alive(target) {
            return;
        }
        let target = self.substitute_disabled(ui, target);
        let root = self.root_of(widget);
        let target_parent = self.get(target).and_then(|instance| instance.parent);
        if let Some(focused) = self.get(root).and_then(|instance| instance.focused) {
            let focused_parent = self.get(focused).and_then(|instance| instance.parent);
            if let (Some(parent), Some(other)) = (target_parent, focused_parent) {
                if parent == other {
                    if let Some(parent) = self.widgets.get_mut(&parent) {
                        parent.focused = Some(target);
                    }
                    return;
                }
            }
            let mut node = Some(focused);
            let mut steps = 0;
            while let Some(id) = node {
                let Some(instance) = self.get(id) else { break };
                let (parent, next) = (instance.parent, instance.focused);
                if let Some(parent) = parent.and_then(|parent| self.widgets.get_mut(&parent)) {
                    parent.focused = None;
                }
                node = next;
                steps += 1;
                if steps > MAXIMUM_DEPTH * 2 {
                    break;
                }
            }
        }
        let mut node = target;
        while let Some(parent) = self.get(node).and_then(|instance| instance.parent) {
            if let Some(instance) = self.widgets.get_mut(&parent) {
                instance.focused = Some(node);
            }
            node = parent;
        }
    }

    fn substitute_disabled(&self, ui: &Ui, target: Id) -> Id {
        if !self.get(target).is_some_and(|instance| instance.disabled) {
            return target;
        }
        let Some(parent) = self.get(target).and_then(|instance| instance.parent) else {
            return target;
        };
        let Some(siblings) = self
            .get(parent)
            .map(|instance| instance.children.as_slice())
        else {
            return target;
        };
        let position = siblings
            .iter()
            .position(|&child| child == target)
            .unwrap_or(0);
        let can = |id: &&Id| self.can_handle_events(ui, **id);
        if let Some(&next) = siblings[position + 1..].iter().find(can) {
            return next;
        }
        if let Some(&first) = siblings.iter().find(can) {
            if self.get(parent).and_then(|instance| instance.focused) != Some(first) {
                return first;
            }
        }
        siblings[..position]
            .iter()
            .rev()
            .find(can)
            .copied()
            .unwrap_or(target)
    }

    fn tab(&mut self, ui: &Ui, widget: Id, forward: bool) {
        let Some(instance) = self.get(widget) else {
            return;
        };
        let children = instance.children.clone();
        let count = children.len();
        if count == 0 {
            return;
        }
        let parent_is_list = is_list(instance.kind);
        let focused = instance.focused;
        let here = focused.and_then(|focused| children.iter().position(|&child| child == focused));
        let mut position = match (here, forward) {
            (Some(here), true) => (here + 1) % count,
            (Some(here), false) => (here + count - 1) % count,
            (None, true) => 0,
            (None, false) => count - 1,
        };
        for _ in 0..count {
            let child = children[position];
            if Some(child) == focused {
                return;
            }
            let eligible = parent_is_list
                || self.definition(ui, child).is_some_and(|definition| {
                    !definition.handlers.is_empty()
                        || definition.flags & Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD != 0
                });
            if eligible {
                if let Some(instance) = self.widgets.get_mut(&widget) {
                    instance.focused = Some(child);
                }
                return;
            }
            position = if forward {
                (position + 1) % count
            } else {
                (position + count - 1) % count
            };
        }
    }

    /// Moves the list `widget` to its next item (`forward`) or its
    /// previous one.
    pub fn move_item(&mut self, ui: &Ui, widget: Id, forward: bool) {
        let Some(instance) = self.get(widget) else {
            return;
        };
        let Some(definition) = ui.widgets.get(instance.definition) else {
            return;
        };
        let children = instance.children.clone();
        let (kind, list, focused) = (instance.kind, instance.list.clone(), instance.focused);
        let three_wide = kind == WidgetKind::SpinnerList && definition.children.len() > 1;
        let selected = if list.from_game && list.count > 0 {
            let index = if forward {
                (list.selected + 1) % list.count
            } else {
                (list.selected + list.count - 1) % list.count
            };
            let here =
                focused.and_then(|focused| children.iter().position(|&child| child == focused));
            if kind == WidgetKind::ColumnList {
                if let Some(&child) = usize::try_from(index)
                    .ok()
                    .and_then(|index| children.get(index))
                {
                    self.give_focus_directly(ui, widget, child);
                }
            } else if three_wide {
                let next = here.and_then(|here| {
                    if forward {
                        children.get(here + 1)
                    } else {
                        here.checked_sub(1).and_then(|before| children.get(before))
                    }
                });
                if let Some(&next) = next {
                    self.give_focus_directly(ui, widget, next);
                }
            }
            index
        } else if kind == WidgetKind::SpinnerList
            && definition.list.flags & LIST_ITEMS_FROM_STRING_LIST != 0
            && definition.children.is_empty()
        {
            let count = list.generated.max(1);
            if forward {
                (list.selected + 1) % count
            } else {
                (list.selected + count - 1) % count
            }
        } else {
            if children.is_empty() {
                return;
            }
            let here =
                focused.and_then(|focused| children.iter().position(|&child| child == focused));
            let position = match (here, forward) {
                (Some(here), true) if here + 1 < children.len() => here + 1,
                (Some(here), false) if here > 0 => here - 1,
                (_, true) => 0,
                (_, false) => children.len() - 1,
            };
            self.give_focus_directly(ui, widget, children[position]);
            i16::try_from(position).unwrap_or(0)
        };
        if let Some(instance) = self.widgets.get_mut(&widget) {
            instance.list.selected = selected;
            instance.list.last_direction = if forward {
                LIST_TAB_DIRECTION
            } else {
                -LIST_TAB_DIRECTION
            };
        }
    }

    fn set_focused_child_by_index(
        &mut self,
        ui: &Ui,
        tag: Option<&TagRef>,
        widget: Id,
        index: i16,
    ) {
        let Some(found) = tag.and_then(|tag| self.find(ui, widget, tag)) else {
            return;
        };
        if let Ok(index) = usize::try_from(index) {
            let children = self
                .get(found)
                .map(|instance| instance.children.clone())
                .unwrap_or_default();
            for (position, &child) in children.iter().enumerate() {
                if self.is_multi_spinner(ui, child) {
                    return;
                }
                if position == index {
                    self.give_focus_directly(ui, found, child);
                    if let Some(instance) = self.widgets.get_mut(&found) {
                        if is_list(instance.kind) {
                            instance.list.selected = i16::try_from(index).unwrap_or(0);
                        }
                    }
                    return;
                }
            }
        } else if self.can_receive_events(ui, found) && !self.is_multi_spinner(ui, found) {
            self.give_focus_directly(ui, found, found);
        }
    }

    // opening and going back

    /// The screen, focused parent and index a screen opened from `widget`
    /// remembers, and the player it opens for.
    fn launch_arguments(
        &self,
        ui: &Ui,
        widget: Id,
        tag: &TagRef,
    ) -> Option<(TagRef, i16, Invoking)> {
        let definition = ui.widget(tag)?;
        let instance = self.get(widget)?;
        let player = if (0..4).contains(&definition.controller) {
            definition.controller
        } else if definition.flags & Widget::ALWAYS_USE_TAG_CONTROLLER_INDEX != 0 {
            NONE
        } else {
            instance.player
        };
        let root = self.definition(ui, self.root_of(widget))?.tag.clone();
        let parent = instance.parent;
        let focused_parent = parent
            .and_then(|parent| self.definition(ui, parent))
            .map(|definition| definition.tag.clone());
        let focused_index = parent
            .and_then(|parent| self.get(parent))
            .and_then(|parent| parent.children.iter().position(|&child| child == widget))
            .and_then(|index| i16::try_from(index).ok())
            .unwrap_or(NONE);
        Some((tag.clone(), player, (root, focused_parent, focused_index)))
    }

    /// Opens the screen `tag` from `widget`, remembering where it came
    /// from.
    pub fn launch(&mut self, ui: &Ui, host: &mut dyn Host, widget: Id, tag: &TagRef) -> Option<Id> {
        let (tag, player, invoking) = self.launch_arguments(ui, widget, tag)?;
        self.load_root(ui, host, &tag, player, Some(invoking))
    }

    /// Closes the screen `widget` is on and opens the one its player came
    /// from.
    pub fn go_back(&mut self, ui: &Ui, host: &mut dyn Host, widget: Id) {
        let Some(instance) = self.get(widget) else {
            return;
        };
        let stack = stack_of(instance.player);
        let entry = self.history[stack].pop();
        let root = self.root_of(widget);
        self.delete(ui, host, root);
        if let Some(entry) = entry {
            self.reopen(ui, host, entry);
        }
    }

    fn replace(&mut self, ui: &Ui, host: &mut dyn Host, widget: Id, tag: &TagRef) -> bool {
        let Some(definition) = definition_index(ui, tag) else {
            return false;
        };
        let Some(instance) = self.get(widget) else {
            return false;
        };
        let (parent, offset, player) = (instance.parent, instance.offset, instance.player);
        let slot = self.roots.iter().position(|&root| root == Some(widget));
        let position = parent
            .and_then(|parent| self.get(parent))
            .and_then(|parent| parent.children.iter().position(|&child| child == widget));
        let was_focused = parent
            .and_then(|parent| self.get(parent))
            .is_some_and(|parent| parent.focused == Some(widget));
        let Some(new) = self.create(ui, definition, parent, player, false, offset) else {
            return false;
        };
        if let (Some(parent), Some(position)) = (
            parent.and_then(|parent| self.widgets.get_mut(&parent)),
            position,
        ) {
            parent.children.retain(|&child| child != new);
            parent.children.insert(position, new);
        }
        self.initialize(ui, host, new);
        self.delete(ui, host, widget);
        if self.alive(new) {
            if was_focused {
                if let Some(parent) = parent.and_then(|parent| self.widgets.get_mut(&parent)) {
                    parent.focused = Some(new);
                }
            }
            if let Some(slot) = slot {
                if self.roots[slot].is_none() {
                    self.roots[slot] = Some(new);
                }
            }
        }
        true
    }

    // handlers

    /// Runs the function `function` for `widget`: whether it succeeded.
    fn invoke(&mut self, ui: &Ui, host: &mut dyn Host, function: i16, widget: Id) -> bool {
        match function {
            0 | 3 | 4 => true,
            LIST_GOTO_NEXT_ITEM => {
                self.move_item(ui, widget, true);
                true
            }
            LIST_GOTO_PREVIOUS_ITEM => {
                self.move_item(ui, widget, false);
                true
            }
            function if function_name(function).is_some() => {
                host.run(function, self, ui, widget) != Outcome::Failed
            }
            _ => false,
        }
    }

    /// Does what `handler` says, for `widget`: whether `widget` closed.
    fn dispatch(
        &mut self,
        ui: &Ui,
        host: &mut dyn Host,
        widget: Id,
        handler: &EventHandler,
    ) -> bool {
        let flags = handler.flags;
        let mut success = true;
        let mut failed = false;
        let mut deleted = false;
        let mut feedback = None;
        if flags & EventHandler::RUN_SCENARIO_SCRIPT != 0 && !handler.script.is_empty() {
            host.script(&handler.script);
        }
        if flags & EventHandler::RUN_FUNCTION != 0
            && !self.invoke(ui, host, handler.function, widget)
        {
            failed = true;
        } else if !self.alive(widget) {
            deleted = true;
        } else {
            let (mut close_current, mut close_screen, mut close_all) = (false, false, false);
            if flags & EventHandler::GIVE_FOCUS_TO_WIDGET != 0 {
                let root = self.root_of(widget);
                match handler
                    .widget
                    .as_ref()
                    .and_then(|tag| self.find(ui, root, tag))
                {
                    Some(target) => {
                        self.give_focus_directly(ui, widget, target);
                        feedback = Some(Feedback::Cursor);
                    }
                    None => success = false,
                }
            }
            if flags & EventHandler::RELOAD_OTHER_WIDGET != 0 && handler.widget.is_none() {
                success = false;
            }
            if flags & EventHandler::CLOSE_CURRENT_WIDGET != 0 {
                close_current = true;
            }
            if flags & EventHandler::CLOSE_OTHER_WIDGET != 0 {
                let other = handler.widget.as_ref().and_then(|tag| {
                    self.roots().find(|&root| {
                        self.definition(ui, root)
                            .is_some_and(|definition| &definition.tag == tag)
                    })
                });
                match other {
                    Some(other) if other == self.root_of(widget) => close_screen = true,
                    Some(other) => self.delete(ui, host, other),
                    None => success = false,
                }
            }
            if flags & EventHandler::CLOSE_ALL_WIDGETS != 0 {
                close_all = true;
            }
            if flags & EventHandler::OPEN_WIDGET != 0 && !deleted && self.alive(widget) {
                match &handler.widget {
                    Some(tag) if self.launch(ui, host, widget, tag).is_some() => {
                        feedback.get_or_insert(Feedback::Forward);
                        deleted = true;
                    }
                    _ => success = false,
                }
            }
            if flags & EventHandler::REPLACE_SELF_WITH_WIDGET != 0 && !deleted && self.alive(widget)
            {
                match &handler.widget {
                    Some(tag) if self.replace(ui, host, widget, tag) => {
                        feedback.get_or_insert(Feedback::Forward);
                        deleted = true;
                    }
                    _ => success = false,
                }
            }
            if flags & EventHandler::GO_BACK_TO_PREVIOUS_WIDGET != 0 && self.alive(widget) {
                self.go_back(ui, host, widget);
                feedback = Some(Feedback::Back);
                deleted = true;
            }
            if let Some(sound) = &handler.sound {
                host.sound(sound);
            }
            if close_all {
                self.close_all(ui, host);
                deleted = true;
            } else if close_screen {
                if self.alive(widget) {
                    let root = self.root_of(widget);
                    self.delete(ui, host, root);
                }
                deleted = true;
            } else if close_current {
                self.delete(ui, host, widget);
                deleted = true;
            }
        }
        if (!success || failed) && failed && flags & EventHandler::TRY_TO_BRANCH_ON_FAILURE != 0 {
            let conditionals = self
                .definition(ui, widget)
                .map(|definition| definition.conditionals.as_slice())
                .unwrap_or_default();
            for conditional in conditionals {
                if conditional.flags & CONDITIONAL_IF_FUNCTION_FAILS == 0
                    || deleted
                    || !self.alive(widget)
                {
                    continue;
                }
                if let Some(tag) = &conditional.widget {
                    self.launch(ui, host, widget, tag);
                    deleted = true;
                }
            }
        }
        if let Some(feedback) = feedback {
            host.feedback(feedback);
        }
        deleted || !self.alive(widget)
    }

    /// Gives `event` to `widget` and, as its flags say, its children:
    /// whether a widget closed.
    fn process(&mut self, ui: &Ui, host: &mut dyn Host, widget: Id, event: &mut Event) -> bool {
        let Some(instance) = self.get(widget) else {
            return true;
        };
        let definition = &ui.widgets[instance.definition];
        let flags = definition.flags;
        let for_this = self.takes(ui, widget, event.controller);
        let mut handled = false;
        let mut deleted = false;
        let mut feedback = None;
        if let Input::Button { index, value } = &mut event.input {
            if *value > 1
                && (EventHandler::DPAD_UP..=EventHandler::DPAD_RIGHT).contains(index)
                && self.dpad_repeats(event.controller, *index)
            {
                *value = 1;
            }
        }
        if for_this {
            if let Input::Button { index, value: 1 } = event.input {
                if (index == EventHandler::B_BUTTON || index == EventHandler::BACK_BUTTON)
                    && definition.handlers_for(index).next().is_none()
                {
                    self.go_back(ui, host, widget);
                    feedback = Some(Feedback::Back);
                    deleted = true;
                    handled = true;
                }
            }
        }
        if !deleted {
            deleted = self.auto_close(ui, host, widget);
        }
        if !deleted && for_this {
            let (kind, has_focus) = self
                .get(widget)
                .map_or((WidgetKind::Container, false), |instance| {
                    (instance.kind, instance.focused.is_some())
                });
            let tabs = [
                (Widget::DPAD_UP_DOWN_TABS_THRU_CHILDREN, true, false),
                (Widget::DPAD_LEFT_RIGHT_TABS_THRU_CHILDREN, false, false),
                (Widget::DPAD_UP_DOWN_TABS_THRU_LIST_ITEMS, true, true),
                (Widget::DPAD_LEFT_RIGHT_TABS_THRU_LIST_ITEMS, false, true),
            ];
            for (flag, vertical, items) in tabs {
                if handled
                    || flags & flag == 0
                    || (items && !is_list(kind))
                    || (!items && !has_focus)
                {
                    continue;
                }
                let Some(direction) = step(&event.input, vertical) else {
                    continue;
                };
                if items {
                    self.move_item(ui, widget, direction > 0);
                } else {
                    self.tab(ui, widget, direction > 0);
                }
                feedback = Some(Feedback::Cursor);
                handled = true;
            }
            for handler in &definition.handlers {
                if deleted {
                    break;
                }
                if matches_event(handler.event, &event.input) {
                    handled = true;
                    deleted = self.dispatch(ui, host, widget, handler);
                }
            }
        }
        if (flags & Widget::PASS_HANDLED_EVENTS_TO_ALL_CHILDREN != 0 || !handled)
            && flags
                & (Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD
                    | Widget::PASS_UNHANDLED_EVENTS_TO_ALL_CHILDREN)
                != 0
            && !deleted
        {
            if flags & Widget::PASS_UNHANDLED_EVENTS_TO_ALL_CHILDREN != 0 {
                let children = self
                    .get(widget)
                    .map(|instance| instance.children.clone())
                    .unwrap_or_default();
                for child in children {
                    if self.takes(ui, child, event.controller)
                        && self.process(ui, host, child, event)
                    {
                        deleted = true;
                        break;
                    }
                }
            } else if let Some(focused) = self.get(widget).and_then(|instance| instance.focused) {
                if self.takes(ui, focused, event.controller) {
                    deleted = self.process(ui, host, focused, event);
                }
            }
        }
        if deleted
            && flags & Widget::RETURN_TO_MAIN_MENU_IF_NO_HISTORY != 0
            && self.roots().next().is_none()
        {
            host.main_menu(self, ui);
        }
        if let Input::Button { index, value: 1 } = event.input {
            if let (Ok(controller), Ok(direction)) = (
                usize::try_from(event.controller),
                usize::try_from(index - EventHandler::DPAD_UP),
            ) {
                if let Some(time) = self
                    .dpad_ms
                    .get_mut(controller)
                    .and_then(|times| times.get_mut(direction))
                {
                    *time = self.now_ms;
                }
            }
        }
        if let Some(feedback) = feedback {
            host.feedback(feedback);
        }
        deleted
    }

    /// Runs `widget`'s timer: whether its screen closed.
    fn auto_close(&mut self, ui: &Ui, host: &mut dyn Host, widget: Id) -> bool {
        let Some(instance) = self.get(widget) else {
            return true;
        };
        if instance.close_ms <= 0 {
            return false;
        }
        let elapsed =
            i64::try_from(self.now_ms.saturating_sub(instance.created_ms)).unwrap_or(i64::MAX);
        let (close, fade) = (i64::from(instance.close_ms), i64::from(instance.fade_ms));
        if elapsed >= close + fade {
            let root = self.root_of(widget);
            self.delete(ui, host, root);
            return true;
        }
        let faded = elapsed - close;
        if fade > 0 && faded > 0 {
            if let Some(instance) = self.widgets.get_mut(&widget) {
                // (both under a minute's milliseconds: exact in an f32)
                #[allow(clippy::cast_precision_loss)]
                let alpha = 1.0 - faded as f32 / fade as f32;
                instance.alpha = alpha;
            }
        }
        false
    }
}
