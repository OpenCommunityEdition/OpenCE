"""Compile the real MCC catalog with narrow host filesystem stubs.

Generated maps contain only a header; the catalog must not read tag payloads.
The stubs require every discovery/open operation to name d:\\mcc_maps\\, so
an accidental fallback to Xbox or CE directories fails the executable.
"""
import os
import re
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
typedef unsigned char byte;
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
#pragma once
#ifndef _WIN32
#define __stdcall
#endif
#define TRUE 1
#define FALSE 0
#define WAIT_OBJECT_0 0
#define INFINITE (~0u)
#define CreateEventA(a,b,c,d) ((void *)0)
#define CreateThread(a,b,c,d,e,f) ((void)(c),(void *)0)
#define WaitForSingleObject(h,t) ((void)(h),(void)(t),0)
#define SetEvent(h) ((void)(h))
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
#include "mcc_package.h"
#include "mcc_cache_format.h"
#include "mcc_errors.h"
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#undef fopen
#undef fread
static char const *root;
static char const *listing="list.txt";
static unsigned inventories;
static int admission_reads;
static int identities_prepared;
static unsigned reads, largest, scans, closed;
void mcc_cache_precache_clear(void) {}
void mcc_menu_preview_clear(void) {}
int config_boolean(char const *name) {
 if(!strcmp(name,"game.mcc_campaigns"))return getenv("TEST_MISSIONS")!=NULL;
 char const *disabled=getenv("TEST_COLLECTIONS_DISABLED");(void)name;return !disabled;
}
#include "../src/mcc_manifest.h"
#include "../src/mcc_menu_metadata.h"
int mcc_manifest_package_root(char const *level,char path[1024]) {
 (void)level;path[0]=0;return getenv("TEST_MENU_METADATA")!=NULL;
}
int mcc_menu_metadata_load(char const *folder,int campaign,struct mcc_menu_metadata *metadata) {
 (void)folder;(void)campaign;memset(metadata,0,sizeof(*metadata));
 strcpy(metadata->title,"Offline package title");metadata->count=1;
 strcpy(metadata->maps[0].name,"arena");strcpy(metadata->maps[0].title,"Authored offline level");return 1;
}
struct steam_workshop_map_text const *mcc_menu_metadata_find(struct mcc_menu_metadata const *metadata,char const *name) {
 return metadata->count && !strcmp(name,metadata->maps[0].name) ? &metadata->maps[0] : NULL;
}
int mcc_manifest_for_level(char const *level,struct mcc_manifest *manifest,char reason[128]) {
 char buffer[512],*name;int count=0;(void)level;reason[0]=0;
 memset(manifest,0,sizeof(*manifest));
 if(!getenv("TEST_MISSIONS"))return 0;
 snprintf(buffer,sizeof(buffer),"%s",getenv("TEST_MISSIONS"));
 for(name=strtok(buffer,",");name&&count<64;name=strtok(NULL,",")) {
  snprintf(manifest->missions[count].name,64,"%s",name);
  snprintf(manifest->missions[count].title,64,"Explicit mission %s",name);count++;
 }
 strcpy(manifest->title,"Example campaign");manifest->count=count;return 1;
}
#include "../src/steam_workshop.h"
static int workshop_items;
static struct steam_workshop_map workshop_list[4];
#define workshop_item workshop_list[0]
int steam_workshop_scan(void) {return workshop_items;}
int steam_workshop_count(void) {return workshop_items;}
struct steam_workshop_map const *steam_workshop_map(int index) {return index>=0 && index<workshop_items ? &workshop_list[index] : NULL;}
int steam_workshop_item_title(char const *id,char *title,unsigned long size) {
    int i;
    for(i=0;i<workshop_items;i++) if(!strcmp(id,workshop_list[i].item_id)) {
        snprintf(title,size,"%s",workshop_list[i].item_title);return 1;
    }
    return 0;
}
/* the CATALOG_ITEMS environment: "id:item title:map name:map title;..." */
static void workshop_from_environment(void) {
    char *text=getenv("CATALOG_ITEMS"),*field[4];
    static char copy[512];
    if(!text)return;
    snprintf(copy,sizeof(copy),"%s",text);text=copy;
    while(*text && workshop_items<4) {
        char *end=strchr(text,';');int f;
        if(end)*end=0;
        field[0]=text;
        for(f=1;f<4;f++){text=strchr(text,':');if(!text)exit(50);*text++=0;field[f]=text;}
        strcpy(workshop_list[workshop_items].item_id,field[0]);
        strcpy(workshop_list[workshop_items].item_title,field[1]);
        strcpy(workshop_list[workshop_items].name,field[2]);
        strcpy(workshop_list[workshop_items].title,field[3]);
        workshop_items++;
        if(!end)break;
        text=end+1;
    }
}
char const *mcc_identity_local_name(char const *name) {return name;}
int mcc_identity_check(char const *name) {(void)name;return 1;}
int mcc_identity_prepare_host(char const *name) {(void)name;identities_prepared++;return 1;}
#include "mcc_runtime.h"
#include "mcc_resources.h"
#include "mcc_precache.h"
#include "mcc_validation.h"
int mcc_precache_snapshot(char const *name,struct mcc_precache_snapshot *s) {(void)name;memset(s,0,sizeof(*s));return 1;}
static char saved_identity[64];
static int allocation_fails;
void *mcc_runtime_allocate(struct mcc_runtime *r,uint32_t bytes) {
    (void)r;
    return bytes<=sizeof(saved_identity) && !allocation_fails ? saved_identity : NULL;
}
FILE *mcc_test_fopen(char const *path,char const *mode) {
    char translated[1024];
    if (strncmp(path,"d:\\mcc_maps\\",12) || strcmp(mode,"rb") ||
        strpbrk(path+12,":*?\"<>|")) exit(20);
    snprintf(translated,sizeof(translated),"%s/mcc_maps/%s",root,path+12);
    {char *p;for(p=translated;*p;p++)if(*p=='\\')*p='/';}
    return fopen(translated,mode);
}
size_t mcc_test_fread(void *out,size_t size,size_t count,FILE *file) {
    size_t bytes=size*count;
    if (bytes>2048 && !admission_reads) exit(21);
    if (bytes>largest) largest=(unsigned)bytes;
    reads++;
    return fread(out,size,count,file);
}
void error(short priority,char const *format,...) {(void)priority;(void)format;}
HANDLE FindFirstFileA(char const *pattern,WIN32_FIND_DATAA *data) {
    char path[1024];FILE *inventory;size_t n;char *p;
    if (strncmp(pattern,"d:\\mcc_maps\\",12)) exit(23);
    snprintf(path,sizeof(path),"%s/mcc_maps/%s",root,pattern+12);
    for(p=path;*p;p++)if(*p=='\\')*p='/';
    n=strlen(path);if(n<3 || strcmp(path+n-3,"*.*"))exit(23);
    strcpy(path+n-3,listing);
    inventory=fopen(path,"r");scans++;
    if (!inventory)return INVALID_HANDLE_VALUE;
    if (FindNextFileA(inventory,data)){inventories++;return inventory;}
    fclose(inventory);return INVALID_HANDLE_VALUE;
}
int FindNextFileA(HANDLE handle,WIN32_FIND_DATAA *data) {
    FILE *inventory=(FILE *)handle;
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
    fclose((FILE *)handle);inventories--;closed++;return TRUE;
}

int main(int argc,char **argv) {
    short count,i;
    if(argc<2)return 2;
    if(argc>=4 && !strcmp(argv[2],"browse")) {
        struct mcc_map_browser b={0};int step;
        root=argv[1];workshop_from_environment();mcc_map_browser_open(&b,atoi(argv[3]));
        for(step=4;step<=argc;step++) {
            int row;wchar_t text[64];
            printf("view%d=%s|%d|%d\n",step-4,b.package,b.count,b.chosen);
            for(row=0;row<b.count;row++) {
                short index=mcc_map_browser_index(&b,(short)row);
                mcc_map_browser_text(&b,(short)row,text);
                printf("row%d_%d=%ls|%s\n",step-4,row,text,index==NONE ? "folder" : mcc_maps_level_name(index));
            }
            if(mcc_map_browser_index(&b,-1)!=NONE || mcc_map_browser_index(&b,b.count)!=NONE)return 40;
            if(mcc_map_browser_enter(&b,-1) || mcc_map_browser_enter(&b,b.count))return 41;
            if(step==argc)break;
            if(!strncmp(argv[step],"enter:",6))mcc_map_browser_enter(&b,(short)atoi(argv[step]+6));
            else if(!strncmp(argv[step],"select:",7))mcc_map_browser_select(&b,(short)atoi(argv[step]+7));
            else if(!strcmp(argv[step],"back"))mcc_map_browser_back(&b);
            else if(!strcmp(argv[step],"rescan")) {
                listing="list2.txt";mcc_maps_rescan();mcc_map_browser_open(&b,b.campaign);
            } else if(!strcmp(argv[step],"reopen"))mcc_map_browser_open(&b,b.campaign);
            else return 42;
        }
        return 0;
    }
    if(argc==5 && !strcmp(argv[2],"admit")) {
        char message[256]={0};unsigned long checksum;
        int status,steps=0;
        root=argv[1];admission_reads=1;
        do {status=mcc_validation_request(argv[3],NULL,NULL);if(!status)mcc_validation_update();}
        while(!status && ++steps<10000);
        checksum=mcc_cache_checksum(argv[3]);
        if(checksum && identities_prepared!=1)return 43;
        printf("checksum=%lu\n",checksum);
        printf("accepted=%d\n",mcc_cache_present(argv[3],strtoul(argv[4],NULL,0),message,sizeof(message)));
        printf("message=%s\n",message);return 0;
    }
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
    root=argv[1];
    if(argc>2 && !strcmp(argv[2],"workshop")) {
        /* one subscribed item: its maps folder is root/mcc_maps/2899609719, as the platform's path translation has it */
        workshop_items=1;
        strcpy(workshop_item.item_id,"2899609719");strcpy(workshop_item.item_title,"Pro Edition");
        strcpy(workshop_item.name,"pat_beavercreek");strcpy(workshop_item.title,"Better Creek");
        argc=2;
    }
    count=mcc_maps_count(FALSE);
    if(workshop_items) {
        struct mcc_map_browser browser;wchar_t text[64];
        memset(&browser,0,sizeof(browser));
        mcc_map_browser_open(&browser,FALSE);
        mcc_map_browser_text(&browser,0,text);
        {wchar_t const *d=mcc_maps_description(0);
         printf("folder=%ls\ndescription=",text);
         while(*d && *d!='\r')putchar((int)*d++);
         printf("\n");}
    }
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
        printf("label_%d=%ls\n",i,mcc_maps_name(i));
        if(getenv("TEST_MISSIONS")) {
            char next[64];int state=mcc_maps_next_mission(level,next);
            printf("next_%d=%d|%s\n",i,state,next);
        }
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
    if(inventories)return 33;
    return 0;
}
'''
    # Exercise the actual side-effect-free runtime admission functions too.
    cache_source = (ROOT / "port/linux/game/mcc_cache.c").read_text()
    admission = re.search(r"static struct \{.*?\} mcc_validation;", cache_source, re.S).group() + "\n" + "\n".join(function(cache_source, name) for name in
                          ("mcc_cache_validation_clear", "mcc_cache_probe", "mcc_path", "mcc_scenario_identity", "mcc_file_read", "mcc_open_explain", "mcc_cache_checksum", "mcc_cache_present"))
    admission += "\n" + (ROOT / "port/linux/game/mcc_validation.inl").read_text()
    source.write_text(source_code.replace("int main(int argc,char **argv)", "#define fopen mcc_test_fopen\n#define fread mcc_test_fread\n" + admission + "\n#undef fopen\n#undef fread\nint main(int argc,char **argv)"))
    output = work / ("catalog.exe" if sys.platform == "win32" else "catalog")
    command = [compiler, "-include", str(ROOT / "port/linux/game/mcc_loading.h"), str(ROOT / "tools/mcc_loading_stub.c"), "-std=c99", "-Wall", "-Wextra", "-Werror", "-DZ_PREFIX",
               "-I", str(work), "-I", str(ROOT / "port/linux/game"), str(source),
               str(ROOT / "port/linux/game/mcc_maps.c"),
               str(ROOT / "port/linux/game/mcc_workshop.c"),
               str(ROOT / "port/linux/game/mcc_package.c"), str(ROOT / "port/linux/src/workshop_collections.c"),
               str(ROOT / "port/linux/game/mcc_cache_format.c"), str(ROOT / "port/linux/game/mcc_resources.c"), str(ROOT / "port/linux/game/mcc_errors.c"), str(ROOT / "port/linux/game/mcc_sha256.c"), str(ROOT / "port/third_party/zlib/crc32.c"), "-o", str(output)]
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


def run(tool, root, *args, items=None):
    environment = dict(os.environ)
    if items:
        environment["CATALOG_ITEMS"] = ";".join(":".join(item) for item in items)
    result = subprocess.run([str(tool), str(root), *args], capture_output=True, text=True, timeout=10, cwd=root,
                            env=environment)
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


def test_package_catalog_duplicates_and_resource_files(catalog_tool, tmp_path):
    install(tmp_path, {"a10.map": header(), "solo.map": header()})
    root = tmp_path / "mcc_maps"
    (root / "list.txt").write_text("a10.map\nsolo.map\n/one\n/two\n/..\n")
    for package, scenario in (("one", 0), ("two", 1)):
        folder = root / package
        folder.mkdir()
        (folder / "a10.map").write_bytes(header(scenario=scenario))
        # Even a resource file that resembles a level header is not a level.
        (folder / "sounds.map").write_bytes(header())
        (folder / "list.txt").write_text("a10.map\nsounds.map\n/nested\n../solo.map\n")
        (folder / (package + ".map")).write_bytes(header(scenario=scenario))
        with (folder / "list.txt").open("a") as listing:
            listing.write(package + ".map\n")
    fields, maps = run(catalog_tool, tmp_path, "mcc_maps/two/a10")
    assert maps == ["mcc_maps\\a10|1", "mcc_maps\\one\\a10|1", "mcc_maps\\one\\one|1", "mcc_maps\\solo|1", "mcc_maps\\two\\a10|0", "mcc_maps\\two\\two|0"]
    assert fields["found"] == "4"
    assert fields["label_0"] == "A10"
    assert fields["label_1"] == "A10 [one]"
    assert fields["label_2"] == "One [one]"
    assert fields["label_3"] == "Solo"
    assert fields["label_4"] == "A10 [two]"
    assert fields["label_5"] == "Two [two]"
    assert fields["closed"] == fields["scans"] == "3"


@pytest.mark.parametrize("stem", ["pkg/level", "pkg\\level", "p/" + "a" * 52])
def test_package_saved_identity(catalog_tool, tmp_path, stem):
    fields, _ = run(catalog_tool, tmp_path, "identity", "mcc_maps/" + stem)
    assert fields["accepted"] == "1"
    assert fields["identity"] == "mcc_maps\\" + stem.replace("/", "\\")


@pytest.mark.parametrize("stem", ["p/q/level", "p//level", "./level", "p/../level", "p /level", "p./level", "/level", "p/", "p/" + "a" * 53])
def test_package_invalid_paths(catalog_tool, tmp_path, stem):
    fields, _ = run(catalog_tool, tmp_path, "path", "mcc_maps/" + stem)
    assert fields["accepted"] == "0"


def test_network_admission_checks_package_resources(catalog_tool, tmp_path):
    folder = tmp_path / "mcc_maps/pkg"
    folder.mkdir(parents=True)
    from test_mcc_cache_format import resource_dependency_map
    level = resource_dependency_map()
    struct.pack_into("<I", level, 0x64, 1234)
    (folder / "a10.map").write_bytes(level)
    from test_mcc_resources import fixture_blob
    blob = fixture_blob()
    # Match the generated tag name and original offset in the dependency map.
    blob[24:40] = b"res__0__0\0" + bytes(6)
    struct.pack_into("<I", level, 0xB00+0x850+0x48, 16)
    (folder / "a10.map").write_bytes(level)
    (folder / "sounds.map").write_bytes(blob)
    host, _ = run(catalog_tool, tmp_path, "admit", "mcc_maps/pkg/a10", "0")
    peer, _ = run(catalog_tool, tmp_path, "admit", "mcc_maps/pkg/a10", host["checksum"])
    assert peer["accepted"] == "1"
    blob[16] ^= 1
    (folder / "sounds.map").write_bytes(blob)
    peer, _ = run(catalog_tool, tmp_path, "admit", "mcc_maps/pkg/a10", host["checksum"])
    assert peer["accepted"] == "0"
    assert "package resources differ" in peer["message"]
    missing, _ = run(catalog_tool, tmp_path, "admit", "mcc_maps/other/a10", host["checksum"])
    assert missing["accepted"] == "0"


def test_shared_resource_directory_is_not_a_package(catalog_tool, tmp_path):
    install(tmp_path, {"level.map": header()})
    root = tmp_path / "mcc_maps"
    (root / "list.txt").write_text("level.map\n/_ShArEd\n")
    shared = root / "_ShArEd"
    shared.mkdir()
    (shared / "level.map").write_bytes(header())
    (shared / "list.txt").write_text("level.map\n/singleplayer\n/multiplayer\n")
    fields, _ = run(catalog_tool, tmp_path)
    assert fields["count"] == "1"
    assert fields["scans"] == "1"

def browser_install(root):
    install(root, {"solo.map": header(), "arena.map": header(scenario=1)})
    base = root / "mcc_maps"
    (base / "list.txt").write_text("solo.map\narena.map\n/zeta\n/Alpha\n/empty\n/_shared\n")
    for package in ("zeta", "Alpha", "empty", "_shared"):
        folder = base / package
        folder.mkdir()
        entries = {"a10.map": header(), "b30.map": header(), "arena.map": header(scenario=1)}
        if package == "empty":
            entries = {"sounds.map": header()}
        for name, contents in entries.items():
            (folder / name).write_bytes(contents)
        (folder / "list.txt").write_text("\n".join(entries))
    return base


@pytest.mark.parametrize("campaign", [0, 1])
def test_browser_groups_packages_and_keeps_loose_maps(catalog_tool, tmp_path, campaign):
    browser_install(tmp_path)
    fields, _ = run(catalog_tool, tmp_path, "browse", str(campaign), "enter:0", "back")
    assert fields["view0"] == "|3|0"
    assert fields["row0_0"] == "[Alpha]|folder"
    assert fields["row0_1"] == "[zeta]|folder"
    assert fields["row0_2"] == ("Solo|mcc_maps\\solo" if campaign else "Arena|mcc_maps\\arena")
    assert fields["view1"] == ("Alpha|2|0" if campaign else "Alpha|1|0")
    assert fields["row1_0"] == ("A10|mcc_maps\\Alpha\\a10" if campaign else "Arena|mcc_maps\\Alpha\\arena")
    assert fields["view2"] == "|3|0"


def test_browser_remembers_selection_and_returns_to_owning_folder(catalog_tool, tmp_path):
    browser_install(tmp_path)
    fields, _ = run(catalog_tool, tmp_path, "browse", "1", "enter:1", "select:1", "reopen", "back")
    assert fields["view3"] == "zeta|2|1"
    assert fields["row3_1"] == "B30|mcc_maps\\zeta\\b30"
    assert fields["view4"] == "|3|1"


def test_browser_rescan_restores_identity_after_indices_change(catalog_tool, tmp_path):
    base = browser_install(tmp_path)
    (base / "list2.txt").write_text("/Alpha\n/zeta\n")
    for package in ("Alpha", "zeta"):
        (base / package / "list2.txt").write_text("b30.map\n")
    fields, _ = run(catalog_tool, tmp_path, "browse", "1", "enter:1", "select:1", "rescan")
    assert fields["view3"] == "zeta|1|0"
    assert fields["row3_0"] == "B30|mcc_maps\\zeta\\b30"


def test_browser_removed_folder_returns_to_root(catalog_tool, tmp_path):
    base = browser_install(tmp_path)
    (base / "list2.txt").write_text("solo.map\n")
    fields, _ = run(catalog_tool, tmp_path, "browse", "1", "enter:1", "rescan")
    assert fields["view2"] == "|1|0"
    assert fields["row2_0"] == "Solo|mcc_maps\\solo"


def test_browser_empty_category_is_not_a_launchable_row(catalog_tool, tmp_path):
    install(tmp_path, {"solo.map": header()})
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:0", "back")
    assert fields["view0"] == fields["view1"] == fields["view2"] == "|0|0"


def test_browser_full_catalog_and_long_labels(catalog_tool, tmp_path):
    install(tmp_path, {})
    base = tmp_path / "mcc_maps"
    package = "p" * 48
    folder = base / package
    folder.mkdir()
    entries = [f"{n:04}.map" for n in range(1024)]
    for name in entries:
        (folder / name).write_bytes(header())
    (folder / "list.txt").write_text("\n".join(entries))
    (base / "list.txt").write_text("/" + package)
    fields, _ = run(catalog_tool, tmp_path, "browse", "1", "enter:0", "select:1023", "reopen", "back")
    assert fields["row0_0"] == f"[{package}]|folder"
    assert fields["view3"] == f"{package}|1024|1023"
    assert fields["view4"] == "|1|0"


def test_workshop_item_maps_are_listed_by_title(catalog_tool, tmp_path):
    """A subscribed Workshop item's map is the level mcc_maps\\<item id>\\<name>, shown by its Workshop title."""
    install(tmp_path, {"a10.map": header()})
    item = tmp_path / "mcc_maps" / "2899609719"
    item.mkdir()
    (item / "pat_beavercreek.map").write_bytes(header(scenario=1))
    fields, maps = run(catalog_tool, tmp_path, "workshop")
    assert maps == ["mcc_maps\\2899609719\\pat_beavercreek|0", "mcc_maps\\a10|1"]
    # the title, not the file name and not a "[package]" suffix
    assert fields["label_0"] == "Better Creek"
    # the package folder is named by the item's title, and the description says where it is from
    assert fields["folder"] == "[Pro Edition]"
    assert fields["description"].startswith("Steam Workshop")
    # the same data root without the item: only the ordinary map
    fields, maps = run(catalog_tool, tmp_path)
    assert maps == ["mcc_maps\\a10|1"]


COLLECTION_ITEMS = [
    # in PRO EDITION
    ("2899666869", "Item One", "alpha_map_file", "Alpha Arena"),
    # in SOCIAL MAP PACK and 2V2 MAPS
    ("2899747345", "Item Two", "beta_map_file", "Beta Arena"),
    # in no collection
    ("12345", "Lone Item", "gamma_map_file", "Gamma Arena"),
]


def collection_install(tmp_path):
    install(tmp_path, {"solo.map": header(scenario=0), "arena.map": header(scenario=1)})
    base = tmp_path / "mcc_maps"
    for item, _, name, _ in COLLECTION_ITEMS:
        (base / item).mkdir()
        (base / item / f"{name}.map").write_bytes(header(scenario=1))


def test_workshop_collections_are_folders_of_their_items_maps(catalog_tool, tmp_path):
    """An item in a collection is listed in that collection's folder; an item in none keeps its own."""
    collection_install(tmp_path)
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:0", "back", items=COLLECTION_ITEMS)
    # the folders: collections in the table's order, then an uncollected item's, then the loose map
    assert fields["row0_0"] == "[PRO EDITION]|folder"
    assert fields["row0_1"] == "[SOCIAL MAP PACK]|folder"
    assert fields["row0_2"] == "[2V2 MAPS]|folder"
    assert fields["row0_3"] == "[Lone Item]|folder"
    assert fields["row0_4"].endswith("|mcc_maps\\arena")
    assert fields["view0"] == "|5|0"
    # inside a collection, the maps of its items by their Workshop titles
    assert fields["view1"] == "PRO EDITION|1|0"
    assert fields["row1_0"] == "Alpha Arena|mcc_maps\\2899666869\\alpha_map_file"
    # back lands on the collection's own row
    assert fields["view2"] == "|5|0"


def test_an_item_in_two_collections_is_in_both(catalog_tool, tmp_path):
    collection_install(tmp_path)
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:1", "back", "enter:2", items=COLLECTION_ITEMS)
    assert fields["row1_0"] == "Beta Arena|mcc_maps\\2899747345\\beta_map_file"
    assert fields["view2"] == "|5|1"
    assert fields["row3_0"] == "Beta Arena|mcc_maps\\2899747345\\beta_map_file"


def test_an_item_in_no_collection_keeps_its_own_folder(catalog_tool, tmp_path):
    collection_install(tmp_path)
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:3", "back", items=COLLECTION_ITEMS)
    assert fields["view1"] == "12345|1|0"
    assert fields["row1_0"] == "Gamma Arena|mcc_maps\\12345\\gamma_map_file"
    assert fields["view2"] == "|5|3"


def test_a_collection_folder_shows_only_items_the_player_has(catalog_tool, tmp_path):
    collection_install(tmp_path)
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", items=COLLECTION_ITEMS[2:])
    # nothing subscribed from any collection: no collection folder
    assert fields["view0"] == "|2|0"
    assert fields["row0_0"] == "[Lone Item]|folder"


def local_packages(root, names):
    install(root, {})
    base = root / "mcc_maps"
    for name in names:
        folder = base / name
        folder.mkdir()
        (folder / "arena.map").write_bytes(header(scenario=1))
        (folder / "list.txt").write_text("arena.map\n")
    (base / "list.txt").write_text("\n".join("/" + name for name in names))
    return base


def test_collection_rows_do_not_exceed_browser_storage(catalog_tool, tmp_path):
    # One map in two groups plus 1023 independent package folders: 1025 rows.
    local_packages(tmp_path, ["2899747345"] + [f"package{i:04}" for i in range(1023)])
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:1024", "back")
    assert fields["view0"] == "|1025|0"
    assert fields["view1"] == "package1022|1|0"
    assert fields["row1_0"] == "Arena|mcc_maps\\package1022\\arena"
    assert fields["view2"] == "|1025|1024"


def test_collection_and_package_names_have_distinct_selection_keys(catalog_tool, tmp_path):
    local_packages(tmp_path, ["2899666869", "PRO EDITION"])
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:1", "back", "enter:0", "back")
    assert fields["view0"] == "|2|0"
    assert fields["row1_0"] == "Arena|mcc_maps\\PRO EDITION\\arena"
    assert fields["view2"] == "|2|1"
    assert fields["row3_0"] == "Arena|mcc_maps\\2899666869\\arena"
    assert fields["view4"] == "|2|0"


def test_collection_identity_uses_sidecar_without_steam(catalog_tool, tmp_path):
    base = local_packages(tmp_path, ["MyPack"])
    (base / "MyPack/workshop_id.txt").write_text("2899666869\nPro Edition\n")
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:0")
    assert fields["row0_0"] == "[PRO EDITION]|folder"
    assert fields["row1_0"] == "Arena|mcc_maps\\MyPack\\arena"


def test_disabled_collections_keep_package_folders(catalog_tool, tmp_path, monkeypatch):
    local_packages(tmp_path, ["2899747345"])
    monkeypatch.setenv("TEST_COLLECTIONS_DISABLED", "1")
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:0")
    assert fields["view0"] == "|1|0"
    assert fields["row0_0"] == "[MCC package]|folder"
    assert fields["row1_0"] == "Arena|mcc_maps\\2899747345\\arena"


def test_campaign_browser_orders_explicit_missions_and_keeps_full_titles(catalog_tool, tmp_path, monkeypatch):
    base = local_packages(tmp_path, ["Campaign"])
    for name in ("z10", "a10", "extra"):
        (base / f"Campaign/{name}.map").write_bytes(header(scenario=0))
    (base / "Campaign/list.txt").write_text("z10.map\na10.map\nextra.map\n")
    monkeypatch.setenv("TEST_MISSIONS", "z10,a10")
    fields, maps = run(catalog_tool, tmp_path)
    assert [m.split("|")[0] for m in maps] == ["mcc_maps\\Campaign\\z10", "mcc_maps\\Campaign\\a10", "mcc_maps\\Campaign\\extra"]
    assert fields["next_0"] == "1|mcc_maps\\Campaign\\a10"
    assert fields["next_1"] == "2|" and fields["next_2"] == "0|"
    fields, _ = run(catalog_tool, tmp_path, "browse", "1", "enter:0", "back")
    assert fields["row0_0"] == "[Example campaign]|folder"
    assert fields["row1_0"] == "Explicit mission z10|mcc_maps\\Campaign\\z10"
    assert fields["view2"] == "|1|0"


def test_missing_campaign_mission_is_not_skipped(catalog_tool, tmp_path, monkeypatch):
    base = local_packages(tmp_path, ["Campaign"])
    for name in ("z10", "a10"):
        (base / f"Campaign/{name}.map").write_bytes(header(scenario=0))
    (base / "Campaign/list.txt").write_text("z10.map\na10.map\n")
    monkeypatch.setenv("TEST_MISSIONS", "z10,missing,a10")
    fields, _ = run(catalog_tool, tmp_path)
    assert fields["next_0"] == "-1|mcc_maps\\Campaign\\missing"
    assert fields["next_1"] == "2|"


@pytest.mark.parametrize("candidates", [1, 2])
def test_downloaded_campaign_does_not_splice_other_snapshots(catalog_tool, tmp_path, monkeypatch, candidates):
    # The real catalog receives identical ordered metadata for every package.
    # Even one same-Workshop candidate cannot prove the intended binary version:
    # campaign JSON names missions but does not contain their gameplay digests.
    # This is a catalog boundary test, not receipt validation or network testing.
    first = "dl_" + "1" * 20
    folders = [first] + ["dl_" + str(i + 2) * 20 for i in range(candidates)]
    install(tmp_path, {})
    root = tmp_path / "mcc_maps"
    (root / "list.txt").write_text("\n".join("/" + name for name in folders))
    for index, folder in enumerate(folders):
        package = root / folder
        package.mkdir()
        mission = "z10" if index == 0 else "a10"
        (package / "list.txt").write_text(mission + ".map\n")
        (package / (mission + ".map")).write_bytes(header(scenario=0, internal=f"revision_{index}"))
        (package / "workshop_id.txt").write_text("12345\nExample campaign\n")
    monkeypatch.setenv("TEST_MISSIONS", "z10,a10")
    fields, maps = run(catalog_tool, tmp_path)
    assert len(maps) == candidates + 1
    assert maps[0] == f"mcc_maps\\{first}\\z10|1"
    assert fields["next_0"] == f"-1|mcc_maps\\{first}\\a10"
    # The following mission remains available as an individual selection.
    assert all(name.endswith("\\a10|1") for name in maps[1:])
    assert all(fields[f"next_{i}"] == "2|" for i in range(1, candidates + 1))


def test_workshop_browser_keeps_title_longer_than_filename(catalog_tool, tmp_path):
    local_packages(tmp_path, ["12345"])
    fields, _ = run(catalog_tool, tmp_path, "browse", "0", "enter:0", items=[("12345", "Item", "arena", "A much longer mission title")])
    assert fields["row1_0"] == "A much longer mission title|mcc_maps\\12345\\arena"


def test_imported_package_titles_do_not_depend_on_steam(catalog_tool,tmp_path,monkeypatch):
    install(tmp_path,{})
    root=tmp_path/"mcc_maps"
    (root/"list.txt").write_text("/12345\n")
    (root/"12345").mkdir()
    (root/"12345/list.txt").write_text("arena.map\n")
    (root/"12345/arena.map").write_bytes(header(scenario=1))
    fields,_=run(catalog_tool,tmp_path,"browse","0","enter:0")
    assert fields["row0_0"].startswith("[MCC package]|")
    assert fields["row1_0"]=="Arena|mcc_maps\\12345\\arena"
    monkeypatch.setenv("TEST_MENU_METADATA","1")
    fields,_=run(catalog_tool,tmp_path,"browse","0","enter:0")
    assert fields["row0_0"].startswith("[Offline package title]|")
    assert fields["row1_0"]=="Authored offline level|mcc_maps\\12345\\arena"
