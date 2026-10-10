/* MCC-only data carried in the checked, otherwise unused CPU arena tail. */
#ifndef MCC_CHECKPOINT_H
#define MCC_CHECKPOINT_H
int mcc_checkpoint_capture(void *image, unsigned long total_bytes,
    unsigned long cpu_used, unsigned long cpu_capacity);
int mcc_checkpoint_validate(void const *image, unsigned long total_bytes,
    unsigned long cpu_used, unsigned long cpu_capacity, void const *current_image);
int mcc_checkpoint_restore(void const *image, unsigned long total_bytes,
    unsigned long cpu_used, unsigned long cpu_capacity);
void mcc_checkpoint_dispose(void);
#endif
