//! The shell's widgets as the app draws them: quads of the widgets'
//! bitmaps, from the level's textures, and of their fonts' glyphs, from
//! one atlas of the glyphs' distance fields, laid out by `opence_ui` on
//! the 640×480 screen scaled into the window.

use std::sync::{Arc, Mutex, MutexGuard};

use opence_content::{Level, TagRef};
use opence_ui::{DrawItem, Key, Layout, MouseButton, Shell, NONE};
use rustc_hash::FxHashMap;

use crate::ui_host::MenuHost;
pub use crate::ui_host::{MenuAction, MenuEntry};
use crate::LevelGeometry;

/// The game's main menu in `ui.map`.
pub const MAIN_MENU: &str = "ui\\shell\\main_menu\\main_menu";

/// The campaign's pause menu.
pub const PAUSE_MENU: &str = "ui\\shell\\solo_game\\pause_game\\pause_game";

/// A network game's, for one player.
pub const PAUSE_MENU_MULTIPLAYER: &str = "ui\\shell\\multiplayer_game\\pause_game\\1p_pause_game";

/// The width of the glyph atlas, in texels.
pub const ATLAS_WIDTH: u32 = 1024;

/// The tag path of the screen the shell opens first.
#[uniffi::export]
pub fn main_menu_widget() -> String {
    MAIN_MENU.into()
}

/// The tag path of a mission's pause menu: a network game's, or the
/// campaign's.
#[uniffi::export]
pub fn pause_menu_widget(multiplayer: bool) -> String {
    if multiplayer {
        PAUSE_MENU_MULTIPLAYER.into()
    } else {
        PAUSE_MENU.into()
    }
}

/// One quad of the UI: a bitmap's image from the level's textures, or a
/// glyph from the atlas.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct UiQuad {
    /// The texture it shows, an index into the level's textures; unused
    /// for a glyph.
    pub texture: u32,
    /// Whether it is a glyph, its texture coordinates in the atlas.
    pub glyph: bool,
    /// Its left edge, in the window's pixels.
    pub left: f32,
    /// Its top edge.
    pub top: f32,
    /// Its right edge.
    pub right: f32,
    /// Its bottom edge.
    pub bottom: f32,
    /// Its left texture coordinate (a bitmap's repeat past 1).
    pub u0: f32,
    /// Its top one.
    pub v0: f32,
    /// Its right one.
    pub u1: f32,
    /// Its bottom one.
    pub v1: f32,
    /// What it is tinted (`0xAARRGGBB`; the alpha is how opaque it is).
    pub color: u32,
}

/// Every font's glyphs' distance fields, packed in rows: one byte a
/// texel, 128 on a glyph's edge.
#[derive(Clone, Debug, Default, PartialEq, Eq, uniffi::Record)]
pub struct UiGlyphAtlas {
    /// Its width, in texels.
    pub width: u32,
    /// Its height.
    pub height: u32,
    /// Its texels, row by row.
    pub texels: Vec<u8>,
}

/// Where each glyph is in the atlas, by its font and its index.
#[derive(Debug, Default)]
pub(crate) struct GlyphAtlas {
    pub(crate) atlas: UiGlyphAtlas,
    places: FxHashMap<(TagRef, usize), [f32; 4]>,
}

impl GlyphAtlas {
    /// The glyphs of the fonts of `level`'s UI, each placed once, a texel
    /// of space between them so that sampling one never reads another.
    pub(crate) fn build(level: &Level) -> Self {
        let width = ATLAS_WIDTH as usize;
        let mut placed: Vec<((TagRef, usize), [usize; 4])> = Vec::new();
        let (mut x, mut y, mut row) = (0usize, 0usize, 0usize);
        for font in &level.ui.fonts {
            for (index, glyph) in font.glyphs.iter().enumerate() {
                let [w, h] = glyph.field_size();
                if w == 0 || h == 0 || w > width || glyph.distance.len() != w * h {
                    continue;
                }
                if x + w > width {
                    x = 0;
                    y += row + 1;
                    row = 0;
                }
                placed.push(((font.tag.clone(), index), [x, y, w, h]));
                x += w + 1;
                row = row.max(h);
            }
        }
        let height = (y + row).max(1).next_power_of_two();
        let mut texels = vec![0u8; width * height];
        let mut places = FxHashMap::default();
        for ((font, index), [x, y, w, h]) in placed {
            let Some(glyph) = level.ui.font(&font).and_then(|font| font.glyphs.get(index)) else {
                continue;
            };
            for (line, source) in glyph.distance.chunks_exact(w).enumerate() {
                let at = (y + line) * width + x;
                texels[at..at + w].copy_from_slice(source);
            }
            let (fw, fh) = (width as f32, height as f32);
            places.insert(
                (font, index),
                [
                    x as f32 / fw,
                    y as f32 / fh,
                    (x + w) as f32 / fw,
                    (y + h) as f32 / fh,
                ],
            );
        }
        Self {
            atlas: UiGlyphAtlas {
                width: ATLAS_WIDTH,
                height: height as u32,
                texels,
            },
            places,
        }
    }

    /// Where glyph `index` of `font` is, in the atlas's coordinates.
    pub(crate) fn place(&self, font: &TagRef, index: usize) -> Option<[f32; 4]> {
        self.places.get(&(font.clone(), index)).copied()
    }
}

/// Every image of every bitmap the UI's widgets draw: their backgrounds
/// and their lists' arrows, each once.
pub(crate) fn ui_images(level: &Level) -> Vec<(TagRef, u16)> {
    let mut bitmaps: Vec<&TagRef> = Vec::new();
    for widget in &level.ui.widgets {
        for tag in [&widget.background, &widget.list.header, &widget.list.footer]
            .into_iter()
            .flatten()
        {
            if !bitmaps.contains(&tag) {
                bitmaps.push(tag);
            }
        }
    }
    let mut images = Vec::new();
    for tag in bitmaps {
        let Some(bitmap) = level.bitmap(tag) else {
            continue;
        };
        for image in 0..bitmap.images.len().min(usize::from(u16::MAX)) {
            images.push((tag.clone(), image as u16));
        }
    }
    images
}

/// The colour `[a, r, g, b]` as `0xAARRGGBB`.
pub(crate) fn pack_color(color: [f32; 4]) -> u32 {
    let byte = |value: f32| -> u32 { libm::roundf(value.clamp(0.0, 1.0) * 255.0) as u32 };
    (byte(color[0]) << 24) | (byte(color[1]) << 16) | (byte(color[2]) << 8) | byte(color[3])
}

/// The draw items as quads: those whose image or glyph isn't loaded are
/// left out.
pub(crate) fn quads(
    items: &[DrawItem],
    textures: &FxHashMap<(TagRef, u16), u32>,
    atlas: &GlyphAtlas,
) -> Vec<UiQuad> {
    items
        .iter()
        .filter_map(|item| match item {
            DrawItem::Quad(quad) => {
                let image = u16::try_from(quad.image).ok()?;
                let &texture = textures.get(&(quad.bitmap.clone(), image))?;
                let [left, top, right, bottom] = quad.rect;
                let [u0, v0, u1, v1] = quad.uv;
                Some(UiQuad {
                    texture,
                    glyph: false,
                    left,
                    top,
                    right,
                    bottom,
                    u0,
                    v0,
                    u1,
                    v1,
                    color: pack_color(quad.color),
                })
            }
            DrawItem::Glyph(glyph) => {
                let [u0, v0, u1, v1] = atlas.place(&glyph.font, glyph.glyph)?;
                let [left, top, right, bottom] = glyph.rect;
                Some(UiQuad {
                    texture: u32::MAX,
                    glyph: true,
                    left,
                    top,
                    right,
                    bottom,
                    u0,
                    v0,
                    u1,
                    v1,
                    color: pack_color(glyph.color),
                })
            }
        })
        .collect()
}

/// A key the menus take: the arrows the d-pad; Space, Return and the
/// keypad's Enter A; Escape and Backspace B; Delete and E X; Tab Y; F1
/// Back; W, A, S and D the left stick.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum MenuKey {
    /// Up.
    Up,
    /// Down.
    Down,
    /// Left.
    Left,
    /// Right.
    Right,
    /// Space.
    Space,
    /// Return.
    Return,
    /// The keypad's Enter.
    KeypadEnter,
    /// Escape.
    Escape,
    /// Backspace.
    Backspace,
    /// Delete.
    Delete,
    /// E.
    E,
    /// Tab.
    Tab,
    /// F1.
    F1,
    /// W.
    W,
    /// A.
    A,
    /// S.
    S,
    /// D.
    D,
}

impl From<MenuKey> for Key {
    fn from(key: MenuKey) -> Self {
        match key {
            MenuKey::Up => Key::Up,
            MenuKey::Down => Key::Down,
            MenuKey::Left => Key::Left,
            MenuKey::Right => Key::Right,
            MenuKey::Space => Key::Space,
            MenuKey::Return => Key::Return,
            MenuKey::KeypadEnter => Key::KeypadEnter,
            MenuKey::Escape => Key::Escape,
            MenuKey::Backspace => Key::Backspace,
            MenuKey::Delete => Key::Delete,
            MenuKey::E => Key::E,
            MenuKey::Tab => Key::Tab,
            MenuKey::F1 => Key::F1,
            MenuKey::W => Key::W,
            MenuKey::A => Key::A,
            MenuKey::S => Key::S,
            MenuKey::D => Key::D,
        }
    }
}

/// The shell and its host, which keeps what the widgets played and asked
/// for since the app last took them.
#[derive(Debug, Default)]
struct State {
    shell: Shell,
    host: MenuHost,
}

/// A point in the window, in pixels.
#[derive(Clone, Copy, Debug, PartialEq, uniffi::Record)]
pub struct MenuPoint {
    /// Across, from the left.
    pub x: f32,
    /// Down, from the top.
    pub y: f32,
}

/// The shell: the UI's widgets loaded from a level (`ui.map`'s, or a
/// mission's pause menu), driven by the app's input and clock.
#[derive(uniffi::Object)]
pub struct Menu {
    geometry: Arc<LevelGeometry>,
    atlas: GlyphAtlas,
    state: Mutex<State>,
}

#[uniffi::export]
impl Menu {
    /// The shell of the level `geometry` was loaded from, with nothing
    /// open.
    #[uniffi::constructor]
    pub fn new(geometry: Arc<LevelGeometry>) -> Arc<Self> {
        let atlas = GlyphAtlas::build(&geometry.content);
        Arc::new(Self {
            geometry,
            atlas,
            state: Mutex::new(State::default()),
        })
    }

    /// Opens the screen whose tag path is `widget` for every player:
    /// false if the level has no such widget.
    pub fn open(&self, widget: String) -> bool {
        let level = &self.geometry.content;
        let tag = TagRef {
            group: opence_content::Group(*b"DeLa"),
            path: widget,
        };
        let state = &mut *self.lock();
        state
            .shell
            .open(&level.ui, &mut state.host, &tag, NONE)
            .is_some()
    }

    /// Runs the widgets' timers and fades to `now_ms`, the app's clock.
    pub fn update(&self, now_ms: u64) {
        let state = &mut *self.lock();
        state
            .shell
            .update(&self.geometry.content.ui, &mut state.host, now_ms);
    }

    /// Whether a screen is open.
    pub fn is_open(&self) -> bool {
        self.lock().shell.root(0).is_some()
    }

    /// The quads to draw over a window `width` by `height` pixels, in
    /// order.
    pub fn draw(&self, width: u32, height: u32) -> Vec<UiQuad> {
        let level = &self.geometry.content;
        let layout = Layout::for_window(width as f32, height as f32);
        let items = self.lock().shell.draw(&level.ui, level.as_ref(), &layout);
        quads(&items, &self.geometry.level.ui_textures, &self.atlas)
    }

    /// `key` pressed.
    pub fn key(&self, key: MenuKey) {
        let state = &mut *self.lock();
        state
            .shell
            .key(&self.geometry.content.ui, &mut state.host, key.into());
    }

    /// The pointer moved to `x`, `y` in a window `width` by `height`
    /// pixels: the item under it takes the focus.
    pub fn pointer_moved(&self, x: f32, y: f32, width: u32, height: u32) {
        let point = Layout::for_window(width as f32, height as f32).to_screen([x, y]);
        let state = &mut *self.lock();
        state
            .shell
            .hover(&self.geometry.content.ui, &mut state.host, point);
    }

    /// A click at `x`, `y` in a window `width` by `height` pixels: the
    /// right button's goes back.
    pub fn click(&self, x: f32, y: f32, width: u32, height: u32, right: bool) {
        let point = Layout::for_window(width as f32, height as f32).to_screen([x, y]);
        let button = if right {
            MouseButton::Right
        } else {
            MouseButton::Left
        };
        let state = &mut *self.lock();
        state
            .shell
            .click(&self.geometry.content.ui, &mut state.host, point, button);
    }

    /// The wheel turned `steps` notches, away from the user positive.
    pub fn wheel(&self, steps: i32) {
        let state = &mut *self.lock();
        state
            .shell
            .wheel(&self.geometry.content.ui, &mut state.host, steps);
    }

    /// The tag paths of the sounds the widgets played since the last call,
    /// in order.
    pub fn take_sounds(&self) -> Vec<String> {
        std::mem::take(&mut self.lock().host.sounds)
    }

    /// What the menus asked of the engine since the last call, in order.
    pub fn take_actions(&self) -> Vec<MenuAction> {
        std::mem::take(&mut self.lock().host.actions)
    }

    /// The tag path of the screen open, if one is.
    pub fn screen(&self) -> Option<String> {
        let state = self.lock();
        let root = state.shell.root(0)?;
        state
            .shell
            .definition(&self.geometry.content.ui, root)
            .map(|definition| definition.tag.path.clone())
    }

    /// The middles of what the pointer can act on, in a window `width` by
    /// `height` pixels, in the order they are drawn.
    pub fn targets(&self, width: u32, height: u32) -> Vec<MenuPoint> {
        let layout = Layout::for_window(width as f32, height as f32);
        let targets = self.lock().shell.targets(&self.geometry.content.ui);
        targets
            .iter()
            .map(|target| {
                let [top, left, bottom, right] = target.bounds.map(f32::from);
                let [x, y] = layout.to_window([(left + right) * 0.5, (top + bottom) * 0.5]);
                MenuPoint { x, y }
            })
            .collect()
    }

    /// The campaign's missions the menus list, in order.
    pub fn set_levels(&self, levels: Vec<MenuEntry>) {
        self.lock().host.levels = levels;
    }

    /// The multiplayer maps they list.
    pub fn set_maps(&self, maps: Vec<MenuEntry>) {
        self.lock().host.maps = maps;
    }

    /// The servers found, by name.
    pub fn set_servers(&self, servers: Vec<String>) {
        self.lock().host.servers = servers;
    }

    /// The player's name.
    pub fn set_profile(&self, profile: String) {
        self.lock().host.profile = profile;
    }

    /// The glyph atlas the glyph quads sample.
    pub fn glyph_atlas(&self) -> UiGlyphAtlas {
        self.atlas.atlas.clone()
    }
}

impl Menu {
    fn lock(&self) -> MutexGuard<'_, State> {
        self.state
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{
        Font, Glyph, Group, StringList, Widget, WidgetKind, WidgetList, WidgetText,
    };

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn glyph(character: u16, size: [i16; 2], fill: u8) -> Glyph {
        let padded = |extent: i16| usize::try_from(extent + 2 * Font::PADDING).unwrap_or(0);
        Glyph {
            character,
            advance: size[0] + 1,
            size,
            origin: [0, size[1]],
            distance: vec![fill; padded(size[0]) * padded(size[1])],
        }
    }

    /// The test map with a font of two glyphs, a string list and a text
    /// box showing "AB".
    fn level() -> Level {
        let mut level = opence_content::devmap::level(None);
        level.ui.fonts.push(Font {
            tag: tag(b"font", "font"),
            flags: 0,
            ascent: 10,
            descent: 2,
            leading_height: 0,
            leading_width: 0,
            styles: [None, None, None, None],
            glyphs: vec![glyph(65, [6, 8], 200), glyph(66, [1020, 8], 9)],
            characters: vec![(65, 0), (66, 1)],
        });
        level.ui.string_lists.push(StringList {
            tag: tag(b"ustr", "strings"),
            strings: vec!["AB".into()],
        });
        level.ui.widgets.push(Widget {
            tag: tag(b"DeLa", "screen"),
            kind: WidgetKind::TextBox,
            controller: NONE,
            name: "screen".into(),
            bounds: [0, 0, 480, 640],
            flags: 0,
            auto_close_ms: 0,
            auto_close_fade_ms: 0,
            background: Some(opence_content::devmap::bitmap_tag()),
            game_data_inputs: Vec::new(),
            handlers: Vec::new(),
            replacements: Vec::new(),
            text: WidgetText {
                string_list: Some(tag(b"ustr", "strings")),
                font: Some(tag(b"font", "font")),
                color: [1.0, 1.0, 1.0, 1.0],
                justification: 0,
                flags: 0,
                string_index: 0,
                offset: [0, 0],
            },
            list: WidgetList::default(),
            extended_description: None,
            conditionals: Vec::new(),
            children: Vec::new(),
        });
        level
    }

    #[test]
    fn colours_pack_as_argb() {
        assert_eq!(pack_color([1.0, 0.0, 0.5, 1.0]), 0xFF00_80FF);
        assert_eq!(pack_color([2.0, -1.0, 0.0, 0.0]), 0xFF00_0000);
    }

    #[test]
    fn the_atlas_holds_each_glyph_that_fits() {
        let atlas = GlyphAtlas::build(&level());
        assert_eq!(atlas.atlas.width, ATLAS_WIDTH);
        assert!(atlas.atlas.height.is_power_of_two());
        assert_eq!(
            atlas.atlas.texels.len(),
            (atlas.atlas.width * atlas.atlas.height) as usize
        );
        let Some([u0, v0, u1, v1]) = atlas.place(&tag(b"font", "font"), 0) else {
            panic!("A is not in the atlas");
        };
        assert_eq!([u0, v0], [0.0, 0.0]);
        assert!((u1 * ATLAS_WIDTH as f32 - 14.0).abs() < 1e-3);
        assert!((v1 * atlas.atlas.height as f32 - 16.0).abs() < 1e-3);
        assert_eq!(atlas.atlas.texels[0], 200);
        assert_eq!(atlas.atlas.texels[14], 0);
        // a glyph wider than the atlas is left out
        assert_eq!(atlas.place(&tag(b"font", "font"), 1), None);
    }

    #[test]
    fn rows_wrap_at_the_atlas_edge() {
        let mut level = level();
        level.ui.fonts[0].glyphs = (0..100).map(|n| glyph(n, [40, 20], 1)).collect();
        level.ui.fonts[0].characters = (0..100).map(|n| (n, u32::from(n))).collect();
        let atlas = GlyphAtlas::build(&level);
        // 48 + 1 texels each: 20 to a row of 1024
        let Some(place) = atlas.place(&tag(b"font", "font"), 20) else {
            panic!("glyph 20 is not in the atlas");
        };
        assert_eq!(place[0], 0.0);
        assert!((place[1] * atlas.atlas.height as f32 - 29.0).abs() < 1e-3);
    }

    #[test]
    fn the_menu_draws_its_screen() {
        let level = level();
        let Some(render) = crate::render_level::RenderLevel::build(&level, 0) else {
            panic!("the test map has no BSP");
        };
        let geometry = Arc::new(LevelGeometry {
            name: "test".into(),
            level: render,
            bsp: 0,
            content: Arc::new(level),
        });
        assert!(!ui_images(&geometry.content).is_empty());
        let menu = Menu::new(geometry);
        assert!(!menu.is_open());
        assert!(!menu.open("missing".into()));
        assert!(menu.open("screen".into()));
        assert!(menu.is_open());
        menu.update(16);
        let quads = menu.draw(1280, 960);
        let glyphs: Vec<&UiQuad> = quads.iter().filter(|quad| quad.glyph).collect();
        // B is too wide for the atlas, so only A is drawn
        assert_eq!(glyphs.len(), 1);
        assert_eq!(glyphs[0].color, 0xFFCC_CCCC);
        let backgrounds: Vec<&UiQuad> = quads.iter().filter(|quad| !quad.glyph).collect();
        assert_eq!(backgrounds.len(), 1);
        assert_eq!([backgrounds[0].left, backgrounds[0].bottom], [0.0, 960.0]);
        assert!(menu.glyph_atlas().height > 0);
        assert_eq!(main_menu_widget(), MAIN_MENU);
        // the screen has no items: the input reaches it and changes nothing
        menu.key(MenuKey::Down);
        menu.pointer_moved(640.0, 480.0, 1280, 960);
        menu.click(640.0, 480.0, 1280, 960, false);
        menu.wheel(1);
        assert!(menu.is_open());
        assert_eq!(menu.take_sounds(), Vec::<String>::new());
        // B with no screen to go back to goes back to the main menu, with
        // the sound of going back
        menu.click(0.0, 0.0, 1280, 960, true);
        assert!(menu
            .take_sounds()
            .iter()
            .all(|sound| sound.starts_with("sound\\")));
    }

    #[test]
    fn every_key_is_the_shells() {
        let keys = [
            MenuKey::Up,
            MenuKey::Down,
            MenuKey::Left,
            MenuKey::Right,
            MenuKey::Space,
            MenuKey::Return,
            MenuKey::KeypadEnter,
            MenuKey::Escape,
            MenuKey::Backspace,
            MenuKey::Delete,
            MenuKey::E,
            MenuKey::Tab,
            MenuKey::F1,
            MenuKey::W,
            MenuKey::A,
            MenuKey::S,
            MenuKey::D,
        ];
        let shells: Vec<Key> = keys.into_iter().map(Key::from).collect();
        assert_eq!(shells[0], Key::Up);
        assert_eq!(shells[6], Key::KeypadEnter);
        assert_eq!(shells[16], Key::D);
    }
}
