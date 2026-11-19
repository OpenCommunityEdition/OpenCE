"""The real attract_mode_should_start and attract_mode_movies_available (source/interface/attract_mode.c): after 75
seconds at the main menu the music stops and a demo movie starts, only with display.attract_mode and one of the
three movies in the bink folder (any language); without them the music plays on. ui_widget.c's
main_menu_screen_is_active takes the PC menus' main menu too."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["movies", "no movies", "off"]
NEGATIVE_CONTROLS = {
    # the music stopped without a movie to play
    "music-stopped": (("if (movies_available && time_elapsed>=ATTRACT_MODE_COUNTDOWN-MUSIC_FADE_TIME)",
                       "if (time_elapsed>=ATTRACT_MODE_COUNTDOWN-MUSIC_FADE_TIME)"), "no movies"),
    # attract mode without a movie to play
    "started": (("if (movies_available && time_elapsed>=ATTRACT_MODE_COUNTDOWN)",
                 "if (time_elapsed>=ATTRACT_MODE_COUNTDOWN)"), "no movies"),
    # the setting not read
    "setting": (('return available && config_boolean("display.attract_mode");', "return available;"), "off"),
    # only the movies of no language looked for
    "languages": (('const char *suffixes[] = { "", "_de", "_fr", "_es", "_it" };', 'const char *suffixes[] = { "" };'),
                  "movies"),
}


def generated(fault=None):
    source = read("source/interface/attract_mode.c").replace("\r\n", "\n")
    names = ["ATTRACT_MODE_COUNTDOWN", "MUSIC_FADE_TIME"]
    attract = read("source/interface/attract_mode.h")
    config = "enum { " + ", ".join(f"{name} = {constant(source, name)}" for name in names) + " };\n" + \
        enum_with(attract.replace("\r\n", "\n"), "NUMBER_OF_ATTRACT_MODE_MOVIES") + "\n"
    code = function(source, "attract_mode_movies_available") + "\n" + function(source, "attract_mode_should_start")
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("attract_mode", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("attract_mode", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_pc_main_menu():
    """The PC menus' main menu (main_menu/main_menu, a widget of theirs) counts as the main menu, as the Xbox's."""
    check = function(read("source/interface/ui_widget.c").replace("\r\n", "\n"), "main_menu_screen_is_active")
    assert 'strcmp(widget_globals.active_widgets[0]->name, "the_main_menu") == 0 ||' in check
    assert ('(pc_menu_tag(widget_globals.active_widgets[0]->definition_tag_index) &&\n'
            '\t\t\t\tstrcmp(widget_globals.active_widgets[0]->name, "main_menu") == 0)') in check
    assert '<widget name="main_menu/main_menu"' in read("port/assets/menus/ce/main_menu.xml")
