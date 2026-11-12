#ifndef MCC_SCRIPTS_H
#define MCC_SCRIPTS_H
struct hs_function_definition;
struct hs_syntax_node;
struct data_array;
struct mcc_runtime;
/* These indices are outside the unchanged Xbox/CE function table. */
#define MCC_HS_DISTANCE_TO_OBJECT 0x7000
#define MCC_HS_MISSION_SEGMENT 0x7001
#define MCC_HS_CORE_SEGMENT 0x7002
#define MCC_HS_PLAYER_EQUIPMENT 0x7003
#define MCC_HS_ABS_INTEGER 0x7004
#define MCC_HS_EXECUTING_THREAD 0x7005
#define MCC_HS_KILL_THREAD 0x7006
#define MCC_HS_AI_DESELECT 0x7007
/* Implemented beside the interpreter's private thread and stack types. */
void mcc_executing_thread_evaluate(short function, long thread, boolean initialize);
void mcc_kill_thread_evaluate(short function, long thread, boolean initialize);
struct hs_function_definition *mcc_script_function(short index);
short mcc_script_find(char const *name);
boolean mcc_script_call_valid(struct hs_syntax_node const *call, struct data_array *syntax);
boolean mcc_script_call_arguments_valid(struct hs_syntax_node const *call, struct data_array *syntax);
int mcc_scripts_prepare(struct mcc_runtime *runtime);
/* MCC permits an AI reference in an object/unit or object-list expression. */
int mcc_script_parse_ai(struct hs_syntax_node *node, char const *text);
boolean mcc_script_ai_cast(short actual_type, short desired_type);
boolean mcc_script_can_cast(short actual_type, short desired_type);
long mcc_script_cast(short actual_type, short desired_type, long value);
long mcc_script_cast_ai(short desired_type, long reference);
#endif
