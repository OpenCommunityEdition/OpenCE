"""Exercise the actual model lifecycle C paths without retail assets."""
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def extract(text, marker):
    start = text.index(marker)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def test_model_lifecycle(tmp_path):
    source = (ROOT / 'port/linux/game/mutator_scripts.c').read_text()
    flags = source.split('SET_FLAG(object->object.flags, _object_cannot_be_activated_bit, TRUE);', 1)[1].split('SET_FLAG(header->flags', 1)[0]
    diagnostics = extract(source, 'static void script_model_failure(\n')
    lookup = extract(source, 'static struct mutator_script_model *script_model_at(')
    code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NUMBEROF(x) (sizeof(x)/sizeof((x)[0]))
#define TEST_FLAG(x,n) ((x)&(1UL<<(n)))
#define SET_FLAG(x,n,v) ((x) = ((x)&~(1UL<<(n)))|((v)?(1UL<<(n)):0))
enum { _object_cannot_be_activated_bit, _object_deleted_when_deactivated_bit, _object_shadowless_bit, _error_silent };
struct mutator_script { char key[32], model_failure_tags[8][256]; unsigned long model_failure_reasons[8]; short model_failure_count; };
struct mutator_script_model { int used; short script_index; long object_index; char tag[256]; };
#define MAXIMUM_SCRIPT_MODELS 2
static struct { struct mutator_script scripts[2]; struct mutator_script_model models[2]; } mutator_scripts;
struct object_datum { struct { unsigned long flags; } object; } live;
struct script_arguments { struct mutator_script *script; long handle; };
static int exists=1, logs=0;
static long argument_integer(struct script_arguments *s,int n) { return s->handle; }
static struct object_datum *object_try_and_get(long i) { return exists && i==0x12340001 ? &live : NULL; }
static void script_model_forget(struct mutator_script_model *m) { memset(m,0,sizeof(*m)); }
static void error(int severity,const char *format,...) { va_list ap; char line[1024]; va_start(ap,format); vsnprintf(line,sizeof(line),format,ap); va_end(ap); assert(strstr(line,"grappling_hook")); logs++; }
'''
    code += diagnostics + '\n' + lookup
    code += '\nstatic void configure(struct object_datum *object) {\n' + flags + '\n}\n'
    code += r'''
int main(void) {
    struct mutator_script *s=&mutator_scripts.scripts[0];
    struct mutator_script_model *m=&mutator_scripts.models[0];
    struct script_arguments a={s,0x12340001};
    strcpy(s->key,"grappling_hook");
    /* Real projectile/garbage constructor deletion flag must be removed. */
    live.object.flags=(1UL<<_object_deleted_when_deactivated_bit)|(1UL<<_object_cannot_be_activated_bit);
    configure(&live);
    assert(!TEST_FLAG(live.object.flags,_object_deleted_when_deactivated_bit));
    m->used=1; m->object_index=a.handle; strcpy(m->tag,"rope");
    assert(script_model_at(&a,1)==m); /* inactive weapon/projectile both accepted */
    a.handle++; assert(script_model_at(&a,1)==NULL); a.handle--;
    exists=0; assert(script_model_at(&a,1)==NULL); assert(logs==1 && !m->used);
    script_model_failure(s,"rope",3,"deleted"); assert(logs==1);
    script_model_failure(s,"rope",0,"tag missing"); assert(logs==2);
    script_model_failure(s,"hook",0,"tag missing"); assert(logs==3);
    script_model_failure(s,"hook",0,"tag missing"); assert(logs==3);
    exists=1; m->used=1; m->object_index=a.handle; strcpy(m->tag,"rope");
    live.object.flags=0; assert(script_model_at(&a,1)==NULL); assert(logs==4);
    return 0;
}
'''
    c = tmp_path / 'models.c'
    exe = tmp_path / 'models'
    c.write_text(code)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Werror', '-Wno-unused-parameter', '-Wno-sign-compare', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
