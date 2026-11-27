"""The real MCC pause builder exposes keyboard identities without editing tags."""
from pathlib import Path
from harness import structure
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_mcc_keyboard_footer_ownership_and_lifecycle(tmp_path):
    (tmp_path/'tag_files').mkdir()
    (tmp_path/'text').mkdir()
    groups=(ROOT/'source/tag_files/tag_groups.h').read_text()
    (tmp_path/'tag_files/tag_groups.h').write_text('\n'.join(structure(groups,n) for n in ('tag_block','tag_reference','tag_data')))
    text=(ROOT/'source/text/text_group.h').read_text()
    (tmp_path/'text/text_group.h').write_text('\n'.join(structure(text,n) for n in ('string_list','string_list_entry')))
    (tmp_path/'cseries.h').write_text('''
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
typedef unsigned char boolean; typedef unsigned short word;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAX(a,b) ((a)>(b)?(a):(b))
''')
    # Include the complete production builder, retaining its ABI and ownership checks.
    source=(ROOT/'port/linux/game/mcc_pause.c').read_text()
    for header in ('mcc_pause.h','mcc_ui.h'):
        (tmp_path/header).write_text((ROOT/'port/linux/game'/header).read_text())
    run_c(tmp_path,source+r'''
#include <assert.h>
static long art(short w,short h,enum mcc_pause_art_kind kind){return 1000+kind;}
int main(void){
 struct mcc_pause_assets assets={{100,101,102},{200,201,202},art,TRUE};
 long i,parent=NONE,old_parent,old_child;int count=0;
 assert(mcc_pause_build(100,200,&assets));
 for(i=0;i<mcc_pause_tag_count();i++){
  struct mcc_pause_tag const *tag=mcc_pause_tag_at(i);
  struct mcc_pause_widget *key=mcc_pause_definition(tag->handle);
  struct mcc_pause_child *children;
  short widths[]={30,38,42,48},k;
  if(!key||strcmp(key->name,"button_key"))continue;
  parent=tag->handle;children=key->children.address;count++;
  assert(key->children.count==4);
  assert(mcc_pause_key_button(parent,children[0].widget.index)==1);
  assert(mcc_pause_key_button(parent,children[2].widget.index)==0);
  assert(mcc_pause_key_button(parent,children[1].widget.index)==NONE);
  assert(mcc_pause_key_button(parent,children[3].widget.index)==NONE);
  for(k=0;k<4;k++){
   short gap=children[2].x-children[1].x-63-8;
   short a=mcc_pause_key_offset(parent,children[2].widget.index,widths[k],30);
   short b=mcc_pause_key_offset(parent,children[0].widget.index,widths[k],30);
   assert(a==mcc_pause_key_offset(parent,children[3].widget.index,widths[k],30));
   assert(b==mcc_pause_key_offset(parent,children[1].widget.index,widths[k],30));
   /* Preserve the original visible center and the gap before the accept cap. */
   assert(a-b==widths[k]-16-gap);
   assert(abs((b-(30-16))+a)<=1);
   assert(children[2].x+16+a-widths[k]>=children[1].x+63+b+8);
  }
  assert(!mcc_pause_key_offset(parent,children[2].widget.index,0,30));
  assert(!mcc_pause_key_offset(parent,NONE,42,30));
  assert(mcc_pause_key_button(NONE,children[0].widget.index)==NONE);
  assert(!mcc_pause_key_offset(NONE,children[0].widget.index,42,30));
 }
 assert(count>=3&&parent!=NONE);
 old_parent=parent;old_child=((struct mcc_pause_child*)mcc_pause_definition(parent)->children.address)[0].widget.index;
 mcc_pause_dispose();
 assert(mcc_pause_key_button(old_parent,old_child)==NONE);
 assert(!mcc_pause_key_offset(old_parent,old_child,42,30));
 assert(mcc_pause_build(100,700,&assets));
 assert(mcc_pause_key_button(old_parent,old_child)==NONE);
 assert(!mcc_pause_key_offset(old_parent,old_child,42,30));
 mcc_pause_dispose();return 0;
}
''')
