/* MCC script functions that the Xbox function table does not have, from MCC's
 * editing kit (hs_doc.txt), and MCC's versions of two native functions. */
#ifndef MCC_SCRIPT_LIBRARY_H
#define MCC_SCRIPT_LIBRARY_H
#include "cseries.h"
struct hs_function_definition;
/* After mcc_scripts.h's and mcc_campaign.h's indices. */
#define MCC_HS_LIBRARY_FIRST 0x7100
struct hs_function_definition *mcc_script_library_function(short index);
short mcc_script_library_find(char const *name);
/* At an MCC map's load: the look rates its scripts set are let go. */
void mcc_script_library_reset(void);
/* After the player's look rates are taken for a controller
 * (input_abstraction.c): the rates an MCC map's scripts set instead. */
void mcc_script_look_rates(short controller_index);
/* Implemented beside the interpreter's private thread types (mcc_script_runtime.inl). */
void mcc_script_started_evaluate(short function, long thread, boolean initialize);
void mcc_kill_active_scripts_evaluate(short function, long thread, boolean initialize);
#endif
