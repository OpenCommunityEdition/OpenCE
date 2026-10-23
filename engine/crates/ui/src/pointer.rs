//! The keyboard and the mouse in the menus, as the desktop builds take them
//! (port/linux/src/xinput_sdl.c's `keyboard_gamepad` and the mouse of
//! source/interface/ui_widget.c): the keys press the pad's buttons, the
//! item under the pointer takes the focus, a left click presses A on it (on
//! a list showing one value, back or on by the half clicked), a right click
//! presses B and the wheel the d-pad.

use opence_content::{EventHandler, Ui, Widget, WidgetKind};

use crate::{Event, Feedback, Host, Id, Input, Shell};

/// The most d-pad presses one turn of the wheel makes.
pub const MAXIMUM_WHEEL_STEPS: i32 = 4;

/// A key the menus take.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Key {
    /// The arrow keys: the d-pad.
    Up,
    /// Down.
    Down,
    /// Left.
    Left,
    /// Right.
    Right,
    /// Space, Return and the keypad's Enter: A.
    Space,
    /// Return.
    Return,
    /// The keypad's Enter.
    KeypadEnter,
    /// Escape and Backspace: B.
    Escape,
    /// Backspace.
    Backspace,
    /// Delete and E: X.
    Delete,
    /// E.
    E,
    /// Tab: Y.
    Tab,
    /// F1: Back.
    F1,
    /// W, A, S and D: the left stick, all the way.
    W,
    /// A.
    A,
    /// S.
    S,
    /// D.
    D,
}

/// What `key`, pressed, gives the widgets.
pub fn key_input(key: Key) -> Input {
    let button = |index| Input::Button { index, value: 1 };
    match key {
        Key::Up => button(EventHandler::DPAD_UP),
        Key::Down => button(EventHandler::DPAD_DOWN),
        Key::Left => button(EventHandler::DPAD_LEFT),
        Key::Right => button(EventHandler::DPAD_RIGHT),
        Key::Space | Key::Return | Key::KeypadEnter => button(EventHandler::A_BUTTON),
        Key::Escape | Key::Backspace => button(EventHandler::B_BUTTON),
        Key::Delete | Key::E => button(EventHandler::X_BUTTON),
        Key::Tab => button(EventHandler::Y_BUTTON),
        Key::F1 => button(EventHandler::BACK_BUTTON),
        Key::W => Input::LeftStick([0, i16::MAX]),
        Key::S => Input::LeftStick([0, i16::MIN]),
        Key::A => Input::LeftStick([i16::MIN, 0]),
        Key::D => Input::LeftStick([i16::MAX, 0]),
    }
}

/// A mouse button the menus take.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MouseButton {
    /// A on the item under the pointer.
    Left,
    /// B.
    Right,
}

/// What a place on the screen is to the pointer.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TargetKind {
    /// An item the d-pad moves the focus to: a click presses A on it.
    Item,
    /// A list showing one value at a time: a click on either half steps it
    /// that way.
    Value,
    /// One of the items a list shows side by side: the list steps to it,
    /// and a click then presses A.
    Slot,
    /// A button's icon in a screen's key: a click presses that button.
    Button(i16),
}

/// A widget the pointer can act on, and where it is drawn.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Target {
    /// The widget.
    pub widget: Id,
    /// Its bounds on the 640×480 screen: top, left, bottom, right.
    pub bounds: [i16; 4],
    /// What the pointer does to it.
    pub kind: TargetKind,
}

/// The button a screen key's icon stands for ("a_butn", and the dialogs'
/// smaller "a_butn_sm"), by the leaf of its tag path.
pub fn key_button(path: &str) -> Option<i16> {
    const KEYS: [(&str, i16); 8] = [
        ("a_butn", EventHandler::A_BUTTON),
        ("b_butn", EventHandler::B_BUTTON),
        ("x_butn", EventHandler::X_BUTTON),
        ("y_butn", EventHandler::Y_BUTTON),
        ("black_butn", EventHandler::BLACK_BUTTON),
        ("white_butn", EventHandler::WHITE_BUTTON),
        ("start_butn", EventHandler::START_BUTTON),
        ("back_butn", EventHandler::BACK_BUTTON),
    ];
    let leaf = path.rsplit('\\').next().unwrap_or(path);
    KEYS.iter().find_map(|&(name, button)| {
        let rest = leaf.strip_prefix(name)?;
        (rest.is_empty() || rest.starts_with('_')).then_some(button)
    })
}

/// The d-pad's back and on for `definition`: left and right if it tabs
/// that way, else up and down.
fn directions(definition: &Widget) -> (i16, i16) {
    if definition.flags
        & (Widget::DPAD_LEFT_RIGHT_TABS_THRU_LIST_ITEMS
            | Widget::DPAD_LEFT_RIGHT_TABS_THRU_CHILDREN)
        != 0
    {
        (EventHandler::DPAD_LEFT, EventHandler::DPAD_RIGHT)
    } else {
        (EventHandler::DPAD_UP, EventHandler::DPAD_DOWN)
    }
}

fn contains(bounds: [i16; 4], point: [f32; 2]) -> bool {
    let [top, left, bottom, right] = bounds.map(f32::from);
    (left..right).contains(&point[0]) && (top..bottom).contains(&point[1])
}

impl Shell {
    /// The controller the keyboard and the mouse are: the first screen's
    /// player, or the first controller for a screen any of them uses.
    fn pointer_controller(&self) -> i16 {
        self.root(0)
            .and_then(|root| self.get(root))
            .map_or(0, |instance| instance.player.max(0))
    }

    fn press_pointer(&mut self, ui: &Ui, host: &mut dyn Host, index: i16) {
        let controller = self.pointer_controller();
        self.handle(ui, host, Event::press(controller, index));
    }

    /// Gives `key`'s press to the widgets.
    pub fn key(&mut self, ui: &Ui, host: &mut dyn Host, key: Key) {
        let controller = self.pointer_controller();
        self.handle(
            ui,
            host,
            Event {
                controller,
                input: key_input(key),
            },
        );
    }

    /// Whether `definition`, a list showing several of its items, is one.
    fn shows_several(kind: WidgetKind, definition: &Widget) -> bool {
        kind == WidgetKind::SpinnerList && definition.children.len() > 1
    }

    /// The child index of `id` among its parent's children.
    fn child_index(&self, id: Id) -> Option<usize> {
        let parent = self.get(id)?.parent?;
        self.get(parent)?
            .children
            .iter()
            .position(|&child| child == id)
    }

    /// Whether the d-pad moves the focus to `id`: an item of a column list,
    /// or a child its parent tabs through; or a panel with handlers whose
    /// parent passes its events to all its children.
    fn is_item(&self, ui: &Ui, id: Id) -> bool {
        let Some(instance) = self.get(id) else {
            return false;
        };
        let Some(parent) = instance.parent.and_then(|parent| self.get(parent)) else {
            return false;
        };
        if parent.kind == WidgetKind::SpinnerList || !self.can_receive_events(ui, id) {
            return false;
        }
        let Some(index) = self.child_index(id) else {
            return false;
        };
        if parent.kind == WidgetKind::ColumnList {
            return !parent.list.from_game
                || i16::try_from(index).is_ok_and(|index| index < parent.list.count);
        }
        let Some(parent_definition) = ui.widgets.get(parent.definition) else {
            return false;
        };
        let has_handlers = || {
            ui.widgets
                .get(instance.definition)
                .is_some_and(|definition| !definition.handlers.is_empty())
        };
        // a panel among several that each take the events (the 4-way
        // screens' quarters) is clicked as an item
        if parent_definition.flags & Widget::PASS_UNHANDLED_EVENTS_TO_ALL_CHILDREN != 0 {
            return has_handlers();
        }
        if parent_definition.flags
            & (Widget::DPAD_UP_DOWN_TABS_THRU_CHILDREN | Widget::DPAD_LEFT_RIGHT_TABS_THRU_CHILDREN)
            == 0
        {
            return false;
        }
        ui.widgets
            .get(instance.definition)
            .is_some_and(|definition| {
                !definition.handlers.is_empty()
                    || definition.flags & Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD != 0
                    || matches!(
                        instance.kind,
                        WidgetKind::SpinnerList | WidgetKind::ColumnList
                    )
            })
    }

    /// What the pointer can act on, in the order the widgets are drawn:
    /// a later target is over an earlier one.
    pub fn targets(&self, ui: &Ui) -> Vec<Target> {
        let mut targets = Vec::new();
        for root in self.roots() {
            self.note_targets(ui, root, 0, &mut targets);
        }
        targets
    }

    fn note_targets(&self, ui: &Ui, id: Id, depth: usize, targets: &mut Vec<Target>) {
        let Some(instance) = self.get(id) else {
            return;
        };
        let Some(definition) = ui.widgets.get(instance.definition) else {
            return;
        };
        if !instance.visible || depth > crate::MAXIMUM_DEPTH {
            return;
        }
        let parent = instance.parent.and_then(|parent| self.get(parent));
        let parent_shows_several = parent.is_some_and(|parent| {
            ui.widgets
                .get(parent.definition)
                .is_some_and(|definition| Self::shows_several(parent.kind, definition))
        });
        let kind = if let Some(button) = key_button(&definition.tag.path) {
            Some(TargetKind::Button(button))
        } else if parent_shows_several {
            instance
                .parent
                .filter(|&list| self.can_receive_events(ui, list))
                .map(|_| TargetKind::Slot)
        } else if instance.kind == WidgetKind::SpinnerList {
            (!Self::shows_several(instance.kind, definition)
                && parent.is_some()
                && self.can_receive_events(ui, id))
            .then_some(TargetKind::Value)
        } else if self.is_item(ui, id) {
            Some(TargetKind::Item)
        } else {
            None
        };
        if let (Some(kind), Some(bounds)) = (kind, self.bounds(ui, id)) {
            targets.push(Target {
                widget: id,
                bounds,
                kind,
            });
        }
        // a list showing one value shows it itself
        if instance.kind == WidgetKind::SpinnerList
            && !Self::shows_several(instance.kind, definition)
        {
            return;
        }
        for &child in &instance.children {
            self.note_targets(ui, child, depth + 1, targets);
        }
    }

    /// The target at `point` on the 640×480 screen: the last drawn there.
    pub fn target_at(&self, ui: &Ui, point: [f32; 2]) -> Option<Target> {
        self.targets(ui)
            .into_iter()
            .rev()
            .find(|target| contains(target.bounds, point))
    }

    /// Whether `id` and each widget above it has the focus of its parent.
    fn has_focus(&self, id: Id) -> bool {
        let mut node = id;
        while let Some(parent) = self.get(node).and_then(|instance| instance.parent) {
            if self.get(parent).and_then(|instance| instance.focused) != Some(node) {
                return false;
            }
            node = parent;
        }
        true
    }

    /// Gives `id` the focus, as the d-pad would have, with the cursor's
    /// sound; a column list above it selects the item it is in.
    fn pointer_focus(&mut self, ui: &Ui, host: &mut dyn Host, id: Id) {
        if self.has_focus(id) {
            return;
        }
        self.give_focus_directly(ui, id, id);
        let mut node = id;
        while let Some(parent) = self.get(node).and_then(|instance| instance.parent) {
            let index = self
                .child_index(node)
                .and_then(|index| i16::try_from(index).ok());
            if let (Some(instance), Some(index)) = (self.get_mut(parent), index) {
                if instance.kind == WidgetKind::ColumnList {
                    instance.list.selected = index;
                }
            }
            node = parent;
        }
        host.feedback(Feedback::Cursor);
    }

    /// Steps the list showing several items that `slot` is one of to it,
    /// as pressing the d-pad that many times would.
    fn step_to_slot(&mut self, ui: &Ui, host: &mut dyn Host, slot: Id) {
        let Some(list) = self.get(slot).and_then(|instance| instance.parent) else {
            return;
        };
        self.pointer_focus(ui, host, list);
        let Some(focused) = self.get(list).and_then(|instance| instance.focused) else {
            return;
        };
        let (Some(to), Some(from)) = (self.child_index(slot), self.child_index(focused)) else {
            return;
        };
        let Some((back, forward)) = self.definition(ui, list).map(directions) else {
            return;
        };
        let (button, steps) = if to >= from {
            (forward, to - from)
        } else {
            (back, from - to)
        };
        for _ in 0..steps {
            self.press_pointer(ui, host, button);
        }
    }

    /// The pointer moved to `point` on the 640×480 screen: the item under
    /// it takes the focus (a list's slot steps the list to it).
    pub fn hover(&mut self, ui: &Ui, host: &mut dyn Host, point: [f32; 2]) {
        let Some(target) = self.target_at(ui, point) else {
            return;
        };
        match target.kind {
            TargetKind::Item | TargetKind::Value => self.pointer_focus(ui, host, target.widget),
            TargetKind::Slot => self.step_to_slot(ui, host, target.widget),
            TargetKind::Button(_) => {}
        }
    }

    /// `button` clicked at `point` on the 640×480 screen.
    pub fn click(&mut self, ui: &Ui, host: &mut dyn Host, point: [f32; 2], button: MouseButton) {
        if button == MouseButton::Right {
            self.press_pointer(ui, host, EventHandler::B_BUTTON);
            return;
        }
        let Some(target) = self.target_at(ui, point) else {
            return;
        };
        match target.kind {
            TargetKind::Item => {
                self.pointer_focus(ui, host, target.widget);
                self.press_pointer(ui, host, EventHandler::A_BUTTON);
            }
            TargetKind::Value => {
                self.pointer_focus(ui, host, target.widget);
                let Some((back, forward)) = self.definition(ui, target.widget).map(directions)
                else {
                    return;
                };
                let [top, left, bottom, right] = target.bounds.map(f32::from);
                let first_half = if back == EventHandler::DPAD_LEFT {
                    point[0] < (left + right) * 0.5
                } else {
                    point[1] < (top + bottom) * 0.5
                };
                self.press_pointer(ui, host, if first_half { back } else { forward });
            }
            TargetKind::Slot => {
                self.step_to_slot(ui, host, target.widget);
                self.press_pointer(ui, host, EventHandler::A_BUTTON);
            }
            TargetKind::Button(index) => self.press_pointer(ui, host, index),
        }
    }

    /// The wheel turned `steps` notches, away from the user positive: the
    /// d-pad pressed back (away) or on, up to [`MAXIMUM_WHEEL_STEPS`] times,
    /// along the way the innermost widget on the focus's path that the
    /// d-pad steps through goes.
    pub fn wheel(&mut self, ui: &Ui, host: &mut dyn Host, steps: i32) {
        const TABS: u32 = Widget::DPAD_UP_DOWN_TABS_THRU_CHILDREN
            | Widget::DPAD_LEFT_RIGHT_TABS_THRU_CHILDREN
            | Widget::DPAD_UP_DOWN_TABS_THRU_LIST_ITEMS
            | Widget::DPAD_LEFT_RIGHT_TABS_THRU_LIST_ITEMS;
        let stepped = self
            .focus_path(0)
            .into_iter()
            .filter_map(|id| self.definition(ui, id))
            .rfind(|definition| definition.flags & TABS != 0);
        let (back, forward) =
            stepped.map_or((EventHandler::DPAD_UP, EventHandler::DPAD_DOWN), directions);
        let button = if steps > 0 { back } else { forward };
        for _ in 0..steps.unsigned_abs().min(MAXIMUM_WHEEL_STEPS.unsigned_abs()) {
            self.press_pointer(ui, host, button);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_key_icon_is_known_by_its_leaf() {
        assert_eq!(
            key_button("ui\\shell\\a_butn"),
            Some(EventHandler::A_BUTTON)
        );
        assert_eq!(
            key_button("ui\\shell\\b_butn_sm"),
            Some(EventHandler::B_BUTTON)
        );
        assert_eq!(key_button("back_butn"), Some(EventHandler::BACK_BUTTON));
        assert_eq!(key_button("ui\\shell\\a_butnx"), None);
        assert_eq!(key_button("ui\\shell\\main_menu"), None);
    }
}
