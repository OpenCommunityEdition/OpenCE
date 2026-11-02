"""Controls Setup's B: it cancels a key being set (as Escape does), else leaves the screen."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ['cancel', 'leave', 'escape', 'earlier-kept']


def generated(fault=None):
    menus = read('port/linux/game/menu_functions.c')
    platform = read('port/linux/src/sdl_platform.c')
    start = platform.index('enum\n{\n\t_binding_capture_idle')
    code = platform[start:platform.index('static unsigned mouse_buttons_down;', start)]
    code += '\n'.join(function(platform, name) for name in
                      ['platform_binding_capture_begin', 'platform_binding_capture_cancel',
                       'platform_binding_capture_poll']) + '\n'
    start = menus.index('#define CONTROL_BINDINGS')
    code += menus[start:menus.index('static void control_bindings_read', start)]
    code += '\n'.join(function(menus, name) for name in
                      ['control_bindings_read', 'control_bindings_text', 'controls_load', 'controls_save',
                       'control_row', 'control_of_row', 'controls_group', 'control_capture_begin',
                       'control_capture_poll', 'controls_back']) + '\n'
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('controls_capture', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    # (B leaves the screen with a key capture pending)
    (('if (controls_screen.capturing_control != NONE)\n\t{\n\t\tcontrols_screen.capturing_control = NONE;',
      'if (FALSE)\n\t{\n\t\tcontrols_screen.capturing_control = NONE;'), 'cancel'),
    (('if (controls_screen.capturing_control != NONE)\n\t{\n\t\tcontrols_screen.capturing_control = NONE;',
      'if (FALSE)\n\t{\n\t\tcontrols_screen.capturing_control = NONE;'), 'earlier-kept'),
    (('controls_screen.capturing_control = NONE;\n\t\tplatform_binding_capture_cancel();',
      'platform_binding_capture_cancel();'), 'cancel'),
    (('\t\tplatform_binding_capture_cancel();\n', ''), 'cancel'),
    (('\tbinding_capture = _binding_capture_idle;\n\tpthread_mutex_unlock', '\tpthread_mutex_unlock'), 'cancel'),
    # (a cancel that reloads the screen's bindings)
    (('controls_screen.capturing_control = NONE;\n\t\tplatform_binding_capture_cancel();',
      'controls_load(FALSE);\n\t\tplatform_binding_capture_cancel();'), 'earlier-kept'),
    (('\tui_widget_port_go_back(widget);\n\t*widget_deleted = TRUE;', '\t*widget_deleted = TRUE;'), 'leave'),
])
def test_negative_control(fault, case):
    status, output = run(build('controls_capture', generated(fault)), case)
    assert status == CHECK_FAILED, output
