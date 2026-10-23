//! What the shell draws: each widget's background, its list's header and
//! footer, and its text, as the game's widget code draws them
//! (`widget_instance_render_recursive` and the text box and spinner list
//! item drawing in source/interface/ui_widget.c, the strings laid out as
//! source/text/draw_string.c lays them out), on the 640×480 screen scaled
//! to the window.

use opence_content::{Bitmap, Font, Level, TagRef, Ui, Widget, WidgetKind};

use crate::{is_list, Id, Shell, LIST_ITEMS_FROM_CODE, LIST_ITEMS_FROM_STRING_LIST};

/// The screen's width, in the widgets' pixels.
pub const SCREEN_WIDTH: f32 = 640.0;
/// Its height.
pub const SCREEN_HEIGHT: f32 = 480.0;
/// The red, green and blue of the UI's white (`get_ui_rgb_white`).
pub const UI_WHITE: f32 = 0.8;
/// The widest bitmap that, filling the whole 640×480 screen, is a flat
/// colour widened to the whole window.
pub const FLAT_FILL_WIDTH: u16 = 16;
/// The text flag "flashing".
pub const TEXT_FLASHING: u16 = 1 << 2;

/// Where the 640×480 screen is in a window: as large as fits, centred.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Layout {
    /// The window's width, in pixels.
    pub width: f32,
    /// Its height.
    pub height: f32,
    /// Window pixels to a screen pixel.
    pub scale: f32,
    /// The screen's left edge in the window.
    pub left: f32,
    /// Its top edge.
    pub top: f32,
}

impl Layout {
    /// The screen in a window `width` by `height` pixels.
    pub fn for_window(width: f32, height: f32) -> Self {
        let scale = (height / SCREEN_HEIGHT).min(width / SCREEN_WIDTH).max(0.0);
        Self {
            width,
            height,
            scale,
            left: (width - SCREEN_WIDTH * scale) * 0.5,
            top: (height - SCREEN_HEIGHT * scale) * 0.5,
        }
    }

    /// The window's point at the screen's `point`.
    pub fn to_window(&self, point: [f32; 2]) -> [f32; 2] {
        [
            self.left + point[0] * self.scale,
            self.top + point[1] * self.scale,
        ]
    }

    /// The screen's point at the window's `point` (the mouse's).
    pub fn to_screen(&self, point: [f32; 2]) -> [f32; 2] {
        if self.scale <= 0.0 {
            return [0.0, 0.0];
        }
        [
            (point[0] - self.left) / self.scale,
            (point[1] - self.top) / self.scale,
        ]
    }

    /// The window's rectangle (left, top, right, bottom) at the screen's
    /// `bounds` (top, left, bottom, right).
    pub fn rect(&self, bounds: [i16; 4]) -> [f32; 4] {
        let [top, left, bottom, right] = bounds.map(f32::from);
        let [x0, y0] = self.to_window([left, top]);
        let [x1, y1] = self.to_window([right, bottom]);
        [x0, y0, x1, y1]
    }
}

/// The bitmaps the widgets' backgrounds and lists draw, by tag.
pub trait Bitmaps {
    /// The bitmap `tag`.
    fn bitmap(&self, tag: &TagRef) -> Option<&Bitmap>;
}

impl Bitmaps for Vec<Bitmap> {
    fn bitmap(&self, tag: &TagRef) -> Option<&Bitmap> {
        self.iter().find(|bitmap| bitmap.tag == *tag)
    }
}

impl Bitmaps for Level {
    fn bitmap(&self, tag: &TagRef) -> Option<&Bitmap> {
        Level::bitmap(self, tag)
    }
}

/// A bitmap's image drawn in a rectangle of the window.
#[derive(Clone, Debug, PartialEq)]
pub struct Quad {
    /// The bitmap.
    pub bitmap: TagRef,
    /// Its image.
    pub image: usize,
    /// Where: left, top, right, bottom, in window pixels.
    pub rect: [f32; 4],
    /// The texture's coordinates at those corners: left, top, right,
    /// bottom (one texel to a screen pixel, so past 1 where the bounds are
    /// larger than the image).
    pub uv: [f32; 4],
    /// The colour it is tinted with: alpha, red, green, blue.
    pub color: [f32; 4],
}

/// A glyph's distance field drawn in a rectangle of the window.
#[derive(Clone, Debug, PartialEq)]
pub struct GlyphQuad {
    /// The font.
    pub font: TagRef,
    /// The glyph's index in its glyphs.
    pub glyph: usize,
    /// Where its field goes, padding and all: left, top, right, bottom,
    /// in window pixels.
    pub rect: [f32; 4],
    /// Its colour: alpha, red, green, blue.
    pub color: [f32; 4],
}

/// One thing drawn, in the order drawn.
#[derive(Clone, Debug, PartialEq)]
pub enum DrawItem {
    /// A bitmap.
    Quad(Quad),
    /// A character.
    Glyph(GlyphQuad),
}

/// The image of `bitmap` its first sequence shows at `frame`.
pub fn sequence_image(bitmap: &Bitmap, frame: usize) -> Option<usize> {
    let image = match bitmap.sequences.first() {
        Some(sequence) => {
            let count = usize::from(sequence.image_count);
            if count == 0 {
                return None;
            }
            usize::from(sequence.first_image?) + frame.min(count - 1)
        }
        None => frame,
    };
    (image < bitmap.images.len()).then_some(image)
}

/// How many frames the first sequence of `bitmap` has.
pub fn frame_count(bitmap: &Bitmap) -> usize {
    bitmap
        .sequences
        .first()
        .map_or(bitmap.images.len(), |sequence| {
            usize::from(sequence.image_count)
        })
}

/// The flashing background's alpha at `ms`.
fn background_flash(ms: u64) -> f32 {
    (libm::cosf(ms as f32 * 0.001 * 3.0) + 1.0) * 0.5
}

/// The colour of a widget's text (alpha, red, green, blue): white when it
/// is focused, its own colour otherwise (pure white is the UI's white).
fn text_color(color: [f32; 4], focus: bool) -> [f32; 4] {
    let [alpha, red, green, blue] = color;
    if focus || (red == 1.0 && green == 1.0 && blue == 1.0) {
        [alpha, UI_WHITE, UI_WHITE, UI_WHITE]
    } else {
        color
    }
}

/// The characters of `text` and where the lines break: `'\r'` and `|n`
/// end a line (a `'\n'` after either is part of the break).
fn lines(text: &str) -> Vec<Vec<u16>> {
    let mut lines = vec![Vec::new()];
    let units: Vec<u16> = text.encode_utf16().collect();
    let mut index = 0;
    while index < units.len() {
        let unit = units[index];
        index += 1;
        let pipe_n = unit == u16::from(b'|') && units.get(index) == Some(&u16::from(b'n'));
        if unit == u16::from(b'\r') || pipe_n {
            if pipe_n {
                index += 1;
            }
            if units.get(index) == Some(&u16::from(b'\n')) {
                index += 1;
            }
            lines.push(Vec::new());
        } else if unit == u16::from(b'\n') {
            lines.push(Vec::new());
        } else if let Some(line) = lines.last_mut() {
            line.push(unit);
        }
    }
    lines
}

/// Lays `text` out in `font` within `bounds` (top, left, bottom, right,
/// on the screen) by `justification`, as `draw_string` does without
/// wrapping: each line starts the font's leading width in, its baseline the
/// ascent below its top; a character that reaches the right edge ends its
/// line, and the rest goes on below; lines below the bounds are not drawn.
/// Each glyph is its index and its field's rectangle on the screen (left,
/// top, right, bottom).
pub fn layout_text(
    font: &Font,
    text: &str,
    bounds: [i16; 4],
    justification: i16,
) -> Vec<(usize, [f32; 4])> {
    let [top, left, bottom, right] = bounds.map(i32::from);
    let leading = i32::from(font.leading_width);
    let line_height = i32::from(font.line_height());
    let glyph_of = |character: u16| {
        let index = font
            .characters
            .binary_search_by_key(&character, |&(code, _)| code)
            .ok()?;
        let glyph = usize::try_from(font.characters[index].1).ok()?;
        font.glyphs.get(glyph).map(|found| (glyph, found))
    };
    let mut placed = Vec::new();
    let mut row = 0;
    for line in lines(text) {
        let mut start = 0;
        loop {
            let x0 = leading + left;
            let mut width = 0;
            let mut end = start;
            while end < line.len() {
                end += 1;
                if let Some((_, glyph)) = glyph_of(line[end - 1]) {
                    if i32::from(glyph.size[0]) + x0 + width < right {
                        width += i32::from(glyph.advance);
                    } else {
                        break;
                    }
                }
            }
            let mut x = match justification {
                2 => ((right - left - width) >> 1) + left,
                1 => right - leading - width,
                _ => x0,
            };
            let y = line_height * row + i32::from(font.ascent) + top;
            if y < bottom {
                for &character in &line[start..end] {
                    let Some((index, glyph)) = glyph_of(character) else {
                        continue;
                    };
                    if glyph.size[0] > 0 && glyph.size[1] > 0 {
                        let padding = i32::from(Font::PADDING);
                        let x0 = x - i32::from(glyph.origin[0]) - padding;
                        let y0 = y - i32::from(glyph.origin[1]) - padding;
                        let [width, height] = glyph.field_size();
                        placed.push((
                            index,
                            [
                                x0 as f32,
                                y0 as f32,
                                x0 as f32 + width as f32,
                                y0 as f32 + height as f32,
                            ],
                        ));
                    }
                    x += i32::from(glyph.advance);
                }
            }
            row += 1;
            if end >= line.len() {
                break;
            }
            start = end;
        }
    }
    placed
}

/// The screen's rectangle to the window's.
fn window_rect(layout: &Layout, rect: [f32; 4]) -> [f32; 4] {
    let [x0, y0] = layout.to_window([rect[0], rect[1]]);
    let [x1, y1] = layout.to_window([rect[2], rect[3]]);
    [x0, y0, x1, y1]
}

impl Shell {
    /// How opaque `id` is: its alpha and every parent's.
    pub fn cumulative_alpha(&self, id: Id) -> f32 {
        let mut alpha = 1.0;
        let mut current = Some(id);
        while let Some(at) = current {
            let Some(instance) = self.get(at) else {
                break;
            };
            alpha *= instance.alpha;
            current = instance.parent;
        }
        alpha
    }

    /// The frame of its background `id` shows: in a list, the focused item
    /// shows its second frame if its bitmap has exactly two.
    pub fn background_frame(&self, ui: &Ui, bitmaps: &dyn Bitmaps, id: Id) -> usize {
        let Some(instance) = self.get(id) else {
            return 0;
        };
        let Some(parent) = instance.parent.and_then(|parent| self.get(parent)) else {
            return 0;
        };
        if !is_list(parent.kind) || parent.focused != Some(id) {
            return 0;
        }
        let two = self
            .definition(ui, id)
            .and_then(|definition| definition.background.as_ref())
            .and_then(|tag| bitmaps.bitmap(tag))
            .is_some_and(|bitmap| frame_count(bitmap) == 2);
        usize::from(two)
    }

    /// Everything the shell's screens draw now, in the order drawn, in a
    /// window as `layout` places the screen; a spinner's arrow lit by its
    /// last turn dims a step each time.
    pub fn draw(&mut self, ui: &Ui, bitmaps: &dyn Bitmaps, layout: &Layout) -> Vec<DrawItem> {
        let mut items = Vec::new();
        let roots: Vec<Id> = self.roots().collect();
        for root in roots {
            self.draw_widget(ui, bitmaps, layout, root, true, &mut items);
        }
        items
    }

    fn draw_widget(
        &mut self,
        ui: &Ui,
        bitmaps: &dyn Bitmaps,
        layout: &Layout,
        id: Id,
        focus: bool,
        items: &mut Vec<DrawItem>,
    ) {
        let Some(instance) = self.get(id) else {
            return;
        };
        let Some(definition) = ui.widgets.get(instance.definition) else {
            return;
        };
        if !instance.visible {
            return;
        }
        let (kind, focused, children) =
            (instance.kind, instance.focused, instance.children.clone());
        let alpha = self.cumulative_alpha(id);
        // an item made from its list's string list is a copy of the list's
        // definition, drawn as one line of the list's text
        if instance.generated {
            if let Some(text) = self.text(ui, id) {
                self.draw_text(ui, layout, id, definition, &text, focus, alpha, items);
            }
            return;
        }
        self.draw_background(ui, bitmaps, layout, id, definition, alpha, items);
        match kind {
            WidgetKind::TextBox => {
                if let Some(text) = self.text(ui, id) {
                    self.draw_text(ui, layout, id, definition, &text, focus, alpha, items);
                }
            }
            WidgetKind::SpinnerList => {
                self.draw_arrows(bitmaps, layout, id, definition, alpha, items);
                if let Some(text) = self.spinner_text(ui, id) {
                    self.draw_text(ui, layout, id, definition, &text, focus, alpha, items);
                }
            }
            _ => {}
        }
        // a spinner shows its selected item itself
        let hide_children = (kind == WidgetKind::ColumnList
            && definition.list.flags & LIST_ITEMS_FROM_CODE != 0)
            || (kind == WidgetKind::SpinnerList
                && definition.list.flags & LIST_ITEMS_FROM_STRING_LIST != 0);
        if !hide_children {
            for child in children {
                self.draw_widget(ui, bitmaps, layout, child, focused == Some(child), items);
            }
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn draw_background(
        &self,
        ui: &Ui,
        bitmaps: &dyn Bitmaps,
        layout: &Layout,
        id: Id,
        definition: &Widget,
        alpha: f32,
        items: &mut Vec<DrawItem>,
    ) {
        let Some(tag) = &definition.background else {
            return;
        };
        let Some(bitmap) = bitmaps.bitmap(tag) else {
            return;
        };
        let Some(bounds) = self.bounds(ui, id) else {
            return;
        };
        let frame = self.background_frame(ui, bitmaps, id);
        let Some(image) = sequence_image(bitmap, frame) else {
            return;
        };
        let size = &bitmap.images[image];
        let [top, left, bottom, right] = bounds;
        let flat = top <= 0
            && left <= 0
            && bottom >= SCREEN_HEIGHT as i16
            && right >= SCREEN_WIDTH as i16
            && size.width <= FLAT_FILL_WIDTH;
        let rect = if flat {
            [0.0, 0.0, layout.width, layout.height]
        } else {
            layout.rect(bounds)
        };
        let mut alpha = alpha;
        if definition.flags & Widget::FLASH_BACKGROUND_BITMAP != 0 {
            alpha *= background_flash(self.now_ms());
        }
        let texels = |extent: i16, of: u16| f32::from(extent) / f32::from(of.max(1));
        items.push(DrawItem::Quad(Quad {
            bitmap: tag.clone(),
            image,
            rect,
            uv: [
                0.0,
                0.0,
                texels(right.saturating_sub(left), size.width),
                texels(bottom.saturating_sub(top), size.height),
            ],
            color: [alpha, 1.0, 1.0, 1.0],
        }));
    }

    /// The spinner's header and footer: the header lit after a turn back,
    /// the footer after a turn on, each dimming back a step a frame.
    fn draw_arrows(
        &mut self,
        bitmaps: &dyn Bitmaps,
        layout: &Layout,
        id: Id,
        definition: &Widget,
        alpha: f32,
        items: &mut Vec<DrawItem>,
    ) {
        let Some(instance) = self.get_mut(id) else {
            return;
        };
        let direction = instance.list.last_direction;
        instance.list.last_direction -= direction.signum();
        let [across, down] = instance.offset;
        let arrows = [
            (
                &definition.list.header,
                definition.list.header_bounds,
                direction < 0,
            ),
            (
                &definition.list.footer,
                definition.list.footer_bounds,
                direction > 0,
            ),
        ];
        for (tag, bounds, lit) in arrows {
            let Some(tag) = tag else {
                continue;
            };
            let Some(bitmap) = bitmaps.bitmap(tag) else {
                continue;
            };
            let Some(image) = sequence_image(bitmap, usize::from(lit)) else {
                continue;
            };
            let [top, left, bottom, right] = bounds;
            let bounds = [
                top.saturating_add(down),
                left.saturating_add(across),
                bottom.saturating_add(down),
                right.saturating_add(across),
            ];
            let size = &bitmap.images[image];
            items.push(DrawItem::Quad(Quad {
                bitmap: tag.clone(),
                image,
                rect: layout.rect(bounds),
                uv: [
                    0.0,
                    0.0,
                    f32::from(right.saturating_sub(left)) / f32::from(size.width.max(1)),
                    f32::from(bottom.saturating_sub(top)) / f32::from(size.height.max(1)),
                ],
                color: [alpha, 1.0, 1.0, 1.0],
            }));
        }
    }

    /// The item a spinner shows: its string list's string at its selected
    /// item, with its replacements made; the game's text otherwise.
    pub fn spinner_text(&self, ui: &Ui, id: Id) -> Option<String> {
        let instance = self.get(id)?;
        let definition = ui.widgets.get(instance.definition)?;
        let Some(tag) = &definition.text.string_list else {
            return instance.text.clone().filter(|text| !text.is_empty());
        };
        let selected = usize::try_from(instance.list.selected).ok()?;
        let mut text = ui.string_list(tag)?.strings.get(selected)?.clone();
        for replacement in &definition.replacements {
            if !replacement.search.is_empty() {
                text = text.replace(
                    &replacement.search,
                    crate::replacement_value(replacement.function, instance.player),
                );
            }
        }
        (!text.is_empty()).then_some(text)
    }

    #[allow(clippy::too_many_arguments)]
    fn draw_text(
        &self,
        ui: &Ui,
        layout: &Layout,
        id: Id,
        definition: &Widget,
        text: &str,
        focus: bool,
        alpha: f32,
        items: &mut Vec<DrawItem>,
    ) {
        let justification = definition.text.justification;
        if !(0..=2).contains(&justification) {
            return;
        }
        let Some(font_tag) = &definition.text.font else {
            return;
        };
        let Some(font) = ui.font(font_tag) else {
            return;
        };
        let Some(mut bounds) = self.bounds(ui, id) else {
            return;
        };
        let spinner = definition.kind == WidgetKind::SpinnerList;
        if !spinner {
            bounds[1] = bounds[1].saturating_add(definition.text.offset[0]);
            bounds[0] = bounds[0].saturating_add(definition.text.offset[1]);
        }
        let mut color = text_color(definition.text.color, focus);
        color[0] *= alpha;
        if definition.text.flags & TEXT_FLASHING != 0 {
            let seconds = self.now_ms() as f32 * 0.001 * 3.0;
            color[0] *= if spinner {
                (libm::sinf(seconds) + 1.0) * 0.5
            } else {
                (libm::cosf(seconds) + 1.5) * 0.4
            };
        }
        for (glyph, rect) in layout_text(font, text, bounds, justification) {
            items.push(DrawItem::Glyph(GlyphQuad {
                font: font_tag.clone(),
                glyph,
                rect: window_rect(layout, rect),
                color,
            }));
        }
    }
}
