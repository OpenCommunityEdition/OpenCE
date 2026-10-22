"""The mutators' files (port/linux/src/port_mutators.c) read from a fake mutators folder, without assets."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run

# the platform layer's, as port_mutators.c calls them: a fake folder
# (port_mutators.c's own test file's folders[]), game.mutators and a log
STUBS = (
    ('platform.h', '#define platform_log(...) ((void)logged++)\n'
                   'static char const *platform_data_root(void) { return "/data"; }\n'),
    ('port_config.h', 'static char const *config_string(char const *name) { (void)name; return setting; }\n'
                      'static int config_write(char const *name, char const *value)\n'
                      '{ (void)name; snprintf(written, sizeof(written), "%s", value);\n'
                      '  snprintf(setting, sizeof(setting), "%s", value); return 1; }\n'
                      'static char *config_file_read(char const *path, size_t *size)\n'
                      '{\n\tint index;\n\tfor (index = 0; index < folder_count; index++)\n\t{\n'
                      '\t\tchar wanted[256];\n'
                      '\t\tsnprintf(wanted, sizeof(wanted), "/data/mutators/%s/mutator.toml", folders[index].name);\n'
                      '\t\tif (folders[index].text && !strcmp(path, wanted))\n'
                      '\t\t{ reads++; *size = folders[index].size; return strdup(folders[index].text); }\n'
                      '\t}\n\treturn NULL;\n}\n'),
    ('posix.h', 'typedef unsigned long posix_ulong;\nenum { _posix_file_is_directory = 1 };\n'
                'struct posix_file_information { posix_ulong flags, size_low, size_high; };\n'
                'static void *posix_directory_open(char const *path)\n'
                '{ listed = 0; return strcmp(path, "/data/mutators") ? NULL : (void *)folders; }\n'
                'static int posix_directory_next(void *directory, char *name, posix_ulong size)\n'
                '{ (void)directory; if (listed >= folder_count) return 0;\n'
                '  snprintf(name, size, "%s", folders[listed++].name); return 1; }\n'
                'static void posix_directory_close(void *directory) { (void)directory; }\n'
                'static int posix_stat(char const *path, struct posix_file_information *information)\n'
                '{\n\tint index;\n\tmemset(information, 0, sizeof(*information));\n'
                '\tfor (index = 0; index < folder_count; index++)\n\t{\n\t\tchar wanted[256];\n'
                '\t\tsnprintf(wanted, sizeof(wanted), "/data/mutators/%s", folders[index].name);\n'
                '\t\tif (!strcmp(path, wanted)) { information->flags = _posix_file_is_directory; return 0; }\n'
                '\t\tstrcat(wanted, "/mutator.toml");\n'
                '\t\tif (folders[index].text && !strcmp(path, wanted))\n'
                '\t\t{ information->size_low = folders[index].size; return 0; }\n\t}\n\treturn -1;\n}\n'),
)

# the game's check of each key (port/linux/game/mutators.c, which test_mutators.py tests)
CHECK_KEY = '''
int mutators_key_check(const char *key, double value, char *reason, unsigned long size)
{
	if (!strcmp(key, "big_head") && value == 1.0)
		return 1;
	if (!strcmp(key, "big_head.scale") && value >= 1.0 && value <= 4.0)
		return 1;
	snprintf(reason, size, strcmp(key, "big_head.scale") ? "this game has no effect %s" : "%s must be from 1 to 4",
		key);
	return 0;
}
'''


def generated(fault=None):
    source = read('port/linux/src/port_mutators.c')
    if fault == 'unknown_key':
        source = mutated(source, '\t\t\trefuse(mutator, path, reason);\n\t\t}\n\t}\n\tvalue = toml_get(result.toptab, "effects");',
                         '\t\t}\n\t}\n\tvalue = toml_get(result.toptab, "effects");')
    if fault == 'size':
        source = mutated(source, 'information.size_low > MAXIMUM_MUTATOR_FILE_SIZE', 'information.size_low > 0x7FFFFFFF')
    if fault == 'refused_on':
        source = mutated(source, 'if (!mutator->on || mutator->refusal[0])\n\t\t\tcontinue;\n\t\tfor (key = 0;',
                         'if (!mutator->on)\n\t\t\tcontinue;\n\t\tfor (key = 0;')
    source = source.replace('int mutators_key_check(const char *key, double value, char *reason, unsigned long size);',
                            CHECK_KEY)
    return STUBS + (('port_mutators.h', read('port/linux/src/port_mutators.h')),
                    ('tomlc17.h', read('port/third_party/tomlc17/tomlc17.h')),
                    ('tomlc17.inc', read('port/third_party/tomlc17/tomlc17.c')),
                    ('port_mutators_source.inc', source))


@pytest.mark.parametrize('case', ['built_in', 'folders', 'menu'])
def test_case(case):
    status, output = run(build('port_mutators', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault', ['unknown_key', 'size', 'refused_on'])
def test_negative(fault):
    status, output = run(build('port_mutators', generated(fault)), 'folders')
    assert status == CHECK_FAILED, output
