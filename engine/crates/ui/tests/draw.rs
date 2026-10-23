//! What the shell draws over a small set of widget definitions made here:
//! the 640×480 screen in the window, backgrounds, highlights, flashing,
//! fading, the spinner's arrows and text laid out in a font.

use opence_content::{
    Bitmap, BitmapImage, ChildWidget, Font, Glyph, Group, Pixels, ResourceRange, Sequence,
    StringList, TagRef, Ui, Widget, WidgetKind, WidgetList, WidgetText,
};
use opence_ui::{
    frame_count, layout_text, sequence_image, DrawItem, GlyphQuad, Layout, NoHost, Quad, Shell,
    LIST_ITEMS_FROM_CODE, LIST_ITEMS_FROM_STRING_LIST, NONE, TEXT_FLASHING, UI_WHITE,
};

fn tag(group: &[u8; 4], path: &str) -> TagRef {
    TagRef {
        group: Group(*group),
        path: path.into(),
    }
}

fn widget(path: &str, kind: WidgetKind) -> Widget {
    Widget {
        tag: tag(b"DeLa", path),
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

fn child(path: &str, offset: [i16; 2]) -> ChildWidget {
    ChildWidget {
        widget: Some(tag(b"DeLa", path)),
        name: path.into(),
        flags: 0,
        controller: NONE,
        offset,
    }
}

fn bitmap(path: &str, width: u16, height: u16, frames: u16) -> Bitmap {
    let image = BitmapImage {
        width,
        height,
        depth: 1,
        kind: 0,
        format: 0,
        flags: 0,
        registration_point: [0, 0],
        mipmap_count: 0,
        pixels: Pixels::ResourceMap(ResourceRange { offset: 0, size: 0 }),
    };
    Bitmap {
        tag: tag(b"bitm", path),
        kind: 4,
        format: 0,
        usage: 0,
        flags: 0,
        bump_height: 0.0,
        sequences: vec![Sequence {
            name: String::new(),
            first_image: Some(1),
            image_count: frames,
            sprites: Vec::new(),
        }],
        images: vec![image; usize::from(frames) + 1],
    }
}

/// A font with an "A" (6 by 8, its pen at its bottom left, 7 across) and a
/// space (empty, 3 across); 10 above the baseline, 2 below.
fn font() -> Font {
    Font {
        tag: tag(b"font", "font"),
        flags: 0,
        ascent: 10,
        descent: 2,
        leading_height: 0,
        leading_width: 1,
        styles: [None, None, None, None],
        glyphs: vec![
            Glyph {
                character: 65,
                advance: 7,
                size: [6, 8],
                origin: [0, 8],
                distance: vec![0; 14 * 16],
            },
            Glyph {
                character: 32,
                advance: 3,
                size: [0, 0],
                origin: [0, 0],
                distance: Vec::new(),
            },
        ],
        characters: vec![(32, 1), (65, 0)],
    }
}

fn text(strings: &str, index: i16) -> WidgetText {
    WidgetText {
        string_list: Some(tag(b"ustr", strings)),
        font: Some(tag(b"font", "font")),
        color: [1.0, 0.5, 0.25, 0.125],
        justification: 0,
        flags: 0,
        string_index: index,
        offset: [0, 0],
    }
}

fn ui(mut widgets: Vec<Widget>, strings: &[(&str, &[&str])]) -> Ui {
    widgets.sort_by(|a, b| a.tag.cmp(&b.tag));
    let mut string_lists: Vec<StringList> = strings
        .iter()
        .map(|(path, strings)| StringList {
            tag: tag(b"ustr", path),
            strings: strings.iter().map(|&string| string.into()).collect(),
        })
        .collect();
    string_lists.sort_by(|a, b| a.tag.cmp(&b.tag));
    Ui {
        widgets,
        string_lists,
        fonts: vec![font()],
        ..Ui::default()
    }
}

fn open(ui: &Ui, path: &str) -> Shell {
    let mut shell = Shell::new();
    let Some(_) = shell.open(ui, &mut NoHost, &tag(b"DeLa", path), NONE) else {
        panic!("{path} did not open");
    };
    shell
}

fn quads(items: &[DrawItem]) -> Vec<&Quad> {
    items
        .iter()
        .filter_map(|item| match item {
            DrawItem::Quad(quad) => Some(quad),
            DrawItem::Glyph(_) => None,
        })
        .collect()
}

fn glyphs(items: &[DrawItem]) -> Vec<&GlyphQuad> {
    items
        .iter()
        .filter_map(|item| match item {
            DrawItem::Glyph(glyph) => Some(glyph),
            DrawItem::Quad(_) => None,
        })
        .collect()
}

fn close(a: f32, b: f32) -> bool {
    (a - b).abs() < 1e-4
}

const SCREEN: Layout = Layout {
    width: 640.0,
    height: 480.0,
    scale: 1.0,
    left: 0.0,
    top: 0.0,
};

#[test]
fn the_screen_is_scaled_to_the_window_and_centred() {
    let same = Layout::for_window(640.0, 480.0);
    assert_eq!(same, SCREEN);

    let wide = Layout::for_window(1920.0, 1080.0);
    assert!(close(wide.scale, 2.25));
    assert!(close(wide.left, 240.0));
    assert!(close(wide.top, 0.0));
    assert_eq!(wide.to_window([0.0, 0.0]), [240.0, 0.0]);
    assert_eq!(wide.to_window([640.0, 480.0]), [1680.0, 1080.0]);
    assert_eq!(wide.to_screen([1680.0, 1080.0]), [640.0, 480.0]);
    assert_eq!(wide.rect([10, 20, 40, 120]), [285.0, 22.5, 510.0, 90.0]);

    let tall = Layout::for_window(640.0, 1000.0);
    assert!(close(tall.scale, 1.0));
    assert!(close(tall.left, 0.0));
    assert!(close(tall.top, 260.0));

    let uhd = Layout::for_window(3840.0, 2160.0);
    assert!(close(uhd.scale, 4.5));
    assert!(close(uhd.left, 480.0));

    let none = Layout::for_window(0.0, 0.0);
    assert_eq!(none.to_screen([5.0, 5.0]), [0.0, 0.0]);
}

#[test]
fn a_background_is_one_texel_to_a_pixel_in_its_bounds() {
    let mut panel = widget("panel", WidgetKind::Container);
    panel.background = Some(tag(b"bitm", "panel"));
    let ui = ui(vec![panel], &[]);
    let bitmaps = vec![bitmap("panel", 200, 60, 1)];
    let mut shell = open(&ui, "panel");
    let layout = Layout::for_window(1280.0, 960.0);
    let items = shell.draw(&ui, &bitmaps, &layout);
    let drawn = quads(&items);
    assert_eq!(drawn.len(), 1);
    assert_eq!(drawn[0].bitmap, tag(b"bitm", "panel"));
    assert_eq!(drawn[0].image, 1);
    assert_eq!(drawn[0].rect, [40.0, 20.0, 240.0, 80.0]);
    assert_eq!(drawn[0].uv, [0.0, 0.0, 0.5, 0.5]);
    assert_eq!(drawn[0].color, [1.0, 1.0, 1.0, 1.0]);

    // a bitmap the map doesn't have draws nothing
    let mut shell = open(&ui, "panel");
    assert!(shell.draw(&ui, &Vec::new(), &layout).is_empty());
}

#[test]
fn a_flat_fill_over_the_screen_covers_the_window() {
    let mut fill = widget("fill", WidgetKind::Container);
    fill.bounds = [0, 0, 480, 640];
    fill.background = Some(tag(b"bitm", "flat"));
    let mut picture = widget("picture", WidgetKind::Container);
    picture.bounds = [0, 0, 480, 640];
    picture.background = Some(tag(b"bitm", "picture"));
    let ui = ui(vec![fill, picture], &[]);
    let bitmaps = vec![bitmap("flat", 16, 16, 1), bitmap("picture", 640, 480, 1)];
    let layout = Layout::for_window(1920.0, 1080.0);

    let mut shell = open(&ui, "fill");
    let items = shell.draw(&ui, &bitmaps, &layout);
    assert_eq!(quads(&items)[0].rect, [0.0, 0.0, 1920.0, 1080.0]);

    let mut shell = open(&ui, "picture");
    let items = shell.draw(&ui, &bitmaps, &layout);
    assert_eq!(quads(&items)[0].rect, [240.0, 0.0, 1680.0, 1080.0]);
    assert_eq!(quads(&items)[0].uv, [0.0, 0.0, 1.0, 1.0]);
}

#[test]
fn a_flashing_background_fades_out_and_in() {
    let mut flash = widget("flash", WidgetKind::Container);
    flash.flags = Widget::FLASH_BACKGROUND_BITMAP;
    flash.background = Some(tag(b"bitm", "panel"));
    let ui = ui(vec![flash], &[]);
    let bitmaps = vec![bitmap("panel", 100, 30, 1)];
    let mut shell = open(&ui, "flash");

    shell.update(&ui, &mut NoHost, 0);
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    assert!(close(quads(&items)[0].color[0], 1.0));

    // cos(3t) = −1 at t = π/3 seconds
    shell.update(&ui, &mut NoHost, 1047);
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    assert!(quads(&items)[0].color[0] < 1e-3);

    shell.update(&ui, &mut NoHost, 524);
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    assert!((quads(&items)[0].color[0] - 0.5).abs() < 0.01);
}

#[test]
fn a_widget_is_as_opaque_as_it_and_its_parents_are() {
    let mut parent = widget("parent", WidgetKind::Container);
    parent.children = vec![child("child", [5, 7])];
    let mut kid = widget("child", WidgetKind::Container);
    kid.background = Some(tag(b"bitm", "panel"));
    let ui = ui(vec![parent, kid], &[]);
    let bitmaps = vec![bitmap("panel", 100, 30, 1)];
    let mut shell = open(&ui, "parent");
    let Some(root) = shell.root(0) else {
        panic!("no root");
    };
    let Some(&kid) = shell.get(root).map(|root| &root.children[0]) else {
        panic!("no child");
    };
    for id in [root, kid] {
        if let Some(instance) = shell.get_mut(id) {
            instance.alpha = 0.5;
        }
    }
    assert!(close(shell.cumulative_alpha(kid), 0.25));
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    let drawn = quads(&items);
    assert_eq!(drawn.len(), 1);
    assert!(close(drawn[0].color[0], 0.25));
    // the child's offset moves it
    assert_eq!(drawn[0].rect, [25.0, 17.0, 125.0, 47.0]);

    // a widget not visible draws nothing, nor do its children
    if let Some(instance) = shell.get_mut(root) {
        instance.visible = false;
    }
    assert!(shell.draw(&ui, &bitmaps, &SCREEN).is_empty());
}

#[test]
fn a_lists_focused_item_shows_its_highlight() {
    let mut menu = widget("menu", WidgetKind::ColumnList);
    menu.children = vec![
        child("item", [0, 0]),
        child("item", [0, 30]),
        child("plain", [0, 60]),
    ];
    let mut item = widget("item", WidgetKind::Container);
    item.background = Some(tag(b"bitm", "highlight"));
    let mut plain = widget("plain", WidgetKind::Container);
    plain.background = Some(tag(b"bitm", "three"));
    let mut coded = widget("coded", WidgetKind::ColumnList);
    coded.list.flags = LIST_ITEMS_FROM_CODE;
    coded.children = vec![child("item", [0, 0])];
    let ui = ui(vec![menu, item, plain, coded], &[]);
    let bitmaps = vec![bitmap("highlight", 100, 30, 2), bitmap("three", 100, 30, 3)];
    assert_eq!(frame_count(&bitmaps[0]), 2);

    let mut shell = open(&ui, "menu");
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    let images: Vec<usize> = quads(&items).iter().map(|quad| quad.image).collect();
    assert_eq!(images, [2, 1, 1]);

    let Some(root) = shell.root(0) else {
        panic!("no root");
    };
    shell.set_list_items(root, 3);
    shell.move_item(&ui, root, true);
    shell.move_item(&ui, root, true);
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    let images: Vec<usize> = quads(&items).iter().map(|quad| quad.image).collect();
    // a bitmap of three frames has no highlight
    assert_eq!(images, [1, 1, 1]);

    // a list whose items are made in code doesn't draw its children
    let mut shell = open(&ui, "coded");
    assert!(shell.draw(&ui, &bitmaps, &SCREEN).is_empty());
}

#[test]
fn a_spinner_lights_the_arrow_it_last_turned_by() {
    let mut spinner = widget("spinner", WidgetKind::SpinnerList);
    spinner.list = WidgetList {
        flags: LIST_ITEMS_FROM_STRING_LIST,
        header: Some(tag(b"bitm", "left")),
        footer: Some(tag(b"bitm", "right")),
        header_bounds: [0, 0, 10, 10],
        footer_bounds: [20, 0, 30, 10],
    };
    spinner.text = text("choices", 0);
    let ui = ui(vec![spinner], &[("choices", &["A", "AA", "A A"])]);
    let bitmaps = vec![bitmap("left", 10, 10, 2), bitmap("right", 10, 10, 2)];
    let mut shell = open(&ui, "spinner");
    let Some(root) = shell.root(0) else {
        panic!("no root");
    };

    let lit = |shell: &mut Shell| -> Vec<usize> {
        let items = shell.draw(&ui, &bitmaps, &SCREEN);
        quads(&items).iter().map(|quad| quad.image).collect()
    };
    assert_eq!(lit(&mut shell), [1, 1]);
    assert_eq!(shell.spinner_text(&ui, root).as_deref(), Some("A"));

    shell.move_item(&ui, root, true);
    assert_eq!(shell.spinner_text(&ui, root).as_deref(), Some("AA"));
    assert_eq!(lit(&mut shell), [1, 2]);
    for _ in 0..13 {
        lit(&mut shell);
    }
    assert_eq!(lit(&mut shell), [1, 2]);
    assert_eq!(lit(&mut shell), [1, 1]);

    shell.move_item(&ui, root, false);
    shell.move_item(&ui, root, false);
    assert_eq!(shell.spinner_text(&ui, root).as_deref(), Some("A A"));
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    assert_eq!(
        quads(&items)
            .iter()
            .map(|quad| quad.image)
            .collect::<Vec<_>>(),
        [2, 1]
    );
    // the spinner's text: its selected item, focused (the screen), not
    // moved by its text's offset
    let drawn = glyphs(&items);
    assert_eq!(drawn.len(), 2);
    assert_eq!(drawn[0].rect, [17.0, 8.0, 31.0, 24.0]);
    assert_eq!(drawn[0].color, [1.0, UI_WHITE, UI_WHITE, UI_WHITE]);

    // the arrows are moved with the spinner
    if let Some(instance) = shell.get_mut(root) {
        instance.offset = [100, 50];
    }
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    assert_eq!(quads(&items)[0].rect, [100.0, 50.0, 110.0, 60.0]);
}

#[test]
fn text_is_coloured_by_focus() {
    let mut menu = widget("menu", WidgetKind::ColumnList);
    menu.children = vec![
        child("label", [0, 0]),
        child("label", [0, 40]),
        child("white", [0, 80]),
    ];
    let mut label = widget("label", WidgetKind::TextBox);
    label.text = text("strings", 0);
    let mut white = widget("white", WidgetKind::TextBox);
    white.text = text("strings", 0);
    white.text.color = [0.5, 1.0, 1.0, 1.0];
    let ui = ui(vec![menu, label, white], &[("strings", &["A"])]);
    let mut shell = open(&ui, "menu");
    let items = shell.draw(&ui, &Vec::new(), &SCREEN);
    let colors: Vec<[f32; 4]> = glyphs(&items).iter().map(|glyph| glyph.color).collect();
    assert_eq!(
        colors,
        [
            [1.0, UI_WHITE, UI_WHITE, UI_WHITE],
            [1.0, 0.5, 0.25, 0.125],
            [0.5, UI_WHITE, UI_WHITE, UI_WHITE],
        ]
    );
}

#[test]
fn text_is_placed_by_its_offset_and_justification() {
    let mut boxes = Vec::new();
    for (path, justification) in [("left", 0), ("right", 1), ("centre", 2), ("bad", 3)] {
        let mut label = widget(path, WidgetKind::TextBox);
        label.text = text("strings", 0);
        label.text.offset = [5, 3];
        label.text.justification = justification;
        boxes.push(label);
    }
    let mut fontless = widget("fontless", WidgetKind::TextBox);
    fontless.text = text("strings", 0);
    fontless.text.font = Some(tag(b"font", "missing"));
    boxes.push(fontless);
    let ui = ui(boxes, &[("strings", &["AA"])]);
    let lefts = |path: &str| -> Vec<f32> {
        let mut shell = open(&ui, path);
        let items = shell.draw(&ui, &Vec::new(), &SCREEN);
        glyphs(&items).iter().map(|glyph| glyph.rect[0]).collect()
    };
    // the pen starts at the leading width in from the offset bounds; the
    // field's rectangle is the padding out from the coverage
    assert_eq!(lefts("left"), [22.0, 29.0]);
    assert_eq!(lefts("centre"), [61.0, 68.0]);
    assert_eq!(lefts("right"), [101.0, 108.0]);
    assert!(lefts("bad").is_empty());
    assert!(lefts("fontless").is_empty());

    let mut shell = open(&ui, "left");
    let items = shell.draw(&ui, &Vec::new(), &SCREEN);
    let drawn = glyphs(&items);
    assert_eq!(drawn[0].font, tag(b"font", "font"));
    assert_eq!(drawn[0].glyph, 0);
    // the baseline is the ascent below the top, the coverage's bottom on it
    assert_eq!(drawn[0].rect, [22.0, 11.0, 36.0, 27.0]);

    // twice the size in a window twice as large
    let mut shell = open(&ui, "left");
    let items = shell.draw(&ui, &Vec::new(), &Layout::for_window(1280.0, 960.0));
    assert_eq!(glyphs(&items)[0].rect, [44.0, 22.0, 72.0, 54.0]);
}

#[test]
fn flashing_text_pulses() {
    let mut label = widget("label", WidgetKind::TextBox);
    label.text = text("strings", 0);
    label.text.flags = TEXT_FLASHING;
    let mut spinner = widget("spinner", WidgetKind::SpinnerList);
    spinner.list.flags = LIST_ITEMS_FROM_STRING_LIST;
    spinner.text = text("strings", 0);
    spinner.text.flags = TEXT_FLASHING;
    let ui = ui(vec![label, spinner], &[("strings", &["A"])]);
    let alpha = |path: &str, ms: u64| -> f32 {
        let mut shell = open(&ui, path);
        shell.update(&ui, &mut NoHost, ms);
        let items = shell.draw(&ui, &Vec::new(), &SCREEN);
        glyphs(&items)[0].color[0]
    };
    assert!(close(alpha("label", 0), 1.0));
    assert!(close(alpha("label", 1047), 0.2));
    assert!(close(alpha("spinner", 0), 0.5));
    assert!(alpha("spinner", 524) > 0.99);
}

#[test]
fn lines_end_at_returns_and_at_the_right_edge() {
    let font = font();
    let rows = |text: &str, bounds: [i16; 4]| -> Vec<[f32; 2]> {
        layout_text(&font, text, bounds, 0)
            .iter()
            .map(|(_, rect)| [rect[0], rect[1]])
            .collect()
    };
    // "\r", "\r\n", "|n" and "\n" each end a line, 12 below the last
    assert_eq!(
        rows("A\r\nA|nA\nA\rA", [0, 0, 100, 100]),
        [
            [-3.0, -2.0],
            [-3.0, 10.0],
            [-3.0, 22.0],
            [-3.0, 34.0],
            [-3.0, 46.0]
        ]
    );
    // a character that reaches the right edge ends its line; lines past
    // the bottom are not drawn
    assert_eq!(
        rows("AAAAAAA", [10, 20, 40, 40]),
        [
            [17.0, 8.0],
            [24.0, 8.0],
            [31.0, 8.0],
            [17.0, 20.0],
            [24.0, 20.0],
            [31.0, 20.0]
        ]
    );
    // a space advances the pen but draws nothing; an unknown character
    // neither
    assert_eq!(
        rows("A A\u{263a}A", [0, 0, 100, 100]),
        [[-3.0, -2.0], [7.0, -2.0], [14.0, -2.0]]
    );
    assert!(rows("", [0, 0, 100, 100]).is_empty());
}

#[test]
fn a_sequence_names_its_images() {
    let mut picture = bitmap("picture", 8, 8, 2);
    assert_eq!(sequence_image(&picture, 0), Some(1));
    assert_eq!(sequence_image(&picture, 1), Some(2));
    assert_eq!(sequence_image(&picture, 9), Some(2));
    picture.sequences[0].first_image = None;
    assert_eq!(sequence_image(&picture, 0), None);
    picture.sequences[0].image_count = 0;
    assert_eq!(sequence_image(&picture, 0), None);
    picture.sequences[0].first_image = Some(5);
    picture.sequences[0].image_count = 1;
    assert_eq!(sequence_image(&picture, 0), None);
    picture.sequences.clear();
    assert_eq!(frame_count(&picture), 3);
    assert_eq!(sequence_image(&picture, 2), Some(2));
    assert_eq!(sequence_image(&picture, 3), None);
}

#[test]
fn a_list_of_strings_draws_a_line_for_each() {
    let mut list = widget("list", WidgetKind::ColumnList);
    list.list.flags = LIST_ITEMS_FROM_STRING_LIST;
    list.background = Some(tag(b"bitm", "panel"));
    list.text = text("lines", 0);
    let ui = ui(vec![list], &[("lines", &["A", "AA", "A A"])]);
    let bitmaps = vec![bitmap("panel", 100, 30, 1)];
    let mut shell = open(&ui, "list");
    let items = shell.draw(&ui, &bitmaps, &SCREEN);
    // the list's own background, once, and its items' text
    assert_eq!(quads(&items).len(), 1);
    assert_eq!(glyphs(&items).len(), 5);
}
