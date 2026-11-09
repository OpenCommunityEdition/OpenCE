/* Included after the interpreter's private types. Checkpoints save threads,
 * so their temporary syntax and text must be saved with them. Map syntax
 * stays in its owning cache; the snapshot restores its allocator and handles. */
#include "hs_console_state.h"

#define HS_CONSOLE_SOURCE_BYTES 1024
#define HS_CONSOLE_SOURCE_COUNT 256
/* The existing signed syntax handle admits at most 32767 slots (MCC).
 * Native maps retain their own 19001-slot arrays and allocator limits. */
#define HS_CONSOLE_SYNTAX_COUNT 32767

struct hs_console_state
{
    unsigned long signature;
    struct data_array syntax;
    struct hs_syntax_node nodes[HS_CONSOLE_SYNTAX_COUNT];
    long roots[HS_CONSOLE_SOURCE_COUNT];
    char sources[HS_CONSOLE_SOURCE_COUNT][HS_CONSOLE_SOURCE_BYTES];
};

static struct hs_console_state *hs_console_state;
static byte hs_console_colors[HS_CONSOLE_SYNTAX_COUNT];
static unsigned short hs_console_walk[HS_CONSOLE_SYNTAX_COUNT];
static byte hs_console_edges[HS_CONSOLE_SYNTAX_COUNT];

static boolean hs_console_refuse(char const *reason)
{
    error(_error_silent, "console checkpoint state is not taken: %s", reason);
    return FALSE;
}

void hs_console_state_initialize(void)
{
    hs_console_state = game_state_malloc("console syntax and text", "console state",
        sizeof(*hs_console_state));
    hs_console_state_reset();
}

void hs_console_state_reset(void)
{
    if (hs_console_state) {
        memset(hs_console_state, 0, sizeof(*hs_console_state));
        memset(hs_console_state->roots, 0xFF, sizeof(hs_console_state->roots));
    }
}

static void hs_console_source_mark_value(byte *marked, unsigned long value)
{
    uintptr_t offset = (uintptr_t)value - (uintptr_t)hs_console_state->sources;
    if (offset < sizeof(hs_console_state->sources))
        marked[offset / HS_CONSOLE_SOURCE_BYTES] = TRUE;
}

/* Stack arguments are untyped and may be unaligned. Conservatively retaining
 * a slot is safe; scanning only live frames avoids pinning discarded stacks.
 * This runs when a command is compiled, never on the simulation tick. */
static boolean hs_console_sources_mark(byte *marked)
{
    long index;
    memset(marked, 0, HS_CONSOLE_SOURCE_COUNT);
    if (hs_thread_data && hs_thread_data->valid) {
        for (index = data_next_index(hs_thread_data, NONE); index != NONE;
             index = data_next_index(hs_thread_data, index)) {
            struct hs_thread_datum *thread = hs_thread_get(index);
            struct hs_stack_frame const *frame = thread->stack;
            byte const *end;
            byte const *cursor;
            unsigned long value;
            long root = NONE;
            long steps = 0;
            long slot;
            if ((uintptr_t)frame < (uintptr_t)thread->stack_data ||
                (uintptr_t)frame > (uintptr_t)mcc_hs_stack_end(thread) - sizeof(*frame) ||
                frame->size < 0 || (unsigned long)frame->size >
                (uintptr_t)mcc_hs_stack_end(thread) - (uintptr_t)frame->data) return FALSE;
            end = frame->data + frame->size;
            hs_console_source_mark_value(marked, thread->result);
            for (cursor = thread->stack_data; cursor + sizeof(value) <= end; ++cursor) {
                memcpy(&value, cursor, sizeof(value));
                hs_console_source_mark_value(marked, value);
            }
            while ((byte const *)frame != thread->stack_data) {
                uintptr_t previous = (uintptr_t)frame->previous;
                root = frame->expression_index;
                if (++steps > (mcc_hs_stack_end(thread) -
                    thread->stack_data) / (long)sizeof(*frame) ||
                    previous < (uintptr_t)thread->stack_data ||
                    previous > (uintptr_t)frame - sizeof(*frame)) return FALSE;
                frame = frame->previous;
            }
            if (thread->type == _hs_thread_type_console_command && root != NONE) {
                for (slot = 0; slot < HS_CONSOLE_SOURCE_COUNT; ++slot)
                    if (hs_console_state->roots[slot] == root) marked[slot] = TRUE;
            }
        }
    }
    if (hs_global_data && hs_global_data->valid) {
        for (index = data_next_index(hs_global_data, NONE); index != NONE;
             index = data_next_index(hs_global_data, index)) {
            struct hs_global_datum const *global = datum_get(hs_global_data, index);
            hs_console_source_mark_value(marked, global->value);
        }
    }
    return TRUE;
}

char *hs_console_source_acquire(long size)
{
    byte marked[HS_CONSOLE_SOURCE_COUNT];
    long slot;
    if (!hs_console_state || size < 0 || size >= HS_CONSOLE_SOURCE_BYTES ||
        !hs_console_sources_mark(marked)) return NULL;
    for (slot = 0; slot < HS_CONSOLE_SOURCE_COUNT; ++slot) {
        if (!marked[slot]) {
            hs_console_state->roots[slot] = NONE;
            memset(hs_console_state->sources[slot], 0, HS_CONSOLE_SOURCE_BYTES);
            return hs_console_state->sources[slot];
        }
    }
    return NULL;
}

void hs_console_source_finish(char *source, long expression)
{
    uintptr_t offset;
    if (!hs_console_state || !source) return;
    offset = (uintptr_t)source - (uintptr_t)hs_console_state->sources;
    if (offset < sizeof(hs_console_state->sources) && !(offset % HS_CONSOLE_SOURCE_BYTES))
        hs_console_state->roots[offset / HS_CONSOLE_SOURCE_BYTES] = expression;
}

static struct hs_syntax_node const *hs_console_saved_node(struct hs_console_state const *state,
    long handle)
{
    unsigned index = (unsigned long)handle & 0xFFFFu;
    if (handle == NONE || index >= (unsigned)state->syntax.count ||
        !state->nodes[index].datum_header || (unsigned short)state->nodes[index].datum_header !=
        (unsigned short)((unsigned long)handle >> 16)) return NULL;
    return state->nodes + index;
}

static boolean hs_console_saved_string(struct hs_console_state const *state, long value)
{
    uintptr_t offset = (uintptr_t)(unsigned long)value - (uintptr_t)hs_console_state->sources;
    if (offset >= sizeof(state->sources)) return FALSE;
    return memchr((char const *)state->sources + offset, 0,
        HS_CONSOLE_SOURCE_BYTES - offset % HS_CONSOLE_SOURCE_BYTES) != NULL;
}

/* Only reachable temporary nodes are interpreted. Failed parses can leave
 * incomplete, unreachable nodes until the existing collector next runs. */
static boolean hs_console_saved_graph(struct hs_console_state const *state, long root)
{
    unsigned depth = 0;
    struct hs_syntax_node const *node = hs_console_saved_node(state, root);
    unsigned first;
    if (!node) return FALSE;
    first = (unsigned)(node - state->nodes);
    if (hs_console_colors[first] == 2) return TRUE;
    hs_console_colors[first] = 1;
    hs_console_walk[depth] = (unsigned short)first;
    hs_console_edges[depth++] = 0;
    while (depth) {
        unsigned index = hs_console_walk[depth - 1];
        unsigned edge = hs_console_edges[depth - 1]++;
        long link;
        unsigned child;
        node = state->nodes + index;
        if (!edge && !(node->flags & 8)) {
            struct hs_syntax_node const *next;
            if (node->flags & ~15 || node->type < _hs_unparsed || node->type >= NUMBER_OF_HS_TYPES)
                return FALSE;
            /* Next links are value arguments; a function-name node belongs
             * only behind a call's data link, including unbounded forms. */
            if (node->next_node_index != NONE &&
                (!(next = hs_console_saved_node(state, node->next_node_index)) || !hs_type_valid(next->type)))
                return FALSE;
            if ((node->flags & 1) && !(node->flags & 4) && hs_type_valid(node->type)) {
                if (!hs_type_valid(node->constant_type)) return FALSE;
                if (node->constant_type == _hs_type_string && !hs_console_saved_string(state, node->data))
                    return FALSE;
            }
            if (!(node->flags & 1) && (node->flags & 2) &&
                (global_scenario_index == NONE || node->script_index < 0 ||
                 node->script_index >= global_scenario_get()->hs_scripts.count)) return FALSE;
        }
        if (edge == 2) {
            hs_console_colors[index] = 2;
            --depth;
            continue;
        }
        link = edge == 0 ? node->next_node_index : ((node->flags & 1) ? NONE : node->data);
        if (link == NONE) continue;
        node = hs_console_saved_node(state, link);
        if (!node) return FALSE;
        child = (unsigned)(node - state->nodes);
        if (hs_console_colors[child] == 1) return FALSE;
        if (hs_console_colors[child] == 2) continue;
        if (depth >= (unsigned)state->syntax.count) return FALSE;
        hs_console_colors[child] = 1;
        hs_console_walk[depth] = (unsigned short)child;
        hs_console_edges[depth++] = 0;
    }
    return TRUE;
}

static void const *hs_console_image_pointer(void const *image, void const *base,
    unsigned long bytes, void const *pointer, unsigned long size)
{
    uintptr_t offset = (uintptr_t)pointer - (uintptr_t)base;
    if (!image || !base || offset > bytes || size > bytes - offset) return NULL;
    return (byte const *)image + offset;
}

/* A sleeping command can still reach a literal which has not entered its
 * stack yet. Its root must retain that literal's source slot after a load. */
static boolean hs_console_saved_source_owner(struct hs_console_state const *state, long root)
{
    struct hs_syntax_node const *node = hs_console_saved_node(state, root);
    unsigned depth = 0, slot;
    boolean found = FALSE;
    if (!node) return FALSE;
    if (node->flags & 8) return TRUE; /* hs_script_start evaluates authored roots too. */
    for (slot = 0; slot < HS_CONSOLE_SOURCE_COUNT; ++slot)
        if (state->roots[slot] == root) found = TRUE;
    if (!found) return FALSE;
    memset(hs_console_edges, 0, sizeof(hs_console_edges));
    hs_console_walk[depth++] = (unsigned short)(node - state->nodes);
    hs_console_edges[node - state->nodes] = 1;
    while (depth) {
        unsigned edge;
        node = state->nodes + hs_console_walk[--depth];
        if (!(node->flags & 8) && (node->flags & 1) && !(node->flags & 4) &&
            node->constant_type == _hs_type_string && hs_type_valid(node->type)) {
            uintptr_t offset = (uintptr_t)(unsigned long)node->data - (uintptr_t)hs_console_state->sources;
            if (offset >= sizeof(state->sources) ||
                state->roots[offset / HS_CONSOLE_SOURCE_BYTES] != root) return FALSE;
        }
        for (edge = 0; edge < 2; ++edge) {
            long link = edge ? ((node->flags & 1) ? NONE : node->data) : node->next_node_index;
            struct hs_syntax_node const *child;
            unsigned index;
            if (link == NONE) continue;
            child = hs_console_saved_node(state, link);
            if (!child) return FALSE;
            index = (unsigned)(child - state->nodes);
            if (hs_console_edges[index]) continue;
            if (depth >= HS_CONSOLE_SYNTAX_COUNT) return FALSE;
            hs_console_edges[index] = 1;
            hs_console_walk[depth++] = (unsigned short)index;
        }
    }
    return TRUE;
}

static boolean hs_console_saved_threads(struct hs_console_state const *state,
    void const *image, void const *base, unsigned long bytes)
{
    struct data_array const *array;
    unsigned slot;
    array = hs_console_image_pointer(image, base, bytes, hs_thread_data, sizeof(*array));
    if (!array || array->data != hs_thread_data->data || array->size != hs_thread_data->size ||
        array->maximum_count != hs_thread_data->maximum_count || array->count < 0 ||
        array->count > array->maximum_count) return FALSE;
    for (slot = 0; slot < (unsigned)array->count; ++slot) {
        struct hs_thread_datum const *live = (void const *)((byte const *)hs_thread_data->data + slot * array->size);
        struct hs_thread_datum const *thread = hs_console_image_pointer(image, base, bytes, live, array->size);
        uintptr_t first = (uintptr_t)live->stack_data;
        uintptr_t end = (uintptr_t)mcc_hs_stack_end((struct hs_thread_datum *)live);
        uintptr_t cursor;
        long root = NONE;
        unsigned steps = 0;
        if (!thread) return FALSE;
        if (!thread->identifier) continue;
        if (thread->type >= NUMBER_OF_HS_THREAD_TYPES) return FALSE;
        cursor = (uintptr_t)thread->stack;
        for (;;) {
            struct hs_stack_frame const *frame;
            struct hs_stack_frame const *previous_frame;
            struct hs_syntax_node const *expression;
            uintptr_t previous, result;
            if (cursor < first || cursor > end - sizeof(*frame) ||
                ++steps > (end - first) / sizeof(*frame)) return FALSE;
            frame = hs_console_image_pointer(image, base, bytes, (void const *)cursor, sizeof(*frame));
            if (!frame || frame->size < 0 ||
                (unsigned long)frame->size > end - cursor - offsetof(struct hs_stack_frame, data)) return FALSE;
            if (cursor == first) {
                if (frame->previous) return FALSE;
                break;
            }
            expression = hs_console_saved_node(state, frame->expression_index);
            if (!expression || !hs_type_valid(expression->type) || (expression->flags & 1) ||
                !hs_console_saved_graph(state, frame->expression_index)) return FALSE;
            root = frame->expression_index;
            previous = (uintptr_t)frame->previous;
            if (previous < first || previous > cursor - sizeof(*frame)) return FALSE;
            previous_frame = hs_console_image_pointer(image, base, bytes,
                (void const *)previous, sizeof(*previous_frame));
            if (!previous_frame || previous_frame->size < 0 ||
                previous + sizeof(*frame) + (unsigned long)previous_frame->size != cursor) return FALSE;
            result = (uintptr_t)previous_frame->result;
            if (result != (uintptr_t)&live->result &&
                (previous_frame->size < sizeof(long) ||
                 result < previous + offsetof(struct hs_stack_frame, data) ||
                 result > previous + offsetof(struct hs_stack_frame, data) + previous_frame->size - sizeof(long)))
                return FALSE;
            cursor = previous;
        }
        if (thread->type == _hs_thread_type_console_command && root != NONE &&
            !hs_console_saved_source_owner(state, root)) return FALSE;
    }
    return TRUE;
}

boolean hs_console_state_validate(void const *image, void const *base, unsigned long bytes)
{
    struct hs_console_state const *state;
    struct hs_syntax_node const *live_nodes;
    struct data_array *saved_syntax;
    struct data_array view;
    unsigned index, actual = 0;
    boolean valid = TRUE;
    if (!hs_console_state || !hs_syntax_data) return hs_console_refuse("syntax is unavailable");
    state = hs_console_image_pointer(image, base, bytes, hs_console_state, sizeof(*state));
    if (!state || state->signature != 0x31534348u) return hs_console_refuse("missing syntax snapshot");
    if (state->syntax.signature != 'd@t@' || !memchr(state->syntax.name, 0, sizeof(state->syntax.name)) ||
        state->syntax.maximum_count != hs_syntax_data->maximum_count ||
        state->syntax.maximum_count <= 0 || state->syntax.maximum_count > HS_CONSOLE_SYNTAX_COUNT ||
        state->syntax.size != sizeof(struct hs_syntax_node) || state->syntax.data || state->syntax.valid != 1 ||
        state->syntax.identifier_zero_invalid > 1 || !state->syntax.next_identifier ||
        state->syntax.count < 0 || state->syntax.count > state->syntax.maximum_count ||
        state->syntax.actual_count < 0 || state->syntax.actual_count > state->syntax.count ||
        state->syntax.first_free_absolute_index < 0 ||
        state->syntax.first_free_absolute_index > state->syntax.maximum_count)
        return hs_console_refuse("invalid syntax allocator");
    live_nodes = hs_syntax_data->data;
    memset(hs_console_colors, 0, sizeof(hs_console_colors));
    for (index = 0; index < (unsigned)state->syntax.maximum_count; ++index) {
        struct hs_syntax_node const *node = state->nodes + index;
        boolean permanent = live_nodes[index].datum_header && (live_nodes[index].flags & 8);
        if (node->datum_header) {
            if (index >= (unsigned)state->syntax.count) return hs_console_refuse("syntax exceeds its count");
            ++actual;
        }
        if (permanent || (node->datum_header && (node->flags & 8))) {
            if (!permanent || memcmp(node, live_nodes + index, sizeof(*node)))
                return hs_console_refuse("scenario syntax changed");
        }
    }
    if (actual != (unsigned)state->syntax.actual_count ||
        !hs_console_saved_threads(state, image, base, bytes))
        return hs_console_refuse("invalid saved syntax references");
    /* The existing checker resolves links through hs_syntax_data. No script
     * executes here; restore the live pointer on every result. */
    view = state->syntax;
    view.data = (void *)state->nodes;
    saved_syntax = hs_syntax_data;
    hs_syntax_data = &view;
    for (index = 0; index < (unsigned)state->syntax.count && valid; ++index) {
        if (hs_console_colors[index] && !(state->nodes[index].flags & 8))
            valid = hs_console_syntax_node_valid(state->nodes + index);
    }
    hs_syntax_data = saved_syntax;
    return valid || hs_console_refuse("invalid console call");
}

boolean hs_console_state_capture(void)
{
    unsigned long used, capacity;
    void *base;
    if (!hs_console_state || !hs_syntax_data || hs_syntax_data->maximum_count <= 0 ||
        hs_syntax_data->maximum_count > HS_CONSOLE_SYNTAX_COUNT ||
        hs_syntax_data->size != sizeof(struct hs_syntax_node))
        return hs_console_refuse("syntax cannot be captured");
    hs_console_state->signature = 0x31534348u; /* HCS1 */
    hs_console_state->syntax = *hs_syntax_data;
    hs_console_state->syntax.data = NULL;
    memcpy(hs_console_state->nodes, hs_syntax_data->data,
        hs_syntax_data->maximum_count * sizeof(struct hs_syntax_node));
    base = game_state_port_cpu_region(&used, &capacity);
    return hs_console_state_validate(base, base, capacity);
}

void hs_console_state_restore(void)
{
    void *data = hs_syntax_data->data;
    *hs_syntax_data = hs_console_state->syntax;
    hs_syntax_data->data = data;
    memcpy(data, hs_console_state->nodes, hs_syntax_data->maximum_count * sizeof(struct hs_syntax_node));
}
