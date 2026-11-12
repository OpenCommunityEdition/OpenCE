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
        "cseries.h": "#include <stdio.h>\ntypedef int boolean;\ntypedef unsigned char byte;\ntypedef unsigned short word;\n#define FALSE 0\n#define TRUE 1\nint csstrcasecmp(char const *,char const *);\nFILE *fixture_fopen(char const *,char const *);\n#define fopen fixture_fopen\n",
        "errors.h": "#define _error_silent 0\nvoid error(short,char const *,...);\n",
        "interface/ui_widget.h": "#include <stddef.h>\nvoid display_error_text_when_main_menu_loaded(wchar_t const *);\n",
        "game/game.h": "enum { _game_connection_local, _game_connection_network_client, _game_connection_network_server };\nint game_connection(void);\nvoid network_game_abort(void);\n",
        "networking/network_game_globals.h": "void network_game_abort(void);\n",
        "networking/network_game_manager.h": "struct network_game {struct {long version;char name[128];} map;unsigned char mcc_content_sha256[32];unsigned char mcc_metadata_sha256[32];};\n",
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
#include "mcc_validation.h"
#include <stdlib.h>
#include "mcc_workshop.h"
#include "networking/network_game_manager.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#undef fopen
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static struct {char name[64];unsigned char digest[32],metadata[32];unsigned stamp;int invalid_metadata;} files[20];
static int count,probes,errors,connection,changing,denied,clears,hash_loads,rescan_install;
static int lease,lease_wait,lease_begins,aborts,open_checks,ready_blocked;
void network_game_abort(void){aborts++;}
int mcc_admission_client_begin(struct network_game const *g){(void)g;lease=1;lease_begins++;return 1;}
int mcc_admission_client_status(void){return lease_wait ? 0 : lease;}
int mcc_admission_client_pending(void){return lease>0;}
void mcc_admission_client_clear(void){lease=0;}
void mcc_admission_client_finish(int ready){if(!ready || !ready_blocked)lease=0;}
void mcc_admission_disconnect(void){lease=0;}
static struct mcc_validation_proof cached;
static char const *sidecar;
/* Controlled virtual filesystem, retaining the real sidecar parser/reader. */
FILE *fixture_fopen(char const *path,char const *mode){
 FILE *file;(void)mode;if(!sidecar || !strstr(path,"workshop_id.txt"))return NULL;
 file=tmpfile();if(file){fputs(sidecar,file);rewind(file);}return file;
}
int csstrcasecmp(char const *a,char const *b){while(*a && tolower(*a)==tolower(*b)){a++;b++;}return tolower(*a)-tolower(*b);}
int game_connection(void){return connection;}
void error(short level,char const *format,...){(void)level;(void)format;}
static char last_error[256];
void display_error_text_when_main_menu_loaded(wchar_t const *text){int k;errors++;for(k=0;text[k] && k<255;k++)last_error[k]=(char)text[k];last_error[k]=0;}
void mcc_share_game_disconnect(void) {}
void mcc_cache_validation_clear(void) {memset(&cached,0,sizeof(cached));}
void mcc_validation_cancel(void) {}
void mcc_cache_precache_clear(void){clears++;}
void mcc_maps_rescan(void){
 if(rescan_install){count=1;strcpy(files[0].name,"mcc_maps\\2899609719\\pat_beavercreek");memset(files[0].digest,42,32);}
}
boolean mcc_maps_workshop_title(char const *id,char *title,unsigned size){(void)id;(void)title;(void)size;return 0;}
short mcc_maps_count(boolean multiplayer){(void)multiplayer;return (short)count;}
char const *mcc_maps_level_name(short i){return files[i].name;}
boolean mcc_level_name(char const *s){return !strncmp(s,"mcc_maps",8);}
static int find(char const *s){int i;for(i=0;i<count;i++)if(!strcmp(s,files[i].name))return i;return -1;}
/* The real metadata parser/digest has its own filesystem regressions. Here each
 * virtual package supplies controlled JSON identities independently of map bytes. */
int mcc_manifest_package_root(char const *name,char root[1024]){strcpy(root,name);return 1;}
int mcc_share_metadata_digest(char const *root,int campaign,char const *level,
 unsigned char digest[32],char reason[128]){
 int i=find(root);(void)campaign;(void)level;(void)reason;memset(digest,0,32);
 if(i<0)return 1;if(files[i].invalid_metadata)return 0;
 memcpy(digest,files[i].metadata,32);return 1;
}
static int offers,allow_offer;
static unsigned char offered_digest[32],offered_metadata[32];
static int offer(char const name[128],unsigned char const digest[32],unsigned char const metadata[32]){
 (void)name;offers++;memcpy(offered_digest,digest,32);memcpy(offered_metadata,metadata,32);if(allow_offer)mcc_admission_client_finish(0);return allow_offer;
}
int mcc_precache_snapshot(char const *s,struct mcc_precache_snapshot *out){
 int i=find(s);memset(out,0,sizeof(*out));if(denied)return 0;
 if(i>=0){out->files[0].present=1;out->files[0].written_low=files[i].stamp;}return 1;
}
int mcc_content_probe(char const *s,unsigned char digest[32]){
 int i=find(s);probes++;if(i<0)return 0;memcpy(digest,files[i].digest,32);
 if(changing)files[i].stamp++;return 1;
}

struct mcc_validation_check {char name[64];struct mcc_precache_snapshot before;int steps;};
int mcc_validation_lookup(char const *s,uint32_t *sum,unsigned char digest[32]){
 struct mcc_precache_snapshot now;
 if(strcmp(s,cached.name) || !mcc_precache_snapshot(s,&now) || memcmp(&now,&cached.files,sizeof(now)))return 0;
 if(sum)*sum=cached.checksum;if(digest)memcpy(digest,cached.digest,32);return 1;
}
int mcc_validation_publish(char const *s,struct mcc_validation_proof const *proof){
 struct mcc_precache_snapshot now;
 if(strcmp(s,proof->name) || !mcc_precache_snapshot(s,&now) || memcmp(&now,&proof->files,sizeof(now)))return 0;
 cached=*proof;return 1;
}
int mcc_validation_request(char const *s,uint32_t *sum,unsigned char digest[32]){
 struct mcc_validation_proof proof;struct mcc_precache_snapshot after;
 if(mcc_validation_lookup(s,sum,digest))return 1;
 memset(&proof,0,sizeof(proof));strcpy(proof.name,s);proof.checksum=123;
 if(!mcc_precache_snapshot(s,&proof.files) || !mcc_content_probe(s,proof.digest) ||
    !mcc_precache_snapshot(s,&after) || memcmp(&after,&proof.files,sizeof(after)))return -1;
 if(sum)*sum=123;if(digest)memcpy(digest,proof.digest,32);return mcc_validation_publish(s,&proof);
}
struct mcc_validation_check *mcc_validation_check_begin(char const *s,char const *dir){
 struct mcc_validation_check *check=malloc(sizeof(*check));(void)dir;
 if(!check)return NULL;memset(check,0,sizeof(*check));strcpy(check->name,s);
 mcc_precache_snapshot(s,&check->before);open_checks++;return check;
}
int mcc_validation_check_step(struct mcc_validation_check *check,struct mcc_validation_proof *proof,char *reason,size_t size){
 struct mcc_precache_snapshot after;(void)reason;(void)size;
 if(!check->steps++)return 0;
 memset(proof,0,sizeof(*proof));strcpy(proof->name,check->name);proof->checksum=123;
 if(!mcc_content_probe(check->name,proof->digest) || !mcc_precache_snapshot(check->name,&after) ||
    memcmp(&after,&check->before,sizeof(after)))return -1;
 proof->files=after;return 1;
}
void mcc_validation_check_end(struct mcc_validation_check *check){if(check){open_checks--;free(check);}}
/* Most legacy cases await the real asynchronous state machine. Separate cases
 * below observe each pending boundary before allowing the next frame. */
static int complete_accept(struct network_game const *game){
 int before=aborts,n;
 if(!mcc_identity_accept(game))return 0;
 for(n=0;n<200 && mcc_identity_pending(NULL);n++)mcc_identity_update();
 return !mcc_identity_pending(NULL) && aborts==before;
}
#define mcc_identity_accept(game) complete_accept(game)
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
 files[1].digest[0]^=1;files[1].stamp++;CHECK(mcc_identity_accept(&game));
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
 /* Prepare before opening the lobby: even a slow fingerprint must not be
  * repeated by the first settings send while a client waits to join. */
 strcpy(game.map.name,files[0].name);game.map.version=123;old=probes;
 CHECK(mcc_identity_prepare_host(game.map.name));CHECK(probes==old+1);
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy)==&copy);CHECK(probes==old+1);
 CHECK(!memcmp(copy.mcc_content_sha256,wanted,32));CHECK(!game.mcc_content_sha256[0]);
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));CHECK(probes==old+1);
 /* The local admission reuses this exact snapshot, but neither a changed
  * file nor a different wire digest can use it. Loading still rehashes. */
 CHECK(mcc_identity_accept(&copy));CHECK(probes==old+1);
 files[0].stamp++;CHECK(mcc_identity_accept(&copy));CHECK(probes==old+2);
 files[0].stamp--;copy.mcc_content_sha256[31]^=1;
 CHECK(!mcc_identity_accept(&copy));CHECK(probes>old+2);
 CHECK(mcc_validation_request(game.map.name,NULL,NULL)==1);
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));
 old=probes;
 files[0].stamp++;CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));CHECK(probes==old);
 CHECK(mcc_identity_prepare_host(game.map.name));CHECK(probes==old+1);
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));
 mcc_identity_disconnect();changing=1;CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));changing=0;
 CHECK(!mcc_identity_host_settings(&game,sizeof(game)-1,&copy));
 /* Invalid or unreadable preparation drops a previously cached identity. */
 CHECK(mcc_identity_prepare_host(game.map.name));old=probes;
 denied=1;CHECK(!mcc_identity_prepare_host(game.map.name));denied=0;
 CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));CHECK(probes==old);
 CHECK(mcc_identity_prepare_host(game.map.name));CHECK(probes==old);
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));
 CHECK(!mcc_identity_prepare_host("custom_maps/a10"));
 CHECK(!mcc_identity_prepare_host("levels/test/bloodgulch/bloodgulch"));
 /* Bound same-filename work before any fallback fingerprints. */
 strcpy(game.map.name,"mcc_maps\\Host\\a10");memcpy(game.mcc_content_sha256,wanted,32);count=17;
 for(i=0;i<count;i++)sprintf(files[i].name,"mcc_maps\\folder%d\\a10",i);
 old=probes;CHECK(!mcc_identity_accept(&game));CHECK(probes==old+1);
 CHECK(errors && clears);
 /* Filename equality does not prove item presence; a known older item may
  * not contain the host's newly added level at all. */
 {
  struct mcc_workshop_item item;
  memset(game.map.name,0,sizeof(game.map.name));strcpy(game.map.name,"mcc_maps\\2899609719\\pat_beavercreek");
  strcpy(item.id,"2899609719");strcpy(item.title,"Better Creek");mcc_workshop_wire_pack(game.map.name,&item);
  count=1;strcpy(files[0].name,"mcc_maps\\Other\\pat_beavercreek");memset(files[0].digest,9,32);
  CHECK(!mcc_identity_accept(&game));
  CHECK(strstr(last_error,"No identified local copy of Better Creek (Steam Workshop item 2899609719)"));
  strcpy(files[0].name,"mcc_maps\\2899609719\\older_level");
  CHECK(!mcc_identity_accept(&game));
  CHECK(strstr(last_error,"Your local package for Better Creek (Steam Workshop item 2899609719) does not match"));
  strcpy(files[0].name,"mcc_maps\\12345\\pat_beavercreek");
  CHECK(!mcc_identity_accept(&game));
  CHECK(strstr(last_error,"No identified local copy"));
  strcpy(files[0].name,"mcc_maps\\2899609719\\pat_beavercreek");
  CHECK(!mcc_identity_accept(&game));
  CHECK(strstr(last_error,"Your local package for Better Creek"));
  /* Item IDs never override content admission in either direction. */
  strcpy(files[0].name,"mcc_maps\\12345\\pat_beavercreek");
  memcpy(files[0].digest,wanted,32);CHECK(mcc_identity_accept(&game));
  memset(files[0].digest,9,32);
  count=1;strcpy(files[0].name,"mcc_maps\\Other\\a10");
  CHECK(!mcc_identity_accept(&game));
  CHECK(strstr(last_error,"No identified local copy") && strstr(last_error,"mcc_maps"));
  memset(game.map.name+64,0,64);
  CHECK(!mcc_identity_accept(&game));
  CHECK(strstr(last_error,"pat_beavercreek.map") && strstr(last_error,"SHA-256"));
  /* Host preparation populates only the outgoing copy. Disconnect and
   * transitions cannot reuse an MCC host identity for another map family. */
  mcc_identity_disconnect();
  memset(&game,0,sizeof(game));game.map.version=123;
  strcpy(game.map.name,"mcc_maps\\2899609719\\pat_beavercreek");
  strcpy(files[0].name,game.map.name);memcpy(files[0].digest,wanted,32);
  CHECK(mcc_identity_prepare_host(game.map.name));
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy)==&copy);
  CHECK(mcc_workshop_wire_unpack(copy.map.name,&item) && !strcmp(item.id,"2899609719"));
  CHECK(!strcmp(item.title,"Pat Beavercreek") && !game.map.name[64]);
  CHECK(!memcmp(copy.mcc_content_sha256,wanted,32));
  mcc_identity_disconnect();CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(mcc_identity_prepare_host(game.map.name));
  memset(&game,0,sizeof(game));strcpy(game.map.name,"custom_maps\\bloodgulch");
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy)==&copy);
  CHECK(!memcmp(&game,&copy,sizeof(game)) && mcc_identity_accept(&copy));
  strcpy(game.map.name,files[0].name);
  CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(mcc_identity_prepare_host(game.map.name));
  memset(&game,0,sizeof(game));strcpy(game.map.name,"levels\\test\\bloodgulch\\bloodgulch");
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy)==&copy);
  CHECK(!memcmp(&game,&copy,sizeof(game)) && mcc_identity_accept(&copy));
 }
 /* An item installed after the cached scan must join on the first retry,
  * even when its canonical path is exactly the host's path. */
 memset(&game,0,sizeof(game));strcpy(game.map.name,"mcc_maps\\2899609719\\pat_beavercreek");
 memcpy(game.mcc_content_sha256,wanted,32);count=0;rescan_install=1;
 CHECK(mcc_identity_accept(&game));CHECK(count==1);
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));
 /* Identical map/resource bytes with different required JSON are different
  * packages. The complete expected pair reaches the download offer. */
 rescan_install=0;mcc_identity_disconnect();memset(files,0,sizeof(files));count=1;
 memset(&game,0,sizeof(game));game.map.version=123;
 strcpy(game.map.name,"mcc_maps\\Host\\a10");memcpy(game.mcc_content_sha256,wanted,32);
 strcpy(files[0].name,"mcc_maps\\Client\\a10");memcpy(files[0].digest,wanted,32);
 memset(game.mcc_metadata_sha256,17,32);memset(files[0].metadata,17,32);
 CHECK(mcc_identity_accept(&game));CHECK(!strcmp(mcc_identity_local_name(game.map.name),files[0].name));
 files[0].metadata[31]^=1;
 CHECK(!mcc_identity_check(game.map.name));
 connection=1;CHECK(!mcc_identity_loaded(game.map.name,NULL));connection=0;
 CHECK(!mcc_identity_accept(&game));CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));
 mcc_identity_set_missing(offer);allow_offer=0;old=offers;
 CHECK(!mcc_identity_accept(&game));CHECK(offers==old+1);
 CHECK(!memcmp(offered_digest,wanted,32) && !memcmp(offered_metadata,game.mcc_metadata_sha256,32));
 allow_offer=1;CHECK(mcc_identity_accept(&game));CHECK(offers==old+2);
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));
 old=offers;CHECK(!mcc_identity_accept_installed(game.map.name,wanted,game.mcc_metadata_sha256,NULL));CHECK(offers==old);
 files[0].metadata[31]^=1;
 CHECK(!mcc_identity_accept_installed(game.map.name,wanted,game.mcc_metadata_sha256,NULL));CHECK(offers==old);
 CHECK(mcc_validation_request(files[0].name,NULL,NULL)==1);
 CHECK(mcc_identity_accept_installed(game.map.name,wanted,game.mcc_metadata_sha256,files[0].name));
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),files[0].name));
 /* A verified installed destination is unambiguous beside matching aliases. */
 files[1]=files[0];strcpy(files[1].name,"mcc_maps\\Duplicate\\a10");count=2;
 CHECK(mcc_identity_accept_installed(game.map.name,wanted,game.mcc_metadata_sha256,files[0].name));
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),files[0].name));
 CHECK(!mcc_identity_accept_installed(game.map.name,wanted,game.mcc_metadata_sha256,"mcc_maps\\Missing\\a10"));
 /* A stale exact path must not mask a fully matching alternate install. */
 files[1]=files[0];count=2;strcpy(files[1].name,game.map.name);files[1].metadata[0]^=1;
 CHECK(mcc_identity_accept(&game));CHECK(!strcmp(mcc_identity_local_name(game.map.name),files[0].name));
 files[0].invalid_metadata=1;allow_offer=0;CHECK(!mcc_identity_accept(&game));
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));
 files[0].invalid_metadata=0;count=1;mcc_identity_set_missing(NULL);
 /* Metadata cannot survive either direction of a map-only transition. */
 memset(files[0].metadata,0,32);CHECK(!mcc_identity_accept(&game));
 memset(game.mcc_metadata_sha256,0,32);CHECK(mcc_identity_accept(&game));
 files[0].metadata[0]=1;CHECK(!mcc_identity_accept(&game));
 /* Host settings carry JSON identity without modifying the source settings. */
 strcpy(game.map.name,files[0].name);memset(game.mcc_content_sha256,0,32);
 CHECK(mcc_identity_prepare_host(game.map.name));
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy)==&copy);
 CHECK(!memcmp(copy.mcc_metadata_sha256,files[0].metadata,32) && !game.mcc_metadata_sha256[0]);
 CHECK(!memcmp(copy.mcc_content_sha256,wanted,32) && !game.mcc_content_sha256[0]);
 {
  unsigned long revision=mcc_identity_host_revision();
  CHECK(revision && mcc_identity_prepare_host(game.map.name) && mcc_identity_host_revision()==revision);
  files[0].metadata[0]^=1;
  CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(!mcc_identity_host_package(game.map.name,wanted));
  CHECK(mcc_identity_prepare_host(game.map.name) && mcc_identity_host_revision()!=revision);
 }
 /* Advisory sidecars are not part of either content digest. Changing their
  * origin still invalidates prepared offers and refreshes the next wire copy. */
 {
  struct mcc_workshop_item item;
  unsigned long revision=mcc_identity_host_revision();
  unsigned char content[32],metadata[32];
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));
  memcpy(content,copy.mcc_content_sha256,32);memcpy(metadata,copy.mcc_metadata_sha256,32);
  old=probes;sidecar="111\nFirst origin\n";
  CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(!mcc_identity_host_package(game.map.name,wanted));
  CHECK(mcc_identity_prepare_host(game.map.name));CHECK(probes==old);
  CHECK(mcc_identity_host_revision()!=revision);revision=mcc_identity_host_revision();
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(mcc_workshop_wire_unpack(copy.map.name,&item) && !strcmp(item.id,"111"));
  CHECK(!memcmp(copy.mcc_content_sha256,content,32) && !memcmp(copy.mcc_metadata_sha256,metadata,32));
  CHECK(mcc_identity_prepare_host(game.map.name));CHECK(mcc_identity_host_revision()==revision && probes==old);
  sidecar="222\nCorrected origin\n";
  CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(!mcc_identity_host_package(game.map.name,wanted));
  CHECK(mcc_identity_prepare_host(game.map.name));CHECK(probes==old && mcc_identity_host_revision()!=revision);
  revision=mcc_identity_host_revision();
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(mcc_workshop_wire_unpack(copy.map.name,&item) && !strcmp(item.id,"222") && !strcmp(item.title,"Corrected origin"));
  CHECK(!memcmp(copy.mcc_content_sha256,content,32) && !memcmp(copy.mcc_metadata_sha256,metadata,32));
  sidecar="222\nCorrected title\n";
  CHECK(mcc_identity_prepare_host(game.map.name));CHECK(probes==old && mcc_identity_host_revision()!=revision);
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));
  CHECK(mcc_workshop_wire_unpack(copy.map.name,&item) && !strcmp(item.title,"Corrected title"));
  revision=mcc_identity_host_revision();sidecar=NULL;
  CHECK(mcc_identity_prepare_host(game.map.name));CHECK(probes==old && mcc_identity_host_revision()!=revision);
  CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy));CHECK(!mcc_workshop_wire_unpack(copy.map.name,&item));
  CHECK(!memcmp(copy.mcc_content_sha256,content,32) && !memcmp(copy.mcc_metadata_sha256,metadata,32));
 }
 files[0].invalid_metadata=1;CHECK(!mcc_identity_prepare_host(game.map.name));
 CHECK(!mcc_identity_host_revision());
 CHECK(!mcc_identity_host_settings(&game,sizeof(game),&copy));

 /* Packet admission returns before hashing; no alias or load permission exists
  * before ACK and completion. Repeated identical settings retain the reader. */
 mcc_identity_disconnect();memset(&cached,0,sizeof(cached));memset(files,0,sizeof(files));count=1;
 memset(&game,0,sizeof(game));strcpy(game.map.name,"mcc_maps\\Host\\a10");
 strcpy(files[0].name,"mcc_maps\\Client\\a10");memcpy(files[0].digest,wanted,32);
 memcpy(game.mcc_content_sha256,wanted,32);lease_wait=1;old=probes;
 CHECK((mcc_identity_accept)(&game));CHECK(mcc_identity_pending(game.map.name));
 CHECK(probes==old && !open_checks && !mcc_identity_check(game.map.name));
 mcc_identity_update();CHECK(probes==old && !open_checks);
 i=lease_begins;CHECK((mcc_identity_accept)(&game));CHECK(lease_begins==i);
 lease_wait=0;mcc_identity_update();CHECK(open_checks==1 && probes==old);
 CHECK((mcc_identity_accept)(&game));CHECK(open_checks==1 && lease_begins==i);
 /* An advisory origin change cancels the precise old reader/lease. */
 game.map.name[64]=1;CHECK((mcc_identity_accept)(&game));CHECK(!open_checks && lease_begins==i+1);
 mcc_identity_update();CHECK(open_checks==1);
 mcc_identity_disconnect();CHECK(!open_checks && !mcc_identity_pending(NULL));
 /* Expired admission refuses without attempting any file reads. */
 CHECK((mcc_identity_accept)(&game));lease=-1;i=aborts;old=probes;mcc_identity_update();
 CHECK(aborts==i+1 && probes==old && !open_checks && !mcc_identity_pending(NULL));

 /* A failed final READY send keeps the local load guard, and expiry must
  * discard even an already-selected alias instead of allowing admission. */
 lease=0;ready_blocked=1;CHECK((mcc_identity_accept)(&game));
 for(i=0;i<100 && strcmp(mcc_identity_local_name(game.map.name),files[0].name);i++)mcc_identity_update();
 CHECK(i<100 && mcc_identity_pending(NULL) && !mcc_identity_check(game.map.name));
 lease=-1;i=aborts;mcc_identity_update();CHECK(aborts==i+1 && !mcc_identity_pending(NULL));
 CHECK(!strcmp(mcc_identity_local_name(game.map.name),game.map.name));ready_blocked=0;
 /* Legacy admission rejects either unexpected tail, and legacy transmission
  * clears both tails while preserving every byte of the established prefix. */
 strcpy(game.map.name,"custom_maps\\bloodgulch");game.mcc_metadata_sha256[31]=1;
 CHECK(!mcc_identity_accept(&game));
 CHECK(mcc_identity_host_settings(&game,sizeof(game),&copy)==&copy);
 CHECK(!memcmp(&copy,&game,offsetof(struct network_game,mcc_content_sha256)));
 CHECK(!copy.mcc_content_sha256[0] && !copy.mcc_metadata_sha256[31] && mcc_identity_accept(&copy));
 return 0;
}
''')
    exe = compile_tool(tmp_path, [source, ROOT / "port/linux/game/mcc_identity.c",
                                  ROOT / "port/linux/game/mcc_package.c",
                                  ROOT / "port/linux/game/mcc_workshop.c"])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


def test_fingerprint_settings_keep_host_and_fragment_checks(tmp_path):
    native = (ROOT / "source/networking/network_client_message_handler.c").read_text()
    (tmp_path / "cseries.h").write_text(
        "#include <stddef.h>\ntypedef int boolean;\ntypedef unsigned char byte;\n")
    source = tmp_path / "settings.c"
    source.write_text('#include "'+(ROOT / "port/linux/include/halo_port_limits.h").as_posix()+'"\n'+r'''
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "mcc_workshop.h"
typedef int boolean;typedef unsigned char byte;typedef uint16_t word;
int mcc_maps_workshop_title(char const *id,char *title,unsigned size) {
 (void)id;(void)title;(void)size;return 0;
}
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
struct network_game {union {byte previous[1030];struct {char name[128];} map;};byte mcc_content_sha256[32];byte mcc_metadata_sha256[32];};
struct network_game_client {int dummy;};struct transport_address {int host;};
static int applies,decodes,state=3;static struct network_game received;
static struct network_game network_game_client_settings_staging;
static long network_game_client_settings_staging_size,network_game_client_settings_staging_total;
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
 struct network_game original;struct mcc_workshop_item item,back;unsigned offset,n;
 memset(&original,42,sizeof(original));
 memset(original.mcc_metadata_sha256,17,sizeof(original.mcc_metadata_sha256));
 strcpy((char *)original.previous,"mcc_maps\\2899609719\\pat_beavercreek");
 strcpy(item.id,"2899609719");strcpy(item.title,"Better Creek");
 mcc_workshop_wire_pack((char *)original.previous,&item);
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
 CHECK(mcc_workshop_wire_unpack((char const *)received.previous,&back));
 CHECK(!strcmp(back.id,item.id) && !strcmp(back.title,item.title));
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
    exe = compile_tool(tmp_path, [source, ROOT / "port/linux/game/mcc_workshop.c",
                                 ROOT / "port/linux/game/mcc_package.c"])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


def test_settings_origin_change_restarts_only_mcc_admission(tmp_path):
    native = (ROOT / "source/networking/network_client_manager.c").read_text()
    source = tmp_path / "settings.c"
    source.write_text(r'''
#include <string.h>
#include <stdio.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define MAXIMUM_NETWORK_MACHINE_COUNT 128
#define MAXIMUM_NUMBER_OF_PLAYERS 128
#define NUMBER_OF_GAME_DIFFICULTY_LEVELS 4
#define VALID_INDEX(i,n) ((unsigned)(i)<(unsigned)(n))
#define match_assert(...) ((void)0)
#define network_event(...) ((void)0)
#define csstrcmp strcmp
#define csmemcpy memcpy
#define _error_network_failed_to_join_game 1
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct network_game {int machine_count,player_count,maximum_players,cooperative_flags,difficulty;
 struct {char name[128];long version;} map;unsigned char mcc_content_sha256[32],mcc_metadata_sha256[32];int local_data;};
struct network_game_client {struct network_game game;};
static int admits,precache,present;
static int third_person_match_flags_valid(int flags){return flags==0;}
static int network_game_client_map_name_is_valid(char const *name,unsigned size){return memchr(name,0,size)!=NULL;}
static int network_game_client_game_record_is_valid(struct network_game *game){(void)game;return 1;}
static int mcc_level_name(char const *name){return !strncmp(name,"mcc_maps\\",9);}
static int mcc_identity_accept(struct network_game *game){(void)game;admits++;return 1;}
static int network_game_is_splitscreen_local(void){return 0;}
static int cache_files_map_present(char const *name,unsigned long version){(void)name;(void)version;present++;return 1;}
static int cache_files_map_plays_multiplayer(char const *name,char *build){(void)name;(void)build;return 1;}
static void cache_files_show_multiplayer_unavailable(char const *name,char const *build){(void)name;(void)build;}
static void display_error_when_main_menu_loaded(int error){(void)error;}
static void main_set_multiplayer_map_name(char const *name){(void)name;precache++;}
''' + function(native, "network_game_client_game_settings_updated") + r'''
int main(void){
 struct network_game_client client;struct network_game next;
 memset(&client,0,sizeof(client));client.game.maximum_players=4;client.game.local_data=123;
 strcpy(client.game.map.name,"mcc_maps\\Host\\a10");next=client.game;
 CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(!admits && !present && !precache);
 next.map.name[64]=7;next.local_data=99;
 CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(admits==1 && present==1 && precache==1);
 CHECK(client.game.local_data==123 && client.game.map.name[64]==7);
 CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(admits==1);
 next.mcc_metadata_sha256[31]=1;CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(admits==2);
 strcpy(next.map.name,"custom_maps\\a10");CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(admits==3);
 next.map.name[64]++;CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(admits==3);
 strcpy(next.map.name,"levels\\test\\bloodgulch\\bloodgulch");CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(admits==4);
 next.map.name[64]++;CHECK(network_game_client_game_settings_updated(&client,&next));CHECK(admits==4);
 return 0;
}
''')
    exe = compile_tool(tmp_path, [source])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
