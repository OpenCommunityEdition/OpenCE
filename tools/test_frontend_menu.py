"""Exercise the production frontend boundary and callbacks without retail maps."""
from pathlib import Path
import json
import re
import xml.etree.ElementTree as ET

import pytest

import frontend_menu
from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def without_includes(path):
    source = (ROOT / path).read_text()
    for name in ('frontend_navigation.inl','frontend_multiplayer.inl','frontend_network_routes.inl'):
        source = source.replace('#include "'+name+'"', (ROOT/'port/linux/game'/name).read_text())
    return re.sub(r'^#include .*$', '', source, flags=re.M)


HEADER = (ROOT / 'port/linux/game/frontend_menu.h').read_text()
POLICY = without_includes('port/linux/game/frontend_menu.c')


def test_policy_map_lifetime_and_fallback(tmp_path):
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <ctype.h>
#define NONE -1
static char const *choice="off";
static char const *legacy_choice="pc";
static int tags, main_menu=1, network;
typedef int boolean;
boolean main_menu_is_active(void) {return main_menu;}
boolean network_game_is_active(void) {return network;}
static char const *tag_get_name(long tag) {return tag==2?"pc\\main_menu\\multiplayer_type_select\\multiplayer_type_select_screen":"unrelated";}
static int csstrcasecmp(char const *a,char const *b) {
    while(*a&&tolower((unsigned char)*a)==tolower((unsigned char)*b)){a++;b++;}
    return tolower((unsigned char)*a)-tolower((unsigned char)*b);
}
static char const *config_string(char const *key) {return !strcmp(key,"display.menus")?legacy_choice:choice;}
static long tag_loaded(long group,char const *name) {(void)group;(void)name;return tags ? 1 : NONE;}
void frontend_input_enable(int enabled) {(void)enabled;}
void frontend_glyphs_dispose(void) {}
static int keyboard_disposals;
void keyboard_glyphs_dispose(void) {keyboard_disposals++;}
void frontend_menu_text_end(void) {}
void frontend_panel_end(void) {}
void frontend_menu_session_reset(void) {}
''' + HEADER + POLICY + r'''
int main(void) {
    int i;
    frontend_menu_map_loaded("ui",0);tags=1;
    assert(!frontend_menu_available());
    assert(!strcmp(frontend_menu_root("stock"),"stock"));
    for(i=0;i<100;i++) {
        choice="modern";frontend_menu_map_loaded("ui",0);
        assert(frontend_menu_available());
        assert(!strcmp(frontend_menu_root("stock"),"stock"));
        assert(frontend_menu_destination(2)==1&&frontend_menu_destination(3)==3);
        main_menu=0;assert(frontend_menu_destination(2)==2);main_menu=1;
        network=1;assert(frontend_menu_destination(2)==2);network=0;
        choice="xbox";assert(frontend_menu_destination(2)==2);assert(!strcmp(frontend_menu_root("stock"),FRONTEND_MENU_XBOX));
        choice="pc";assert(!strcmp(frontend_menu_root("stock"),FRONTEND_MENU_PC));
        tags=0;assert(!strcmp(frontend_menu_root("stock"),"stock"));tags=1;
        legacy_choice="xbox";tags=0;
        assert(!strcmp(frontend_menu_root(FRONTEND_MENU_PC),FRONTEND_MENU_XBOX));
        legacy_choice="XBOX";assert(!strcmp(frontend_menu_root(FRONTEND_MENU_PC),FRONTEND_MENU_XBOX));
        legacy_choice="pc";tags=1;
        frontend_menu_unloaded();assert(!frontend_menu_available());
        frontend_menu_map_loaded("a10",0);assert(!frontend_menu_available());
        choice="modern";assert(!strcmp(frontend_menu_root("stock"),"stock"));
        frontend_menu_map_loaded("custom_maps\\test",0);assert(!frontend_menu_available());
        frontend_menu_map_loaded("ui",1);assert(!frontend_menu_available()&&frontend_menu_destination(2)==2);
        frontend_menu_map_loaded("mcc_maps\\a10",1);assert(!frontend_menu_available());
        frontend_menu_map_loaded("ui",0);assert(frontend_menu_available());
    }
    assert(keyboard_disposals==100);
    choice="invalid";frontend_menu_map_loaded("ui",0);assert(!frontend_menu_available());
    return 0;
}
''')


def test_runtime_guards_staging_cancel_failure_and_reentry(tmp_path):
    source = without_includes('port/linux/game/frontend_menu_runtime.inl')
    pages = (ROOT / 'port/linux/game/frontend_pages.inl').read_text()
    run_c(tmp_path, RUNTIME_STUBS + HEADER + pages + source + r'''
int main(void) {
    struct widget_instance root={0},spinner={0},child={0},item={0},list={0},rows[20]={{0}};
    struct event_record event={0};int deleted=0,i;
    root.definition_tag_index=1;root.child=&child;child.parent=&root;child.name="legacy";
    frontend_menu_attach(&root);assert(child.next==&entry&&entry.previous==&child&&entry.parent==&root);
    assert(entry.horizontal_offset==450);
    child.type=3;child.child=&item;root.focused_child=&entry;
    assert(frontend_menu_event(&entry,"port frontend focus",&deleted));
    assert(root.focused_child==&child&&child.focused_child==&item);
    event.type=3;event.data.button.index=3;event.data.button.value=1;
    assert(frontend_menu_shortcut(&root,&event));
    event.data.button.value=2;assert(!frontend_menu_shortcut(&root,&event));event.data.button.value=1;
    root.definition_tag_index=2;assert(frontend_menu_shortcut(&root,&event));
    root.definition_tag_index=4;assert(!frontend_menu_shortcut(&root,&event));
    root.definition_tag_index=3;root.child=&spinner;spinner.parent=&root;spinner.type=2;spinner.definition_tag_index=5;
    spinner.parameters.list.selected_index=2;
    assert(frontend_capture(&root));assert(frontend_session.count==1&&!writes);
    /* Recreated page gets its pending value, including Cancel after a tab switch. */
    spinner.parameters.list.selected_index=0;frontend_restore(&spinner,&setting);
    assert(spinner.parameters.list.selected_index==2);
    assert(frontend_menu_event(&spinner,"port frontend cancel",&deleted));
    assert(!writes&&transitions==1&&!deleted&&!frontend_session.count);
    fail=1;assert(!frontend_menu_event(&spinner,"port frontend apply",&deleted));
    assert(message==1&&transitions==1&&!display&&frontend_session.count==1);
    fail=0;assert(frontend_menu_event(&spinner,"port frontend apply",&deleted));
    assert(transitions==2&&display==1&&!frontend_session.count);
    /* Returning a choice to its baseline must not rewrite config. */
    spinner.parameters.list.selected_index=2;assert(frontend_capture(&root));
    spinner.parameters.list.selected_index=0;assert(frontend_capture(&root));
    i=writes;assert(frontend_commit()&&writes==i);frontend_menu_session_reset();
    spinner.parameters.list.selected_index=9;assert(!frontend_capture(&root));
    /* Keyboard focus can reach all 20 rows; rendering and pointer targets
       contain only the eight-row window, including after a reverse/wrap. */
    root.child=&list;list.parent=&root;list.name="options";list.child=rows;
    for(i=0;i<20;i++){rows[i].name="row_test";rows[i].visible=1;rows[i].parent=&list;rows[i].next=i<19?&rows[i+1]:0;}
    list.focused_child=&rows[0];frontend_menu_help(&list);
    assert(rows[0].vertical_offset==137&&frontend_menu_row_drawable(&rows[7])&&!frontend_menu_row_drawable(&rows[8]));
    list.focused_child=&rows[19];frontend_menu_help(&list);
    assert(frontend_session.first[0]==12&&!frontend_menu_row_drawable(&rows[11])&&frontend_menu_row_drawable(&rows[19]));
    list.focused_child=&rows[2];frontend_menu_help(&list);assert(frontend_session.first[0]==2);
    list.focused_child=&rows[0];frontend_menu_help(&list);assert(!frontend_session.first[0]);
    /* Per-page offsets and pending values cannot leak past unload/reset. */
    frontend_session.first[1]=5;frontend_session.owns_profile=1;frontend_menu_session_reset();
    assert(!frontend_session.first[1]&&ended==1);
    for(i=0;i<3;i++) {
        available=i!=0;main_menu=i!=1;network=i==2;
        assert(!frontend_menu_event(&spinner,"port frontend apply",&deleted));
        root.definition_tag_index=1;assert(!frontend_menu_shortcut(&root,&event));
        root.definition_tag_index=3;
    }
    assert(writes==2&&transitions==2);
    return 0;
}
''')


RUNTIME_STUBS = r'''
#define PIN(x,a,b) ((x)<(a)?(a):(x)>(b)?(b):(x))
#include <assert.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#define NONE -1
#define TRUE 1
#define FALSE 0
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define ROW_TEXT_LENGTH 64
#define _gamepad_analog_button_y 3
typedef int boolean;
struct widget_instance {
    long definition_tag_index;short local_player_index,horizontal_offset,vertical_offset,type;
    char const *name;int visible,disabled;float alpha_modifier;
    struct widget_instance *parent,*previous,*next,*child,*focused_child;
    struct {struct {short selected_index;struct widget_instance *extended_description;} list;
            struct {short string_list_index;wchar_t *text;} text_box;} parameters;
    struct {short current_frame_index;} animation;
};
struct controller_device {int family;};
static short controller_of(struct widget_instance *w) {(void)w;return 0;}
static void controller_device_get(short port,struct controller_device *d) {(void)port;d->family=0;}
static int controller_button_style_index(char const *name) {return strcmp(name,"classic")?3:0;}
struct event_record {short type,controller_index;struct {struct {unsigned char index,value;} button;} data;};
struct pc_menu_setting {short value_count,loaded_index;char const *setting;char const *values[3];};
static int available=1,main_menu=1,network=0,writes=0,transitions=0,opens=0,message=0,fail=0,display=0,ended=0;
static struct widget_instance entry={0};
static struct pc_menu_setting setting={3,0,"display.menu_frontend",{"modern","pc","xbox"}};
int frontend_menu_available(void) {return available;}
static int main_menu_is_active(void) {return main_menu;}
boolean network_game_is_active(void) {return network;}
static char const *tag_get_name(long id) {return id==1?"pc\\main_menu\\main_menu":id==2?"ui\\shell\\main_menu\\main_menu":id==3?"pc\\modern\\settings":id==7?"pc\\modern\\audio\\settings":"pc\\lobby";}
static struct widget_instance *ui_widget_load_by_name_or_tag(char const *name,long id,struct widget_instance *parent,short player,long a,long b,short c) {
    (void)name;(void)id;(void)player;(void)a;(void)b;(void)c;opens++;entry.parent=parent;return &entry;
}
static int spinner_item(struct widget_instance *widget) {(void)widget;return 0;}
static struct pc_menu_setting *pc_menu_setting_get(long id) {return id==5?&setting:0;}
static int setting_write(char const *key,char const *value) {assert(!strcmp(key,"display.menu_frontend"));assert(!strcmp(value,"xbox"));writes++;return !fail;}
static struct widget_instance *screen_of(struct widget_instance *widget) {while(widget->parent)widget=widget->parent;return widget;}
static struct widget_instance *named(struct widget_instance *root,char const *name,int n) {(void)root;(void)name;(void)n;return 0;}
static void text_set(struct widget_instance *widget,wchar_t const *text) {(void)widget;if(wcsstr(text,L"Save failed"))message++;}
static void platform_display_apply(void) {display++;}
static void main_goto_main_menu(void) {transitions++;}
static void player_ui_end_editing_profile(void) {ended++;}
static int player_ui_edit_profile_is_dirty(void) {return 0;}
static int player_ui_save_profile(void) {return 1;}
static int pc_menu_profile_edit_begin(void) {return 1;}
static int setting_load(struct widget_instance *widget) {widget->parameters.list.selected_index=0;setting.loaded_index=0;return 1;}
static boolean setting_default_show(struct widget_instance *widget,struct pc_menu_setting *s) {(void)widget;(void)s;return 1;}
static void settings_each(struct widget_instance *widget,boolean(*visit)(struct widget_instance *,struct pc_menu_setting *)) {(void)widget;(void)visit;}
static int game_function(char const *s,struct widget_instance *w,struct event_record *e,boolean *d) {(void)s;(void)w;(void)e;(void)d;return 1;}
boolean ui_widget_port_open(struct widget_instance *w,char const *s,boolean *d) {(void)w;(void)s;(void)d;return 1;}
static void settings_help(struct widget_instance *w) {(void)w;}
static void master_servers_show(struct widget_instance *w) {(void)w;}
static int usnprintf(wchar_t *out,int n,wchar_t const *format,...) {(void)n;(void)format;out[0]=0;return 0;}
static long tag_loaded(long group,char const *path) {(void)group;(void)path;return -1;}
static wchar_t const *unicode_string_list_get_string(long tag,short i) {(void)tag;(void)i;return L"";}
'''


@pytest.mark.parametrize('broken', [0, 1])
def test_separate_parser_cache_and_failure(tmp_path, broken):
    legacy = function((ROOT / 'port/linux/src/menu_files.c').read_text(), 'halo_menus_load')
    frontend = function((ROOT / 'port/linux/src/frontend_menu_files.inl').read_text(), 'frontend_menu_files_load')
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
struct halo_menus {int widget_count;char const *root;};
struct reader {struct halo_menus menus;int failed;void *last_child,*last_handler,*last_input,*last_conditional,*last_replace;};
struct menu_file {char const *path;};
static struct menu_file files[]={{"ce/main.xml"},{"modern/settings.xml"},{"modern/panel.png"}};
static int file_count=3,gathers=0,modern_reads=0,reads=0;
static void files_gather(void) {gathers++;}
static void read_file(struct reader *reader,struct menu_file *file) {
    reads++;reader->menus.widget_count++;reader->menus.root="main_menu/main_menu";
    if(!strncmp(file->path,"modern/",7)) {modern_reads++;reader->failed=BROKEN;}
}
static void platform_log(char const *text) {(void)text;}
static int config_boolean(char const *key) {(void)key;return 1;}
static void changelog_menu_configure(struct halo_menus *menus,int enabled) {(void)menus;(void)enabled;}
'''.replace('BROKEN', str(broken)) + legacy + frontend + r'''
int main(void) {
    struct halo_menus const *legacy=halo_menus_load(),*frontend;
    assert(legacy&&legacy->widget_count==1&&!modern_reads);
    frontend=frontend_menu_files_load();assert(frontend);
    assert(gathers==1&&modern_reads==1&&reads==3);
    assert(frontend_menu_files_load()==frontend&&halo_menus_load()==legacy&&reads==3);
    assert(legacy->widget_count==1);
    assert(frontend->widget_count==EXPECTED);
    return 0;
}
'''.replace('EXPECTED', '1' if broken else '2'))


def test_interface_pending_hud_gate_family_changes_and_focus(tmp_path):
    source = (ROOT / 'port/linux/game/frontend_menu_runtime.inl').read_text()
    stubs = RUNTIME_STUBS.replace('d->family=0;', 'd->family=test_family;')
    stubs = 'static int test_family;\n' + stubs.replace(
        'return id==5?&setting:0;', 'return id>=10&&id<18?&options[id-10]:id==5?&setting:0;')
    stubs = stubs.replace('static int available=1', 'static struct pc_menu_setting options[8];\nstatic int available=1')
    run_c(tmp_path, stubs + ''.join(function(source, name) for name in
          ('frontend_setting_row', 'frontend_row_value', 'frontend_button_rows_show')) + r'''
int main(void) {
    struct widget_instance list={0},rows[8]={{0}},spinners[8]={{0}};
    char const *keys[]={"display.high_res_hud","input.auto_button_detection",
        "display.hud_button_style","display.hud_button_style_xbox","display.hud_button_style_ps",
        "display.hud_button_style_switch","display.hud_bumper_style","display.keyboard_glyphs"};
    int i,family,repeat;
    list.child=rows;
    for(i=0;i<8;i++) {
        rows[i].child=&spinners[i];rows[i].visible=1;rows[i].next=i<7?&rows[i+1]:0;
        spinners[i].type=2;spinners[i].definition_tag_index=10+i;
        options[i].setting=keys[i];options[i].value_count=2;
        options[i].values[0]=i<2?"true":"classic";
        options[i].values[1]=i<2?"false":"keyboard";
    }
    for(repeat=0;repeat<10;repeat++) for(family=-1;family<5;family++) {
        int selected=family>0&&family<=3?family:0;
        test_family=family;spinners[0].parameters.list.selected_index=0;
        spinners[1].parameters.list.selected_index=0;list.focused_child=&rows[2];
        frontend_button_rows_show(&list);
        assert(rows[1].visible&&rows[7].visible&&rows[6].visible);
        for(i=0;i<4;i++) assert(rows[2+i].visible==(i==selected));
        assert(list.focused_child==&rows[2+selected]);
        assert(list.parameters.list.selected_index==2+selected);
        /* Manual mode restores its own choice, including Keyboard. */
        spinners[1].parameters.list.selected_index=1;
        spinners[2].parameters.list.selected_index=1;
        frontend_button_rows_show(&list);
        assert(rows[2].visible&&!rows[7].visible&&!rows[6].visible);
        for(i=3;i<6;i++) assert(!rows[i].visible);
        /* Pending HUD off hides all dependent rows and recovers focus. */
        spinners[0].parameters.list.selected_index=1;
        frontend_button_rows_show(&list);
        for(i=1;i<8;i++) assert(!rows[i].visible);
        assert(list.focused_child==rows&&!list.parameters.list.selected_index);
        assert(spinners[2].parameters.list.selected_index==1&&!writes);
        /* Restoring HUD does not discard the pending manual choice. */
        spinners[0].parameters.list.selected_index=0;
        frontend_button_rows_show(&list);
        assert(rows[1].visible&&rows[2].visible&&!rows[6].visible);
        spinners[2].parameters.list.selected_index=0;
    }
    return 0;
}
''')


def test_interface_rendering_order_and_unique_placement():
    doc = ET.fromstring(frontend_menu.document())
    settings = [w.get('setting') for w in doc.findall('widget')
                if w.get('name', '').startswith('modern/interface/') and w.get('setting')]
    assert settings[:4] == ['display.menu_frontend', 'display.high_res_text',
                            'display.high_res_hud', 'input.auto_button_detection']
    for key in ('display.high_res_text', 'display.high_res_hud'):
        matches = [w for w in doc.findall('widget') if w.get('setting') == key]
        assert len(matches) == 1
        assert matches[0].get('name').startswith('modern/interface/')


def test_generated_assets_routes_and_platform_filter(tmp_path):
    menus = ROOT / 'port/assets/menus'
    (tmp_path / 'menus.json').write_text('{"files": ["ce/untouched.xml"]}')
    frontend_menu.generate(tmp_path)
    frontend_menu.generate(tmp_path)
    for path in (tmp_path / 'modern').iterdir():
        assert path.read_bytes() == (menus / 'modern' / path.name).read_bytes()
    manifest = json.loads((menus / 'menus.json').read_text())['files']
    doc = ET.parse(menus / 'modern/settings.xml').getroot()
    widgets = {node.get('name'): node for node in doc.findall('widget')}
    destinations = set(widgets)
    for file in (menus / 'ce').glob('*.xml'):
        destinations.update(node.get('name') for node in ET.parse(file).getroot().findall('widget'))
    assert doc.get('root') is None  # must not override the legacy root
    for node in doc.findall('.//child'):
        assert node.get('widget') in destinations
    for node in doc.findall('.//on'):
        if node.get('open'):
            assert node.get('open') in destinations
    for node in doc.findall('.//frame'):
        assert node.get('png') in manifest
    for node in doc.findall('widget'):
        assert 'pause_game' not in node.get('flags', '')
    spinners = [w for w in widgets.values() if w.get('setting') and not w.get('name').startswith('modern/network/')]
    assert len(spinners) >= 90
    for page, spec in enumerate(frontend_menu.PAGES):
        base = frontend_menu.page_base(page)
        screen = widgets[base + '/settings']
        assert screen.find("on[@event='y']").get('run') == 'port frontend next tab'
        assert widgets[base + '/button_defaults'].find("on[@event='a']").get('run') == 'port frontend defaults'
        assert widgets[base + '/button_cancel'].find("on[@event='a']").get('run') == 'port frontend cancel'
        assert len(widgets[base + '/tabs'].findall('child')) == 6
        for name in ('help_value', 'status'):
            assert widgets[base + '/' + name].get('text') is None
            assert widgets[base + '/' + name].get('string_list') is None
    for spinner in spinners:
        path = spinner.get('name')
        row = widgets[path.replace('/spinner_', '/row_')]
        children = row.findall('child')
        assert int(children[0].get('y')) == int(children[1].get('y')) == 3
        assert spinner.get('text_y') is None
        arrows = [list(map(int, spinner.get(name).split())) for name in ('header_bounds', 'footer_bounds')]
        assert sum(a[1] + a[3] for a in arrows) / 4 == int(spinner.get('width')) / 2
        assert all((a[0] + a[2]) / 2 + 3 == int(row.get('height')) / 2 for a in arrows)
        assert 'nifty_fx' in row.get('flags')
    fps = next(w for w in spinners if w.get('setting') == 'display.max_fps')
    assert widgets[fps.get('name').replace('/spinner_', '/row_')].get('platform') == 'desktop web'
    style = next(w for w in spinners if w.get('setting') == 'display.menu_frontend')
    assert style.get('values') == 'modern|pc|xbox|off'
    # The generated C routes and XML must agree, including every subpage.
    routes = (ROOT / 'port/linux/game/frontend_pages.inl').read_text()
    for index in range(len(frontend_menu.PAGES)):
        path = "pc\\" + (frontend_menu.page_base(index) + "/settings").replace("/", "\\")
        assert json.dumps(path) in routes


def test_labels_and_values_fit_narrow_layout():
    doc = ET.fromstring(frontend_menu.document())
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    strings = {s.get('name'): [v.get('text') for v in s] for s in doc.findall('strings')}
    for w in widgets.values():
        if w.get('name').startswith('modern/network/'):
            continue  # Multiplayer contract and geometry have their own tests.
        if w.get('setting') or '/label_' in w.get('name'):
            assert w.get('font') == 'ui\\large_ui'
        if '/label_' in w.get('name') and int(w.get('width')) == 152:
            caption = strings[w.get('string_list')][int(w.get('string_index'))]
            size = 10  # The entire narrow viewport uses the same option face.
            assert frontend_menu.text_width(caption, size) <= 152, caption
        if w.get('setting'):
            size = 10  # The entire narrow viewport uses the same option face.
            for caption in w.get('strings').split('|'):
                assert frontend_menu.text_width(caption, size) <= 136, caption


def test_scroll_hook_precedes_pointer_registration():
    render = function((ROOT / 'source/interface/ui_widget.c').read_text(), 'widget_instance_render_recursive')
    assert render.index('frontend_menu_row_drawable(widget)') < render.index('ui_mouse_note_target(widget, definition, offset)')
