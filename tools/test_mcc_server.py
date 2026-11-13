"""Dedicated MCC resolution uses its own real header reader and namespace."""
from pathlib import Path
import subprocess
from mcc_test_support import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def test_mcc_rotation_and_catalog(tmp_path):
    (tmp_path / "cseries.h").write_text(
        "#ifndef MCC_SERVER_TEST_CSERIES_H\n#define MCC_SERVER_TEST_CSERIES_H\n"
        "#include <stddef.h>\ntypedef unsigned char boolean;\n#define TRUE 1\n#define FALSE 0\n#endif\n")
    source = tmp_path / "server.c"
    source.write_text(r'''
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cseries.h"
#include "mcc_server.h"
#include "mcc_maps.h"
#include "server_rotation.h"
static unsigned char header[0x828];
static char requested[128];
static int rescans,listed;
static FILE *fixture_fopen(char const *name,char const *mode) {
    snprintf(requested,sizeof(requested),"%s",name);
    return fopen("fixture.map",mode);
}
#define fopen fixture_fopen
#include "SERVER_SOURCE"
#undef fopen
void mcc_maps_rescan(void) {rescans++;}
short mcc_maps_count(boolean multiplayer) {assert(!multiplayer);return 2;}
short mcc_maps_index(short row,boolean multiplayer) {assert(row<2&&!multiplayer);return row;}
char const *mcc_maps_level_name(short index) {return index==0 ? "mcc_maps\\123\\arena" : "mcc_maps\\123\\mission";}
static void each(void *user,char const *name,char const *level,char const *kind) {
    (void)user;assert(!strcmp(name,listed%2 ? "123\\mission" : "123\\arena"));assert(!strcmp(level+9,name));
    assert(!strcmp(kind,"mcc"));listed++;
}
static void save(unsigned version,unsigned type) {
    FILE *f;
    memset(header,0,sizeof(header));memcpy(header,"daeh",4);memcpy(header+0x7fc,"toof",4);
    header[4]=(unsigned char)version;header[5]=(unsigned char)(version>>8);
    header[0x11]=8;header[0x14]=0x28;header[0x60]=(unsigned char)type;
    f=fopen("fixture.map","wb");assert(f);assert(fwrite(header,1,sizeof(header),f)==sizeof(header));fclose(f);
}
static struct server_entry parse(char const *text) {
    struct server_entry entry;
    assert(server_rotation_parse_entry(text,(int)strlen(text),&entry));return entry;
}
int main(void) {
    struct server_entry e;
    save(13,0);e=parse("MCC_MAPS/Workshop Package/mission:coop");
    assert(e.map_kind==_server_map_mcc&&!e.rejected[0]);mcc_server_resolve(&e);
    assert(!e.rejected[0]&&!strcmp(e.level,"mcc_maps\\Workshop Package\\mission"));
    assert(!strcmp(requested,"d:\\mcc_maps\\Workshop Package\\mission.map"));
    e=parse("mcc_maps/a10:slayer");mcc_server_resolve(&e);assert(e.rejected[0]);
    save(13,1);e=parse("mcc_maps/123/arena:slayer");mcc_server_resolve(&e);assert(!e.rejected[0]);
    e=parse("mcc_maps/123/arena:coop");mcc_server_resolve(&e);assert(e.rejected[0]);
    save(5,1);e=parse("mcc_maps/123/arena:slayer");mcc_server_resolve(&e);assert(e.rejected[0]);
    save(609,1);e=parse("mcc_maps/123/arena:slayer");mcc_server_resolve(&e);assert(e.rejected[0]);
    save(13,1);header[0x7fc]=0;
    {FILE *f=fopen("fixture.map","wb");assert(f);fwrite(header,1,sizeof(header),f);fclose(f);}
    e=parse("mcc_maps/123/arena:slayer");mcc_server_resolve(&e);assert(e.rejected[0]);
    requested[0]=0;e=parse("mcc_maps/../arena:slayer");mcc_server_resolve(&e);
    assert(e.rejected[0]&&!requested[0]);
    e=parse("bloodgulch:ctf");assert(e.map_kind==_server_map_name&&!e.rejected[0]);
    e=parse("custom_maps/arena:slayer");assert(e.map_kind==_server_map_custom&&!e.rejected[0]);
    e=parse("levels/a10/a10:coop");assert(e.map_kind==_server_map_path&&!e.rejected[0]);
    assert(mcc_server_maps_each(each,NULL)==2&&rescans==0&&listed==2);
    assert(mcc_server_maps_each(each,NULL)==2&&rescans==0&&listed==4);
    mcc_server_request_rescan();mcc_server_request_rescan();
    mcc_server_update(0);assert(rescans==0);
    assert(mcc_server_maps_each(each,NULL)==2 && rescans==0);
    mcc_server_update(1);assert(rescans==1);
    mcc_server_update(1);assert(rescans==1);
    mcc_server_request_rescan();mcc_server_update(1);assert(rescans==2);
    return 0;
}
'''.replace("SERVER_SOURCE", (ROOT / "port/linux/game/mcc_server.c").as_posix()))
    # The rotation grammar has no file dependencies; resolution is exercised
    # through the MCC adapter above, without stubbing its header validation.
    rotation = tmp_path / "rotation.c"
    rotation.write_text('#define SERVER_ROTATION_NO_FILES\n' +
                        (ROOT / "port/linux/src/server_rotation.c").read_text())
    (tmp_path / "server_rotation.h").write_text((ROOT / "port/linux/src/server_rotation.h").read_text())
    exe = compile_tool(tmp_path, [source, rotation, ROOT / "port/linux/game/mcc_package.c",
                                  ROOT / "port/linux/game/mcc_cache_format.c"])
    subprocess.run([str(exe)], cwd=tmp_path, check=True, capture_output=True)
