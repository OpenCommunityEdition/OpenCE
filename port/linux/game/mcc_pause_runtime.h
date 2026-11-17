#ifndef MCC_PAUSE_RUNTIME_H
#define MCC_PAUSE_RUNTIME_H

boolean mcc_pause_runtime_load(void);
void mcc_pause_runtime_unload(void);
boolean mcc_pause_runtime_active(void);
long mcc_pause_runtime_screen(short local_count, boolean first_player);

#endif
