#ifndef HALO_LOOSE_SOUND_H
#define HALO_LOOSE_SOUND_H

#include "cseries/cseries.h"

struct sound_permutation;

/* Desktop-only overrides for sound tags already present in the current map. */
void loose_sound_open(void);
void loose_sound_close(void);
boolean loose_sound_reload(char const *name);
/* Stop voices, release unused samples and select current config.toml mode. */
boolean loose_sound_restart(void);
void *loose_sound_tag_get(long tag_index, void *original);
byte const *loose_sound_samples(struct sound_permutation const *permutation);

#endif
