"""Compile the real MCC catalog with narrow host filesystem stubs.

Generated maps contain only a header; the catalog must not read tag payloads.
The stubs require every discovery/open operation to name d:\\mcc_maps\\, so
an accidental fallback to Xbox or CE directories fails the executable.
"""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def header(version=13, scenario=0, internal="different_internal_name"):
    data = bytearray(0x828)
    for offset, value in {0: 0x68656164, 4: version, 8: len(data),
                          16: 0x800, 20: 40, 0x7FC: 0x666F6F74}.items():
        struct.pack_into("<I", data, offset, value)
    struct.pack_into("<H", data, 0x60, scenario)
    data[32:32 + len(internal)] = internal.encode("ascii")
    data[64:69] = b"test\0"
    return data


@pytest.fixture(scope="session")
def catalog_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed for MCC catalog tests")
    work = tmp_path_factory.mktemp("mcc-catalog")
    (work / "cseries.h").write_text(r'''
#ifndef MCC_TEST_CSERIES_H
#define MCC_TEST_CSERIES_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
typedef unsigned char boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define FLAG(x) (1u << (x))
#ifdef _WIN32
#define csstrcasecmp _stricmp
#else
#include <strings.h>
#define csstrcasecmp strcasecmp
#define _strnicmp strncasecmp
#endif
FILE *mcc_test_fopen(char const *,char const *);
size_t mcc_test_fread(void *,size_t,size_t,FILE *);
#define fopen mcc_test_fopen
#define fread mcc_test_fread
#endif
''')
    (work / "errors.h").write_text('#define _error_silent 0\nvoid error(short,const char *,...);\n')
    (work / "xtl.h").write_text(r'''
typedef void *HANDLE;
typedef struct {unsigned long dwFileAttributes;char cFileName[256];} WIN32_FIND_DATAA;
#define FILE_ATTRIBUTE_DIRECTORY 16
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
HANDLE FindFirstFileA(char const *,WIN32_FIND_DATAA *);
int FindNextFileA(HANDLE,WIN32_FIND_DATAA *);
int CloseHandle(HANDLE);
''')
    source = work / "catalog.c"
    source_code = r'''
#include "cseries.h"
#include "xtl.h"
#include "mcc_maps.h"
#include <stdarg.h>
#include <stdint.h>
#undef fopen
#undef fread
static char const *root;
static char const *listing="list.txt";
static FILE *inventory;
static unsigned reads, largest, scans, closed;
struct mcc_runtime {
    unsigned char *tag_index;
    struct {uint32_t scenario_handle,tag_count;} report;
};
static char saved_identity[64];
static int allocation_fails;
void *mcc_runtime_allocate(struct mcc_runtime *r,uint32_t bytes) {
    (void)r;
    return bytes<=sizeof(saved_identity) && !allocation_fails ? saved_identity : NULL;
}
FILE *mcc_test_fopen(char const *path,char const *mode) {
    char translated[1024];
    if (strncmp(path,"d:\\mcc_maps\\",12) || strcmp(mode,"rb") ||
        strpbrk(path+12,"\\/:*?\"<>|")) exit(20);
    snprintf(translated,sizeof(translated),"%s/mcc_maps/%s",root,path+12);
    return fopen(translated,mode);
}
size_t mcc_test_fread(void *out,size_t size,size_t count,FILE *file) {
    size_t bytes=size*count;
    if (bytes>2048) exit(21);
    if (bytes>largest) largest=(unsigned)bytes;
    reads++;
    return fread(out,size,count,file);
}
void error(short priority,char const *format,...) {(void)priority;(void)format;}
HANDLE FindFirstFileA(char const *pattern,WIN32_FIND_DATAA *data) {
    char path[1024];
    if (strcmp(pattern,"d:\\mcc_maps\\*.*") || inventory) exit(23);
    snprintf(path,sizeof(path),"%s/mcc_maps/%s",root,listing);
    inventory=fopen(path,"r");scans++;
    if (!inventory)return INVALID_HANDLE_VALUE;
    if (FindNextFileA(inventory,data))return inventory;
    fclose(inventory);inventory=NULL;return INVALID_HANDLE_VALUE;
}
int FindNextFileA(HANDLE handle,WIN32_FIND_DATAA *data) {
    if (!inventory || handle!=inventory)exit(24);
    if (!fgets(data->cFileName,sizeof(data->cFileName),inventory))return FALSE;
    data->cFileName[strcspn(data->cFileName,"\r\n")]=0;
    data->dwFileAttributes=0;
    if (data->cFileName[0]=='/') {
        data->dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY;
        memmove(data->cFileName,data->cFileName+1,strlen(data->cFileName));
    }
    return TRUE;
}
int CloseHandle(HANDLE handle) {
    if (!inventory || handle!=inventory)exit(24);
    fclose(inventory);inventory=NULL;closed++;return TRUE;
}
int main(int argc,char **argv) {
    short count,i;
    if(argc<2)return 2;
    if(argc==4 && (!strcmp(argv[2],"identity") || !strcmp(argv[2],"identity_fail"))) {
        unsigned char entries[64],before[64];
        uint32_t address=0;int accepted;
        struct mcc_runtime r;
        memset(entries,0xAB,sizeof(entries));memcpy(before,entries,sizeof(entries));
        r.tag_index=entries;r.report.scenario_handle=0xE1750001;r.report.tag_count=2;
        allocation_fails=!strcmp(argv[2],"identity_fail");
        accepted=mcc_scenario_identity(&r,argv[3]);
        printf("accepted=%d\n",accepted);
        if(accepted) {
            memcpy(&address,entries+48,4);
            if(address!=(uint32_t)(uintptr_t)saved_identity)return 28;
            memcpy(before+48,entries+48,4);
            printf("identity=%s\n",saved_identity);
        }
        if(memcmp(entries,before,sizeof(entries)))return 29;
        return 0;
    }
    if(argc==4 && !strcmp(argv[2],"path")) {
        char path[128];int accepted=mcc_path(argv[3],path);
        printf("accepted=%d\n",accepted);
        if(accepted)printf("path=%s\n",path);
        return 0;
    }
    root=argv[1];count=mcc_maps_count(FALSE);
    printf("count=%d\nmultiplayer=%d\nsingleplayer=%d\n",count,mcc_maps_count(TRUE),mcc_maps_type_count(TRUE));
    if(mcc_maps_type_count(FALSE)!=mcc_maps_count(TRUE) ||
       mcc_maps_type_count(TRUE)+mcc_maps_type_count(FALSE)!=count)return 30;
    {
        int campaign;
        for(campaign=0;campaign<=1;campaign++) {
            short total=mcc_maps_type_count((boolean)campaign),previous=NONE;
            if(mcc_maps_type_index(-1,(boolean)campaign)!=NONE ||
               mcc_maps_type_index(total,(boolean)campaign)!=NONE)return 31;
            for(i=0;i<total;i++) {
                short index=mcc_maps_type_index(i,(boolean)campaign);
                if(index<=previous || index>=count || mcc_maps_campaign(index)!=campaign)return 32;
                if(argc==3 && !strcmp(argv[2],"types"))
                    printf("%s_%d=%s\n",campaign ? "singleplayer" : "multiplayer",i,mcc_maps_level_name(index));
                previous=index;
            }
        }
    }
    for(i=0;i<count;i++) {
        char const *level=mcc_maps_level_name(i);
        short found=mcc_maps_find(level),row=mcc_maps_index(i,FALSE);
        if(found!=i || row!=i || !level || strlen(level)>63)return 25;
        printf("map=%s|%d\n",level,mcc_maps_campaign(i));
    }
    for(i=0;i<mcc_maps_count(TRUE);i++) {
        short index=mcc_maps_index(i,TRUE);
        if(index<0 || mcc_maps_campaign(index))return 26;
    }
    if(mcc_maps_index(-1,FALSE)!=NONE || mcc_maps_index(count,FALSE)!=NONE ||
       mcc_maps_find("a10")!=NONE || mcc_maps_find("custom_maps\\a10")!=NONE ||
       mcc_maps_find("maps\\a10")!=NONE || mcc_maps_find(NULL)!=NONE)return 27;
    if(argc>2 && !strcmp(argv[2],"rescan")) {
        listing="list2.txt";
        printf("cached=%d\n",mcc_maps_count(FALSE));
        mcc_maps_rescan();printf("rescanned=%d\n",mcc_maps_count(FALSE));
    } else if(argc>2) printf("found=%d\n",mcc_maps_find(argv[2]));
    printf("scans=%u\nreads=%u\nlargest_read=%u\n",scans,reads,largest);
    printf("closed=%u\n",closed);
    if(inventory)return 33;
    return 0;
}
'''
    # Exercise the actual side-effect-free runtime admission functions too.
    cache_source = (ROOT / "port/linux/game/mcc_cache.c").read_text()
    admission = "\n".join(function(cache_source, name) for name in
                          ("mcc_prefix", "mcc_path", "mcc_scenario_identity"))
    source.write_text(source_code.replace("int main(int argc,char **argv)", admission + "\nint main(int argc,char **argv)"))
    output = work / ("catalog.exe" if sys.platform == "win32" else "catalog")
    command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
               "-I", str(work), "-I", str(ROOT / "port/linux/game"), str(source),
               str(ROOT / "port/linux/game/mcc_maps.c"),
               str(ROOT / "port/linux/game/mcc_cache_format.c"), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


def install(root, entries):
    directory = root / "mcc_maps"
    directory.mkdir(exist_ok=True)
    for name, contents in entries.items():
        (directory / name).write_bytes(contents)
    (directory / "list.txt").write_text("\n".join(entries))


def run(tool, root, *args):
    result = subprocess.run([str(tool), str(root), *args], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
    fields = dict(line.split("=", 1) for line in result.stdout.splitlines() if not line.startswith("map="))
    maps = [line[4:] for line in result.stdout.splitlines() if line.startswith("map=")]
    return fields, maps


def test_versions_and_folders_are_isolated(catalog_tool, tmp_path):
    install(tmp_path, {"a10.map": header(), "bloodgulch.map": header(scenario=1),
                       "xbox.map": header(version=5), "custom.map": header(version=609),
                       "ui.map": header(scenario=2), "truncated.map": header()[:300],
                       "ignored.yelo": header()})
    for folder in ("maps", "custom_maps"):
        (tmp_path / folder).mkdir()
        for name in ("a10", "bloodgulch", "outside_only"):
            (tmp_path / folder / (name + ".map")).write_bytes(header())
    fields, maps = run(catalog_tool, tmp_path)
    assert fields["count"] == "2"
    assert fields["multiplayer"] == "1"
    assert maps == ["mcc_maps\\a10|1", "mcc_maps\\bloodgulch|0"]
    assert fields["largest_read"] == "2048"
    assert fields["scans"] == "1"


@pytest.mark.parametrize("entries,singleplayer,multiplayer", [
    ({}, [], []),
    ({"only_sp": 0}, ["only_sp"], []),
    ({"only_mp": 1}, [], ["only_mp"]),
    ({"a10": 1, "bloodgulch": 0}, ["bloodgulch"], ["a10"]),
    ({"a_mp": 1, "b_sp": 0, "c_mp": 1, "d_sp": 0}, ["b_sp", "d_sp"], ["a_mp", "c_mp"]),
])
def test_scenario_type_lists_keep_catalog_identity(catalog_tool, tmp_path, entries, singleplayer, multiplayer):
    install(tmp_path, {name + ".map": header(scenario=kind) for name, kind in entries.items()})
    fields, _ = run(catalog_tool, tmp_path, "types")
    for label, expected in [("singleplayer", singleplayer), ("multiplayer", multiplayer)]:
        assert int(fields[label]) == len(expected)
        assert [fields[f"{label}_{row}"] for row in range(len(expected))] == [
            "mcc_maps\\" + name for name in expected]


def test_filename_identity_case_and_duplicate_filter(catalog_tool, tmp_path):
    install(tmp_path, {"UPPER.map": header(internal="a10"), "lower.map": header()})
    (tmp_path / "mcc_maps/list.txt").write_text("UPPER.MAP\nUPPER.map\nlower.map\n")
    fields, maps = run(catalog_tool, tmp_path, "MCC_MAPS\\upper")
    assert fields["count"] == "2"
    assert fields["found"] == "1"
    assert maps == ["mcc_maps\\lower|1", "mcc_maps\\UPPER|1"]


def test_listing_skips_directories_and_closes_its_handle(catalog_tool, tmp_path):
    install(tmp_path, {"folder.map": header(), "level.map": header()})
    (tmp_path / "mcc_maps/list.txt").write_text("/folder.map\nlevel.map\nno_extension\n")
    fields, maps = run(catalog_tool, tmp_path)
    assert maps == ["mcc_maps\\level|1"]
    assert fields["closed"] == "1"


def test_missing_directory_has_no_handle_to_close(catalog_tool, tmp_path):
    fields, maps = run(catalog_tool, tmp_path)
    assert maps == []
    assert fields["scans"] == "1"
    assert fields["closed"] == "0"


def test_filename_length_limit(catalog_tool, tmp_path):
    install(tmp_path, {"a" * 54 + ".map": header(), "b" * 55 + ".map": header()})
    fields, maps = run(catalog_tool, tmp_path)
    assert fields["count"] == "1"
    assert maps[0] == "mcc_maps\\" + "a" * 54 + "|1"


def test_rescan_rebuilds_only_mcc_catalog(catalog_tool, tmp_path):
    install(tmp_path, {"first.map": header(), "second.map": header(scenario=1)})
    (tmp_path / "mcc_maps/list.txt").write_text("first.map\n")
    (tmp_path / "mcc_maps/list2.txt").write_text("second.map\nfirst.map\n")
    fields, _ = run(catalog_tool, tmp_path, "rescan")
    assert fields["count"] == fields["cached"] == "1"
    assert fields["rescanned"] == "2"
    assert fields["scans"] == "2"


def test_catalog_limit(catalog_tool, tmp_path):
    install(tmp_path, {f"map{i:04}.map": header(scenario=i % 2) for i in range(1025)})
    fields, maps = run(catalog_tool, tmp_path)
    assert fields["count"] == "1024"
    assert fields["multiplayer"] == "512"
    assert len(maps) == 1024


def test_empty_or_missing_mcc_directory(catalog_tool, tmp_path):
    fields, maps = run(catalog_tool, tmp_path)
    assert fields["count"] == fields["multiplayer"] == "0"
    assert maps == []


def test_unsafe_or_unlaunchable_names_are_not_opened(catalog_tool, tmp_path):
    install(tmp_path, {"valid.map": header()})
    unsafe = ["../a10.map", "..\\a10.map", "bad?.map", "bad*.map", "bad|.map",
              'bad".map', "bad<.map", "bad>.map", "d:a10.map", ".map"]
    (tmp_path / "mcc_maps/list.txt").write_text("\n".join(unsafe + ["valid.map"]))
    fields, maps = run(catalog_tool, tmp_path)
    assert fields["count"] == "1"
    assert maps == ["mcc_maps\\valid|1"]


@pytest.mark.parametrize("name,accepted", [
    ("mcc_maps\\a10", True), ("MCC_MAPS/bloodgulch", True),
    ("mcc_maps\\" + "a" * 54, True), ("mcc_maps\\" + "a" * 55, False),
    ("a10", False), ("custom_maps\\a10", False), ("maps\\a10", False),
    ("mcc_maps\\..\\a10", False), ("mcc_maps/../a10", False),
    ("mcc_maps\\", False), ("mcc_maps\\a10.", False),
    ("mcc_maps\\a10 ", False), ("mcc_maps\\a?0", False),
])
def test_runtime_path_admission(catalog_tool, tmp_path, name, accepted):
    fields, _ = run(catalog_tool, tmp_path, "path", name)
    assert fields["accepted"] == str(int(accepted))
    if accepted:
        assert fields["path"] == "d:\\mcc_maps\\" + name[9:] + ".map"


@pytest.mark.parametrize("name,expected", [
    ("mcc_maps\\a10", "mcc_maps\\a10"),
    ("MCC_MAPS/BloodGulch", "mcc_maps\\BloodGulch"),
    ("mcc_maps/" + "a" * 54, "mcc_maps\\" + "a" * 54),
    ("custom_maps\\a10", None),
])
def test_saved_identity_keeps_mcc_namespace(catalog_tool, tmp_path, name, expected):
    fields, _ = run(catalog_tool, tmp_path, "identity", name)
    assert fields["accepted"] == str(int(expected is not None))
    if expected:
        assert fields["identity"] == expected


def test_saved_identity_allocation_failure_does_not_modify_tags(catalog_tool, tmp_path):
    fields, _ = run(catalog_tool, tmp_path, "identity_fail", "mcc_maps\\a10")
    assert fields["accepted"] == "0"
