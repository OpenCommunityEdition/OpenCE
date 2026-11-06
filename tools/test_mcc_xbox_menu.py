"""Execute the MCC wrapper, with native profile/list services as boundaries."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

import mcc_xbox_menu
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_native_card_geometry_and_separate_routes():
    doc = ET.Element('menus')
    mcc_xbox_menu.append(doc)
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    entry = widgets['mcc_xbox/campaign_entry']
    assert (entry.get('width'), entry.get('height')) == ('256', '33')
    assert entry.get('bitmap') == 'mcc_xbox/menu_campaigns'
    assert entry.get('text') is None and entry.find('child') is None
    assert {e.get('event') for e in entry.findall('on')} == {'a', 'start', 'left_mouse'}
    frames = doc.find("bitmap[@name='mcc_xbox/menu_campaigns']").findall('frame')
    assert len(frames) == 2
    assert all((f.get('width'), f.get('height')) == ('256', '64') for f in frames)
    for i, (x, width, height) in enumerate(((72, 165, 330), (239, 163, 326), (406, 162, 326))):
        card = widgets[f'mcc_xbox/card_{i}']
        assert tuple(int(card.get(k)) for k in ('left', 'top', 'width', 'height')) == (x, 75, width, height)
        assert card.get('bitmap') == r'ui\shell\bitmaps\spinner_list_3_wide_item_background'
        image = widgets[f'mcc_xbox/card_{i}/picture']
        assert tuple(int(image.get(k)) for k in ('left', 'top', 'width', 'height')) == (x+13, 87, 140, 114)
        for leaf, top, height in (('mcc_folder_name', 204, 43), ('mcc_folder_help', 258, 136)):
            text = widgets[f'mcc_xbox/card_{i}/{leaf}']
            assert tuple(int(text.get(k)) for k in ('left', 'top', 'width', 'height')) == (x+11, top, 144+i, height)
            assert text.get('text') is None  # dynamic text must not be overwritten by a string-list tag
        for marker in range(3):
            badge = widgets[f'mcc_xbox/card_{i}/badge_{marker}']
            assert tuple(int(badge.get(k)) for k in ('left','top','width','height')) == (x+10+50*marker,343,44,48)
    for leaf, box in (('previous',(50,174,18,170)),('next',(575,174,35,170))):
        assert tuple(int(widgets['mcc_xbox/'+leaf].get(k)) for k in ('left','top','width','height')) == box
    profiles = widgets['mcc_xbox/profiles']
    assert profiles.find('child').get('widget') == mcc_xbox_menu.NATIVE+'player_profiles_select\\player_profile_select_list'
    difficulty = widgets['mcc_xbox/difficulty_list']
    assert len(difficulty.findall('child')) == 4
    assert difficulty.get('description') == mcc_xbox_menu.NATIVE+'difficulty_select\\difficulty_ext_desc'
    assert difficulty.find('data').get('input') == 'difficulty menu update desc'
    for index in range(4):
        row = widgets[f'mcc_xbox/difficulty_{index}']
        # PC list navigation skips rows without actions as labels. Each native-
        # styled MCC row must remain selectable, with its own difficulty index.
        assert row.get('string_index') == str(index)
        assert row.find("on[@event='a']").get('run') == 'port mcc xbox choose'
    # No native map list or native map-selection callback is wired into MCC.
    assert all('mp_map_select_list' not in ET.tostring(w, encoding='unicode') for w in widgets.values())


def test_profile_folder_difficulty_back_and_host_handoff(tmp_path):
    body = re.sub(r'^#include[^\n]*\n', '', (ROOT/'port/linux/game/mcc_xbox_menu.inl').read_text(), flags=re.M)
    run_c(tmp_path, r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define BUTTON_X 2
#define SOUND_BACK 1
#define SOUND_FORWARD 2
#define NUMBER_OF_GAME_DIFFICULTY_LEVELS 4
#define MCC_MAPS_FIRST_DISPLAY_INDEX 0x1000
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PIN(x,a,b) ((x)<(a)?(a):(x)>(b)?(b):(x))
#define FRONTEND_MENU_XBOX "ui\\shell\\main_menu\\main_menu"
struct widget_instance {
 long definition_tag_index;const char *name;short local_player_index;int visible;
 short horizontal_offset,vertical_offset;
 struct widget_instance *parent,*child,*next,*previous,*focused_child;
 struct {short current_frame_index;} animation;
 struct {struct {short selected_index;} list;} parameters;
};
struct event_record {short type,controller_index;struct {struct {int index,value;} button;} data;};
struct player_profile {int unused;};
struct mcc_map_browser {short count,chosen;int campaign;char package[64];};
static int available=1,main_menu=1,enabled=1,backs,starts,rescans,native_calls,mp_choices;
static const char *style="xbox",*root_tag=FRONTEND_MENU_XBOX;
static long active_profile=2;
static short started_skill;
static char opened[256],started[64];
static struct widget_instance entry;
static int frontend_menu_available(void){return available;}
static int main_menu_is_active(void){return main_menu;}
static const char *frontend_menu_style(void){return style;}
static int config_boolean(const char *key){(void)key;return enabled;}
static const char *tag_get_name(long tag){return tag==0?root_tag:tag==1?"pc\\mcc_xbox\\card_0":tag==2?"pc\\mcc_xbox\\card_1":"pc\\mcc_xbox\\card_2";}
static struct widget_instance *screen_of(struct widget_instance *w){while(w->parent)w=w->parent;return w;}
static struct widget_instance *named(struct widget_instance *w,const char *name,int n){
 struct widget_instance *c,*found;(void)n;if(!w)return NULL;
 for(c=w->child;c;c=c->next){if(!strcmp(c->name,name))return c;found=named(c,name,0);if(found)return found;}return NULL;
}
static struct widget_instance *ui_widget_load_by_name_or_tag(const char *name,long tag,struct widget_instance *parent,short player,long a,long b,short c){
 (void)tag;(void)player;(void)a;(void)b;(void)c;strcpy(opened,name);memset(&entry,0,sizeof(entry));entry.parent=parent;entry.name="campaign_entry";return &entry;
}
static int ui_widget_port_open(struct widget_instance *w,const char *path,boolean *deleted){(void)w;strcpy(opened,path);*deleted=1;return 1;}
static void ui_widget_port_go_back(struct widget_instance *w){(void)w;backs++;}
static short ui_widget_port_list_index(struct widget_instance *w){return w?w->parameters.list.selected_index:-1;}
static void text_set(struct widget_instance *w,const wchar_t *text){(void)w;(void)text;}
static void text_set_length(struct widget_instance *w,const wchar_t *text,short n){(void)n;text_set(w,text);}
static void visible_set(struct widget_instance *w,int visible){if(w)w->visible=visible;}
static int usnprintf(wchar_t *out,int n,const wchar_t *format,...){(void)n;(void)format;out[0]=0;return 0;}
static void mcc_maps_rescan(void){rescans++;}
static void mcc_map_browser_open(struct mcc_map_browser *b,int campaign){b->count=1;b->chosen=0;b->campaign=campaign;}
static void mcc_map_browser_select(struct mcc_map_browser *b,short row){assert(row>=0&&row<b->count);b->chosen=row;}
static int mcc_map_browser_enter(struct mcc_map_browser *b,short row){if(b->package[0]||row>=b->count)return 0;strcpy(b->package,"pkg");b->count=4;b->chosen=0;return 1;}
static int mcc_map_browser_back(struct mcc_map_browser *b){if(!b->package[0])return 0;b->package[0]=0;b->count=1;b->chosen=0;return 1;}
static short mcc_map_browser_index(struct mcc_map_browser *b,short row){return b->package[0]&&row>=0&&row<b->count?row:-1;}
static void mcc_map_browser_text(struct mcc_map_browser *b,short row,wchar_t text[64]){(void)b;(void)row;wcscpy(text,L"Mission");}
static const char *mcc_maps_level_name(short i){return i>=0&&i<4?"mcc_maps\\pkg\\a10":NULL;}
static const wchar_t *mcc_maps_description(short i){(void)i;return L"Description";}
static unsigned mcc_progress_flags(long p,const char *level){assert(p==2&&level);return 10;}
static long player_ui_get_active_player_profile_index(short p){assert(p==0);return active_profile;}
static int player_profile_get(long p,struct player_profile *profile){(void)profile;return p==2;}
static int game_function(const char *name,struct widget_instance *w,struct event_record *e,boolean *deleted){(void)w;(void)e;(void)deleted;assert(strstr(name,"controller")||strstr(name,"prof"));native_calls++;return 1;}
static void ui_play_audio_feedback_sound(short s){(void)s;}
static int campaign_fail(void){return 0;}
static void campaign_start(const char *level,short skill,short controller){assert(controller==0);strcpy(started,level);started_skill=skill;starts++;}
static int ui_widget_port_mcc_multiplayer_map_choose(const char *level){assert(!strncmp(level,"mcc_maps\\",9));mp_choices++;return 1;}
''' + body + r'''
int main(void){
 struct widget_instance root={0},children[12]={{0}},cards[3]={{0}},parts[3][6]={{{0}}};
 struct widget_instance menu={0},rows[3]={{0}};
 const char *names[]={"heading_campaigns","heading_levels","heading_maps","heading_difficulty","previous","next","difficulty_list","status"};
 const char *leaves[]={"picture","mcc_folder_name","mcc_folder_help","badge_0","badge_1","badge_2"};
 struct event_record event={3,0,{{2,1}}};int deleted=0,i,j;
 root.child=children;for(i=0;i<8;i++){children[i].name=names[i];children[i].parent=&root;children[i].next=i<7?children+i+1:cards;}
 for(i=0;i<3;i++){cards[i].name=i==0?"card_0":i==1?"card_1":"card_2";cards[i].definition_tag_index=i+1;cards[i].parent=&root;cards[i].next=i<2?cards+i+1:NULL;cards[i].child=parts[i];
  for(j=0;j<6;j++){parts[i][j].name=leaves[j];parts[i][j].parent=cards+i;parts[i][j].next=j<5?parts[i]+j+1:NULL;}}
 assert(!mcc_xbox_shortcut(&root,&event));
 assert(mcc_xbox_event("open",&root,&event,&deleted)&&!strcmp(opened,"pc\\mcc_xbox\\profiles"));
 event.data.button.value=2;assert(!mcc_xbox_shortcut(&root,&event));event.data.button.value=1;
 style="modern";assert(!mcc_xbox_shortcut(&root,&event));style="xbox";
 enabled=0;assert(!mcc_xbox_shortcut(&root,&event));enabled=1;
 main_menu=0;assert(!mcc_xbox_shortcut(&root,&event));main_menu=1;
 root_tag="pc\\main_menu\\main_menu";assert(!mcc_xbox_shortcut(&root,&event));
 root_tag=FRONTEND_MENU_XBOX;
 root.child=&menu;menu.name="main_menu_select_list";menu.parent=&root;menu.child=rows;menu.focused_child=rows;
 for(i=0;i<3;i++){rows[i].name=i==0?"load_campaign_item_":i==1?"multiplayer_item":"settings_item";
  rows[i].parent=&menu;rows[i].previous=i?rows+i-1:NULL;rows[i].next=i<2?rows+i+1:NULL;rows[i].horizontal_offset=192;rows[i].vertical_offset=247+i*36;}
 enabled=0;mcc_xbox_attach(&root);assert(rows[0].next==rows+1&&rows[1].vertical_offset==283);enabled=1;
 style="modern";mcc_xbox_attach(&root);assert(rows[0].next==rows+1);style="xbox";
 mcc_xbox_attach(&root);assert(rows[0].next==&entry&&entry.previous==rows&&entry.next==rows+1&&rows[1].previous==&entry);
 assert(entry.parent==&menu&&entry.horizontal_offset==192&&entry.vertical_offset==283);
 assert(rows[0].vertical_offset==247&&rows[1].vertical_offset==319&&rows[2].vertical_offset==355&&menu.focused_child==rows);
 mcc_xbox_attach(&root);assert(rows[1].vertical_offset==319&&entry.next==rows+1);
 assert(mcc_xbox_event("open",&entry,&event,&deleted)&&!strcmp(opened,"pc\\mcc_xbox\\profiles"));
 root.child=children;
 root_tag=MCC_XBOX_NATIVE "connected\\connected_map_select_wrapper";
 assert(mcc_xbox_shortcut(&root,&event)&&!strcmp(opened,"pc\\mcc_xbox\\connected"));
 mcc_xbox_attach(&root);assert(cards[2].next==&entry&&entry.previous==cards+2);cards[2].next=NULL;
 root_tag="pc\\mcc_xbox\\profiles";assert(mcc_xbox_event("profile",&root,&event,&deleted)&&native_calls==2&&!strcmp(opened,"pc\\mcc_xbox\\campaign"));
 root_tag="pc\\mcc_xbox\\campaign";assert(mcc_xbox_event("init",&root,&event,&deleted)&&rescans==1);
 assert(children[0].visible&&!children[1].visible&&!children[6].visible);
 assert(mcc_xbox_event("choose",&root,&event,&deleted));assert(mcc_xbox_browser.count==4&&children[1].visible);
 assert(parts[0][0].animation.current_frame_index==0x1000);
 assert(parts[0][3].visible&&!parts[0][4].visible&&parts[0][5].visible);
 for(i=0;i<3;i++)assert(mcc_xbox_event("right",&root,&event,&deleted));
 assert(mcc_xbox_browser.chosen==3&&mcc_xbox_slot==2);
 assert(mcc_xbox_event("right",&root,&event,&deleted)&&mcc_xbox_browser.chosen==0);
 assert(mcc_xbox_event("choose",&root,&event,&deleted)&&mcc_xbox_difficulty&&children[6].visible&&!cards[0].visible&&children[3].visible);
 children[6].parameters.list.selected_index=2;
 assert(mcc_xbox_event("choose",&root,&event,&deleted)&&starts==1&&started_skill==2&&!strcmp(started,"mcc_maps\\pkg\\a10"));
 active_profile=3;assert(!mcc_xbox_event("choose",&root,&event,&deleted)&&starts==1);active_profile=2;
 assert(mcc_xbox_event("back",&root,&event,&deleted)&&!mcc_xbox_difficulty&&mcc_xbox_browser.package[0]);
 assert(mcc_xbox_event("back",&root,&event,&deleted)&&!mcc_xbox_browser.package[0]&&!backs);
 assert(mcc_xbox_event("back",&root,&event,&deleted)&&backs==1);
 root_tag="pc\\mcc_xbox\\connected";assert(mcc_xbox_event("init",&root,&event,&deleted));
 assert(!mcc_xbox_browser.campaign);mcc_xbox_event("choose",&root,&event,&deleted);mcc_xbox_event("choose",&root,&event,&deleted);
 assert(mp_choices==1&&!strcmp(opened,MCC_XBOX_NATIVE "connected\\gametype_select_screen_wrapper"));
 mcc_xbox_reset();assert(!mcc_xbox_browser.count&&!mcc_xbox_browser.package[0]&&!mcc_xbox_difficulty&&mcc_xbox_profile==NONE);
 return 0;
}
''')
