"""Required host files are an admission barrier, not optional cosmetics."""
import os
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'port/linux/game/mutator_share.c'


def function(text, name):
    start = re.search(r'^(?:static )?(?:void|boolean) ' + name + r'\(', text, re.M).start()
    end = text.index('\n}', start) + 2
    return text[start:end]


def test_required_content_admission():
    source = SOURCE.read_text()
    code = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define MAXIMUM_SHARED_MUTATORS 2
#define MAX(a,b) ((a)>(b)?(a):(b))
#define _error_log 0
#define _error_silent 0
#define _distributed_message_mutator_share_done 1
#define _distributed_message_mutator_share_fetching 2
#define ANSWER_TIMEOUT 100
#define FETCHING_INTERVAL 20
#define _share_asking 1
#define _share_fetching 2
#define _share_done 3
#define _share_failed 4
struct file { unsigned long size; char name[32]; };
struct port_mutator_share { char id[32]; int file_count; struct file files[3]; };
static struct { int total, received, mutator, file, state, in_progress; int script_allowed[2];
struct port_mutator_share entries[2]; unsigned long arrived, heard_at, fetching_at, asked;
int progress_ready, required[2][3]; unsigned long rate_at;
unsigned long long required_bytes, received_bytes; void *bytes; } mutator_share;
static int valid=1, has=1, script=0, add=1, downloads=1, failures, done, asks, active;
#define error(...) ((void)0)
static int port_mutators_share_valid(const void *p) { return valid; }
static int port_mutators_share_has(const void *p) { return has; }
static int port_mutators_share_has_script(const void *p) { return script; }
static int port_mutators_host_add(const void *p, int allowed) { active++; return add; }
static void port_mutators_host_begin(void) { active=0; }
static void port_mutators_host_end(void) {}
static int config_boolean(char *key) { return downloads; }
static unsigned long system_milliseconds(void) { return 123; }
static void client_send(int type) { if(type==1) done++; }
static int client_scripts_decided(void) { return 1; }
static int port_mutators_share_file_has(const void *p, int index) { return 0; }
static int port_mutators_share_store(const void *p, int index, void *bytes, unsigned long size) { has=1; return 1; }
static void client_ask_file(void) { asks++; }
static void mutator_share_fail(char const *reason) { failures++; active=0; mutator_share.state=_share_failed; }
'''
    code += function(source, 'client_finish') + '\n' + function(source, 'client_next_file')
    code += r'''
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d\n",__LINE__); return 1; } } while(0)
static void reset(void) { memset(&mutator_share,0,sizeof(mutator_share));
mutator_share.total=mutator_share.received=1; mutator_share.script_allowed[0]=1;
mutator_share.entries[0].file_count=2; mutator_share.entries[0].files[0].size=4;
mutator_share.entries[0].files[1].size=8; valid=has=add=downloads=1;
script=failures=done=asks=active=0; }
int main(void) {
reset(); downloads=0; client_next_file(); CHECK(done==1 && active==1 && !failures);
reset(); downloads=0; has=0; client_next_file(); CHECK(failures==1 && !done && !active);
reset(); has=0; client_next_file(); CHECK(!done && !failures && asks==0 && mutator_share.state==_share_fetching); CHECK(mutator_share.required_bytes==12); free(mutator_share.bytes);
reset(); has=0; mutator_share.received=0; client_next_file(); CHECK(!mutator_share.progress_ready && !mutator_share.required_bytes);
reset(); client_next_file(); CHECK(mutator_share.required_bytes==0);
reset(); has=0; mutator_share.entries[0].file_count=1; mutator_share.entries[0].files[0].size=0; client_next_file(); CHECK(done==1 && !failures && !mutator_share.bytes && mutator_share.required_bytes==0);
reset(); has=0; mutator_share.entries[0].file_count=2; mutator_share.entries[0].files[1].size=24; client_next_file(); CHECK(mutator_share.required_bytes==28); CHECK(mutator_share.required[0][0] && mutator_share.required[0][1]); free(mutator_share.bytes);
reset(); valid=0; client_next_file(); CHECK(failures==1 && !done);
reset(); script=1; mutator_share.script_allowed[0]=0; client_next_file(); CHECK(failures==1 && !done);
reset(); add=0; client_next_file(); CHECK(failures==1 && !done && !active);
reset(); mutator_share.received=0; client_finish(); CHECK(failures==1 && !done);
reset(); has=0; client_finish(); CHECK(failures==1 && !done);
return 0;
}
'''
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder)
        (path / 'admission.c').write_text(code)
        executable = path / 'admission'
        subprocess.run([os.environ.get('CC', 'clang'), '-std=gnu99', '-Werror', '-Wno-unused-function', str(path / 'admission.c'), '-o', str(executable)], check=True)
        fd = os.open(executable, os.O_RDONLY)
        try:
            os.fchmod(fd, 0o755)
        finally:
            os.close(fd)
        subprocess.run([str(executable)], check=True)
