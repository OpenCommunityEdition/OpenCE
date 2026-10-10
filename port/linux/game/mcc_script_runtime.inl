/* Included after the native interpreter's declarations. This MCC-only
 * adapter uses its existing stack service, so arguments, recursion, sleep
 * and checkpoint state remain inside each native 512-byte thread stack. */
#include "mcc_script_parameters.h"
#include "mcc_campaign.h"
long hs_cast(long thread_index, short actual_type, short desired_type, long value);

void mcc_sleep_forever_evaluate(short function, long thread_index, boolean initialize)
{
    struct hs_thread_datum *thread=hs_thread_get(thread_index);
    long *script=hs_stack_allocate(thread_index,sizeof(long));
    long target=thread_index;
    (void)function;
    if (!script) return;
    if (initialize) {
        struct hs_syntax_node *call=hs_syntax_get(thread->stack->expression_index);
        struct hs_syntax_node *predicate=call ? hs_syntax_get(call->data) : NULL;
        if (!predicate) { hs_syntax_error(thread_index); return; }
        *script=NONE;
        if (predicate->next_node_index!=NONE) {
            hs_evaluate(thread_index,predicate->next_node_index,script);
            return;
        }
    }
    if ((short)*script!=NONE) target=hs_find_thread_by_script((short)*script);
    if (target!=NONE) {
        struct hs_thread_datum *sleeper=hs_thread_get(target);
        if (sleeper->sleep_until!=NONE) {
            if (target!=thread_index && !TEST_FLAG(sleeper->flags,_hs_thread_sleeping_bit)) {
                sleeper->previous_sleep_until=sleeper->sleep_until;
                SET_FLAG(sleeper->flags,_hs_thread_sleeping_bit,TRUE);
            }
            sleeper->sleep_until=NONE-1;
        }
    }
    hs_return(thread_index,0);
}

struct mcc_hs_arguments {
    unsigned long magic;
    short script, count, next, phase;
    unsigned long references;
    long expression, result;
    long values[1];
};
#define MCC_HS_ARGUMENT_MAGIC 0x5043434DUL

static long mcc_hs_arguments_size(short count)
{
    return (long)offsetof(struct mcc_hs_arguments, values) + count * (long)sizeof(long);
}

static struct mcc_hs_arguments *mcc_hs_frame(struct hs_stack_frame *frame)
{
    struct hs_syntax_node *call;
    struct mcc_hs_arguments *state;
    struct mcc_script_signature const *signature;
    if (!frame || frame->size < (long)offsetof(struct mcc_hs_arguments, values) ||
        !(call = hs_syntax_get(frame->expression_index)) || (call->flags & 3) != 2 ||
        !(signature = mcc_parameters_signature(call->script_index)) || !signature->count) return NULL;
    state = (struct mcc_hs_arguments *)frame->data;
    return frame->size >= mcc_hs_arguments_size(signature->count) &&
        state->magic == MCC_HS_ARGUMENT_MAGIC && state->script == call->script_index &&
        state->count == signature->count ? state : NULL;
}

static void mcc_hs_release_arguments(struct mcc_hs_arguments *state)
{
    short i;
    for (i = 0; i < state->count; ++i)
        if (state->references & (1u << i)) object_list_remove_reference(state->values[i]);
    state->references = 0;
}

static void mcc_hs_release_thread(long thread_index)
{
    struct hs_thread_datum *thread = hs_thread_get(thread_index);
    struct hs_stack_frame *frame;
    if (!mcc_parameters_signature(0)) return;
    for (frame = thread->stack; frame && frame != (struct hs_stack_frame *)thread->stack_data;
        frame = frame->previous) {
        struct mcc_hs_arguments *state = mcc_hs_frame(frame);
        if (state) mcc_hs_release_arguments(state);
    }
}

static long *mcc_hs_parameter_address(long thread_index, struct hs_syntax_node *node, short *type)
{
    struct hs_thread_datum *thread = hs_thread_get(thread_index);
    struct hs_stack_frame *frame;
    short owner;
    int value_type = mcc_parameter_type(node, &owner);
    if (value_type < 0) return NULL;
    *type = (short)value_type;
    for (frame = thread->stack; frame && frame != (struct hs_stack_frame *)thread->stack_data;
        frame = frame->previous) {
        struct mcc_hs_arguments *state = mcc_hs_frame(frame);
        /* A nested call still evaluating actual arguments is in its caller's
         * lexical scope, even when it recursively names the same script. */
        if (state && state->phase == 1) {
            if (state->script != owner || node->data >= state->count) return NULL;
            return &state->values[node->data];
        }
    }
    return NULL;
}

static boolean mcc_hs_parameter_evaluate(long thread_index, struct hs_syntax_node *node, long *destination)
{
    long *value;
    short type;
    if (mcc_parameter_type(node, NULL) == -1) return FALSE;
    value = mcc_hs_parameter_address(thread_index, node, &type);
    if (!value) hs_syntax_error(thread_index);
    else *destination = hs_cast(thread_index, type, node->type, *value);
    return TRUE;
}

static boolean mcc_hs_script_evaluate(short script_index, long thread_index, boolean initialize)
{
    struct mcc_script_signature const *signature = mcc_parameters_signature(script_index);
    struct hs_thread_datum *thread;
    struct hs_script *script;
    struct mcc_hs_arguments *state;
    struct hs_syntax_node *argument;
    if (!signature || !signature->count) return FALSE;
    thread = hs_thread_get(thread_index);
    script = TAG_BLOCK_GET_ELEMENT(&global_scenario_get()->hs_scripts, script_index, struct hs_script);
    state = hs_stack_allocate(thread_index, mcc_hs_arguments_size(signature->count));
    if (!state) return TRUE;
    if (initialize) {
        struct hs_syntax_node *call = hs_syntax_get(thread->stack->expression_index);
        struct hs_syntax_node *predicate = call ? hs_syntax_get(call->data) : NULL;
        if (!predicate) { hs_syntax_error(thread_index); return TRUE; }
        csmemset(state, 0, mcc_hs_arguments_size(signature->count));
        state->magic = MCC_HS_ARGUMENT_MAGIC; state->script = script_index;
        state->count = signature->count; state->expression = predicate->next_node_index;
        state->result = hs_type_default_value(script->return_type);
    }
    if (state->magic != MCC_HS_ARGUMENT_MAGIC || state->script != script_index ||
        state->count != signature->count || state->next < 0 || state->next > state->count ||
        state->phase < 0 || state->phase > 1) { hs_syntax_error(thread_index); return TRUE; }
    if (!state->phase) {
        if (state->next && signature->types[state->next - 1] == _hs_type_object_list &&
            !(state->references & (1u << (state->next - 1)))) {
            object_list_add_reference(state->values[state->next - 1]);
            state->references |= 1u << (state->next - 1);
        }
        if (state->next < state->count) {
            long expression = state->expression;
            short slot = state->next++;
            argument = hs_syntax_get(expression);
            if (!argument || argument->type != signature->types[slot]) { hs_syntax_error(thread_index); return TRUE; }
            state->expression = argument->next_node_index;
            hs_evaluate(thread_index, expression, &state->values[slot]);
            return TRUE;
        }
        if (state->expression != NONE) { hs_syntax_error(thread_index); return TRUE; }
        state->phase = 1;
        if (hs_scenario_script_disabled(script_index)) {
            mcc_hs_release_arguments(state);
            hs_return(thread_index, state->result);
        } else hs_evaluate(thread_index, script->root_expression_index, &state->result);
    } else {
        long result = state->result;
        mcc_hs_release_arguments(state);
        hs_return(thread_index, result);
    }
    return TRUE;
}

static boolean mcc_hs_parameter_set(long thread_index, struct hs_syntax_node *variable, boolean initialize)
{
    short type;
    long *destination, *value;
    if (mcc_parameter_type(variable, NULL) == -1) return FALSE;
    destination = mcc_hs_parameter_address(thread_index, variable, &type);
    value = hs_stack_allocate(thread_index, sizeof(long));
    if (!value) return TRUE;
    if (!destination) { hs_syntax_error(thread_index); return TRUE; }
    if (initialize) hs_evaluate(thread_index, variable->next_node_index, value);
    else {
        if (type == _hs_type_object_list) {
            object_list_remove_reference(*destination);
            object_list_add_reference(*value);
        }
        *destination = *value;
        hs_return(thread_index, *value);
    }
    return TRUE;
}
