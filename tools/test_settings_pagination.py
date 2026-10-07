"""Exercise the production settings pager without opening a real profile."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools import port_settings

ROOT = Path(__file__).resolve().parents[1]


def block(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


PRELUDE = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define ROW_TEXT_LENGTH 64
#define MAX(a,b) ((a)>(b)?(a):(b))
#define usnprintf swprintf
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
struct widget_instance {
 long definition_tag_index;const char *name;short type,vertical_offset;boolean visible;
 struct widget_instance *parent,*child,*next,*focused_child;
 union {struct {wchar_t *text;short string_list_index;} text_box;
 struct {short selected_index;struct widget_instance *extended_description;} list;} parameters;
};
struct pc_menu_setting {short loaded_index;long value_count;const char **values;};
static const char *modes[]={"borderless","windowed"};
static short slots[12000];static struct pc_menu_setting settings[12000];
static struct widget_instance *named(struct widget_instance *w,const char *name,long nth){
 (void)nth;for(struct widget_instance *r=w->child;r;r=r->next){if(!strcmp(r->name,name))return r;struct widget_instance *c=named(r,name,0);if(c)return c;}return NULL;
}
static short pc_menu_string_index(long i){return slots[i];}
static void text_set(struct widget_instance *w,const wchar_t *s){if(!w->parameters.text_box.text)w->parameters.text_box.text=calloc(64,sizeof(wchar_t));wcscpy(w->parameters.text_box.text,s);}
static boolean spinner_item(struct widget_instance *w){return w->parent&&w->parent->definition_tag_index==w->definition_tag_index;}
static struct pc_menu_setting *pc_menu_setting_get(long i){return i>=6000?&settings[i-6000]:NULL;}
static void settings_help(struct widget_instance *w){(void)w;}
'''

TESTS = r'''
struct menu {struct widget_instance list,pager,buttons;struct widget_instance *rows,*spinners;long count;};
static struct menu make(long n,int packed){
 struct menu m={0};m.count=n;m.rows=calloc(n?n:1,sizeof(*m.rows));m.spinners=calloc(n?n:1,sizeof(*m.spinners));
 m.list.name="options_menu";m.list.visible=1;
 m.pager.name="settings_next_page";m.pager.type=1;m.pager.visible=1;m.pager.definition_tag_index=5500;
 m.pager.parameters.text_box.string_list_index=NONE;slots[5500]=packed;
 m.buttons.name="button_bar";m.buttons.visible=1;
 for(long i=0;i<n;i++){
  m.rows[i].name="op_test";m.rows[i].definition_tag_index=i;m.rows[i].visible=1;slots[i]=i;
  m.rows[i].vertical_offset=73+(packed?i%11*24:i*24);
  m.rows[i].child=&m.spinners[i];m.spinners[i].vertical_offset=m.rows[i].vertical_offset+4;m.spinners[i].name="test_spinner";m.spinners[i].type=2;m.spinners[i].definition_tag_index=i+6000;
  m.spinners[i].parent=&m.rows[i];m.spinners[i].parameters.list.selected_index=0;settings[i].loaded_index=0;settings[i].value_count=2;settings[i].values=modes;
 }
 return m;
}
static void link(struct menu *m){
 m->list.child=m->count?m->rows:&m->pager;
 for(long i=0;i<m->count;i++){m->rows[i].parent=&m->list;m->rows[i].next=i+1<m->count?&m->rows[i+1]:&m->pager;}
 m->pager.parent=&m->list;m->pager.next=&m->buttons;m->buttons.parent=&m->list;
}
static void release(struct menu *m){free(m->pager.parameters.text_box.text);free(m->rows);free(m->spinners);}
static long saved,defaults;
static boolean save(struct widget_instance *s,struct pc_menu_setting *v){saved+=s->parameters.list.selected_index!=v->loaded_index;v->loaded_index=s->parameters.list.selected_index;return TRUE;}
static boolean reset(struct widget_instance *s,struct pc_menu_setting *v){(void)v;defaults++;s->parameters.list.selected_index=0;return TRUE;}
static unsigned long probes;
static void sweep(long n){
 struct menu m=make(n,n>12);link(&m);settings_paginate(&m.list,FALSE);
 long pages=n>12?(n+10)/11:1;
 CHECK(m.pager.visible==(pages>1));
 for(long p=0;p<pages;p++){
  CHECK(m.pager.parameters.text_box.string_list_index==p);
  wchar_t caption[64];swprintf(caption,64,L"Page %ld",(p+1)%pages+1);CHECK(!wcscmp(m.pager.parameters.text_box.text,caption));
  long first=-1,visible=0;
  for(long i=0;i<n;i++){
   boolean expected=n<=12||i/11==p;CHECK(m.rows[i].visible==expected);
   if(expected){if(first<0)first=i;visible++;CHECK(m.rows[i].vertical_offset==73+(n<=12?i:i%11)*24);CHECK(m.spinners[i].vertical_offset==m.rows[i].vertical_offset+4);}
   /* A pending edit on every page must survive both navigation and refresh. */
   m.spinners[i].parameters.list.selected_index=1;
   CHECK(settings[i].loaded_index==0);probes++;
  }
  CHECK(visible<=12);CHECK(pages==1||visible<=11);CHECK(m.buttons.visible);
  if(first>=0)CHECK(m.list.focused_child==&m.rows[first]);
  for(int frame=0;frame<10;frame++)settings_paginate(&m.list,FALSE);
  CHECK(m.pager.parameters.text_box.string_list_index==p);
  CHECK(settings_next_page(&m.pager));
 }
 CHECK(m.pager.parameters.text_box.string_list_index==0);
 saved=0;settings_each(&m.list,save);CHECK(saved==n); /* hidden pages included */
 defaults=0;settings_each(&m.list,reset);CHECK(defaults==n);
 for(long i=0;i<n;i++)CHECK(m.spinners[i].parameters.list.selected_index==0&&settings[i].loaded_index==1);
 release(&m);
}
static void variants(void){
 struct menu m=make(14,1);link(&m);
 /* Desktop Resolution/Window Size are mutually exclusive, one slot. */
 m.rows[0].name="op_mode";m.spinners[0].name="mode_spinner";
 m.rows[1].name="op_resolution";m.rows[2].name="op_window_size";slots[2]=slots[1];
 video_rows_show(&m.list);settings_paginate(&m.list,FALSE);
 CHECK(m.pager.visible&&!m.rows[2].visible&&m.rows[11].visible&&!m.rows[12].visible);
 CHECK(settings_next_page(&m.pager));CHECK(m.rows[12].visible&&m.rows[13].visible);
 /* Display mode changes off page one cannot unhide its unavailable row. */
 m.spinners[0].parameters.list.selected_index=1;video_rows_show(&m.list);settings_paginate(&m.list,FALSE);
 CHECK(!m.rows[1].visible&&!m.rows[2].visible&&m.rows[12].visible);
 for(int frame=0;frame<10;frame++){video_rows_show(&m.list);settings_paginate(&m.list,FALSE);CHECK(m.pager.parameters.text_box.string_list_index==1);}
 CHECK(settings_next_page(&m.pager));
 CHECK(!m.rows[1].visible&&m.rows[2].visible&&m.list.focused_child==&m.rows[0]);
 release(&m);
 /* Platform filtering can leave <=12 rows of a packed desktop layout. */
 m=make(9,1);link(&m);for(long i=0;i<9;i++){slots[i]=i*2;m.rows[i].vertical_offset=73+(i*2)%11*24;m.spinners[i].vertical_offset=m.rows[i].vertical_offset+4;}
 settings_paginate(&m.list,FALSE);CHECK(!m.pager.visible);
 for(long i=0;i<9;i++)CHECK(m.rows[i].visible&&m.rows[i].vertical_offset==73+i*24&&m.spinners[i].vertical_offset==m.rows[i].vertical_offset+4);
 /* A nested screen retains its origin; every descendant moves together. */
 m.list.vertical_offset=100;settings_paginate(&m.list,FALSE);
 for(long i=0;i<9;i++)CHECK(m.rows[i].vertical_offset==173+i*24&&m.spinners[i].vertical_offset==m.rows[i].vertical_offset+4);
 struct widget_instance label={0},item={0};label.vertical_offset=177;item.vertical_offset=179;label.child=&item;
 settings_row_move(&label,24);CHECK(label.vertical_offset==201&&item.vertical_offset==203);
 m.list.focused_child=&m.buttons;settings_paginate(&m.list,FALSE);CHECK(m.list.focused_child==&m.buttons);
 release(&m);
 /* Independent screen/controller instances do not share a current page. */
 struct menu a=make(34,1),b=make(34,1);link(&a);link(&b);
 settings_paginate(&a.list,FALSE);settings_paginate(&b.list,FALSE);settings_next_page(&a.pager);
 CHECK(a.pager.parameters.text_box.string_list_index==1&&b.pager.parameters.text_box.string_list_index==0);
 release(&a);release(&b);
}
int main(void){long counts[]={0,1,11,12,13,22,23,33,34,121,1025};for(unsigned i=0;i<sizeof(counts)/sizeof(*counts);i++)sweep(counts[i]);variants();printf("PASS: %lu production paging probes, 0-1025 settings, 94 pages, wrap/focus/refresh, pending edits, all-page OK/defaults, platform/alternate rows and independent screens\n",probes);}
'''


def markup():
    for name, lines in port_settings.settings_files().items():
        source = '\n'.join(lines)
        assert (ROOT/'port/assets/menus/ce'/name).read_text() == source
    spec = dict(port_settings.SCREENS['video_settings'])
    spec['same_place'] = []
    for count in (12, 13, 23, 1025):
        spec['rows'] = [('TEST:', f'display.test{i}', [('ON', 'true')], 'help', None) for i in range(count)]
        root = ET.fromstring('<menus>' + '\n'.join(port_settings._setting_screen('test', spec)) + '</menus>')
        widgets = root.findall('widget')
        pager = next(w for w in widgets if w.get('name').endswith('/settings_next_page'))
        assert pager.get('string_list') is None  # generated caption; no fixed page table
        assert [e.get('run') for e in pager.findall('on')][:2] == ['port settings next page']*2
        rows = [w for w in widgets if '/op_test' in w.get('name', '')]
        assert [int(w.get('string_index')) for w in rows] == list(range(count))
        assert all(not any(e.get('event') == 'deleted' for e in w.findall('on')) for w in widgets)
        options = next(w for w in widgets if w.get('name').endswith('/options_menu'))
        assert all(int(c.get('y', 0)) <= 414 for c in options.findall('child'))
    print('PASS: regenerated menus; unbounded generated page captions, 1025-slot metadata, compact overflow coordinates, normal A/Start/mouse handlers and no save-on-page/deletion')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', default='clang')
    args = p.parse_args()
    markup()
    source = (ROOT/'port/linux/game/menu_functions.c').read_text()
    body = PRELUDE + '\n'.join(block(source, marker) for marker in (
        'static void settings_each(', 'static void video_rows_show(', 'static boolean settings_row_available(', 'static void settings_row_move(',
        'static void settings_paginate(', 'static boolean settings_next_page(')) + TESTS
    flags = ['-std=gnu11', '-O2', '-fuse-ld=lld'] + (['--target=i686-pc-windows-msvc'] if sys.platform == 'win32' else ['-m32'])
    with tempfile.TemporaryDirectory(prefix='halo-settings-pages-') as d:
        folder = Path(d)
        (folder/'test.c').write_text(body)
        subprocess.run([args.cc, *flags, str(folder/'test.c'), '-o', str(folder/'test.exe')], check=True)
        subprocess.run([str(folder/'test.exe')], check=True, timeout=15)


if __name__ == '__main__':
    main()
