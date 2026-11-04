"""The mutators' files (port/linux/src/port_mutators.c) read from a fake mutators folder, without assets."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run
from embed_assets import MUTATOR_ASSETS, ROOT, mutator_folders, words


def stock_mutators():
    """mutator_files.h, with the stock mutators port/assets/mutators has, as tools/embed_assets.py embeds them"""
    text = read('port/linux/src/mutator_files.h')
    folders = mutator_folders()
    for index, name in enumerate(folders):
        data = (ROOT / MUTATOR_ASSETS / name / 'mutator.toml').read_bytes()
        text += f'static const unsigned int mutator{index}[] = {{\n' + '\n'.join(words(data + b'\0')) + '\n};\n'
    text += 'const struct mutator_file_embedded mutator_files_embedded[] = {\n'
    text += ''.join(f'\t{{ "{name}", mutator{index}, '
                    f'{(ROOT / MUTATOR_ASSETS / name / "mutator.toml").stat().st_size} }},\n'
                    for index, name in enumerate(folders))
    return text + f'}};\nconst unsigned int mutator_files_embedded_count = {len(folders)};\n'


# the platform layer's, as port_mutators.c calls them: a fake folder
# (port_mutators.c's own test file's folders[]), the files written (the
# stock mutators: files_written[], each written to and read back from it,
# and not written while files_unwritable), game.mutators and a log
STUBS = (
    ('platform.h', '#define platform_log(...) ((void)logged++)\n'
                   'static char const *platform_data_root(void) { return "/data"; }\n'),
    ('port_config.h', 'static struct { char path[256]; char *text; } files_written[16];\n'
                      'static int file_writes, files_unwritable;\n'
                      '/* (game.mutator_options a setting of its own; the rest game.mutators) */\n'
                      'static char option_setting[300];\n'
                      'static char const *config_string(char const *name)\n'
                      '{ return !strcmp(name, "game.mutator_options") ? option_setting : setting; }\n'
                      'static int config_boolean(char const *name) { (void)name; return 1; }\n'
                      'static int config_write(char const *name, char const *value)\n'
                      '{ if (!strcmp(name, "game.mutator_options"))\n'
                      '  { snprintf(option_setting, sizeof(option_setting), "%s", value); return 1; }\n'
                      '  snprintf(written, sizeof(written), "%s", value);\n'
                      '  snprintf(setting, sizeof(setting), "%s", value); return 1; }\n'
                      'static char *config_file_read(char const *path, size_t *size)\n'
                      '{\n\tint index;\n\tfor (index = 0; index < 16; index++)\n\t{\n'
                      '\t\tif (files_written[index].text && !strcmp(path, files_written[index].path))\n'
                      '\t\t{ *size = strlen(files_written[index].text); return strdup(files_written[index].text); }\n'
                      '\t}\n\tfor (index = 0; index < file_count; index++)\n\t{\n'
                      '\t\tif (!strcmp(path, files[index].path))\n'
                      '\t\t{ char *bytes = malloc(files[index].size + 1);\n'
                      '\t\t  memcpy(bytes, files[index].bytes, files[index].size + 1);\n'
                      '\t\t  *size = files[index].size; return bytes; }\n'
                      '\t}\n\tfor (index = 0; index < folder_count; index++)\n\t{\n'
                      '\t\tchar wanted[256];\n'
                      '\t\tsnprintf(wanted, sizeof(wanted), "/data/mutators/%s/mutator.toml", folders[index].name);\n'
                      '\t\tif (folders[index].text && !strcmp(path, wanted))\n'
                      '\t\t{ reads++; *size = folders[index].size; return strdup(folders[index].text); }\n'
                      '\t}\n\treturn NULL;\n}\n'
                      'static int config_file_write(char const *path, char const *text)\n'
                      '{\n\tint index;\n\tif (files_unwritable)\n\t\treturn 0;\n'
                      '\tfor (index = 0; index < 16 && files_written[index].text &&\n'
                      '\t\tstrcmp(path, files_written[index].path); index++)\n\t{\n\t}\n'
                      '\tif (index == 16)\n\t\treturn 0;\n'
                      '\tsnprintf(files_written[index].path, sizeof(files_written[index].path), "%s", path);\n'
                      '\tfree(files_written[index].text);\n'
                      '\tfiles_written[index].text = strdup(text);\n\tfile_writes++;\n\treturn 1;\n}\n'),
    ('posix.h', 'typedef unsigned long posix_ulong;\nenum { _posix_file_is_directory = 1 };\n'
                'struct posix_file_information { posix_ulong flags, size_low, size_high; };\n'
                'static void *posix_directory_open(char const *path)\n'
                '{ listed = 0; return strcmp(path, "/data/mutators") ? NULL : (void *)folders; }\n'
                'static int posix_directory_next(void *directory, char *name, posix_ulong size)\n'
                '{ (void)directory; if (listed >= folder_count) return 0;\n'
                '  snprintf(name, size, "%s", folders[listed++].name); return 1; }\n'
                'static void posix_directory_close(void *directory) { (void)directory; }\n'
                'static int posix_make_directory(char const *path) { (void)path; return 0; }\n'
                'static int posix_stat(char const *path, struct posix_file_information *information)\n'
                '{\n\tint index;\n\tmemset(information, 0, sizeof(*information));\n'
                '\tfor (index = 0; index < folder_count; index++)\n\t{\n\t\tchar wanted[256];\n'
                '\t\tsnprintf(wanted, sizeof(wanted), "/data/mutators/%s", folders[index].name);\n'
                '\t\tif (!strcmp(path, wanted)) { information->flags = _posix_file_is_directory; return 0; }\n'
                '\t\tstrcat(wanted, "/mutator.toml");\n'
                '\t\tif (folders[index].text && !strcmp(path, wanted))\n'
                '\t\t{ information->size_low = folders[index].size; return 0; }\n\t}\n'
                '\tfor (index = 0; index < file_count; index++)\n'
                '\t\tif (!strcmp(path, files[index].path)) { information->size_low = files[index].size; return 0; }\n'
                '\treturn -1;\n}\n'),
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

# the game's check of each tag edit (mutator_tags.c, which test_mutator_tags.py tests)
CHECK_TAG_EDIT = '''
int mutators_tag_edit_check(const unsigned char *edit, long size, char *reason, unsigned long reason_size)
{
	(void)edit;
	(void)size;
	(void)reason;
	(void)reason_size;
	return 1;
}
'''


def sha256_source(source):
    """port_mutators.c with p2p_crypto.c's SHA-256 in place of its prototype"""
    crypto = read('port/linux/src/p2p_crypto.c')
    sha256 = crypto[crypto.index('/* ---------- SHA-256 */'):crypto.index('void p2p_hmac_sha256')]
    return mutated(source, 'void p2p_sha256(const void *data, int size, unsigned char *digest);\n', sha256)


def generated(fault=None):
    source = sha256_source(read('port/linux/src/port_mutators.c'))
    if fault == 'unknown_key':
        source = mutated(source, '\t\t\trefuse(mutator, path, reason);\n\t\t}\n\t}\n\tvalue = toml_get(result.toptab, "effects");',
                         '\t\t}\n\t}\n\tvalue = toml_get(result.toptab, "effects");')
    if fault == 'size':
        source = mutated(source, 'information.size_low > MAXIMUM_MUTATOR_FILE_SIZE', 'information.size_low > 0x7FFFFFFF')
    if fault == 'refused_on':
        source = mutated(source, 'if (!mutator->on || mutator->refusal[0])\n\t\t\tcontinue;\n\t\tfor (key = 0;',
                         'if (!mutator->on)\n\t\t\tcontinue;\n\t\tfor (key = 0;')
    if fault == 'addon_path':
        source = mutated(source, "if (!name[0] || strchr(name, '/') || strchr(name, '\\\\') ||",
                         "if (!name[0] ||")
    if fault == 'same_files':
        source = mutated(source, '!memcmp(added->digest, port_mutators.mutators[index].digest, SHA256_SIZE))',
                         'FALSE)')
    if fault == 'same_folder':
        source = mutated(source, 'same_folder = 1;', 'same_folder = 0;')
    if fault == 'kind':
        source = mutated(source, '\telse\n\t{\n\t\trefuse(mutator, path, "its addon is not a .cevehicle or a .ceweapon");',
                         '\telse\n\t{\n\t\tmutator->addon_kind = _port_mutator_addon_vehicle;')
    if fault == 'at':
        source = mutated(source, " || *id == '@')", ")")
    if fault == 'rewritten':
        source = mutated(source, 'if (!file_holds(path, text) && !config_file_write(path, text))',
                         'if (!config_file_write(path, text))')
    if fault == 'not_overwritten':
        source = mutated(source, 'if (!file_holds(path, text) && !config_file_write(path, text))',
                         'if (!config_file_read(path, &size_unused) && !config_file_write(path, text))')
        source = mutated(source, '\tconst char *text = (const char *)mutator_files_embedded[index].data;\n',
                         '\tconst char *text = (const char *)mutator_files_embedded[index].data;\n\t\tsize_t size_unused;\n')
    if fault == 'stock_from_file':
        source = mutated(source, 'mutator_read(mutator, file->folder, (const char *)file->data, file->size, path);',
                         '{ size_t read_size = 0; char *read_text = config_file_read(path, &read_size);\n'
                         '\t\tif (read_text) mutator_read(mutator, file->folder, read_text, read_size, path);\n'
                         '\t\tfree(read_text); }')
    source = source.replace('int mutators_key_check(const char *key, double value, char *reason, unsigned long size);',
                            CHECK_KEY)
    source = source.replace('int mutators_tag_edit_check(const unsigned char *edit, long size, char *reason, '
                            'unsigned long reason_size);', CHECK_TAG_EDIT)
    return STUBS + (('port_mutators.h', read('port/linux/src/port_mutators.h')),
                    ('mutator_files.h', stock_mutators()),
                    ('tomlc17.h', read('port/third_party/tomlc17/tomlc17.h')),
                    ('tomlc17.inc', read('port/third_party/tomlc17/tomlc17.c')),
                    ('port_mutators_source.inc', source))


@pytest.mark.parametrize('case', ['stock', 'stock_unwritable', 'folders', 'scripts', 'addons', 'weapons', 'ids', 'menu'])
def test_case(case):
    status, output = run(build('port_mutators', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [('unknown_key', 'folders'), ('size', 'folders'), ('refused_on', 'folders'),
                                        ('rewritten', 'stock'), ('not_overwritten', 'stock'),
                                        ('stock_from_file', 'stock_unwritable'), ('addon_path', 'addons'),
                                        ('same_files', 'ids'), ('same_folder', 'ids'), ('at', 'ids'),
                                        ('kind', 'weapons')])
def test_negative(fault, case):
    status, output = run(build('port_mutators', generated(fault)), case)
    assert status == CHECK_FAILED, output
