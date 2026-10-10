"""Read generated MCC pause tags through the actual native widget ABI.

The builder runs intact with synthetic font/art handles. These tests require
no proprietary map assets and never enter the Xbox or Custom Edition loaders.
"""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def pause_tool(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for the MCC pause builder tests")
    work = tmp_path_factory.mktemp("mcc-pause")
    native = (ROOT / "source/interface/ui_widget.c").read_text()
    wrap_hook_start = native.index("\n\t{\n\t\textern boolean mcc_pause_text_wrap_needed(long);")
    wrap_hook_end = native.index("\n\tif (string_has_icons_to_draw(*text))", wrap_hook_start)
    wrap_hook = native[wrap_hook_start:wrap_hook_end]
    tag_groups = (ROOT / "source/tag_files/tag_groups.h").read_text()
    native_types = "\n".join(structure(tag_groups, name) for name in [
        "tag_block", "tag_reference", "tag_data"])
    native_types += "\n" + "\n".join(structure(native, name) for name in [
        "ui_widget_event_handler_reference", "ui_widget_child_reference",
        "ui_widget_conditional_reference", "ui_widget_game_data_input_reference",
        "ui_widget_search_and_replace_reference", "ui_widget_definition"])
    (work / "tag_files").mkdir()
    (work / "text").mkdir()
    (work / "tag_files/tag_groups.h").write_text("/* Native tag structures already included by harness. */\n")
    text_groups = (ROOT / "source/text/text_group.h").read_text()
    (work / "text/text_group.h").write_text("\n".join(structure(text_groups, name) for name in [
        "string_list", "string_list_entry"]))
    (work / "cseries.h").write_text(r'''
#ifndef MCC_PAUSE_TEST_CSERIES
#define MCC_PAUSE_TEST_CSERIES
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef unsigned char boolean,byte;
typedef unsigned short word;
typedef struct {short y0,x0,y1,x1;} rectangle2d;
typedef struct {float alpha,red,green,blue;} real_argb_color;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define FLAG(bit) (1u<<(bit))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define csstrnzcpy(d,s,n) do {strncpy(d,s,n);(d)[(n)-1]=0;} while(0)
#endif
''')
    source = r'''
#include "cseries.h"
#include "mcc_pause.h"
#include "mcc_ui.h"
''' + native_types + r'''
typedef char widget_size_must_match_native[sizeof(struct ui_widget_definition)==0x3ec?1:-1];
typedef char child_size_must_match_native[sizeof(struct ui_widget_child_reference)==0x50?1:-1];
typedef char event_size_must_match_native[sizeof(struct ui_widget_event_handler_reference)==0x48?1:-1];
#define CHECK(c) do {if(!(c)){fprintf(stderr,"line %d: %s\n",__LINE__,#c);return 1;}}while(0)
static unsigned allocation_calls,live_allocations,fail_allocation;
static void *test_malloc(size_t bytes) {
    void *p;
    if(++allocation_calls==fail_allocation)return NULL;
    p=malloc(bytes);if(p)live_allocations++;return p;
}
static void *test_calloc(size_t count,size_t bytes) {
    void *p=test_malloc(count*bytes);if(p)memset(p,0,count*bytes);return p;
}
static void test_free(void *p) {if(p){assert(live_allocations);live_allocations--;free(p);}}
#define malloc test_malloc
#define calloc test_calloc
#define free test_free
#include "mcc_pause.c"
#undef malloc
#undef calloc
#undef free
static struct {short width,height;enum mcc_pause_art_kind kind;long handle;} art_records[64];
static unsigned art_count,art_calls,fail_art;
static long art(short width,short height,enum mcc_pause_art_kind kind) {
    unsigned i;
    if(++art_calls==fail_art)return NONE;
    assert(width>0&&height>0&&width<=640&&height<=480);
    assert(kind>=MCC_PAUSE_ART_PANEL&&kind<=MCC_PAUSE_ART_DIM);
    for(i=0;i<art_count;i++)if(art_records[i].width==width&&art_records[i].height==height&&
        art_records[i].kind==kind)return art_records[i].handle;
    assert(art_count<NUMBEROF(art_records));
    art_records[art_count].width=width;art_records[art_count].height=height;
    art_records[art_count].kind=kind;art_records[art_count].handle=0x20000000+(long)art_count;
    return art_records[art_count++].handle;
}
static struct mcc_pause_assets assets={{100,101,102},{200,201,202},art,TRUE};
/* Variable-width metrics stand in for an asset-provided font. The wrapping
 * implementation and its ownership/clip hook are the actual native code. */
#define csmemcpy memcpy
static void draw_unicode_string_compute_bounds(rectangle2d const *bounds,wchar_t const *text,
    rectangle2d *text_bounds,rectangle2d *cursor) {
    short width=0;while(*text){width+=(short)(*text=='W'?9:*text==' '?3:6);text++;}
    *text_bounds=*cursor=*bounds;cursor->x0=(short)(bounds->x0+width);
}
''' + function(native, "ui_widget_port_text_wrap") + r'''
static void apply_wrap_hook(long handle,wchar_t *value,rectangle2d bounds,rectangle2d *clipped) {
    struct {long definition_tag_index;} instance={handle},*widget=&instance;
    wchar_t **text=&value;
    rectangle2d clip=*clipped;
''' + wrap_hook + r'''
    *clipped=clip;
}
static int wide_equal(wchar_t const *a,wchar_t const *b) {
    while(*a&&*a==*b){a++;b++;}return *a==*b;
}
static struct mcc_pause_tag const *find(long handle) {
    long i;
    for(i=0;i<mcc_pause_tag_count();i++)if(mcc_pause_tag_at(i)->handle==handle)return mcc_pause_tag_at(i);
    return NULL;
}
static struct ui_widget_definition const *widget(long handle) {
    struct mcc_pause_tag const *tag=find(handle);
    assert(tag&&tag->group=='DeLa'&&tag->data);return tag->data;
}
static unsigned actions(long handle,unsigned depth) {
    struct ui_widget_definition const *w=widget(handle);
    struct ui_widget_event_handler_reference const *events=w->event_handlers.address;
    struct ui_widget_child_reference const *children=w->child_widgets.address;
    unsigned result=0;long i;
    assert(depth<10);
    for(i=0;i<w->event_handlers.count;i++) {
        if((events[i].flags&128)&&events[i].function>=MCC_PAUSE_ACTION_RESUME&&
            events[i].function<=MCC_PAUSE_ACTION_SETTINGS)
            result|=1u<<(events[i].function-MCC_PAUSE_ACTION_RESUME);
        if(events[i].flags&8)result|=actions(events[i].widget_tag.index,depth+1);
    }
    for(i=0;i<w->child_widgets.count;i++)result|=actions(children[i].widget_tag.index,depth+1);
    return result;
}
static unsigned objectives;
static int walk_bounds(long handle,int x,int y,int width,int height,int layout,unsigned depth) {
    struct ui_widget_definition const *w=widget(handle);
    struct ui_widget_child_reference const *children=w->child_widgets.address;
    struct ui_widget_game_data_input_reference const *inputs=w->game_data_inputs.address;
    long i;
    CHECK(depth<10);
    CHECK(w->bounds.x0+x>=0&&w->bounds.y0+y>=0&&w->bounds.x1+x<=width&&w->bounds.y1+y<=height);
    CHECK(w->bounds.x1>w->bounds.x0&&w->bounds.y1>w->bounds.y0);
    if(w->type==1)CHECK(w->text_font.index==assets.font[layout]||w->text_font.index==assets.small_font[layout]);
    for(i=0;i<w->game_data_inputs.count;i++){CHECK(inputs[i].function==18);objectives++;}
    if(w->type==3) {
        CHECK((w->flags&0x61)==0x21||(w->flags&0x61)==0x41);
        for(i=1;i<w->child_widgets.count;i++) {
            struct ui_widget_definition const *previous=widget(children[i-1].widget_tag.index);
            if(w->flags&0x20)CHECK(children[i].vertical_offset>=children[i-1].vertical_offset+previous->bounds.y1);
            else CHECK(children[i].horizontal_offset>=children[i-1].horizontal_offset+previous->bounds.x1);
        }
    }
    for(i=0;i<w->child_widgets.count;i++)CHECK(!walk_bounds(children[i].widget_tag.index,
        x+children[i].horizontal_offset,y+children[i].vertical_offset,width,height,layout,depth+1));
    return 0;
}
static int context_check(boolean do_bounds) {
    int campaign,network,host,teams,count,first;
    for(campaign=0;campaign<2;campaign++)for(network=0;network<2;network++)
    for(host=0;host<2;host++)for(teams=0;teams<2;teams++)for(count=1;count<=4;count++)for(first=0;first<2;first++) {
        int layout=count==1?0:count==2||(count==3&&first)?1:2;
        int width=layout==2?320:640,height=layout==0?480:240;
        long root=mcc_pause_select(campaign,network,host,teams,(short)count,first);
        struct ui_widget_definition const *w;
        unsigned bits;
        CHECK(root!=NONE&&mcc_pause_owns(root));w=widget(root);
        CHECK(w->bounds.x0==0&&w->bounds.y0==0&&w->bounds.x1==width&&w->bounds.y1==height);
        if(do_bounds) {
            struct ui_widget_child_reference const *children=w->child_widgets.address;
            long child,list=NONE,footer=NONE;
            objectives=0;CHECK(!walk_bounds(root,0,0,width,height,layout,0));
            CHECK(objectives==(unsigned)campaign);
            for(child=0;child<w->child_widgets.count;child++) {
                struct ui_widget_definition const *definition=widget(children[child].widget_tag.index);
                if(definition->type==3)list=child;
                if(!strcmp(definition->name,"button_key"))footer=child;
            }
            CHECK(list!=NONE&&footer!=NONE);
            CHECK(children[footer].vertical_offset>=children[list].vertical_offset+
                widget(children[list].widget_tag.index)->bounds.y1);
            CHECK(widget(children[footer].widget_tag.index)->text_font.index==assets.small_font[layout]);
            CHECK(widget(children[footer].widget_tag.index)->event_handlers.count==0);
            if(campaign&&layout!=2) {
                /* The stock campaign panels have a five-pixel gap. */
                CHECK(widget(children[0].widget_tag.index)->bounds.x1==218);
                CHECK(children[0].horizontal_offset+218+5==children[1].horizontal_offset);
            }
        }
        bits=actions(root,0);
        CHECK(bits&1u); /* Resume is always reachable. */
        CHECK(bits&(1u<<(MCC_PAUSE_ACTION_QUIT-MCC_PAUSE_ACTION_RESUME)));
        if(network&&!host)CHECK(!(bits&((1u<<1)|(1u<<2)|(1u<<3)|(1u<<5))));
        if(campaign)CHECK(!(bits&((1u<<5)|(1u<<6)|(1u<<7)|(1u<<8))));
        /* The native Settings screen requires a full viewport, including
         * when the first player owns the half-screen in a three-player game. */
        CHECK(!!(bits&(1u<<8))==(!campaign&&assets.settings&&count==1));
        if(!campaign)CHECK(!(bits&((1u<<1)|(1u<<3))));
        if(!network||campaign||!teams)CHECK(!(bits&((1u<<6)|(1u<<7))));
        if(!campaign&&network&&teams)CHECK((bits&((1u<<6)|(1u<<7)))==((1u<<6)|(1u<<7)));
        if(!campaign&&network&&host)CHECK(bits&(1u<<5));
    }
    return 0;
}
int main(int argc,char **argv) {
    struct mcc_pause_assets original=assets;
    long i,j;unsigned calls,art_total;
    CHECK(argc==2);
    CHECK(mcc_pause_tag_count()==0&&mcc_pause_tag_at(0)==NULL&&!mcc_pause_owns(0));
    CHECK(mcc_pause_select(TRUE,FALSE,FALSE,FALSE,1,TRUE)==NONE);
    CHECK(mcc_pause_build(1024,0x4321,&assets));
    CHECK(!memcmp(&assets,&original,sizeof(assets)));
    calls=allocation_calls;art_total=art_calls;
    if(!strcmp(argv[1],"identity")) {
        CHECK(mcc_pause_tag_count()>24&&mcc_pause_tag_count()<4096);
        CHECK(mcc_pause_tag_at(-1)==NULL&&mcc_pause_tag_at(mcc_pause_tag_count())==NULL);
        for(i=0;i<mcc_pause_tag_count();i++) {
            struct mcc_pause_tag const *t=mcc_pause_tag_at(i);
            CHECK(t&&t->data&&t->name&&strlen(t->name)>0);
            CHECK(!strncmp(t->name,"mcc_pause\\",10));
            CHECK((t->handle&0xffff)==1024+i);
            CHECK(t->group=='DeLa'||t->group=='ustr');
            CHECK(mcc_pause_owns(t->handle)==(t->group=='DeLa'));
            CHECK(!mcc_pause_owns(t->handle^0x01000000));
            for(j=0;j<i;j++)CHECK(strcmp(t->name,mcc_pause_tag_at(j)->name)!=0&&t->handle!=mcc_pause_tag_at(j)->handle);
        }
    } else if(!strcmp(argv[1],"contexts")) {
        CHECK(!context_check(FALSE));
    } else if(!strcmp(argv[1],"layout")) {
        CHECK(!context_check(TRUE));
    } else if(!strcmp(argv[1],"actions")) {
        for(i=0;i<mcc_pause_tag_count();i++) {
            struct mcc_pause_tag const *t=mcc_pause_tag_at(i);
            struct ui_widget_definition const *w;
            struct ui_widget_event_handler_reference const *e;
            if(t->group!='DeLa')continue;
            w=t->data;e=w->event_handlers.address;
            for(j=0;j<w->event_handlers.count;j++) {
                CHECK(!(e[j].flags&1024)&&!e[j].script[0]);
                if(e[j].flags&128)CHECK(e[j].function>=MCC_PAUSE_ACTION_RESUME&&e[j].function<=MCC_PAUSE_ACTION_SETTINGS);
                if(e[j].flags&8)CHECK(find(e[j].widget_tag.index)&&mcc_pause_owns(e[j].widget_tag.index));
                if(e[j].event_type==0) {
                    long k;
                    for(k=0;k<w->event_handlers.count;k++)if(e[k].event_type==28)break;
                    CHECK(k<w->event_handlers.count&&e[k].flags==e[j].flags&&e[k].function==e[j].function&&
                          e[k].widget_tag.index==e[j].widget_tag.index);
                }
            }
        }
    } else if(!strcmp(argv[1],"controller-close")) {
        unsigned resume_accept=0,resume_back=0,red=0,blue=0;
        for(i=0;i<mcc_pause_tag_count();i++) {
            struct mcc_pause_tag const *t=mcc_pause_tag_at(i);
            struct ui_widget_definition const *w;
            struct ui_widget_event_handler_reference const *e;
            if(t->group!='DeLa')continue;
            w=t->data;e=w->event_handlers.address;
            for(j=0;j<w->event_handlers.count;j++) {
                if(e[j].function!=MCC_PAUSE_ACTION_RESUME&&
                    e[j].function!=MCC_PAUSE_ACTION_RED_TEAM&&
                    e[j].function!=MCC_PAUSE_ACTION_BLUE_TEAM)continue;
                /* These callbacks own controller-local tree/history cleanup.
                 * Generic close-all would dismiss another player's settings. */
                CHECK(e[j].flags==128&&e[j].widget_tag.index==NONE);
                if(e[j].function==MCC_PAUSE_ACTION_RESUME) {
                    if(e[j].event_type==0||e[j].event_type==28)resume_accept++;
                    else {CHECK(e[j].event_type==1||e[j].event_type==13);resume_back++;}
                } else {
                    CHECK(e[j].event_type==0||e[j].event_type==28);
                    if(e[j].function==MCC_PAUSE_ACTION_RED_TEAM)red++;else blue++;
                }
            }
        }
        CHECK(resume_accept&&resume_back&&red&&blue);
    } else if(!strcmp(argv[1],"dialogs")) {
        unsigned dialogs=0;
        for(i=0;i<mcc_pause_tag_count();i++) {
            struct mcc_pause_tag const *t=mcc_pause_tag_at(i);
            struct ui_widget_definition const *w;
            struct ui_widget_event_handler_reference const *e;
            struct ui_widget_child_reference const *children;
            long list=NONE;
            if(t->group!='DeLa')continue;
            w=t->data;
            if(strcmp(w->name,"confirmation"))continue;
            dialogs++;children=w->child_widgets.address;
            for(j=0;j<w->child_widgets.count;j++)if(widget(children[j].widget_tag.index)->type==3)list=children[j].widget_tag.index;
            CHECK(list!=NONE);
            children=widget(list)->child_widgets.address;
            e=widget(children[0].widget_tag.index)->event_handlers.address;
            CHECK(e[0].event_type==0&&e[0].flags==512); /* Native default focus starts on Cancel. */
            e=widget(children[1].widget_tag.index)->event_handlers.address;
            CHECK(e[0].event_type==0&&(e[0].flags&128)&&e[0].function!=MCC_PAUSE_ACTION_RESUME);
            e=w->event_handlers.address;
            CHECK(w->event_handlers.count==2&&e[0].event_type==1&&e[1].event_type==13&&e[0].flags==512&&e[1].flags==512);
            objectives=0;
            CHECK(!walk_bounds(t->handle,0,0,w->bounds.x1,w->bounds.y1,
                w->bounds.x1==320?2:w->bounds.y1==240?1:0,0));
        }
        CHECK(dialogs>=15);
    } else if(!strcmp(argv[1],"paragraphs")) {
        unsigned objective_count=0,confirm_count=0;
        for(i=0;i<mcc_pause_tag_count();i++) {
            struct mcc_pause_tag const *t=mcc_pause_tag_at(i);
            struct ui_widget_definition const *w=t->data;
            boolean paragraph=t->group=='DeLa'&&(!strcmp(w->name,"objective")||!strcmp(w->name,"confirmation_text"));
            CHECK(mcc_pause_text_wrap_needed(t->handle)==paragraph);
            CHECK(!mcc_pause_text_wrap_needed(t->handle^0x01000000));
            if(!paragraph)continue;
            if(!strcmp(w->name,"objective"))objective_count++;else confirm_count++;
        }
        CHECK(objective_count&&confirm_count);
        CHECK(!mcc_pause_text_wrap_needed(NONE));
    } else if(!strncmp(argv[1],"wrap-",5)) {
        long paragraph=NONE,plain=mcc_pause_select(TRUE,FALSE,FALSE,FALSE,1,TRUE);
        rectangle2d bounds={20,100,90,130},clip={0,0,480,640},before=clip;
        struct {wchar_t text[128];unsigned canary;} value={L"aa aa aa",0x1234abcd};
        for(i=0;i<mcc_pause_tag_count();i++)if(mcc_pause_text_wrap_needed(mcc_pause_tag_at(i)->handle)) {
            paragraph=mcc_pause_tag_at(i)->handle;break;
        }
        CHECK(paragraph!=NONE);
        if(!strcmp(argv[1],"wrap-boundaries")) {
            wchar_t exact[]=L"WW WW",over[]=L"WW WW",unbroken[]=L"WWWWWW";
            CHECK(ui_widget_port_text_wrap(exact,&bounds,39)&&wide_equal(exact,L"WW WW"));
            CHECK(ui_widget_port_text_wrap(over,&bounds,38)&&wide_equal(over,L"WW\rWW"));
            CHECK(!ui_widget_port_text_wrap(unbroken,&bounds,30)&&wide_equal(unbroken,L"WWWWWW"));
            apply_wrap_hook(paragraph,value.text,bounds,&clip);
            CHECK(wide_equal(value.text,L"aa aa\raa")&&value.canary==0x1234abcd);
            CHECK(!memcmp(&clip,&bounds,sizeof(clip)));
        } else if(!strcmp(argv[1],"wrap-lines")) {
            wchar_t lines[]=L"aa aa\r\nbb bb cc",empty[]=L"";
            apply_wrap_hook(paragraph,lines,bounds,&clip);
            CHECK(wide_equal(lines,L"aa aa\r\nbb bb\rcc"));
            apply_wrap_hook(paragraph,lines,bounds,&clip);
            CHECK(wide_equal(lines,L"aa aa\r\nbb bb\rcc"));
            CHECK(ui_widget_port_text_wrap(empty,&bounds,30)&&!empty[0]);
        } else {
            CHECK(!strcmp(argv[1],"wrap-isolation"));
            apply_wrap_hook(plain,value.text,bounds,&clip);
            CHECK(wide_equal(value.text,L"aa aa aa")&&!memcmp(&clip,&before,sizeof(clip)));
            apply_wrap_hook(paragraph^0x01000000,value.text,bounds,&clip);
            CHECK(wide_equal(value.text,L"aa aa aa")&&!memcmp(&clip,&before,sizeof(clip)));
            clip=(rectangle2d){25,105,80,125};before=clip;
            apply_wrap_hook(paragraph,value.text,bounds,&clip);
            CHECK(wide_equal(value.text,L"aa aa\raa")&&!memcmp(&clip,&before,sizeof(clip)));
        }
    } else if(!strcmp(argv[1],"strings")) {
        unsigned lists=0;
        for(i=0;i<mcc_pause_tag_count();i++) {
            struct mcc_pause_tag const *t=mcc_pause_tag_at(i);
            struct tag_block const *block=t->data;
            struct tag_data const *entries=block->address;
            if(t->group!='ustr')continue;
            lists++;CHECK(block->count>=20&&block->count<128);
            for(j=0;j<block->count;j++) {
                unsigned short const *text=entries[j].address;long k;
                CHECK(text&&entries[j].size>=2&&entries[j].size<=256&&!(entries[j].size&1));
                CHECK(!text[entries[j].size/2-1]);
                for(k=0;k<entries[j].size/2-1;k++)CHECK(text[k]>=32&&text[k]<127);
            }
        }
        CHECK(lists==1);
    } else if(!strcmp(argv[1],"failure")) {
        mcc_pause_dispose();CHECK(!live_allocations);
        for(fail_allocation=1;fail_allocation<=calls;fail_allocation++) {
            allocation_calls=0;CHECK(!mcc_pause_build(1024,0x4321,&assets));
            CHECK(!live_allocations&&!mcc_pause_tag_count());
            CHECK(mcc_pause_select(TRUE,FALSE,FALSE,FALSE,1,TRUE)==NONE);
        }
        fail_allocation=0;
        CHECK(mcc_pause_build(1024,0x4321,&assets));
    } else if(!strcmp(argv[1],"art-failure")) {
        mcc_pause_dispose();CHECK(!live_allocations);
        for(fail_art=1;fail_art<=art_total;fail_art++) {
            art_calls=0;CHECK(!mcc_pause_build(1024,0x4321,&assets));
            CHECK(!live_allocations&&!mcc_pause_tag_count());
        }
        fail_art=0;CHECK(mcc_pause_build(1024,0x4321,&assets));
    } else if(!strcmp(argv[1],"invalid")) {
        CHECK(mcc_pause_select(TRUE,FALSE,FALSE,FALSE,0,TRUE)==NONE);
        CHECK(mcc_pause_select(TRUE,FALSE,FALSE,FALSE,5,TRUE)==NONE);
        CHECK(!mcc_pause_build(-1,1,&assets));CHECK(!live_allocations&&!mcc_pause_tag_count());
        CHECK(!mcc_pause_build(65535,1,&assets));CHECK(!live_allocations&&!mcc_pause_tag_count());
        CHECK(!mcc_pause_build(0,1,NULL));CHECK(!live_allocations&&!mcc_pause_tag_count());
        assets.art=NULL;CHECK(!mcc_pause_build(1024,1,&assets));assets=original;
        for(i=0;i<3;i++) {
            assets.font[i]=NONE;CHECK(!mcc_pause_build(1024,1,&assets));assets=original;
            assets.small_font[i]=NONE;CHECK(!mcc_pause_build(1024,1,&assets));assets=original;
        }
        CHECK(!live_allocations&&!mcc_pause_tag_count());
    } else if(!strcmp(argv[1],"lifetime")) {
        long old=mcc_pause_select(TRUE,FALSE,FALSE,FALSE,1,TRUE);
        mcc_pause_dispose();mcc_pause_dispose();CHECK(!live_allocations&&!mcc_pause_owns(old));
        assets.settings=FALSE;CHECK(mcc_pause_build(2048,0x4322,&assets));CHECK(!mcc_pause_owns(old));
        for(i=0;i<2;i++)for(j=0;j<2;j++) {
            long root=mcc_pause_select(FALSE,TRUE,(boolean)i,(boolean)j,1,TRUE);
            CHECK(!(actions(root,0)&(1u<<8)));
        }
    } else return 99;
    mcc_pause_dispose();CHECK(!live_allocations&&!mcc_pause_tag_count());return 0;
}
'''
    path = work / "pause_check.c"
    path.write_text(source)
    binary = work / ("pause_check.exe" if sys.platform == "win32" else "pause_check")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror",
               "-Wno-unused-function", "-I", str(work), "-I", str(ROOT / "port/linux/game"),
               str(path), "-o", str(binary)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command[1:1] = ["-m32", "-fshort-wchar"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


@pytest.mark.parametrize("case", ["identity", "contexts", "layout", "actions", "controller-close", "dialogs", "paragraphs", "strings",
                                 "wrap-boundaries", "wrap-lines", "wrap-isolation", "failure", "art-failure", "invalid", "lifetime"])
def test_mcc_pause_builder(pause_tool, case):
    result = subprocess.run([str(pause_tool), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, f"{case}: exit {result.returncode}\n{result.stdout}{result.stderr}"
