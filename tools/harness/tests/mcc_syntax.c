/* Generated MCC syntax blobs: no shipped game data or external resources. */
#include "cseries.h"
#include "math/real_math.h"
#include "errors.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/data.h"
#include "scenario/scenario_definitions.h"
#include "mcc_syntax.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef free
#undef strcmp
#undef strcpy

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; } } while (0)

static boolean loaded;
static unsigned char *allocation;
static long allocation_bytes;
static char strings[1024];
static unsigned errors, visited;

void *csmemset(void *buffer, long value, unsigned long bytes) { return memset(buffer, (int)value, bytes); }
boolean mcc_cache_tags_loaded(void) { return loaded; }
boolean mcc_cache_contains(void const *pointer, long bytes)
{
    uintptr_t address = (uintptr_t)pointer, base = (uintptr_t)allocation;
    if (bytes < 0) return FALSE;
    if (address >= base && address - base <= (unsigned long)allocation_bytes &&
        (unsigned long)bytes <= (unsigned long)allocation_bytes - (address - base)) return TRUE;
    return pointer == strings && bytes <= sizeof(strings);
}
void error(short severity, char const *format, ...) { (void)severity; (void)format; ++errors; }

static long handle(unsigned index) { return (long)(0x81230000u | index); }
static short refusal(struct hs_syntax_node const *node, char const **name)
{
    ++visited;
    *name = node->source_offset == 99 ? "blocked_function" : NULL;
    return node->source_offset == 99 ? 1 : 0;
}
static void live_node(struct hs_syntax_node *node)
{
    node->datum_header = (short)0x8123;
    node->flags = 9;
    node->type = node->constant_type = _hs_type_boolean;
    node->next_node_index = NONE;
}

int main(int argc, char **argv)
{
    struct data_array *syntax;
    struct hs_syntax_node *nodes;
    struct scenario scenario = {0};
    struct hs_script scripts[2] = {0};
    struct hs_global globals[1] = {0};
    unsigned long disabled_scripts[16] = {0}, disabled_globals[32] = {0};
    unsigned char *before;
    long bytes;
    unsigned i;
    int expected = 1;
    char const *test;
    CHECK(argc == 2);
    test = argv[1];
    allocation_bytes = sizeof(*syntax) + MCC_SYNTAX_CAPACITY * sizeof(*nodes);
    allocation = malloc(allocation_bytes);
    before = malloc(allocation_bytes);
    CHECK(allocation && before);
    memset(allocation, 0, allocation_bytes);
    syntax = (struct data_array *)allocation;
    nodes = (struct hs_syntax_node *)(syntax + 1);
    syntax->signature = 'd@t@';
    syntax->size = sizeof(*nodes);
    syntax->maximum_count = MCC_SYNTAX_CAPACITY;
    syntax->valid = TRUE;
    syntax->identifier_zero_invalid = TRUE;
    syntax->count = syntax->actual_count = 2;
    syntax->data = (void *)1; /* File pointer is stale; validation must ignore it. */
    for (i = 0; i < 2; ++i) live_node(nodes + i);
    bytes = allocation_bytes;

    if (!strcmp(test, "empty")) {
        syntax->count = syntax->actual_count = 0;
        memset(nodes, 0, 2 * sizeof(*nodes));
    } else if (!strcmp(test, "high_slot") || !strcmp(test, "callbacks")) {
        syntax->count = MCC_SYNTAX_CAPACITY;
        memset(nodes + 1, 0, sizeof(*nodes));
        live_node(nodes + MCC_SYNTAX_CAPACITY - 1);
        nodes[0].flags = 8;
        nodes[0].data = handle(MCC_SYNTAX_CAPACITY - 1);
    } else if (!strcmp(test, "sparse")) {
        syntax->count = 19723;
        syntax->actual_count = 18480;
        memset(nodes, 0, 2 * sizeof(*nodes));
        for (i = 1243; i < 19723; ++i) live_node(nodes + i);
    } else if (!strcmp(test, "deep") || !strcmp(test, "wide") || !strcmp(test, "deep_cycle")) {
        syntax->count = syntax->actual_count = MCC_SYNTAX_CAPACITY;
        for (i = 0; i < MCC_SYNTAX_CAPACITY; ++i) {
            live_node(nodes + i);
            if (i + 1 < MCC_SYNTAX_CAPACITY) {
                if (!strcmp(test, "wide") && i) nodes[i].next_node_index = handle(i + 1);
                else { nodes[i].flags = 8; nodes[i].data = handle(i + 1); }
            }
        }
        if (!strcmp(test, "deep_cycle")) {
            nodes[MCC_SYNTAX_CAPACITY - 1].next_node_index = handle(0);
            expected = 0;
        }
    } else if (!strcmp(test, "salt")) { nodes[0].next_node_index = handle(1) ^ 0x10000; expected = 0;
    } else if (!strcmp(test, "zero_salt")) { nodes[0].next_node_index = 1; expected = 0;
    } else if (!strcmp(test, "out_of_range")) { nodes[0].next_node_index = handle(32767); expected = 0;
    } else if (!strcmp(test, "unused")) { nodes[1].datum_header = 0; syntax->actual_count = 1; nodes[0].next_node_index = handle(1); expected = 0;
    } else if (!strcmp(test, "sibling_cycle")) { nodes[0].next_node_index = handle(1); nodes[1].next_node_index = handle(0); expected = 0;
    } else if (!strcmp(test, "call_cycle")) { nodes[0].flags = 8; nodes[0].data = handle(0); expected = 0;
    } else if (!strcmp(test, "short_blob")) { --bytes; expected = 0;
    } else if (!strcmp(test, "long_blob")) { ++bytes; expected = 0;
    } else if (!strcmp(test, "negative_count")) { syntax->count = -1; expected = 0;
    } else if (!strcmp(test, "negative_capacity")) { syntax->maximum_count = (short)0x8000; expected = 0;
    } else if (!strcmp(test, "count_over_capacity")) { syntax->maximum_count = 1; expected = 0;
    } else if (!strcmp(test, "actual_count")) { syntax->actual_count = 1; expected = 0;
    } else if (!strcmp(test, "negative_actual")) { syntax->actual_count = -1; expected = 0;
    } else if (!strcmp(test, "free_hint")) { syntax->first_free_absolute_index = -1; expected = 0;
    } else if (!strcmp(test, "signature")) { syntax->signature = 0; expected = 0;
    } else if (!strcmp(test, "node_size")) { ++syntax->size; expected = 0;
    } else if (!strcmp(test, "invalid_array")) { syntax->valid = FALSE; expected = 0;
    }
    memcpy(before, allocation, allocation_bytes);
    CHECK(mcc_syntax_graph_valid(syntax, bytes) == expected);
    CHECK(!memcmp(before, allocation, allocation_bytes));
    CHECK(!mcc_syntax_graph_valid(NULL, bytes));
    CHECK(!mcc_syntax_graph_valid(syntax, 1));
    CHECK(!mcc_syntax_graph_valid((struct data_array *)(allocation + 1), bytes));
    if (!expected) { free(before); free(allocation); return 0; }

    scenario.hs_syntax_data.address = syntax;
    scenario.hs_syntax_data.size = bytes;
    scenario.hs_string_constants.address = strings;
    scenario.hs_string_constants.size = sizeof(strings);
    CHECK(mcc_syntax_scenario_valid((struct scenario *)1) == -1);
    CHECK(!mcc_syntax_functions_check((struct scenario *)1, (struct data_array *)1, NULL, NULL, NULL));
    loaded = TRUE;
    CHECK(mcc_syntax_scenario_valid(&scenario) == 1);
    scenario.hs_string_constants.size = 1023;
    CHECK(mcc_syntax_scenario_valid(&scenario) == 0);
    scenario.hs_string_constants.size = 1024;
    scenario.hs_syntax_data.size = bytes + 1;
    CHECK(mcc_syntax_scenario_valid(&scenario) == 0);
    scenario.hs_syntax_data.size = bytes;
    scenario.hs_syntax_data.address = (void *)1;
    CHECK(mcc_syntax_scenario_valid(&scenario) == 0);
    scenario.hs_syntax_data.address = syntax;
    CHECK(mcc_syntax_scenario_valid(NULL) == 0);

    if (!strcmp(test, "callbacks") || !strcmp(test, "wide") || !strcmp(test, "deep") ||
        !strcmp(test, "root_type") || !strcmp(test, "damaged_root") || !strcmp(test, "shared_child")) {
        scenario.hs_scripts.count = 1;
        scenario.hs_scripts.address = scripts;
        scenario.hs_globals.count = 1;
        scenario.hs_globals.address = globals;
        strcpy(scripts[0].name, "script");
        scripts[0].root_expression_index = handle(0);
        scripts[0].return_type = _hs_type_boolean;
        globals[0].initialization_expression_index = NONE;
        globals[0].type = _hs_type_boolean;
        if (!strcmp(test, "root_type")) scripts[0].return_type = _hs_type_void;
        if (!strcmp(test, "damaged_root")) scripts[0].root_expression_index = handle(32767);
        if (!strcmp(test, "shared_child")) {
            syntax->count = syntax->actual_count = 3;
            live_node(nodes + 2);
            nodes[0].flags = nodes[1].flags = 8;
            nodes[0].data = handle(1);
            nodes[1].data = handle(2);
            nodes[1].next_node_index = handle(2); /* Acyclic, but two references within one expression. */
            CHECK(mcc_syntax_graph_valid(syntax, bytes));
        }
        CHECK(mcc_syntax_functions_check(&scenario, syntax, refusal, disabled_scripts, disabled_globals));
        if (!strcmp(test, "root_type") || !strcmp(test, "damaged_root") || !strcmp(test, "shared_child")) {
            CHECK(disabled_scripts[0] == 1 && !disabled_globals[0] && errors == 1);
        } else {
            CHECK(!disabled_scripts[0] && !disabled_globals[0] && !errors);
            CHECK(visited == (unsigned)syntax->actual_count);
        }
        if (!strcmp(test, "callbacks")) {
            nodes[MCC_SYNTAX_CAPACITY - 1].source_offset = 99;
            globals[0].initialization_expression_index = handle(MCC_SYNTAX_CAPACITY - 1);
            CHECK(mcc_syntax_functions_check(&scenario, syntax, refusal, disabled_scripts, disabled_globals));
            CHECK(disabled_scripts[0] == 1 && disabled_globals[0] == 1 && errors == 1);
            loaded = FALSE;
            CHECK(!mcc_syntax_functions_check(&scenario, syntax, refusal, disabled_scripts, disabled_globals));
            CHECK(disabled_scripts[0] == 1 && disabled_globals[0] == 1 && errors == 1);
        }
    }
    free(before);
    free(allocation);
    return 0;
}
