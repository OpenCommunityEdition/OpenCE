#ifndef NETWORK_KILLFEED_H
#define NETWORK_KILLFEED_H
#include "cseries.h"
struct damage_data;
void network_killfeed_reset(void);
void network_killfeed_note_death(long unit_index, struct damage_data const *damage);
long network_killfeed_entry_size(void);
void network_killfeed_handle_message(void const *entries, word size, byte count, boolean from_stream);
#endif
