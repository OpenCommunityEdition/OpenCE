"""Modern views preserve native multiplayer services and viewport navigation."""
import xml.etree.ElementTree as ET

import frontend_multiplayer
from frontend_menu import text_width
from test_frontend_menu import ROOT, HEADER, RUNTIME_STUBS, without_includes
from test_frontend_presentation import source
from test_hud_button_style import run_c
from harness import function


def test_service_contracts_and_routes():
    doc = ET.Element('menus')
    roles, routes = frontend_multiplayer.append(doc)
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    legacy = frontend_multiplayer.inventory()
    assert len(routes) == len(frontend_multiplayer.PAGES)
    for old, new in routes:
        service = next((w for path,w in widgets.items() if path.startswith(new.rsplit('/',1)[0]+'/') and path.endswith('/'+old.rsplit('/',1)[-1])), widgets[new])
        assert [n.attrib for n in service.findall('on')] == [n.attrib for n in legacy[old].findall('on')]
        assert new.startswith('modern/network/')
        assert 'pause_game' not in widgets[new].get('flags', '')
    # Preserve child order and count for the native paged services. Presentation
    # must not change what index the service treats as a map/gametype/player.
    lists = ('mp_map_select_list_2', 'gametype_select_list_2', 'lobby_list')
    for name in lists:
        old = next(w for path,w in legacy.items() if path.endswith('/'+name))
        new = next(w for path,w in widgets.items() if path.endswith('/'+name))
        assert [c.get('widget').rsplit('/',1)[-1] for c in new.findall('child')] == [c.get('widget').rsplit('/',1)[-1] for c in old.findall('child')]
        assert new.findall('data')[-1].get('input') == 'port frontend multiplayer update'
    assert not any('/browser/' in n or '/filters/' in n for n in widgets)
    assert not any('join_game_screen' in old for old,_ in routes)
    # Every dependency is in the isolated generated set or an existing service.
    for w in widgets.values():
        assert sum(key in w.attrib for key in ('text', 'strings', 'string_list')) <= 1
        if w.get('text') is not None:
            assert int(w.get('string_index', '0')) == 0
        for c in w.findall('child'):
            assert c.get('widget') in widgets
        for c in w.findall('on'):
            if c.get('open'): assert c.get('open') in legacy
    assert not any('main_menu/main_menu' == old for old,_ in routes)
    assert not any(w.get('bitmap') in ('bitmaps/gradient', 'bitmaps/gradient_big') for w in widgets.values())
    assert not any(path.rsplit('/', 1)[-1] == 'header' or path.rsplit('/', 1)[-1].startswith('header_') for path in widgets)
    assert len(dict(roles)) < len(widgets)


def test_embedded_difficulty_replaces_detail_and_restores_rows(tmp_path):
    stubs = RUNTIME_STUBS.replace('return id==1?',
        'return id==8?"pc\\\\modern\\\\network\\\\maps\\\\screen":id==9?"pc\\\\modern\\\\network\\\\maps\\\\list_item_text":id==10?"pc\\\\modern\\\\network\\\\maps\\\\list_item_text_centered":id==1?')
    stubs = stubs.replace('(void)group;(void)path;return -1;',
                         '(void)group;return strstr(path,"_centered")?10:9;')
    stubs = stubs.replace('(void)root;(void)name;(void)n;return 0;', '''
        struct widget_instance *child,*found;(void)n;
        if(!root)return 0;
        if(root->name&&!strcmp(root->name,name))return root;
        for(child=root->child;child;child=child->next){found=named(child,name,0);if(found)return found;}
        return 0;''')
    code = (ROOT/'port/linux/game/frontend_multiplayer.inl').read_text()
    run_c(tmp_path, stubs + ''.join(function(code, n) for n in
          ('frontend_multiplayer_root', 'frontend_multiplayer_difficulty')) + r'''
int main(void){
    struct widget_instance root={0},list={0},description={0},icon={0},text={0},map={0},row={0},label={0};
    int i;
    root.definition_tag_index=8;list.parent=&root;list.child=&row;row.child=&label;
    label.name="list_item_text";label.definition_tag_index=9;
    list.parameters.list.extended_description=&description;description.child=&icon;
    icon.name="difficulty_options";icon.next=&text;text.name="difficulty_extended_description";
    text.next=&map;map.name="mp_map_right_item";
    for(i=0;i<4;i++){
        map.visible=1;frontend_multiplayer_difficulty(&list,i);
        assert(!map.visible&&icon.visible&&text.visible);
        assert(icon.animation.current_frame_index==i&&text.parameters.text_box.string_list_index==i);
        assert(label.definition_tag_index==10);
    }
    map.visible=1;frontend_multiplayer_difficulty(&list,NONE);
    assert(map.visible&&!icon.visible&&!text.visible&&label.definition_tag_index==9);
    available=0;frontend_multiplayer_difficulty(&list,2);assert(map.visible&&!icon.visible);
    available=1;root.definition_tag_index=1;frontend_multiplayer_difficulty(&list,2);
    assert(map.visible&&!icon.visible&&label.definition_tag_index==9);
    return 0;
}
''')


def test_pregame_routes_and_style_switch_guard(tmp_path):
    run_c(tmp_path,r'''
#include <assert.h>
#include <string.h>
#define NONE -1
typedef int boolean;
static char const *choice="modern",*name;
static int main_menu=1,network=1,tags=1;
boolean main_menu_is_active(void){return main_menu;}
boolean network_game_is_active(void){return network;}
static char const *config_string(char const *key){(void)key;return choice;}
static char const *tag_get_name(long tag){(void)tag;return name;}
static int csstrcasecmp(char const *a,char const *b){return strcmp(a,b);}
static long tag_loaded(long g,char const *n){(void)g;(void)n;return tags?91:NONE;}
void frontend_input_enable(int x){(void)x;}
void keyboard_glyphs_dispose(void){}
void frontend_menu_text_end(void){}
void frontend_panel_end(void){}
void frontend_menu_session_reset(void){}
''' + HEADER + without_includes('port/linux/game/frontend_menu.c') + r'''
int main(void){
    unsigned int i;
    frontend_menu_map_loaded("ui",0);
    for(i=0;i<sizeof(frontend_network_routes)/sizeof(frontend_network_routes[0]);i++){
        name=frontend_network_routes[i].legacy;
        assert(frontend_menu_destination(4)==91); /* host/client pregame */
        main_menu=0;assert(frontend_menu_destination(4)==4);main_menu=1;
        tags=0;assert(frontend_menu_destination(4)==4);tags=1;
        choice="xbox";assert(frontend_menu_destination(4)==4);choice="modern";
        frontend_menu_map_loaded("ui",1);assert(frontend_menu_destination(4)==4);
        frontend_menu_map_loaded("custom_maps\\test",0);assert(frontend_menu_destination(4)==4);
        frontend_menu_map_loaded("ui",0);
    }
    name="pc\\main_menu\\multiplayer_type_select\\join_game\\join_game_screen";assert(frontend_menu_destination(4)==4);
    name="pc\\main_menu\\main_menu";assert(frontend_menu_destination(4)==4);
    name="ui\\shell\\main_menu\\main_menu";assert(frontend_menu_destination(4)==4);
    return 0;
}
''')


def test_session_options_keep_host_callbacks_and_fit_narrow_columns():
    doc = ET.Element('menus')
    _, routes = frontend_multiplayer.append(doc)
    root = dict(routes)[frontend_multiplayer.PAGES['session'][0]]
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    assert root.startswith('modern/network/session/')
    session = [w for w in widgets.values() if w.get('name').startswith('modern/network/session/')]
    options = next(w for w in session if w.get('name').endswith('/options_menu'))
    assert options.find("on[@event='created']").get('run') == 'server settings init'
    assert [n.get('input') for n in options.findall('data')] == ['server settings update', 'port frontend multiplayer update']
    for leaf in ('session_compatibility_spinner', 'camera_policy_spinner'):
        spinner = next(w for w in session if w.get('name').endswith('/'+leaf))
        for caption in spinner.get('strings').split('|'):
            assert text_width(caption, 10) <= int(spinner.get('width'))
        assert spinner.find("on[@event='left_mouse']").get('run') == 'mouse spinner 1wide click'
    compatibility = next(w for w in session if w.get('name').endswith('/session_compatibility_spinner'))
    assert compatibility.get('strings') == 'CROSSPLAY|GULCHCE ONLY'


def test_added_options_keep_help_inside_panel_and_voice_modes_in_order():
    doc = ET.Element('menus')
    roles, _ = frontend_multiplayer.append(doc)
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    for page, leaf in [('coop', 'coop_options_help'), ('voice', 'voice_options_help'), ('sharing', 'help')]:
        help = next(w for name, w in widgets.items()
                    if name.startswith('modern/network/'+page+'/') and name.endswith('/'+leaf))
        assert dict(roles)[help.get('name')] == 'detail_wrap'
        assert int(help.get('left')) >= 396
        assert int(help.get('top')) + int(help.get('height')) <= 390
    voice = next(w for name, w in widgets.items() if name.endswith('/voice_mode_spinner'))
    assert voice.get('strings').split('|') == ['OFF', 'TEAM NEAR', 'ANYONE NEAR', 'TEAM', 'TEAM + NEARBY']
    assert all(text_width(s, 10) <= int(voice.get('width')) for s in voice.get('strings').split('|'))
    label = next(w for name, w in widgets.items() if name.endswith('/voice_proximity_label'))
    assert text_width(label.get('text'), 10) <= int(label.get('width'))


def test_scroll_preserves_service_indices_visibility_and_lifetime(tmp_path):
    stubs=RUNTIME_STUBS.replace('return id==1?', 'return id==8?"pc\\\\modern\\\\network\\\\lobby\\\\0\\\\lobby_screen":id==1?')
    run_c(tmp_path,stubs + HEADER + without_includes('port/linux/game/frontend_multiplayer.inl') + r'''
int main(void){
    struct widget_instance root={0},list={0},rows[30]={{0}};
    int i;
    root.definition_tag_index=8;root.child=&list;list.parent=&root;
    list.name="lobby_list";list.type=3;list.child=rows;network=1;
    for(i=0;i<30;i++){
        rows[i].name="list_item_1";rows[i].parent=&list;rows[i].visible=1;
        rows[i].vertical_offset=110+30*i;rows[i].next=i<29?rows+i+1:0;
    }
    list.focused_child=rows+29;frontend_multiplayer_update(&list);
    assert(rows[29].vertical_offset==350&&frontend_multiplayer_drawable(rows+29));
    assert(!frontend_multiplayer_drawable(rows));
    for(i=0;i<30;i++)assert(rows[i].visible&&rows[i].next==(i<29?rows+i+1:0));
    list.focused_child=rows;frontend_multiplayer_update(&list);
    assert(rows[0].vertical_offset==110&&rows[8].vertical_offset==350);
    rows[0].visible=0;list.focused_child=rows+1;frontend_multiplayer_update(&list);
    assert(rows[1].vertical_offset==110&&!rows[0].visible);
    main_menu=0;list.focused_child=rows+29;frontend_multiplayer_update(&list);
    assert(rows[1].vertical_offset==110);main_menu=1;
    available=0;frontend_multiplayer_update(&list);assert(rows[1].vertical_offset==110);
    return 0;
}
''')


def test_geometry_contains_rows_at_all_aspects(tmp_path):
    run_c(tmp_path,r'''
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#define NONE -1
static int frontend_menu_available(void){return 1;}
static long tag_loaded(long g,char const *n){(void)g;(void)n;return 0;}
static char const *tag_name="";
static char const *tag_get_name(long t){(void)t;return tag_name;}
static long halo_screen_width(void){return 852;}
''' + HEADER + source('port/linux/game/frontend_layout.c') + r'''
int main(void){
    int sizes[]={640,768,852,1120,1706},i,j;
    short left,right,shift;
    for(i=0;i<5;i++)for(j=0;j<(int)(sizeof(frontend_network_widgets)/sizeof(frontend_network_widgets[0]));j++){
        int role=frontend_network_widgets[j].role;
        tag_name=frontend_network_widgets[j].name;
        assert(frontend_menu_stretch_bitmap(0)==(role!=FRONTEND_NET_DETAIL_FIXED));
        left=0;right=336;shift=0;
        frontend_layout_geometry(frontend_network_widgets[j].name,sizes[i],&left,&right,&shift);
        if(role==FRONTEND_NET_ROW){
            int extra=frontend_layout_width(sizes[i])-640;
            assert(28+shift>=-(sizes[i]-640)/2);
            assert(28+shift+right==364-extra/2+extra*3/5);
        }
        if(role==FRONTEND_NET_FULL)assert(right-left==sizes[i]);
    }
    return 0;
}
''')


def test_difficulty_native_order_and_background_layer():
    doc = ET.Element('menus')
    _, routes = frontend_multiplayer.append(doc)
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    root = widgets[dict(routes)[frontend_multiplayer.PAGES['difficulty'][0]]]
    children = [c.get('widget') for c in root.findall('child')]
    service = next(n for n in children if n.endswith('/difficulty_select_list_screen'))
    panel = next(n for n in children if n.endswith('/help_panel'))
    assert children.index(panel) < children.index(service)
    assert 'nifty_fx' in widgets[panel].get('flags')
    assert widgets[service].get('bitmap') is None
    native_children = [c.get('widget') for c in widgets[service].findall('child')]
    assert native_children[0].endswith('/difficulty_select_list')
    assert native_children[1].endswith('/warning_message')
    warning = widgets[native_children[1]]
    assert warning.get('align') == 'center'
    assert int(warning.get('width')) == 312
    desc = next(w for n,w in widgets.items() if n.startswith('modern/network/difficulty/') and n.endswith('/difficulty_extended_description'))
    assert desc.get('align') == 'center'
    assert desc.get('text_x') == desc.get('text_y') == '0'


def test_campaign_list_keeps_modern_layout_without_legacy_update_callback():
    doc = ET.Element('menus')
    roles, _ = frontend_multiplayer.append(doc)
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    campaign = next(w for name, w in widgets.items()
                    if name.startswith('modern/network/campaign/') and name.endswith('/solo_level_select_list'))
    assert campaign.find("on[@event='created']").get('run') == 'initialize sp level list solo'
    assert campaign.find("data[@input='port frontend multiplayer update']") is not None
    rows = [widgets[c.get('widget')] for c in campaign.findall('child')
            if '/list_item_' in c.get('widget')]
    assert len(rows) == 11
    assert all(dict(roles)[row.get('name')] == 'row' for row in rows)
    assert all(row.get('width') == '336' for row in rows)
