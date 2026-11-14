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
/* Implemented beside the interpreter's private thread types (mcc_script_runtime.inl). */
void mcc_script_started_evaluate(short function, long thread, boolean initialize);
#endif
