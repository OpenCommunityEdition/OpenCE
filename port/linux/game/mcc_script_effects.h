#ifndef MCC_SCRIPT_EFFECTS_H
#define MCC_SCRIPT_EFFECTS_H
#include "cseries.h"
enum mcc_script_effect {
    MCC_EFFECT_JETPACK, MCC_EFFECT_SUPER_JUMP, MCC_EFFECT_BOTTOMLESS_CLIP,
    MCC_EFFECT_POSSESSION, MCC_EFFECT_MEDUSA, MCC_EFFECT_OMNIPOTENCE, MCC_EFFECT_DEATHLESS,
    MCC_EFFECT_COUNT
};
#define MCC_EFFECT_MASK 127u
boolean mcc_script_effect_global(char const *name);
void mcc_script_effects_begin(void);
void mcc_script_effects_dispose(void);
unsigned long mcc_script_effects_snapshot(void);
void mcc_script_effects_restore(unsigned long flags);
boolean mcc_script_effect_enabled(short effect);
boolean mcc_script_effects_bump(long source, long target);
boolean mcc_script_effects_host_movement(void);
boolean mcc_script_effects_preserve_body(void);
#endif
