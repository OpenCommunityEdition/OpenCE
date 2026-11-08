"""The real MCC browser adapter with a deterministic page/OPFS boundary.

These tests exercise C ABI, quota and visibility decisions; actual OPFS durability
and cross-tab locks are checked separately in browser tests.
"""
from pathlib import Path
import json
import subprocess

import pytest
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def bridge_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-web-storage")
    (work / "emscripten").mkdir()
    (work / "emscripten/threading.h").write_text(
        "static int emscripten_futex_wait(void *p,int v,double t){(void)p;(void)v;(void)t;return 0;}\n")
    (work / "emscripten.h").write_text("static double emscripten_get_now(void){return 0;}\n")
    (work / "pthread.h").write_text(
        "#ifndef _WIN32\n#include_next <pthread.h>\n#else\n"
        "typedef int pthread_mutex_t;\n#define PTHREAD_MUTEX_INITIALIZER 0\n"
        "static int pthread_mutex_lock(pthread_mutex_t *p){(void)p;return 0;}\n"
        "static int pthread_mutex_unlock(pthread_mutex_t *p){(void)p;return 0;}\n#endif\n")
    (work / "unistd.h").write_text(
        "#ifdef _WIN32\n#define lstat stat\n#define fileno _fileno\n#endif\n"
        "static int fsync(int fd){return fd<0 ? -1 : 0;}\n")
    source = work / "test.c"
    source.write_text(r'''
#include "mcc_share_stage.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void web_mcc_share_io(struct mcc_share_io *);
uint64_t web_mcc_free_bytes(void);
int web_mcc_package_visible(char const *);
int web_mcc_directory_visible(char const *);
int web_mcc_remove_begin(char const *);
void web_mcc_remove_end(char const *);
static int calls,response=1,translated;
static uint64_t quota;
static char last_action[32],last_payload[34000];
void platform_translate_path(char const *path,char *out,unsigned long size) {
 unsigned i;
 translated++;
 if(strncmp(path,"d:\\",3)){fprintf(stderr,"native path translated twice\n");exit(5);}
 snprintf(out,size,"/data/%s",path+3);
 for(i=0;out[i];i++)if(out[i]=='\\')out[i]='/';
}
void web_js_mcc_storage(char const *action,char const *payload,int *box,int generation) {
 calls++;snprintf(last_action,sizeof(last_action),"%s",action);snprintf(last_payload,sizeof(last_payload),"%s",payload);
 if(box[0]!=generation)exit(6);
 box[2]=(int)(uint32_t)quota;box[3]=(int)(uint32_t)(quota>>32);box[1]=response;
}
void web_js_mcc_storage_fence(void){exit(7);}
int main(int argc,char **argv) {
 static char const level[]="mcc_maps\\dl_0123456789abcdef0123\\m0";
 static char const directory[]="d:\\mcc_maps\\dl_0123456789abcdef0123";
 struct mcc_share_io io;
 struct mcc_share_file file;
 if(argc!=2)return 2;
 if(!strcmp(argv[1],"quota")) {
  response=0;if(web_mcc_free_bytes()!=0)return 10;
  response=1;if(web_mcc_free_bytes()!=1)return 11;
  quota=UINT64_C(10737418240);if(web_mcc_free_bytes()!=quota)return 12;
 } else if(!strcmp(argv[1],"visibility")) {
  response=0;if(!web_mcc_package_visible("mcc_maps\\manual\\m0")||calls)return 13;
  if(web_mcc_package_visible(level)||calls!=1)return 14;
  if(web_mcc_package_visible("mcc_maps\\DL_0123456789ABCDEF0123\\m0"))return 24;
  if(web_mcc_directory_visible("/data/mcc_maps/dl_0123456789abcdef0123")||translated)return 15;
  response=1;if(!web_mcc_package_visible(level))return 16;
  if(!web_mcc_directory_visible(directory)||translated!=1)return 17;
  if(!web_mcc_directory_visible("/data/mcc_maps/manual"))return 18;
 } else if(!strcmp(argv[1],"begin")) {
  memset(&io,0,sizeof(io));web_mcc_share_io(&io);memset(&file,0,sizeof(file));
  strcpy(file.leaf,"Metadata/Mission.json");file.size=123;
  if(!io.direct || !io.direct->begin(directory,&file,1) || strcmp(last_action,"begin"))return 19;
  puts(last_payload);return 0;
 } else if(!strcmp(argv[1],"removal")) {
  response=0;if(web_mcc_remove_begin("dl_0123456789abcdef0123"))return 20;
  response=1;if(!web_mcc_remove_begin("dl_0123456789abcdef0123"))return 21;
  web_mcc_remove_end("dl_0123456789abcdef0123");if(strcmp(last_action,"removeEnd"))return 22;
  calls=0;if(web_mcc_remove_begin("../escape") || calls)return 23;
 } else return 3;
 puts("ok");return 0;
}
''')
    return compile_tool(work, [source, ROOT / "port/web/src/web_mcc_storage.c"])


@pytest.mark.parametrize("case", ["quota", "visibility", "removal"])
def test_browser_storage_boundary(bridge_tool, case):
    result = subprocess.run([str(bridge_tool), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == "ok"


def test_download_journal_request_preserves_exact_paths(bridge_tool):
    result = subprocess.run([str(bridge_tool), "begin"], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stderr
    assert json.loads(result.stdout) == {
        "folder": "dl_0123456789abcdef0123",
        "files": [{"path": "Metadata/Mission.json", "size": 123}],
        "companions": ["workshop_id.txt", "download.receipt"],
    }


def test_uncommitted_package_is_unavailable_except_exact_validation_redirect(tmp_path):
    source = tmp_path / "test.c"
    source.write_text(r'''
#include "mcc_package.h"
#include <string.h>
static int visible;
int web_mcc_package_visible(char const *name){(void)name;return visible;}
int main(void){
 char path[128];
 if(mcc_package_map_path("mcc_maps\\dl_0123456789abcdef0123\\m0",path))return 1;
 if(mcc_package_resource_path("mcc_maps\\dl_0123456789abcdef0123\\m0",1,path))return 2;
 mcc_package_stage_redirect("mcc_maps\\dl_0123456789abcdef0123\\m0","mcc_maps/dl_0123456789abcdef0123");
 if(!mcc_package_map_path("mcc_maps\\dl_0123456789abcdef0123\\m0",path))return 3;
 if(!mcc_package_resource_path("mcc_maps\\dl_0123456789abcdef0123\\m0",1,path))return 4;
 if(mcc_package_map_path("mcc_maps\\dl_0123456789abcdef0123\\m1",path))return 5;
 mcc_package_stage_redirect(0,0);
 if(mcc_package_map_path("mcc_maps\\dl_0123456789abcdef0123\\m0",path))return 6;
 visible=1;
 if(!mcc_package_map_path("mcc_maps\\dl_0123456789abcdef0123\\m0",path))return 7;
 return 0;
}
''')
    wrapper = tmp_path / "package.c"
    wrapper.write_text('#define HALO_WEB 1\n#include "' +
                       (ROOT / "port/linux/game/mcc_package.c").as_posix() + '"\n')
    tool = compile_tool(tmp_path, [source, wrapper])
    result = subprocess.run([str(tool)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stderr
