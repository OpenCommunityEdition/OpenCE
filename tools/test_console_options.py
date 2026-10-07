"""Run the production console parser/config writer in an isolated folder.

File/rename failure injection checks that both runtime and disk retain the old
value. A second process reads the persisted TOML through the real parser.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

PRELUDE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#endif
typedef unsigned char boolean;
#define FALSE 0
typedef int pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER 0
static void pthread_mutex_lock(pthread_mutex_t *m) {(void)m;}
static void pthread_mutex_unlock(pthread_mutex_t *m) {(void)m;}
static void platform_log(const char *f, ...) {(void)f;}
static void console_printf(int b,const char *f,...) {(void)b;(void)f;}
static void console_warning(const char *f,...) {(void)f;}
static int fail_save,fail_rename;
static const char *SDL_GetBasePath(void){return "./";}
static void SDL_free(void*p){free(p);}
static void *SDL_LoadFile(const char *p,size_t *n){FILE*f=fopen(p,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);*n=ftell(f);rewind(f);void*b=malloc(*n+1);if(fread(b,1,*n,f)!=*n)abort();fclose(f);((char*)b)[*n]=0;return b;}
static int SDL_SaveFile(const char *p,const void*b,size_t n){FILE*f=fopen(p,"wb");if(!f)return 0;int ok=fwrite(b,1,fail_save?n/2:n,f)==n;fclose(f);return ok;}
static int SDL_RenamePath(const char*a,const char*b){if(fail_rename)return 0;
#ifdef _WIN32
return MoveFileExA(a,b,MOVEFILE_REPLACE_EXISTING);
#else
return rename(a,b)==0;
#endif
}
static void SDL_RemovePath(const char*p){remove(p);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
'''

TESTS = r'''
static char *file_text(void){size_t n;return SDL_LoadFile("config.toml",&n);}
static int execute(const char*s){int ok=0;CHECK(console_option_execute(s,&ok));return ok;}
int main(int argc,char**argv){
 if(argc>1){if(find_option("fov_desired")){CHECK(config_real("display.fov")==110);CHECK(config_real("display.viewmodel_fov")==90);}CHECK(config_boolean("display.vsync")==0);CHECK(fabs(config_real("audio.volume")-.25)<1e-9);return 0;}
 FILE*f=fopen("config.toml","wb");CHECK(f);fputs("# retained comment\n[display]\nvsync = true\n[custom]\nmy_setting = 7 # retained unknown key\n",f);fclose(f);
 CHECK(config_boolean("display.vsync"));
 CHECK(console_option_get(0));CHECK(console_option_get(10000)==NULL);
 CHECK(console_option_help("display.vsync"));CHECK(!console_option_help("not_an_option"));
 int ok;CHECK(!console_option_execute("show_hud 0",&ok));CHECK(!console_option_execute("display.vsync_extra 0",&ok));
 unsigned long generation=config_changes();CHECK(execute("display.vsync"));CHECK(config_changes()==generation);
 CHECK(execute(" (DISPLAY.VSYNC 0) "));CHECK(!config_boolean("display.vsync"));
 CHECK(config_changes()==generation+1);
 const char*bad[]={"display.vsync 2","display.vsync -1","display.vsync nan","display.vsync 0 extra","(display.vsync 1","display.vsync 1)","(display.vsync 1) (game_speed 2)","display.vsync 1;game_speed 2","display.vsync 1junk","display.vsync \"1\""};
 char*before=file_text();generation=config_changes();
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){CHECK(!execute(bad[i]));CHECK(!config_boolean("display.vsync"));CHECK(config_changes()==generation);char*after=file_text();CHECK(!strcmp(before,after));free(after);}
 fail_save=1;CHECK(!execute("display.vsync 1"));fail_save=0;
 CHECK(!config_boolean("display.vsync"));CHECK(config_changes()==generation);char*after=file_text();CHECK(!strcmp(before,after));free(after);
 fail_rename=1;CHECK(!execute("display.vsync on"));fail_rename=0;
 CHECK(!config_boolean("display.vsync"));CHECK(config_changes()==generation);after=file_text();CHECK(!strcmp(before,after));free(after);free(before);
 CHECK(fopen("config.toml.tmp","rb")==NULL);
 CHECK(execute("display.vsync default"));CHECK(config_boolean("display.vsync"));
 CHECK(execute("display.vsync off"));
 if(find_option("fov_desired")){
  CHECK(execute("fov_desired 20.25"));CHECK(config_real("display.fov")==20.25);
  CHECK(execute("fov_desired 150"));CHECK(config_real("display.fov")==150);
  const char*invalid[]={"fov_desired 19.99","fov_desired 150.01","fov_desired nan","fov_desired inf","viewmodel_fov -1","viewmodel_fov 1e9999"};
  for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++)CHECK(!execute(invalid[i]));
  CHECK(execute("fov_desired 0"));CHECK(config_real("display.fov")==0);
  CHECK(execute("fov_desired 110"));CHECK(execute("viewmodel_fov 90"));
 }

 CHECK(config_write("audio.volume",".25"));CHECK(fabs(config_real("audio.volume")-.25)<1e-9);
 after=file_text();CHECK(strstr(after,"# retained comment"));CHECK(strstr(after,"my_setting = 7 # retained unknown key"));CHECK(strstr(after,"volume = 0.25"));free(after);
 char oversized[500];memset(oversized,'a',sizeof(oversized));oversized[sizeof(oversized)-1]=0;CHECK(!console_option_execute(oversized,&ok));
 puts("PASS: production console queries/writes/defaults, strict syntax, completion metadata, atomic save/rename failures, generation and retained comments/unknown keys");
 return 0;
}
'''


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--cc', default='clang')
    a = p.parse_args()
    config = (ROOT/'port/linux/src/port_config.c').read_text()
    parser = (ROOT/'port/linux/game/console_options.c').read_text()
    config = re.sub(r'^#include "platform.h"\n|^#include <SDL3/SDL.h>\n|^#include <pthread.h>\n', '', config, flags=re.M)
    parser = re.sub(r'^#include "cseries.h"\n|^#include "main/console.h"\n', '', parser, flags=re.M)
    flags = ['-std=gnu11', '-O2', '-fuse-ld=lld', '-D_CRT_SECURE_NO_WARNINGS', '-D_CRT_NONSTDC_NO_WARNINGS']
    if sys.platform == 'win32':
        flags += ['--target=i686-pc-windows-msvc']
    else:
        flags += ['-lm']
    hs = (ROOT/'source/hs/hs.c').read_text()
    command = hs[hs.index('static boolean hs_compile_and_evaluate_command(\n\tchar const *expression)\n{'):]
    assert command.index('console_option_execute(') < command.index('hs_playing_in_anothers_game(')
    assert 'console_option_help(function_name)' in hs
    assert 'hs_tokens_enumerate_add_string(option->command)' in hs
    with tempfile.TemporaryDirectory(prefix='halo-console-settings-') as directory:
        d = Path(directory)
        (d/'console_options.h').write_text((ROOT/'port/linux/include/console_options.h').read_text())
        (d/'test.c').write_text(PRELUDE + config + parser + TESTS)
        exe = d/'test.exe'
        subprocess.run([a.cc, *flags, '-I'+str(d), '-I'+str(ROOT/'port/linux/src'),
                        '-I'+str(ROOT/'port/third_party/tomlc17'), str(d/'test.c'),
                        str(ROOT/'port/third_party/tomlc17/tomlc17.c'), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], cwd=d, check=True, timeout=20)
        subprocess.run([str(exe), '--reload'], cwd=d, check=True, timeout=20)
    print('PASS: real TOML restart persistence; registered options allowed for clients, help and Tab completion hooked without changing map-script tables')


if __name__ == '__main__':
    main()
