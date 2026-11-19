/* port: isolated scalar HS foundation. All parsing/evaluation is engine HS. */
#include "cseries.h"
#include "hs.h"
#include "hs_scenario_definitions.h"
#include "data.h"
#include "scenario.h"
#include "scenario_definitions.h"
#include "mutator_hs.h"
#include <stdlib.h>
#include <string.h>
/* Private storage is not registered with the engine debug heap. */
#undef malloc
#undef free
#undef realloc
#undef calloc

extern struct data_array *hs_syntax_data;
static int mutator_hs_busy;

struct mutator_hs_program
{
    struct scenario scenario;
    struct data_array *syntax, *threads, *globals;
    char *source;
    const struct mutator_hs_api *api;
    short api_count;
    void *context;
};
static struct mutator_hs_program *mutator_hs_current;
extern short const hs_external_global_count;
int mutator_hs_program_active(void) { return mutator_hs_current != NULL; }
struct data_array *mutator_hs_program_globals(void)
{ return mutator_hs_current ? mutator_hs_current->globals : NULL; }
const struct mutator_hs_api *mutator_hs_api_get(short index)
{
    index -= MUTATOR_HS_API_BASE;
    return mutator_hs_current && index >= 0 && index < mutator_hs_current->api_count ?
        mutator_hs_current->api + index : NULL;
}
short mutator_hs_api_find(const char *name)
{
    short i;
    if (mutator_hs_current)
        for (i = 0; i < mutator_hs_current->api_count; i++)
            if (!strcmp(name, mutator_hs_current->api[i].name))
                return MUTATOR_HS_API_BASE + i;
    return NONE;
}
int mutator_hs_api_call(short index, const long *arguments, long *value)
{
    const struct mutator_hs_api *api = mutator_hs_api_get(index);
    long normalized[MUTATOR_HS_MAX_ARGUMENTS];
    short k;

    if (!api)
        return FALSE;
    /* HS evaluates a boolean or short argument into only the low bytes of
     * its 32-bit slot; the rest is whatever the stack held before. Hand
     * the host the value, not the slot, so `false` cannot read as nonzero. */
    for (k = 0; k < api->argument_count && k < MUTATOR_HS_MAX_ARGUMENTS; k++)
    {
        long slot = arguments ? arguments[k] : 0;
        switch (api->argument_types[k])
        {
        case _hs_type_boolean: normalized[k] = (*(const boolean *)&slot) != 0; break;
        case _hs_type_short_integer: normalized[k] = *(const short *)&slot; break;
        default: normalized[k] = slot; break;
        }
    }
    return api->call(mutator_hs_current->context, api->argument_count ? normalized : arguments, value);
}

int hs_isolated_function_allowed(short index)
{
    if (mutator_hs_current && (mutator_hs_api_get(index) ||
        index == _hs_function_set || index == _hs_function_equal ||
        index == _hs_function_not_equal))
        return TRUE;
    switch (index)
    {
    case _hs_function_begin:
    case _hs_function_if:
    case _hs_function_and:
    case _hs_function_or:
    case _hs_function_plus:
    case _hs_function_minus:
    case _hs_function_times:
    case _hs_function_min:
    case _hs_function_max:
    case _hs_function_gt:
    case _hs_function_lt:
    case _hs_function_gte:
    case _hs_function_lte:
        return TRUE;
    default:
        return FALSE;
    }
}

static struct data_array *mutator_hs_array(short count, short size)
{
    struct data_array *array;
    array = (struct data_array *)calloc(1, data_allocation_size(count, size));
    if (array)
    {
        data_initialize(array, "isolated hs", count, size);
        data_make_valid(array);
    }
    return array;
}

int mutator_hs_execute(const char *source, long length, short type,
    long budget, struct mutator_hs_result *result)
{
    struct data_array *syntax = NULL;
    struct data_array *threads = NULL;
    struct data_array *saved_syntax;
    struct scenario *scenario = NULL;
    struct scenario *saved_scenario;
    long saved_scenario_index;
    char text[1024];
    long i, depth = 0, nodes = 0, expression;
    int success = FALSE;

    if (!result)
        return FALSE;
    memset(result, 0, sizeof(*result));
    if (mutator_hs_busy || !source || length <= 0 || length > 1023 ||
        budget < 1 || budget > 4096 || type < _hs_type_boolean ||
        type > _hs_type_long_integer)
    {
        strcpy(result->error, "invalid isolated HS request");
        return FALSE;
    }
    /* Resource preflight only, not a language parser. Exclude strings/comments
     * and non-ASCII before the native recursive tokenizer sees the input. */
    for (i = 0; i < length; i++)
    {
        unsigned char c = (unsigned char)source[i];
        if (!c || c > 126 || (c < 32 && c != '\n' && c != '\r' && c != '\t') ||
            c == '"' || c == ';' || c == '\'')
            break;
        if (c == '(')
        {
            if (++depth > 32)
                break;
            nodes++;
        }
        else if (c == ')')
        {
            if (--depth < 0)
                break;
        }
        else if (c != ' ' && c != '\t' && c != '\n' && c != '\r' &&
            (i == 0 || source[i-1] == '(' || source[i-1] == ')' ||
             source[i-1] == ' ' || source[i-1] == '\t' ||
             source[i-1] == '\n' || source[i-1] == '\r'))
            nodes++;
        if (nodes > 128)
            break;
    }
    if (i != length || depth)
    {
        strcpy(result->error, "isolated HS source exceeds syntax/resource limits");
        return FALSE;
    }
    memcpy(text, source, length);
    text[length] = 0;
    mutator_hs_busy = TRUE;
    syntax = mutator_hs_array(128, sizeof(struct hs_syntax_node));
    threads = mutator_hs_array(1, 0x1018);
    scenario = (struct scenario *)calloc(1, sizeof(*scenario));
    if (!syntax || !threads || !scenario)
    {
        strcpy(result->error, "isolated HS allocation failed");
        goto done;
    }
    /* Never call hs_compile_initialize(TRUE): it destroys live scenario HS.
     * Empty private scenario and arrays isolate even accidental lookups. */
    saved_scenario = global_scenario;
    saved_scenario_index = global_scenario_index;
    saved_syntax = hs_syntax_data;
    global_scenario = scenario;
    global_scenario_index = NONE;
    hs_syntax_data = syntax;
    expression = hs_isolated_compile(text, length, type, syntax, result->error);
    if (expression != NONE)
    {
        success = hs_isolated_evaluate(expression, threads, budget,
            &result->value, &result->steps);
        if (!success)
            strcpy(result->error, "isolated HS execution budget or stack exhausted");
    }
    hs_syntax_data = saved_syntax;
    global_scenario = saved_scenario;
    global_scenario_index = saved_scenario_index;
done:
    free(scenario);
    free(threads);
    free(syntax);
    mutator_hs_busy = FALSE;
    return success;
}

void mutator_hs_program_free(struct mutator_hs_program *p)
{
    if (!p || mutator_hs_busy) return;
    free(p->scenario.hs_globals.address); free(p->scenario.hs_scripts.address);
    free(p->source); free(p->syntax); free(p->threads); free(p->globals); free(p);
}

static int mutator_hs_program_eval(struct mutator_hs_program *p,
    long expression, long budget, struct mutator_hs_result *result)
{
    struct scenario *saved = global_scenario;
    long saved_index = global_scenario_index;
    struct data_array *saved_syntax = hs_syntax_data;
    int success;
    mutator_hs_busy = TRUE; mutator_hs_current = p;
    global_scenario = &p->scenario; global_scenario_index = 0;
    hs_syntax_data = p->syntax;
    success = hs_isolated_evaluate(expression, p->threads, budget,
        &result->value, &result->steps);
    hs_syntax_data = saved_syntax; global_scenario = saved;
    global_scenario_index = saved_index;
    mutator_hs_current = NULL; mutator_hs_busy = FALSE;
    if (!success) strcpy(result->error, hs_isolated_error() ?
        hs_isolated_error() : "HS execution budget, stack or host failure");
    return success;
}

int mutator_hs_program_call(struct mutator_hs_program *p, const char *name,
    long budget, struct mutator_hs_result *result)
{
    short i;
    struct hs_script *scripts;
    if (!result) return FALSE;
    memset(result, 0, sizeof(*result));
    if (!p || !name || mutator_hs_busy || budget < 1 || budget > 65536)
    { strcpy(result->error, "invalid HS program call"); return FALSE; }
    scripts = (struct hs_script *)p->scenario.hs_scripts.address;
    for (i = 0; i < p->scenario.hs_scripts.count; i++)
        if (!strcmp(name, scripts[i].name))
        {
            int success = mutator_hs_program_eval(p, scripts[i].root_expression_index, budget, result);
            if (!success) {
                char detail[sizeof(result->error)];
                strcpy(detail, result->error);
                snprintf(result->error, sizeof(result->error), "callback %.32s: %.160s", name, detail);
            }
            return success;
        }
    /* Lifecycle hooks are optional. Helpers/unknown names remain errors. */
    if (!strcmp(name, "startup") || !strcmp(name, "stop") ||
        !strcmp(name, "frame") || !strcmp(name, "tick") ||
        !strcmp(name, "player_tick") || !strcmp(name, "player_stop") ||
        !strcmp(name, "player_frame"))
        return TRUE;
    strcpy(result->error, "HS script not found");
    return FALSE;
}

struct mutator_hs_program *mutator_hs_program_compile(const char *source,
    long length, const struct mutator_hs_api *api, short api_count,
    void *context, struct mutator_hs_result *result)
{
    struct mutator_hs_program *p;
    struct scenario *saved;
    long saved_index, i, depth = 0;
    short j, k;
    int quoted = FALSE, comment = FALSE, success;
    struct hs_global *globals;
    if (!result) return NULL;
    memset(result, 0, sizeof(*result));
    if (mutator_hs_busy || !source || length < 1 || length > 32767 ||
        api_count < 0 || api_count > 128 || (api_count && !api))
        goto invalid;
    for (j = 0; j < api_count; j++)
    {
        if (!api[j].name || !*api[j].name || !api[j].call ||
            api[j].return_type < _hs_type_void || api[j].return_type > _hs_type_string ||
            api[j].argument_count < 0 || api[j].argument_count > MUTATOR_HS_MAX_ARGUMENTS)
            goto invalid;
        for (k = 0; k < api[j].argument_count; k++)
            if (api[j].argument_types[k] < _hs_type_boolean ||
                api[j].argument_types[k] > _hs_type_string) goto invalid;
        for (k = 0; k < j; k++) if (!strcmp(api[j].name, api[k].name)) goto invalid;
    }
    /* Resource scan only. The native tokenizer owns all language parsing. */
    for (i = 0; i < length; i++)
    {
        unsigned char c = (unsigned char)source[i];
        if (!c || c > 126 || (c < 32 && c != '\n' && c != '\r' && c != '\t')) goto invalid;
        if (comment) { if (c == '\n' || c == '\r') comment = FALSE; continue; }
        if (c == '"') { quoted = !quoted; continue; }
        if (quoted) continue;
        if (c == ';') { comment = TRUE; continue; }
        if (c == '(' && ++depth > 32) goto invalid;
        if (c == ')' && --depth < 0) goto invalid;
    }
    if (depth || quoted) goto invalid;
    p = (struct mutator_hs_program *)calloc(1, sizeof(*p));
    if (!p) goto invalid;
    p->source = (char *)malloc(length + 1);
    p->syntax = mutator_hs_array(4096, sizeof(struct hs_syntax_node));
    p->threads = mutator_hs_array(1, 0x1018);
    p->globals = mutator_hs_array(hs_external_global_count + 64, 8);
    p->scenario.hs_globals.address = calloc(64, sizeof(struct hs_global));
    p->scenario.hs_scripts.address = calloc(64, sizeof(struct hs_script));
    p->api = api; p->api_count = api_count; p->context = context;
    if (!p->source || !p->syntax || !p->threads || !p->globals ||
        !p->scenario.hs_globals.address || !p->scenario.hs_scripts.address) goto failed;
    memcpy(p->source, source, length); p->source[length] = 0;
    saved = global_scenario; saved_index = global_scenario_index;
    global_scenario = &p->scenario; global_scenario_index = 0;
    mutator_hs_busy = TRUE; mutator_hs_current = p;
    success = hs_isolated_program_compile(p->source, length, p->syntax, result->error);
    mutator_hs_current = NULL; mutator_hs_busy = FALSE;
    global_scenario = saved; global_scenario_index = saved_index;
    if (!success) goto failed;
    for (i = 0; i < hs_external_global_count + p->scenario.hs_globals.count; i++)
        datum_new(p->globals);
    globals = (struct hs_global *)p->scenario.hs_globals.address;
    for (i = 0; i < p->scenario.hs_globals.count; i++)
    {
        if (!mutator_hs_program_eval(p, globals[i].initialization_expression_index, 4096, result)) goto failed;
        *(long *)((char *)p->globals->data + (hs_external_global_count + i)*8 + 4) = result->value;
    }
    return p;
failed:
    mutator_hs_program_free(p);
    if (!result->error[0]) strcpy(result->error, "HS program allocation failed");
    return NULL;
invalid:
    strcpy(result->error, "invalid HS program or resource limits");
    return NULL;
}

static int mutator_hs_test_host(void *context, const long *arguments, long *value)
{
    if (strcmp((const char *)arguments[0], "hello")) return FALSE;
    *value = *(long *)context;
    return TRUE;
}

static int mutator_hs_program_self_test(void)
{
    static const char source[] =
        "(global long counter 1) "
        "(script static long helper (+ counter 1)) "
        "(script static void startup (set counter 10)) "
        "(script static long frame (set counter (helper)) counter) "
        "(script static long tick (host_test \"hello\")) "
        "(script static void stop (set counter 0)) "
        "(script static long recurse (recurse))";
    static const char *reject[] = {
        "(script static void frame (sleep 1))",
        "(script static void frame (set debug_scripting true))",
        "(script static long frame debug_scripting)",
        "(script startup frame 1)",
        "(global long a 1) (global long a 2)",
        "(script static long a 1) (script static long a 2)",
        "(script static long frame (host_test))",
        "(script static long frame (host_test \"hello\" 1))",
        "(script static long frame (unknown_api))"
    };
    struct mutator_hs_api api;
    struct mutator_hs_program *a, *b;
    struct mutator_hs_result result;
    long context = 73;
    int success;
    unsigned int i;
    memset(&api, 0, sizeof(api));
    api.name = "host_test"; api.return_type = _hs_type_long_integer;
    api.argument_count = 1; api.argument_types[0] = _hs_type_string;
    api.call = mutator_hs_test_host;
    a = mutator_hs_program_compile(source, sizeof(source)-1, &api, 1, &context, &result);
    if (!a) { fprintf(stderr, "HS compile: %s\n", result.error); return FALSE; }
    b = mutator_hs_program_compile(source, sizeof(source)-1, &api, 1, &context, &result);
    if (!b) { mutator_hs_program_free(a); return FALSE; }
    success = mutator_hs_program_call(a, "startup", 128, &result) &&
        mutator_hs_program_call(a, "frame", 128, &result) && result.value == 11 &&
        mutator_hs_program_call(a, "frame", 128, &result) && result.value == 12 &&
        mutator_hs_program_call(b, "frame", 128, &result) && result.value == 2 &&
        mutator_hs_program_call(a, "tick", 128, &result) && result.value == 73 &&
        !mutator_hs_program_call(a, "recurse", 4, &result) && result.steps <= 4 &&
        mutator_hs_program_call(a, "stop", 128, &result) &&
        mutator_hs_program_call(a, "frame", 128, &result) && result.value == 1;
    mutator_hs_program_free(a); mutator_hs_program_free(b);
    for (i = 0; i < sizeof(reject)/sizeof(reject[0]); i++)
    {
        a = mutator_hs_program_compile(reject[i], strlen(reject[i]), &api, 1, &context, &result);
        if (a) { fprintf(stderr, "HS unexpectedly accepted: %s\n", reject[i]); mutator_hs_program_free(a); return FALSE; }
    }
    return success;
}

int mutator_hs_self_test(void)
{
    struct mutator_hs_result result;
    static const char *reject[] = {
        "(sleep 1)", "(set developer_mode 1)", "(random_range 1 2)",
        "(script static void test)", "(print hello)", "developer_mode",
        "(begin 1) 2", "\"string\"", "(/ 1 0)"
    };
    unsigned int i;
    if (!mutator_hs_execute("(+ 2 3)", 7, _hs_type_long_integer, 128, &result) ||
        result.value != 5)
        return FALSE;
    if (!mutator_hs_execute("(if (> 3 2) 7 9)", 16,
        _hs_type_long_integer, 128, &result) || result.value != 7)
        return FALSE;
    if (mutator_hs_execute("(+ 2 3)", 7, _hs_type_long_integer, 1, &result))
        return FALSE;
    for (i = 0; i < sizeof(reject)/sizeof(reject[0]); i++)
        if (mutator_hs_execute(reject[i], strlen(reject[i]),
            _hs_type_long_integer, 128, &result))
            return FALSE;
    return mutator_hs_execute("42", 2, _hs_type_long_integer, 1, &result) &&
        result.value == 42 && mutator_hs_program_self_test();
}
