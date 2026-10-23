//! The UI's tags: every widget definition (`DeLa`), widget collection
//! (`Soul`), string list (`ustr`, `str#`), font (`font`), colour table
//! (`colo`) and virtual keyboard (`vcky`) a cache has. `ui.map` has the
//! shell's; every other map has the pause menu's and the dialogs'.
//!
//! The bitmaps they draw are reached, to import with the level's. Each
//! font's glyphs get a signed distance field built from their coverage,
//! so that the UI's text can be drawn sharp at any size.

use std::collections::BTreeMap;

use opence_content::{
    ChildWidget, ColorTable, ConditionalWidget, EventHandler, Font, Glyph, StringList,
    TextReplacement, Ui, VirtualKey, VirtualKeyboard, Widget, WidgetCollection, WidgetKind,
    WidgetList, WidgetText,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_ref, Reached};
use crate::tags::{Element, GroupTag};

const DELA: GroupTag = GroupTag::new(b"DeLa");
const SOUL: GroupTag = GroupTag::new(b"Soul");
const USTR: GroupTag = GroupTag::new(b"ustr");
const STR8: GroupTag = GroupTag::new(b"str#");
const FONT: GroupTag = GroupTag::new(b"font");
const COLO: GroupTag = GroupTag::new(b"colo");
const VCKY: GroupTag = GroupTag::new(b"vcky");
const SND: GroupTag = GroupTag::new(b"snd!");

const WIDGET_SIZE: usize = 0x3EC;
const HANDLER_SIZE: usize = 0x48;
const REPLACEMENT_SIZE: usize = 0x22;
const GAME_DATA_INPUT_SIZE: usize = 0x24;
const CHILD_SIZE: usize = 0x50;
const CONDITIONAL_SIZE: usize = 0x50;
const COLLECTION_SIZE: usize = 0x0C;
const COLLECTION_ELEMENT_SIZE: usize = 0x10;
const STRING_LIST_SIZE: usize = 0x0C;
const STRING_SIZE: usize = 0x14;
const FONT_SIZE: usize = 0x9C;
const CHARACTER_TABLE_SIZE: usize = 0x0C;
const CHARACTER_SIZE: usize = 0x14;
const COLOR_TABLE_SIZE: usize = 0x0C;
const COLOR_SIZE: usize = 0x30;
const KEYBOARD_SIZE: usize = 0x3C;
const KEY_SIZE: usize = 0x50;

/// The most elements each of a widget's blocks may have (the tag
/// definitions' maxima).
const MAX_HANDLERS: usize = 32;
const MAX_REPLACEMENTS: usize = 32;
const MAX_GAME_DATA_INPUTS: usize = 64;
const MAX_CONDITIONALS: usize = 32;
const MAX_CHILDREN: usize = 32;
const MAX_COLLECTION_WIDGETS: usize = 32;
const MAX_STRINGS: usize = 32_767;
const MAX_COLORS: usize = 512;
const MAX_KEYS: usize = 64;
/// A string's most bytes: `SHORT_MAX` characters, of two bytes in a
/// `ustr`.
const MAX_STRING_BYTES: usize = 2 * 32_767;
/// A font's most tables, characters and bytes of coverage.
const MAX_CHARACTER_TABLES: usize = 256;
const MAX_CHARACTERS: usize = 32_767;
const MAX_FONT_PIXELS: usize = 16 << 20;
/// A glyph's largest width or height, in pixels.
const MAX_GLYPH_EXTENT: i16 = 256;

/// The UI tags of `cache`, sorted by tag, with the bitmaps they draw
/// added to `reached`. Tags in Custom Edition's resource maps (its
/// `loc.map` fonts and strings) are left out.
pub(crate) fn import_ui(cache: &Cache, reached: &mut Reached) -> Result<Ui, ImportError> {
    let mut ui = Ui::default();
    for (index, instance) in cache.tags().iter().enumerate() {
        if instance.in_resource_map {
            continue;
        }
        if instance.group == DELA {
            ui.widgets.push(import_widget(cache, index, reached)?);
        } else if instance.group == SOUL {
            ui.collections.push(import_collection(cache, index)?);
        } else if instance.group == USTR {
            ui.string_lists
                .push(import_string_list(cache, index, true)?);
        } else if instance.group == STR8 {
            ui.string_lists
                .push(import_string_list(cache, index, false)?);
        } else if instance.group == FONT {
            ui.fonts.push(import_font(cache, index)?);
        } else if instance.group == COLO {
            ui.color_tables.push(import_color_table(cache, index)?);
        } else if instance.group == VCKY {
            ui.keyboards.push(import_keyboard(cache, index, reached)?);
        }
    }
    ui.widgets.sort_by(|a, b| a.tag.cmp(&b.tag));
    ui.collections.sort_by(|a, b| a.tag.cmp(&b.tag));
    ui.string_lists.sort_by(|a, b| a.tag.cmp(&b.tag));
    ui.fonts.sort_by(|a, b| a.tag.cmp(&b.tag));
    ui.color_tables.sort_by(|a, b| a.tag.cmp(&b.tag));
    ui.keyboards.sort_by(|a, b| a.tag.cmp(&b.tag));
    Ok(ui)
}

/// The UI tags of `cache` alone, as [`crate::import_level`] imports them
/// (for fuzzing).
pub fn import_ui_tags(cache: &Cache) -> Result<Ui, ImportError> {
    import_ui(cache, &mut Reached::default())
}

fn reference(
    element: &Element<'_>,
    at: usize,
    field: &str,
    groups: &[GroupTag],
) -> Result<Option<opence_content::TagRef>, ImportError> {
    Ok(element
        .reference(at, field, groups)?
        .map(|index| tag_ref(element.cache(), index)))
}

fn rectangle(element: &Element<'_>, at: usize, field: &str) -> Result<[i16; 4], ImportError> {
    Ok([
        element.i16(at, field)?,
        element.i16(at + 2, field)?,
        element.i16(at + 4, field)?,
        element.i16(at + 6, field)?,
    ])
}

fn color(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 4], ImportError> {
    Ok([
        element.f32(at, field)?,
        element.f32(at + 4, field)?,
        element.f32(at + 8, field)?,
        element.f32(at + 12, field)?,
    ])
}

fn import_widget(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<Widget, ImportError> {
    let widget = cache.tag(index, DELA, WIDGET_SIZE)?;
    let game_data_inputs = widget
        .block(
            0x48,
            "game data inputs",
            GAME_DATA_INPUT_SIZE,
            MAX_GAME_DATA_INPUTS,
        )?
        .iter()
        .map(|input| input.i16(0, "function"))
        .collect::<Result<_, _>>()?;
    let handlers = widget
        .block(0x54, "event handlers", HANDLER_SIZE, MAX_HANDLERS)?
        .iter()
        .map(|handler| {
            Ok(EventHandler {
                flags: handler.u32(0, "flags")?,
                event: handler.i16(4, "event type")?,
                function: handler.i16(6, "function")?,
                widget: reference(&handler, 8, "widget tag", &[DELA])?,
                sound: reference(&handler, 0x18, "sound effect", &[SND])?,
                script: handler.text(0x28, "script", 31)?.to_owned(),
            })
        })
        .collect::<Result<_, ImportError>>()?;
    let replacements = widget
        .block(
            0x60,
            "search and replace functions",
            REPLACEMENT_SIZE,
            MAX_REPLACEMENTS,
        )?
        .iter()
        .map(|replacement| {
            Ok(TextReplacement {
                search: replacement.text(0, "search string", 31)?.to_owned(),
                function: replacement.i16(0x20, "replace function")?,
            })
        })
        .collect::<Result<_, ImportError>>()?;
    let conditionals = widget
        .block(
            0x2D4,
            "conditional widgets",
            CONDITIONAL_SIZE,
            MAX_CONDITIONALS,
        )?
        .iter()
        .map(|conditional| {
            Ok(ConditionalWidget {
                widget: reference(&conditional, 0, "widget tag", &[DELA])?,
                name: conditional.text(0x10, "name", 31)?.to_owned(),
                flags: conditional.u32(0x30, "flags")?,
                controller: conditional.i16(0x34, "custom controller index")?,
            })
        })
        .collect::<Result<_, ImportError>>()?;
    let children = widget
        .block(0x3E0, "child widgets", CHILD_SIZE, MAX_CHILDREN)?
        .iter()
        .map(|child| {
            Ok(ChildWidget {
                widget: reference(&child, 0, "widget tag", &[DELA])?,
                name: child.text(0x10, "name", 31)?.to_owned(),
                flags: child.u32(0x30, "flags")?,
                controller: child.i16(0x34, "custom controller index")?,
                offset: [
                    child.i16(0x38, "horizontal offset")?,
                    child.i16(0x36, "vertical offset")?,
                ],
            })
        })
        .collect::<Result<_, ImportError>>()?;
    Ok(Widget {
        tag: tag_ref(cache, index),
        kind: WidgetKind::from_index(widget.i16(0, "widget type")?),
        controller: widget.i16(2, "controller index")?,
        name: widget.text(4, "name", 31)?.to_owned(),
        bounds: rectangle(&widget, 0x24, "bounds")?,
        flags: widget.u32(0x2C, "flags")?,
        auto_close_ms: widget.i32(0x30, "milliseconds to auto close")?,
        auto_close_fade_ms: widget.i32(0x34, "milliseconds auto close fade time")?,
        background: reached.bitmap(&widget, 0x38, "background bitmap")?,
        game_data_inputs,
        handlers,
        replacements,
        text: WidgetText {
            string_list: reference(&widget, 0xEC, "text label unicode strings list", &[USTR])?,
            font: reference(&widget, 0xFC, "text font", &[FONT])?,
            color: color(&widget, 0x10C, "text color")?,
            justification: widget.i16(0x11C, "justification")?,
            flags: widget.u16(0x11E, "text box flags")?,
            string_index: widget.i16(0x12E, "string list index")?,
            offset: [
                widget.i16(0x130, "horizontal offset")?,
                widget.i16(0x132, "vertical offset")?,
            ],
        },
        list: WidgetList {
            flags: widget.u32(0x150, "list flags")?,
            header: reached.bitmap(&widget, 0x154, "list header bitmap")?,
            footer: reached.bitmap(&widget, 0x164, "list footer bitmap")?,
            header_bounds: rectangle(&widget, 0x174, "header bounds")?,
            footer_bounds: rectangle(&widget, 0x17C, "footer bounds")?,
        },
        extended_description: reference(&widget, 0x1A4, "extended description widget", &[DELA])?,
        conditionals,
        children,
    })
}

fn import_collection(cache: &Cache, index: usize) -> Result<WidgetCollection, ImportError> {
    let collection = cache.tag(index, SOUL, COLLECTION_SIZE)?;
    let widgets = collection
        .block(
            0,
            "ui widget definitions",
            COLLECTION_ELEMENT_SIZE,
            MAX_COLLECTION_WIDGETS,
        )?
        .iter()
        .map(|widget| reference(&widget, 0, "ui widget definition", &[DELA]))
        .collect::<Result<_, _>>()?;
    Ok(WidgetCollection {
        tag: tag_ref(cache, index),
        widgets,
    })
}

/// A string list: UTF-16 (`ustr`) or 8-bit (`str#`), each string without
/// the end it is stored with.
fn import_string_list(cache: &Cache, index: usize, wide: bool) -> Result<StringList, ImportError> {
    let list = cache.tag(index, if wide { USTR } else { STR8 }, STRING_LIST_SIZE)?;
    let strings = list
        .block(0, "string references", STRING_SIZE, MAX_STRINGS)?
        .iter()
        .map(|string| {
            let longest = if wide {
                MAX_STRING_BYTES
            } else {
                MAX_STRING_BYTES / 2
            };
            let bytes = string.data(0, "string", longest)?;
            Ok(if wide {
                let units: Vec<u16> = bytes
                    .as_chunks::<2>()
                    .0
                    .iter()
                    .map(|pair| u16::from_le_bytes(*pair))
                    .collect();
                let end = units
                    .iter()
                    .position(|&unit| unit == 0)
                    .unwrap_or(units.len());
                String::from_utf16_lossy(&units[..end])
            } else {
                let end = bytes
                    .iter()
                    .position(|&byte| byte == 0)
                    .unwrap_or(bytes.len());
                bytes[..end].iter().map(|&byte| char::from(byte)).collect()
            })
        })
        .collect::<Result<_, ImportError>>()?;
    Ok(StringList {
        tag: tag_ref(cache, index),
        strings,
    })
}

fn import_font(cache: &Cache, index: usize) -> Result<Font, ImportError> {
    let font = cache.tag(index, FONT, FONT_SIZE)?;
    let pixels = font.data(0x88, "pixels", MAX_FONT_PIXELS)?;
    let characters = font.block(0x7C, "characters", CHARACTER_SIZE, MAX_CHARACTERS)?;
    let mut glyphs = Vec::with_capacity(characters.len());
    for character in characters.iter() {
        let extent = |at: usize, field: &str| {
            // (the shell's fonts give their spaces a width of -2 and a height
            // of 0: a negative extent is an empty glyph)
            let value = character.i16(at, field)?;
            if value <= MAX_GLYPH_EXTENT {
                Ok(value.max(0))
            } else {
                Err(character.fault(field, format!("is {value}, above {MAX_GLYPH_EXTENT}")))
            }
        };
        let size = [extent(4, "bitmap width")?, extent(6, "bitmap height")?];
        let offset = character.i32(0x10, "pixels offset")?;
        let area = usize::try_from(size[0]).unwrap_or_default()
            * usize::try_from(size[1]).unwrap_or_default();
        let coverage = usize::try_from(offset)
            .ok()
            .and_then(|start| pixels.get(start..start.checked_add(area)?))
            .ok_or_else(|| {
                character.fault(
                    "pixels offset",
                    format!(
                        "is {offset}, and its {area} pixels are not in the font's {}",
                        pixels.len()
                    ),
                )
            })?;
        glyphs.push(Glyph {
            character: character.u16(0, "character")?,
            advance: character.i16(2, "character width")?,
            size,
            origin: [
                character.i16(8, "bitmap origin x")?,
                character.i16(0xA, "bitmap origin y")?,
            ],
            distance: distance_field(coverage, size),
        });
    }

    // each glyph is its own character's, unless the font's tables (one of
    // 256 for each high byte) say otherwise
    let mut map: BTreeMap<u16, u32> = BTreeMap::new();
    for (glyph_index, glyph) in glyphs.iter().enumerate().rev() {
        map.insert(glyph.character, glyph_index as u32);
    }
    let tables = font.block(
        0x30,
        "character tables",
        CHARACTER_TABLE_SIZE,
        MAX_CHARACTER_TABLES,
    )?;
    for (high, table) in tables.iter().enumerate() {
        let indices = table.block(0, "character indices", 2, 256)?;
        if indices.len() != 256 {
            continue;
        }
        for (low, entry) in indices.iter().enumerate() {
            let glyph_index = entry.i16(0, "character index")?;
            let code = (high << 8 | low) as u16;
            match usize::try_from(glyph_index) {
                Ok(glyph_index) if glyph_index < glyphs.len() => {
                    map.insert(code, glyph_index as u32);
                }
                Ok(glyph_index) => {
                    return Err(entry.fault(
                        "character index",
                        format!(
                            "is {glyph_index}, past the font's {} characters",
                            glyphs.len()
                        ),
                    ))
                }
                Err(_) => {
                    map.remove(&code);
                }
            }
        }
    }

    Ok(Font {
        tag: tag_ref(cache, index),
        flags: font.u32(0, "flags")?,
        ascent: font.i16(4, "ascending height")?,
        descent: font.i16(6, "descending height")?,
        leading_height: font.i16(8, "leading height")?,
        leading_width: font.i16(0xA, "leading width")?,
        styles: [
            reference(&font, 0x3C, "bold", &[FONT])?,
            reference(&font, 0x4C, "italic", &[FONT])?,
            reference(&font, 0x5C, "condense", &[FONT])?,
            reference(&font, 0x6C, "underline", &[FONT])?,
        ],
        glyphs,
        characters: map.into_iter().collect(),
    })
}

/// A glyph's signed distance field: for each texel of its coverage padded
/// by [`Font::PADDING`], the distance to the nearest texel on the other
/// side of its edge (coverage 128), out to [`Font::RADIUS`], as
/// 128 + distance × 127 / radius, more inside.
pub(crate) fn distance_field(coverage: &[u8], size: [i16; 2]) -> Vec<u8> {
    let width = usize::try_from(size[0]).unwrap_or_default();
    let height = usize::try_from(size[1]).unwrap_or_default();
    let padding = Font::PADDING as usize;
    let field_width = width + 2 * padding;
    let field_height = height + 2 * padding;
    let inside = |x: isize, y: isize| -> bool {
        let (x, y) = (x - padding as isize, y - padding as isize);
        if x < 0 || y < 0 || x as usize >= width || y as usize >= height {
            return false;
        }
        coverage[y as usize * width + x as usize] >= 128
    };
    // (far enough to see the radius in every direction)
    let reach = Font::RADIUS as isize + 1;
    let mut field = Vec::with_capacity(field_width * field_height);
    for y in 0..field_height as isize {
        for x in 0..field_width as isize {
            let here = inside(x, y);
            let mut nearest = f32::MAX;
            for dy in -reach..=reach {
                for dx in -reach..=reach {
                    if inside(x + dx, y + dy) != here {
                        nearest = nearest.min(((dx * dx + dy * dy) as f32).sqrt());
                    }
                }
            }
            // (the edge is half a texel from the texel across it)
            let distance = (nearest - 0.5).min(Font::RADIUS);
            let signed = if here { distance } else { -distance };
            let value = 128.0 + signed * 127.0 / Font::RADIUS;
            field.push(value.round().clamp(0.0, 255.0) as u8);
        }
    }
    field
}

fn import_color_table(cache: &Cache, index: usize) -> Result<ColorTable, ImportError> {
    let table = cache.tag(index, COLO, COLOR_TABLE_SIZE)?;
    let colors = table
        .block(0, "colors", COLOR_SIZE, MAX_COLORS)?
        .iter()
        .map(|entry| {
            Ok((
                entry.text(0, "name", 31)?.to_owned(),
                color(&entry, 0x20, "color")?,
            ))
        })
        .collect::<Result<_, ImportError>>()?;
    Ok(ColorTable {
        tag: tag_ref(cache, index),
        colors,
    })
}

fn import_keyboard(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<VirtualKeyboard, ImportError> {
    let keyboard = cache.tag(index, VCKY, KEYBOARD_SIZE)?;
    let keys = keyboard
        .block(0x30, "keys", KEY_SIZE, MAX_KEYS)?
        .iter()
        .map(|key| {
            let mut characters = [0; 7];
            for (offset, character) in characters.iter_mut().enumerate() {
                *character = key.u16(2 + 2 * offset, "character")?;
            }
            Ok(VirtualKey {
                keycode: key.i16(0, "keyboard key")?,
                characters,
                bitmaps: [
                    reached.bitmap(&key, 0x10, "unselected background bitmap")?,
                    reached.bitmap(&key, 0x20, "selected background bitmap")?,
                    reached.bitmap(&key, 0x30, "active background bitmap")?,
                    reached.bitmap(&key, 0x40, "sticky background bitmap")?,
                ],
            })
        })
        .collect::<Result<_, ImportError>>()?;
    Ok(VirtualKeyboard {
        tag: tag_ref(cache, index),
        font: reference(&keyboard, 0, "display font", &[FONT])?,
        background: reached.bitmap(&keyboard, 0x10, "background bitmap")?,
        special_key_labels: reference(&keyboard, 0x20, "special key labels string list", &[USTR])?,
        keys,
    })
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, empty, put, reference};
    use std::error::Error;

    const BITM: GroupTag = GroupTag::new(b"bitm");

    fn data_field(bytes: &mut [u8], at: usize, size: usize, address: u32) {
        put(bytes, at, &(size as i32).to_le_bytes());
        put(bytes, at + 12, &address.to_le_bytes());
    }

    fn name(bytes: &mut [u8], at: usize, text: &str) {
        put(bytes, at, text.as_bytes());
    }

    /// Adds a widget with a child, a handler that opens another widget and a
    /// replacement; the other widget; a collection of both; string lists
    /// of either width; a colour table and a keyboard.
    pub(crate) fn add_shell(fake: &mut FakeCache) {
        let bitmap = fake.add_tag(BITM, "ui\\shell\\bitmaps\\background", &[0; 4]);
        let mut sub = [0u8; WIDGET_SIZE];
        put(&mut sub, 0, &1i16.to_le_bytes());
        name(&mut sub, 4, "options");
        for at in [0x38, 0xEC, 0xFC, 0x154, 0x164, 0x1A4] {
            empty(&mut sub, at);
        }
        let sub = fake.add_tag(DELA, "ui\\shell\\options", &sub);

        let mut handler = [0u8; HANDLER_SIZE];
        put(
            &mut handler,
            0,
            &EventHandler::CLOSE_CURRENT_WIDGET.to_le_bytes(),
        );
        put(&mut handler, 4, &EventHandler::A_BUTTON.to_le_bytes());
        put(&mut handler, 6, &3i16.to_le_bytes());
        reference(&mut handler, 8, DELA, fake.handle(sub));
        empty(&mut handler, 0x18);
        name(&mut handler, 0x28, "start_game");
        let handler = fake.add_data(&handler);
        let mut child = [0u8; CHILD_SIZE];
        reference(&mut child, 0, DELA, fake.handle(sub));
        name(&mut child, 0x10, "first");
        put(&mut child, 0x30, &ChildWidget::IS_LIST_ITEM.to_le_bytes());
        put(&mut child, 0x36, &5i16.to_le_bytes());
        put(&mut child, 0x38, &7i16.to_le_bytes());
        let child = fake.add_data(&child);
        let mut replacement = [0u8; REPLACEMENT_SIZE];
        name(&mut replacement, 0, "<player>");
        put(&mut replacement, 0x20, &2i16.to_le_bytes());
        let replacement = fake.add_data(&replacement);
        let mut inputs = [0u8; 2 * GAME_DATA_INPUT_SIZE];
        put(&mut inputs, 0, &4i16.to_le_bytes());
        put(&mut inputs, GAME_DATA_INPUT_SIZE, &9i16.to_le_bytes());
        let inputs = fake.add_data(&inputs);
        let mut main = [0u8; WIDGET_SIZE];
        name(&mut main, 4, "main menu");
        put(&mut main, 0x24, &[0, 0, 0, 0, 224, 1, 128, 2]);
        put(&mut main, 0x2C, &Widget::DONT_PUSH_HISTORY.to_le_bytes());
        put(&mut main, 0x30, &1000i32.to_le_bytes());
        reference(&mut main, 0x38, BITM, fake.handle(bitmap));
        block(&mut main, 0x48, 2, inputs);
        block(&mut main, 0x54, 1, handler);
        block(&mut main, 0x60, 1, replacement);
        for at in [0xEC, 0xFC, 0x154, 0x164, 0x1A4] {
            empty(&mut main, at);
        }
        put(&mut main, 0x10C, &1.0f32.to_le_bytes());
        put(&mut main, 0x12E, &3i16.to_le_bytes());
        block(&mut main, 0x3E0, 1, child);
        let main = fake.add_tag(DELA, "ui\\shell\\main_menu", &main);

        let mut entries = [0u8; 2 * COLLECTION_ELEMENT_SIZE];
        reference(&mut entries, 0, DELA, fake.handle(main));
        empty(&mut entries, COLLECTION_ELEMENT_SIZE);
        let entries = fake.add_data(&entries);
        let mut collection = [0u8; COLLECTION_SIZE];
        block(&mut collection, 0, 2, entries);
        fake.add_tag(SOUL, "ui\\shell\\screens", &collection);

        let wide: Vec<u8> = "Hé\0".encode_utf16().flat_map(u16::to_le_bytes).collect();
        let wide_at = fake.add_data(&wide);
        let mut string = [0u8; STRING_SIZE];
        data_field(&mut string, 0, wide.len(), wide_at);
        let string = fake.add_data(&string);
        let mut list = [0u8; STRING_LIST_SIZE];
        block(&mut list, 0, 1, string);
        fake.add_tag(USTR, "ui\\shell\\strings", &list);
        let narrow_at = fake.add_data(b"abc\0");
        let mut string = [0u8; STRING_SIZE];
        data_field(&mut string, 0, 4, narrow_at);
        let string = fake.add_data(&string);
        let mut list = [0u8; STRING_LIST_SIZE];
        block(&mut list, 0, 1, string);
        fake.add_tag(STR8, "ui\\shell\\narrow", &list);

        let mut colour = [0u8; COLOR_SIZE];
        name(&mut colour, 0, "red");
        put(&mut colour, 0x20, &1.0f32.to_le_bytes());
        put(&mut colour, 0x24, &1.0f32.to_le_bytes());
        let colour = fake.add_data(&colour);
        let mut table = [0u8; COLOR_TABLE_SIZE];
        block(&mut table, 0, 1, colour);
        fake.add_tag(COLO, "ui\\colors", &table);

        let mut key = [0u8; KEY_SIZE];
        put(&mut key, 0, &12i16.to_le_bytes());
        put(&mut key, 2, &u16::from(b'q').to_le_bytes());
        reference(&mut key, 0x10, BITM, fake.handle(bitmap));
        for at in [0x20, 0x30, 0x40] {
            empty(&mut key, at);
        }
        let key = fake.add_data(&key);
        let mut keyboard = [0u8; KEYBOARD_SIZE];
        for at in [0, 0x10, 0x20] {
            empty(&mut keyboard, at);
        }
        block(&mut keyboard, 0x30, 1, key);
        fake.add_tag(VCKY, "ui\\keyboard", &keyboard);
    }

    #[test]
    fn widgets_and_their_tags_are_read() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        add_shell(&mut fake);

        let cache = Cache::open(fake.build())?;
        let mut reached = Reached::default();
        let ui = import_ui(&cache, &mut reached)?;
        assert_eq!(reached.bitmaps.len(), 1, "the background, once");
        assert_eq!(ui.widgets.len(), 2);
        let Some(menu) = ui.widgets.iter().find(|widget| widget.name == "main menu") else {
            panic!("the main menu was not read");
        };
        assert!(ui.widget(&menu.tag).is_some(), "the widgets are sorted");
        assert_eq!(menu.kind, WidgetKind::Container);
        assert_eq!(menu.bounds, [0, 0, 480, 640]);
        assert_eq!(menu.flags, Widget::DONT_PUSH_HISTORY);
        assert_eq!(menu.auto_close_ms, 1000);
        assert_eq!(menu.game_data_inputs, [4, 9]);
        assert_eq!(menu.text.color[0], 1.0);
        assert_eq!(menu.text.string_index, 3);
        let handlers = menu
            .handlers_for(EventHandler::A_BUTTON)
            .collect::<Vec<_>>();
        assert_eq!(handlers.len(), 1);
        assert_eq!(handlers[0].function, 3);
        assert_eq!(handlers[0].script, "start_game");
        let Some(opened) = &handlers[0].widget else {
            panic!("the handler opens no widget");
        };
        assert_eq!(opened.path, "ui\\shell\\options");
        assert_eq!(menu.replacements[0].search, "<player>");
        assert_eq!(menu.children[0].name, "first");
        assert_eq!(menu.children[0].offset, [7, 5]);
        assert_eq!(menu.children[0].flags, ChildWidget::IS_LIST_ITEM);
        let Some(options) = ui.widget(opened) else {
            panic!("the opened widget is not in the UI");
        };
        assert_eq!(options.kind, WidgetKind::TextBox);
        assert_eq!(ui.collections[0].widgets.len(), 2);
        assert!(ui.collections[0].widgets[1].is_none());
        let strings: Vec<&str> = ui
            .string_lists
            .iter()
            .flat_map(|list| list.strings.iter().map(String::as_str))
            .collect();
        assert!(
            strings.contains(&"Hé") && strings.contains(&"abc"),
            "{strings:?}"
        );
        assert_eq!(
            ui.color_tables[0].colors[0],
            ("red".to_owned(), [1.0, 1.0, 0.0, 0.0])
        );
        let keyboard = &ui.keyboards[0];
        assert_eq!(keyboard.keys[0].keycode, 12);
        assert_eq!(keyboard.keys[0].characters[0], u16::from(b'q'));
        assert!(keyboard.keys[0].bitmaps[0].is_some() && keyboard.keys[0].bitmaps[1].is_none());
        Ok(())
    }

    /// A child naming a bitmap where a widget belongs is refused.
    #[test]
    fn a_reference_of_the_wrong_group_is_refused() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let bitmap = fake.add_tag(BITM, "ui\\bitmap", &[0; 4]);
        let mut child = [0u8; CHILD_SIZE];
        reference(&mut child, 0, BITM, fake.handle(bitmap));
        let child = fake.add_data(&child);
        let mut widget = [0u8; WIDGET_SIZE];
        for at in [0x38, 0xEC, 0xFC, 0x154, 0x164, 0x1A4] {
            empty(&mut widget, at);
        }
        block(&mut widget, 0x3E0, 1, child);
        fake.add_tag(DELA, "ui\\widget", &widget);
        let cache = Cache::open(fake.build())?;
        assert!(import_ui_tags(&cache).is_err());
        Ok(())
    }

    /// Adds a font of two glyphs, one mapped by its table too, and one whose
    /// pixels are past the font's if `past`.
    pub(crate) fn add_font(fake: &mut FakeCache, past: bool) {
        // (a 2×2 glyph, all covered, and a 1×1 empty one)
        let pixels = fake.add_data(&[255, 255, 255, 255, 0]);
        let mut characters = [0u8; 2 * CHARACTER_SIZE];
        put(&mut characters, 0, &u16::from(b'A').to_le_bytes());
        put(&mut characters, 2, &3i16.to_le_bytes());
        put(&mut characters, 4, &[2, 0, 2, 0, 1, 0, 2, 0]);
        let second = CHARACTER_SIZE;
        put(&mut characters, second, &u16::from(b' ').to_le_bytes());
        put(&mut characters, second + 2, &2i16.to_le_bytes());
        put(&mut characters, second + 4, &[1, 0, 1, 0]);
        put(
            &mut characters,
            second + 0x10,
            &(if past { 5i32 } else { 4 }).to_le_bytes(),
        );
        let characters = fake.add_data(&characters);
        // (one table, for the high byte 0, sending 'a' to 'A')
        let mut indices = [0xFFu8; 512];
        put(&mut indices, 2 * usize::from(b'a'), &0i16.to_le_bytes());
        let indices = fake.add_data(&indices);
        let mut table = [0u8; CHARACTER_TABLE_SIZE];
        block(&mut table, 0, 256, indices);
        let table = fake.add_data(&table);
        let mut font = [0u8; FONT_SIZE];
        put(&mut font, 4, &[10, 0, 3, 0, 2, 0, 1, 0]);
        block(&mut font, 0x30, 1, table);
        for at in [0x3C, 0x4C, 0x5C, 0x6C] {
            empty(&mut font, at);
        }
        block(&mut font, 0x7C, 2, characters);
        data_field(&mut font, 0x88, 5, pixels);
        fake.add_tag(FONT, "ui\\fonts\\small", &font);
    }

    /// A font of two glyphs, one mapped by its table too, and one whose
    /// pixels are past the font's.
    #[test]
    fn fonts_are_read_with_their_distance_fields() -> Result<(), Box<dyn Error>> {
        let font_with = |past: bool| -> Result<Cache, Box<dyn Error>> {
            let mut fake = FakeCache::new(CacheKind::CustomEdition);
            add_font(&mut fake, past);
            Ok(Cache::open(fake.build())?)
        };
        let ui = import_ui_tags(&font_with(false)?)?;
        let font = &ui.fonts[0];
        assert_eq!((font.ascent, font.descent), (10, 3));
        assert_eq!((font.leading_height, font.leading_width), (2, 1));
        let Some(a) = font.glyph(u16::from(b'a')) else {
            panic!("the table's 'a' is not mapped");
        };
        assert_eq!(a.character, u16::from(b'A'));
        assert!(
            font.glyph(u16::from(b' ')).is_none(),
            "the table unmapped ' '"
        );
        assert_eq!(a.size, [2, 2]);
        assert_eq!(a.origin, [1, 2]);
        assert_eq!(a.distance.len(), 10 * 10);
        // (inside above the middle, outside below it, and the far corner
        // as far out as the field reaches)
        let padding = Font::PADDING as usize;
        assert!(a.distance[padding * 10 + padding] > 128);
        assert!(a.distance[(padding - 1) * 10 + padding] < 128);
        assert_eq!(a.distance[0], 1);
        assert!(
            import_ui_tags(&font_with(true)?).is_err(),
            "pixels past the font's"
        );
        Ok(())
    }

    #[test]
    fn a_distance_field_follows_its_coverage() {
        // (a 4×1 bar, its left half covered)
        let field = distance_field(&[255, 200, 10, 0], [4, 1]);
        let padding = Font::PADDING as usize;
        let width = 4 + 2 * padding;
        let row = &field[padding * width..(padding + 1) * width];
        let (left, right) = (row[padding + 1], row[padding + 2]);
        assert!(left >= 128 && right < 128, "{row:?}");
        assert_eq!(
            i32::from(left) + i32::from(right),
            256,
            "the edge is between them"
        );
        assert!(row[padding] >= left, "deeper inside");
        assert!(distance_field(&[], [0, 0]).iter().all(|&texel| texel == 1));
    }

    /// Over coverages of every size up to 12 by 12, a field is inside
    /// (128 or more) exactly where its coverage is, outside all around its
    /// padding, and deeper the further a texel is from the edge.
    #[test]
    fn a_distance_field_is_inside_exactly_where_its_glyph_covers() {
        let mut state = 0x0F0E_7A11u32;
        let mut next = move || {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            state
        };
        let padding = Font::PADDING as usize;
        for _ in 0..400 {
            let width = (next() % 13) as usize;
            let height = (next() % 13) as usize;
            let coverage: Vec<u8> = (0..width * height).map(|_| next() as u8).collect();
            let field = distance_field(&coverage, [width as i16, height as i16]);
            let field_width = width + 2 * padding;
            assert_eq!(field.len(), field_width * (height + 2 * padding));
            for (index, &texel) in field.iter().enumerate() {
                let (x, y) = (index % field_width, index / field_width);
                let covered = x >= padding
                    && y >= padding
                    && x - padding < width
                    && y - padding < height
                    && coverage[(y - padding) * width + x - padding] >= 128;
                assert_eq!(texel >= 128, covered, "({x}, {y}) of {width}×{height}");
            }
            // (the corner is as far from any coverage as the field reaches)
            assert_eq!(field[0], 1);
        }
    }

    /// `ui.map`'s fonts: every glyph's field is its padded size, outside on
    /// its border, and inside somewhere if it has any coverage; every
    /// character maps to a glyph of the font; no advance is negative.
    #[test]
    fn the_shells_fonts_have_their_distance_fields() -> Result<(), Box<dyn Error>> {
        let Some(cache) = crate::level::tests::stock_map("ui") else {
            return Ok(());
        };
        let ui = import_ui_tags(&cache)?;
        assert!(!ui.fonts.is_empty());
        for font in &ui.fonts {
            assert!(font.line_height() > 0, "{}", font.tag.path);
            for &(_, glyph) in &font.characters {
                assert!((glyph as usize) < font.glyphs.len(), "{}", font.tag.path);
            }
            for glyph in &font.glyphs {
                let [width, height] = glyph.field_size();
                assert_eq!(glyph.distance.len(), width * height, "{}", font.tag.path);
                assert!(glyph.advance >= 0, "{}: {}", font.tag.path, glyph.character);
                for x in 0..width {
                    assert!(
                        glyph.distance[x] < 128 && glyph.distance[(height - 1) * width + x] < 128
                    );
                }
                for y in 0..height {
                    assert!(
                        glyph.distance[y * width] < 128
                            && glyph.distance[y * width + width - 1] < 128
                    );
                }
            }
            let inked = font
                .glyphs
                .iter()
                .filter(|glyph| glyph.distance.iter().any(|&texel| texel >= 128));
            assert!(
                inked.count() > font.glyphs.len() / 2,
                "{}: mostly empty glyphs",
                font.tag.path
            );
        }
        Ok(())
    }

    /// `ui.map`'s shell: every widget, list and font, each widget's
    /// children and the widgets its handlers open found among them.
    #[test]
    fn the_shell_imports_when_ui_map_is_here() -> Result<(), Box<dyn Error>> {
        let Some(cache) = crate::level::tests::stock_map("ui") else {
            return Ok(());
        };
        let ui = import_ui_tags(&cache)?;
        let count = |group: GroupTag| {
            cache
                .tags()
                .iter()
                .filter(|tag| tag.group == group && !tag.in_resource_map)
                .count()
        };
        assert_eq!(ui.widgets.len(), count(DELA));
        assert_eq!(ui.collections.len(), count(SOUL));
        assert_eq!(ui.string_lists.len(), count(USTR) + count(STR8));
        assert_eq!(ui.fonts.len(), count(FONT));
        assert!(ui.widgets.len() > 400, "{} widgets", ui.widgets.len());
        for widget in &ui.widgets {
            let named = widget
                .children
                .iter()
                .filter_map(|child| child.widget.as_ref())
                .chain(
                    widget
                        .handlers
                        .iter()
                        .filter_map(|handler| handler.widget.as_ref()),
                );
            for tag in named {
                assert!(
                    ui.widget(tag).is_some(),
                    "{} names {}",
                    widget.tag.path,
                    tag.path
                );
            }
        }
        for font in &ui.fonts {
            assert!(!font.glyphs.is_empty(), "{} has no glyphs", font.tag.path);
            assert!(
                font.glyph(u16::from(b'A')).is_some(),
                "{} has no A",
                font.tag.path
            );
        }
        Ok(())
    }
}
