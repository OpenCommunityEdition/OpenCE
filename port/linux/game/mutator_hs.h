/* port: bounded native HS scalar-expression sandbox. Not a map-script VM. */
#ifndef MUTATOR_HS_H
#define MUTATOR_HS_H

/* Call only on the game thread, outside any HS evaluation. No concurrent use.
 * Source is one native HS expression, <=1023 bytes; no embedded NULs.
 * Result types: boolean, real, short, long (hs_type enum).
 * No globals, definitions, strings, objects, tags, sleeping or game functions.
 * Builtins: begin, if, and, or, +, -, *, min, max, >, <, >=, <=.
 * Maximum 128 nodes, nesting 32, and 4096 evaluator transitions.
 * value is the engine's raw 32-bit scalar representation; real uses float bits.
 */
struct mutator_hs_result
{
    long value;
    long steps;
    char error[256];
};
int mutator_hs_execute(const char *source, long length, short type,
    long budget, struct mutator_hs_result *result);
int mutator_hs_self_test(void);

/* Internal engine bridge; callers must use execute, not these functions. */
struct data_array;
long hs_isolated_compile(char *source, long length, short type,
    struct data_array *syntax, char *error);
int hs_isolated_evaluate(long expression, struct data_array *threads,
    long budget, long *value, long *steps);
char const *hs_isolated_error(void);
int hs_isolated_function_allowed(short index);
/* Persistent programs use native (global TYPE NAME EXPR) and
 * (script static TYPE NAME EXPR...) declarations. Entry names are ordinary
 * static scripts: startup/frame/tick/stop. Call them explicitly on game thread.
 * API descriptors and strings must outlive the program. Strings returned by
 * callbacks must also outlive it. Scalars use native HS bits (32-bit target).
 * Vectors are exposed as separate real x/y/z calls, not interpreter tables. */
#define MUTATOR_HS_MAX_ARGUMENTS 8
struct mutator_hs_api
{
    const char *name;
    short return_type;
    short argument_count;
    short argument_types[MUTATOR_HS_MAX_ARGUMENTS];
    int (*call)(void *context, const long *arguments, long *value);
};
struct mutator_hs_program;
struct mutator_hs_program *mutator_hs_program_compile(const char *source,
    long length, const struct mutator_hs_api *api, short api_count,
    void *context, struct mutator_hs_result *result);
int mutator_hs_program_call(struct mutator_hs_program *program,
    const char *script, long budget, struct mutator_hs_result *result);
void mutator_hs_program_free(struct mutator_hs_program *program);
/* Internal native compiler/runtime hooks. */
int hs_isolated_program_compile(char *source, long length,
    struct data_array *syntax, char *error);
const struct mutator_hs_api *mutator_hs_api_get(short index);
short mutator_hs_api_find(const char *name);
int mutator_hs_api_call(short index, const long *arguments, long *value);
int mutator_hs_program_active(void);
struct data_array *mutator_hs_program_globals(void);
#define MUTATOR_HS_API_BASE 30000
#endif
