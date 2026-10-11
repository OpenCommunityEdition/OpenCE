#ifndef MCC_CAMPAIGN_H
#define MCC_CAMPAIGN_H
#include "cseries.h"
struct hs_function_definition;
#define MCC_CAMPAIGN_SNAPSHOT_V1_BYTES 752u
#define MCC_CAMPAIGN_SNAPSHOT_BYTES 764u
#define MCC_HS_CAMPAIGN_FIRST 0x7010
struct hs_function_definition *mcc_campaign_function(short index);
short mcc_campaign_find(char const *name);
boolean mcc_campaign_global_settable(short index);
void mcc_campaign_begin(void);
void mcc_campaign_dispose(void);
void mcc_campaign_render(short local_player);
void mcc_campaign_snapshot(void *out);
real mcc_campaign_effects_gain(void);
real mcc_campaign_mix_gain(boolean music);
int mcc_campaign_validate(void const *in, unsigned long bytes);
int mcc_campaign_restore(void const *in, unsigned long bytes);
/* Implemented beside the native interpreter's private stack declarations. */
void mcc_sleep_forever_evaluate(short function, long thread, boolean initialize);
void mcc_script_finished_evaluate(short function, long thread, boolean initialize);
#endif
