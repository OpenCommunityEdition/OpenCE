/* Actual datum and relocating pool functions, using engine object header and
   biped/weapon sizes. Allocation is a constructor prerequisite, not spawning. */
#include "cseries.h"
#include "memory/data.h"
#include "memory/memory_pool.h"
#include "objects/objects.h"
#include "units/bipeds.h"
#include "items/weapons.h"
#include <stdio.h>
static unsigned checks,failures;
#define CHECK(x) do { ++checks;if(!(x)){++failures;fprintf(stderr,"FAIL pool line %d\n",__LINE__);} } while(0)
static void pattern(void *p,unsigned n,unsigned char value)
{ unsigned i;for(i=0;i<n;++i)((unsigned char *)p)[i]=value; }
static void verify(void *p,unsigned n,unsigned char value)
{ unsigned i;int ok=1;for(i=0;i<n;++i)if(((unsigned char *)p)[i]!=value)ok=0;CHECK(ok); }
int main(void)
{
    struct data_array *headers;
    struct memory_pool *pool;
    struct object_header_datum *h[3];
    long handles[3],i,size,free_start;
    void *before,*failed=NULL;
    headers=(struct data_array *)malloc(data_allocation_size(4,sizeof(*h[0])));
    pool=memory_pool_new("native object pool",16384);
    if(!headers || !pool)return 2;
    data_initialize(headers,"objects",4,sizeof(*h[0]));data_make_valid(headers);
    free_start=memory_pool_get_free_size(pool);
    for(i=0;i<3;++i) {
        handles[i]=datum_new(headers);CHECK(handles[i]!=NONE);
        h[i]=(struct object_header_datum *)datum_get(headers,handles[i]);
        size=i==0?sizeof(struct biped_datum):sizeof(struct weapon_datum);
        CHECK(memory_pool_block_allocate(pool,(void **)&h[i]->datum,size));
        h[i]->data_size=(short)size;
        pattern(h[i]->datum,size,(unsigned char)(0x41+i));
    }
    before=h[2]->datum;
    memory_pool_block_free(pool,(void **)&h[1]->datum);datum_delete(headers,handles[1]);
    memory_pool_compact(pool);CHECK(h[2]->datum!=before);
    verify(h[0]->datum,h[0]->data_size,0x41);verify(h[2]->datum,h[2]->data_size,0x43);
    CHECK(datum_try_and_get(headers,handles[1])==NULL);
    before=h[0]->datum;
    CHECK(memory_pool_block_reallocate(pool,(void **)&h[0]->datum,h[0]->data_size+19*52));
    CHECK(h[0]->datum!=before);verify(h[0]->datum,h[0]->data_size,0x41);
    pattern((byte *)h[0]->datum+h[0]->data_size,19*52,0x7c);
    verify(h[2]->datum,h[2]->data_size,0x43);
    CHECK(!memory_pool_block_allocate(pool,&failed,32768));CHECK(!failed);
    verify(h[0]->datum,h[0]->data_size,0x41);
    verify((byte *)h[0]->datum+h[0]->data_size,19*52,0x7c);
    memory_pool_block_free(pool,(void **)&h[0]->datum);datum_delete(headers,handles[0]);
    memory_pool_block_free(pool,(void **)&h[2]->datum);datum_delete(headers,handles[2]);
    CHECK(memory_pool_get_free_size(pool)==free_start);CHECK(headers->actual_count==0);
    printf("actual object allocation: biped=%lu weapon=%lu header=%lu checks=%u failures=%u; no constructor or spawn\n",
        (unsigned long)sizeof(struct biped_datum),(unsigned long)sizeof(struct weapon_datum),(unsigned long)sizeof(*h[0]),checks,failures);
    memory_pool_delete(pool);free(headers);return failures?1:0;
}
