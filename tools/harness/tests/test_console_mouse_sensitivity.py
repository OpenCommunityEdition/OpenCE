"""Real console/config, sensitivity cache and Mouse Setup code in an isolated fake menu."""

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import ROOT, function, mutated, read  # noqa: E402
from test_console_options import PRELUDE  # noqa: E402
from tools import port_settings  # noqa: E402


def structure(source, name):
    start = source.index(f"struct {name}\n{{")
    return source[start:source.index("};", start) + 2]


def generated(fault=None):
    config = read("port/linux/src/port_config.c")
    config = re.sub(r'^#include "platform.h"\n|^#include <SDL3/SDL.h>\n|^#include <pthread.h>\n', '', config, flags=re.M)
    parser = re.sub(r'^#include "cseries.h"\n|^#include "main/console.h"\n', '', read("port/linux/game/console_options.c"), flags=re.M)
    menu = read("port/linux/game/menu_functions.c")
    layout = structure(menu, "pc_menu_setting")
    assert layout == structure(read("port/linux/game/menu_tags.c"), "pc_menu_setting")
    choices = port_settings.SCREENS['mouse_settings']['rows'][:2]
    assert choices[1][2][0] == ('SAME', '0')
    arrays = ""
    for index, (_, _, values, *_) in enumerate(choices):
        arrays += f"static char const *choices_{index}[] = {{" + ','.join('"' + value + '"' for _, value in values) + "};\n"
        assert values[-1] == ('CUSTOM', 'custom')
    text = PRELUDE + config + parser + '\n#include <wchar.h>\n' + FAKE_WORLD + layout + arrays + WORLD_FUNCTIONS
    text += function(read('port/linux/game/menu_tags.c'), 'setting_add') + '\n'
    for name in ('text_is_number', 'spinner_item', 'setting_value_index', 'setting_custom_load',
                 'pc_menu_setting_custom_text', 'setting_load', 'setting_changed_save', 'setting_default_show'):
        code = function(menu, name)
        if fault and name == fault[0]:
            code = mutated(code, *fault[1:])
        text += code + '\n'
    text += 'static float vertical_sensitivity;\n' + function(read('port/linux/src/xinput_sdl.c'), 'mouse_sensitivity') + '\n'
    # The actual renderer's string-selection path, with tag lookup stubbed.
    render = function(read('source/interface/ui_widget.c'), 'widget_instance_render_spinner_list')
    start = render.index('short string_index = widget->parameters.list.selected_index;')
    end = render.index('length = ustrlen(string);', start) + len('length = ustrlen(string);')
    text += 'static wchar_t *rendered(struct widget_instance *widget, struct test_definition *definition) {\n' \
        + render[start:end] + '\n(void)length; return string; }\n'
    return text + TESTS


FAKE_WORLD = r'''
#define MAXIMUM_STRINGS 64
#define NONE (-1)
#define TRUE 1
#ifdef _WIN32
#define _stricmp _stricmp
#else
#define _stricmp strcasecmp
#endif
typedef unsigned short word;
struct widget_instance {
 long definition_tag_index;struct widget_instance *parent;
 union {struct {short selected_index;word number_of_items;} list;} parameters;
};
struct halo_menu_widget {char const *setting;};
struct test_definition {struct {long index;} text_label_string_list;};
'''

WORLD_FUNCTIONS = r'''
static struct {struct pc_menu_setting *settings;long setting_count;} menu_tags;
static long spinner_split(struct halo_menu_widget const *source, int values, char const **out){
 (void)values;int vertical=!strcmp(source->setting,"input.mouse_vertical_sensitivity");
 char const **choices=vertical?choices_1:choices_0;
 long count=vertical?sizeof(choices_1)/sizeof(*choices_1):sizeof(choices_0)/sizeof(*choices_0);
 for(long i=0;i<count;i++)out[i]=choices[i];return count;
}
static struct pc_menu_setting *pc_menu_setting_get(long index){
 for(long i=0;i<menu_tags.setting_count;i++)if(menu_tags.settings[i].definition_index==index)return &menu_tags.settings[i];return NULL;
}
static int setting_text(char const *name,char *text,unsigned size,int defaults){return defaults?config_default(name,text,size):config_text(name,text,size);}
static int setting_write(char const *name,char const *value){return config_write(name,value);}
static wchar_t *ascii_to_wide(char const *value,wchar_t *text,unsigned size){
 unsigned i=0;for(;value[i]&&i+1<size/sizeof(*text);i++)text[i]=(unsigned char)value[i];text[i]=0;return text;
}
static unsigned long ustrlen(wchar_t const *text){return wcslen(text);}
static wchar_t *spinner_string_list_get_string(long index,short item){
 static wchar_t text[128];struct pc_menu_setting *setting=pc_menu_setting_get(index);
 CHECK(setting);if(item==setting->custom_index)return L"CUSTOM";
 if(index==2&&item==0)return L"SAME";
 return ascii_to_wide(setting->values[item],text,sizeof(text));
}
'''

TESTS = r'''
static int execute(char const *command){int ok;CHECK(console_option_execute(command,&ok));return ok;}
static struct widget_instance reopen(long index){
 struct pc_menu_setting *setting=pc_menu_setting_get(index);
 struct widget_instance spinner={0};spinner.definition_tag_index=index;spinner.parameters.list.number_of_items=setting->value_count;
 CHECK(setting_load(&spinner));return spinner;
}
static void shown(struct widget_instance *spinner,char const *value){
 wchar_t expected[128];struct test_definition definition={{spinner->definition_tag_index}};
 ascii_to_wide(value,expected,sizeof(expected));CHECK(!wcscmp(rendered(spinner,&definition),expected));
}
int main(int argc,char **argv){
 (void)argv;struct halo_menu_widget horizontal={"input.mouse_sensitivity"},vertical={"input.mouse_vertical_sensitivity"};
 setting_add(&horizontal,1);setting_add(&vertical,2);
 struct pc_menu_setting *h=pc_menu_setting_get(1),*v=pc_menu_setting_get(2);
 if(argc>1){CHECK(config_real(horizontal.setting)==.333);CHECK(config_real(vertical.setting)==.777);
  struct widget_instance hs=reopen(1),vs=reopen(2);shown(&hs,"0.333");shown(&vs,"0.777");return 0;}
 FILE *file=fopen("config.toml","wb");CHECK(file);fputs("# kept\n[custom]\nvalue=7\n",file);fclose(file);
 CHECK(console_option_help("mouse_sensitivity"));CHECK(console_option_help("mouse_vertical_sensitivity"));
 CHECK(execute("mouse_sensitivity .333"));CHECK(execute("mouse_vertical_sensitivity 0"));
 CHECK(config_real(horizontal.setting)==.333);CHECK(fabs(mouse_sensitivity()-.333)<1e-7);
 CHECK(vertical_sensitivity==mouse_sensitivity());
 unsigned long generation=config_changes();CHECK(execute("mouse_sensitivity"));CHECK(config_changes()==generation);
 struct widget_instance hs=reopen(1),vs=reopen(2);shown(&hs,"0.333");
 CHECK(hs.parameters.list.selected_index==h->custom_index);CHECK(vs.parameters.list.selected_index==0);
 CHECK(vs.parameters.list.number_of_items==v->custom_index);shown(&vs,"SAME"); /* No redundant custom choice. */
 CHECK(setting_changed_save(&hs,h));CHECK(config_changes()==generation); /* Opening and OK never quantizes. */
 const char *bad[]={"mouse_sensitivity 0","mouse_sensitivity -1","mouse_sensitivity nan",
  "mouse_sensitivity inf","mouse_sensitivity 1e39","mouse_sensitivity 1e-100",
  "mouse_vertical_sensitivity -1","mouse_vertical_sensitivity nan","mouse_sensitivity .5 junk"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)CHECK(!execute(bad[i]));CHECK(config_changes()==generation);
 fail_save=1;CHECK(!execute("mouse_sensitivity .8"));fail_save=0;
 fail_rename=1;CHECK(!execute("mouse_vertical_sensitivity .8"));fail_rename=0;
 CHECK(config_changes()==generation);CHECK(config_real(horizontal.setting)==.333);CHECK(config_real(vertical.setting)==0);
 CHECK(execute("mouse_vertical_sensitivity .777"));CHECK(fabs(mouse_sensitivity()-.333)<1e-7);
 CHECK(fabs(vertical_sensitivity-.777)<1e-7);vs=reopen(2);shown(&vs,"0.777");
 CHECK(setting_default_show(&hs,h));CHECK(setting_changed_save(&hs,h));CHECK(config_real(horizontal.setting)==1);
 CHECK(setting_default_show(&vs,v));CHECK(setting_changed_save(&vs,v));CHECK(config_real(vertical.setting)==0);
 hs=reopen(1);CHECK(hs.parameters.list.number_of_items==h->custom_index);CHECK(hs.parameters.list.selected_index!=h->custom_index);
 CHECK(execute("mouse_sensitivity .333"));hs=reopen(1);shown(&hs,"0.333");
 hs.parameters.list.selected_index=0;CHECK(setting_changed_save(&hs,h));CHECK(config_real(horizontal.setting)==strtod(h->values[0],NULL));
 hs.parameters.list.selected_index=h->custom_index;CHECK(setting_changed_save(&hs,h));CHECK(config_real(horizontal.setting)==.333);
 /* Cancel means no save; reopening still reads the console's value. */
 hs.parameters.list.selected_index=0;hs=reopen(1);shown(&hs,"0.333");
 CHECK(execute("mouse_sensitivity default"));CHECK(config_real(horizontal.setting)==1);
 CHECK(execute("mouse_vertical_sensitivity default"));CHECK(config_real(vertical.setting)==0);
 CHECK(execute("mouse_sensitivity .333"));CHECK(execute("mouse_vertical_sensitivity .777"));
 size_t n;char *text=SDL_LoadFile("config.toml",&n);CHECK(strstr(text,"# kept"));CHECK(strstr(text,"value=7"));free(text);
 free(menu_tags.settings);puts("PASS: saved sensitivity, exact menu rendering, presets/Defaults/Cancel/Same, live cache, invalid values, failed writes, restart");return 0;
}
'''


def check(fault=None):
    with tempfile.TemporaryDirectory(prefix='halo-mouse-console-') as directory:
        work = Path(directory)
        (work / 'console_options.h').write_text(read('port/linux/include/console_options.h'))
        (work / 'test.c').write_text(generated(fault))
        executable = work / 'test'
        command = [os.environ.get('CC', 'clang'), '-m32', '-std=gnu11', '-O2', '-fuse-ld=lld',
                   '-I' + str(work), '-I' + str(ROOT / 'port/linux/src'),
                   '-I' + str(ROOT / 'port/third_party/tomlc17'), str(work / 'test.c'),
                   str(ROOT / 'port/third_party/tomlc17/tomlc17.c'), '-lm', '-o', str(executable)]
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        result = subprocess.run([str(executable)], cwd=work, capture_output=True, text=True, timeout=30)
        if fault:
            assert result.returncode == 1, result.stdout + result.stderr
        else:
            assert result.returncode == 0, result.stdout + result.stderr
            result = subprocess.run([str(executable), '--reload'], cwd=work, capture_output=True, text=True, timeout=30)
            assert result.returncode == 0, result.stdout + result.stderr


def test_saved_sensitivity_and_exact_menu():
    check()


@pytest.mark.parametrize('fault', [
    ('setting_custom_load', 'return setting->custom_index;', 'return 0;'),
    ('pc_menu_setting_custom_text', 'return setting &&', 'return FALSE &&'),
    ('setting_changed_save', 'index == setting->loaded_index', 'FALSE'),
])
def test_negative_control(fault):
    check(fault)
