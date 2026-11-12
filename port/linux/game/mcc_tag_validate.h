/* Independent MCC validation context and dispatch for shared schema callbacks. */
#ifndef MCC_TAG_VALIDATE_H
#define MCC_TAG_VALIDATE_H

#include "cseries.h"
#include <stdarg.h>

struct mcc_runtime;
struct tag_validation;

int mcc_tags_validate(struct mcc_runtime *runtime);
int mcc_bsp_validate(struct mcc_runtime *runtime, long handle, void *address, long size);
void mcc_validation_dispose(struct mcc_runtime *runtime);

/* Identity comparisons only: a legacy validator is never read as an MCC one. */
boolean mcc_validation_owns(struct tag_validation *validation);
boolean mcc_validation_callback_active(void);
void mcc_validation_message(struct tag_validation *validation, boolean refuse,
    char const *format, va_list arguments);
boolean mcc_validation_file_contains(struct tag_validation *validation, long offset, long size);
void *mcc_validation_root(struct tag_validation *validation);
boolean mcc_validation_contains(struct tag_validation *validation, void const *address, unsigned long size);
void *mcc_validation_tag_get(struct tag_validation *validation, long handle, unsigned long group);
void *mcc_validation_buffer_data(struct tag_validation *validation, void const *buffer, boolean indices);
boolean mcc_validation_any_claimed(void const *address, unsigned long size);

/* Optional MCC callback routing. The stock standalone validator has no MCC
 * runtime dependency: this pointer is NULL except inside an MCC callback. */
struct mcc_validation_dispatch {
    boolean (*owns)(struct tag_validation *);
    void (*message)(struct tag_validation *, boolean, char const *, va_list);
    boolean (*file_contains)(struct tag_validation *, long, long);
    void *(*root)(struct tag_validation *);
    boolean (*contains)(struct tag_validation *, void const *, unsigned long);
    void *(*tag_get)(struct tag_validation *, long, unsigned long);
    void *(*buffer_data)(struct tag_validation *, void const *, boolean);
    boolean (*any_claimed)(void const *, unsigned long);
};
extern struct mcc_validation_dispatch const *mcc_validation_callbacks;

#endif
