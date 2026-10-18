"""Compile the actual debug session parser; no maps or game processes needed."""
import re

import pytest

from tools.harness import enum_with, function, read, structure
from tools.test_infection import compile_run


def parser_world():
    source = read('port/linux/game/network_test.c')
    state = re.search(r'static struct\s*\{.*?\} network_test;', source, re.S).group()
    functions = function(source, 'network_test_variant')
    if 'network_test_parse_host(' in source:
        functions += function(source, 'network_test_parse_host')
    functions += function(source, 'network_test_read_settings')
    # The adapter also runs the regression against the original copying API.
    adapter = ('return network_test_variant(index);' if 'const *network_test_variant' in source else
               'static char name[512]; return network_test_variant(index, name, sizeof(name)) ? name : NULL;')
    return r'''
    #define _CRT_SECURE_NO_WARNINGS
    #include <assert.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    typedef int boolean;
    typedef float real;
    #define TRUE 1
    #define FALSE 0
    static const char *setting, *pickup = "";
    static int diagnostics;
    static double chat_time;
    static const char *config_string(const char *key) {
        return !strcmp(key, "debug.network_test") ? setting : pickup;
    }
    static int config_boolean(const char *key) { return 0; }
    static double config_real(const char *key) {
        return !strcmp(key, "debug.network_test_chat") ? chat_time : 0;
    }
    static long config_integer(const char *key) { return 0; }
    static void platform_log(const char *fmt, ...) { diagnostics++; }
    ''' + structure(read('source/networking/network_game_manager.h'), 'network_game_map') + \
        enum_with(source, '_network_test_off') + state + functions + \
        '\nstatic const char *variant(size_t index) {' + adapter + '}\n'


CASES = {
    'exact_64_byte_suffix': r'''
        setting = "host:beavercreek:slayer,infection-combat-host,infection-combat-client";
        assert(strlen(setting + 5) == 64);
        network_test_read_settings();
        assert(network_test.mode == _network_test_host);
        assert(!strcmp(network_test.map_name, "beavercreek"));
        assert(!strcmp(variant(2), "infection-combat-client"));
        assert(!variant(3));
    ''',
    'long_list_and_token': r'''
        char text[200000], token[256]; size_t i;
        memset(token, 'a', sizeof(token)-1); token[sizeof(token)-1] = 0;
        strcpy(text, "host:beavercreek:"); strcat(text, token);
        for (i = 0; i < 33000; i++) strcat(text, ",coop");
        setting = text; network_test_read_settings();
        assert(network_test.mode == _network_test_host);
        assert(!strcmp(variant(0), token));
        assert(!strcmp(variant(33000), "coop"));
        assert(!variant(33001)); assert(!variant((size_t)-1));
        memset(text, 'z', strlen(text)); /* parser owns its snapshot */
        assert(!strcmp(variant(0), token));
    ''',
    'default_variant': r'''
        setting = "host:beavercreek"; network_test_read_settings();
        assert(!strcmp(variant(0), "slayer")); assert(!variant(1));
    ''',
    'long_pickup_filter': r'''
        char filter[300]; memset(filter, 'p', 299); filter[299] = 0;
        setting = "join"; pickup = filter; network_test_read_settings();
        assert(!strcmp(network_test.pickup_weapon, filter));
    ''',
    'chat_with_untruncated_infection_settings': r'''
        char filter[300]; memset(filter, 'p', 299); filter[299] = 0;
        setting = "host:beavercreek:slayer,infection-combat-host,infection-combat-client";
        pickup = filter; chat_time = 2.5; network_test_read_settings();
        assert(network_test.mode == _network_test_host);
        assert(network_test.chat_time == 2.5f);
        assert(!strcmp(variant(2), "infection-combat-client"));
        assert(!strcmp(network_test.pickup_weapon, filter));
        filter[0] = 'x'; /* the fixture owns both string snapshots */
        assert(network_test.pickup_weapon[0] == 'p');
    ''',
    'map_path_boundary': r'''
        char text[140]; strcpy(text, "host:custom_maps\\");
        memset(text + strlen(text), 'a', 127 - strlen(text + 5)); text[132] = 0;
        setting = text; network_test_read_settings();
        assert(network_test.mode == _network_test_host);
        assert(strlen(network_test.map_name) == 127);
    ''',
}


@pytest.mark.parametrize('case', CASES)
def test_parser(tmp_path, case):
    assert compile_run(tmp_path, parser_world() + '\nint main(void) {' + CASES[case] + 'return 0;}') == 0


@pytest.mark.parametrize('setting', [
    'host:', 'host::slayer', 'host:beavercreek:', 'host:beavercreek:slayer,,coop',
    'host:beavercreek:slayer,', 'host:beavercreek:,slayer',
    'host:' + 'a' * 58, 'host:custom_maps\\' + 'a' * 116,
])
def test_invalid_settings_fail_closed(tmp_path, setting):
    literal = setting.replace('\\', '\\\\')
    case = f'setting="{literal}"; network_test_read_settings(); assert(network_test.mode == _network_test_off); assert(diagnostics);'
    assert compile_run(tmp_path, parser_world() + '\nint main(void) {' + case + 'return 0;}') == 0
