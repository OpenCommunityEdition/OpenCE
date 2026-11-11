"""Local receipts own downloads, never original packages or added user files."""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import pytest
from harness import function
from mcc_test_support import compile_tool

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"
SRC = ROOT / "port/linux/src"
FOLDER = "dl_0123456789abcdef0123"

DRIVER = r'''
#include "mcc_share_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(p,m) _mkdir(p)
#define rmdir _rmdir
#define S_ISDIR(m) (((m)&_S_IFMT)==_S_IFDIR)
#else
#include <unistd.h>
#endif
int posix_mcc_share_path_safe(char const *);
int posix_mcc_share_publish(char const *,char const *);
int posix_mcc_share_remove(char const *,int);
static int make(char const *p){return mkdir(p,0700)==0;}
static int erase_directory(char const *p){return rmdir(p)==0;}
static int exists(char const *p){struct stat s;return !stat(p,&s)&&S_ISDIR(s.st_mode);}
static int move(char const *a,char const *b){
 int result=posix_mcc_share_publish(a,b);
 if(!result){
#ifdef _WIN32
  fprintf(stderr,"publish failed: Windows error %lu; %s -> %s\n",(unsigned long)GetLastError(),a,b);
#else
  fprintf(stderr,"publish failed: errno %d; %s -> %s\n",errno,a,b);
#endif
 }
 return result;
}
static int erase(char const *p){return remove(p)==0;}
static int refuse_publish(char const *a,char const *b){(void)a;(void)b;return 0;}
static struct mcc_share_io io={make,erase_directory,exists,move,erase,posix_mcc_share_path_safe,move,mcc_share_open_native,NULL};
static int native_attempts;
static FILE *refuse_native(char const *p){(void)p;native_attempts++;return NULL;}
/* Metadata/gameplay verification is covered by the identity suites. This unit
 * exercises the mandatory successful-verification boundary and actual files. */
static int verified(void *user,char const *path){(void)user;(void)path;return 1;}
static void entry(struct mcc_share_file *file,int kind,char const *leaf,char const *text){
 struct mcc_sha256 hash;
 memset(file,0,sizeof(*file));file->kind=(uint8_t)kind;strcpy(file->leaf,leaf);file->size=(uint32_t)strlen(text);
 mcc_sha256_begin(&hash);mcc_sha256_add(&hash,text,(int)file->size);mcc_sha256_end(&hash,file->sha256);
}
static void show(struct mcc_share_storage_item const *item){
 printf("%s\t%u\t%llu\t%s\t%s\t%s\t%s\n",item->folder,item->files,
  (unsigned long long)item->bytes,item->source,item->maker,item->workshop.id,item->workshop.title);
}
int main(int argc,char **argv){
 static struct mcc_share_stage stage;
 static struct mcc_share_file files[MCC_SHARE_STAGE_FILES];
 struct mcc_workshop_item workshop;
 struct mcc_share_io native_io;
 struct mcc_share_file expected;
 struct mcc_share_storage_item items[MCC_SHARE_STORAGE_LIMIT];
 char root[200],maps[256],path[512];
 char const *folder=argc>3?argv[3]:"dl_0123456789abcdef0123";
 char const *mode=argv[1];
 char const *sidecar="43210\nTransferred maker\n";
 int count=3,i,result,truncated=0;
 FILE *collision;
 if(argc<3)return 2;
 mcc_share_storage_native_io(&native_io);
 if(!strcmp(mode,"native_open")){
  collision=mcc_share_open_native(argv[2]);if(!collision){puts("0");return 0;}
  printf("1 %d\n",fgetc(collision));fclose(collision);return 0;
 }
 if(!strcmp(mode,"native_remove")&&argc==4){printf("%d\n",posix_mcc_share_remove(argv[2],atoi(argv[3])));return 0;}
 snprintf(root,sizeof(root),"%s/",argv[2]);snprintf(maps,sizeof(maps),"%s/mcc_maps",argv[2]);
 if(!strcmp(mode,"native_route")){
  int read_result,list_result,remove_result,reuse_result;
  char candidate[1024];
  native_io.open_native_read=refuse_native;
  read_result=mcc_share_storage_read(maps,&native_io,folder,items);
  list_result=mcc_share_storage_list(maps,&native_io,items,64,&truncated);
  remove_result=mcc_share_storage_remove(maps,&native_io,folder);
  entry(&expected,MCC_SHARE_FILE_BITMAPS,"bitmaps.map","x");
  reuse_result=mcc_share_storage_find_resource(maps,&native_io,&expected,candidate);
  printf("%d %d %d %d %d\n",read_result,list_result,remove_result,reuse_result,native_attempts);return 0;
 }
 if(!strcmp(mode,"list")){
  result=mcc_share_storage_list(maps,&native_io,items,argc>3?atoi(argv[3]):64,&truncated);
  printf("%d %d\n",result,truncated);for(i=0;i<result;i++)show(items+i);return 0;
 }
 if(!strcmp(mode,"read")){
  result=mcc_share_storage_read(maps,&native_io,folder,items);printf("%d\n",result);if(result)show(items);return 0;
 }
 if(!strcmp(mode,"remove")){printf("%d\n",mcc_share_storage_remove(maps,&native_io,folder));return 0;}
 if(!strncmp(mode,"find_",5)){
  char candidate[1024];
  entry(&expected,MCC_SHARE_FILE_BITMAPS,"bitmaps.map","x");
  if(!strcmp(mode,"find_sounds"))entry(&expected,MCC_SHARE_FILE_SOUNDS,"sounds.map","x");
  if(!strcmp(mode,"find_map"))entry(&expected,MCC_SHARE_FILE_MAP,"m0.map","x");
  if(!strcmp(mode,"find_json"))entry(&expected,MCC_SHARE_FILE_JSON,"metadata/m0.json","x");
  if(!strcmp(mode,"find_mismatch"))expected.sha256[0]^=1;
  if(!strcmp(mode,"find_size"))expected.size++;
  if(!strcmp(mode,"find_path"))strcpy(expected.leaf,"../bitmaps.map");
  result=mcc_share_storage_find_resource(maps,&native_io,&expected,candidate);
  printf("%d\n",result);if(result)printf("%s\n",candidate);return 0;
 }
 entry(files,MCC_SHARE_FILE_MAP,"m0.map","x");
 entry(files+1,MCC_SHARE_FILE_JSON,"metadata/m0.json","x");
 entry(files+2,MCC_SHARE_FILE_PNG,"assets/m0.png","x");
 if(!strcmp(mode,"receipt_from_network")){
  entry(files+1,MCC_SHARE_FILE_JSON,MCC_SHARE_STAGE_RECEIPT,"x");
  printf("%d\n",mcc_share_manifest_valid(files,3,"m0"));return 0;
 }
 if(!strcmp(mode,"transferred")||!strcmp(mode,"conflicting_id")){entry(files+3,MCC_SHARE_FILE_SIDECAR,"workshop_id.txt",sidecar);count=4;}
 if(!strcmp(mode,"resources")){
  entry(files+3,MCC_SHARE_FILE_BITMAPS,"bitmaps.map","x");entry(files+4,MCC_SHARE_FILE_SOUNDS,"sounds.map","x");count=5;
 }
 if(!strcmp(mode,"maximum")){
  count=MCC_SHARE_STAGE_FILES;
  for(i=1;i<count;i++){char leaf[64];sprintf(leaf,"metadata/m%d.json",i);entry(files+i,MCC_SHARE_FILE_JSON,leaf,"x");}
 }
 if(!strcmp(mode,"install_fail"))io.publish_directory=refuse_publish;
 result=mcc_share_stage_begin(&stage,&io,root,folder,"m0",files,count);
 if(result!=MCC_SHARE_STAGE_OK){printf("begin %d\n",result);return 0;}
 for(i=0;i<count;i++){
  result=mcc_share_stage_write(&stage,files[i].kind==MCC_SHARE_FILE_SIDECAR?sidecar:"x",files[i].size);
  if(result!=(i==count-1?MCC_SHARE_STAGE_ALL_DONE:MCC_SHARE_STAGE_FILE_DONE))return 3;
 }
 memset(&workshop,0,sizeof(workshop));strcpy(workshop.id,"12345");strcpy(workshop.title,"Advertised title");
 if(!strcmp(mode,"transferred"))strcpy(workshop.id,"43210");
 if(!strcmp(mode,"invalid_id"))strcpy(workshop.id,"https://example.org");
 if(!strcmp(mode,"invalid_title"))strcpy(workshop.title,"unsafe\nline");
 if(!strcmp(mode,"unverified")){
  printf("%d %d\n",mcc_share_storage_record(&stage,"mcc_maps\\host\\m0","Map maker",&workshop),
   mcc_share_stage_companion(&stage,MCC_SHARE_STAGE_RECEIPT,"x",1));mcc_share_stage_cancel(&stage);return 0;
 }
 if(!strcmp(mode,"discard")&&!mcc_share_stage_discard_preview(&stage,2))return 4;
 if(mcc_share_stage_verify(&stage,verified,NULL)!=MCC_SHARE_STAGE_OK)return 5;
 if(!strcmp(mode,"receipt_collision")||!strcmp(mode,"sidecar_collision")){
  snprintf(path,sizeof(path),"%s/%s",stage.stage_dir,!strcmp(mode,"receipt_collision")?MCC_SHARE_STAGE_RECEIPT:"workshop_id.txt");
  collision=fopen(path,"wb");if(!collision)return 6;
  if(fwrite("unowned",1,7,collision)!=7||fclose(collision))return 7;
 }
 if(!strcmp(mode,"bad_companion")){
  printf("%d %d\n",mcc_share_stage_companion(&stage,"../outside","x",1),
   mcc_share_stage_companion(&stage,"readme.txt","x",1));mcc_share_stage_cancel(&stage);return 0;
 }
 result=mcc_share_storage_record(&stage,"mcc_maps\\host\\m0","Map maker",!strcmp(mode,"local")?NULL:&workshop);
 if(!result){mcc_share_stage_cancel(&stage);printf("record 0\n");return 0;}
 if(!strcmp(mode,"cancel")){mcc_share_stage_cancel(&stage);printf("cancelled\n");return 0;}
 if(!strcmp(mode,"tamper_companion")){
  snprintf(path,sizeof(path),"%s/workshop_id.txt",stage.stage_dir);collision=fopen(path,"wb");if(!collision)return 8;
  if(fwrite("x",1,1,collision)!=1||fclose(collision))return 9;
 }
 result=mcc_share_stage_install(&stage);printf("install %d\n",result);return 0;
}
'''


@pytest.fixture(scope="module")
def storage_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-storage")
    source = work / "storage.c"
    (work / "posix.h").write_text(f'#include "{(SRC / "posix.h").as_posix()}"\n')
    workshop = (GAME / "mcc_workshop.c").read_text()
    parser = "\n".join(function(workshop, name) for name in (
        "mcc_workshop_digits", "mcc_workshop_title_copy", "mcc_workshop_parse_sidecar"))
    source.write_text(DRIVER + "\n" + parser)
    filesystem = ROOT / "port/windows/src/win32_files.c" if sys.platform == "win32" else SRC / "posix_files.c"
    sources = [source, GAME / "mcc_share_storage.c", GAME / "mcc_share_stage.c",
               GAME / "mcc_share_rules.c", GAME / "mcc_package.c", GAME / "mcc_sha256.c",
               SRC / "posix_mcc_share.c", filesystem]
    if os.environ.get("MCC_SHARE_STORAGE_SANITIZE"):
        compiler = shutil.which("clang")
        assert compiler, "clang required for storage sanitizer coverage"
        exe = work / "storage"
        result = subprocess.run([compiler, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
            "-I", str(work), "-I", str(GAME), *map(str, sources), "-o", str(exe)],
            capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        return exe
    return compile_tool(work, sources)


def run(tool, mode, root, *args):
    result = subprocess.run([str(tool), mode, str(root), *map(str, args)],
                            capture_output=True, text=True, timeout=20)
    assert result.returncode == 0, result.stderr
    if result.stderr:
        print(result.stderr, file=sys.stderr, end="")
    return result.stdout.strip()


def install(tool, root, mode="install", folder=FOLDER):
    assert run(tool, mode, root, folder) == "install 0"
    return root / "mcc_maps" / folder


def test_receipt_lists_real_bytes_and_attribution(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path)
    fields = run(storage_tool, "list", tmp_path).splitlines()
    assert fields[0] == "1 0"
    row = fields[1].split("\t")
    assert row[:2] == [FOLDER, "5"]
    assert int(row[2]) == sum(p.stat().st_size for p in package.rglob("*") if p.is_file())
    assert row[3:] == ["mcc_maps\\host\\m0", "Map maker", "12345", "Advertised title"]
    assert (package / "workshop_id.txt").read_bytes() == b"12345\nAdvertised title\n"
    assert run(storage_tool, "remove", tmp_path) == "1"
    assert not package.exists()


def test_native_reads_use_callback_in_every_storage_route(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path)
    assert run(storage_tool, "native_route", tmp_path) == "0 0 0 0 4"
    assert (package / "m0.map").read_bytes() == b"x"


def test_native_open_uses_actual_file_and_refuses_nonregular_paths(storage_tool, tmp_path):
    native = tmp_path / "native.map"
    native.write_bytes(b"x")
    assert run(storage_tool, "native_open", native) == "1 120"
    assert run(storage_tool, "native_open", tmp_path) == "0"
    assert run(storage_tool, "native_open", tmp_path / "missing.map") == "0"
    if sys.platform != "win32":
        pipe = tmp_path / "pipe"
        os.mkfifo(pipe)
        assert run(storage_tool, "native_open", pipe) == "0"


def test_native_removal_is_typed_and_never_recursive(storage_tool, tmp_path):
    native = tmp_path / "native.map"
    native.write_bytes(b"x")
    for flag in [-1, 2, 1]:
        assert run(storage_tool, "native_remove", native, flag) == "0"
        assert native.read_bytes() == b"x"
    for flag in [0, 1]:
        assert run(storage_tool, "native_remove", tmp_path, flag) == "0"
        assert native.read_bytes() == b"x"
    assert run(storage_tool, "native_remove", native, 0) == "1"
    assert not native.exists()


@pytest.mark.skipif(sys.platform != "win32", reason="Windows alternate data streams")
def test_native_open_refuses_alternate_data_stream(storage_tool, tmp_path):
    native = tmp_path / "native.map"
    native.write_bytes(b"x")
    stream = Path(str(native) + ":other")
    stream.write_bytes(b"hidden")
    assert run(storage_tool, "native_open", stream) == "0"
    assert run(storage_tool, "native_remove", stream, 0) == "0"
    assert native.read_bytes() == b"x" and stream.read_bytes() == b"hidden"


def test_descriptor_is_closed_when_guest_stream_allocation_fails(tmp_path):
    helper = function((GAME / "mcc_share_storage.c").read_text(), "mcc_share_open_native")
    source = tmp_path / "descriptor.c"
    source.write_text(r'''
#include <stdio.h>
static int opened=42,wrap_ok,closed;
static int posix_mcc_share_open(char const *p){(void)p;return opened;}
static void posix_mcc_share_close(int fd){if(fd==42)closed++;}
static FILE *wrap(int fd,char const *mode){(void)fd;(void)mode;return wrap_ok?(FILE *)1:NULL;}
#define _fdopen wrap
#define fdopen wrap
''' + helper + r'''
int main(void){
 if(mcc_share_open_native("native")||closed!=1)return 1;
 wrap_ok=1;if(mcc_share_open_native("native")!=(FILE *)1||closed!=1)return 2;
 opened=-1;if(mcc_share_open_native("missing")||closed!=1)return 3;
 return 0;
}
''')
    exe = compile_tool(tmp_path, [source])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


def test_local_package_needs_no_workshop_sidecar(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path, "local")
    assert not (package / "workshop_id.txt").exists()
    assert run(storage_tool, "list", tmp_path).splitlines()[0] == "1 0"
    assert run(storage_tool, "remove", tmp_path) == "1"


def test_transferred_sidecar_is_never_replaced(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path, "transferred")
    assert (package / "workshop_id.txt").read_bytes() == b"43210\nTransferred maker\n"
    assert run(storage_tool, "list", tmp_path).splitlines()[1].split("\t")[-2:] == ["43210", "Transferred maker"]
    assert run(storage_tool, "remove", tmp_path) == "1"


def test_discarded_preview_is_not_receipted(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path, "discard")
    assert not (package / "assets").exists()
    assert run(storage_tool, "read", tmp_path).splitlines()[1].split("\t")[1] == "4"
    assert run(storage_tool, "remove", tmp_path) == "1"


def test_maximum_network_manifest_has_room_for_local_companions(storage_tool, tmp_path):
    install(storage_tool, tmp_path, "maximum")
    assert run(storage_tool, "read", tmp_path).splitlines()[1].split("\t")[1] == "170"
    assert run(storage_tool, "remove", tmp_path) == "1"


@pytest.mark.parametrize("mode,expected", [("unverified", "0 0"), ("bad_companion", "0 0"),
    ("invalid_id", "record 0"), ("invalid_title", "record 0"), ("conflicting_id", "record 0"), ("cancel", "cancelled"),
    ("install_fail", "install 10"), ("tamper_companion", "install 10")])
def test_failed_install_cleans_generated_files(storage_tool, tmp_path, mode, expected):
    assert run(storage_tool, mode, tmp_path) == expected
    assert not (tmp_path / "mcc_maps" / FOLDER).exists()
    assert not (tmp_path / "mcc_staging" / FOLDER).exists()


@pytest.mark.parametrize("mode,leaf", [("receipt_collision", "download.receipt"), ("sidecar_collision", "workshop_id.txt")])
def test_companions_are_exclusive_and_unowned_collisions_survive(storage_tool, tmp_path, mode, leaf):
    assert run(storage_tool, mode, tmp_path) == "record 0"
    stage = tmp_path / "mcc_staging" / FOLDER
    assert {p.name: p.read_bytes() for p in stage.iterdir()} == {leaf: b"unowned"}


def test_receipt_cannot_arrive_from_network(storage_tool, tmp_path):
    assert run(storage_tool, "receipt_from_network", tmp_path) == "0"


@pytest.mark.parametrize("leaf,contents", [("m0.map", b"y"), ("metadata/m0.json", b"y"),
                                         ("workshop_id.txt", b"different"), ("m0.map", b"longer")])
def test_changed_file_prevents_all_removal(storage_tool, tmp_path, leaf, contents):
    package = install(storage_tool, tmp_path)
    (package / leaf).write_bytes(contents)
    before = {p.relative_to(package): p.read_bytes() for p in package.rglob("*") if p.is_file()}
    assert run(storage_tool, "remove", tmp_path) == "0"
    assert {p.relative_to(package): p.read_bytes() for p in package.rglob("*") if p.is_file()} == before


def test_missing_recorded_file_prevents_all_removal(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path)
    (package / "metadata/m0.json").unlink()
    assert run(storage_tool, "remove", tmp_path) == "0"
    assert (package / "m0.map").read_bytes() == b"x"


def test_added_files_and_saves_are_preserved(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path)
    (package / "savegame.bin").write_bytes(b"save")
    (package / "metadata/personal.json").write_bytes(b"my addition")
    assert run(storage_tool, "remove", tmp_path) == "2"
    assert {p.relative_to(package).as_posix(): p.read_bytes() for p in package.rglob("*") if p.is_file()} == {
        "savegame.bin": b"save", "metadata/personal.json": b"my addition"}
    assert run(storage_tool, "list", tmp_path) == "0 0"


@pytest.mark.parametrize("change", ["truncated", "oversized", "count", "kind", "duplicate", "sidecar_duplicate",
    "traversal", "absolute", "device", "self", "save", "nonascii", "unterminated", "source", "workshop"])
def test_untrusted_or_corrupt_receipt_cannot_authorize_deletion(storage_tool, tmp_path, change):
    package = install(storage_tool, tmp_path)
    path = package / "download.receipt"
    data = bytearray(path.read_bytes())
    if change == "truncated":
        data = data[:-1]
    elif change == "oversized":
        data.extend(b"x" * 32768)
    elif change == "count":
        struct.pack_into("<I", data, 224, 0xffffffff)
    elif change == "kind":
        data[228] = 255
    elif change == "duplicate":
        data[228 + 165:228 + 330] = data[228:228 + 165]
    elif change == "sidecar_duplicate":
        data[228 + 165:228 + 330] = data[228 + 495:228 + 660]
    elif change == "unterminated":
        data[228 + 165 + 37:228 + 330] = b"x" * 128
    elif change == "source":
        data[32] = ord("x")
    elif change == "workshop":
        data[160] = ord(":")
    else:
        leaf = {"traversal": b"../../outside.json", "absolute": b"/outside.json", "device": b"CON .json",
                "self": b"download.receipt", "save": b"savegame.bin", "nonascii": b"caf\xc3\xa9.json"}[change]
        offset = 228 + 165 + 37
        data[offset:offset + 128] = leaf + bytes(128 - len(leaf))
    path.write_bytes(data)
    assert run(storage_tool, "read", tmp_path) == "0"
    assert run(storage_tool, "remove", tmp_path) == "0"
    assert (package / "m0.map").read_bytes() == b"x"


def test_receipt_is_tied_to_managed_folder(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path)
    copied = tmp_path / "mcc_maps" / ("dl_" + "f" * 20)
    shutil.copytree(package, copied)
    assert run(storage_tool, "read", tmp_path, copied.name) == "0"
    original = tmp_path / "mcc_maps/original"
    shutil.copytree(package, original)
    assert run(storage_tool, "remove", tmp_path, "original") == "0"
    assert (original / "m0.map").exists()


def test_list_is_bounded_and_does_not_hide_truncation(storage_tool, tmp_path):
    install(storage_tool, tmp_path)
    install(storage_tool, tmp_path, folder="dl_" + "f" * 20)
    assert run(storage_tool, "list", tmp_path, 1).splitlines()[0] == "1 1"
    assert run(storage_tool, "list", tmp_path, 64).splitlines()[0] == "2 0"


def test_receipt_symlink_cannot_authorize_removal(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path)
    receipt = package / "download.receipt"
    outside = tmp_path / "outside.receipt"
    receipt.rename(outside)
    try:
        receipt.symlink_to(outside)
    except OSError:
        pytest.skip("File symlink creation unavailable")
    try:
        assert run(storage_tool, "list", tmp_path) == "0 0"
        assert run(storage_tool, "remove", tmp_path) == "0"
        assert run(storage_tool, "native_open", receipt) == "0"
        assert outside.is_file() and (package / "m0.map").exists()
    finally:
        receipt.unlink()


def test_resource_reuse_requires_matching_managed_receipt(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path, "resources")
    for mode, name in [("find_bitmaps", "bitmaps.map"), ("find_sounds", "sounds.map")]:
        lines = run(storage_tool, mode, tmp_path).splitlines()
        assert lines[0] == "1" and Path(lines[1]) == package / name
        assert Path(lines[1]).read_bytes() == b"x"
    for mode in ["find_mismatch", "find_size", "find_map", "find_json", "find_path"]:
        assert run(storage_tool, mode, tmp_path) == "0"
    (package / "download.receipt").unlink()
    assert run(storage_tool, "find_bitmaps", tmp_path) == "0"


def test_resource_reuse_checks_actual_size_and_regular_file(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path, "resources")
    resource = package / "bitmaps.map"
    resource.write_bytes(b"longer")
    assert run(storage_tool, "find_bitmaps", tmp_path) == "0"
    resource.unlink()
    resource.mkdir()
    assert run(storage_tool, "find_bitmaps", tmp_path) == "0"


def test_resource_reuse_does_not_follow_receipted_package_link(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path, "resources")
    outside = tmp_path / "outside"
    package.rename(outside)
    if sys.platform == "win32":
        result = subprocess.run(["cmd", "/c", "mklink", "/J", str(package), str(outside)], capture_output=True, text=True)
        if result.returncode:
            pytest.skip("Directory junction creation unavailable: " + result.stderr.strip())
    else:
        package.symlink_to(outside, target_is_directory=True)
    try:
        assert run(storage_tool, "find_bitmaps", tmp_path) == "0"
        assert run(storage_tool, "native_open", package / "bitmaps.map") == "0"
        assert run(storage_tool, "read", tmp_path) == "0"
        assert run(storage_tool, "remove", tmp_path) == "0"
        assert (outside / "bitmaps.map").read_bytes() == b"x"
    finally:
        if sys.platform == "win32":
            os.rmdir(package)
        else:
            package.unlink()


def test_linked_metadata_parent_never_deletes_outside_files(storage_tool, tmp_path):
    package = install(storage_tool, tmp_path)
    outside = tmp_path / "outside"
    (package / "metadata").rename(outside)
    link = package / "metadata"
    if sys.platform == "win32":
        result = subprocess.run(["cmd", "/c", "mklink", "/J", str(link), str(outside)], capture_output=True, text=True)
        if result.returncode:
            pytest.skip("Directory junction creation unavailable: " + result.stderr.strip())
    else:
        link.symlink_to(outside, target_is_directory=True)
    try:
        assert run(storage_tool, "remove", tmp_path) == "0"
        assert (outside / "m0.json").read_bytes() == b"x"
        assert (package / "m0.map").read_bytes() == b"x"
    finally:
        if sys.platform == "win32":
            os.rmdir(link)
        else:
            link.unlink()
