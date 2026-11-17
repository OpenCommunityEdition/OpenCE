/* Converter tools/tests have no display. The real controller is tested separately. */
#include "../port/linux/game/mcc_loading.h"
/* Deterministic clock: standalone checks exercise the byte budget. */
unsigned long system_milliseconds(void) { return 0; }
int mcc_loading_begin(char const *name) { (void)name; return 0; }
void mcc_loading_stage(enum mcc_loading_stage stage) { (void)stage; }
void mcc_loading_update(enum mcc_loading_stage stage, unsigned long done, unsigned long total)
{ (void)stage; (void)done; (void)total; }
void mcc_loading_pulse(void) { }
void mcc_loading_end(int success) { (void)success; }
