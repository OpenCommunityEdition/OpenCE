"""Exercise MCC traversal and the real shared callback dispatch without assets."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"


@pytest.fixture(scope="module")
def validation_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if compiler is None:
        pytest.skip("clang is required for native MCC validation tests")
    directory = tmp_path_factory.mktemp("mcc-validation")
    (directory / "cache").mkdir()
    (directory / "tag_files").mkdir()
    (directory / "cseries.h").write_text("""
#ifndef TEST_CSERIES_H
#define TEST_CSERIES_H
#include <stddef.h>
#include <stdlib.h>
void *validation_malloc(size_t size);
void validation_free(void *pointer);
/* Match cseries.h: malloc/free are paired wrappers; calloc is not. */
#define malloc validation_malloc
#define free validation_free
typedef unsigned char byte;
typedef unsigned char boolean;
typedef float real;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define UNSIGNED_SHORT_MAX 65535
#define FLAG(n) (1UL << (n))
#define TEST_FLAG(v,n) (((v) & FLAG(n)) != 0)
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
""")
    (directory / "cache/physical_memory_map.h").write_text("#define TAG_CACHE_SIZE 0x01600000\n")
    (directory / "tag_files/tag_groups.h").write_text("""
#ifndef TEST_TAG_GROUPS_H
#define TEST_TAG_GROUPS_H
#include "cseries.h"
struct tag_block { long count; void *address; void *definition; };
struct tag_data { long size; unsigned long pad; long file_offset; void *address; void *definition; };
struct tag_reference { unsigned long group_tag; char *name; long name_length; long index; };
#endif
""")
    binary = directory / ("validation.exe" if os.name == "nt" else "validation")
    allocator = directory / "allocator.c"
    allocator.write_text('''
#include <stdlib.h>
static void *allocations[100000];
void *validation_malloc(size_t size) {
    unsigned i;
    void *p = malloc(size);
    if (!p) return NULL;
    for (i = 0; i < 100000; i++) if (!allocations[i]) {
        allocations[i] = p; return p;
    }
    abort();
}
void validation_free(void *p) {
    unsigned i;
    if (!p) return;
    for (i = 0; i < 100000; i++) if (allocations[i] == p) {
        allocations[i] = NULL; free(p); return;
    }
    abort(); /* Detect mixing the engine allocator with CRT calloc. */
}
''')
    target = (["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
              if sys.platform == "win32" else ["-m32"])
    command = [compiler, *target, "-std=gnu99", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
               "-Wno-multichar", "-fsanitize=undefined", "-fsanitize-trap=undefined",
               f"-I{directory}", f"-I{GAME}", str(GAME / "mcc_tag_validate.c"),
               str(GAME / "tag_validate.c"), str(ROOT / "tools/harness/tests/mcc_tag_validation.c"), str(allocator),
               "-o", str(binary)]
    built = subprocess.run(command, capture_output=True, text=True)
    assert built.returncode == 0, built.stdout + built.stderr
    # The existing map_validate executable links tag_validate.c without any
    # game runtime adapters. MCC callback routing must retain that property.
    legacy = directory / "legacy.c"
    legacy.write_text('''
#include "cseries.h"
#include "tag_schema.h"
struct tag_schema_group const *const tag_schema_group_lists[] = {NULL};
struct tag_schema_group const tag_schema_custom_edition_groups[] = {{0, {0, 0}, NULL}};
void tag_validate_report(char const *message) { (void)message; }
int main(void) { return tag_validate_corrections() != 0; }
''')
    legacy_binary = directory / ("legacy.exe" if os.name == "nt" else "legacy")
    standalone = command[:command.index(str(GAME / "mcc_tag_validate.c"))] + [
        str(GAME / "tag_validate.c"), str(legacy), "-o", str(legacy_binary)]
    linked = subprocess.run(standalone, capture_output=True, text=True)
    assert linked.returncode == 0, linked.stdout + linked.stderr
    assert subprocess.run([str(legacy_binary)], timeout=30).returncode == 0
    return binary


@pytest.mark.parametrize("case", ["valid", "overlap", "outside", "negative", "cycle", "alias", "alias_partial", "alias_group", "alias_bad_child", "alias_block", "alias_block_partial",
                                     "graph_model", "graph_model_mismatch", "graph_model_outside", "graph_model_cycle", "graph_model_tail", "graph_model_tail_conflict", "graph_model_tail_cycle", "graph_model_empty", "graph_model_empty_partial", "damage_names",
                                     "prefix_smaller", "prefix_expand", "prefix_tail_overlap", "empty_weapon", "empty_weapon_nonzero",
                                     "animation_data", "animation_data_outside",
                                     "file_range", "handle", "unknown_group", "bsp",
                                     "legacy_isolation", "callbacks", "misaligned", "stream_hole",
                                     "collection", "collection_outside", "device", "device_outside",
                                     "trimmed_structure", "graph_large", "graph_cycle", "graph_node_cycle",
                                     "graph_orphan", "graph_missing_parent", "graph_conflicting_parent", "graph_missing_parent_cycle", "graph_root_sibling", "graph_root_outside", "graph_root_orphan", "bitmap_owned", "bitmap_unowned", "first_person_slots",
                                     "grenades_zero", "grenades_one", "grenades_two", "grenades_four", "grenades_excess",
                                     "anchor_0", "anchor_4", "anchor_5", "anchor_6", "anchor_7", "anchor_8", "anchor_9", "anchor_-1",
                                     "flag_name", "names_512", "names_513", "names_640", "names_32767", "names_32768",
                                     "palette_100", "palette_101", "palette_32767", "palette_32768",
                                     "palette_bad_index", "palette_outside", "palette_overlap",
                                     "syntax_full", "syntax_overflow", "syntax_outside", "syntax_overlap", "syntax_legacy"])
def test_mcc_validation(validation_tool, case):
    result = subprocess.run([str(validation_tool), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
