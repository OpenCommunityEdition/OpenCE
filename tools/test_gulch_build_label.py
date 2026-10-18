"""Exercise the actual build-label handler and existing widget text ownership."""

import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    start = source.index("static void " + name + "(")
    return source[start:source.index("\n}", start) + 2]


@pytest.mark.parametrize("number,expected", [
    (0, "gulch build local"), (321, "gulch build 321"),
    (2147483647, "gulch build 2147483647"),
])
def test_handler_uses_executable_build_and_owns_only_its_widget(tmp_path, number, expected):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("C compiler unavailable")
    helpers = (ROOT / "port/linux/game/menu_functions.c").read_text(encoding="utf-8")
    code = r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
#define ROW_TEXT_LENGTH 64
struct widget_instance {
 int type; long definition_tag_index;
 struct { struct { wchar_t *text; } text_box; } parameters;
};
static char const *current_name;
static long allocations;
static char const *tag_get_name(long index) { return index == 7 ? current_name : NULL; }
static void *ui_widget_realloc(void *old, size_t size, char const *file, long line) {
 (void)file; (void)line; allocations++; assert(old == NULL);
 return malloc(size);
}
static void ustrncpy(wchar_t *target, wchar_t const *text, size_t length) {
 size_t i; for(i=0;i<length;i++) { target[i]=*text; if(*text) text++; }
}
static void halo_menus_utf16(char const *text, unsigned short *wide, size_t capacity) {
 size_t i; for(i=0;i+1<capacity && text[i];i++) wide[i]=(unsigned char)text[i]; wide[i]=0;
}
static int text_is(struct widget_instance const *widget, char const *expected) {
 size_t i;
 if(!widget->parameters.text_box.text) return 0;
 for(i=0;expected[i];i++) if(widget->parameters.text_box.text[i] != (unsigned char)expected[i]) return 0;
 return widget->parameters.text_box.text[i]==0;
}
''' + function(helpers, "text_set_length") + "\n" + function(helpers, "text_set") + r'''
#include "gulch_build_label.inl"
int main(void) {
 struct widget_instance widget={0}, other={0};
 wchar_t *owned;
 assert(sizeof(wchar_t)==2);
 widget.type=1;widget.definition_tag_index=7;
 current_name="pc\\main_menu\\gulch_build_number";
 assert(gulch_build_label_event(&widget));
 assert(text_is(&widget, EXPECTED));assert(allocations==1);
 owned=widget.parameters.text_box.text;
 assert(gulch_build_label_event(&widget));assert(widget.parameters.text_box.text==owned);
 assert(allocations==1 && text_is(&widget, EXPECTED));
 assert(!gulch_build_label_event(NULL));
 other.type=1;other.definition_tag_index=7;
 current_name="pc\\main_menu\\build_number";
 assert(!gulch_build_label_event(&other));assert(!other.parameters.text_box.text);
 current_name="pc\\main_menu\\gulch_build_number_extra";
 assert(!gulch_build_label_event(&other));
 current_name="pc\\main_menu\\gulch_build_number";
 other.type=0;assert(!gulch_build_label_event(&other));other.type=1;
 other.definition_tag_index=8;assert(!gulch_build_label_event(&other));other.definition_tag_index=7;
 assert(allocations==1);
 free(widget.parameters.text_box.text);
 return 0;
}
'''
    source = tmp_path / "build_label.c"
    binary = tmp_path / ("build_label.exe" if os.name == "nt" else "build_label")
    source.write_text(code, encoding="utf-8")
    flags = ["-std=gnu89", "-fshort-wchar", "-Wall", "-Wextra", "-Werror",
             "-I", str(ROOT / "port/linux/game"), f'-DEXPECTED="{expected}"']
    flags.append(f"-DHALO_BUILD_NUMBER={number}")
    if os.name == "nt":
        flags.insert(0, "--target=i686-pc-windows-msvc")
    built = subprocess.run([compiler, *flags, str(source), "-o", str(binary)],
                           capture_output=True, text=True)
    assert built.returncode == 0, built.stdout + built.stderr
    subprocess.run([str(binary)], check=True, capture_output=True, text=True)
