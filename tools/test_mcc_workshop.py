"""The Workshop item id and title travel in the map name's unused bytes; joiners are told which item."""
from pathlib import Path
import subprocess
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def test_workshop_identity(tmp_path):
    (tmp_path / "cseries.h").write_text(
        "#include <stddef.h>\ntypedef int boolean;\ntypedef unsigned char byte;\n")
    source = tmp_path / "test.c"
    source.write_text(r'''
#include "mcc_workshop.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
 struct mcc_workshop_item item,back;char name[128],text[256],folder[64],title[MCC_WORKSHOP_TITLE_SIZE];
 /* the sidecar: id, optional title; nothing else is an id */
 CHECK(mcc_workshop_parse_sidecar("2899609719\r\nBetter Creek\r\n",&item));
 CHECK(!strcmp(item.id,"2899609719") && !strcmp(item.title,"Better Creek"));
 CHECK(mcc_workshop_parse_sidecar("42",&item) && !item.title[0]);
 CHECK(!mcc_workshop_parse_sidecar("",&item));
 CHECK(!mcc_workshop_parse_sidecar("12ab",&item));
 CHECK(!mcc_workshop_parse_sidecar("123456789012345678901",&item));
 CHECK(!mcc_workshop_parse_sidecar("..\\evil",&item));
 /* package folders */
 CHECK(mcc_workshop_package_folder("mcc_maps\\2899609719\\pat_beavercreek",folder) && !strcmp(folder,"2899609719"));
 CHECK(!mcc_workshop_package_folder("mcc_maps\\a10",folder));
 CHECK(!mcc_workshop_package_folder("maps\\a\\b",folder));
 mcc_workshop_title_from_level("mcc_maps\\x\\pat_beavercreek",title);
 CHECK(!strcmp(title,"Pat Beavercreek"));
 /* the wire: a name's first 64 bytes are untouched, older builds read the same name */
 memset(name,0,sizeof(name));strcpy(name,"mcc_maps\\2899609719\\pat_beavercreek");
 strcpy(item.id,"2899609719");strcpy(item.title,"Better Creek");
 mcc_workshop_wire_pack(name,&item);
 CHECK(!strcmp(name,"mcc_maps\\2899609719\\pat_beavercreek"));
 CHECK(mcc_workshop_wire_unpack(name,&back) && !strcmp(back.id,item.id) && !strcmp(back.title,item.title));
 /* longest id and title still fit and end in the field */
 memset(item.id,'7',20);item.id[20]=0;memset(item.title,'T',39);item.title[39]=0;
 mcc_workshop_wire_pack(name,&item);
 CHECK(mcc_workshop_wire_unpack(name,&back) && !strcmp(back.id,item.id) && strlen(back.title)==39);
 /* no id: the tail is cleared, and unpack finds nothing */
 memset(&item,0,sizeof(item));memset(name+64,0x55,64);
 mcc_workshop_wire_pack(name,&item);
 CHECK(!name[64] && !mcc_workshop_wire_unpack(name,&back));
 /* hostile tails are ignored */
 memset(name+64,0x41,64);CHECK(!mcc_workshop_wire_unpack(name,&back));
 memcpy(name+64,"WS\1" "12x",7);memset(name+71,0,57);CHECK(!mcc_workshop_wire_unpack(name,&back));
 memset(name+64,0,64);memcpy(name+64,"WS\1",3);memset(name+67,'1',61);CHECK(!mcc_workshop_wire_unpack(name,&back));
 /* the texts */
 strcpy(item.id,"2899609719");strcpy(item.title,"Better Creek");
 mcc_workshop_refusal_text(MCC_WORKSHOP_MISSING,&item,"pat_beavercreek",text,sizeof(text));
 CHECK(!strcmp(text,"Subscribe to Better Creek on the Steam Workshop (item 2899609719) to join."));
 mcc_workshop_refusal_text(MCC_WORKSHOP_DIFFERS,&item,"pat_beavercreek",text,sizeof(text));
 CHECK(!strcmp(text,"Your copy of Better Creek differs from the host's: let Steam update Workshop item 2899609719, then join again."));
 item.title[0]=0;
 mcc_workshop_refusal_text(MCC_WORKSHOP_MISSING,&item,"pat_beavercreek",text,sizeof(text));
 CHECK(strstr(text,"Subscribe to Pat Beavercreek on"));
 mcc_workshop_refusal_text(MCC_WORKSHOP_NOT_WORKSHOP,&item,"pat_beavercreek",text,sizeof(text));
 CHECK(strstr(text,"pat_beavercreek.map") && strstr(text,"SHA-256"));
 return 0;
}
''')
    game = ROOT / "port/linux/game"
    exe = compile_tool(tmp_path, [source, game / "mcc_workshop.c", game / "mcc_package.c"])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
