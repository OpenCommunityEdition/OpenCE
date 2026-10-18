"""Exercise generated build metadata, the actual label and widget text ownership."""

import io
import os
from pathlib import Path
import re
import shutil
import subprocess
from types import SimpleNamespace

import pytest

from tools import android_build, linux_build, ninja_syntax, web_build, windows_build

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    start = source.index("static void " + name + "(")
    return source[start:source.index("\n}", start) + 2]


def run_handler(tmp_path, metadata_flags, expected):
    tmp_path.mkdir(exist_ok=True)
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
    flags.extend(metadata_flags)
    if os.name == "nt":
        flags.insert(0, "--target=i686-pc-windows-msvc")
    built = subprocess.run([compiler, *flags, str(source), "-o", str(binary)],
                           capture_output=True, text=True)
    assert built.returncode == 0, built.stdout + built.stderr
    subprocess.run([str(binary)], check=True, capture_output=True, text=True)


@pytest.mark.parametrize("number,expected", [
    (0, "gulch build local"), (321, "gulch build 321"),
    (2147483647, "gulch build 2147483647"),
])
def test_handler_uses_executable_build_and_owns_only_its_widget(tmp_path, number, expected):
    run_handler(tmp_path, [f"-DHALO_BUILD_NUMBER={number}"], expected)


def menu_compile_flags(platform, monkeypatch, tmp_path):
    """Run the real graph generators without downloading or compiling dependencies."""
    output = io.StringIO()
    sln = SimpleNamespace(build_dir=tmp_path / "graph", linux_cc="clang", windows_cc="clang",
                          compiler_launcher=None, port_release=True, port_profile=False,
                          port_portable=False, port_lto="off", port_pgo="off", port_pgo_profile=None,
                          web_emcc="/test/emsdk/emcc", android_guest_cc="clang")
    with monkeypatch.context() as setup:
        setup.chdir(ROOT)
        if platform == "windows":
            setup.setattr(windows_build.sys, "platform", "win32")
            setup.setattr(windows_build, "fetch_sdl", lambda: None)
            setup.setattr(windows_build, "inline_export_wrapper", lambda source: source)
            generate = windows_build.generate_windows_build
        elif platform == "android":
            sln.android_ndk = tmp_path / "ndk"
            (sln.android_ndk / "toolchains/llvm/prebuilt/test-host").mkdir(parents=True, exist_ok=True)
            setup.setenv("ANDROID_NDK_HOST_TAG", "test-host")
            setup.setattr(android_build, "fetch_third_party", lambda: None)
            generate = android_build.generate_android_build
        else:
            generate = {"linux": linux_build.generate_linux_build,
                        "web": web_build.generate_web_build}[platform]
        generate(ninja_syntax.Writer(output), sln)
    text = re.sub(r" \$\n *", " ", output.getvalue())
    rule = "android_guest_cc" if platform == "android" else platform + "_cc"
    source = r"port[/\\]linux[/\\]game[/\\]menu_functions\.c"
    match = re.search(r"^build [^\n]*: " + rule + " " + source +
                      r"(?: [^\n]*)?\n  cflags = ([^\n]*)", text, re.M)
    assert match, f"{platform} graph did not compile menu_functions.c"
    return match.group(1)


@pytest.mark.parametrize("platform", ["linux", "windows", "android", "web"])
def test_generated_menu_flags_reach_the_actual_label_and_reset_to_local(platform, monkeypatch, tmp_path):
    for number in (192, None):
        if number is None:
            monkeypatch.delenv("HALO_BUILD_NUMBER", raising=False)
        else:
            monkeypatch.setenv("HALO_BUILD_NUMBER", str(number))
        flags = menu_compile_flags(platform, monkeypatch, tmp_path)
        metadata = re.findall(r"-DHALO_BUILD_NUMBER=\S+", flags)
        assert metadata == [f"-DHALO_BUILD_NUMBER={number or 0}"], flags
        expected = f"gulch build {number}" if number else "gulch build local"
        run_handler(tmp_path / ("numbered" if number else "local"), metadata, expected)
