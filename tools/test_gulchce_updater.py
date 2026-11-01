"""Exercise the actual desktop release checker without downloading/installing files."""
from pathlib import Path
import os
import re
import shutil
import subprocess

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def test_release_feed_and_build_comparison(tmp_path):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is required")
    source = (ROOT / "port/linux/src/updater.c").read_text()
    repository = re.search(r'^#define UPDATE_REPOSITORY .*$', source, re.M).group()
    harness = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define HALO_BUILD_NUMBER 60
#define MAXIMUM_CHECK_SIZE (1024ULL * 1024)
#define SDLCALL
#define SDL_free free
enum { _updater_idle, _updater_checking, _updater_available, _updater_handled };
static int updater_state, download_ok = 1, deleted;
static long updater_latest_build, updater_latest_network_version, updater_release_network_version;
static const char *response;
static void updater_path(char *path, size_t size, const char *name) {
    snprintf(path, size, "%s", name);
}
static int update_download(const char *url, const char *path, unsigned long long maximum, void *a, void *b,
    char *error, size_t size) {
    assert(!strcmp(url, "https://api.github.com/repos/chestahh/GulchCE/releases/latest"));
    assert(maximum == MAXIMUM_CHECK_SIZE);
    return download_ok;
}
static char *SDL_LoadFile(const char *path, size_t *size) {
    char *copy = malloc(strlen(response) + 1);
    strcpy(copy, response); *size = strlen(response); return copy;
}
static void update_delete_file(const char *path) { deleted++; }
static void platform_log(const char *format, ...) {}
static void SDL_SetAtomicInt(int *state, int value) { *state = value; }
'''
    harness += repository + "\n" + function(source, "updater_latest_release")
    harness += "\n" + function(source, "updater_check_thread")
    harness += r'''
int main(void) {
    response = "{\"tag_name\":\"build-61\"}";
    updater_check_thread(NULL);
    assert(updater_state == _updater_available && updater_latest_build == 61);
    assert(deleted == 1 && updater_latest_network_version == 0);
    response = "{\"tag_name\":\"build-62\",\"body\":\"Network version: 41\"}";
    updater_check_thread(NULL);
    assert(updater_latest_build == 62 && updater_latest_network_version == 41);
    response = "{\"tag_name\":\"build-60\"}";
    updater_check_thread(NULL); assert(updater_state == _updater_handled);
    response = "{\"tag_name\":\"build-59\"}";
    updater_check_thread(NULL); assert(updater_state == _updater_handled);
    response = "{\"message\":\"Not Found\"}";
    updater_check_thread(NULL); assert(updater_state == _updater_handled);
    download_ok = 0;
    updater_check_thread(NULL); assert(updater_state == _updater_handled);
    return 0;
}
'''
    test = tmp_path / "updater.c"
    binary = tmp_path / "release_check.exe"
    test.write_text(harness)
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    subprocess.run([compiler, *flags, str(test), "-o", str(binary)], check=True, capture_output=True)
    subprocess.run([str(binary)], check=True, capture_output=True)


def test_android_and_downloads_use_the_same_repository():
    desktop = (ROOT / "port/linux/src/updater.c").read_text()
    android = (ROOT / "port/android/app/src/main/java/com/halo/decomp/Updater.java").read_text()
    assert 'REPOSITORY = "chestahh/GulchCE"' in android
    assert '"https://github.com/" UPDATE_REPOSITORY "/releases/download/build-' in desktop
    assert '"https://github.com/" + REPOSITORY + "/releases/download/build-' in android
    assert "OpenCommunityEdition/OpenCE" not in desktop + android
