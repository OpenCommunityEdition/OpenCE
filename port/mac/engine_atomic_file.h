#ifndef HALO_MAC_ENGINE_ATOMIC_FILE_H
#define HALO_MAC_ENGINE_ATOMIC_FILE_H
#include <stdio.h>
/* Apply only to errors.c: its one intrinsic publishes a FILE pointer. */
FILE *mac_atomic_file_compare_exchange(FILE *volatile *,FILE *,FILE *);
#define __sync_val_compare_and_swap mac_atomic_file_compare_exchange
#endif
