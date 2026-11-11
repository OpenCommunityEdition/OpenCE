"""Tests for the native Linux build tooling (port/linux, tools/linux_*.py)."""

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

from tools import linux_build, linux_link_check, linux_msvc_semantics
from tools.harness import biped_riding_sources

# for tests that compile for the Linux target (and run what they build)
linux_host_only = pytest.mark.skipif(not sys.platform.startswith("linux"),
                                     reason="builds for the Linux target, so needs a Linux host")


def test_vehicle_riding_port_hooks():
    """Riding implementation lives in the port; native movement keeps ordered hooks."""
    root = Path(__file__).resolve().parent.parent
    bipeds = (root / "source/units/bipeds.c").read_text(encoding="latin-1")
    assert not re.search(r"^static .* biped_riding_\w+\(", bipeds, re.M)
    moving = riding_function(bipeds, "biped_update_moving")
    riding_order(moving, "biped_get_physics_pill(",
                 "biped_riding_prepare(biped_index, biped, definition, &physics, &riding_tick);",
                 "biped_update_physics(&physics);",
                 "biped_riding_finish(biped_index, biped, definition, &physics, &riding_tick);",
                 "object_translate(biped_index,")


def test_vehicle_riding_update_count():
    """Every object update counts once (object_update calls object_count_update first in its do-not-update test),
    and objects_update runs its main loop through objects_update_passes (vehicle_riding.c)."""
    root = Path(__file__).resolve().parent.parent
    objects = (root / "source/objects/objects.c").read_text(encoding="latin-1")
    update = objects[objects.index("static boolean object_update("):]
    update = update[:update.index("\n}\n")]
    assert re.search(r"if \(!TEST_FLAG\(header->flags, _object_header_do_not_update_bit\)\)\n\t\{\n"
                     r"(\t\t/\*[^\n]*\*/\n)?\t\tobject_count_update\(object_index\);", update)
    loop = objects[objects.index("void objects_update(\n"):]
    loop = loop[:loop.index("\n}\n")]
    assert "objects_update_passes(dont_update_object, object_update_deferred, object_update);" in loop


def test_vehicle_riding_contacts_last_one_tick():
    """The vehicles' touches of riders are kept for one game tick: objects_update clears them before its main loop,
    so a rider whose own tick never came takes none into the next; a rider's tick resolves them after its air
    frame, from the velocity its physics moves it by (vehicle_riding.c)."""
    root = Path(__file__).resolve().parent.parent
    objects = (root / "source/objects/objects.c").read_text(encoding="latin-1")
    loop = objects[objects.index("void objects_update(\n"):]
    loop = loop[:loop.index("\n}\n")]
    assert "vehicle_riding_contacts_clear();" in loop
    assert loop.index("vehicle_riding_contacts_clear();") < \
        loop.index("objects_update_passes(dont_update_object, object_update_deferred, object_update);")
    # resolved after the seed and the air frame, from the velocity the physics moves the rider by: resolved
    # before them, a change of the air frame along the touch would be counted twice
    bipeds = biped_riding_sources()
    moving = bipeds[bipeds.index("static void biped_update_moving(\n"):]
    moving = moving[:moving.index("\n}\n")]
    contacts = moving.index("biped_riding_contacts(")
    assert moving.index("vehicle_riding_air_frame(") < contacts < moving.index("biped_update_physics(&physics);")
    assert "riding_carried, &physics.velocity, riding_player," in moving[contacts:contacts + 300]


def test_vehicle_collision_damage_factored():
    """The original vehicle collision damage lives in one function, which a vehicle's update and a rider's tick both
    call along the original push, so a crush in the rider's tick is the original's (physics.c, bipeds.c)."""
    root = Path(__file__).resolve().parent.parent

    def body(text, signature):
        start = text.index(signature)
        return text[start:text.index("\n}\n", start)]

    physics = (root / "source/physics/physics.c").read_text(encoding="latin-1")
    assert "struct game_globals_falling_damage" not in \
        (root / "port/linux/game/vehicle_riding_bipeds.c").read_text(encoding="latin-1")
    damage = body(physics, "void physics_biped_vehicle_collision_damage(\n")
    assert physics.count("vehicle_killed_unit_damage_effect.index") == 2
    assert damage.count("vehicle_killed_unit_damage_effect.index") == 2
    assert damage.count("vehicle_collision_damage.index") == 2
    collision = body(physics, "static boolean physics_compute_biped_collision(\n\tstruct collision_model_instance *instance,"
                              "\n\tlong biped_index)\n{")
    assert "physics_biped_vehicle_collision_push(instance->object_index, biped_index, &acceleration);" in collision
    assert "physics_biped_vehicle_collision_damage(instance->object_index, biped_index, &acceleration);" in collision
    bipeds = biped_riding_sources()
    contacts = body(bipeds, "static boolean biped_riding_contacts(\n")
    assert contacts.index("physics_biped_vehicle_collision_push(") < \
        contacts.index("physics_biped_vehicle_collision_damage(")


# ---------- MSVC semantics header


def test_strip_cplusplus_keeps_c_branches_only():
    text = "\n".join([
        "a",
        "#ifdef __cplusplus", "cpp1", "#else", "c1", "#endif",
        "#if FOO", "b", "#else", "c", "#endif",
        "#ifndef __cplusplus", "c2", "#else", "cpp2", "#endif",
        "#if defined(__cplusplus)", "cpp3", "#if X", "cpp4", "#endif", "#endif",
        "#ifdef _WIN64", "win64", "#endif",
        "end",
    ])
    assert linux_msvc_semantics.strip_cplusplus(text).split() == ["a", "c1", "b", "c", "c2", "end"]


def write(path: Path, text: str) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="latin-1")
    return path


def test_scan_finds_prototype_scope_tags_and_inline_functions(tmp_path):
    header = write(tmp_path / "a.h", """
        struct location;
        void f(struct scenario *s, union color *c);
        /* struct commented_out */
        __inline long prototyped(long x) { return x; }
        __inline long private_helper(long x) { return x; }
        long prototyped(long x);
        #ifdef __cplusplus
        __inline long cpp_only(long x) { return x; }
        #endif
    """)
    files = [header]
    tags = linux_msvc_semantics.scan_tags(files)
    assert tags == {"location": {"struct"}, "scenario": {"struct"}, "color": {"union"}}
    assert linux_msvc_semantics.scan_inline_functions(files, all_inlines=False) == {"prototyped"}
    assert linux_msvc_semantics.scan_inline_functions(files, all_inlines=True) == {
        "prototyped", "private_helper",
    }


def test_render_skips_tags_used_as_both_struct_and_union():
    text = linux_msvc_semantics.render(
        {"a": {"struct"}, "b": {"union"}, "mixed": {"struct", "union"}}, {"f"}
    )
    assert "struct a;" in text
    assert "union b;" in text
    assert "mixed" not in text
    assert "#pragma weak f" in text


# ---------- Xbox SDK declarations (port/include/xdk)


XDK_INCLUDE = Path(__file__).resolve().parent.parent / "port" / "include" / "xdk"


def test_xdk_headers_use_the_sdk_spellings():
    # each port gives the SDK's keywords and names its own meaning (its
    # prefix header), so none of one port's must be written into them
    for header in sorted(XDK_INCLUDE.glob("*.h")):
        text = re.sub(r"/\*.*?\*/", "", header.read_text(encoding="utf-8"), flags=re.S)
        assert "__attribute__" not in text, header.name
        assert not re.findall(r"\bhalo_\w+", text), header.name


@linux_host_only
@pytest.mark.skipif(shutil.which("clang") is None, reason="clang is needed to compile the headers")
def test_xdk_headers_compile_for_the_game(tmp_path):
    root = XDK_INCLUDE.parent.parent.parent
    source = write(tmp_path / "unit.c", "".join(
        f"#include <{name}>\n" for name in ("xtl.h", "xbdm.h", "xkbd.h", "d3d8perf.h")
    ))
    flags = [flag for flag in linux_build.LINUX_ABI_FLAGS + linux_build.GAME_FLAGS if flag != "-w"]
    for defines in ([], ["-DDEBUG_KEYBOARD"], ["-DNOD3D", "-DNODSOUND"]):
        subprocess.run(
            ["clang", *flags, *defines, "-Werror", "-fsyntax-only",
             "-include", str(root / "port/linux/include/halo_linux_prefix.h"),
             "-I", str(root / "port/linux/include"), "-idirafter", str(XDK_INCLUDE), str(source)],
            check=True,
        )


def pdb_writer():
    """An xdk_headers Writer over an empty type table: basic types only."""
    from tools import pdb200_types, xdk_headers
    pdb = object.__new__(pdb200_types.Pdb)
    pdb.types, pdb.aggregates = {}, {}
    return xdk_headers, xdk_headers.Writer(pdb)


def test_xdk_headers_regroups_anonymous_members():
    # the PDB lists an anonymous structure's or union's members in their
    # container, at their offsets
    xdk_headers, writer = pdb_writer()
    member = xdk_headers.Member
    ulong, int64 = 0x22, 0x13
    union = writer.render(writer.group(
        [member("LowPart", ulong, 0), member("HighPart", ulong, 4), member("QuadPart", int64, 0)], True), "", "u")
    assert union == ["struct {", "    unsigned long LowPart;", "    unsigned long HighPart;", "};",
                     "__int64 QuadPart;"]
    structure = writer.render(writer.group(
        [member("Tag", ulong, 0), member("Low", ulong, 4), member("High", ulong, 8),
         member("Whole", int64, 4), member("After", ulong, 12)], False), "", "s")
    assert structure == ["unsigned long Tag;", "union {", "    struct {", "        unsigned long Low;",
                         "        unsigned long High;", "    };", "    __int64 Whole;", "};",
                         "unsigned long After;"]


# ---------- weak reference link check


@pytest.mark.skipif(shutil.which("clang") is None or shutil.which("nm") is None,
                    reason="clang and nm are needed to build test objects")
def test_link_check_rejects_undefined_weak_references(tmp_path):
    def compile_object(name: str, text: str) -> Path:
        source = write(tmp_path / f"{name}.c", text)
        output = tmp_path / f"{name}.o"
        subprocess.run(["clang", "-c", "-o", str(output), str(source)], check=True)
        return output

    caller = compile_object("caller", "#pragma weak helper\nint helper(int);\nint call(void) { return helper(1); }\n")
    local = compile_object("local", "static int helper(int x) { return x; }\nint use(void) { return helper(2); }\n")
    provider = compile_object("provider", "#pragma weak helper\nint helper(int x) { return x; }\n")

    def check(*objects: Path) -> int:
        response = write(tmp_path / "objects.rsp", "\n".join(f"'{o}'" for o in objects))
        return subprocess.run(
            [sys.executable, linux_link_check.__file__, str(response)], capture_output=True
        ).returncode

    # a file-local copy elsewhere does not satisfy the reference
    assert check(caller, local) == 1
    assert check(caller, local, provider) == 0


# ---------- game sources


def test_game_sources_leave_out_the_excluded(tmp_path, monkeypatch):
    write(tmp_path / "source" / "a.c", "")
    write(tmp_path / "source" / "zlib" / "b.c", "")
    write(tmp_path / "source" / "zlib" / "example.c", "")
    write(tmp_path / "source" / "tiff" / "c.c", "")
    write(tmp_path / "source" / "tiff" / "deep" / "d.c", "")
    write(tmp_path / "source" / "tiffx.c", "")
    write(tmp_path / "source" / "a.h", "")
    monkeypatch.chdir(tmp_path)
    # (a folder is a name ending in "/": not a file that only starts like it)
    config = {"game": {"root": "source", "exclude": ["source/zlib/example.c", "source/tiff/"],
                       "defines": ["DEBUG"], "include_dirs": ["source", "source/saved games"]}}
    assert linux_build.game_sources(config) == [Path("source/a.c"), Path("source/tiffx.c"), Path("source/zlib/b.c")]
    assert linux_build.game_defines_and_includes(config) == '-DDEBUG -Isource -I"source/saved games"'


# ---------- menus (port/assets/menus)


MENUS = Path(__file__).resolve().parent.parent / "port/assets/menus"
MENU_ATTRIBUTES = {
    "menus": {"root"},
    "bitmap": {"name", "width", "height", "frames", "platform"},
    "frame": {"png", "map", "index", "width", "height", "x", "y", "platform"},
    "strings": {"name", "map", "platform"},
    "string": {"text", "map", "platform"},
    "widget": {"name", "type", "controller", "flags", "bitmap", "text", "strings", "values", "setting", "font",
               "color", "align", "description", "string_list", "list_flags", "text_flags", "header_bitmap",
               "footer_bitmap", "header_bounds", "footer_bounds", "child_controller", "x", "y", "left", "top",
               "width", "height", "text_x", "text_y", "auto_close", "auto_close_fade", "string_index", "platform"},
    "child": {"widget", "controller", "x", "y", "platform"},
    "on": {"event", "run", "script", "open", "replace", "close", "widget", "focus", "reload", "sound", "otherwise",
           "label", "back", "branch", "platform"},
    "data": {"input", "platform"},
    "conditional": {"widget", "if_failed", "platform"},
    "replace": {"search", "function", "platform"},
}


def c_strings(path: Path, start: str, end: str) -> list:
    """the string literals of a C file between two markers"""
    text = path.read_text(encoding="latin-1")
    text = text[text.index(start):]
    return re.findall(r'"([^"\\]*)"', text[:text.index(end)])


# menu_files.c's attributes read as whole numbers (its tables' { name, NULL, &long })
MENU_INTEGER_ATTRIBUTES = {"auto_close", "auto_close_fade", "height", "index", "left", "string_index", "text_x",
                           "text_y", "top", "width", "x", "y"}


def test_menus_are_well_formed():
    """What port/linux/src/menu_files.c and port/linux/game/menu_tags.c check
    when the game loads them, but the map's own names (paths with backslashes),
    which only the map has."""
    import json
    import xml.etree.ElementTree as ElementTree

    root = MENUS.parent.parent.parent
    tags = root / "port/linux/game/menu_tags.c"
    events = c_strings(tags, "event_names[] =", "};")
    flags = c_strings(tags, "widget_flag_names[] =", "};")
    functions = set(c_strings(root / "source/interface/ui_widget_event_handler_functions.c", '\t{\n\t\t"NULL",', "}"))
    functions |= set(c_strings(tags, "port_function_names[] =", "};"))
    inputs = set(c_strings(tags, "game_data_input_names[] =", "};"))
    inputs |= set(c_strings(tags, "port_game_data_input_names[] =", "};"))
    listed = json.loads((MENUS / "menus.json").read_text())["files"]
    files = sorted(MENUS.rglob("*.xml"))
    assert sorted(path.relative_to(MENUS).as_posix() for path in files) == \
        sorted(name for name in listed if name.endswith(".xml"))
    widgets, bitmaps, strings, references, roots = {}, {}, {}, [], []
    for path in files:
        tree = ElementTree.parse(path)
        assert tree.getroot().tag == "menus", path
        if tree.getroot().get("root"):
            roots.append(tree.getroot().get("root"))
        for element in tree.getroot().iter():
            where = f"{path.name}: <{element.tag} {element.get('name', '')}>"
            assert element.tag in MENU_ATTRIBUTES, where
            assert set(element.attrib) <= MENU_ATTRIBUTES[element.tag], where
            # (the platforms that show it: menu_files.c's MENU_PLATFORM)
            assert element.get("platform") is None or \
                set(element.get("platform").split()) <= {"desktop", "android", "web"}, where
            # (menu_files.c's whole numbers, which the tags keep in shorts,
            # its true/false attributes, and no text but whitespace outside
            # <string>s)
            for attribute in MENU_INTEGER_ATTRIBUTES & set(element.attrib):
                value = element.get(attribute)
                assert re.fullmatch(r"-?[0-9]+", value) and -32768 <= int(value) <= 32767, f"{where} {attribute}"
            for attribute in ("back", "branch", "if_failed"):
                assert element.get(attribute) in (None, "true", "false"), f"{where} {attribute}"
            assert not (element.text or "").strip() and not (element.tail or "").strip(), where
            if element.tag == "bitmap":
                bitmaps[element.get("name")] = element
                for frame in element.iter("frame"):
                    assert (frame.get("png") is None) != (frame.get("map") is None), where
                    if frame.get("map"):
                        assert "\\" in frame.get("map") and int(frame.get("index")) >= 0, where
                        continue
                    assert frame.get("png") in listed, where
                    with (MENUS / frame.get("png")).open("rb") as png:
                        header = png.read(24)
                    width, height = int.from_bytes(header[16:20], "big"), int.from_bytes(header[20:24], "big")
                    logical = int(frame.get("width")), int(frame.get("height"))
                    assert width % logical[0] == 0 and width // logical[0] == height // logical[1], where
            elif element.tag == "strings":
                strings[element.get("name")] = element
                # (a string's map= is its number in the list's map list, menu_tags.c)
                for string in element.findall("string"):
                    assert string.get("map") is None or (element.get("map") and string.get("map").isdigit()), where
            elif element.tag == "widget":
                name = element.get("name")
                assert name and name not in widgets, where
                widgets[name] = element
                assert element.get("type", "container") in ("container", "text", "spinner", "column_list"), where
                assert set(element.get("flags", "").split()) <= set(flags), where
                children = element.findall("widget") + element.findall("child")
                if element.get("type") == "spinner":
                    assert len(children) in (0, 1, 3), where
                if element.get("strings") or "items_from_strings" in element.get("list_flags", ""):
                    assert element.get("type") == "spinner" and not children, where
                if element.get("setting"):
                    assert len(element.get("strings").split("|")) == len(element.get("values").split("|")), where
                for attribute in ("bitmap", "header_bitmap", "footer_bitmap"):
                    references.append((where, bitmaps, element.get(attribute)))
                references.append((where, strings, element.get("string_list")))
                references.append((where, widgets, element.get("description")))
            elif element.tag == "child" or element.tag == "conditional":
                references.append((where, widgets, element.get("widget")))
            elif element.tag == "on":
                assert set(element.get("event").split()) <= set(events), where
                run = element.get("run")
                assert run is None or run in functions or run.startswith("unwired "), where
                assert element.get("otherwise") is None or run, where
                for attribute in ("open", "replace", "focus", "widget", "otherwise"):
                    references.append((where, widgets, element.get(attribute)))
            elif element.tag == "data":
                assert element.get("input") in inputs or element.get("input").startswith("unwired "), where
    assert all(root_name in widgets for root_name in roots)
    for where, names, name in references:
        if name and "\\" not in name:
            assert name in names, f"{where} {name}"


def test_menu_settings_exist():
    """Every setting a menu's spinner sets is one of config.toml's
    (port/linux/src/port_config.c), of the profile or an action of a row's
    (menu_functions.c), and
    the keyboard's controls are the same in the settings, in the controls'
    screen and in the input code."""
    root = MENUS.parent.parent.parent
    config = (root / "port/linux/src/port_config.c").read_text()
    functions = (root / "port/linux/game/menu_functions.c").read_text()
    known = set(re.findall(r'^\t\{ "([a-z_]+\.[a-z_]+)", _config_', config, re.M))
    profile = set(re.findall(r'\{ "(profile\.[a-z_]+)", \d', functions))
    # (and the actions a row does on OK rather than a setting: DISCORD ACCOUNT's LINK, setting_write's)
    actions = set(re.findall(r'!strcmp\(name, "([a-z_]+\.action)"\)', functions))
    for path in (MENUS / "ce").glob("*.xml"):
        for setting in re.findall(r'setting="([^"]+)"', path.read_text()):
            assert setting in known | profile | actions, f"{path.name}: {setting}"
    controls = set(re.findall(r'"(controls\.[a-z_]+)"', (root / "port/linux/src/xinput_sdl.c").read_text()))
    assert controls == {name for name in known if name.startswith("controls.")}
    assert controls == set(re.findall(r'\{ "(controls\.[a-z_]+)", L"', functions))


def assert_vehicle_riding_names(config, commands, sources):
    entry = config.split('{ "network.vehicle_riding",', 1)[1].split('},', 1)[0]
    assert '{ "network.vehicle_riding", _config_boolean, "false", "HALO_NET_VEHICLE_RIDING", _environment_value, _platform_all,' in config
    override = '{ "server.vehicle_riding_override_gametype", _config_boolean, "false", "HALO_SERVER_VEHICLE_RIDING_OVERRIDE_GAMETYPE",'
    assert override in config
    assert "_platform_server" in config.split(override, 1)[1].split('},', 1)[0]
    help_text = entry.split("_platform_all,", 1)[1]
    assert help_text.lstrip().startswith('"Host:')
    assert "gametype" in help_text and "co-op" in help_text
    assert '{ "game.vehicle_riding"' not in config
    block = commands.split("settable_settings[] =", 1)[1].split("};", 1)[0]
    assert '"network.vehicle_riding"' in block and '"server.vehicle_riding_override_gametype"' in block
    for text in sources:
        assert '"game.vehicle_riding"' not in text


def riding_names_texts():
    root = Path(__file__).resolve().parent.parent
    paths = [p for top in ("port", "source") for suffix in ("*.c", "*.h") for p in (root / top).rglob(suffix)]
    paths += [p for p in (root / "tools/harness").rglob("*") if p.is_file() and p.suffix in (".c", ".h", ".py")]
    return ((root / "port/linux/src/port_config.c").read_text(),
            (root / "port/linux/game/server_commands.c").read_text(),
            [p.read_text(encoding="utf-8", errors="replace") for p in paths])


def test_vehicle_riding_names():
    """The host's option is network.vehicle_riding, the dedicated override is settable, nothing reads the old name."""
    assert_vehicle_riding_names(*riding_names_texts())


@pytest.mark.parametrize("target,before,after", [
    ("sources", None, "int config_boolean(const char *name) { return !strcmp(name, \"game.vehicle_riding\"); }"),
    ("commands", '"server.vehicle_riding_override_gametype"', '"server.removed"'),
    ("config", '"Host: the base', '"the base'),
], ids=["old-name-in-fixture", "override-not-settable", "help-not-host"])
def test_vehicle_riding_names_negative_control(target, before, after):
    config, commands, sources = riding_names_texts()
    assert_vehicle_riding_names(config, commands, sources)
    if target == "sources":
        sources = sources + [after]
    elif target == "commands":
        assert before in commands
        commands = commands.replace(before, after)
    else:
        assert before in config
        config = config.replace(before, after)
    with pytest.raises(AssertionError):
        assert_vehicle_riding_names(config, commands, sources)


def assert_vehicle_riding_settings(config, readme):
    names = set(re.findall(r'^\t\{ "([a-z_]+\.[a-z_]+)", _config_', config, re.M))
    assert "network.vehicle_riding" in names and "`network.vehicle_riding`" in readme
    assert '{ "network.vehicle_riding", _config_boolean, "false",' in config
    assert "Server Setup's VEHICLE" not in config
    help_text = config.split('{ "network.vehicle_riding",', 1)[1].split('},', 1)[0]
    assert "gametype" in help_text
    assert re.search(r"\| `network\.vehicle_riding` \| `false` \|", readme)
    assert "Split-screen multiplayer, film playback and the editor never ride" not in readme
    for tunable in ("riding_damage_speed", "riding_teleport_step", "riding_moved_alone", "riding_transition_leash", "riding_look_carry",
                    "riding_air_grip", "riding_enclosure_grace_ticks"):
        assert f"debug.{tunable}" in names and f"`debug.{tunable}`" in readme
        assert f"game.{tunable}" not in names
    riding_rows = "\n".join(line for line in readme.splitlines() if line.startswith("| `debug.riding_"))
    assert "stay settings" in riding_rows
    assert "`0`, `4` or `0.25`" in riding_rows
    # the air grip is an acceleration (world units a tick, each tick): its default, the same in all three places
    assert '{ "debug.riding_air_grip", _config_real, "0.045",' in config
    assert "`0`, `4`, `0.25`, `0.25`, `true`, `0.045`" in readme
    assert '{ "debug.riding_transition_leash", _config_real, "0.25", "HALO_RIDING_TRANSITION_LEASH", _environment_value, _platform_all,' in config
    # the cabin's grace (player ticks, 0-30): a real, so the latch sees fractions and not-a-number
    assert '{ "debug.riding_enclosure_grace_ticks", _config_real, "4", "HALO_RIDING_ENCLOSURE_GRACE_TICKS",' in config
    assert "`debug.riding_enclosure_grace_ticks` | `4` | `HALO_RIDING_ENCLOSURE_GRACE_TICKS`" in readme


def test_vehicle_riding_settings():
    """Riding defaults off; its help and documented debug tunables agree with the config."""
    root = Path(__file__).resolve().parent.parent
    config = (root / "port/linux/src/port_config.c").read_text()
    readme = (root / "port/linux/README.md").read_text(encoding="utf-8")
    assert_vehicle_riding_settings(config, readme)
    riding = (root / "port/linux/game/vehicle_riding.c").read_text()
    assert "static real riding_air_grip = .045f;" in riding
    assert "static short riding_cabin_grace = RIDING_CABIN_GRACE;" in riding


@pytest.mark.parametrize("target,before,after", [
    ("config", '{ "network.vehicle_riding", _config_boolean, "false",',
     '{ "network.vehicle_riding", _config_boolean, "true",'),
    ("readme", "| `network.vehicle_riding` | `false` |", "| `network.vehicle_riding` | `true` |"),
    ("config", "co-op's options in Server Setup", "Server Setup's VEHICLE RIDING"),
    ("config", "gametype", "Other Options"),
    ("readme", "Film playback and the editor never ride", "Split-screen multiplayer, film playback and the editor never ride"),
    *[("readme", f"`debug.riding_{name}`", f"`removed.riding_{name}`") for name in
      ("damage_speed", "teleport_step", "moved_alone", "transition_leash", "look_carry", "air_grip",
       "enclosure_grace_ticks")],
    ("readme", "stay settings", "are constants"),
    ("readme", "`0`, `4` or `0.25`", "`1`, `5` or `0.5`"),
], ids=["config-default-true", "readme-default-true", "help-old-menu-name", "help-no-vehicle-options",
        "split-screen-disabled", "missing-damage-speed", "missing-teleport-step", "missing-moved-alone",
        "missing-transition-leash", "missing-look-carry", "missing-air-grip", "missing-enclosure-grace",
        "tunables-not-settings", "wrong-fallbacks"])
def test_vehicle_riding_settings_negative_control(target, before, after):
    root = Path(__file__).resolve().parent.parent
    texts = {"config": (root / "port/linux/src/port_config.c").read_text(),
             "readme": (root / "port/linux/README.md").read_text(encoding="utf-8")}
    assert_vehicle_riding_settings(**texts)
    assert before in texts[target], f"control no longer applies: {before!r}"
    texts[target] = texts[target].replace(before, after)
    with pytest.raises(AssertionError):
        assert_vehicle_riding_settings(**texts)


def test_default_brokers_are_brokers_txt():
    """The brokers a game uses with no brokers.txt beside its config.toml
    (p2p_signal.c's DEFAULT_BROKERS) are port/assets/network/brokers.txt's."""
    root = MENUS.parent.parent.parent
    listed = [line.split("#", 1)[0].strip()
              for line in (root / "port/assets/network/brokers.txt").read_text().splitlines()]
    default = re.search(r'^#define DEFAULT_BROKERS "([^"]*)"',
                        (root / "port/linux/src/p2p_signal.c").read_text(), re.M).group(1)
    assert default.split(",") == [line for line in listed if line]


@linux_host_only
def test_p2p_signatures_and_listings(tmp_path):
    """internet play's Ed25519 (RFC 8032), the X25519 key of a seed, and the
    server browser's listings from host to browser (tools/p2p_lobby_check.c),
    built with the flags ninja gives the platform layer"""
    import shlex

    if not shutil.which("clang") or not shutil.which("ninja") or not Path("build.ninja").is_file():
        pytest.skip("needs clang, ninja and a configured build")
    command = subprocess.run(["ninja", "-t", "commands", "build/linux/obj/port/linux/src/p2p_crypto.o"],
                             capture_output=True, text=True, check=True).stdout.strip().splitlines()[-1]
    words = shlex.split(command)
    # (the compiler, and whatever runs it, ccache in CI: up to the first flag)
    while words and not words[0].startswith("-"):
        words = words[1:]
    flags = []
    skip = False
    for word in words:
        if skip:
            skip = False
        elif word in ("-MF", "-o", "-c"):
            skip = True
        elif word == "-MMD" or word.startswith("-flto") or word.startswith("-fprofile-use"):
            continue
        else:
            flags.append(word)
    program = tmp_path / "p2p_lobby_check"
    built = subprocess.run(["clang", *flags, "-O1", "-no-pie", "-Wl,--unresolved-symbols=ignore-all", "-o",
                            str(program), "tools/p2p_lobby_check.c", "port/linux/src/p2p_crypto.c",
                            "port/linux/src/p2p_lobby.c", "port/third_party/monocypher/monocypher.c",
                            "port/third_party/monocypher/monocypher-ed25519.c"],
                           capture_output=True, text=True)
    assert built.returncode == 0, built.stderr[-4000:]
    result = subprocess.run([str(program)], capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout
    assert "PASS" in result.stdout



# port_config.c on its own, with the app's file paths (HALO_DATA_ROOT) and C
# library calls: "get <setting>" prints it, "write <setting> <value>" sets it
CONFIG_PROGRAM = r"""
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "port_config.h"

void platform_log(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
}

int main(int argc, char **argv)
{
    char text[256];

    if (argc == 3 && !strcmp(argv[1], "get") && config_text(argv[2], text, sizeof(text)))
        return puts(text) < 0;
    if (argc == 4 && !strcmp(argv[1], "write"))
        return !config_write(argv[2], argv[3]);
    return 1;
}
"""


def test_config_has_each_section_once(tmp_path):
    """config.toml names each section once, as TOML asks: the defaults the
    game writes, and a file an older version wrote with [display] twice
    (which went unread), joined"""
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("needs a C compiler")
    root = Path(__file__).resolve().parent.parent
    stubs = tmp_path / "stubs"
    (stubs / "SDL3").mkdir(parents=True)
    (stubs / "SDL3" / "SDL.h").write_text("")
    (stubs / "platform.h").write_text("void platform_log(const char *format, ...);\n")
    # (beside the stubs, which it then includes instead of the platform layer's)
    for name in ("port_config.c", "port_config.h"):
        shutil.copy(root / "port/linux/src" / name, stubs / name)
    (tmp_path / "main.c").write_text(CONFIG_PROGRAM)
    program = tmp_path / "config_program"
    subprocess.run([compiler, "-DHALO_ANDROID=1", "-DHALO_ARM64_GUEST=1", f"-I{stubs}",
                    f"-I{root}/port/third_party/tomlc17", "-o", str(program),
                    str(tmp_path / "main.c"), str(stubs / "port_config.c"),
                    str(root / "port/third_party/tomlc17/tomlc17.c"), "-lpthread"], check=True)
    data = tmp_path / "data"
    data.mkdir()
    config = data / "config.toml"

    def run(*command):
        result = subprocess.run([str(program), *command], capture_output=True, text=True,
                                env={**os.environ, "HALO_DATA_ROOT": str(data)})
        assert result.returncode == 0, result.stderr
        assert "using the defaults" not in result.stderr, result.stderr
        return result.stdout.strip(), result.stderr

    def sections():
        return re.findall(r"^\[(\w+)\]$", config.read_text(), re.M)

    # the defaults
    assert run("get", "display.first_person_legs")[0] == "false"
    assert len(sections()) == len(set(sections())), sections()
    run("write", "display.first_person_legs", "true")
    assert run("get", "display.first_person_legs")[0] == "true"

    # MCC discovery starts enabled; an explicit later opt-out stays saved.
    for key in ("game.workshop_maps", "game.mcc_campaigns", "game.mcc_collections"):
        assert run("get", key)[0] == "true"
        run("write", key, "false")
        assert run("get", key)[0] == "false"

    # an older version's
    config.write_text("[display]\nvsync = false\n\n[game]\nconsole_log = \"all\"\n\n"
                      "[display]\nfirst_person_legs = true\n")
    value, log = run("get", "display.first_person_legs")
    assert value == "true" and "joined" in log
    assert sections().count("display") == 1
    assert run("get", "display.vsync")[0] == "false"
    assert run("get", "game.console_log")[0] == "all"


# ---------- system link's stand-in machines (tools/system_link_bots.py)


def test_system_link_bots_messages_are_the_games():
    """the stand-in machines number their messages as network_messages.h does,
    and send each at the size network_messages.c's fields decode (pad 0,
    bytes 1, shorts 2, longs 4; an array a count byte and one element)"""
    import struct
    import time
    import types
    from tools import system_link_bots as bots

    root = Path(__file__).resolve().parents[1]
    header = (root / "source/networking/network_messages.h").read_text()
    enum = header[header.index("enum network_game_message_type"):]
    names = re.findall(r"\t_message_(\w+)", enum[:enum.index("};")])
    for name, value in vars(bots).items():
        if name.lower() in names:
            assert names.index(name.lower()) == value, name
    sizes = {}
    source = (root / "source/networking/network_messages.c").read_text()
    for fields, name in re.findall(r"\t\{\n((?:\t\t.*\n)*?)\t\},\n\tNETWORK_GAME_MESSAGE_DEFINITION\((\w+),", source):
        sizes[name] = sum(int(count) * {"pad": 0, "bytes": 1, "shorts": 2, "longs": 4}.get(kind, 0) + (kind == "array")
                          for kind, count in re.findall(r"_data_packet_field_(\w+), (\d+)", fields))

    from tools import network_format
    assert bots.NETWORK_VERSION == network_format.version()
    sent = []
    machine = bots.Machine(1, "127.0.0.2", "127.0.0.1", print)
    machine.udp.close()
    machine.send = sent.append
    machine.udp = types.SimpleNamespace(sendto=lambda data, address: sent.append(data))
    machine.joined()
    machine.handle(bots.SERVER_MACHINE_ACCEPTED, struct.pack(">ih", 0, 1))
    settings = bytes(0x24) + b"levels\\test\\bloodgulch\\bloodgulch".ljust(0x80, b"\0")
    machine.handle(bots.SERVER_GAME_SETTINGS_UPDATE, struct.pack(">HHH", len(settings), 0, len(settings)) + settings)
    machine.tick(time.monotonic())
    machine.handle(bots.SERVER_BEGIN_GAME, b"")
    machine.tick(time.monotonic() + 60)
    machine.handle(bots.SERVER_GAME_UPDATE, struct.pack(">I", 1))
    machine.tick(time.monotonic())
    assert len(sent) == 6
    for data in sent:
        name = names[data[-1]]
        assert len(data) - 4 == sizes[name], name


# ---------- the wire format (tools/network_format.py)


def test_network_format_is_the_recorded_one():
    """what the machines send each other is what port/linux/network_format.txt
    records for HALO_PORT_NETWORK_VERSION, and NETCODE.md says what that
    version changed"""
    from tools import network_format

    problems = network_format.check()
    assert not problems, "\n".join(problems)


# ---------- the tag validator (port/linux/game/tag_validate.c)

RETAIL_MAPS = Path("assets/maps")
MAP_VALIDATE = Path("build/linux/map_validate")


def _retail_maps():
    maps = sorted(RETAIL_MAPS.glob("*.map"))
    if not maps or not MAP_VALIDATE.is_file():
        pytest.skip("needs the retail maps in assets/maps and ninja linux's build/linux/map_validate")
    return maps


def test_retail_maps_need_no_corrections():
    """every retail map in assets/maps passes the tag validator, each of its
    structure bsps too, with no correction: the schemas (tag_schema_*.c)
    say what the game's own maps hold"""
    maps = _retail_maps()
    result = subprocess.run([str(MAP_VALIDATE), "--strict", *map(str, maps)], capture_output=True, text=True,
                            timeout=1800)
    assert result.returncode == 0, result.stdout[-6000:]


def test_tag_validator_survives_damaged_maps():
    """retail maps with a few of their tags' words changed at random: the
    validator never crashes or hangs, and a map it lets through is clean
    when checked again"""
    maps = _retail_maps()
    chosen = [path for path in maps if path.stem in ("bloodgulch", "a10", "ui")] or maps[:1]
    result = subprocess.run([str(MAP_VALIDATE), "--fuzz", "300", "--seed", "1", *map(str, chosen)],
                            capture_output=True, text=True, timeout=1800)
    assert result.returncode == 0, (result.stdout + result.stderr)[-6000:]


# ---------- Custom Edition maps (port/linux/game/cache_file_formats.c, custom_edition_cache.c)

def _custom_edition_maps():
    """the Custom Edition maps (HALO_CUSTOM_EDITION_MAPS, or the gitignored
    assets/custom_edition), with the bitmaps.map, sounds.map and loc.map
    they need beside them"""
    configured = os.environ.get("HALO_CUSTOM_EDITION_MAPS")
    folder = Path(configured) if configured else Path("assets/custom_edition")
    maps = [path for path in sorted(folder.glob("*.map"))
            if path.stem.lower() not in ("bitmaps", "sounds", "loc")]
    if not maps or not (folder / "bitmaps.map").is_file() or not MAP_VALIDATE.is_file():
        pytest.skip("needs Custom Edition maps (HALO_CUSTOM_EDITION_MAPS or assets/custom_edition) and "
                    "ninja linux's build/linux/map_validate")
    return maps


def test_custom_edition_maps_are_loaded_and_checked():
    """every Custom Edition map is loaded, converted and passed by the tag
    validator, each of its structure bsps too: none is refused (a community
    map's own mistakes are corrected and logged)"""
    maps = _custom_edition_maps()
    result = subprocess.run([str(MAP_VALIDATE), *map(str, maps)], capture_output=True, text=True, timeout=3600)
    assert result.returncode == 0, result.stdout[-6000:]
    assert "refused" not in result.stdout, result.stdout[-6000:]


def test_custom_edition_loader_and_validator_survive_damaged_maps():
    """Custom Edition maps with a few of their file's words changed at random:
    the loader, the conversion and the validator never crash or hang, and a
    map they let through is clean when checked again"""
    maps = _custom_edition_maps()
    chosen = [path for path in maps if path.stem.lower() in ("bloodgulch", "hugeass", "ui")] or maps[:2]
    result = subprocess.run([str(MAP_VALIDATE), "--fuzz", "100", "--seed", "1", *map(str, chosen)],
                            capture_output=True, text=True, timeout=3600)
    assert result.returncode == 0, (result.stdout + result.stderr)[-6000:]
def test_vehicle_riding_saved_games():
    """The biped row is sized by BIPED_DATUM_SIZE, saved games of another layout are neither offered nor loaded, and
    riding is latched at a game's start and after every load (vehicle_riding.c)."""
    root = Path(__file__).resolve().parent.parent

    def body(text, signature):
        start = text.index(signature)
        return text[start:text.index("\n}\n", start)]

    object_types = (root / "source/objects/object_types.c").read_text(encoding="latin-1")
    assert "\"biped\", 'bipd', BIPED_DATUM_SIZE," in object_types
    game_state = (root / "source/saved games/game_state.c").read_text(encoding="latin-1")
    initialize = body(game_state, "void game_state_initialize(")
    assert initialize.index("crc_new(") < initialize.index("object_type_definitions[") < \
        initialize.index("game_state_allocate_buffer(")
    assert "allocation_size_checksum" in body(game_state, "boolean game_state_test_persistent_storage(")
    procs = game_state[game_state.index("static game_state_after_load_proc after_load_procs[] ="):]
    procs = procs[:procs.index("};")]
    assert procs.rstrip().endswith("bipeds_riding_latch")
    game = (root / "source/game/game.c").read_text(encoding="latin-1")
    assert "bipeds_riding_latch();" in body(game, "void game_initial_pulse(")
    bipeds = (root / "source/units/bipeds.c").read_text(encoding="latin-1")
    assert "vehicle_riding_latch();" in body(bipeds, "void bipeds_initialize_for_new_map(")


SERVER_SETUP = "main_menu.multiplayer_type_select.server_settings.xml"


def riding_menu_layout(files):
    """The host's switch belongs to vehicle and co-op options, not the main list."""
    import xml.etree.ElementTree as ElementTree

    tree = ElementTree.fromstring("\n".join(files[SERVER_SETUP]))
    base = "main_menu/multiplayer_type_select/server_settings"
    widgets = {w.get("name"): w for w in tree.findall("widget")}
    strings = {s.get("name"): s for s in tree.findall("strings")}
    children = widgets[base + "/options_menu"].findall("child")
    names = [c.get("widget") for c in children]
    assert base + "/op_vehicle_riding" not in names
    assert base + "/op_vehicle_riding" not in widgets
    assert base + "/var_vehicle_riding" not in strings
    assert "VEHICLE RIDING:" not in [s.get("text") for s in strings[base + "/labels"]]
    assert len(strings[base + "/help_strings"]) == 17
    help_widget = widgets[base + "/help"]
    assert help_widget.get("font") == "ui\\small_ui" and int(help_widget.get("top")) == 366
    assert int(help_widget.get("top")) + int(help_widget.get("height")) <= 414
    for child in children:
        name = child.get("widget")
        if not name.endswith("/button_bar"):
            assert int(child.get("y")) + int(widgets[name].get("height")) <= 366, name

    # (no empty row slot where a row left, and no row over the next one's)
    rows_y = sorted({int(c.get("y")) for c in children if not c.get("widget").endswith("/button_bar")})
    assert {b - a for a, b in zip(rows_y, rows_y[1:])} == {22}, rows_y
    coop = base + "/coop_options"
    coop_rows = widgets[coop + "/coop_options_menu"].findall("child")[:-1]
    for above, below in zip(coop_rows, coop_rows[1:]):
        assert int(below.get("y")) - int(above.get("y")) >= int(widgets[above.get("widget")].get("height")), above.get("widget")
    assert coop_rows[-1].get("widget") == coop + "/op_coop_vehicle_riding"
    vehicle = "main_menu/settings_select/multiplayer_setup/vehicle_options_edit"
    extra = ElementTree.fromstring("\n".join(files[vehicle.replace("/", ".") + ".port.xml"]))
    for screen, root, key, labels in ((coop, tree, "coop_vehicle_riding", "coop_options_labels"),
                                      (vehicle, extra, "vehicle_riding", "vehicle_options_labels")):
        made = {w.get("name"): w for w in root.findall("widget")}
        row = made[screen + "/op_" + key]
        assert [c.get("widget") for c in row.findall("child")] == [
            screen + "/" + key + "_label", screen + "/" + key + "_spinner"]
        label = made[screen + "/" + key + "_label"]
        assert label.get("string_index") == "9"
        assert label.get("string_list") == screen + "/" + labels
        spinner = made[screen + "/" + key + "_spinner"]
        assert spinner.get("string_list") == screen + "/var_" + key
        words = root.find(f"strings[@name='{screen}/var_{key}']")
        assert [s.get("text") for s in words] == ["ON", "OFF"]
    assert strings[coop + "/coop_options_labels"][-1].get("text") == "VEHICLE RIDING:"
    assert [s.get("text") for s in strings[coop + "/cap_coop_options"]][-2:] == [RIDING_HELP] * 2


RIDING_HELP = "Players standing on a vehicle are carried by it.\\nOff: as the original game, riders slide off."


def riding_vehicle_patch(ns):
    vehicle = ns["VEHICLE_EDIT"]
    assert vehicle + "/vehicle_options_menu" in ns["WIDGET_PATCHES"]
    assert ns["WIDGET_PATCHES"][vehicle + "/vehicle_options_menu"]["insert_before"] == {
        vehicle + "/op_team": [f'<child widget="{vehicle}/op_vehicle_riding" x="54" y="100"/>']}
    assert ns["STRING_INSERTS"][vehicle + "/vehicle_options_labels"] == [(9, ["VEHICLE RIDING:"])]
    assert ns["STRING_INSERTS"][vehicle + "/cap_vehicle_options"] == [
        (17, ["Every vehicle the map places will spawn, as in\\nHalo PC."]), (7, [RIDING_HELP] * 2)]


def riding_port_settings_module(source=None):
    """tools/port_settings.py's multiplayer_files, from its file or (a control) from a changed copy of its text."""
    import importlib.util

    path = Path(__file__).resolve().parent / "port_settings.py"
    if source is None:
        spec = importlib.util.spec_from_file_location("riding_port_settings", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return vars(module)
    namespace = {"__name__": "riding_port_settings_changed", "__file__": str(path)}
    exec(compile(source, str(path), "exec"), namespace)
    return namespace


def riding_port_settings(source=None):
    return riding_port_settings_module(source)["multiplayer_files"]


def test_vehicle_riding_patch_data():
    riding_vehicle_patch(riding_port_settings_module())


def test_vehicle_riding_server_setup():
    """Generated host options and committed extras agree, with every main row above help."""
    root = Path(__file__).resolve().parent.parent
    files = riding_port_settings()()
    raw = (root / "port/assets/menus/ce" / SERVER_SETUP).read_bytes()
    generated = "\n".join(files[SERVER_SETUP]).encode("utf-8")
    if b"\r\n" in raw:
        generated = generated.replace(b"\n", b"\r\n")
    assert raw == generated, "the Server Setup XML is not the generator's"
    riding_menu_layout(files)
    riding_vehicle_patch(riding_port_settings_module())
    vehicle = "main_menu/settings_select/multiplayer_setup/vehicle_options_edit"
    extra = vehicle.replace("/", ".") + ".port.xml"
    assert (root / "port/assets/menus/ce" / extra).read_text() == "\n".join(files[extra])
    for other, lines in files.items():
        if other != SERVER_SETUP:
            assert not any('font="ui\\small_ui"' in line and '/help"' in line for line in lines), other


SERVER_SETUP_CONTROLS = {
    "riding-on-main-list": ('rows.append((f"{base}/op_password", None))',
                            'rows.append((f"{base}/op_password", None))\n    rows.append((f"{base}/op_vehicle_riding", None, SETUP_OPTIONS_PLACE - 2))'),
    "riding-row-after-team": ('f"{VEHICLE_EDIT}/op_team": [', 'f"{VEHICLE_EDIT}/op_vehicle_presets": ['),
    "riding-label-index": ('"vehicle_riding", 9, ["ON", "OFF"]', '"vehicle_riding", 8, ["ON", "OFF"]'),
    "setup-gap-left": ("SETUP_OPTIONS_PLACE = 6", "SETUP_OPTIONS_PLACE = 7"),
    "coop-rows-overlap": ('("height", min(28, spacing))', '("height", 28)'),
    "coop-riding-missing": ('        ("coop_vehicle_riding", "VEHICLE RIDING:", ["ON", "OFF"], [VEHICLE_RIDING_HELP] * 2),\n', ''),
}


@pytest.mark.parametrize("control", sorted(SERVER_SETUP_CONTROLS))
def test_vehicle_riding_server_setup_control(control):
    """Each fault in the generator, run in memory (the committed XML untouched), fails the layout's assertions."""
    source = (Path(__file__).resolve().parent / "port_settings.py").read_text(encoding="latin-1")
    before, after = SERVER_SETUP_CONTROLS[control]
    assert source.count(before) == 1, f"negative control no longer applies: {before!r}"
    ns = riding_port_settings_module(source.replace(before, after, 1))
    files = ns["multiplayer_files"]()
    with pytest.raises(AssertionError):
        if control == "riding-row-after-team":
            riding_vehicle_patch(ns)
        else:
            riding_menu_layout(files)


# the riding network wiring (port/linux/game/vehicle_riding.c, port/linux/NETCODE.md): each check reads the sources
# and asserts; each control is a fault in one function (or, with no function, in the whole file) its check must refuse
RIDING_SOURCES = {
    "limits": "port/linux/include/halo_port_limits.h",
    "netcode": "port/linux/NETCODE.md",
    "network": "port/linux/game/network_distributed.c",
    "objects": "port/linux/game/network_objects.c",
    "coop": "port/linux/game/network_coop.c",
    "riding": "port/linux/game/vehicle_riding.c",
    "server": "source/networking/network_server_manager.c",
    "manager": "source/networking/network_game_manager.h",
    "menus": "port/linux/game/menu_functions.c",
    "director": "port/linux/game/server_director.c",
    "gametypes": "port/linux/game/server_gametypes.c",
    "game": "source/game/game.c",
    "bipeds": "source/units/bipeds.c",
    "format": "port/linux/network_format.txt",
    "hs": "source/hs/hs.c",
    "vbipeds": "port/linux/game/vehicle_riding_bipeds.c",
    "main": "source/main/main.c",
    "game_state": "source/saved games/game_state.c",
    "hs_globals": "source/hs/hs_globals_external.c",
    "hs_runtime": "source/hs/hs_runtime.c",
}
RIDING_CHECKS = {}
RIDING_CONTROLS = {}


def riding_sources():
    root = Path(__file__).resolve().parent.parent
    text = {key: (root / path).read_text(encoding="latin-1") for key, path in RIDING_SOURCES.items()}
    text["bipeds"] = biped_riding_sources()
    return text


def riding_function(text, name):
    from tools.harness import function
    return function(text, name)


def riding_version_check(text):
    assert re.search(r"#define HALO_PORT_NETWORK_VERSION 43\b", text["limits"])
    netcode = " ".join(text["netcode"].split())
    assert ("Version 43 also carries a rider standing on a vehicle: the deck part of a unit's state and the host's "
            "riding decision for the game, made at its start and kept until the map ends, a co-op flag") in netcode
    assert "It also carries a rider's cabin: its enclosure verdict while it stands as well as in the air." in netcode
    assert ("The host takes a rider's word in its vehicle's frame and checks it there; a client places riders on "
            "its own copy of the deck.") in netcode


RIDING_CHECKS["version"] = riding_version_check


RIDING_MCC_GUARD = ("\tif (mcc_script_effects_host_movement())\n\t\treturn;\n")


def riding_mcc_guard_check(text):
    body = riding_function(text["network"], "distributed_take_prediction")
    assert body.index("mcc_script_effects_host_movement()") < body.index("distributed_riding_transition(")
    assert body.index("mcc_script_effects_host_movement()") < body.index("distributed_take_riding(")


RIDING_CHECKS["mcc-guard-first"] = riding_mcc_guard_check
RIDING_CONTROLS["riding-before-mcc-guard"] = ("network", "distributed_take_prediction", RIDING_MCC_GUARD, "", "mcc-guard-first")
RIDING_CONTROLS["version-lowered"] = ("limits", None, "#define HALO_PORT_NETWORK_VERSION 43",
                                      "#define HALO_PORT_NETWORK_VERSION 42", "version")
RIDING_CONTROLS["versions-without-the-rider"] = ("netcode", None, "Version 43 also carries a rider",
                                                 "Version 43 also carries a car", "version")
RIDING_CONTROLS["versions-without-the-cabin"] = ("netcode", None, "It also carries a rider's cabin",
                                                 "It also carries a car", "version")
RIDING_CONTROLS["versions-without-authority"] = ("netcode", None, "The host takes a rider's word",
                                                   "The host takes a car's word", "version")


def riding_format_check(text):
    # (the record names every item of the wire format the sources have, the deck flags' cabin bit among them)
    from tools import network_format

    recorded = {" ".join(line.split(" ")[:2]): line.split(" ", 2)[2] for line in text["format"].splitlines()[1:]
                if line.strip()}
    assert recorded == network_format.items()
    assert recorded["enum _vehicle_riding_deck_cabin_bit"] == "3"


RIDING_CHECKS["format"] = riding_format_check
RIDING_CONTROLS["cabin-bit-unrecorded"] = ("format", None, "enum _vehicle_riding_deck_cabin_bit 3", "", "format")


def riding_keying_check(text):
    moving = riding_function(text["bipeds"], "biped_update_moving")
    assert "riding_player = biped_riding_player(biped);" in moving
    assert "riding_air = vehicle_riding_player_air(riding_player);" in moving
    assert "if (riding_player != NONE)\n\t{\n\t\triding_cabin_settled = biped_riding_contacts(" in moving
    # (the look stays the local player's: the view is only a local player's)
    assert "vehicle_riding_look(local_player_index, riding_carried" in moving
    collision = riding_function(text["bipeds"], "biped_riding_collision")
    assert "vehicle_riding_contact_record(biped_riding_player(biped), vehicle_index, &normal);" in collision


RIDING_CHECKS["keying"] = riding_keying_check
RIDING_CONTROLS["air-by-local-index"] = ("bipeds", "biped_update_moving",
                                         "riding_air = vehicle_riding_player_air(riding_player);",
                                         "riding_air = vehicle_riding_player_air(local_player_index);", "keying")
RIDING_CONTROLS["record-by-local-index"] = ("bipeds", "biped_riding_collision", "biped_riding_player(biped), vehicle_index",
                                            "unit_get_local_player_index(biped_index), vehicle_index", "keying")


def riding_codec_check(text):
    network = text["network"]
    # (both loops of unit states stop at a state they cannot read: the rest of that message is dropped)
    for name in ("distributed_handle_predictions", "distributed_handle_unit_states"):
        assert "if (!size)\n\t\t\tbreak;" in riding_function(network, name)
    for name in ("distributed_client_send_predictions", "distributed_host_send_players"):
        assert "byte entry[DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE];" in riding_function(network, name)
    gather = riding_function(network, "distributed_state_from_player")
    assert "biped_riding_deck(unit_index, &state->deck)" in gather
    deck = riding_function(text["bipeds"], "biped_riding_deck")
    assert "biped_get_physics_pill(biped_index, &base, &height, &width);" in deck
    assert "vehicle_riding_player_air(biped_riding_player(biped))" in deck
    assert "vehicle_riding_player_cabin(biped_riding_player(biped))" in deck
    nodes = riding_function(text["bipeds"], "biped_riding_vehicle_node_count")
    assert nodes.index("vehicle_riding_deck_vehicle_named(vehicle_index)") < nodes.index("object_try_and_get_and_verify_type")


RIDING_CHECKS["codec"] = riding_codec_check
RIDING_CONTROLS["unit-states-continue"] = ("network", "distributed_handle_unit_states", "if (!size)\n\t\t\tbreak;",
                                           "if (!size)\n\t\t\tcontinue;", "codec")
RIDING_CONTROLS["predictions-continue"] = ("network", "distributed_handle_predictions", "if (!size)\n\t\t\tbreak;",
                                           "if (!size)\n\t\t\tcontinue;", "codec")
RIDING_CONTROLS["deck-not-gathered"] = ("network", "distributed_state_from_player",
                                        "biped_riding_deck(unit_index, &state->deck)", "FALSE", "codec")
RIDING_CONTROLS["deck-air-by-local-index"] = ("bipeds", "biped_riding_deck", "vehicle_riding_player_air(biped_riding_player(biped))",
                                              "vehicle_riding_player_air(unit_get_local_player_index(biped_index))", "codec")
RIDING_CONTROLS["deck-without-cabin"] = ("bipeds", "biped_riding_deck", "vehicle_riding_player_cabin(biped_riding_player(biped))",
                                         "NULL", "codec")
RIDING_CONTROLS["nodes-looked-up-first"] = ("bipeds", "biped_riding_vehicle_node_count",
                                            "\tif (!vehicle_riding_deck_vehicle_named(vehicle_index))\n\t\treturn NONE;\n",
                                            "", "codec")


def riding_deck_frame_check(text):
    deck = riding_function(text["bipeds"], "biped_riding_deck")
    assert "vehicle_riding_deck_frame(&riding, &vehicle, &frame);" in deck
    assert "return vehicle_riding_deck_make(&riding, &frame, &base," in deck


RIDING_CHECKS["deck-carried-frame"] = riding_deck_frame_check
RIDING_CONTROLS["deck-frame-unwired"] = ("bipeds", "biped_riding_deck", "vehicle_riding_deck_make(&riding, &frame, &base,",
                                         "vehicle_riding_deck_make(&riding, &vehicle, &base,", "deck-carried-frame")


def riding_bit_writers(text):
    """Every statement under port/ and source/ that sets or clears the riding bit, as (path, statement)."""
    root = Path(__file__).resolve().parent.parent
    held = {path: text[key] for key, path in RIDING_SOURCES.items() if key != "bipeds" and path.endswith(".c")}
    writer = re.compile(r"SET_FLAG\([^;]*_network_game_cooperative_vehicle_riding_bit[^;]*;|"
                        r"(?:\|=|&=)[^;]*_network_game_cooperative_vehicle_riding_bit[^;]*;")
    found = []
    for folder in ("port", "source"):
        for path in sorted((root / folder).rglob("*.c")):
            name = path.relative_to(root).as_posix()
            source = held[name] if name in held else path.read_text(encoding="latin-1")
            found += [(name, match.group(0)) for match in writer.finditer(source)]
    return found


def riding_switch_check(text):
    assert re.search(r"_network_game_cooperative_vehicle_riding_bit = 3,", text["manager"])
    for key in ("server", "menus", "director"):
        assert "network_game_server_port_set_vehicle_riding" not in text[key]
    assert "network_game_server_port_set_vehicle_riding" not in (
        Path(__file__).resolve().parent.parent / "source/networking/network_server_manager.h").read_text(encoding="latin-1")
    start = riding_function(text["server"], "network_game_server_start_network_game")
    call = "network_game_port_vehicle_riding_start(&server->game);"
    assert start.index("if (server->sent_start_game_message == FALSE)") < start.index(call) < start.index(
        "network_game_server_send_game_settings_to_all_machines(server, &server->game, sizeof(server->game))")
    writers = riding_bit_writers(text)
    assert [path for path, _ in writers] == ["port/linux/game/network_coop.c"], writers
    assert writers[0][1] in riding_function(text["coop"], "network_game_port_vehicle_riding_start")


RIDING_CHECKS["switch"] = riding_switch_check
RIDING_CONTROLS["start-not-called"] = ("server", "network_game_server_start_network_game",
                                       "\t\tnetwork_game_port_vehicle_riding_start(&server->game);\n", "", "switch")
RIDING_CONTROLS["start-after-send"] = ("server", "network_game_server_start_network_game",
                                       "\t\tnetwork_game_port_vehicle_riding_start(&server->game);\n", "", "switch")
RIDING_CONTROLS["second-writer"] = ("director", "server_apply_game_settings",
                                    "\tnetwork_game_server_port_set_cooperative_dedicated(TRUE);\n",
                                    "\tnetwork_game_server_port_set_cooperative_dedicated(TRUE);\n"
                                    "\tSET_FLAG(server->game.cooperative_flags, _network_game_cooperative_vehicle_riding_bit, TRUE);\n",
                                    "switch")


RIDING_FLAG_ROW = '{ "vehicle_riding_spinner", _option_flag, 0, FLAG(_game_variant_vehicle_riding_bit), 2, { 1, 0 } },'
RIDING_COOP_ROW = '{ "coop_vehicle_riding_spinner", _option_setting, 0, 0, 2, { 0 }, "network.vehicle_riding", { "true", "false" } },'


def riding_menu_check(text):
    menus = text["menus"]
    assert RIDING_FLAG_ROW in menus
    assert RIDING_COOP_ROW in menus
    assert '"vehicle_riding_spinner", _option_setting' not in menus
    assert '"op_vehicle_riding")' not in menus
    assert "VEHICLE_RIDING_HELP" not in menus
    init = riding_function(menus, "server_settings_initialize")
    update = riding_function(menus, "server_settings_update")
    for body in (init, update):
        assert "op_vehicle_riding" not in body
        assert "setting_options_each(" not in body
    assert "visible_set(row, gametype_edit.setup);" in riding_function(menus, "gametype_options_each")
    assert "gametype_options_each(list, TRUE);" in riding_function(menus, "gametype_options_save")
    save = menus.split('!strcmp(name, "port setup options save")', 1)[1].split('}', 1)[0]
    assert "setting_options_each(list, TRUE);" in save
    rows = menus.split("server_settings_gametype_rows[] =", 1)[1].split("};", 1)[0]
    assert '"op_vehicle_options"' in rows


RIDING_CHECKS["menu"] = riding_menu_check
RIDING_CONTROLS["riding-flag-inverted"] = ("menus", None, "FLAG(_game_variant_vehicle_riding_bit), 2, { 1, 0 } },",
                                           "FLAG(_game_variant_vehicle_riding_bit), 2, { 0, 1 } },", "menu")
RIDING_CONTROLS["coop-riding-setting-inverted"] = ("menus", None, '"network.vehicle_riding", { "true", "false" }',
                                                   '"network.vehicle_riding", { "false", "true" }', "menu")
RIDING_CONTROLS["riding-row-setting-again"] = ("menus", None, RIDING_FLAG_ROW,
    '{ "vehicle_riding_spinner", _option_setting, 0, 0, 2, { 0 }, "network.vehicle_riding", { "true", "false" } },', "menu")
RIDING_CONTROLS["riding-row-not-saved"] = ("menus", "gametype_options_save", "gametype_options_each(list, TRUE);",
                                           "gametype_options_each(list, FALSE);", "menu")
RIDING_CONTROLS["riding-row-in-edit-gametypes"] = ("menus", "gametype_options_each", "visible_set(row, gametype_edit.setup);",
                                                "visible_set(row, TRUE);", "menu")
RIDING_CONTROLS["coop-riding-not-saved"] = ("menus", None,
    'if (list)\n\t\t\t\tsetting_options_each(list, TRUE);',
    'if (list)\n\t\t\t\tsetting_options_each(list, FALSE);', "menu")
RIDING_CONTROLS["vehicle-screen-shown-in-coop"] = ("menus", None, '"op_vehicle_options", ', '', "menu")


def riding_observation_check(text):
    network = text["network"]
    correct = riding_function(network, "distributed_correct_own_unit")
    # (upstream's elevator height tolerance never alters a rider's drop or an air claim's take)
    assert "if (!dropping && !air_claim && own->elevator_index != NONE &&" in correct
    assert "distributed_handle_unit_state(host_time, &state);" in riding_function(network, "distributed_handle_unit_states")
    assert "distributed_handle_unit_states(header.game_time," in network
    # (every netcode write of a vehicle's pose tells the one function: section 2's seam)
    objects = text["objects"]
    assert objects.count("object_set_position(") == 4
    assert objects.count("network_objects_vehicle_written(object_index, &before, &before_forward, &before_up, kind);") + \
        objects.count("network_objects_vehicle_written(object_index, &before, &before_forward, &before_up, _network_vehicle_write_state);") + \
        objects.count("network_objects_vehicle_written(iterator.index, &before, &before_forward, &before_up, _network_vehicle_write_carry);") == 4
    transforms = riding_function(text["coop"], "network_coop_handle_object_transforms")
    assert "network_objects_vehicle_written(object_index, &before, &before_forward, &before_up, _network_vehicle_write_coop);" in transforms


RIDING_CHECKS["observation"] = riding_observation_check
RIDING_CONTROLS["unsteered-write-untold"] = ("objects", "distributed_client_carry_unsteered_vehicles",
                                              "\t\t\tnetwork_objects_vehicle_written(iterator.index, &before, &before_forward, &before_up, _network_vehicle_write_carry);\n", "",
                                             "observation")
RIDING_CONTROLS["blend-write-untold"] = ("objects", "network_objects_reconcile",
                                          "\t\tnetwork_objects_vehicle_written(object_index, &before, &before_forward, &before_up, kind);\n", "", "observation")


def riding_order(body, *parts):
    """Each part is in the body, in this order."""
    for part in parts:
        assert part in body, f"missing riding anchor: {part}"
    positions = [body.index(part) for part in parts]
    assert positions == sorted(positions), parts


def riding_cabin_tick_check(text):
    bipeds = text["bipeds"]
    moving = riding_function(bipeds, "biped_update_moving")
    # (the cabin sampled after the carry and before the window's ray, whose verdict needs it)
    riding_order(moving, "biped_riding_carry(biped_index, biped, definition, riding.object_index, &riding_step.displacement,",
                 "vehicle_riding_cabin_begin(riding_cabin, biped_index, &riding, riding_applies && riding_step_valid);",
                 "riding_exception = biped_riding_cabin_tick(",
                 "riding_step.over_deck = biped_riding_window_over_deck(definition, &riding, &riding_step.vehicle,\n"
                 "\t\t\t&physics.position, physics.height, riding_exception);")
    # (a first claim seeded after the recording; the take-off spent before this tick's jump marks the next)
    riding_order(moving, "biped_riding_set(biped, &riding);", "riding_cabin->take_off = FALSE;",
                 "vehicle_riding_cabin_acquire(riding_cabin, biped_index, &riding)",
                 "biped_update_jumping(biped_index, animation);")
    assert "vehicle_riding_cabin_up(step)" in riding_function(bipeds, "biped_riding_cabin_tick")
    assert "&vehicle->body_up" in riding_function(bipeds, "biped_riding_cabin_sample")
    collision = riding_function(bipeds, "biped_riding_collision")
    assert "over_deck = biped_riding_recast_over_deck(biped_index, biped, definition, &riding, &vehicle, base, height);" in collision
    for name in ("biped_riding_collision", "biped_riding_cabin_protected", "biped_riding_recast_over_deck"):
        body = riding_function(bipeds, name)
        assert "biped_riding_cabin_sample" not in body and "biped_riding_cabin_rays" not in body, name
    jump = riding_function(bipeds, "biped_jump")
    assert "biped_riding_take_off(biped_index, biped, &riding);" in jump
    take_off = riding_function(bipeds, "biped_riding_take_off")
    assert take_off.split("{", 1)[1].rsplit("}", 1)[0].strip() == (
        "vehicle_riding_cabin_take_off(vehicle_riding_player_cabin(biped_riding_player(biped)), riding, biped_index);")
    # (the jump's frame arithmetic is the vector helpers', not six component lines)
    assert "subtract_vectors3d(&jump_velocity, &jump_frame, &jump_velocity);" in jump
    assert "add_vectors3d(&jump_velocity, &jump_frame, &jump_velocity);" in jump
    assert not re.search(r"jump_velocity\.[ijk] [-+]= jump_frame\.[ijk];", jump)
    assert "vehicle_riding_cabin_clear(cabin);" in riding_function(bipeds, "biped_riding_seated")


RIDING_CHECKS["cabin-tick"] = riding_cabin_tick_check
RIDING_CONTROLS["recast-site-unwired"] = (
    "bipeds", "biped_riding_collision",
    "over_deck = biped_riding_recast_over_deck(biped_index, biped, definition, &riding, &vehicle, base, height);",
    "over_deck = biped_riding_over_deck(definition, &vehicle, base, height);", "cabin-tick")
RIDING_CONTROLS["tick-site-unwired"] = (
    "bipeds", "biped_update_moving",
    "riding_step.over_deck = biped_riding_window_over_deck(definition, &riding, &riding_step.vehicle,\n"
    "\t\t\t&physics.position, physics.height, riding_exception);",
    "riding_step.over_deck = biped_riding_over_deck(definition, &riding_step.vehicle, &physics.position,\n"
    "\t\t\tphysics.height);", "cabin-tick")
RIDING_CONTROLS["cabin-before-carry"] = (
    "bipeds", "biped_update_moving",
    "\t\tbiped_riding_carry(biped_index, biped, definition, riding.object_index, &riding_step.displacement,\n"
    "\t\t\tphysics.height, physics.width, &physics.position);\n", "", "cabin-tick")
RIDING_CONTROLS["node-up-in-tick"] = ("bipeds", "biped_riding_cabin_tick", "vehicle_riding_cabin_up(step)",
                                      "&step->vehicle.up", "cabin-tick")
RIDING_CONTROLS["take-off-not-marked"] = (
    "bipeds", "biped_jump",
    "biped_riding_take_off(biped_index, biped, &riding);", "", "cabin-tick")
RIDING_CONTROLS["take-off-null-cabin"] = (
    "bipeds", "biped_riding_take_off", "vehicle_riding_player_cabin(biped_riding_player(biped))", "NULL", "cabin-tick")
RIDING_CONTROLS["jump-subtract-by-component"] = (
    "bipeds", "biped_jump", "subtract_vectors3d(&jump_velocity, &jump_frame, &jump_velocity);",
    "jump_velocity.i -= jump_frame.i;", "cabin-tick")
RIDING_CONTROLS["jump-add-by-component"] = (
    "bipeds", "biped_jump", "add_vectors3d(&jump_velocity, &jump_frame, &jump_velocity);",
    "jump_velocity.i += jump_frame.i;", "cabin-tick")
RIDING_CONTROLS["seated-keeps-cabin"] = ("bipeds", "biped_riding_seated", "vehicle_riding_cabin_clear(cabin);", "",
                                         "cabin-tick")
RIDING_CONTROLS["acquisition-not-seeded"] = ("bipeds", "biped_update_moving",
                                             "vehicle_riding_cabin_acquire(riding_cabin, biped_index, &riding)", "FALSE",
                                             "cabin-tick")
RIDING_CONTROLS["take-off-never-spent"] = ("bipeds", "biped_update_moving", "riding_cabin->take_off = FALSE;", "",
                                           "cabin-tick")


def riding_cabin_carry_check(text):
    bipeds = text["bipeds"]
    physics = riding_function(bipeds, "biped_update_physics")
    # (after the native support is chosen, before the bumps: the carrier never decides the ground)
    riding_order(physics, "biped_riding_claimed_support(", "if (physics->cabin_object_index != NONE)\n"
                 "\t\tbiped_riding_cabin_carrier(physics, collisions, collision_count);",
                 "long bumped_object_index = NONE;")
    carrier = riding_function(bipeds, "biped_riding_cabin_carrier")
    for native in ("out_flags", "ground_plane", "landing_velocity", "support_surface_index", "support_object_index"):
        assert native not in carrier, native
    moving = riding_function(bipeds, "biped_update_moving")
    assert ("physics.cabin_object_index = vehicle_riding_cabin_carrier_vehicle(riding_cabin, &riding, riding_exception);"
            in moving)
    assert ("carried = physics.carrier_object_index != NONE ?\n"
            "\t\t\tbiped_riding_vehicle(physics.carrier_object_index, physics.carrier_node_index, &support) :") in moving
    # (a rider the deck carried in, named by its exception, is held with no touch, before the carrier decides carried)
    riding_order(moving, "vehicle_riding_cabin_holds(physics.cabin_object_index, riding_carried)",
                 "physics.carrier_object_index = physics.cabin_object_index;",
                 "carried = physics.carrier_object_index != NONE ?")
    assert "\t\t\tcarried ? &support : NULL,\n\t\t\tsupported,\n" in moving


RIDING_CHECKS["cabin-carry"] = riding_cabin_carry_check
RIDING_CONTROLS["native-ground-forced"] = (
    "bipeds", "biped_riding_cabin_carrier", "\tphysics->carrier_object_index = collisions[carrier].object_index;\n",
    "\tphysics->carrier_object_index = collisions[carrier].object_index;\n"
    "\tSET_FLAG(physics->out_flags, _biped_physics_out_airborne_bit, FALSE);\n", "cabin-carry")
RIDING_CONTROLS["carrier-chooses-ground"] = (
    "bipeds", "biped_riding_cabin_carrier", "\tphysics->carrier_object_index = collisions[carrier].object_index;\n",
    "\tphysics->carrier_object_index = collisions[carrier].object_index;\n"
    "\tphysics->ground_plane = collisions[carrier].plane;\n", "cabin-carry")
RIDING_CONTROLS["carrier-before-support"] = (
    "bipeds", "biped_update_physics",
    "\tif (physics->cabin_object_index != NONE)\n\t\tbiped_riding_cabin_carrier(physics, collisions, collision_count);\n",
    "", "cabin-carry")
RIDING_CONTROLS["hold-never-carries"] = (
    "bipeds", "biped_update_moving", "\t\t\tphysics.carrier_object_index = physics.cabin_object_index;\n", "", "cabin-carry")
RIDING_CONTROLS["record-ignores-carrier"] = ("bipeds", "biped_update_moving", "\t\t\tcarried ? &support : NULL,\n",
                                             "\t\t\tsupported ? &support : NULL,\n", "cabin-carry")


def riding_cabin_recovery_check(text):
    bipeds = text["bipeds"]
    settle = riding_function(bipeds, "biped_riding_cabin_settle")
    # (no closing move here: the touch's move is made once, in biped_riding_contacts; no push, no damage, ever; the
    # rider's velocity is not the settle's to change, so a failed recovery leaves it as it came)
    for absent in ("vehicle_riding_contact_move", "physics_biped_vehicle_collision_push",
                   "physics_biped_vehicle_collision_damage", "velocity"):
        assert absent not in settle.replace("jump_velocity", ""), absent
    contacts = riding_function(bipeds, "biped_riding_contacts")
    assert contacts.count("vehicle_riding_contact_move(") == 1
    riding_order(contacts, "if (vehicle.object_index == cabin_vehicle || (cabin && contacts[index].deferred))",
                 "biped_riding_cabin_settle(", "settled = TRUE;")
    branch = contacts[contacts.index("if (vehicle.object_index == cabin_vehicle"):]
    branch = branch[:branch.index("continue;")]
    for absent in ("velocity", "physics_biped_vehicle_collision", "touching", "candidate"):
        assert absent not in branch, absent
    assert "&ahead, FALSE, height, width, base);" in branch
    moving = riding_function(bipeds, "biped_update_moving")
    # (after the carry, where the vehicle's update recorded no touch: the check all the same, no closing move; after
    # its own movement and before the recording: the place kept on the deck is the settled one)
    riding_order(moving, "riding_cabin_settled = biped_riding_contacts(",
                 "if (riding_step_valid && riding_carried && !riding_cabin_settled && vehicle_riding_cabin_watched(riding_cabin, &riding, biped_index))",
                 "global_zero_vector3d, FALSE, physics.height, physics.width, &physics.position);",
                 "biped_update_physics(&physics);",
                 "if (riding_step_valid && vehicle_riding_cabin_watched(riding_cabin, &riding, biped_index))",
                 "global_zero_vector3d, TRUE, physics.height, physics.width, &physics.new_position);",
                 "if (vehicle_riding_record(")
    # (the final call's ahead is zero: its anchor is stored at the base, the touch test reads base + ahead; and neither
    # settle block touches the rider's velocity)
    assert moving.count("global_zero_vector3d, TRUE, physics.height, physics.width, &physics.new_position);") == 1
    assert moving.count("global_zero_vector3d, FALSE, physics.height, physics.width, &physics.position);") == 1
    assert "NULL,\n\t\t\tglobal_zero_vector3d, TRUE," in moving
    for start in ("if (riding_step_valid && riding_carried && !riding_cabin_settled", "if (riding_step_valid && vehicle_riding_cabin_watched("):
        block = moving[moving.index(start):]
        block = block[:block.index("\n\t}\n")]
        assert "velocity" not in block, start
    collision = riding_function(bipeds, "biped_riding_collision")
    assert "deferred = vehicle_riding_cabin_collision(decision, vehicle_riding_cabin_defers(" in collision
    riding_order(collision, "deferred = vehicle_riding_cabin_collision(",
                 "if (deferred == _vehicle_riding_collision_resolved)\n\t\tdecision = _vehicle_riding_collision_resolved;",
                 "vehicle_riding_contact_record(biped_riding_player(biped), vehicle_index, &normal);",
                 "if (deferred == _vehicle_riding_collision_resolved)\n\t\t\tvehicle_riding_contact_defer(")


RIDING_CHECKS["cabin-recovery"] = riding_cabin_recovery_check
RIDING_CONTROLS["move-twice"] = ("bipeds", "biped_riding_cabin_settle", "\tsettle = vehicle_riding_cabin_recover(&check);\n",
                                 "\tvehicle_riding_contact_move(NULL, FALSE, vehicle, base, NULL, normal, FALSE, NULL, NULL);\n"
                                 "\tsettle = vehicle_riding_cabin_recover(&check);\n", "cabin-recovery")
RIDING_CONTROLS["stale-place-after-recovery"] = (
    "bipeds", "biped_update_moving",
    "\t\tbiped_riding_cabin_settle(biped_index, biped, definition, riding_cabin, &riding_step.vehicle, &entry, NULL,\n"
    "\t\t\tglobal_zero_vector3d, TRUE, physics.height, physics.width, &physics.new_position);\n", "", "cabin-recovery")
RIDING_CONTROLS["after-carry-check-missing"] = (
    "bipeds", "biped_update_moving",
    "\t\tbiped_riding_cabin_settle(biped_index, biped, definition, riding_cabin, &riding_step.vehicle, &entry, NULL,\n"
    "\t\t\tglobal_zero_vector3d, FALSE, physics.height, physics.width, &physics.position);\n", "", "cabin-recovery")
RIDING_CONTROLS["after-carry-twice"] = ("bipeds", "biped_update_moving",
                                        "if (riding_step_valid && riding_carried && !riding_cabin_settled && vehicle_riding_cabin_watched(",
                                        "if (riding_step_valid && vehicle_riding_cabin_watched(", "cabin-recovery")
RIDING_CONTROLS["deferral-not-applied"] = (
    "bipeds", "biped_riding_collision",
    "\tif (deferred == _vehicle_riding_collision_resolved)\n\t\tdecision = _vehicle_riding_collision_resolved;\n", "",
    "cabin-recovery")
RIDING_CONTROLS["cabin-contacts-crush"] = ("bipeds", "biped_riding_contacts", "\t\tif (vehicle.object_index == cabin_vehicle",
                                           "\t\tif (FALSE && vehicle.object_index == cabin_vehicle", "cabin-recovery")
RIDING_CONTROLS["deferral-not-marked"] = ("bipeds", "biped_riding_collision",
                                          "\t\t\tvehicle_riding_contact_defer(player, vehicle_index);\n",
                                          "", "cabin-recovery")
RIDING_CONTROLS["final-settle-ahead"] = (
    "bipeds", "biped_update_moving",
    "NULL,\n\t\t\tglobal_zero_vector3d, TRUE, physics.height", "NULL,\n\t\t\t&physics.velocity, TRUE, physics.height",
    "cabin-recovery")
RIDING_CONTROLS["failed-recovery-zeroes-velocity"] = (
    "bipeds", "biped_riding_contacts", "\t\t\tsettled = TRUE;\n",
    "\t\t\tsettled = TRUE;\n\t\t\tscale_vector3d(velocity, 0.f, (real_vector3d *)velocity);\n", "cabin-recovery")


def riding_run_jump_check(text):
    body = riding_function(text["network"], "distributed_on_foot_bound")
    assert "vehicle_riding_run_jump_speed(" in body
    assert "run_speed = vehicle_riding_run_jump_speed(" in body
    assert "run_speed * run_speed + jump_speed * jump_speed" not in body


def riding_damage_speed_shared_check(text):
    body = riding_function(text["riding"], "vehicle_riding_damage_speed")
    assert "vehicle_riding_run_jump_speed(" in body
    assert "sqrt(" not in body


RIDING_CHECKS["run-jump"] = riding_run_jump_check
RIDING_CHECKS["damage-speed-shared"] = riding_damage_speed_shared_check
RIDING_CONTROLS["bound-own-run-speed"] = (
    "network", "distributed_on_foot_bound",
    "base_speed = PREDICTED_ON_FOOT_SPEED_SCALE * vehicle_riding_run_jump_speed(information != NULL,\n"
    "\t\tinformation ? information->run_forward_speed : 0.f, information ? information->run_backward_speed : 0.f,\n"
    "\t\tinformation ? information->run_sideways_speed : 0.f, game_players_are_double_speed(),\n"
    "\t\tinformation ? information->double_speed_multiplier : 1.f, player ? player->speed_multiplier : 1.f, jump_speed);",
    "base_speed = PREDICTED_ON_FOOT_SPEED_SCALE * (real)sqrt(run_speed * run_speed + jump_speed * jump_speed);",
    "run-jump")
RIDING_CONTROLS["damage-speed-own-sqrt"] = (
    "riding", "vehicle_riding_damage_speed",
    "return 2.f * vehicle_riding_run_jump_speed(have_information, run_forward, run_backward, run_sideways,\n"
    "\t\tdouble_speed, double_speed_multiplier, player_speed_multiplier, jump_velocity);",
    "speed = 2.f*(real)sqrt(run*run + jump_velocity*jump_velocity);\n\treturn speed;", "damage-speed-shared")

def riding_vehicle_writes_check(text):
    for owner in ("objects", "coop"):
        code = text[owner]
        for match in re.finditer(r"\bobject_set_position\(", code):
            # (the hook stays in the write's own block, before any next write)
            tail = code[match.end():]
            end = tail.index("}")
            block = tail[:end]
            hook = re.search(r"network_objects_vehicle_written\([^;]+;", block)
            assert hook, f"unhooked pose write in {owner}"
            assert "&before_forward" in hook.group() and "&before_up" in hook.group()
    hook = riding_function(text["objects"], "network_objects_vehicle_written")
    assert "if (!network_objects_vehicle_followed(object_index))" in hook
    assert "vehicle_riding_vehicle_corrected(object_index, before_position, before_forward, before_up," in hook
    vehicle = riding_function(text["bipeds"], "biped_riding_vehicle")
    assert "vehicle->correction = *vehicle_riding_vehicle_correction(vehicle_index);" in vehicle
    for name in ("distributed_object_move", "network_objects_reconcile"):
        body = riding_function(text["objects"], name)
        assert "before_forward = object->object.forward" in body
        assert "before_up = object->object.up" in body
    get = riding_function(text["bipeds"], "biped_riding_get")
    put = riding_function(text["bipeds"], "biped_riding_set")
    assert "vehicle_riding_player_mark(biped_riding_player(biped))" in get
    assert "vehicle_riding_player_mark(biped_riding_player(biped))" in put


RIDING_CHECKS["vehicle-writes"] = riding_vehicle_writes_check
RIDING_CONTROLS["coop-write-unhooked"] = (
    "coop", None,
    "network_objects_vehicle_written(object_index, &before, &before_forward, &before_up, _network_vehicle_write_coop);",
    "", "vehicle-writes")


def riding_followed_check(text):
    followed = riding_function(text["objects"], "network_objects_vehicle_followed")
    # (the host's own copy of a co-op vehicle is simulated: only a client's follows)
    assert "game_connection() == _game_connection_network_client && distributed_vehicle_unsteered(vehicle_index)" in followed
    vehicle = riding_function(text["bipeds"], "biped_riding_vehicle")
    assert "vehicle->role = network_objects_vehicle_followed(vehicle_index) ?" in vehicle
    assert "_vehicle_riding_role_followed : _vehicle_riding_role_simulated;" in vehicle
    hook = riding_function(text["objects"], "network_objects_vehicle_written")
    assert "if (!network_objects_vehicle_followed(object_index))" in hook
    assert "distributed_object_follows_host" not in hook


def riding_unsteered_order_check(text):
    body = riding_function(text["objects"], "network_objects_handle_states")
    reconcile = body.index("if (network_objects_reconcile(state->object_index")
    # (an unsteered vehicle's path word is its whole take: it never reaches the reconcile, whose write of the host's
    # physics velocity would replace the path's)
    taken = re.search(r"distributed_client_note_unsteered_path\(state->object_index[^;]*;\s*continue;", body)
    assert taken and taken.end() < reconcile


RIDING_CHECKS["unsteered-order"] = riding_unsteered_order_check
RIDING_CONTROLS["unsteered-note-before-reconcile"] = (
    "objects", "network_objects_handle_states",
    "host_time, TEST_FLAG(state->flags, _distributed_object_at_rest_bit));\n\t\t\tcontinue;\n\t\t}",
    "host_time, TEST_FLAG(state->flags, _distributed_object_at_rest_bit));\n\t\t}", "unsteered-order")


RIDING_CHECKS["followed"] = riding_followed_check
RIDING_CONTROLS["followed-on-host"] = ("objects", "network_objects_vehicle_written",
                                       "if (!network_objects_vehicle_followed(object_index))",
                                       "if (!distributed_object_follows_host(object_index))", "followed")
RIDING_CONTROLS["followed-on-any-connection"] = ("objects", "network_objects_vehicle_followed",
                                                 "game_connection() == _game_connection_network_client && ", "", "followed")
RIDING_CONTROLS["followed-any-vehicle"] = ("objects", "network_objects_vehicle_followed",
                                           " && distributed_vehicle_unsteered(vehicle_index)", "", "followed")
RIDING_CONTROLS["role-always-simulated"] = ("bipeds", "biped_riding_vehicle",
                                            "network_objects_vehicle_followed(vehicle_index) ?",
                                            "FALSE ?", "followed")


def riding_host_order_check(text):
    body = riding_function(text["network"], "network_distributed_tick")
    before = "profile_scope(distributed_tick_apply_predictions_section, distributed_apply_predictions(FALSE);)"
    vehicles = "profile_scope(distributed_tick_apply_vehicle_predictions_section, network_objects_apply_vehicle_predictions();)"
    after = "profile_scope(distributed_tick_apply_predictions_section, distributed_apply_predictions(TRUE);)"
    assert before in body and vehicles in body and after in body
    assert body.index(before) < body.index(vehicles) < body.index(after)
    assert body.count("distributed_prediction_rides(") == 1
    assert body.index("distributed_rider_pass[player_index] = distributed_prediction_rides(") < body.index(before)


RIDING_CHECKS["host-order"] = riding_host_order_check
RIDING_CONTROLS["riders-before-vehicles"] = (
    "network", "network_distributed_tick",
    "profile_scope(distributed_tick_apply_vehicle_predictions_section, network_objects_apply_vehicle_predictions();)\n"
    "\t\tprofile_scope(distributed_tick_apply_predictions_section, distributed_apply_predictions(TRUE);)",
    "profile_scope(distributed_tick_apply_predictions_section, distributed_apply_predictions(TRUE);)\n"
    "\t\tprofile_scope(distributed_tick_apply_vehicle_predictions_section, network_objects_apply_vehicle_predictions();)",
    "host-order")



def riding_crush_owner_check(text):
    # (the crush is the original's own vehicle collision, owned by the vehicle's driver: no direct damage call of ours)
    contacts = riding_function(text["bipeds"], "biped_riding_contacts")
    assert "physics_biped_vehicle_collision_damage(" in contacts
    assert "damage_object_" not in contacts


RIDING_CHECKS["crush-owner"] = riding_crush_owner_check
RIDING_CONTROLS["crush-direct-damage"] = ("bipeds", "biped_riding_contacts",
                                          "physics_biped_vehicle_collision_damage(vehicle.object_index, biped_index, &push);",
                                          "damage_object_direct(biped_index, &push);", "crush-owner")


def riding_write_carry_check(text):
    objects = text["objects"]
    gate = riding_function(objects, "network_objects_vehicle_carries")
    # (a client's host state correction of a copy it does not follow, of a vehicle, with the riding code on)
    for term in ("kind == _network_vehicle_write_state", "game_connection() == _game_connection_network_client",
                 "!network_objects_vehicle_followed(object_index)", "vehicle_riding_enabled()",
                 "object_get(object_index)->object.type == _object_type_vehicle"):
        assert term in gate, term
    writing = riding_function(objects, "network_objects_vehicle_writing")
    assert "if (network_objects_vehicle_carries(object_index, kind))\n\t\tbiped_riding_vehicle_writing(object_index);" in writing
    # (measured against the unwritten hull: before each of the two writes a state correction makes)
    for name, write in (("distributed_object_move", "object_set_position(object_index, position, forward, up);"),
                        ("network_objects_reconcile",
                         "object_set_position(object_index, &blended, &valid_forward, &valid_up);")):
        body = riding_function(objects, name)
        call = "network_objects_vehicle_writing(object_index, kind);"
        assert body.count(call) == 1 and body.count(write) == 1, name
        assert body.index(call) < body.index(write), name
    written = riding_function(objects, "network_objects_vehicle_written")
    carry = ("if (network_objects_vehicle_carries(object_index, kind))\n"
             "\t\t\tbiped_riding_vehicle_written(object_index, before_position, before_forward, before_up);")
    assert written.count(carry) == 1
    # (after the correction is recorded, which a window rider's rebase reads, and inside the vehicle's block)
    assert written.index("vehicle_riding_vehicle_corrected(") < written.index(carry)
    assert written.index("if (object->object.type == _object_type_vehicle)") < written.index(carry)


RIDING_CHECKS["write-carry"] = riding_write_carry_check
RIDING_CONTROLS["write-carry-any-kind"] = ("objects", "network_objects_vehicle_carries",
                                           "kind == _network_vehicle_write_state && ", "", "write-carry")
RIDING_CONTROLS["write-carry-on-host"] = ("objects", "network_objects_vehicle_carries",
                                          "game_connection() == _game_connection_network_client &&\n\t\t", "", "write-carry")
RIDING_CONTROLS["write-carry-ungated"] = ("objects", "network_objects_vehicle_carries",
                                          " && vehicle_riding_enabled()", "", "write-carry")
RIDING_CONTROLS["write-carry-followed"] = ("objects", "network_objects_vehicle_carries",
                                           "!network_objects_vehicle_followed(object_index) && ", "", "write-carry")
RIDING_CONTROLS["write-carry-measured-after"] = (
    "objects", "distributed_object_move",
    "\tnetwork_objects_vehicle_writing(object_index, kind);\n\tobject_set_position(object_index, position, forward, up);",
    "\tobject_set_position(object_index, position, forward, up);\n\tnetwork_objects_vehicle_writing(object_index, kind);",
    "write-carry")
RIDING_CONTROLS["write-carry-measured-after-reconcile"] = (
    "objects", "network_objects_reconcile",
    "\t\tnetwork_objects_vehicle_writing(object_index, kind);\n\t\tobject_set_position(object_index, &blended, &valid_forward, &valid_up);",
    "\t\tobject_set_position(object_index, &blended, &valid_forward, &valid_up);\n\t\tnetwork_objects_vehicle_writing(object_index, kind);",
    "write-carry")
RIDING_CONTROLS["write-carry-unwired"] = (
    "objects", "network_objects_vehicle_written",
    "\t\tif (network_objects_vehicle_carries(object_index, kind))\n\t\t\tbiped_riding_vehicle_written(object_index, before_position, before_forward, before_up);\n",
    "", "write-carry")


def riding_script_function_check(text):
    # the standalone tool imports its siblings as top-level modules
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    try:
        from custom_edition_script_names import build_names, build_types
    finally:
        sys.path.pop(0)
    types = build_types()
    assert build_names()[0]['vehicle_riding'] == (types['_hs_type_void'], 'hs_macro_function_parse',
                                                [types['_hs_type_boolean']])
    hs = text['hs']
    table = re.search(r'struct hs_function_table_storage hs_function_table\s*=\s*\{\s*\{(.*?)\n\t\},', hs, re.S)
    assert table
    entries = re.findall(r'&\w+(?:\.definition)?', table.group(1))
    assert entries[-1] == '&vehicle_riding_definition.definition'
    assert hs.count('418 + 8') == 2
    allowed = re.search(r'hs_function_allowed_in_maps\[\]\s*=\s*\{(.*?)\n\};', hs, re.S)
    assert allowed and re.search(r'TRUE, /\* vehicle_riding \*/\s*$', allowed.group(1))
    denied = re.search(r'hs_functions_denied_to_downloaded_maps\[\]\s*=\s*\{(.*?)\n\};', hs, re.S)
    assert denied and '"vehicle_riding"' not in denied.group(1)
    assert 'HS_EVALUATE_VOID_BOOLEAN(hs_vehicle_riding_evaluate, vehicle_riding_map_script)' in hs
    assert 'vehicle_riding_map_reset();' in riding_function(text['vbipeds'], 'vehicle_riding_objects_reset')
    assert 'vehicle_riding_map_initialize();' in riding_function(text['vbipeds'], 'vehicle_riding_objects_initialize')
    # (the engine reads a boolean from its low byte: a false global with upper bytes set stays false)
    assert 'return (boolean)hs_global_evaluate(designator)!=FALSE;' in riding_function(text['hs_runtime'], 'hs_runtime_port_scenario_boolean')
    assert 'network_riding_map_update();' in text['main']
    new_game = riding_function(text['network'], 'network_distributed_new_game')
    assert 'network_riding_map_reset();' in new_game
    handler = riding_function(text['network'], 'network_distributed_handle_message')
    assert 'network_riding_map_handle_message(machine_index, message, size);' in handler
    assert handler.index('network_riding_map_handles_message(message, size)') < handler.index('network_shield_effects_handles_message(message, size)')
    ready = handler[handler.index("case _distributed_message_client_ready:\n\t{"):]
    assert 'network_riding_map_send(machine_index);' in ready[:ready.index('break;')]
    pulse = riding_function(text['game'], 'game_initial_pulse')
    assert 'vehicle_riding_map_start();' in pulse and 'bipeds_riding_latch();' in pulse
    assert pulse.index('vehicle_riding_map_start();') < pulse.index('bipeds_riding_latch();')
    after_load = re.search(r'after_load_procs\[\]\s*=\s*\{(.*?)\n\};', text['game_state'], re.S)
    assert after_load and 'vehicle_riding_map_start' not in after_load.group(1)
    assert 'vehicle_riding_map_start' not in riding_function(text['riding'], 'vehicle_riding_latch')
    assert 'vehicle_riding_map_start' not in riding_function(text['vbipeds'], 'bipeds_riding_latch')
    assert 'vehicle_riding' not in re.findall(r'definition\s*=\s*\{\s*"([a-z0-9_]+)"', text['hs_globals'])


RIDING_CHECKS['script-function'] = riding_script_function_check
RIDING_CONTROLS.update({
    'script-not-allowed': ('hs', None, 'TRUE, /* vehicle_riding */', 'FALSE, /* vehicle_riding */', 'script-function'),
    'script-not-last': ('hs', None, '\t\t&vehicle_riding_definition.definition,\n', '', 'script-function'),
    'global-read-whole-long': ('hs_runtime', 'hs_runtime_port_scenario_boolean', 'return (boolean)hs_global_evaluate(designator)!=FALSE;', 'return hs_global_evaluate(designator)!=FALSE;', 'script-function'),
    'riding-main-update-unwired': ('main', None, 'network_riding_map_update();', '', 'script-function'),
    'riding-new-game-reset-unwired': ('network', 'network_distributed_new_game', 'network_riding_map_reset();', '', 'script-function'),
    'riding-dispatch-unwired': ('network', 'network_distributed_handle_message', 'network_riding_map_handle_message(machine_index, message, size);', '', 'script-function'),
    'riding-client-ready-unsent': ('network', 'network_distributed_handle_message', 'network_riding_map_send(machine_index);', '', 'script-function'),
    'script-map-reset-unwired': ('vbipeds', 'vehicle_riding_objects_reset', 'vehicle_riding_map_reset();', '', 'script-function'),
    'global-read-after-latch': ('game', 'game_initial_pulse', 'vehicle_riding_map_start();\n\tbipeds_riding_latch();',
                              'bipeds_riding_latch();\n\tvehicle_riding_map_start();', 'script-function'),
    'global-read-on-load': ('game_state', None, 'after_load_procs[] =\n{', 'after_load_procs[] =\n{\n\tvehicle_riding_map_start,', 'script-function'),
    'global-shadowed': ('hs_globals', None, 'static struct hs_external_global_definition screenshot_size_definition',
                        'struct hs_external_global_definition vehicle_riding_definition = { "vehicle_riding", 0 };\nstatic struct hs_external_global_definition screenshot_size_definition', 'script-function'),
})


def riding_map_decision_check(text):
    reset = riding_function(text['vbipeds'], 'vehicle_riding_objects_reset')
    assert 'vehicle_riding_map_reset();' in reset and 'vehicle_riding_map_decide();' in reset
    assert reset.index('vehicle_riding_map_reset();') < reset.index('vehicle_riding_map_decide();')
    riding = text['riding']
    reads = [m.start() for m in re.finditer(re.escape('config_boolean("network.vehicle_riding")'), riding)]
    decide = riding.index(riding_function(riding, 'vehicle_riding_map_decide'))
    decide_end = decide + len(riding_function(riding, 'vehicle_riding_map_decide'))
    assert len(reads) == 1 and decide <= reads[0] < decide_end, reads


RIDING_CHECKS['map-decision'] = riding_map_decision_check
RIDING_CONTROLS.update({
    'decide-not-called': ('vbipeds', 'vehicle_riding_objects_reset', 'vehicle_riding_map_decide();', '', 'map-decision'),
    'decide-before-reset': ('vbipeds', 'vehicle_riding_objects_reset', 'vehicle_riding_map_reset();\n\tvehicle_riding_map_decide();',
                            'vehicle_riding_map_decide();\n\tvehicle_riding_map_reset();', 'map-decision'),
    'option-read-in-latch': ('riding', 'vehicle_riding_latch', 'riding_look_carry = config_boolean("debug.riding_look_carry") != 0;',
                             'riding_look_carry = config_boolean("network.vehicle_riding") != 0;', 'map-decision'),
})


RIDING_RULE_LINE = '\telse if (!strcmp(key, "vehicle_riding")) server_gametype_flag(variant, _game_variant_vehicle_riding_bit, value);\n'


def riding_gametype_rule_check(text):
    assert RIDING_RULE_LINE in riding_function(text['gametypes'], 'server_gametype_rule'), 'vehicle_riding is not applied'


RIDING_CHECKS['gametype-rule'] = riding_gametype_rule_check
RIDING_CONTROLS.update({
    'riding-rule-unapplied': ('gametypes', 'server_gametype_rule', RIDING_RULE_LINE, '', 'gametype-rule'),
    'riding-rule-wrong-bit': ('gametypes', 'server_gametype_rule', '_game_variant_vehicle_riding_bit, value);',
                              '_game_variant_generic_starting_equipment_bit, value);', 'gametype-rule'),
})


def riding_entry_wiring_check(text):
    call = 'network_game_port_vehicle_riding_entry(server_entry_riding_rule(entry));'
    apply = riding_function(text['director'], 'server_apply_entry')
    assert apply.count(call) == 1, 'the entry rule is not set exactly once'
    before, after = apply.split(call)
    # (the success point: after the co-op branch and the multiplayer branch, directly before the entry is remembered)
    assert 'ui_widget_port_cooperative_settings_apply();' in before
    assert 'network_game_server_change_game_variant(network_server, &variant);' in before
    assert after.lstrip().startswith('server.current = *entry;'), after[:60]
    # (set for every applied entry: what precedes it is the end of the multiplayer branch, not an if that skips the call)
    lines = [line for line in re.sub(r'/\*.*?\*/', '', before, flags=re.S).split('\n') if line.strip()]
    assert lines[-1] == '\t}', lines[-1]
    root = Path(__file__).resolve().parent.parent
    held = {path: text[key] for key, path in RIDING_SOURCES.items() if key != 'bipeds' and path.endswith('.c')}
    calls = []
    for folder in ('port', 'source'):
        for path in sorted((root / folder).rglob('*.c')):
            name = path.relative_to(root).as_posix()
            source = held[name] if name in held else path.read_text(encoding='latin-1')
            calls += [name for _ in re.finditer(r'(?<!void )network_game_port_vehicle_riding_entry\(', source)]
    assert calls == ['port/linux/game/server_director.c'], calls


RIDING_CHECKS['entry-wiring'] = riding_entry_wiring_check
RIDING_ENTRY_CALL = '\tnetwork_game_port_vehicle_riding_entry(server_entry_riding_rule(entry));\n'
RIDING_CONTROLS.update({
    'entry-rule-only-when-given': ('director', 'server_apply_entry', RIDING_ENTRY_CALL,
                                   '\tif (server_entry_riding_rule(entry) != NONE)\n\t' + RIDING_ENTRY_CALL, 'entry-wiring'),
    'entry-rule-before-apply': ('director', 'server_apply_entry', RIDING_ENTRY_CALL, '', 'entry-wiring'),
})


# (the riding wiring tests: every check and control above)
@pytest.mark.parametrize("name", sorted(RIDING_CHECKS))
def test_riding_network_wiring(name):
    RIDING_CHECKS[name](riding_sources())


@pytest.mark.parametrize("control", sorted(RIDING_CONTROLS))
def test_riding_network_wiring_control(control):
    from tools.harness import mutated
    owner, scope, before, after, check = RIDING_CONTROLS[control]
    text = riding_sources()
    body = riding_function(text[owner], scope) if scope else text[owner]
    text[owner] = text[owner].replace(body, mutated(body, before, after), 1)
    if control == 'riding-before-mcc-guard':
        text['network'] = text['network'].replace('\tif (distributed_riding_elevator(unit_index))\n\t\tposition.z = object->object.position.z;\n\tdx = ',
                                                  RIDING_MCC_GUARD + '\tif (distributed_riding_elevator(unit_index))\n\t\tposition.z = object->object.position.z;\n\tdx = ', 1)
    if control == 'start-after-send':
        text['server'] = text['server'].replace('\t\t\tboolean told = TRUE;\n',
                                                '\t\t\tboolean told = TRUE;\n\t\t\tnetwork_game_port_vehicle_riding_start(&server->game);\n', 1)
    if control == 'entry-rule-before-apply':
        text['director'] = text['director'].replace('\tcsmemset(&variant, 0, sizeof(variant));\n',
                                                    '\tcsmemset(&variant, 0, sizeof(variant));\n' + RIDING_ENTRY_CALL, 1)
    if control == 'script-not-last':
        text['hs'] = text['hs'].replace('\t\t&sv_say_definition.definition,',
                                        '\t\t&vehicle_riding_definition.definition,\n\t\t&sv_say_definition.definition,', 1)
    # (a check finds code by its text: a fault that removes the text fails its .index as ValueError)
    with pytest.raises(AssertionError if check == 'script-function' else (AssertionError, ValueError)):
        RIDING_CHECKS[check](text)


def riding_port_prefix_comments(text):
    return re.findall(r"/\* port:[^\n]*", text)


def test_vehicle_riding_port_files_have_no_port_prefix():
    """port: marks port code inside the decompiled source/ tree; the riding files are all port code."""
    root = Path(__file__).resolve().parent.parent
    for path in sorted((root / "port/linux/game").glob("vehicle_riding*.[ch]")):
        found = riding_port_prefix_comments(path.read_text(encoding="latin-1"))
        assert not found, f"{path.name}: {found[0]}"


def test_vehicle_riding_port_prefix_pin_control():
    text = "\n".join(["#include \"a.h\" /* port: x */", "\t/* port: y */", "/* port: z */",
                      "boolean riding_applies; /* port: riding state */", "/* ---------- port: w */"])
    assert riding_port_prefix_comments(text) == ["/* port: x */", "/* port: y */", "/* port: z */", "/* port: riding state */"]
