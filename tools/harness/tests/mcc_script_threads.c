#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib_prefixed.h"
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
#define TAG_STRING_LENGTH 31
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define TEST_FLAG(v,b) (((v) & (1u << (b))) != 0)
#define match_hs_assert(a,b,c,d,e) ((void)0)
typedef unsigned char byte,boolean;
#include "types.inc"
static byte image[2*1024*1024], saved[sizeof(image)];
static unsigned long used=160000, capacity=sizeof(image);
static int loaded, lighting_resets;
static void mcc_campaign_reset_model_lighting(void) {++lighting_resets;}
static struct data_array *hs_thread_data;
static struct hs_thread_datum *hs_thread_get(long index) {(void)index;return hs_thread_data->data;}
static void hs_stack_overflow(long index) {hs_thread_get(index)->flags|=1u<<_hs_thread_stack_overflow_bit;}
static int mcc_cache_tags_loaded(void) {return loaded;}
static void *game_state_port_cpu_region(unsigned long *u,unsigned long *c) {*u=used;*c=capacity;return image;}
static void data_initialize(struct data_array *a,char const *name,short count,short stride)
{
    memset(a,0,sizeof(*a));memcpy(a->name,name,strlen(name)+1);a->maximum_count=count;
    a->size=stride;a->signature='d@t@';a->data=a+1;
}
static void data_make_invalid(struct data_array *a) {a->valid=0;}
#include "under_test.inc"

/* A real stack shape: root result, a large parent and an odd-aligned leaf.
 * Frame offsets follow the native push operation, including its padding. */
static struct hs_thread_datum *populate(struct data_array *a,unsigned size)
{
    struct hs_thread_datum *t=(void *)(a+1);
    struct hs_stack_frame *root=(void *)t->stack_data;
    struct hs_stack_frame *parent=(void *)(t->stack_data+20);
    struct hs_stack_frame *leaf=(void *)(t->stack_data+36+size);
    a->valid=1;a->next_identifier=2;a->count=a->actual_count=1;
    t->identifier=1;t->sleep_until=123;t->script_index=0;
    root->size=4;root->previous=NULL;root->result=root->data;
    parent->previous=root;parent->size=(short)size;parent->result=parent->data;
    leaf->previous=parent;leaf->size=4;t->stack=leaf;
    memcpy(leaf->data,"test",4);
    return t;
}

int main(int argc,char **argv)
{
    struct data_array *native=(void *)(image+1024),*incoming;
    struct hs_thread_datum *t,*copy;
    struct hs_stack_frame *root;
    byte *storage,*saved_storage;
    unsigned long offset;
    CHECK((argc==2 || argc==3) && sizeof(long)==4 && sizeof(void *)==4);
    if (!strcmp(argv[1],"crc")) {
        unsigned long i,bytes;
        unsigned char *data;
        CHECK(argc==3);
        if (!strcmp(argv[2],"null")) {printf("%08lx\n",mcc_threads_checksum(NULL,0));return 0;}
        if (!strcmp(argv[2],"known")) {printf("%08lx\n",mcc_threads_checksum((byte const *)"123456789",9));return 0;}
        bytes=strtoul(argv[2],NULL,10);CHECK(bytes<=MCC_SCRIPT_THREAD_STORAGE);
        data=malloc(bytes?bytes:1);CHECK(data);
        for(i=0;i<bytes;i++)data[i]=(unsigned char)(i*37+(i>>8)+19);
        printf("%08lx\n",mcc_threads_checksum(data,bytes));free(data);return 0;
    }
    memset(image,0xA5,sizeof(image));memset(native,0,140000);
    data_initialize(native,"hs thread",256,sizeof(struct hs_thread_datum));
    hs_thread_data=native;
    t=populate(native,65);
    CHECK(mcc_hs_stack_end(t)==t->stack_data+512);
    if (!strcmp(argv[1],"bounds")) {
        CHECK(!hs_stack_allocate(0,1024));CHECK(TEST_FLAG(t->flags,_hs_thread_stack_overflow_bit));
        t->flags=0;
    }
    if (!strcmp(argv[1],"capacity")) {
        used=capacity;CHECK(!mcc_script_threads_prepare());
        CHECK(!mcc_threads.original && hs_thread_data==native);
        CHECK(!mcc_checkpoint_script_storage(image,0,8,1));
        CHECK(!mcc_checkpoint_script_storage(image,0,capacity,0xFFFFFFFFu));return 0;
    }
    CHECK(mcc_script_threads_prepare());CHECK(!mcc_script_threads_prepare());
    storage=mcc_threads.storage;offset=(unsigned long)(storage-image);
    CHECK(storage>=image+used && storage+MCC_SCRIPT_THREAD_STORAGE<=image+capacity);
    if (!strcmp(argv[1],"unactivated")) {
        mcc_script_threads_dispose();CHECK(native->valid && hs_thread_data==native);
        CHECK(storage[0]==0xA5 && storage[MCC_SCRIPT_THREAD_STORAGE-1]==0xA5);return 0;
    }
    mcc_hs_threads_activate();CHECK(hs_thread_data==native && !lighting_resets);
    if (!strcmp(argv[1],"migration")) memcpy(saved,image,sizeof(image));
    loaded=1;mcc_hs_threads_activate();CHECK(hs_thread_data==mcc_threads.array && lighting_resets==1);
    if (!strcmp(argv[1],"bounds")) {
        t=populate(hs_thread_data,1025);
        CHECK(hs_stack_allocate(0,1024));CHECK(hs_stack_push(0));
        CHECK(!hs_stack_allocate(0,4096));CHECK(TEST_FLAG(t->flags,_hs_thread_stack_overflow_bit));
        CHECK(!hs_stack_push(0));mcc_script_threads_dispose();return 0;
    }
    if (!strcmp(argv[1],"migration")) {
        CHECK(mcc_script_threads_validate(saved,image,used,capacity));
        memcpy(image,saved,sizeof(image));
        CHECK(mcc_script_threads_restore(image,used,capacity));
        t=(void *)(hs_thread_data+1);
        CHECK(t->stack==(void *)(t->stack_data+101) && !memcmp(t->stack->data,"test",4));
        CHECK(t->stack->previous==(void *)(t->stack_data+20));
        CHECK(t->stack->previous->result==t->stack->previous->data);
        CHECK(mcc_script_threads_capture());
    } else {
        t=populate(hs_thread_data,1025);
        CHECK(mcc_hs_stack_end(t)==t->stack_data+4096);
        CHECK(mcc_script_threads_capture());
        memcpy(saved,image,sizeof(image));saved_storage=saved+offset;
        if (!strcmp(argv[1],"snapshot")) {
            FILE *output;
            CHECK(argc==3);
            output=fopen(argv[2],"wb");CHECK(output);
            CHECK(fwrite(saved,1,sizeof(saved),output)==sizeof(saved));CHECK(!fclose(output));
            mcc_script_threads_dispose();return 0;
        }
        incoming=(void *)(saved_storage+16);copy=(void *)(incoming+1);
        root=(void *)copy->stack_data;
        CHECK(mcc_script_threads_validate(saved,image,used,capacity));
        if (!strcmp(argv[1],"checksum")) saved_storage[16]^=1;
        else if (!strcmp(argv[1],"missing")) saved_storage[0]=0;
        else if (!strcmp(argv[1],"flags")) copy->flags=0x80;
        else if (!strcmp(argv[1],"type")) copy->type=255;
        else if (!strcmp(argv[1],"sleep")) copy->sleep_until=-3;
        else if (!strcmp(argv[1],"unused_slot")) ((struct hs_thread_datum *)((byte *)copy+MCC_SCRIPT_THREAD_BYTES))->identifier=4;
        else if (!strcmp(argv[1],"stride")) incoming->size=512;
        else if (!strcmp(argv[1],"count")) incoming->actual_count=2;
        else if (!strcmp(argv[1],"pointer")) incoming->data=incoming+1;
        else if (!strcmp(argv[1],"stack")) copy->stack=(void *)(t->stack_data+4090);
        else if (!strcmp(argv[1],"cycle")) ((struct hs_stack_frame *)(copy->stack_data+1061))->previous=t->stack;
        else if (!strcmp(argv[1],"overlap")) root->size=5;
        else if (!strcmp(argv[1],"result")) root->result=&t->stack;
        else if (!strcmp(argv[1],"size")) ((struct hs_stack_frame *)(copy->stack_data+1061))->size=4096;
        else if (!strcmp(argv[1],"root")) root->previous=(void *)t->stack_data;
        else if (!strcmp(argv[1],"other_thread")) copy->stack=(void *)((byte *)t->stack+MCC_SCRIPT_THREAD_BYTES);
        else if (strcmp(argv[1],"lifecycle") && strcmp(argv[1],"roundtrip")) return 2;
        if (strcmp(argv[1],"lifecycle") && strcmp(argv[1],"roundtrip")) {
            if (strcmp(argv[1],"checksum") && strcmp(argv[1],"missing")) mcc_threads_stamp(saved_storage);
            CHECK(!mcc_script_threads_validate(saved,image,used,capacity));
        } else {
            memset(t->stack->data,0,4);memcpy(image,saved,sizeof(image));
            CHECK(mcc_script_threads_restore(image,used,capacity));
            CHECK(!memcmp(t->stack->data,"test",4));
        }
    }
    mcc_script_threads_dispose();CHECK(hs_thread_data==native && !mcc_threads.array);
    CHECK(!native->valid && mcc_hs_stack_end((void *)(native+1))==(byte *)(native+1)+sizeof(struct hs_thread_datum));
    CHECK(storage[0]==0xA5 && storage[MCC_SCRIPT_THREAD_STORAGE-1]==0xA5);
    mcc_script_threads_dispose();return 0;
}
