"""The real HUD button styles of controller_buttons.c: "auto" draws each player's controller's family's style (a kind
with no family of its own the Xbox's), a family's setting naming another family's style is that family's default,
one chosen style is everyone's, the settings are read again only when one changes, port_config.c allows every style
and only those, and controller_glyphs.c finds every artwork of each style it draws (and none of the Xbox's first
three, which hud_hires_tags.c draws)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["auto-by-family", "wrong-family-setting", "one-style", "settings-cached", "settings-table", "glyph-artwork"]

BUTTONS = "port/linux/src/controller_buttons.c"
GLYPHS = "port/linux/game/controller_glyphs.c"

# a fault in the code (its file, the change) and the cases that must catch it
NEGATIVE_CONTROLS = {
    "ps5-unknown": ((BUTTONS, "\tcase SDL_GAMEPAD_TYPE_PS5:\n", ""), ["auto-by-family"]),
    "family-unchecked": ((BUTTONS, "controller_button_style_family(style) == family ? style : family_defaults[family]",
                          "style >= 0 ? style : family_defaults[family]"), ["wrong-family-setting"]),
    "never-cached": ((BUTTONS, "\t\tread_at = config_changes();\n", ""), ["settings-cached"]),
    "no-dark-gems-alias": ((BUTTONS, '\tif (!strcmp(name, "dark_gems"))\n\t\treturn _button_style_dark_gems;\n', ""),
                           ["one-style", "settings-table"]),
    "controls-by-style": ((GLYPHS, "family_names[controller_button_style_family(each_style)]",
                           "controller_button_style_name(each_style)"), ["glyph-artwork"]),
    "faceless-styles-kept": ((GLYPHS, "if (indices[each_style][0] == NONE)", "if (0)"), ["glyph-artwork"]),
}


def allowed(config, setting):
    """port_config.c's allowed values of a setting, as a C array"""
    match = re.search(r'\{ "' + re.escape(setting) + r'", \{ (.*?)NULL \}', config, re.S)
    if not match:
        raise LookupError(f"no allowed values in port_config.c: {setting}")
    values = re.findall(r'"([^"]+)"', match.group(1))
    name = "allowed_" + (setting.rsplit("_", 1)[-1] if setting != "display.hud_button_style" else "styles")
    return f"static char const *const {name}[] = {{ {', '.join(chr(34) + value + chr(34) for value in values)} }};"


def generated(fault=None):
    sources = {BUTTONS: read(BUTTONS), GLYPHS: read(GLYPHS)}
    if fault:
        file, before, after = NEGATIVE_CONTROLS[fault][0]
        sources[file] = mutated(sources[file], before, after)
    # (the file without its includes: the test gives SDL and the settings)
    buttons = "\n".join(line for line in sources[BUTTONS].splitlines() if not line.startswith("#include"))
    # (the artwork table and its lookup, not the drawing)
    glyphs = sources[GLYPHS]
    glyphs = glyphs[glyphs.index("/* the button slots"):glyphs.index("static struct\n{\n\tstruct bitmap_data")]
    glyphs += function(sources[GLYPHS], "controller_glyph_asset")
    config = read("port/linux/src/port_config.c")
    settings = ["display.hud_button_style", "display.hud_button_style_xbox", "display.hud_button_style_ps",
                "display.hud_button_style_switch"]
    return (("config.inc", read("port/linux/include/controller_buttons.h")),
            ("buttons.inc", buttons + "\n"),
            ("glyphs.inc", glyphs + "\n"),
            ("controller_glyph_assets.h", read("port/linux/game/controller_glyph_assets.h")),
            ("allowed.inc", "\n".join(allowed(config, setting) for setting in settings) + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_controller_button_styles(case):
    status, output = run(build("controller_button_styles", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("fault,case", [(fault, case) for fault, (_, cases) in NEGATIVE_CONTROLS.items()
                                        for case in cases])
def test_fault_detected(fault, case):
    status, output = run(build("controller_button_styles", generated(fault)), case)
    assert status == CHECK_FAILED, output
