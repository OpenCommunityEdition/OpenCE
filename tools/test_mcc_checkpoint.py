"""Actual MCC snapshot module and native save dispatch behavior, without assets."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def checkpoint_tool(tmp_path_factory):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed for MCC checkpoint tests")
    work = tmp_path_factory.mktemp("mcc-checkpoint")
    (work / "saved games").mkdir()
    (work / "memory").mkdir()
    (work / "objects").mkdir()
    (work / "cseries.h").write_text("#pragma once\ntypedef unsigned char boolean,byte;\n")
    (work / "errors.h").write_text("#define _error_silent 0\nvoid error(int level,char const *format,...);\n")
    (work / "saved games/game_state.h").write_text(
        "#pragma once\n#include <stdint.h>\nstruct game_state_header {uint32_t allocation_checksum;"
        "char map_name[256],build_number[32];short player_count,difficulty;"
        "uint32_t cache_file_checksum,unused[7],checksum;};\n")
    (work / "memory/data.h").write_text(
        "struct data_array {short maximum_count,size;unsigned char valid;short count;void *data;};\n")
    (work / "objects/objects.h").write_text(
        "#define _object_mask_unit 3\nstruct object_datum {unsigned char state[64];};\n"
        "struct object_header_datum {short identifier;unsigned char flags,type;"
        "short cluster_index,data_size;struct object_datum *datum;};\n"
        "extern struct data_array *object_header_data;\n")
    native = (ROOT / "source/saved games/game_state.c").read_text()
    funcs = "\n".join(function(native, name) for name in
                      ("game_state_save", "game_state_save_core", "game_state_image_accept"))
    source = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define TRUE 1
#define FALSE 0
#define GAME_STATE_SIZE 262144
#define GAME_STATE_CPU_SIZE 196608
#define csmemcpy memcpy
static int loaded=1, malformed=0, reject_handles=0, captures=0, restores=0, writes=0;
static uint32_t payload_size=16;
static unsigned char live[GAME_STATE_SIZE], saved[GAME_STATE_SIZE];
static struct {void *base_address;long cpu_allocation_size;int saved_game_valid;} game_state_globals;
enum {_game_state_allocation_data,_game_state_allocation_memory_pool,_game_state_allocation_lruv_cache};
struct game_state_allocation {int kind;void *address;long maximum_count,element_size,size;};
static struct game_state_allocation game_state_allocations[1];
static long game_state_allocation_count;
static int native_valid=1;
''' + '#include "' + (ROOT / "port/linux/game/mcc_checkpoint.c").as_posix() + '"\n' + r'''
boolean mcc_cache_tags_loaded(void) {return loaded;}
static int campaign_restores;
void mcc_campaign_snapshot(void *out) {memset(out,0,MCC_CAMPAIGN_SNAPSHOT_BYTES);((unsigned char *)out)[0]=1;}
int mcc_campaign_validate(void const *in,unsigned long bytes) {
    return bytes==MCC_CAMPAIGN_SNAPSHOT_BYTES && ((unsigned char const *)in)[0]==1;
}
int mcc_campaign_restore(void const *in,unsigned long bytes) {
    if(!mcc_campaign_validate(in,bytes))return 0;campaign_restores++;return 1;
}
void mcc_campaign_begin(void) {campaign_restores++;}
struct data_array *object_header_data;
short mcc_grenades_type_count(void) {return 4;}
boolean mcc_level_name(char const *name) {return !strncmp(name,"mcc_maps\\",9);}
void error(int level,char const *format,...) {(void)level;(void)format;}
uint32_t mcc_grenades_snapshot(void *out,uint32_t capacity) {
    unsigned char *p=out;
    if(!p)return payload_size;
    if(capacity<payload_size)return 0;
    memset(p,0,payload_size);memcpy(p,"GMC4",4);p[4]=1;
    captures++;return malformed?0:payload_size;
}
int mcc_grenades_snapshot_validate(void const *in,uint32_t bytes) {
    return bytes>=8 && !memcmp(in,"GMC4\1\0",6);
}
int mcc_grenades_restore(void const *in,uint32_t bytes) {
    if(!mcc_grenades_snapshot_validate(in,bytes)||reject_handles)return 0;
    restores++;return 1;
}
void game_state_call_before_save_procs(void) {}
void main_stop_time(void) {}
void main_start_time(void) {}
void game_state_note_event(char const *event) {(void)event;}
int game_state_write_to_file(void) {memcpy(saved,live,sizeof(saved));writes++;return 1;}
int game_state_write_core(char const *name,void const *data,long size) {
    (void)name;memcpy(saved,data,size);writes++;return 1;
}
void console_printf(int flag,char const *format,...) {(void)flag;(void)format;}
int game_state_image_refuse(char const *name,char const *reason) {(void)name;(void)reason;return 0;}
int game_state_image_data_valid(void *image,void *address,long count,long size) {
    (void)image;(void)address;(void)count;(void)size;return native_valid;
}
int game_state_image_memory_pool_valid(void *image,void *address,long size) {
    (void)image;(void)address;(void)size;return native_valid;
}
int game_state_image_lruv_cache_valid(void *image,void *address,struct game_state_allocation const *a) {
    (void)image;(void)address;(void)a;return native_valid;
}
''' + funcs + r'''
int main(int argc,char **argv) {
    unsigned char *tail,*footer;
    unsigned long used=4096;
    int result=0;
    struct game_state_header *header=(void *)live,*incoming=(void *)saved;
    if(argc!=2)return 2;
    memset(live,0xA5,sizeof(live));
    memset(header,0,sizeof(*header));strcpy(header->map_name,"mcc_maps\\a10");
    header->cache_file_checksum=0x12345678;
    object_header_data=(void *)(live+512);
    memset(object_header_data,0,sizeof(*object_header_data));
    object_header_data->maximum_count=object_header_data->count=1;
    object_header_data->valid=1;object_header_data->size=sizeof(struct object_header_datum);
    object_header_data->data=live+640;
    {struct object_header_datum *unit=object_header_data->data;
     memset(unit,0,sizeof(*unit));unit->identifier=1;unit->datum=(void *)(live+768);}
    game_state_globals.base_address=live;game_state_globals.cpu_allocation_size=used;
    tail=live+GAME_STATE_CPU_SIZE-MCC_CHECKPOINT_SLOT_BYTES;
    footer=tail+MCC_CHECKPOINT_TOTAL_PAYLOAD;
    if(!strcmp(argv[1],"stock")) {
        loaded=0;strcpy(header->map_name,"levels\\a10\\a10");
        game_state_save();result=game_state_image_accept(saved,sizeof(saved));
        if(captures||restores||memcmp(tail,saved+(tail-live),MCC_CHECKPOINT_SLOT_BYTES))return 3;
    } else if(!strcmp(argv[1],"capacity")) {
        game_state_globals.cpu_allocation_size=GAME_STATE_CPU_SIZE-MCC_CHECKPOINT_SLOT_BYTES+1;
        game_state_save();result=!writes&&!game_state_globals.saved_game_valid;
    } else if(!strcmp(argv[1],"capture_fail")) {
        malformed=1;game_state_save();result=!writes&&!game_state_globals.saved_game_valid;
    } else if(!strcmp(argv[1],"maximum")) {
        payload_size=MCC_CHECKPOINT_PAYLOAD_LIMIT;
        result=mcc_checkpoint_capture(live,sizeof(live),used,GAME_STATE_CPU_SIZE);
        result &= mcc_checkpoint_restore(live,sizeof(live),used,GAME_STATE_CPU_SIZE);
    } else if(!strcmp(argv[1],"oversize")) {
        payload_size=MCC_CHECKPOINT_PAYLOAD_LIMIT+1;
        result=!mcc_checkpoint_capture(live,sizeof(live),used,GAME_STATE_CPU_SIZE);
    } else {
        if(!strcmp(argv[1],"core"))game_state_save_core("test");else game_state_save();
        if(writes!=1||captures!=1)return 4;
        if(!strcmp(argv[1],"v1")) {
            unsigned char *f=saved+(footer-live),*p=saved+(tail-live);
            memmove(p+MCC_CAMPAIGN_SNAPSHOT_BYTES,p,payload_size);
            mcc_checkpoint_put(f+4,1);mcc_checkpoint_put(f+8,payload_size);
            mcc_checkpoint_put(f+12,mcc_checkpoint_checksum(p+MCC_CAMPAIGN_SNAPSHOT_BYTES,payload_size));
            mcc_checkpoint_put(f+20,0);
            result=game_state_image_accept(saved,sizeof(saved));
            if(restores!=1 || campaign_restores!=1)return 12;
        } else if(!strcmp(argv[1],"roundtrip")||!strcmp(argv[1],"core")) {
            live[4095]=33;result=game_state_image_accept(saved,sizeof(saved));
            if(restores!=1||campaign_restores!=1||live[4095]!=0xA5)return 5;
        } else if(!strcmp(argv[1],"cleanup")) {
            mcc_checkpoint_dispose();
            for(unsigned long i=0;i<MCC_CHECKPOINT_SLOT_BYTES;i++)if(tail[i]!=0xA5)return 6;
            result=1;
        } else {
            unsigned char *f=saved+(footer-live),*p=saved+(tail-live);
            if(!strcmp(argv[1],"payload"))p[7]^=1;
            else if(!strcmp(argv[1],"campaign")) {
                p[payload_size]=2;
                mcc_checkpoint_put(f+12,mcc_checkpoint_checksum(p,payload_size+MCC_CAMPAIGN_SNAPSHOT_BYTES));
            }
            else if(!strcmp(argv[1],"footer"))f[0]^=1;
            else if(!strcmp(argv[1],"version"))f[4]=3;
            else if(!strcmp(argv[1],"reserved"))f[88]=1;
            else if(!strcmp(argv[1],"name"))strcpy(incoming->map_name,"mcc_maps\\other");
            else if(!strcmp(argv[1],"checksum"))incoming->cache_file_checksum++;
            else if(!strcmp(argv[1],"highwater"))game_state_globals.cpu_allocation_size=GAME_STATE_CPU_SIZE-1;
            else if(!strcmp(argv[1],"native")){game_state_allocation_count=1;native_valid=0;}
            else if(!strcmp(argv[1],"handles"))reject_handles=1;
            else if(!strncmp(argv[1],"unit_",5)) {
                struct data_array *a=(void *)(saved+512);
                struct object_header_datum *unit=(void *)(saved+640);
                p[6]=1;p[8]=0;p[9]=0;p[10]=1;p[11]=0;p[12]=2;
                mcc_checkpoint_put(f+12,mcc_checkpoint_checksum(p,16+MCC_CAMPAIGN_SNAPSHOT_BYTES));
                if(!strcmp(argv[1],"unit_salt"))unit->identifier=2;
                else if(!strcmp(argv[1],"unit_type"))unit->type=2;
                else if(!strcmp(argv[1],"unit_pointer"))unit->datum=(void *)(live+sizeof(live));
                else if(!strcmp(argv[1],"unit_array"))a->data=(void *)(live+sizeof(live));
                else return 11;
            }
            else return 7;
            result=!game_state_image_accept(saved,sizeof(saved));
            if(restores)return 8;
        }
    }
    mcc_checkpoint_dispose();
    if(!result)return 9;
    puts("ok");return 0;
}
'''
    path = work / "checkpoint.c"
    path.write_text(source)
    output = work / ("checkpoint.exe" if sys.platform == "win32" else "checkpoint")
    command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror", "-I", str(work), str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


@pytest.mark.parametrize("case", ["stock", "capacity", "capture_fail", "maximum", "oversize",
                                      "roundtrip", "core", "cleanup", "payload", "footer", "version",
                                      "reserved", "name", "checksum", "highwater", "native", "handles",
                                      "unit_salt", "unit_type", "unit_pointer", "unit_array", "v1", "campaign"])
def test_mcc_checkpoint_and_native_hooks(checkpoint_tool, case):
    result = subprocess.run([str(checkpoint_tool), case], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr + f" ({case}: {result.returncode})"
