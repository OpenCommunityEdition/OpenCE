"""Real diagnostic formatters, failed-load lifetime and unchanged fatal UI."""
from pathlib import Path
import subprocess

from harness import function, structure
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"


def run_c(tmp_path, code):
    source = tmp_path / "diagnostics.c"
    source.write_text(code)
    binary = compile_tool(tmp_path, [source, GAME / "mcc_errors.c", GAME / "custom_edition_errors.c"])
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_family_messages_are_bounded_and_keep_actionable_reasons(tmp_path):
    run_c(tmp_path, r'''
#include "mcc_errors.h"
#include "custom_edition_errors.h"
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#define CHECK(c) do {if(!(c)){fprintf(stderr,"line %d\n",__LINE__);return 1;}}while(0)
int main(void) {
    char text[512], tiny[4]={'x','x','x','!'}, long_name[1000];
    wchar_t wide[512];
    memset(long_name,'A',sizeof(long_name));long_name[999]=0;
    custom_edition_error_message("a10",custom_edition_status_explanation(_cache_file_status_opensauce_cache),text,sizeof(text));
    CHECK(strstr(text,"OpenSauce") && strstr(text,"a10"));
    custom_edition_error_message("b30",custom_edition_status_explanation(_cache_file_status_tag_cache_overflow),text,sizeof(text));
    CHECK(strstr(text,"tag-cache limit") && !strstr(text,"Close other"));
    mcc_error_message("mcc_maps/pack/a10",mcc_status_explanation(MCC_CACHE_WRONG_VERSION),text,sizeof(text));
    CHECK(strstr(text,"pack/a10") && strstr(text,"version 13") && !strstr(text,"custom_maps"));
    mcc_error_text("mcc_maps/other/a10","Resource sounds.map is invalid.",wide,512);
    CHECK(wcsstr(wide,L"other/a10") && wcsstr(wide,L"sounds.map"));
    mcc_error_message(long_name,"Install matching resources.",text,sizeof(text));
    CHECK(strstr(text,"Install matching resources.") && strlen(text)<=110);
    mcc_error_message("mcc_maps/long_package_name/long_level_name",long_name,text,sizeof(text));
    CHECK(strstr(text,"long_packa...") && strstr(text,"_level_name") && strstr(text,"See debug.txt."));
    CHECK(strlen(text)<=110);
    mcc_error_message("mcc_maps/p/a\n%ls", "bad\tresource",text,sizeof(text));
    CHECK(!strchr(text,'\n') && !strchr(text,'\t') && strstr(text,"%ls"));
    mcc_error_message(NULL,NULL,tiny,3);CHECK(!tiny[2] && tiny[3]=='!');
    custom_edition_error_message(NULL,NULL,tiny,1);CHECK(!tiny[0]);
    mcc_error_message(NULL,NULL,NULL,0);custom_edition_error_message(NULL,NULL,NULL,0);
    return 0;
}
''')


def test_mcc_failure_reason_survives_cleanup_and_next_attempt_is_fresh(tmp_path):
    actual = function((GAME / "mcc_cache.c").read_text(), "mcc_cache_tags_load")
    run_c(tmp_path, r'''
#include "mcc_runtime.h"
#include "mcc_errors.h"
#include "mcc_loading.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
typedef int boolean;
#define TRUE 1
#define NULL_HEADER ((struct cache_file_tag_header *)0)
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define _error_silent 0
struct cache_file_tag_header {int unused;};
static struct mcc_runtime mcc_loaded_cache;
static FILE *mcc_stream;
static int mcc_loaded, fail, popups, cleanups;
static wchar_t queued[512];
static void error(int p,char const *f,...) {(void)p;(void)f;}
static char const *mcc_identity_local_name(char const *s) {return s;}
static void mcc_cache_tags_unload(void) {
    if(mcc_loaded_cache.tags)free(mcc_loaded_cache.tags);
    memset(&mcc_loaded_cache,0,sizeof(mcc_loaded_cache));mcc_loaded=0;cleanups++;
}
static FILE *mcc_open_explain(char const *n,struct mcc_cache_source *s,struct mcc_cache_identity *i,char *reason,size_t size) {
    (void)n;(void)s;memset(i,0,sizeof(*i));i->tag_data_size=64;
    if(fail==1){snprintf(reason,size,"Its .map file is missing.");return NULL;}
    return (FILE *)1;
}
static int mcc_package_name(char const *name,char *out) {strcpy(out,name);return 1;}
enum mcc_cache_status mcc_cache_load(struct mcc_cache_source const *s,void *raw,size_t size,struct mcc_cache_report *r) {
    (void)s;(void)raw;(void)size;memset(r,0,sizeof(*r));return fail==2 ? MCC_CACHE_BAD_BSP : MCC_CACHE_OK;
}
char const *mcc_cache_status_string(enum mcc_cache_status s) {(void)s;return "technical reason";}
static int mcc_resources_select(struct mcc_runtime *r,void *raw) {
    (void)raw;if(fail==3){strcpy(r->failure,"Resource sounds.map is invalid.");return 0;}return 1;
}
static int mcc_resources_checksum(struct mcc_runtime *r,unsigned mask,uint32_t *sum) {(void)r;(void)mask;*sum=1;return fail!=4;}
static int mcc_identity_loaded(char const *name,struct mcc_runtime *r) {(void)name;(void)r;return fail!=5;}
static void *mcc_memory_reserve(uint32_t a,uint32_t s) {(void)a;(void)s;return fail==6 ? NULL : calloc(1,128);}
static int mcc_scenario_identity(struct mcc_runtime *r,char const *n) {(void)r;(void)n;return fail!=7;}
static int mcc_hud_bitmaps_prepare(struct mcc_runtime *r) {(void)r;return 1;}
static int mcc_tags_prepare(struct mcc_runtime *r) {(void)r;return fail!=8;}
static int mcc_scripts_prepare(struct mcc_runtime *r) {
    if(fail==9){strcpy(r->failure,"Unsupported script: fixture_function.");return 0;}return 1;
}
int mcc_geometry_prepare(struct mcc_runtime *r) {(void)r;return fail!=10;}
int mcc_audio_prepare(struct mcc_runtime *r) {(void)r;return fail!=11;}
int mcc_bitmaps_prepare(struct mcc_runtime *r) {(void)r;return fail!=12;}
void mcc_bitmaps_prepare_gpu(struct mcc_runtime *r) {(void)r;}
static int mcc_tags_validate(struct mcc_runtime *r) {(void)r;return fail!=13;}
int mcc_runtime_read(struct mcc_runtime *r,uint32_t off,uint32_t size,void *out) {(void)r;(void)off;memset(out,0,size);return fail!=14;}
static int mcc_script_threads_prepare(void) {return fail!=15;}
static void mcc_campaign_begin(void) {}
void mcc_loading_stage(enum mcc_loading_stage s) {(void)s;}
void mcc_loading_update(enum mcc_loading_stage s,unsigned long a,unsigned long b) {(void)s;(void)a;(void)b;}
static void display_error_text_when_main_menu_loaded(wchar_t const *s) {wcscpy(queued,s);popups++;}
''' + actual + r'''
#define CHECK(c) do {if(!(c)){fprintf(stderr,"line %d fail=%d\n",__LINE__,fail);return 1;}}while(0)
int main(void) {
    unsigned char header[2048];
    wchar_t const *reason[]={L"",L"missing",L"geometry",L"sounds.map",L"read completely",L"changed after joining",
        L"memory address",L"saved-game",L"converted",L"fixture_function",L"geometry",L"sounds",L"textures",L"validation",L"header",L"memory"};
    for(fail=1;fail<=15;fail++) {
        CHECK(mcc_cache_tags_load("mcc_maps/package/a10",header)==NULL_HEADER);
        CHECK(wcsstr(queued,L"package/a10") && wcsstr(queued,reason[fail]));
        CHECK(!mcc_loaded_cache.failure[0] && !mcc_loaded_cache.tags && !mcc_loaded);
    }
    CHECK(popups==15 && cleanups==30);
    fail=0;queued[0]=0;
    CHECK(mcc_cache_tags_load("mcc_maps/other/a10",header)!=NULL_HEADER && mcc_loaded && !queued[0]);
    mcc_cache_tags_unload();
    return 0;
}
''')


def test_fatal_media_text_preserves_widget_and_loop_lifecycle(tmp_path):
    ui = (ROOT / "source/interface/ui_widget.c").read_text()
    actual = "\n".join(function(ui, name) for name in (
        "display_error_abort_to_dashboard", "display_error_damaged_media", "display_error_damaged_media_text"))
    run_c(tmp_path, r'''
#include <stddef.h>
#include <wchar.h>
#include <string.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define _ui_widget_type_text_box 1
#define _error_media_damaged 2
#define _error_silent 0
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define match_vassert(a,b,c,d) ((void)0)
#define ustrncpy wcsncpy
struct widget_instance {int type;struct {struct {int string_list_index;} text_box;} parameters;int widget_is_error_dialog;};
static struct widget_instance widget, *ui_widget_port_error_text_box;
static wchar_t const *ui_widget_port_fatal_text,*ui_widget_port_error_text;
static int dashboard_abort_error,closes,loops,ends,missing;
static void error(int p,char const *f,...) {(void)p;(void)f;}
static void ui_widgets_close_all(void) {closes++;ui_widget_port_error_text=NULL;ui_widget_port_error_text_box=NULL;}
static struct widget_instance *ui_widget_load_by_name_or_tag(char const *n,int a,void *b,int c,int d,int e,int f) {
    (void)n;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;widget.type=1;return missing ? NULL : &widget;
}
static void input_frame_end(void) {ends++;}
static void main_loop_of_death(void) {loops++;}
''' + actual + r'''
int main(void) {
    wchar_t text[32]=L"Missing bitmaps.map";
    display_error_damaged_media_text(text);text[0]='X';
    if(closes!=1 || loops!=1 || ends!=1 || dashboard_abort_error!=2 || !widget.widget_is_error_dialog)return 1;
    if(ui_widget_port_fatal_text || ui_widget_port_error_text_box!=&widget || wcscmp(ui_widget_port_error_text,L"Missing bitmaps.map"))return 2;
    display_error_damaged_media();
    if(closes!=2 || loops!=2 || ends!=2 || ui_widget_port_error_text)return 3;
    missing=1;display_error_damaged_media_text(L"another failure");
    if(ui_widget_port_fatal_text || ui_widget_port_error_text || loops!=3)return 4;
    return 0;
}
''')


def test_main_menu_queue_copies_and_keeps_first_failure(tmp_path):
    actual = function((ROOT / "source/interface/ui_widget.c").read_text(), "display_error_text_when_main_menu_loaded")
    run_c(tmp_path, r'''
#include <stddef.h>
#include <wchar.h>
#define NONE -1
#define _error_silent 0
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define ustrncpy wcsncpy
#define _error_cannot_create_saved_game_file_with_empty_name 10
static struct {int main_menu_deferred_error_code;} widget_globals={NONE};
static wchar_t const *ui_widget_port_error_pending_text;
static void error(int p,char const *f,...) {(void)p;(void)f;}
''' + actual + r'''
int main(void) {
    wchar_t reason[64]=L"Resource sounds.map is invalid";
    display_error_text_when_main_menu_loaded(reason);reason[0]='X';
    display_error_text_when_main_menu_loaded(L"generic recovery fallback");
    if(wcscmp(ui_widget_port_error_pending_text,L"Resource sounds.map is invalid"))return 1;
    widget_globals.main_menu_deferred_error_code=NONE;
    display_error_text_when_main_menu_loaded(L"next attempt");
    return wcscmp(ui_widget_port_error_pending_text,L"next attempt")!=0;
}
''')


def test_ce_load_failure_is_consumed_without_changing_cleanup(tmp_path):
    cache = (GAME / "custom_edition_cache.c").read_text()
    types = "\n".join(structure(cache, name) for name in (
        "custom_edition_file", "custom_edition_cache_globals"))
    actual = "\n".join(function(cache, name) for name in (
        "custom_edition_cache_tags_load", "custom_edition_cache_load_error"))
    run_c(tmp_path, r'''
#include "custom_edition_errors.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef int boolean;
typedef unsigned char byte;
#define TRUE 1
#define FALSE 0
#define MAP_PATH_SIZE 256
#define READ_STAGING_BYTES 65536
#define COMBINED_DECODED_OFFSET 0x30000000u
#define COMBINED_BITMAPS_OFFSET 0x40000000u
#define COMBINED_SOUNDS_OFFSET 0x60000000u
#define COMBINED_OFFSET_LIMIT 0x80000000u
#define CACHE_FILE_HEADER_FILE_LENGTH_OFFSET 8
#define _error_silent 0
#define csstrcasecmp strcmp
struct cache_file_tag_header {int unused;};
''' + types + r'''
static struct custom_edition_cache_globals custom_edition_cache_globals;
static char custom_edition_failure_name[64],custom_edition_failure_reason[320];
static int fail,closes,disposals;
static unsigned char tags[128];
static int custom_edition_level_name(char const *s) {return !strncmp(s,"custom_maps\\",12);}
static char const *tag_name_strip_path(char const *s) {char const *p=strrchr(s,'\\');return p ? p+1 : s;}
static unsigned char *halo_custom_edition_tag_cache(void) {return fail==1 ? NULL : tags;}
static char const *custom_edition_unavailable_reason(void) {return "Custom Edition maps are turned off.";}
static void error(int p,char const *f,...) {(void)p;(void)f;}
static int custom_edition_map_path(char const *n,char *p) {strcpy(p,n);return fail!=2;}
static int read_file(void *c,uint32_t off,uint32_t size,void *out) {(void)c;(void)off;memset(out,0,size);return 1;}
static int custom_edition_file_open(struct custom_edition_file *f,char const *p) {(void)p;f->source.read=read_file;return 1;}
enum cache_file_status cache_file_identify(struct cache_file_source *s,struct cache_file_identity *i) {
    (void)s;memset(i,0,sizeof(*i));i->format=_cache_file_format_custom_edition_cache;
    return fail==3 ? _cache_file_status_opensauce_cache : _cache_file_status_ok;
}
char const *cache_file_status_describe(enum cache_file_status s) {(void)s;return "technical check";}
static void custom_edition_cache_files_close(void) {memset(&custom_edition_cache_globals,0,sizeof(custom_edition_cache_globals));closes++;}
static int custom_edition_resource_map_path(enum resource_map_type t,char *p) {(void)t;strcpy(p,"resource");return 1;}
enum cache_file_status resource_map_open(struct cache_file_source *s,enum resource_map_type t,struct resource_map *m) {
    (void)s;(void)t;(void)m;return _cache_file_status_ok;
}
enum cache_file_status custom_edition_cache_load(struct cache_file_source *s,struct resource_map *const *m,
    uint8_t *data,uint32_t size,struct custom_edition_load_report *r) {
    (void)s;(void)m;(void)data;(void)size;memset(r,0,sizeof(*r));
    return fail==4 ? _cache_file_status_tag_cache_overflow : _cache_file_status_ok;
}
static void custom_edition_cache_report_log(struct custom_edition_load_report const *r) {(void)r;}
static int custom_edition_cache_tags_convert(uint8_t *data,struct custom_edition_load_report const *r) {
    (void)data;(void)r;if(fail==5){strcpy(custom_edition_failure_reason,"Its scripts could not be prepared.");return 0;}return 1;
}
static void custom_edition_sounds_dispose(void) {disposals++;}
static void custom_edition_models_dispose(void) {disposals++;}
static void custom_edition_bitmaps_dispose(void) {disposals++;}
''' + actual + r'''
#define CHECK(c) do {if(!(c)){fprintf(stderr,"line %d fail=%d\n",__LINE__,fail);return 1;}}while(0)
int main(void) {
    unsigned char header[2048];
    char const *reason[]={"","turned off","missing","OpenSauce","tag-cache limit","scripts"};
    for(fail=1;fail<=5;fail++) {
        CHECK(!custom_edition_cache_tags_load("a10",header));
        CHECK(!strcmp(custom_edition_cache_load_error("levels\\a10\\a10"),"game_load() failed."));
        CHECK(strstr(custom_edition_cache_load_error("custom_maps\\a10"),reason[fail]));
        CHECK(!custom_edition_failure_name[0] && !custom_edition_failure_reason[0]);
    }
    CHECK(closes==3 && disposals==3);
    fail=4;CHECK(!custom_edition_cache_tags_load("a10",header));
    CHECK(!strstr(custom_edition_cache_load_error("custom_maps\\b30"),"tag-cache limit"));
    fail=0;CHECK(custom_edition_cache_tags_load("a10",header)!=NULL);
    CHECK(!custom_edition_failure_name[0] && !custom_edition_failure_reason[0] && custom_edition_cache_globals.tags_loaded);
    return 0;
}
''')
