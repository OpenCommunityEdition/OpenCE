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
#include "cseries.h"
#include "mcc_workshop.h"
#include <stdio.h>
#include <string.h>
static char steam_title[64];
boolean mcc_maps_workshop_title(char const *id,char *title,unsigned size){
 if(strcmp(id,"2899609719") || !steam_title[0])return 0;
 snprintf(title,size,"%s",steam_title);return 1;
}
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
 /* a package folder named by an item id is that item; its title is Steam's when this machine has the item */
 CHECK(mcc_workshop_local_item("mcc_maps\\2899609719\\pat_beavercreek",&item) && !strcmp(item.id,"2899609719"));
 CHECK(!strcmp(item.title,"Pat Beavercreek"));
 strcpy(steam_title,"Pro Edition \xc3\xa9 of Halo");
 CHECK(mcc_workshop_local_item("mcc_maps\\2899609719\\pat_beavercreek",&item) && !strcmp(item.title,"Pro Edition  of Halo"));
 steam_title[0]=0;
 CHECK(!mcc_workshop_local_item("mcc_maps\\notitem\\pat_beavercreek",&item));
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
 CHECK(!strcmp(text,"No identified local copy of Better Creek (Steam Workshop item 2899609719). "
     "Download the host's version and copy its map and required resources into mcc_maps."));
 mcc_workshop_refusal_text(MCC_WORKSHOP_DIFFERS,&item,"pat_beavercreek",text,sizeof(text));
 CHECK(!strcmp(text,"Your local package for Better Creek (Steam Workshop item 2899609719) does not match the host. "
     "Check versions and copy the matching map and required resources into mcc_maps."));
 item.title[0]=0;
 mcc_workshop_refusal_text(MCC_WORKSHOP_MISSING,&item,"pat_beavercreek",text,sizeof(text));
 CHECK(strstr(text,"No identified local copy of Pat Beavercreek"));
 mcc_workshop_refusal_text(MCC_WORKSHOP_NOT_WORKSHOP,&item,"pat_beavercreek",text,sizeof(text));
 CHECK(strstr(text,"pat_beavercreek.map") && strstr(text,"SHA-256"));
 /* Long metadata still leaves the actionable installation instruction. */
 memset(item.id,'7',20);item.id[20]=0;memset(item.title,'T',39);item.title[39]=0;
 mcc_workshop_refusal_text(MCC_WORKSHOP_DIFFERS,&item,"level",text,sizeof(text));
 CHECK(strstr(text,"resources into mcc_maps.") && strlen(text)<sizeof(text)-1);
 return 0;
}
''')
    game = ROOT / "port/linux/game"
    exe = compile_tool(tmp_path, [source, game / "mcc_workshop.c", game / "mcc_package.c"])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


def test_workshop_sidecar_files(tmp_path):
    """Read real files; replace only the platform's Xbox-path translation."""
    (tmp_path / "cseries.h").write_text(
        "#include <stddef.h>\ntypedef int boolean;\ntypedef unsigned char byte;\n")
    (tmp_path / "stdio.h").write_text(
        '#include_next <stdio.h>\nFILE *workshop_test_fopen(char const *,char const *);\n'
        '#define fopen workshop_test_fopen\n')
    for folder, contents in {
        "Named": b"2899609719\r\nBetter Creek\r\n",
        "12345": b"67890\nSidecar wins\n",
        "Bad": b"not-an-id\n",
        "42": b"not-an-id\n",
        "TitleOnly": b"\nNot an ID\n",
        "Long": b"7\n" + b"T" * 300,
        "IdOnly": b"2899609719\n",
    }.items():
        package = tmp_path / "mcc_maps" / folder
        package.mkdir(parents=True)
        (package / "workshop_id.txt").write_bytes(contents)
    source = tmp_path / "sidecar.c"
    source.write_text(r'''
#include "mcc_workshop.h"
#include <stdio.h>
#include <string.h>
#undef fopen
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
FILE *workshop_test_fopen(char const *path,char const *mode) {
 char local[160];unsigned i;
 if(strncmp(path,"d:\\mcc_maps\\",12))return NULL;
 for(i=3;path[i] && i-3<sizeof(local)-1;i++) {
  local[i-3]=path[i]=='\\' ? '/' : path[i];
 }
 local[i-3]=0;
 return fopen(local,mode);
}
int mcc_maps_workshop_title(char const *id,char *title,unsigned size) {
 (void)id;(void)title;(void)size;return 0;
}
int main(void) {
 struct mcc_workshop_item item;
 CHECK(mcc_workshop_local_item("mcc_maps\\Named\\level",&item));
 CHECK(!strcmp(item.id,"2899609719") && !strcmp(item.title,"Better Creek"));
 CHECK(mcc_workshop_local_item("mcc_maps\\12345\\level",&item));
 CHECK(!strcmp(item.id,"67890") && !strcmp(item.title,"Sidecar wins"));
 CHECK(mcc_workshop_local_item("mcc_maps\\42\\old_level",&item));
 CHECK(!strcmp(item.id,"42") && !strcmp(item.title,"Old Level"));
 CHECK(!mcc_workshop_local_item("mcc_maps\\Bad\\level",&item) && !item.id[0]);
 CHECK(!mcc_workshop_local_item("mcc_maps\\TitleOnly\\level",&item) && !item.id[0]);
 CHECK(!mcc_workshop_local_item("mcc_maps\\Absent\\level",&item) && !item.id[0]);
 CHECK(!mcc_workshop_local_item("mcc_maps\\level",&item));
 CHECK(!mcc_workshop_local_item("custom_maps\\Named\\level",&item));
 CHECK(mcc_workshop_local_item("mcc_maps\\Long\\level",&item));
 CHECK(!strcmp(item.id,"7") && strlen(item.title)==39);
 CHECK(mcc_workshop_local_item("mcc_maps\\IdOnly\\level",&item));
 CHECK(!strcmp(item.id,"2899609719") && !item.title[0]);
 return 0;
}
''')
    game = ROOT / "port/linux/game"
    exe = compile_tool(tmp_path, [source, game / "mcc_workshop.c", game / "mcc_package.c"])
    result = subprocess.run([str(exe)], cwd=tmp_path, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
