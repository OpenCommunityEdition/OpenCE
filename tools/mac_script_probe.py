"""Generate cross-architecture regression tests from actual scripting sources.

python3 tools/mac_script_probe.py OUTPUT.c [--legacy-layout | --legacy-store]
Build with 32-bit long, -fms-extensions and -fno-strict-aliasing.
The optional mutations are negative controls and must fail on PPC.
"""
import re
import sys
from pathlib import Path
from harness import function, read, structure, enum_with

runtime = read('source/hs/hs_runtime.c')
engine = read('source/hs/hs.c')
compiler = read('source/hs/hs_compile.c')
header = read('source/hs/hs_value.h')
if '--legacy-layout' in sys.argv:
    header = re.sub(r'^#if defined\(__BIG_ENDIAN__\).*$', '#if 0', header, flags=re.M)
if '--legacy-store' in sys.argv:
    runtime = runtime.replace('result = (unsigned short)(short)l;', '*(short *)&result = (short)l;')

def aggregate(source, name, kind='struct'):
    start = re.search(r'^' + kind + r' ' + name + r'\s*\{', source, re.M).start()
    at = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[at] == '{') - (source[at] == '}')
        at += 1
    return source[start:source.index(';', at) + 1]

def macro(source, name):
    m = re.search(r'^#define ' + name + r'\b.*(?:\\\n.*)*', source, re.M)
    # Walk physical lines so continuation comments and blank bodies are retained.
    lines = source[m.start():].splitlines()
    out = []
    for line in lines:
        out.append(line)
        if not line.endswith('\\'): break
    return '\n'.join(out)

prefix = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <limits.h>
typedef unsigned char boolean;
typedef unsigned char byte;
typedef unsigned short word;
typedef float real;
#define SHORT_MAX SHRT_MAX
#define SHORT_MIN SHRT_MIN
#define TRUE 1
#define FALSE 0
#define NONE -1
#define CHECK(x) do { checks++; if (!(x)) { failures++; printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
#define match_assert(file,line,x) do { if (!(x)) abort(); } while (0)
#define csstrcmp strcmp
#define csmemcmp memcmp
static int failures, checks;
'''
parts = [prefix, header, enum_with(read('source/hs/hs.h'), '_hs_type_boolean')]
parts += [aggregate(engine, name, 'union') for name in (
    'hs_real_value','hs_short_result','hs_evaluation_argument','hs_boolean_result')]
parts += [structure(read('source/hs/hs_scenario_definitions.h'), 'hs_syntax_node')]
names = re.findall(r'^struct (hs_arguments_\w+|hs_object_list_get_element_arguments)\s*\{', engine, re.M)
layout_tests = []
for name in names:
    definition = aggregate(engine, name)
    parts.append(definition)
    fields = re.findall(r'HS_(SHORT|WORD|BOOLEAN)_SLOT\((\w+)\);|\b(long|real|char const \*)\s*(\w+);', definition)
    layout_tests.append('{ struct %s *p = (struct %s *)slots; memset(slots,0,sizeof(slots));' % (name,name))
    for index, (narrow, nname, wide, wname) in enumerate(fields):
        member = nname or wname
        if narrow:
            value = '0xffff' if narrow in ('SHORT','WORD') else '0xaabbcc01UL'
            expected = '-1' if narrow == 'SHORT' else '65535' if narrow == 'WORD' else '1'
        else:
            value = '0x41820000' if wide == 'real' else '0x12345678'
            expected = '16.25f' if wide == 'real' else '(char const *)0x12345678' if wide == 'char const *' else '0x12345678'
        layout_tests += [f'slots[{index}]={value}; CHECK(p->{member} == {expected});']
        # Narrow fields must lie inside the expected slot; following fields must not drift.
        layout_tests += [f'CHECK(offsetof(struct {name},{member}) / 4 == {index});']
    layout_tests.append('}')
parts += [function(runtime,n) for n in ('hs_long_to_short','hs_short_to_real','hs_real_to_short','hs_long_to_boolean','hs_short_to_boolean')]
parts.append(r'''
static struct hs_syntax_node syntax;
static struct { char const *compiled_source; char const *error; long error_offset; } hs_compile_globals;
#define hs_syntax_get(index) (&syntax)
''')
parts += [function(compiler,n) for n in ('hs_parse_boolean','hs_parse_integer')]
# Test real global reconciliation switch bodies with an external-global fixture.
parts += [structure(read('source/hs/hs.h'),'hs_external_global_definition')]
parts.append(r'''
struct hs_global_datum { long value; } fake_global;
static struct hs_external_global_definition fake_external;
static short global_type;
#define HS_GLOBAL_DESIGNATOR_IS_EXTERNAL(d) ((d)&0x8000)
#define HS_GLOBAL_DESIGNATOR_TO_INDEX(d) ((d)&0x7fff)
#define hs_external_global_count 0
#define datum_get(data,index) (&fake_global)
#define hs_global_external_get(index) (&fake_external)
#define hs_global_get_type(index) global_type
#define display_assert(a,b,c,d) abort()
#define system_exit(code) abort()
''')
for decl in re.findall(r'^extern (.*?\b_hs_type_\w+_default);', runtime, re.M):
    parts.append('static '+decl+' = 0;')
parts += [function(runtime,n) for n in ('hs_global_reconcile_read','hs_global_reconcile_write')]
# Runtime equality/logical readers and return values, against a minimal fake stack.
parts.append(r'''
enum { _hs_function_equal, _hs_function_not_equal, _hs_function_and, _hs_function_or, _hs_function_sleep_until };
struct stack_fixture { long expression_index; } fake_stack;
struct hs_thread_datum { struct stack_fixture *stack; long sleep_until; } fake_thread = { &fake_stack };
static long fake_arguments[8], returned, stack_values[5];
static int allocation;
static short hs_type_sizes[NUMBER_OF_HS_TYPES];
#define hs_thread_get(index) (&fake_thread)
#define hs_arguments_evaluate(a,b,c,d) fake_arguments
static void *hs_stack_allocate(long thread, long size) { return &stack_values[allocation++]; }
static void hs_return(long thread, long value) { returned=value; }
static void hs_evaluate(long thread, long expression, long *value) { *value=1; syntax.next_node_index=NONE; }
#define game_time_get() 100
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
''')
parts += [function(runtime,n) for n in ('hs_evaluate_equality','hs_evaluate_logical','hs_evaluate_sleep_until')]
# Invoke actual evaluator macros, so alias writes and the raw-short reader are covered.
parts.append(r'''
#define hs_macro_function_evaluate(a,b,c) fake_arguments
static short return_negative(void) { return -1; }
static boolean return_true(void) { return 1; }
static real word_as_real(unsigned short value) { return (real)value; }
''')
parts += [macro(engine,n) for n in ('HS_EVALUATE_SHORT_NO_ARGUMENTS','HS_EVALUATE_BOOLEAN_NO_ARGUMENTS','HS_EVALUATE_REAL_FROM_UNSIGNED_SHORT') if re.search(r'^#define '+n+r'\b',engine,re.M)]
# Names are obtained from the actual source, not guessed by the runner.
# Locate by bounded macro bodies rather than spanning earlier definitions.
for name in re.findall(r'^#define (HS_EVALUATE_\w+)\(',engine,re.M):
    body=macro(engine,name)
    if 'result.short_value = function();' in body: short_macro=name
    if 'result.boolean = function();' in body: boolean_macro=name
for name in (short_macro,boolean_macro):
    if macro(engine,name) not in parts: parts.append(macro(engine,name))
parts += [f'{short_macro}(probe_short,return_negative)',f'{boolean_macro}(probe_boolean,return_true)',
          'HS_EVALUATE_REAL_FROM_UNSIGNED_SHORT(probe_word,word_as_real)']
main = r'''
int main(void)
{
    long values[] = {-32768,-7,-1,0,1,7,32767};
    short narrow_types[] = {_hs_type_script,_hs_type_trigger_volume,_hs_type_cutscene_flag,
        _hs_type_cutscene_camera_point,_hs_type_cutscene_title,_hs_type_cutscene_recording,
        _hs_type_device_group,_hs_type_ai_command_list,_hs_type_starting_profile,
        _hs_type_conversation,_hs_type_navpoint,_hs_type_hud_message,
        _hs_type_enum_game_difficulty,_hs_type_enum_team,_hs_type_enum_ai_default_state,
        _hs_type_enum_actor_type,_hs_type_enum_hud_corner,_hs_type_object_name};
    long slots[8], bits, slot;
    int i;
    real converted;
    short external_short;
    boolean external_boolean;
    union hs_evaluation_argument arg;
    if(sizeof(long)!=4 || sizeof(void*)!=4) return 2;
    for(i=0;i<7;i++) {
        slot=hs_long_to_short(values[i]); CHECK(slot==(unsigned short)values[i]);
        bits=hs_short_to_real(slot); memcpy(&converted,&bits,4);
        CHECK(converted==(real)values[i]); CHECK(hs_real_to_short(bits)==slot);
        CHECK(hs_short_to_boolean(slot)==(values[i]==0));
        CHECK(hs_long_to_boolean(values[i])==(values[i]==0));
        arg.long_value=slot; CHECK(arg.short_value==(short)values[i]);
        CHECK(arg.unsigned_short_value==(unsigned short)values[i]);
        external_short=(short)values[i]; fake_external.address=&external_short;
        global_type=_hs_type_short_integer; fake_global.value=0x12340000;
        hs_global_reconcile_read(0x8000);
        CHECK(fake_global.value==(long)(0x12340000UL|(unsigned short)values[i]));
        external_short=0; hs_global_reconcile_write(0x8000);
        CHECK(external_short==(short)values[i]);
    }
    for(i=0;i<(int)(sizeof(narrow_types)/sizeof(narrow_types[0]));i++) {
        external_short=-32768; fake_external.address=&external_short;
        global_type=narrow_types[i]; fake_global.value=0x12340000;
        hs_global_reconcile_read(0x8000); CHECK(fake_global.value==0x12348000);
        external_short=0; hs_global_reconcile_write(0x8000); CHECK(external_short==-32768);
    }
    for(i=0;i<2;i++) {
        external_boolean=i; fake_external.address=&external_boolean;
        global_type=_hs_type_boolean; fake_global.value=0x12345600;
        hs_global_reconcile_read(0x8000); CHECK(fake_global.value==0x12345600+i);
        external_boolean=!i; hs_global_reconcile_write(0x8000); CHECK(external_boolean==i);
        memset(&syntax,0,sizeof(syntax)); syntax.type=syntax.constant_type=_hs_type_boolean;
        hs_compile_globals.compiled_source=i?"true":"false";
        CHECK(hs_parse_boolean(0)); CHECK(syntax.data==i);
        arg.long_value=syntax.data; CHECK(arg.boolean_value==i);
    }
    memset(&syntax,0,sizeof(syntax)); syntax.type=syntax.constant_type=_hs_type_short_integer;
    hs_compile_globals.compiled_source="-32768"; CHECK(hs_parse_integer(0));
    CHECK(syntax.data==0x8000); CHECK(syntax.short_value==-32768);
    syntax.short_value=(short)0x8001; CHECK(syntax.data==0x8001); /* CE designator */
    CHECK(sizeof(syntax)==20); CHECK(offsetof(struct hs_syntax_node,data)==16);
    hs_type_sizes[_hs_type_short_integer]=2; syntax.type=_hs_type_short_integer;
    fake_arguments[0]=1; fake_arguments[1]=2; hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==0);
    fake_arguments[1]=1; hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==1);
    fake_arguments[1]=0x12340001; hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==1);
    hs_evaluate_equality(_hs_function_not_equal,0,0); CHECK(returned==0);
    for(i=0;i<2;i++) {
        stack_values[0]=NONE; stack_values[1]=i; stack_values[2]=0; *(boolean *)&stack_values[2]=1; allocation=0;
        hs_evaluate_logical(_hs_function_and,0,0); CHECK(returned==i);
        stack_values[0]=NONE; stack_values[1]=i; stack_values[2]=0; allocation=0;
        hs_evaluate_logical(_hs_function_or,0,0); CHECK(returned==i);
    }
    syntax.type=_hs_type_boolean; hs_type_sizes[_hs_type_boolean]=1;
    fake_arguments[0]=0x12340001; fake_arguments[1]=1;
    hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==1);
    fake_arguments[1]=0; hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==0);
    syntax.type=_hs_type_real; hs_type_sizes[_hs_type_real]=4;
    fake_arguments[0]=0; fake_arguments[1]=0x80000000UL;
    hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==0); /* preserve signed-zero bit inequality */
    fake_arguments[1]=0; hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==1);
    syntax.type=_hs_type_void; hs_type_sizes[_hs_type_void]=0;
    fake_arguments[1]=123; hs_evaluate_equality(_hs_function_equal,0,0); CHECK(returned==1);
    syntax.next_node_index=42; allocation=0; returned=-99;
    hs_evaluate_logical(_hs_function_and,0,1); CHECK(returned==-99); CHECK(stack_values[1]==1);
    allocation=0; hs_evaluate_logical(_hs_function_and,0,0); CHECK(returned==1);
    memset(stack_values,0xff,sizeof(stack_values)); syntax.next_node_index=NONE; allocation=0;
    hs_evaluate_sleep_until(_hs_function_sleep_until,0,1);
    CHECK(stack_values[0]==1); CHECK(stack_values[1]==30); CHECK(fake_thread.sleep_until==130);
    allocation=0; hs_evaluate_sleep_until(_hs_function_sleep_until,0,0); CHECK(returned==0);
    probe_short(0,0,0); CHECK(returned==65535);
    probe_boolean(0,0,0); CHECK(returned==1);
    fake_arguments[0]=65535; probe_word(0,0,0); memcpy(&converted,&returned,4); CHECK(converted==65535.f);
'''
parts.append(main+'\n'.join(layout_tests)+r'''
    printf("scripting_slots: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
''')
Path(sys.argv[1]).write_text('\n\n'.join(parts))
