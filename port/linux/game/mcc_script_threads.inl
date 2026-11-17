/* Included by hs_runtime.c after its private types. MCC threads use a bounded
 * array in checked, otherwise unused CPU-image space. Inline native stacks,
 * allocation records and Xbox/CE checkpoints keep their original layout. */
#include "mcc_script_threads.h"
#include "mcc_checkpoint.h"
#include "mcc_campaign.h"
#include "saved games/game_state.h"
#include "../../third_party/zlib/zlib_prefixed.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MCC_SCRIPT_STACK_BYTES 4096
#define MCC_SCRIPT_THREAD_COUNT 256
#define MCC_SCRIPT_THREAD_BYTES (offsetof(struct hs_thread_datum,stack_data)+MCC_SCRIPT_STACK_BYTES)
#define MCC_SCRIPT_THREAD_STORAGE (16+sizeof(struct data_array)+MCC_SCRIPT_THREAD_COUNT*MCC_SCRIPT_THREAD_BYTES)

static struct {
    byte *storage, *original;
    struct data_array *array, *native;
} mcc_threads;

static byte *mcc_hs_stack_end(struct hs_thread_datum *thread)
{
    if (mcc_threads.array && hs_thread_data==mcc_threads.array)
        return (byte *)thread+MCC_SCRIPT_THREAD_BYTES;
    return thread->stack_data+HS_THREAD_STACK_SIZE;
}

static unsigned long mcc_threads_checksum(byte const *bytes, unsigned long count)
{
    return (uint32_t)crc32(0, bytes, (uInt)count);
}

int mcc_script_threads_prepare(void)
{
    unsigned long used,capacity;
    void *image=game_state_port_cpu_region(&used,&capacity);
    byte *storage=mcc_checkpoint_script_storage((void *)image,used,capacity,MCC_SCRIPT_THREAD_STORAGE);
    if (mcc_threads.storage || !storage || !hs_thread_data ||
        hs_thread_data->size!=sizeof(struct hs_thread_datum) ||
        hs_thread_data->maximum_count!=MCC_SCRIPT_THREAD_COUNT) return 0;
    mcc_threads.original=malloc(MCC_SCRIPT_THREAD_STORAGE);
    if (!mcc_threads.original) return 0;
    memcpy(mcc_threads.original,storage,MCC_SCRIPT_THREAD_STORAGE);
    memset(storage,0,MCC_SCRIPT_THREAD_STORAGE);
    mcc_threads.storage=storage;
    mcc_threads.native=hs_thread_data;
    mcc_threads.array=(struct data_array *)(storage+16);
    data_initialize(mcc_threads.array,"MCC hs thread",MCC_SCRIPT_THREAD_COUNT,MCC_SCRIPT_THREAD_BYTES);
    return 1;
}

static void mcc_hs_threads_activate(void)
{
    if (mcc_cache_tags_loaded() && mcc_threads.array) {
        mcc_campaign_reset_model_lighting();
        data_make_invalid(mcc_threads.native);
        hs_thread_data=mcc_threads.array;
    }
}

void mcc_script_threads_dispose(void)
{
    if (mcc_threads.storage) {
        if (hs_thread_data==mcc_threads.array) {
            hs_thread_data=mcc_threads.native;
            /* An old checkpoint may have made the dormant native array valid. */
            data_make_invalid(mcc_threads.native);
        }
        memcpy(mcc_threads.storage,mcc_threads.original,MCC_SCRIPT_THREAD_STORAGE);
        free(mcc_threads.original);
    }
    memset(&mcc_threads,0,sizeof(mcc_threads));
}

/* Saved frame pointers refer to the fixed live CPU arena. Check against that
 * thread's own stack, not against the whole image or another thread's buffer. */
static int mcc_threads_thread_valid(struct hs_thread_datum const *thread,
    struct hs_thread_datum const *live, unsigned long stack_bytes)
{
    uintptr_t first=(uintptr_t)live->stack_data, end=first+stack_bytes;
    uintptr_t cursor=(uintptr_t)thread->stack;
    unsigned long steps=0;
    if (thread->type>=NUMBER_OF_HS_THREAD_TYPES || (thread->flags & ~7u) ||
        thread->sleep_until<NONE-1) return 0;
    for (;;) {
        struct hs_stack_frame const *frame;
        uintptr_t previous,result;
        if (cursor<first || cursor>end-sizeof(*frame) || ++steps>stack_bytes/sizeof(*frame)) return 0;
        frame=(struct hs_stack_frame const *)(thread->stack_data+(cursor-first));
        if (frame->size<0 || (unsigned long)frame->size>end-cursor-offsetof(struct hs_stack_frame,data)) return 0;
        if (cursor==first) return frame->previous==NULL;
        previous=(uintptr_t)frame->previous;
        if (previous<first || previous>cursor-sizeof(*frame)) return 0;
        frame=(struct hs_stack_frame const *)(thread->stack_data+(previous-first));
        if (frame->size<0 || previous+sizeof(*frame)+(unsigned long)frame->size!=cursor) return 0;
        result=(uintptr_t)frame->result;
        if (result!=(uintptr_t)&live->result &&
            (frame->size<sizeof(long) || result<previous+offsetof(struct hs_stack_frame,data) ||
             result>previous+offsetof(struct hs_stack_frame,data)+frame->size-sizeof(long))) return 0;
        cursor=previous;
    }
}

static int mcc_threads_array_valid(struct data_array const *array,
    struct data_array const *live, unsigned long stride)
{
    short i,actual=0;
    if (array->signature!='d@t@' || array->maximum_count!=MCC_SCRIPT_THREAD_COUNT ||
        array->size!=stride || array->valid!=1 || array->identifier_zero_invalid>1 ||
        !array->next_identifier || !memchr(array->name,0,sizeof(array->name)) ||
        array->data!=live+1 || array->count<0 || array->count>MCC_SCRIPT_THREAD_COUNT ||
        array->actual_count<0 || array->actual_count>array->count ||
        array->first_free_absolute_index<0 || array->first_free_absolute_index>MCC_SCRIPT_THREAD_COUNT) return 0;
    for (i=0;i<MCC_SCRIPT_THREAD_COUNT;i++) {
        struct hs_thread_datum const *thread=(void const *)((byte const *)(array+1)+i*stride);
        struct hs_thread_datum const *target=(void const *)((byte const *)(live+1)+i*stride);
        if (!thread->identifier) continue;
        if (i>=array->count) return 0;
        actual++;
        if (!mcc_threads_thread_valid(thread,target,stride-offsetof(struct hs_thread_datum,stack_data))) return 0;
    }
    return actual==array->actual_count;
}

static void mcc_threads_stamp(byte *storage)
{
    unsigned long *header=(unsigned long *)storage;
    header[0]=0x5453434Du; /* MCST */
    header[1]=1;
    header[2]=MCC_SCRIPT_THREAD_STORAGE;
    header[3]=mcc_threads_checksum(storage+16,MCC_SCRIPT_THREAD_STORAGE-16);
}

int mcc_script_threads_capture(void)
{
    if (!mcc_threads.array || hs_thread_data!=mcc_threads.array ||
        !mcc_threads_array_valid(mcc_threads.array,mcc_threads.array,MCC_SCRIPT_THREAD_BYTES)) return 0;
    mcc_threads_stamp(mcc_threads.storage);
    return 1;
}

int mcc_script_threads_validate(void const *image, void const *current,
    unsigned long used, unsigned long capacity)
{
    byte *storage=mcc_checkpoint_script_storage((void *)image,used,capacity,MCC_SCRIPT_THREAD_STORAGE);
    unsigned long const *header=(unsigned long const *)storage;
    struct data_array *array;
    uintptr_t native_offset;
    if (!storage || !mcc_threads.array ||
        mcc_checkpoint_script_storage((void *)current,used,capacity,MCC_SCRIPT_THREAD_STORAGE)!=mcc_threads.storage) return 0;
    array=(struct data_array *)(storage+16);
    if (header[0]==0x5453434Du)
        return header[1]==1 && header[2]==MCC_SCRIPT_THREAD_STORAGE &&
            header[3]==mcc_threads_checksum(storage+16,MCC_SCRIPT_THREAD_STORAGE-16) &&
            mcc_threads_array_valid(array,mcc_threads.array,MCC_SCRIPT_THREAD_BYTES);
    /* Old checkpoints used only the native inline stacks. Validate them first,
     * then copy their live frames and relocate only intra-thread pointers. */
    native_offset=(uintptr_t)mcc_threads.native-(uintptr_t)current;
    if (native_offset>used || sizeof(struct data_array)+MCC_SCRIPT_THREAD_COUNT*sizeof(struct hs_thread_datum)>used-native_offset) return 0;
    {
        struct data_array const *native=(void const *)((byte const *)image+native_offset);
        if (!mcc_threads_array_valid(native,mcc_threads.native,sizeof(struct hs_thread_datum))) return 0;
        return 1;
    }
}

int mcc_script_threads_restore(void *image, unsigned long used, unsigned long capacity)
{
    byte *storage=mcc_checkpoint_script_storage(image,used,capacity,MCC_SCRIPT_THREAD_STORAGE);
    struct data_array *array;
    struct data_array const *native=mcc_threads.native;
    short i;
    if (!mcc_script_threads_validate(image,image,used,capacity)) return 0;
    array=(void *)(storage+16);
    if (*(unsigned long *)storage==0x5453434Du) return 1;
    {
        memset(storage,0,MCC_SCRIPT_THREAD_STORAGE);
        memcpy(array,native,sizeof(*array));
        array->size=MCC_SCRIPT_THREAD_BYTES;
        array->data=mcc_threads.array+1;
        for (i=0;i<native->count;i++) {
            struct hs_thread_datum const *old=(void const *)((byte const *)(native+1)+i*sizeof(*old));
            struct hs_thread_datum const *old_live=(void const *)((byte const *)(mcc_threads.native+1)+i*sizeof(*old));
            struct hs_thread_datum *new_thread=(void *)((byte *)(array+1)+i*MCC_SCRIPT_THREAD_BYTES);
            struct hs_thread_datum *new_live=(void *)((byte *)(mcc_threads.array+1)+i*MCC_SCRIPT_THREAD_BYTES);
            uintptr_t offset,cursor,delta=(uintptr_t)new_live-(uintptr_t)old_live;
            memcpy(new_thread,old,sizeof(*old));
            if (!old->identifier) continue;
            cursor=(uintptr_t)old->stack;
            new_thread->stack=(void *)(cursor+delta);
            while (cursor!=(uintptr_t)old_live->stack_data) {
                struct hs_stack_frame *frame;
                offset=cursor-(uintptr_t)old_live;
                frame=(void *)((byte *)new_thread+offset);
                cursor=(uintptr_t)frame->previous;
                frame->previous=(void *)(cursor+delta);
                frame=(void *)((byte *)new_thread+(cursor-(uintptr_t)old_live));
                frame->result=(void *)((uintptr_t)frame->result+delta);
            }
        }
    }
    mcc_threads_stamp(storage);
    return mcc_threads_array_valid(array,mcc_threads.array,MCC_SCRIPT_THREAD_BYTES);
}
