"""MCC readiness polling must not repeat resource validation or cache admission."""
from pathlib import Path
import shutil
import subprocess
import sys
import pytest

ROOT = Path(__file__).resolve().parents[1]


def test_readiness_reuses_only_unchanged_successful_inputs(tmp_path):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    (tmp_path / "cseries.h").write_text("typedef int boolean;\ntypedef unsigned char byte;\n#define TRUE 1\n#define FALSE 0\n")
    (tmp_path / "xtl.h").write_text(r'''
#include <stdint.h>
typedef uint32_t DWORD;
typedef struct {DWORD dwLowDateTime,dwHighDateTime;} FILETIME;
typedef struct {
 DWORD dwFileAttributes;FILETIME ftCreationTime,ftLastAccessTime,ftLastWriteTime;
 DWORD nFileSizeHigh,nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA;
#define GetFileExInfoStandard 0
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_PATH_NOT_FOUND 3
int GetFileAttributesExA(char const *,int,void *);
DWORD GetLastError(void);
''')
    source = tmp_path / "test.c"
    source.write_text(r'''
#include "cseries.h"
#include "xtl.h"
#include "mcc_cache.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static char paths[9][128];
static WIN32_FILE_ATTRIBUTE_DATA files[9];
static int count,probes,valid=1,mutate,denied=-1,present[9];
static DWORD last_error;
char const *mcc_identity_local_name(char const *name) {
 return name && !strcmp(name,"mcc_maps/host/a10") ? "mcc_maps/package/a10" : name;
}
DWORD GetLastError(void) {return last_error;}
int GetFileAttributesExA(char const *path,int level,void *out) {
 int i;(void)level;
 if(strstr(path,"custom_maps") || strstr(path,"d:\\maps\\"))exit(90);
 for(i=0;i<count;i++)if(!strcmp(paths[i],path))break;
 if(i==count){if(count==9)exit(91);strcpy(paths[count++],path);}
 if(i==denied){last_error=5;return 0;}
 if(!present[i]){last_error=ERROR_FILE_NOT_FOUND;return 0;}
 memcpy(out,&files[i],sizeof(files[i]));return 1;
}
boolean mcc_cache_require(char const *name,unsigned long checksum) {
 (void)name;if(checksum)exit(92);probes++;
 if(mutate==1)files[0].ftLastWriteTime.dwLowDateTime++;
 if(mutate==2)denied=3;
 return valid;
}
int main(void) {
 int i,old;char const *name="mcc_maps/package/a10";
 present[0]=1;files[0].nFileSizeLow=200;
 CHECK(mcc_cache_precache_ready(name));CHECK(probes==1 && count==9);
 for(i=0;i<1000;i++)CHECK(mcc_cache_precache_ready(name));
 CHECK(probes==1);
 CHECK(mcc_cache_precache_ready("mcc_maps\\package\\a10"));CHECK(probes==1);
 CHECK(mcc_cache_precache_ready("mcc_maps/host/a10"));CHECK(probes==1);
 /* Each map/resource candidate, including a previously absent override,
  * invalidates on add, removal, same-size write, replacement or resize. */
 for(i=0;i<9;i++) {
  old=probes;present[i]=1;files[i].nFileSizeLow++;
  CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+1);
  files[i].ftLastAccessTime.dwLowDateTime++;
  CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+1);
  files[i].ftLastWriteTime.dwLowDateTime++;
  CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+2);
  files[i].ftLastWriteTime.dwHighDateTime++;
  CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+3);
  files[i].ftCreationTime.dwHighDateTime++;
  CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+4);
  present[i]=0;
  CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+5);
 }
 /* Denied metadata queries never become an unchanged missing-file stamp. */
 denied=3;old=probes;
 CHECK(mcc_cache_precache_ready(name));CHECK(mcc_cache_precache_ready(name));
 CHECK(probes==old+2);denied=-1;
 mcc_cache_precache_clear();valid=0;old=probes;
 CHECK(!mcc_cache_precache_ready(name));CHECK(!mcc_cache_precache_ready(name));
 CHECK(probes==old+2);valid=1;
 CHECK(mcc_cache_precache_ready(name));
 mcc_cache_precache_clear();old=probes;
 CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+1);
 /* An in-flight installation cannot leave a cached ready result. */
 present[0]=1;mutate=1;
 CHECK(!mcc_cache_precache_ready(name));old=probes;mutate=0;
 CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+1);
 mcc_cache_precache_clear();mutate=2;
 CHECK(!mcc_cache_precache_ready(name));mutate=0;denied=-1;old=probes;
 CHECK(mcc_cache_precache_ready(name));CHECK(probes==old+1);
 CHECK(!mcc_cache_precache_ready("custom_maps/a10"));
 CHECK(!mcc_cache_precache_ready("mcc_maps/../a10"));
 CHECK(!mcc_cache_precache_ready(NULL));
 /* A different package must not inherit the previous package's success. */
 count=0;old=probes;
 CHECK(mcc_cache_precache_ready("mcc_maps/other/a10"));CHECK(probes==old+1);
 return 0;
}
''')
    binary = tmp_path / ("test.exe" if sys.platform == "win32" else "test")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if sys.platform == "win32" else []
    result = subprocess.run([compiler, *flags, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
                             "-I", str(tmp_path), "-I", str(ROOT / "port/linux/game"),
                             str(source), str(ROOT / "port/linux/game/mcc_precache.c"),
                             str(ROOT / "port/linux/game/mcc_package.c"), "-o", str(binary)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr


def test_cached_polling_does_not_replace_admission_or_load_validation():
    from harness import function
    native = (ROOT / "source/cache/cache_files.c").read_text()
    cache = (ROOT / "port/linux/game/mcc_cache.c").read_text()
    assert "mcc_cache_precache_ready(map_name)" in function(native, "cache_files_give_time_to_precache")
    assert "mcc_cache_require(map_name, version)" in function(native, "cache_files_map_present")
    assert "mcc_resources_probe" in function(cache, "mcc_cache_present")
    assert "mcc_resources_checksum" in function(cache, "mcc_cache_tags_load")
    assert "mcc_cache_precache_clear" in function(cache, "mcc_cache_tags_unload")
    assert "mcc_cache_precache_clear" in function((ROOT / "port/linux/game/mcc_maps.c").read_text(), "mcc_maps_rescan")
