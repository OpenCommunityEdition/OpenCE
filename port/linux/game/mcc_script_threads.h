#ifndef MCC_SCRIPT_THREADS_H
#define MCC_SCRIPT_THREADS_H
/* Separate MCC thread storage; native thread datums and limits are unchanged. */
int mcc_script_threads_prepare(void);
void mcc_script_threads_dispose(void);
int mcc_script_threads_capture(void);
int mcc_script_threads_validate(void const *image, void const *current,
    unsigned long used, unsigned long capacity);
int mcc_script_threads_restore(void *image, unsigned long used, unsigned long capacity);
#endif
