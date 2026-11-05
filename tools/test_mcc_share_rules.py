"""Which MCC packages a host may offer: hand-placed only, never stock, Workshop or unknown."""
from pathlib import Path
import subprocess
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def test_share_rules(tmp_path):
    source = tmp_path / "test.c"
    source.write_text(r'''
#include "mcc_share_rules.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static enum mcc_share_reason why(char const *name,enum mcc_share_sidecar sidecar,int known,int opt){
 struct mcc_share_package p;p.name=name;p.sidecar=sidecar;p.content_known=known;p.host_opt_in=opt;
 return mcc_share_decide(&p);
}
#define ABSENT MCC_SHARE_SIDECAR_ABSENT
int main(void) {
 struct mcc_share_package p;
 /* a hand-placed, non-numeric package with known content is offered */
 CHECK(why("mcc_maps\\solar_flare\\solar_flare",ABSENT,1,1)==MCC_SHARE_ELIGIBLE);
 CHECK(why("MCC_MAPS/Solar_Flare/solar_flare",ABSENT,1,1)==MCC_SHARE_ELIGIBLE);
 p.name="mcc_maps\\solar_flare\\solar_flare";p.sidecar=ABSENT;p.content_known=1;p.host_opt_in=1;
 CHECK(mcc_share_eligible(&p));p.host_opt_in=0;CHECK(!mcc_share_eligible(&p));
 CHECK(!mcc_share_eligible(NULL));
 /* sharing is opt-in */
 CHECK(why("mcc_maps\\solar_flare\\solar_flare",ABSENT,1,0)==MCC_SHARE_NOT_OPTED_IN);
 /* Workshop items: numeric folder, or a valid sidecar in a named folder */
 CHECK(why("mcc_maps\\2899609719\\pat_beavercreek",ABSENT,1,1)==MCC_SHARE_WORKSHOP);
 CHECK(why("mcc_maps\\solar_flare\\solar_flare",MCC_SHARE_SIDECAR_VALID,1,1)==MCC_SHARE_WORKSHOP);
 /* an unreadable sidecar means the origin is unknown */
 CHECK(why("mcc_maps\\solar_flare\\solar_flare",MCC_SHARE_SIDECAR_UNREADABLE,1,1)==MCC_SHARE_UNKNOWN_ORIGIN);
 /* no content identity */
 CHECK(why("mcc_maps\\solar_flare\\solar_flare",ABSENT,0,1)==MCC_SHARE_UNKNOWN_CONTENT);
 /* stock maps, loose maps and shared resources */
 CHECK(why("mcc_maps\\bloodgulch\\bloodgulch",ABSENT,1,1)==MCC_SHARE_STOCK);
 CHECK(why("mcc_maps\\BloodGulch\\x",ABSENT,1,1)==MCC_SHARE_STOCK);
 CHECK(why("mcc_maps\\a10",ABSENT,1,1)==MCC_SHARE_NOT_A_PACKAGE);
 CHECK(why("mcc_maps\\solar_flare",ABSENT,1,1)==MCC_SHARE_NOT_A_PACKAGE);
 CHECK(why("mcc_maps\\_shared\\multiplayer",ABSENT,1,1)==MCC_SHARE_SHARED_RESOURCES);
 CHECK(why("mcc_maps\\_SHARED\\x",ABSENT,1,1)==MCC_SHARE_SHARED_RESOURCES);
 /* unsafe or foreign identities */
 CHECK(why("maps\\solar_flare\\x",ABSENT,1,1)==MCC_SHARE_BAD_NAME);
 CHECK(why("mcc_maps\\..\\x\\y",ABSENT,1,1)==MCC_SHARE_BAD_NAME);
 CHECK(why("mcc_maps\\a\\b\\c",ABSENT,1,1)==MCC_SHARE_BAD_NAME);
 CHECK(why("mcc_maps\\a:b\\c",ABSENT,1,1)==MCC_SHARE_BAD_NAME);
 CHECK(why("mcc_maps\\\\c",ABSENT,1,1)==MCC_SHARE_BAD_NAME);
 CHECK(why("",ABSENT,1,1)==MCC_SHARE_BAD_NAME);
 CHECK(why(NULL,ABSENT,1,1)==MCC_SHARE_BAD_NAME);
 /* reason words are distinct */
 CHECK(!strcmp(mcc_share_reason_word(MCC_SHARE_WORKSHOP),"workshop"));
 CHECK(!strcmp(mcc_share_reason_word(MCC_SHARE_STOCK),"stock"));
 /* the files that go with a package */
 CHECK(mcc_share_file_allowed("bitmaps.map") && mcc_share_file_allowed("SOUNDS.MAP"));
 CHECK(!mcc_share_file_allowed("..\\bitmaps.map") && !mcc_share_file_allowed("x/sounds.map"));
 CHECK(!mcc_share_file_allowed("loc.map") && !mcc_share_file_allowed("") && !mcc_share_file_allowed(NULL));
 return 0;
}
''')
    exe = compile_tool(tmp_path, [source, ROOT / "port/linux/game/mcc_share_rules.c",
                                  ROOT / "port/linux/game/mcc_package.c"])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
