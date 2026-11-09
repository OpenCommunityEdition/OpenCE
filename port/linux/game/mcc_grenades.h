#ifndef MCC_GRENADES_H
#define MCC_GRENADES_H
#include <stdint.h>

boolean mcc_grenades_active(void);
short mcc_grenades_type_count(void);
short mcc_grenades_get(long unit,short type);
void mcc_grenades_set(long unit,short type,short count);
short mcc_grenades_total(long unit);
short mcc_grenades_next(long unit,short current,short delta);
short mcc_grenades_add(long unit,short type,short count);
/* Clear/reset affect only the independently owned extra-slot state. */
void mcc_grenades_clear(long unit);
void mcc_grenades_reset(void);
void mcc_grenades_initialize_unit(long unit);
boolean mcc_grenades_pickup(long unit,long equipment);
void mcc_grenades_move_to_hand(long unit);
void mcc_grenades_drop_extra(long unit,void (*drop_item)(long,long));
void mcc_grenades_hud(short local_player,long unit);

/* Stable little-endian snapshot: GMC4, version u16=1, record count u16;
 * records are full salted handle u32, slot2 u8, slot3 u8, zero u16.
 * Selection already belongs to the native unit game-state image. */
uint32_t mcc_grenades_snapshot(void *out,uint32_t capacity);
int mcc_grenades_snapshot_validate(void const *in,uint32_t bytes);
int mcc_grenades_restore(void const *in,uint32_t bytes);
#endif
