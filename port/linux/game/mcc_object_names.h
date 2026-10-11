/* Expanded MCC names keep the legacy table and game-state layout intact. */
#ifndef MCC_OBJECT_NAMES_H
#define MCC_OBJECT_NAMES_H
#define MCC_OBJECT_NAMES_MAXIMUM 32767
#define MCC_OBJECT_NAMES_SNAPSHOT_MAXIMUM (4u * MCC_OBJECT_NAMES_MAXIMUM)
int mcc_object_names_active(void);
long mcc_object_names_count(void);
void mcc_object_names_clear(void);
long mcc_object_names_get(short name);
void mcc_object_names_set(short name, long object);
void mcc_object_names_claim(short name, long object);
void mcc_object_names_delete(long object);
unsigned long mcc_object_names_snapshot(void *out);
int mcc_object_names_validate(void const *in, unsigned long bytes);
int mcc_object_names_restore(void const *in, unsigned long bytes);
#endif
