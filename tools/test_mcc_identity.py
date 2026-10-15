"""MCC content matching uses all digest bytes and never rewrites wire paths."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import sys
import pytest
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


def compile_tool(work, sources):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    exe = work / ("test.exe" if sys.platform == "win32" else "test")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if sys.platform == "win32" else []
    result = subprocess.run([clang, *flags, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
                             "-I", str(work), "-I", str(ROOT / "port/linux/game"),
                             *map(str, sources), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.fixture(scope="module")
def sha_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-sha256")
    source = work / "test.c"
    source.write_text(r'''
#include "mcc_sha256.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
 struct mcc_sha256 hash;unsigned char data[32768],digest[32];size_t count;unsigned i;
 FILE *f;if(argc!=3)return 2;f=fopen(argv[1],"rb");if(!f)return 3;
 count=(size_t)atoi(argv[2]);if(!count || count>sizeof(data))return 4;
 mcc_sha256_begin(&hash);
 while((i=(unsigned)fread(data,1,count,f))!=0)mcc_sha256_add(&hash,data,(int)i);
 if(ferror(f))return 5;fclose(f);mcc_sha256_end(&hash,digest);
 for(i=0;i<32;i++)printf("%02x",digest[i]);return 0;
}
''')
    return compile_tool(work, [source, ROOT / "port/linux/game/mcc_sha256.c"])


@pytest.mark.parametrize("data", [b"", b"abc", b"a" * 55, b"a" * 56, b"a" * 64,
                                  b"a" * 65, bytes(range(256)) * 4096],
                         ids=["empty", "abc", "55-bytes", "56-bytes", "64-bytes", "65-bytes", "1-MiB"])
@pytest.mark.parametrize("chunk", [1, 63, 32768])
def test_sha256_vectors(sha_tool, tmp_path, data, chunk):
    path = tmp_path / "bytes"
    path.write_bytes(data)
    result = subprocess.run([str(sha_tool), str(path), str(chunk)], capture_output=True, text=True)
    assert result.returncode == 0
    assert result.stdout == hashlib.sha256(data).hexdigest()


def test_session_resolution_and_validation(tmp_path):
    headers = {
        "cseries.h": "typedef int boolean;\ntypedef unsigned char byte;\n#define FALSE 0\n#define TRUE 1\nint csstrcasecmp(char const *,char const *);\n",
        "errors.h": "#define _error_silent 0\nvoid error(short,char const *,...);\n",
        "interface/ui_widget.h": "#include <stddef.h>\nvoid display_error_text_when_main_menu_loaded(wchar_t const *);\n",
        "game/game.h": "enum { _game_connection_local, _game_connection_network_client, _game_connection_network_server };\nint game_connection(void);\n",
        "networking/network_game_manager.h": "struct network_game {struct {long version;char name[128];} map;unsigned char mcc_content_sha256[32];};\n",
    }
    for name, text in headers.items():
        path = tmp_path / name
        path.parent.mkdir(exist_ok=True)
        path.write_text(text)
    source = tmp_path / "test.c"
    source.write_text(r'''
#include "cseries.h"
#include "mcc_identity.h"
#include "mcc_precache.h"
#include "networking/network_game_manager.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static struct {char name[64];unsigned char digest[32];unsigned stamp;} files[20];
static int count,probes,errors,connection,changing,denied,clears,hash_loads;
int csstrcasecmp(char const *a,char const *b){while(*a && tolower(*a)==tolower(*b)){a++;b++;}return tolower(*a)-tolower(*b);}
int game_connection(void){return connection;}
void error(short level,char const *format,...){(void)level;(void)format;}
void display_error_text_when_main_menu_loaded(wchar_t const *text){(void)text;errors++;}
void mcc_cache_precache_clear(void){clears++;}
void mcc_maps_rescan(void){}
short mcc_maps_count(boolean multiplayer){(void)multiplayer;return (short)count;}
char const *mcc_maps_level_name(short i){return files[i].name;}
boolean mcc_level_name(char const *s){return !strncmp(s,"mcc_maps",8);}
static int find(char const *s){int i;for(i=0;i<count;i++)if(!strcmp(s,files[i].name))return i;return -1;}
int mcc_precache_snapshot(char const *s,struct mcc_precache_snapshot *out){
 int i=find(s);memset(out,0,sizeof(*out));if(denied)return 0;
 if(i>=0){out->files[0].present=1;out->files[0].written_low=files[i].stamp;}return 1;
}
int mcc_content_probe(char const *s,unsigned char digest[32]){
 int i=find(s);probes++;if(i<0)return 0;memcpy(digest,files[i].digest,32);
 if(changing)files[i].stamp++;return 1;
}
int mcc_resources_fingerprint(struct mcc_runtime *r,unsigned char digest[32]){
 (void)r;hash_loads++;memcpy(digest,files[0].digest,32);return 1;
}
int main(void){
 struct network_game game,copy;unsigned char wanted[32];int old,i;
 memset(&game,0,sizeof(game));memset(wanted,42,32);
 strcpy(game.map.name,"mcc_maps\\Host\\a10");memcpy(game.mcc_content_sha256,wanted,32);
 count=1;strcpy(files[0].name,"mcc_maps\\Client\\a10");memcpy(files[0].digest,wanted,32);
 CHECK(mcc_identity_accept(&game));CHECK(!strcmp(game.map.name,"mcc_maps\\Host\\a10"));
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),files[0].name));
 old=probes;CHECK(mcc_identity_check(game.map.name));CHECK(probes==old);
 /* Full loading rehashes the open map/resources, even with unchanged metadata. */
 connection=1;CHECK(mcc_identity_loaded(game.map.name,NULL));CHECK(hash_loads==1);
 files[0].digest[31]^=1;CHECK(!mcc_identity_loaded(game.map.name,NULL));
 files[0].stamp++;CHECK(!mcc_identity_check(game.map.name));
 CHECK(!mcc_identity_accept(&game));CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));
 files[0].digest[31]^=1;CHECK(mcc_identity_accept(&game));
 /* Duplicate fallback matches are refused, but an exact verified path wins. */
 files[1]=files[0];strcpy(files[1].name,"mcc_maps\\Duplicate\\a10");count=2;
 CHECK(!mcc_identity_accept(&game));strcpy(files[1].name,game.map.name);
 CHECK(mcc_identity_accept(&game));CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));
 files[1].digest[0]^=1;CHECK(mcc_identity_accept(&game));
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),files[0].name));
 changing=1;CHECK(!mcc_identity_accept(&game));changing=0;
 denied=1;CHECK(!mcc_identity_accept(&game));denied=0;
 /* No zero digest, traversal, or stale alias after a legacy map/disconnect. */
 memset(game.mcc_content_sha256,0,32);CHECK(!mcc_identity_accept(&game));
 memcpy(game.mcc_content_sha256,wanted,32);strcpy(game.map.name,"mcc_maps\\..\\a10");
 CHECK(!mcc_identity_accept(&game));strcpy(game.map.name,"mcc_maps\\Host\\a10");
 CHECK(mcc_identity_accept(&game));mcc_identity_disconnect();
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));
 CHECK(!mcc_identity_loaded(game.map.name,NULL));connection=0;CHECK(mcc_identity_loaded(game.map.name,NULL));
 strcpy(game.map.name,"levels\\test\\bloodgulch\\bloodgulch");
 memset(game.mcc_content_sha256,0,32);CHECK(mcc_identity_accept(&game));
 /* Host cache avoids repeat hashing; mutations and disconnect invalidate it. */
 strcpy(game.map.name,files[0].name);old=probes;
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy)==&copy);CHECK(probes==old+1);
 CHECK(!memcmp(copy.mcc_content_sha256,wanted,32));CHECK(!game.mcc_content_sha256[0]);
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));CHECK(probes==old+1);
 files[0].stamp++;CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));CHECK(probes==old+2);
 mcc_identity_disconnect();changing=1;CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));changing=0;
 CHECK(!mcc_identity_host_settings(&game,sizeof(game)-1,&copy));
 /* Bound same-filename work before any fallback fingerprints. */
 strcpy(game.map.name,"mcc_maps\\Host\\a10");memcpy(game.mcc_content_sha256,wanted,32);count=17;
 for(i=0;i<count;i++)sprintf(files[i].name,"mcc_maps\\folder%d\\a10",i);
 old=probes;CHECK(!mcc_identity_accept(&game));CHECK(probes==old+1);
 CHECK(errors && clears);return 0;
}
''')
    exe = compile_tool(tmp_path, [source, ROOT / "port/linux/game/mcc_identity.c",
                                  ROOT / "port/linux/game/mcc_package.c"])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


def test_fingerprint_settings_keep_host_and_fragment_checks(tmp_path):
    native = (ROOT / "source/networking/network_client_message_handler.c").read_text()
    source = tmp_path / "settings.c"
    source.write_text('#include "'+(ROOT / "port/linux/include/halo_port_limits.h").as_posix()+'"\n'+r'''
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
typedef int boolean;typedef unsigned char byte;typedef uint16_t word;
#define TRUE 1
#define FALSE 0
#define NETWORK_CLIENT_MESSAGE_HANDLER_FILE "test"
#define NETWORK_GAME_MESSAGE_VERSION 2
#define _network_game_client_state_pregame 3
#define _message_server_game_settings_update 4
#define _network_game_packet_class_pregame 5
#define match_assert(file,line,value) ((void)0)
#define network_event(...) ((void)0)
#define csmemcpy memcpy
#define csmemset memset
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct network_game {byte previous[1030];byte mcc_content_sha256[32];};
struct network_game_client {int dummy;};struct transport_address {int host;};
static int applies,decodes,state=3;static struct network_game received;
static struct network_game network_game_client_settings_staging;
static long network_game_client_settings_staging_size,network_game_client_settings_staging_total;
static int mcc_protocol_settings_valid(struct network_game const *game,long bytes){(void)game;return bytes==sizeof(*game);}
static boolean network_game_client_game_settings_updated(struct network_game_client *c,struct network_game *g){(void)c;received=*g;applies++;return 1;}
static boolean network_game_client_address_matches_server(struct network_game_client *c,struct transport_address *a){(void)c;return a->host;}
static int network_game_client_get_state(struct network_game_client *c,void *unused){(void)c;(void)unused;return state;}
''' + structure(native, "message_server_game_settings_update") + r'''
static struct message_server_game_settings_update decoded;
static boolean decode_network_game_message(void *out,word *m,short *size,short *type,short *version,int cls){
 (void)m;(void)size;(void)type;(void)version;(void)cls;decodes++;memcpy(out,&decoded,sizeof(decoded));return 1;
}
''' + function(native, "network_game_client_receive_game_settings_piece") + "\n" +
        function(native, "network_game_client_handle_message_server_game_settings_update") + r'''
int main(void){
 struct network_game_client c;struct transport_address address={0};word packet[4]={0};
 struct network_game original;unsigned offset,n;memset(&original,42,sizeof(original));
 memset(&decoded,0,sizeof(decoded));decoded.total_size=sizeof(original);
 CHECK(network_game_client_handle_message_server_game_settings_update(&c,packet,8,&address));
 CHECK(!decodes && !applies);address.host=1;
 for(offset=0;offset<sizeof(original);offset+=n){
  n=sizeof(original)-offset;if(n>sizeof(decoded.data))n=sizeof(decoded.data);
  decoded.offset=(word)offset;decoded.length=(word)n;memcpy(decoded.data,(byte *)&original+offset,n);
  CHECK(network_game_client_handle_message_server_game_settings_update(&c,packet,8,&address));
  CHECK(applies==(offset+n==sizeof(original)));
 }
 CHECK(!memcmp(&received,&original,sizeof(original)));
 decoded.total_size-=32;CHECK(!network_game_client_receive_game_settings_piece(&c,&decoded));
 decoded.total_size=sizeof(original);decoded.offset=sizeof(original)-1;decoded.length=2;
 CHECK(!network_game_client_receive_game_settings_piece(&c,&decoded));
 decoded.offset=0;decoded.length=sizeof(decoded.data)+1;
 CHECK(!network_game_client_receive_game_settings_piece(&c,&decoded));
 decoded.offset=20;decoded.length=1;CHECK(network_game_client_receive_game_settings_piece(&c,&decoded));
 CHECK(applies==1 && !network_game_client_settings_staging_size);
 state=0;CHECK(network_game_client_handle_message_server_game_settings_update(&c,packet,8,&address));
 CHECK(applies==1);return 0;
}
''')
    exe = compile_tool(tmp_path, [source])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
