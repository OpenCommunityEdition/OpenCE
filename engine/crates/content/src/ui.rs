//! The UI (`ui.map`, and the pause menu and dialogs every map carries): its
//! widget definitions (`DeLa`), the collections of them (`Soul`), the
//! string lists (`ustr`, `str#`), the fonts (`font`), the colour tables
//! (`colo`) and the virtual keyboard (`vcky`).
//!
//! A widget names the others by tag: its children, the ones its handlers
//! open and its conditional ones. The bitmaps they draw are imported with
//! the level's; the sounds and scripts they name stay names.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// A map's UI tags, each list sorted by tag.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Ui {
    /// The widget definitions.
    pub widgets: Vec<Widget>,
    /// The collections of widgets (the shell's screens).
    pub collections: Vec<WidgetCollection>,
    /// The string lists.
    pub string_lists: Vec<StringList>,
    /// The fonts.
    pub fonts: Vec<Font>,
    /// The colour tables.
    pub color_tables: Vec<ColorTable>,
    /// The virtual keyboards.
    pub keyboards: Vec<VirtualKeyboard>,
}

impl Ui {
    /// The widget whose tag is `tag`.
    pub fn widget(&self, tag: &TagRef) -> Option<&Widget> {
        self.widgets
            .binary_search_by(|widget| widget.tag.cmp(tag))
            .ok()
            .map(|index| &self.widgets[index])
    }

    /// The string list whose tag is `tag`.
    pub fn string_list(&self, tag: &TagRef) -> Option<&StringList> {
        self.string_lists
            .binary_search_by(|list| list.tag.cmp(tag))
            .ok()
            .map(|index| &self.string_lists[index])
    }

    /// The font whose tag is `tag`.
    pub fn font(&self, tag: &TagRef) -> Option<&Font> {
        self.fonts
            .binary_search_by(|font| font.tag.cmp(tag))
            .ok()
            .map(|index| &self.fonts[index])
    }

    /// The collection whose tag is `tag`.
    pub fn collection(&self, tag: &TagRef) -> Option<&WidgetCollection> {
        self.collections
            .binary_search_by(|collection| collection.tag.cmp(tag))
            .ok()
            .map(|index| &self.collections[index])
    }
}

/// What a widget is.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub enum WidgetKind {
    /// It holds its children.
    #[default]
    Container,
    /// A line of text.
    TextBox,
    /// A list that shows one item, turned with left and right.
    SpinnerList,
    /// A list of items, one above another.
    ColumnList,
    /// A model drawn in the UI (unused by the game).
    GameModel,
    /// A movie (unused by the game).
    Movie,
    /// Drawn by code of its own.
    Custom,
}

impl WidgetKind {
    /// The kind a tag's index names; one past the last is a container.
    pub fn from_index(index: i16) -> Self {
        match index {
            1 => Self::TextBox,
            2 => Self::SpinnerList,
            3 => Self::ColumnList,
            4 => Self::GameModel,
            5 => Self::Movie,
            6 => Self::Custom,
            _ => Self::Container,
        }
    }
}

/// A widget definition (`DeLa`): its place, its look, its text, its list
/// and what it does with each event.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Widget {
    /// Its tag.
    pub tag: TagRef,
    /// What it is.
    pub kind: WidgetKind,
    /// The controller it listens to (0 to 3 one pad, 4 any).
    pub controller: i16,
    /// Its name.
    pub name: String,
    /// Its bounds, in the 640×480 screen: top, left, bottom, right.
    pub bounds: [i16; 4],
    /// Its flags ([`Widget::PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD`] and
    /// the rest).
    pub flags: u32,
    /// After how many milliseconds it closes itself (0: never).
    pub auto_close_ms: i32,
    /// How many milliseconds it fades out over when it does.
    pub auto_close_fade_ms: i32,
    /// The bitmap drawn behind it.
    pub background: Option<TagRef>,
    /// The functions that give it its data (its list's items, its text).
    pub game_data_inputs: Vec<i16>,
    /// What it does with each event.
    pub handlers: Vec<EventHandler>,
    /// The strings in its text replaced by functions' values.
    pub replacements: Vec<TextReplacement>,
    /// Its text.
    pub text: WidgetText,
    /// Its list's header, footer and flags.
    pub list: WidgetList,
    /// The widget that describes its focused item at more length.
    pub extended_description: Option<TagRef>,
    /// The widgets put in its place when a condition holds.
    pub conditionals: Vec<ConditionalWidget>,
    /// Its children, in order.
    pub children: Vec<ChildWidget>,
}

impl Widget {
    /// The flag "pass unhandled events to focused child".
    pub const PASS_UNHANDLED_EVENTS_TO_FOCUSED_CHILD: u32 = 1 << 0;
    /// The flag "pause game time".
    pub const PAUSE_GAME_TIME: u32 = 1 << 1;
    /// The flag "flash background bitmap".
    pub const FLASH_BACKGROUND_BITMAP: u32 = 1 << 2;
    /// The flag "dpad up down tabs through children".
    pub const DPAD_UP_DOWN_TABS_THRU_CHILDREN: u32 = 1 << 3;
    /// The flag "dpad left right tabs through children".
    pub const DPAD_LEFT_RIGHT_TABS_THRU_CHILDREN: u32 = 1 << 4;
    /// The flag "dpad up down tabs through list items".
    pub const DPAD_UP_DOWN_TABS_THRU_LIST_ITEMS: u32 = 1 << 5;
    /// The flag "dpad left right tabs through list items".
    pub const DPAD_LEFT_RIGHT_TABS_THRU_LIST_ITEMS: u32 = 1 << 6;
    /// The flag "dont focus a specific child widget".
    pub const DONT_FOCUS_A_SPECIFIC_CHILD_WIDGET: u32 = 1 << 7;
    /// The flag "pass unhandled events to all children".
    pub const PASS_UNHANDLED_EVENTS_TO_ALL_CHILDREN: u32 = 1 << 8;
    /// The flag "render regardless of controller index".
    pub const RENDER_REGARDLESS_OF_CONTROLLER_INDEX: u32 = 1 << 9;
    /// The flag "pass handled events to all children".
    pub const PASS_HANDLED_EVENTS_TO_ALL_CHILDREN: u32 = 1 << 10;
    /// The flag "return to main menu if no history".
    pub const RETURN_TO_MAIN_MENU_IF_NO_HISTORY: u32 = 1 << 11;
    /// The flag "always use tag controller index".
    pub const ALWAYS_USE_TAG_CONTROLLER_INDEX: u32 = 1 << 12;
    /// The flag "always use nifty render fx".
    pub const ALWAYS_USE_NIFTY_RENDER_FX: u32 = 1 << 13;
    /// The flag "dont push history".
    pub const DONT_PUSH_HISTORY: u32 = 1 << 14;

    /// Its width and height, in the 640×480 screen.
    pub fn size(&self) -> [i16; 2] {
        [
            self.bounds[3].saturating_sub(self.bounds[1]),
            self.bounds[2].saturating_sub(self.bounds[0]),
        ]
    }

    /// The handlers for `event`, in order.
    pub fn handlers_for(&self, event: i16) -> impl Iterator<Item = &EventHandler> {
        self.handlers
            .iter()
            .filter(move |handler| handler.event == event)
    }
}

/// What a widget does with one event.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct EventHandler {
    /// What it does ([`EventHandler::CLOSE_CURRENT_WIDGET`] and the rest).
    pub flags: u32,
    /// The event ([`EventHandler::A_BUTTON`] and the rest).
    pub event: i16,
    /// The function it runs, if [`EventHandler::RUN_FUNCTION`] is set.
    pub function: i16,
    /// The widget it opens or puts in its place.
    pub widget: Option<TagRef>,
    /// The sound it plays.
    pub sound: Option<TagRef>,
    /// The scenario's script it runs, if
    /// [`EventHandler::RUN_SCENARIO_SCRIPT`] is set.
    pub script: String,
}

impl EventHandler {
    /// The flag "close current widget".
    pub const CLOSE_CURRENT_WIDGET: u32 = 1 << 0;
    /// The flag "close other widget".
    pub const CLOSE_OTHER_WIDGET: u32 = 1 << 1;
    /// The flag "close all widgets".
    pub const CLOSE_ALL_WIDGETS: u32 = 1 << 2;
    /// The flag "open widget".
    pub const OPEN_WIDGET: u32 = 1 << 3;
    /// The flag "reload self".
    pub const RELOAD_SELF: u32 = 1 << 4;
    /// The flag "reload other widget".
    pub const RELOAD_OTHER_WIDGET: u32 = 1 << 5;
    /// The flag "give focus to widget".
    pub const GIVE_FOCUS_TO_WIDGET: u32 = 1 << 6;
    /// The flag "run function".
    pub const RUN_FUNCTION: u32 = 1 << 7;
    /// The flag "replace self with widget".
    pub const REPLACE_SELF_WITH_WIDGET: u32 = 1 << 8;
    /// The flag "go back to previous widget".
    pub const GO_BACK_TO_PREVIOUS_WIDGET: u32 = 1 << 9;
    /// The flag "run scenario script".
    pub const RUN_SCENARIO_SCRIPT: u32 = 1 << 10;
    /// The flag "try to branch on failure".
    pub const TRY_TO_BRANCH_ON_FAILURE: u32 = 1 << 11;

    // the events: the pad's buttons (input.h's order), then its sticks'
    // directions, then the widget's own
    /// The event "a button".
    pub const A_BUTTON: i16 = 0;
    /// The event "b button".
    pub const B_BUTTON: i16 = 1;
    /// The event "x button".
    pub const X_BUTTON: i16 = 2;
    /// The event "y button".
    pub const Y_BUTTON: i16 = 3;
    /// The event "black button".
    pub const BLACK_BUTTON: i16 = 4;
    /// The event "white button".
    pub const WHITE_BUTTON: i16 = 5;
    /// The event "left trigger".
    pub const LEFT_TRIGGER: i16 = 6;
    /// The event "right trigger".
    pub const RIGHT_TRIGGER: i16 = 7;
    /// The event "dpad up".
    pub const DPAD_UP: i16 = 8;
    /// The event "dpad down".
    pub const DPAD_DOWN: i16 = 9;
    /// The event "dpad left".
    pub const DPAD_LEFT: i16 = 10;
    /// The event "dpad right".
    pub const DPAD_RIGHT: i16 = 11;
    /// The event "start button".
    pub const START_BUTTON: i16 = 12;
    /// The event "back button".
    pub const BACK_BUTTON: i16 = 13;
    /// The event "left thumb".
    pub const LEFT_THUMB: i16 = 14;
    /// The event "right thumb".
    pub const RIGHT_THUMB: i16 = 15;
    /// The event "left analog stick up".
    pub const LEFT_ANALOG_STICK_UP: i16 = 16;
    /// The event "left analog stick down".
    pub const LEFT_ANALOG_STICK_DOWN: i16 = 17;
    /// The event "left analog stick left".
    pub const LEFT_ANALOG_STICK_LEFT: i16 = 18;
    /// The event "left analog stick right".
    pub const LEFT_ANALOG_STICK_RIGHT: i16 = 19;
    /// The event "right analog stick up".
    pub const RIGHT_ANALOG_STICK_UP: i16 = 20;
    /// The event "right analog stick down".
    pub const RIGHT_ANALOG_STICK_DOWN: i16 = 21;
    /// The event "right analog stick left".
    pub const RIGHT_ANALOG_STICK_LEFT: i16 = 22;
    /// The event "right analog stick right".
    pub const RIGHT_ANALOG_STICK_RIGHT: i16 = 23;
    /// The event "created".
    pub const CREATED: i16 = 24;
    /// The event "deleted".
    pub const DELETED: i16 = 25;
    // (the rest of the tag's enumeration, as the PC version names it)
    /// The event "get focus".
    pub const GET_FOCUS: i16 = 26;
    /// The event "lose focus".
    pub const LOSE_FOCUS: i16 = 27;
    /// The event "left mouse".
    pub const LEFT_MOUSE: i16 = 28;
    /// The event "middle mouse".
    pub const MIDDLE_MOUSE: i16 = 29;
    /// The event "right mouse".
    pub const RIGHT_MOUSE: i16 = 30;
    /// The event "double click".
    pub const DOUBLE_CLICK: i16 = 31;
    /// The event "custom activation".
    pub const CUSTOM_ACTIVATION: i16 = 32;
    /// The event "post render".
    pub const POST_RENDER: i16 = 33;
}

/// A string in a widget's text replaced by a function's value.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct TextReplacement {
    /// The string replaced.
    pub search: String,
    /// The function whose value replaces it.
    pub function: i16,
}

/// A widget's text.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WidgetText {
    /// The string list its text is from.
    pub string_list: Option<TagRef>,
    /// The font it is drawn in.
    pub font: Option<TagRef>,
    /// Its colour (alpha, red, green, blue).
    pub color: [f32; 4],
    /// How it is aligned: 0 left, 1 right, 2 centred.
    pub justification: i16,
    /// Its flags (bit 0: editable; bit 1: a password; bit 2: flashes; bit 3:
    /// doesn't flash when focused).
    pub flags: u16,
    /// Its string's index in the list.
    pub string_index: i16,
    /// Its offset from the widget's corner, across and down.
    pub offset: [i16; 2],
}

/// A list widget's header, footer and flags.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WidgetList {
    /// Its flags (bit 0: its items are made by code; bit 1: its items
    /// are made from its text's string list, one each; bit 2: one tooltip
    /// entry only; bit 3: one preview box, not scrolled).
    pub flags: u32,
    /// The bitmap above the list.
    pub header: Option<TagRef>,
    /// The bitmap below it.
    pub footer: Option<TagRef>,
    /// The header's bounds: top, left, bottom, right.
    pub header_bounds: [i16; 4],
    /// The footer's.
    pub footer_bounds: [i16; 4],
}

/// A widget put in another's place when its condition holds.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ConditionalWidget {
    /// The widget.
    pub widget: Option<TagRef>,
    /// Its name.
    pub name: String,
    /// Its flags (bit 0: it is loaded when the event handler's function
    /// fails).
    pub flags: u32,
    /// The controller it listens to.
    pub controller: i16,
}

/// A widget's child.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ChildWidget {
    /// The child's definition.
    pub widget: Option<TagRef>,
    /// Its name.
    pub name: String,
    /// Its flags (bit 0: uses the custom controller; bit 1: is the
    /// parent's list's item; bit 2: is its previous-page button; bit 3:
    /// its next-page button).
    pub flags: u32,
    /// The controller it listens to, with the custom controller flag.
    pub controller: i16,
    /// Its offset from its parent, across and down.
    pub offset: [i16; 2],
}

impl ChildWidget {
    /// The flag "use custom controller index".
    pub const USE_CUSTOM_CONTROLLER_INDEX: u32 = 1 << 0;
    /// The flag "is list item".
    pub const IS_LIST_ITEM: u32 = 1 << 1;
    /// The flag "previous page button".
    pub const PREVIOUS_PAGE_BUTTON: u32 = 1 << 2;
    /// The flag "next page button".
    pub const NEXT_PAGE_BUTTON: u32 = 1 << 3;
}

/// A collection of widgets (`Soul`): the shell's screens.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct WidgetCollection {
    /// Its tag.
    pub tag: TagRef,
    /// Its widgets, in order.
    pub widgets: Vec<Option<TagRef>>,
}

/// A list of strings (`ustr`, or `str#` widened from 8 bits).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct StringList {
    /// Its tag.
    pub tag: TagRef,
    /// Its strings, without their ends.
    pub strings: Vec<String>,
}

/// A colour table (`colo`).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ColorTable {
    /// Its tag.
    pub tag: TagRef,
    /// Its colours, by name (alpha, red, green, blue).
    pub colors: Vec<(String, [f32; 4])>,
}

/// A font (`font`): its metrics and its glyphs, each with a distance field
/// built from its coverage.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Font {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags.
    pub flags: u32,
    /// How far its glyphs rise above the baseline, in pixels.
    pub ascent: i16,
    /// How far they fall below it.
    pub descent: i16,
    /// The space between lines.
    pub leading_height: i16,
    /// The space between characters.
    pub leading_width: i16,
    /// Its bold, italic, condensed and underlined fonts.
    pub styles: [Option<TagRef>; 4],
    /// Its glyphs.
    pub glyphs: Vec<Glyph>,
    /// Each character's glyph, sorted by character.
    pub characters: Vec<(u16, u32)>,
}

impl Font {
    /// The texels of empty space each glyph's distance field has around
    /// its coverage.
    pub const PADDING: i16 = 4;
    /// The distance, in texels, at which a field reaches 0 or 255.
    pub const RADIUS: f32 = 4.0;

    /// The glyph of `character`.
    pub fn glyph(&self, character: u16) -> Option<&Glyph> {
        let index = self
            .characters
            .binary_search_by_key(&character, |&(code, _)| code)
            .ok()?;
        self.glyphs.get(self.characters[index].1 as usize)
    }

    /// The height of a line: its ascent, its descent and its leading.
    pub fn line_height(&self) -> i16 {
        self.ascent
            .saturating_add(self.descent)
            .saturating_add(self.leading_height)
    }

    /// How far a line of `text` reaches, in pixels: the sum of its glyphs'
    /// advances (a font tag has no kerning pairs; a character it has no
    /// glyph for advances nothing).
    pub fn width(&self, text: &str) -> i32 {
        text.encode_utf16()
            .filter_map(|character| self.glyph(character))
            .map(|glyph| i32::from(glyph.advance) + i32::from(self.leading_width))
            .sum()
    }
}

/// A font's glyph.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Glyph {
    /// Its character.
    pub character: u16,
    /// How far the pen moves past it.
    pub advance: i16,
    /// Its coverage's width and height, in pixels.
    pub size: [i16; 2],
    /// The pen's place in its coverage, across and down from its top left.
    pub origin: [i16; 2],
    /// Its signed distance field, (width + 2 × [`Font::PADDING`]) by
    /// (height + 2 × padding), row by row: 128 on the edge, more inside,
    /// each step of 127 / [`Font::RADIUS`] a texel.
    pub distance: Vec<u8>,
}

impl Glyph {
    /// Its distance field's width and height.
    pub fn field_size(&self) -> [usize; 2] {
        let padded =
            |extent: i16| usize::try_from(extent.max(0) + 2 * Font::PADDING).unwrap_or_default();
        [padded(self.size[0]), padded(self.size[1])]
    }
}

/// A virtual keyboard (`vcky`).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct VirtualKeyboard {
    /// Its tag.
    pub tag: TagRef,
    /// Its keys' font.
    pub font: Option<TagRef>,
    /// The bitmap behind it.
    pub background: Option<TagRef>,
    /// The labels of its special keys (`ustr`).
    pub special_key_labels: Option<TagRef>,
    /// Its keys, in their places' order.
    pub keys: Vec<VirtualKey>,
}

/// A virtual keyboard's key.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct VirtualKey {
    /// What it does (a character, or one of the special keys).
    pub keycode: i16,
    /// Its characters: plain, shifted, caps, symbols, shifted caps,
    /// shifted symbols, caps symbols.
    pub characters: [u16; 7],
    /// Its bitmaps: unselected, selected, active and sticky.
    pub bitmaps: [Option<TagRef>; 4],
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Group;

    fn glyph(character: u8, advance: i16, size: [i16; 2]) -> Glyph {
        Glyph {
            character: u16::from(character),
            advance,
            size,
            origin: [0, size[1]],
            distance: Vec::new(),
        }
    }

    /// A font of 'A' (advance 7), 'i' (advance 3) and a space (advance 4,
    /// empty), with 'a' sent to 'A'.
    fn font() -> Font {
        Font {
            tag: TagRef {
                group: Group(*b"font"),
                path: "ui\\fonts\\small".into(),
            },
            flags: 0,
            ascent: 12,
            descent: 4,
            leading_height: 2,
            leading_width: 1,
            styles: [None, None, None, None],
            glyphs: vec![
                glyph(b'A', 7, [6, 9]),
                glyph(b'i', 3, [2, 9]),
                glyph(b' ', 4, [0, 0]),
            ],
            characters: vec![
                (u16::from(b' '), 2),
                (u16::from(b'A'), 0),
                (u16::from(b'a'), 0),
                (u16::from(b'i'), 1),
            ],
        }
    }

    #[test]
    fn each_character_finds_its_glyph() {
        let font = font();
        let advance = |character: u8| font.glyph(u16::from(character)).map(|glyph| glyph.advance);
        assert_eq!(advance(b'A'), Some(7));
        assert_eq!(advance(b'a'), Some(7), "the table's 'a'");
        assert_eq!(advance(b'i'), Some(3));
        assert_eq!(advance(b'z'), None);
    }

    /// A line is its glyphs' advances, each with the font's leading width:
    /// a font tag has no kerning pairs, so a pair is spaced as its glyphs
    /// are alone.
    #[test]
    fn a_line_is_its_advances_without_kerning() {
        let font = font();
        assert_eq!(font.width(""), 0);
        assert_eq!(font.width("A"), 8);
        assert_eq!(font.width("Ai"), 8 + 4);
        assert_eq!(font.width("iA"), font.width("Ai"), "no pair is kerned");
        assert_eq!(font.width("A i"), 8 + 5 + 4);
        assert_eq!(
            font.width("Az"),
            8,
            "a character with no glyph advances nothing"
        );
        assert_eq!(font.line_height(), 18);
    }

    #[test]
    fn a_distance_field_is_padded_around_its_glyph() {
        let padding = usize::try_from(Font::PADDING).unwrap_or_default();
        assert_eq!(
            glyph(b'A', 7, [6, 9]).field_size(),
            [6 + 2 * padding, 9 + 2 * padding]
        );
        assert_eq!(
            glyph(b' ', 4, [0, 0]).field_size(),
            [2 * padding, 2 * padding]
        );
        assert_eq!(
            glyph(b' ', 4, [-2, 0]).field_size(),
            [2 * padding, 2 * padding]
        );
    }

    #[test]
    fn the_ui_finds_its_tags() {
        let font = font();
        let tag = font.tag.clone();
        let ui = Ui {
            fonts: vec![font],
            ..Ui::default()
        };
        assert!(ui.font(&tag).is_some());
        let missing = TagRef {
            group: Group(*b"font"),
            path: "ui\\fonts\\large".into(),
        };
        assert!(ui.font(&missing).is_none());
        assert!(ui.widget(&tag).is_none());
        assert!(ui.string_list(&tag).is_none());
        assert!(ui.collection(&tag).is_none());
    }
}
