"""Check changelog packaging, navigation and the generated presentation."""

import json
import copy
import os
from pathlib import Path
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

from PIL import Image, ImageFont
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import changelog_menu

MENUS = Path(__file__).resolve().parents[1] / "port/assets/menus"
ROOT = MENUS.parents[2]


def feature():
    return ET.parse(MENUS / "ce" / changelog_menu.FILE).getroot()


def notes_data(notes=None):
    return {"schema_version": 2, "head": "a" * 40, "entries": [
        {"build": 12, "date": "2026-10-18", "commit": "a" * 40,
         "notes": notes if notes is not None else ["A change for players."]}]}


def widgets_for(data):
    root = ET.fromstring("<menus>" + "\n".join(changelog_menu.widget_lines(data)) + "</menus>")
    return {node.get("name"): node for node in root.findall("widget")}


def test_focused_generation_needs_no_pc_tags(monkeypatch):
    import ce_menus
    calls = []
    monkeypatch.setattr(sys, "argv", ["ce_menus.py", "--changelog-only"])
    monkeypatch.setattr(changelog_menu, "regenerate", lambda: calls.append(True))
    ce_menus.main()
    assert calls == [True]


def test_shipped_widgets_match_generator():
    expected = ET.fromstring("<menus>" + "\n".join(changelog_menu.widget_lines()) + "</menus>")
    actual = feature()
    for root in (expected, actual):
        for node in root.iter():
            node.tail = None
    for element in ("widget", "strings"):
        assert [ET.tostring(widget) for widget in actual.findall(element)] == [
            ET.tostring(widget) for widget in expected.findall(element)]


def test_full_generation_uses_the_same_real_data(monkeypatch, tmp_path):
    (tmp_path / "ce").mkdir()
    monkeypatch.setattr(changelog_menu, "MENUS", tmp_path)
    monkeypatch.setattr(changelog_menu.port_settings, "menu_item_frames", lambda *args: [])

    class Art:
        redraws = None

        def port_picture(self, name, frames):
            return f'<bitmap name="{name}"/>'

        def title(self, name, text):
            return f'<bitmap name="{name}"/>'

    data = notes_data(["Recorded player improvement."])
    changelog_menu.generate(Art(), data)
    actual = ET.parse(tmp_path / "ce" / changelog_menu.FILE).getroot()
    expected = widgets_for(data)
    assert {node.get("name"): node.get("text") for node in actual.findall("widget")} == {
        name: node.get("text") for name, node in expected.items()}


def test_feature_files_are_embedded_and_navigation_resolves():
    manifest = json.loads((MENUS / "menus.json").read_text())["files"]
    assert f"ce/{changelog_menu.FILE}" in manifest
    assert len(manifest) == len(set(manifest))
    widgets, bitmaps = {}, {}
    for file in manifest:
        if not file.endswith(".xml"):
            continue
        root = ET.parse(MENUS / file).getroot()
        for node in root.findall("widget"):
            assert node.get("name") not in widgets
            widgets[node.get("name")] = node
        for node in root.findall("bitmap"):
            assert node.get("name") not in bitmaps
            bitmaps[node.get("name")] = node
    for node in feature().findall("widget"):
        if node.get("bitmap"):
            assert node.get("bitmap") in bitmaps
        for child in node:
            for attribute in ("widget", "open", "replace"):
                if child.get(attribute):
                    assert child.get(attribute) in widgets
            assert child.get("run") in (None, "mouse emit accept event", "port changelog init",
                                        "port changelog up", "port changelog down", "port gulch build label")
    for bitmap in feature().findall("bitmap"):
        for frame in bitmap:
            assert frame.get("png") in manifest
            with Image.open(MENUS / frame.get("png")) as image:
                assert image.mode == "RGBA"
                assert image.width == int(frame.get("width")) * 4
                assert image.height == int(frame.get("height")) * 4
                assert image.getchannel("A").getbbox()


def test_builds_have_real_labels_back_and_history_preserving_navigation():
    data = notes_data([f"Player change {i}." for i in range(30)])
    data["entries"] += [dict(data["entries"][0], build=11), dict(data["entries"][0], build=10)]
    widgets = widgets_for(data)
    for number in range(1, 4):
        name = f"{changelog_menu.BASE}/build_{number}"
        page = widgets[name]
        events = {event: handler for handler in page.findall("on") for event in handler.get("event").split()}
        assert events["b"].get("back") == "true"
        assert events["back"].get("back") == "true"
        assert events["start"].get("back") == "true"
        if number < 3:
            assert events["left"].get("open") == f"{changelog_menu.BASE}/build_{number + 1}"
        else:
            assert "left" not in events
        if number > 1:
            assert events["right"].get("open") == f"{changelog_menu.BASE}/build_{number - 1}"
        else:
            assert "right" not in events
        assert "no_history" in page.get("flags").split()
        assert widgets[f"{name}/label"].get("text") == "PLAYER NOTES"
        assert widgets[f"{name}/heading"].get("text") == f"BUILD {13 - number}"
        assert widgets[f"{name}/date"].get("text") == "2026-10-18"
        assert widgets[f"{name}/button_back"].find("on").get("back") == "true"
        assert "pass_unhandled_to_focused_child" in widgets[f"{name}/buttons"].get("flags")
        assert "tabs" not in widgets[f"{name}/buttons"].get("flags")
        for direction in ("up", "down"):
            assert events[direction].get("run") == f"port changelog {direction}"
            assert events[f"stick_{direction}"].get("run") == f"port changelog {direction}"
            assert events[direction].get("open") is None
        assert events["created"].get("run") == "port changelog init"
        for node in [widgets[child.get("widget")] for child in page.findall("child")]:
            if node.get("text"):
                assert node.get("font") in ("ui\\small_ui", "ui\\large_ui")
        # ui_widget_load_by_name_or_tag (source/interface/ui_widget.c,
        # invoking_widget_tag history decision) tests the invoking root's
        # no_history flag. Open plus no_history preserves the original main
        # menu history entry across any number of page changes.
        assert all(handler.get("replace") is None for handler in page.findall("on"))
        for node in widgets.values():
            if node.get("name", "").startswith(name + "/"):
                assert all(handler.get("replace") is None for handler in node.findall("on"))
                for handler in node.findall("on"):
                    if handler.get("open"):
                        assert handler.get("open") in (
                            f"{changelog_menu.BASE}/build_{number - 1}",
                            f"{changelog_menu.BASE}/build_{number + 1}")
        main = ET.parse(MENUS / "ce/main_menu.xml").getroot().find("widget[@name='main_menu/main_menu']")
        assert "no_history" not in main.get("flags", "").split()


def test_reading_area_and_controls_fit_the_menu_canvas():
    data = notes_data(["W" * 100, "Fixed " + "a" * 94] * 10)
    widgets = widgets_for(data)

    def visit(name, x=0, y=0):
        node = widgets[name]
        left = x + int(node.get("left", 0))
        top = y + int(node.get("top", 0))
        assert 0 <= left < left + int(node.get("width", 640)) <= 640
        assert 0 <= top < top + int(node.get("height", 480)) <= 480
        for child in node.findall("child"):
            visit(child.get("widget"), left + int(child.get("x", 0)), top + int(child.get("y", 0)))

    visit(f"{changelog_menu.BASE}/build_1")


def test_long_notes_are_complete_in_one_build_with_fixed_visible_rows():
    notes = ["W" * 100, "Fixed " + "a" * 94, "a " * 49 + "a"] * 3
    data = notes_data(notes)
    contents = changelog_menu.reading_lines(data["entries"][0])
    assert len(contents) > 8
    assert "".join(line[2:] for line in contents if line).replace(" ", "") == "".join(notes).replace(" ", "")
    assert all(sum(2 if c.isupper() or c in "mw@%&#" else 1 for c in line) <= changelog_menu.NOTE_COLUMNS for line in contents)
    widgets = widgets_for(data)
    assert f"{changelog_menu.BASE}/build_2" not in widgets
    for row in range(changelog_menu.VISIBLE_LINES):
        node = widgets[f"{changelog_menu.BASE}/build_1/row_{row}"]
        assert node.get("string_list") == f"{changelog_menu.BASE}/build_1/lines"
        assert int(node.get("string_index")) == row
        assert int(node.get("height")) == changelog_menu.NOTE_LINE_HEIGHT


def test_reading_width_and_bottom_right_arrow_pair():
    # ui.map's small_ui capitals are 10 logical pixels; use the bundled
    # font without the runtime's 98% narrowing for a conservative bound.
    font = ImageFont.truetype(str(ROOT / "port/assets/fonts/Overpass-750.ttf"), 1000)
    box = font.getbbox("H")
    scale = 10 / (box[3] - box[1])
    per_column = max(font.getlength(chr(c)) * scale /
                     (2 if chr(c).isupper() or chr(c) in "mw@%&#" else 1) for c in range(32, 127))
    assert per_column * changelog_menu.NOTE_COLUMNS <= 420
    assert max(font.getlength(c) for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") * 24 * scale <= 420
    data = notes_data(["0" * 100, "W" * 100])
    data["entries"][0]["categories"] = ["W" * 24, ""]
    widgets = widgets_for(data)
    screen = widgets[f"{changelog_menu.BASE}/build_1"]
    children = {child.get("widget").rsplit("/", 1)[-1]: child for child in screen.findall("child")}
    controls = children["scroll_controls"]
    assert controls.get("x") == "480" and controls.get("y") == "314"
    column = widgets[f"{changelog_menu.BASE}/build_1/scroll_controls"]
    assert column.get("type") == "column_list"
    assert "tabs" not in column.get("flags")
    up, down = column.findall("child")
    assert int(up.get("y", 0)) == 0 and down.get("y") == "28"
    assert int(down.get("y")) - int(up.get("y", 0)) - 24 == 4
    for name in ("scroll_up", "scroll_down"):
        assert widgets[f"{changelog_menu.BASE}/build_1/{name}"].get("width") == "128"


@pytest.mark.parametrize("empty_entries", [True, False])
def test_empty_notes_are_truthful(empty_entries):
    data = notes_data([])
    if empty_entries:
        data["entries"] = []
    text = "\n".join(changelog_menu.widget_lines(data))
    assert "No player-facing changes recorded." in text
    assert "SAMPLE" not in text
    assert "build_2" not in text


def test_notes_are_xml_escaped():
    data = notes_data(['Improved <HUD> & "controls".'])
    root = ET.fromstring("<menus>" + "\n".join(changelog_menu.widget_lines(data)) + "</menus>")
    assert next(node.get("text") for node in root.find("strings") if node.get("text")) == '- Improved <HUD> & "controls".'


def test_categories_are_blue_grouped_and_notes_have_no_blank_separator():
    data = notes_data(["First interface change.", "A convenience.", "Second interface change.", "Another change."])
    data["entries"][0]["categories"] = ["INTERFACE", "QUALITY OF LIFE", "INTERFACE", ""]
    rows = changelog_menu.reading_rows(data["entries"][0])
    assert rows == [("", "INTERFACE"), ("- First interface change.", ""),
                    ("- Second interface change.", ""), ("", ""),
                    ("", "QUALITY OF LIFE"), ("- A convenience.", ""), ("", ""),
                    ("", "GENERAL"), ("- Another change.", "")]
    widgets = widgets_for(data)
    for row in range(changelog_menu.VISIBLE_LINES):
        body = widgets[f"{changelog_menu.BASE}/build_1/row_{row}"]
        heading = widgets[f"{changelog_menu.BASE}/build_1/heading_row_{row}"]
        assert body.get("top") == heading.get("top")
        assert body.get("string_index") == heading.get("string_index")
        assert heading.get("color") == "#FF2896FF"
        assert heading.get("string_list").endswith("/headings")


def test_oversized_wrapped_build_is_rejected_before_serializing_widgets():
    data = notes_data([("W" * 22 + " ") * 4] * 4096)
    # Strip the trailing separator; every accepted note remains under100 chars.
    data["entries"][0]["notes"] = [note.strip() for note in data["entries"][0]["notes"]]
    data["entries"][0]["categories"] = [f"GROUP {index}" for index in range(4096)]
    with pytest.raises(ValueError, match="32767-line"):
        changelog_menu.widget_lines(data)


@pytest.mark.parametrize("change", [
    lambda d: d.update(schema_version=1),
    lambda d: d.update(schema_version=True),
    lambda d: d.update(head="bad"),
    lambda d: d.update(entries={}),
    lambda d: d.update(extra="unsupported"),
    lambda d: d["entries"][0].update(notes=["Too many notes."] * 4097),
    lambda d: d["entries"][0].update(build="12"),
    lambda d: d["entries"][0].update(build=True),
    lambda d: d["entries"][0].update(build=0),
    lambda d: d["entries"][0].update(build=-1),
    lambda d: d["entries"].append(dict(d["entries"][0])),
    lambda d: d["entries"][0].update(commit="bad"),
    lambda d: d["entries"][0].update(date="2026-02-30"),
    lambda d: d["entries"][0].update(date="20261018"),
    lambda d: d["entries"][0].update(notes="not a list"),
    *[lambda d, labels=labels: d["entries"][0].update(categories=labels) for labels in
      ("FIXES", [], ["lowercase"], [" FIXES"], ["A  B"], ["0FIXES"], ["A" * 25], ["A\\n"], [1])],
    *[lambda d, note=note: d["entries"][0].update(notes=[note]) for note in
      ("", "x" * 101, "newline\nhere", "tab\there", "a\\n", "a|b", "caf\u00e9", " leading", "trailing ", "\x00")],
])
def test_malformed_notes_are_rejected(change):
    data = copy.deepcopy(notes_data())
    change(data)
    with pytest.raises(ValueError):
        changelog_menu.widget_lines(data)


def test_text_only_cli_uses_no_art_dependencies_and_preserves_bitmap_xml(tmp_path):
    # A fresh isolated interpreter makes accidental heavy imports fail,
    # even when the surrounding pytest process already loaded Pillow.
    menus = tmp_path / "menus"
    (menus / "ce").mkdir(parents=True)
    target = menus / "ce" / changelog_menu.FILE
    original = (MENUS / "ce" / changelog_menu.FILE).read_text(encoding="utf-8")
    target.write_text(original, encoding="utf-8")
    notes = tmp_path / "notes.json"
    notes.write_text(json.dumps(notes_data()), encoding="utf-8")
    script = r'''
import importlib.abc, sys
from pathlib import Path
class BlockArt(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        if fullname.split('.')[0] in ('ce_menus', 'numpy', 'PIL', 'fontTools'):
            raise AssertionError('art dependency imported: ' + fullname)
sys.meta_path.insert(0, BlockArt())
sys.path.insert(0, sys.argv[1])
import changelog_menu
changelog_menu.MENUS = Path(sys.argv[2])
sys.argv = ['changelog_menu.py', '--notes', sys.argv[3], '--text-only']
# Exercise the CLI with its module globals redirected to an isolated tree.
code = Path(changelog_menu.__file__).read_text()
exec(compile(code[code.index('if __name__ == "__main__":'):], changelog_menu.__file__, 'exec'),
     dict(vars(changelog_menu), __name__='__main__'))
'''
    command = [sys.executable, "-S", "-c", script, str(Path(changelog_menu.__file__).parent), str(menus), str(notes)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    first = target.read_text(encoding="utf-8")
    assert changelog_menu.re.findall(r'\t<bitmap\b[^>]*>.*?</bitmap>', first, changelog_menu.re.DOTALL) == changelog_menu.re.findall(
        r'\t<bitmap\b[^>]*>.*?</bitmap>', original, changelog_menu.re.DOTALL)
    subprocess.run(command, check=True, capture_output=True, text=True)
    assert target.read_text(encoding="utf-8") == first


def test_insertion_preserves_legacy_menu_and_is_idempotent():
    main = (MENUS / "ce/main_menu.xml").read_text()
    baseline = main.replace("\t\t" + changelog_menu.CHILD + "\n", "")
    assert changelog_menu.insert_item(baseline) == main
    assert changelog_menu.insert_item(main) == main
    rows = ET.fromstring(main).find("widget[@name='main_menu/main_menu_select_list']").findall("child")
    assert [row.get("widget") for row in rows[-2:]] == [
        "main_menu/main_menu_item_mods", "main_menu/main_menu_item_quit_game"]
    assert [int(row.get("y")) for row in rows] == [247, 283, 319, 355, 391, 427]
    root = ET.fromstring(main).find("widget[@name='main_menu/main_menu']")
    assert root.find(f"child[@widget='{changelog_menu.CORNER}']").attrib == {
        "widget": changelog_menu.CORNER, "x": "447", "y": "438"}
    label = root.find(f"child[@widget='{changelog_menu.BUILD_LABEL}']")
    assert label.get("x") == "424" and label.get("y") == "420"
    label_widget = feature().find(f"widget[@name='{changelog_menu.BUILD_LABEL}']")
    assert label_widget.get("align") == "center" and label_widget.get("width") == "150"
    assert int(label.get("x")) + 150 / 2 == 499
    assert 447 + changelog_menu.CORNER_WIDTH / 2 == 499
    corner = feature().find(f"widget[@name='{changelog_menu.CORNER}']")
    assert corner.get("type") == "column_list"
    assert corner.find("child").get("widget") == changelog_menu.ITEM
    assert corner.get("width") == "104" and corner.get("height") == "21"
    item = feature().find(f"widget[@name='{changelog_menu.ITEM}']")
    assert item.get("width") == "104" and item.get("height") == "21"


def test_corner_art_scales_the_whole_design_without_changing_its_groups():
    frames = changelog_menu.corner_frames(MENUS / "svg")
    assert len(frames) == 2
    for index, (svg, width, height) in enumerate(frames):
        root = ET.fromstring(svg)
        assert (width, height) == (104, 21)
        assert root.get("viewBox") == "0 0 160 32"
        assert root.get("preserveAspectRatio") == "xMinYMin meet"
        # Keeping both design groups unchanged scales the caption, outline
        # and glow together; fitting the outline separately would restyle it.
        groups = list(root)
        assert [group.get("transform") for group in groups] == [
            "translate(0,3) scale(1.25,1)", "translate(2,4) scale(0.6)"]
        expected_border = ET.parse(MENUS / f"svg/shell/bitmaps/text_button_background__{index}.svg").getroot()
        expected_caption = ET.fromstring(changelog_menu.port_settings.menu_item_frames(
            "CHANGELOG", MENUS / "svg", changelog_menu.SPACING)[index][0])
        for actual, expected in zip(groups, (expected_border, expected_caption)):
            for node in (*actual.iter(), *expected.iter()):
                node.tail = None
                if node.text and not node.text.strip():
                    node.text = None
            assert [ET.tostring(child) for child in actual] == [ET.tostring(child) for child in expected]


def c_function(source, name):
    """Take the real function body for asset-free input routing checks."""
    start = source.rindex(name)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def test_real_scroll_runtime_and_existing_wheel_route(tmp_path):
    cc = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not cc:
        pytest.skip("a C compiler is needed for the scroll runtime test")
    engine = (ROOT / "source/interface/ui_widget.c").read_text()
    wheel = c_function(engine, "static struct widget_instance *ui_mouse_wheel_widget(")
    directions = c_function(engine, "static void ui_mouse_list_directions(")
    mouse_item = c_function(engine, "static boolean ui_mouse_widget_is_item(")
    code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
#define FALSE 0
#define TRUE 1
#define NONE (-1)
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define PIN(a,b,c) MIN(MAX(a,b),c)
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define FLAG(a) (1L << (a))
#define TEST_FLAG(a,b) ((a) & FLAG(b))
#define UNICODE_STRING_LIST_TAG 1
enum { _widget_dpad_updown_tabs_thru_children_bit=3,
 _widget_dpad_leftright_tabs_thru_children_bit,
 _widget_dpad_updown_tabs_thru_list_items_bit,
 _widget_dpad_leftright_tabs_thru_list_items_bit,
 _widget_pass_unhandled_events_to_children_bit };
enum { _ui_widget_type_spinner_list=2, _ui_widget_type_column_list=3 };
enum { _widget_event_dpad_up=8, _widget_event_dpad_down,
 _widget_event_dpad_left, _widget_event_dpad_right };
struct widget_instance {
 long definition_tag_index; char const *name; short type; boolean disabled;
 struct widget_instance *parent, *child, *next, *focused_child;
 union { struct { short selected_index; void *list_items; long number_of_items; } list;
         struct { short string_list_index; } text_box; } parameters;
 struct { short current_frame_index, number_of_sprite_frames; } animation;
};
struct ui_widget_definition { long flags; struct { long count; } event_handlers; } definitions[6];
struct string_list { struct { long count; } strings; } strings, headings;
static struct ui_widget_definition *ui_widget_definition_get(long n) { return &definitions[n]; }
static boolean ui_mouse_list_shows_several(struct widget_instance *w) { (void)w; return FALSE; }
static long ui_mouse_child_index(struct widget_instance *w) { return w && w->parent ? 0 : NONE; }
static boolean widget_instance_can_receive_events(struct widget_instance *w) { return !w->disabled; }
char *tag_get_name(long n) {
 return n==1 ? "pc\\main_menu\\changelog\\build_1" : n==2 ? "pc\\main_menu\\changelog\\build_2" : n==4 ? "pc\\main_menu\\main_menu" : "pc\\main_menu\\settings";
}
static int missing_tag, missing_heading;
static long heading_count=-1;
static long tag_loaded(long type, char const *name) {
 (void)type;
 if(strstr(name,"\\headings")) return missing_heading ? NONE : 2;
 assert(strstr(name,"\\lines")); return missing_tag ? NONE : 1;
}
static struct string_list *test_strings(long n) {
 if(n==1) return &strings;
 assert(n==2); headings.strings.count=heading_count<0 ? strings.strings.count : heading_count; return &headings;
}
#define unicode_string_list_definition_get(n) test_strings(n)
static wchar_t position[64];
static struct widget_instance *named(struct widget_instance *root, char const *name, long nth) {
 struct widget_instance *child;
 if(!root) return NULL;
 for(child=root->child;child;child=child->next)
  if(!strcmp(child->name,name) && nth--==0) return child;
 return NULL;
}
static void text_set(struct widget_instance *text, wchar_t const *value) {
 long n=0; if(text) do { position[n]=value[n]; } while(value[n++]);
}
static int position_is(char const *value) {
 long n=0; do { if(position[n]!=(unsigned char)value[n]) return 0; } while(value[n++]); return 1;
}
static long halo_menus_utf16(char const *text, unsigned short *out, long capacity) {
 long n=0; while(text[n] && n+1<capacity) { out[n]=(unsigned char)text[n]; n++; } out[n]=0; return n+1;
}
''' + wheel + "\n" + directions + "\n" + mouse_item + '\n#include "changelog_menu_runtime.inl"\n' + r'''
int main(void) {
 struct widget_instance root={0}, other={0}, nodes[12]={{0}}, other_nodes[12], footer={0}, button={0};
 struct widget_instance heading_nodes[8]={{0}}, other_headings[8];
 struct widget_instance main={0}, central={0}, corner={0}, item={0}, rows[6]={{0}};
 struct widget_instance controls={0}, arrows[2]={{0}};
 char names[8][16], heading_names[8][24]; long n; short back,forward;
 root.definition_tag_index=1; other.definition_tag_index=2;
 root.child=nodes; other.child=other_nodes;
 for(n=0;n<12;n++) {
  nodes[n].parent=&root; nodes[n].type=1; nodes[n].next=n<11 ? &nodes[n+1] : NULL;
  if(n<8) { snprintf(names[n],sizeof(names[n]),"row_%ld",n); nodes[n].name=names[n]; }
 }
 nodes[8].name="scroll_position";nodes[9].name="scroll_up";
 nodes[10].name="scroll_down";nodes[11].name="unused";
 nodes[11].next=heading_nodes;
 for(n=0;n<8;n++) {
  snprintf(heading_names[n],sizeof(heading_names[n]),"heading_row_%ld",n);
  heading_nodes[n].name=heading_names[n];heading_nodes[n].parent=&root;heading_nodes[n].type=1;
  heading_nodes[n].next=n<7 ? &heading_nodes[n+1] : NULL;
 }
 memcpy(other_nodes,nodes,sizeof(nodes));
 for(n=0;n<12;n++) { other_nodes[n].parent=&other; other_nodes[n].next=n<11 ? &other_nodes[n+1] : NULL; }
 memcpy(other_headings,heading_nodes,sizeof(heading_nodes));other_nodes[11].next=other_headings;
 for(n=0;n<8;n++) { other_headings[n].parent=&other;other_headings[n].next=n<7 ? &other_headings[n+1] : NULL; }
 strings.strings.count=28;
 assert(changelog_menu_event(&root,"port changelog init"));
 assert(root.parameters.list.selected_index==0 && nodes[9].disabled && !nodes[10].disabled);
 assert(position_is("Lines 1-8 of 20"));
 for(n=0;n<100;n++) assert(changelog_menu_event(&nodes[10],"port changelog down"));
 assert(root.parameters.list.selected_index==12 && nodes[10].disabled && !nodes[9].disabled);
 for(n=0;n<8;n++) { assert(nodes[n].parameters.text_box.string_list_index==12+n); assert(heading_nodes[n].parameters.text_box.string_list_index==12+n); }
 assert(other.parameters.list.selected_index==0);
 assert(changelog_menu_event(&other,"port changelog init"));
 assert(root.parameters.list.selected_index==12 && other.parameters.list.selected_index==0);
 for(n=0;n<8;n++) { assert(nodes[n].parameters.text_box.string_list_index==12+n); assert(other_nodes[n].parameters.text_box.string_list_index==n); assert(heading_nodes[n].parameters.text_box.string_list_index==12+n); assert(other_headings[n].parameters.text_box.string_list_index==n); }
 heading_count=27;assert(!changelog_menu_event(&root,"port changelog down"));heading_count=-1;
 missing_heading=1;assert(!changelog_menu_event(&root,"port changelog down"));missing_heading=0;
 assert(changelog_menu_event(&root,"port changelog init"));
 assert(root.parameters.list.selected_index==0);
 for(n=0;n<100;n++) assert(changelog_menu_event(&root,"port changelog up"));
 assert(root.parameters.list.selected_index==0);
 for(n=0;n<=8;n++) { strings.strings.count=n; assert(!changelog_menu_event(&root,"port changelog init")); }
 strings.strings.count=32768; assert(!changelog_menu_event(&root,"port changelog down"));
 strings.strings.count=9; assert(changelog_menu_event(&root,"port changelog down"));
 assert(root.parameters.list.selected_index==0 && nodes[9].disabled && nodes[10].disabled);
 missing_tag=1; assert(!changelog_menu_event(&root,"port changelog init"));missing_tag=0;
 root.definition_tag_index=3; assert(!changelog_menu_event(&root,"port changelog down"));root.definition_tag_index=1;
 root.type=1; assert(!changelog_menu_event(&root,"port changelog down"));root.type=0;
 assert(!changelog_menu_event(&root,"unknown"));
 /* The exact engine wheel functions see no tab flags in the changelog's
    footer, including when mouse hover/click has focused a footer button. */
 footer.type=3;footer.definition_tag_index=4;button.type=1;button.definition_tag_index=5;
 root.focused_child=&footer;footer.focused_child=&button;
 assert(ui_mouse_wheel_widget(&root,FALSE)==NULL);
 ui_mouse_list_directions(&footer,&back,&forward);
 assert(back==_widget_event_dpad_up && forward==_widget_event_dpad_down);
 definitions[4].flags=FLAG(_widget_dpad_leftright_tabs_thru_list_items_bit);
 assert(ui_mouse_wheel_widget(&root,FALSE)==&footer);
 ui_mouse_list_directions(&footer,&back,&forward);
 assert(back==_widget_event_dpad_left && forward==_widget_event_dpad_right);
 root.child=&footer;footer.name="buttons";footer.child=&button;footer.next=&controls;
 controls.name="scroll_controls";controls.type=3;controls.child=arrows;
 arrows[0].next=&arrows[1];arrows[0].disabled=TRUE;
 arrows[0].animation.number_of_sprite_frames=arrows[1].animation.number_of_sprite_frames=2;
 button.animation.number_of_sprite_frames=2;button.animation.current_frame_index=1;
 arrows[0].animation.current_frame_index=1;controls.focused_child=&arrows[1];root.focused_child=&controls;
 changelog_menu_focus_update(&root);
 assert(button.animation.current_frame_index==0 && arrows[0].animation.current_frame_index==0);
 assert(arrows[1].animation.current_frame_index==1);
 root.focused_child=&footer;changelog_menu_focus_update(&root);
 assert(button.animation.current_frame_index==1 && arrows[1].animation.current_frame_index==0);
 root.definition_tag_index=3;arrows[1].animation.current_frame_index=1;
 changelog_menu_focus_update(&root);assert(arrows[1].animation.current_frame_index==1);
 /* Right/Left preserve the six-row list and its remembered selection.
    A mouse transition can clear focused_child without clearing the index. */
 main.definition_tag_index=4;main.child=&central;main.focused_child=&central;
 central.name="main_menu_select_list";central.type=3;central.parent=&main;
 central.next=&corner;central.child=rows;central.parameters.list.selected_index=5;
 for(n=0;n<6;n++) { rows[n].parent=&central;rows[n].next=n<5 ? &rows[n+1] : NULL; }
 central.focused_child=&rows[5];rows[5].animation.number_of_sprite_frames=2;
 rows[5].animation.current_frame_index=1;corner.name="changelog_corner";corner.type=3;corner.parent=&main;
 corner.child=&item;item.name="main_menu_item_changelog";item.parent=&corner;
 assert(changelog_menu_event(&rows[5],"port changelog corner focus"));
 assert(main.focused_child==&corner && central.parameters.list.selected_index==5);
 assert(central.focused_child==NULL);
 assert(ui_mouse_widget_is_item(&item));
 /* Standalone root buttons were not mouse targets. The arrows now
    belong to a column list, preserving the engine's existing contract. */
 button.parent=&main;assert(!ui_mouse_widget_is_item(&button));
 button.parent=&footer;assert(ui_mouse_widget_is_item(&button));
 button.disabled=TRUE;assert(!ui_mouse_widget_is_item(&button));button.disabled=FALSE;
 changelog_menu_focus_update(&main);assert(item.animation.current_frame_index==1);
 assert(rows[5].animation.current_frame_index==0);
 central.focused_child=NULL;
 assert(changelog_menu_event(&item,"port changelog main focus"));
 assert(main.focused_child==&central && central.focused_child==&rows[5]);
 assert(rows[5].animation.current_frame_index==1);
 changelog_menu_focus_update(&main);assert(item.animation.current_frame_index==0);
 /* Mouse focus uses the engine's direct focus path, not our event helper. */
 main.focused_child=&corner;corner.focused_child=&item;central.focused_child=NULL;
 rows[5].animation.current_frame_index=1;
 changelog_menu_focus_update(&main);assert(rows[5].animation.current_frame_index==0);
 main.definition_tag_index=3;assert(!changelog_menu_event(&main,"port changelog corner focus"));
 main.definition_tag_index=4;central.next=NULL;
 assert(!changelog_menu_event(&main,"port changelog corner focus"));
 return 0;
}
'''
    source = tmp_path / "scroll.c"
    binary = tmp_path / ("scroll.exe" if os.name == "nt" else "scroll")
    source.write_text(code)
    command = [cc, "-std=gnu89", "-fshort-wchar", "-Wall", "-Wextra", "-Werror", "-iquote", str(ROOT / "source"),
               "-iquote", str(ROOT / "port/linux/game"), str(source), "-o", str(binary)]
    if os.name == "nt":
        command.insert(1, "--target=i686-pc-windows-msvc")
    built = subprocess.run(command, capture_output=True, text=True)
    assert built.returncode == 0, built.stdout + built.stderr
    subprocess.run([str(binary)], check=True, capture_output=True, text=True)
