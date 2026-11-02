"""MCC menu admission reuses only successful, unchanged package checksums."""
from pathlib import Path
import re
import subprocess
import pytest
from harness import function
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def test_menu_validation_cache(tmp_path):
    source = (ROOT / "port/linux/game/mcc_cache.c").read_text()
    state = re.search(r"static struct \{.*?\} mcc_validation;", source, re.S)
    declarations = state.group()
    names = ["mcc_cache_validation_clear", "mcc_cache_probe"]
    helpers = "\n".join(function(source, name) for name in names)
    (tmp_path / "cseries.h").write_text("typedef int boolean;\ntypedef unsigned char byte;\n#define TRUE 1\n#define FALSE 0\n")
    test = r'''
#include "cseries.h"
#include "mcc_precache.h"
#include "mcc_cache_format.h"
#include "mcc_errors.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
static int probes, valid=1, stamped=1, changing, prepares, identity_ok=1;
static struct mcc_precache_snapshot files;
static int mcc_validation_lookup(char const *name,uint32_t *checksum,unsigned char *digest) {
    (void)name;(void)checksum;(void)digest;return 0;
}
int mcc_precache_snapshot(char const *name, struct mcc_precache_snapshot *out) {
    (void)name; *out=files; return stamped;
}
static char const *mcc_identity_local_name(char const *name) {return name;}
static int mcc_identity_prepare_host(char const *name) {(void)name; prepares++;return identity_ok;}
static int mcc_identity_check(char const *name) {(void)name;return identity_ok;}
static FILE *mcc_open(char const *name, struct mcc_cache_source *s, struct mcc_cache_identity *i) {
    (void)s;(void)i;return name && !strncmp(name,"mcc_maps/",9) ? (FILE *)1 : NULL;
}
static FILE *mcc_open_explain(char const *name, struct mcc_cache_source *s, struct mcc_cache_identity *i,
    char *reason,size_t size) {
    if(reason && size)reason[0]=0;
    return mcc_open(name,s,i);
}
static int close_stub(FILE *f) {(void)f;return 0;}
#define fclose close_stub
static int mcc_resources_probe_explain(char const *name,struct mcc_cache_source const *s,uint32_t *checksum,
    char *reason,size_t size) {
    (void)name;(void)s;probes++;*checksum=123;
    if(!valid && reason && size)snprintf(reason,size,"No matching sounds.map. Install its MCC package resources beside the level.");
    if(changing)files.files[0].written_low++;
    return valid;
}
''' + declarations + "\n" + helpers + r'''
/* This cache test supplies an already-complete lobby result. Incremental
 * requests and cancellation are exercised separately below. */
static int mcc_validation_request(char const *name,uint32_t *sum,unsigned char *digest) {
    (void)digest;return mcc_cache_probe(name,NULL,sum,NULL,0);
}
''' + "\n".join(function(source, name) for name in
        ["mcc_cache_checksum", "mcc_cache_present"]) + r'''
int main(void) {
    char message[256],reason[320];uint32_t checksum;int i,old;
    char const *name="mcc_maps/package/a10";
    assert(mcc_cache_checksum(name)==123);assert(probes==1 && prepares==1);
    for(i=0;i<100;i++)assert(mcc_cache_present(name,123,message,sizeof(message)));
    assert(probes==1);
    strcpy(reason,"stale failure");
    assert(mcc_cache_probe(name,NULL,&checksum,reason,sizeof(reason)));
    assert(!reason[0] && checksum==123 && probes==1);
    assert(!mcc_cache_present(name,124,message,sizeof(message)));assert(probes==1);
    assert(mcc_cache_checksum(name)==123);assert(probes==1 && prepares==2);
    identity_ok=0;assert(!mcc_cache_present(name,123,message,sizeof(message)));
    assert(!mcc_cache_checksum(name));identity_ok=1;
    for(i=0;i<MCC_PRECACHE_FILES;i++) {
        old=probes;files.files[i].present++;
        assert(mcc_cache_present(name,123,message,sizeof(message)));assert(probes==old+1);
        files.files[i].written_low++;
        assert(mcc_cache_present(name,123,message,sizeof(message)));assert(probes==old+2);
    }
    files.files[0].written_low++;valid=0;old=probes;
    assert(!mcc_cache_present(name,123,message,sizeof(message)));
    assert(strstr(message,"sounds.map") && strstr(message,"Install"));
    assert(!mcc_cache_present(name,123,message,sizeof(message)));assert(probes==old+2);
    valid=1;assert(mcc_cache_checksum(name)==123);
    stamped=0;old=probes;
    assert(mcc_cache_present(name,123,message,sizeof(message)));
    assert(mcc_cache_present(name,123,message,sizeof(message)));assert(probes==old+2);
    stamped=1;changing=1;
    assert(!mcc_cache_present(name,123,message,sizeof(message)));
    assert(strstr(message,"changed during validation"));
    changing=0;assert(mcc_cache_present(name,123,message,sizeof(message)));
    old=probes;assert(mcc_cache_checksum("mcc_maps/other/a10")==123);assert(probes==old+1);
    assert(mcc_cache_checksum(name)==123);assert(probes==old+2);
    mcc_cache_validation_clear();
    old=probes;assert(mcc_cache_checksum(name)==123);assert(probes==old+1);
    return 0;
}
'''
    path = tmp_path / "validation.c"
    path.write_text(test)
    binary = compile_tool(tmp_path, [path, ROOT / "port/linux/game/mcc_errors.c"])
    subprocess.run([str(binary)], check=True, capture_output=True)


@pytest.mark.parametrize("threaded", [False, True])
def test_incremental_cache_cancellation_and_changed_files(tmp_path, threaded):
    (tmp_path / "cseries.h").write_text("typedef int boolean;\ntypedef unsigned char byte;\n")
    (tmp_path / "xtl.h").write_text(r'''
#ifndef _WIN32
#define __stdcall
#endif
#define TRUE 1
#define FALSE 0
#define WAIT_OBJECT_0 0
#define INFINITE (~0u)
static int worker_enabled,worker_quit,worker_done,worker_hold,event_handle,thread_handle;
static unsigned long (__stdcall *worker_function)(void *);
static void *CreateEventA(void *a,int b,int c,void *d) {
    (void)a;(void)b;(void)c;(void)d;worker_quit=0;return &event_handle;
}
static void *CreateThread(void *a,unsigned long b,unsigned long (__stdcall *fn)(void *),void *d,int e,void *f) {
    (void)a;(void)b;(void)d;(void)e;(void)f;worker_function=fn;worker_done=0;
    return worker_enabled ? &thread_handle : NULL;
}
static int SetEvent(void *h) {assert(h==&event_handle);worker_quit=1;return 1;}
static unsigned long WaitForSingleObject(void *h,unsigned long timeout) {
    (void)timeout;
    if(h==&event_handle)return worker_quit ? WAIT_OBJECT_0 : 258;
    assert(h==&thread_handle);
    if(!timeout && worker_hold && !worker_quit)return 258;
    if(!worker_done){worker_function(NULL);worker_done=1;}
    return WAIT_OBJECT_0;
}
static int CloseHandle(void *h) {(void)h;return 1;}
''')
    source = r'''
#include "mcc_precache.h"
#include "mcc_cache_format.h"
#include "mcc_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static struct mcc_precache_snapshot stamps;
static int opened,closed,allocated,freed,steps,bad,unreadable,changes;
struct mcc_resource_check {int step;};
int mcc_package_name(char const *name,char *out) {
    if(strncmp(name,"mcc_maps/",9)||strlen(name)>=64)return 0;
    strcpy(out,name);return 1;
}
int mcc_precache_snapshot(char const *name,struct mcc_precache_snapshot *out) {
    (void)name;*out=stamps;return !unreadable;
}
static FILE *mcc_open_explain(char const *n,struct mcc_cache_source *s,struct mcc_cache_identity *i,
    char *reason,size_t size) {
    (void)n;(void)s;(void)i;(void)reason;(void)size;opened++;return tmpfile();
}
static int close_file(FILE *f) {closed++;return fclose(f);}
#define fclose close_file
static struct mcc_resource_check *mcc_resources_check_begin(char const *n,
    struct mcc_cache_source const *s,char *r,size_t z) {
    (void)n;(void)s;(void)r;(void)z;allocated++;return calloc(1,sizeof(struct mcc_resource_check));
}
static int mcc_resources_check_step(struct mcc_resource_check *c,uint32_t *sum,unsigned char *digest) {
    steps++;if(bad)return -1;
    if(++c->step<3)return 0;
    *sum=123;memset(digest,42,32);if(changes)stamps.files[0].written_low++;return 1;
}
static void mcc_resources_check_end(struct mcc_resource_check *c) {if(c){freed++;free(c);}}
''' + (ROOT / "port/linux/game/mcc_validation.inl").read_text() + r'''
static void finish(char const *name) {
    int i;assert(!mcc_validation_request(name,NULL,NULL));
    for(i=0;i<4;i++)mcc_validation_update();
    assert(mcc_validation_lookup(name,NULL,NULL));
}
int main(void) {
    char const *name="mcc_maps/p/a";uint32_t sum;unsigned char digest[32];int i,old;
    assert(!mcc_validation_request(name,NULL,NULL));assert(!opened&&!steps);
    mcc_validation_update();assert(opened==1 && !steps);
    if(worker_enabled) {worker_hold=1;mcc_validation_update();assert(!steps&&!closed);worker_hold=0;}
    if(!worker_enabled) {mcc_validation_update();assert(steps==1 && !mcc_validation_lookup(name,NULL,NULL));}
    mcc_validation_cancel();assert(closed==opened && freed==allocated);
    /* Creating again does not accept the cancelled result. */
    finish(name);old=steps;
    assert(mcc_validation_request(name,&sum,digest)==1 && sum==123 && digest[31]==42);
    mcc_validation_cancel();assert(mcc_validation_request(name,NULL,NULL)==1 && steps==old);
    /* Every resource candidate, including absent overrides, is part of validity. */
    for(i=0;i<MCC_PRECACHE_FILES;i++) {
        stamps.files[i].present++;assert(!mcc_validation_lookup(name,NULL,NULL));finish(name);
        stamps.files[i].written_low++;assert(!mcc_validation_lookup(name,NULL,NULL));finish(name);
    }
    assert(!mcc_validation_request("mcc_maps/p/b",NULL,NULL));mcc_validation_update();
    assert(!mcc_validation_request("mcc_maps/p/c",NULL,NULL));assert(closed==opened);
    mcc_validation_cancel();
    changes=1;assert(!mcc_validation_request("mcc_maps/p/changed",NULL,NULL));
    for(i=0;i<4;i++)mcc_validation_update();
    assert(!mcc_validation_lookup("mcc_maps/p/changed",NULL,NULL));assert(strstr(mcc_validation_error(),"changed"));
    mcc_validation_cancel();changes=0;
    bad=1;assert(!mcc_validation_request("mcc_maps/p/bad",NULL,NULL));
    mcc_validation_update();mcc_validation_update();old=steps;
    assert(mcc_validation_request("mcc_maps/p/bad",NULL,NULL)==-1);
    for(i=0;i<10;i++)mcc_validation_update();assert(steps==old && !mcc_validation_lookup("mcc_maps/p/bad",NULL,NULL));
    mcc_validation_cancel();bad=0;finish("mcc_maps/p/bad");
    unreadable=1;assert(!mcc_validation_lookup("mcc_maps/p/bad",NULL,NULL));unreadable=0;
    assert(mcc_validation_request("custom_maps/a",NULL,NULL)==-1);
    finish("mcc_maps/p/d");finish("mcc_maps/p/e");finish("mcc_maps/p/f");finish("mcc_maps/p/g");
    assert(!mcc_validation_lookup("mcc_maps/p/bad",NULL,NULL));
    mcc_validation_cancel();assert(opened==closed && allocated==freed);
    return 0;
}
'''
    path = tmp_path / "incremental.c"
    source = source.replace('assert(!opened&&!steps);', 'assert(!opened&&!steps);worker_enabled='+str(int(threaded))+';')
    path.write_text(source)
    binary = compile_tool(tmp_path, [path])
    subprocess.run([str(binary)], check=True, capture_output=True)
