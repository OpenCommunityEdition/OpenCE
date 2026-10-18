#ifndef HALO_MAC_ENGINE_WIDE_H
#define HALO_MAC_ENGINE_WIDE_H
/* Include explicitly, never add the Linux headers ahead of Darwin's libc. */
#include "../linux/include/wchar.h"
wchar_t **mac_wide_tokenizer_state(void);
#endif
