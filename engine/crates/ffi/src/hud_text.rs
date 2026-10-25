//! The HUD's state message, as `hud_messaging.c` draws it: the message the
//! action button's prompt names, its pieces of text, its controls written
//! as the keys they are bound to, and its own pictures (the vehicle's and
//! the seat's names, or the weapon's picture), set left to right in the
//! messaging font from the messaging point.
//!
//! The text is glyphs of the UI's atlas, which the app draws with the
//! menus' pass; a picture is a quad of the messaging icons, drawn with the
//! HUD's.

use opence_content::hud::{HudIcon, HudMessageElement};
use opence_content::{Font, Level, TagRef};
use opence_sim::hud::ActionPrompt;

use crate::hud::{HudQuad, Painter};
use crate::ui::{GlyphAtlas, UiQuad};

/// The state message, drawn: its glyphs and its pictures.
#[derive(Clone, Debug, Default, PartialEq, uniffi::Record)]
pub struct HudText {
    /// Its text, as glyphs of the UI's atlas.
    pub glyphs: Vec<UiQuad>,
    /// Its pictures, as quads of the HUD's bitmaps.
    pub icons: Vec<HudQuad>,
}

/// The messages' indices (`_hud_message_*`, in the HUD globals' `hmt `).
const SWAP_WEAPON: usize = 4;
const REMIND_TO_SWITCH: usize = 5;
const ENTER_VEHICLE: usize = 6;
const FLIP_VEHICLE: usize = 8;

/// The first of a message's controls (`_icon_action`), and the first of its
/// own pictures (`_icon_custom_1`).
const FIRST_CONTROL: u8 = 18;
const FIRST_CUSTOM: u8 = 32;

/// How far back the next piece starts from where a piece ends
/// (`line_cursor.x0 -= 3`).
const PIECE_OVERLAP: f32 = 3.0;

/// One of a message's own pictures (`hud_state_message_info`).
#[derive(Clone, Debug, PartialEq)]
enum Custom {
    /// A string of the alternate icon text, or none (`<unknown>`).
    Text(i16),
    /// A picture of the messaging icons.
    Icon(HudIcon),
}

/// The message `prompt` shows and its own pictures, as `hud.c` fills them
/// (`hud_show_action_response`).
fn message(level: &Level, prompt: &ActionPrompt) -> (usize, Vec<Custom>) {
    let icon_text = |tag: &TagRef| {
        level
            .object(tag)
            .map_or(-1, |object| object.icon_text_index)
    };
    match prompt {
        ActionPrompt::EnterVehicle { vehicle, seat } => {
            let seat = level
                .object(vehicle)
                .and_then(|object| object.vehicle.as_ref())
                .and_then(|vehicle| vehicle.seats.get(*seat))
                .map_or(-1, |seat| seat.icon_text_index);
            (
                ENTER_VEHICLE,
                vec![Custom::Text(seat), Custom::Text(icon_text(vehicle))],
            )
        }
        ActionPrompt::FlipVehicle { vehicle } => {
            (FLIP_VEHICLE, vec![Custom::Text(icon_text(vehicle))])
        }
        ActionPrompt::SwapWeapon { weapon } => (SWAP_WEAPON, vec![weapon_picture(level, weapon)]),
        ActionPrompt::RemindToSwitch { weapon } => {
            (REMIND_TO_SWITCH, vec![weapon_picture(level, weapon)])
        }
    }
}

/// A weapon's picture in a message: its HUD's messaging icon, or its name.
fn weapon_picture(level: &Level, weapon: &TagRef) -> Custom {
    let object = level.object(weapon);
    let icon = object
        .and_then(|object| object.weapon.as_ref())
        .and_then(|weapon| weapon.hud_interface.as_ref())
        .and_then(|tag| level.weapon_huds.iter().find(|hud| hud.tag == *tag))
        .and_then(|hud| hud.messaging_icon.clone());
    match icon {
        Some(icon) => Custom::Icon(icon),
        None => Custom::Text(object.map_or(-1, |object| object.icon_text_index)),
    }
}

/// A piece of a state message, resolved: text to write, or a picture.
#[derive(Clone, Debug, PartialEq)]
enum Piece<'a> {
    Text(&'a str),
    Icon(&'a HudIcon),
}

/// The pieces of `prompt`'s message, its controls written as `keys` (the
/// keys of action, throw grenade, fire, flashlight, jump, equipment, swap
/// weapons, swap grenades, crouch, zoom, accept, back, move and look, in
/// that order): none if the level has no messages.
fn pieces<'a>(
    level: &'a Level,
    customs: &'a [Custom],
    index: usize,
    keys: &'a [String],
) -> Vec<Piece<'a>> {
    let Some(messaging) = level.hud_globals.as_ref().map(|globals| &globals.messaging) else {
        return Vec::new();
    };
    let Some(message) = messaging.messages.get(index) else {
        return Vec::new();
    };
    let strings = messaging
        .alternate_icon_text
        .as_ref()
        .and_then(|tag| level.ui.string_list(tag));
    message
        .elements
        .iter()
        .filter_map(|element| match element {
            HudMessageElement::Text(text) => Some(Piece::Text(text)),
            &HudMessageElement::Icon(icon) if (FIRST_CONTROL..FIRST_CUSTOM).contains(&icon) => {
                let key = keys.get(usize::from(icon - FIRST_CONTROL));
                Some(Piece::Text(key.map_or("?", String::as_str)))
            }
            &HudMessageElement::Icon(icon) if icon >= FIRST_CUSTOM => {
                // (past the prompt's own: not drawn, as the original)
                match customs.get(usize::from(icon - FIRST_CUSTOM))? {
                    Custom::Text(index) => {
                        let text = usize::try_from(*index)
                            .ok()
                            .and_then(|index| strings?.strings.get(index));
                        Some(Piece::Text(text.map_or("<unknown>", String::as_str)))
                    }
                    Custom::Icon(icon) => Some(Piece::Icon(icon)),
                }
            }
            // (a button's own picture: there are no pads here)
            HudMessageElement::Icon(_) => None,
        })
        .collect()
}

/// What `prompt`'s message says, its controls written as `keys` (see
/// [`prompt_text`]) and a picture as `[picture]`: none if the level has
/// no such message.
pub fn prompt_words(level: &Level, prompt: &ActionPrompt, keys: &[String]) -> Option<String> {
    let (index, customs) = message(level, prompt);
    let pieces = pieces(level, &customs, index, keys);
    (!pieces.is_empty()).then(|| {
        pieces
            .iter()
            .map(|piece| match piece {
                Piece::Text(text) => *text,
                Piece::Icon(_) => "[picture]",
            })
            .collect()
    })
}

/// The state message of `prompt`, drawn on a window `width` by `height`
/// pixels at `seconds` into the game, its glyphs those of a glyph atlas of
/// `level`'s fonts built for it (see [`prompt_text`]).
pub fn prompt_text_of(
    level: &Level,
    prompt: &ActionPrompt,
    keys: &[String],
    size: [u32; 2],
    seconds: f32,
) -> HudText {
    prompt_text(
        level,
        &GlyphAtlas::build(level),
        prompt,
        keys,
        size,
        seconds,
    )
}

/// The state message of `prompt`, drawn on a window `width` by `height`
/// pixels at `seconds` into the game, its controls written as `keys` (the
/// keys of action, throw grenade, fire, flashlight, jump, equipment, swap
/// weapons, swap grenades, crouch, zoom, accept, back, move and look, in
/// that order), its glyphs placed as `atlas` has them: nothing if the
/// level has no messages or no font for them.
pub(crate) fn prompt_text(
    level: &Level,
    atlas: &GlyphAtlas,
    prompt: &ActionPrompt,
    keys: &[String],
    [width, height]: [u32; 2],
    seconds: f32,
) -> HudText {
    let Some(messaging) = level.hud_globals.as_ref().map(|globals| &globals.messaging) else {
        return HudText::default();
    };
    let Some(font) = messaging.font.as_ref().and_then(|tag| level.ui.font(tag)) else {
        return HudText::default();
    };
    let Some(mut painter) = Painter::new(level, width, height, seconds) else {
        return HudText::default();
    };
    let (index, customs) = message(level, prompt);
    let [x, top] = painter.point(messaging.anchor, &messaging.placement);
    let mut line = Line {
        font,
        atlas,
        scale: painter.scale,
        color: messaging.color,
        pen: x + f32::from(font.leading_width),
        baseline: top + f32::from(font.ascent),
        glyphs: Vec::new(),
    };
    for piece in pieces(level, &customs, index, keys) {
        match piece {
            Piece::Text(text) => line.write(text),
            Piece::Icon(icon) => {
                if let Some(icons) = messaging.icons.as_ref() {
                    let bottom = line.baseline + f32::from(font.descent);
                    line.pen += painter.icon(icons, icon, [line.pen, bottom], messaging.color);
                }
            }
        }
    }
    HudText {
        glyphs: line.glyphs,
        icons: painter.quads,
    }
}

/// A line of a state message being set, in the HUD's pixels.
struct Line<'a> {
    font: &'a Font,
    atlas: &'a GlyphAtlas,
    /// The window's pixels to the HUD's.
    scale: f32,
    color: u32,
    /// Where the next glyph's pen starts.
    pen: f32,
    baseline: f32,
    glyphs: Vec<UiQuad>,
}

impl Line<'_> {
    /// Sets `text` from the pen, then steps the pen back for the next piece.
    fn write(&mut self, text: &str) {
        let padding = f32::from(Font::PADDING);
        for character in text.encode_utf16() {
            let Some((index, glyph)) = self
                .font
                .characters
                .binary_search_by_key(&character, |&(code, _)| code)
                .ok()
                .and_then(|at| usize::try_from(self.font.characters[at].1).ok())
                .and_then(|index| Some((index, self.font.glyphs.get(index)?)))
            else {
                continue;
            };
            if glyph.size[0] > 0 && glyph.size[1] > 0 {
                if let Some([u0, v0, u1, v1]) = self.atlas.place(&self.font.tag, index) {
                    let [w, h] = glyph.field_size().map(|extent| extent as f32);
                    let left = self.pen - f32::from(glyph.origin[0]) - padding;
                    let top = self.baseline - f32::from(glyph.origin[1]) - padding;
                    self.glyphs.push(UiQuad {
                        texture: u32::MAX,
                        glyph: true,
                        left: left * self.scale,
                        top: top * self.scale,
                        right: (left + w) * self.scale,
                        bottom: (top + h) * self.scale,
                        u0,
                        v0,
                        u1,
                        v1,
                        color: self.color,
                    });
                }
            }
            self.pen += f32::from(glyph.advance);
        }
        self.pen -= PIECE_OVERLAP;
    }
}
