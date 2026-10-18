/* Diagnostic access to TU-private allocation services. Compile this whole
   engine source once; dead stripping removes unrelated gameplay functions.
   No rewritten allocator or fake object lookup replaces the engine code. */
#include "../source/objects/objects.c"
#include "items/weapons.h"
#undef malloc
#undef free
int mac_constructor_storage_start(void)
{
    long size;
    if(object_header_data || object_memory_pool)return 0;
    size=data_allocation_size(4,sizeof(struct object_header_datum));
    object_header_data=(struct data_array *)malloc((size_t)size);
    if(!object_header_data)return 0;
    data_initialize(object_header_data,"objects",4,sizeof(struct object_header_datum));data_make_valid(object_header_data);
    object_memory_pool=memory_pool_new("objects",65536);
    if(!object_memory_pool){free(object_header_data);object_header_data=NULL;return 0;}return 1;
}
long mac_constructor_storage_new(long definition)
{
    long index=object_header_new(object_header_data,NONE,sizeof(struct weapon_datum));
    if(index!=NONE){struct object_header_datum*h=object_header_get(index);struct weapon_datum*w=(struct weapon_datum*)h->datum;
        h->type=_object_type_weapon;w->definition_index=definition;w->object.type=_object_type_weapon;}
    return index;
}
void mac_constructor_storage_delete(long index){object_header_delete(object_header_data,index);}
void mac_constructor_storage_compact(void){memory_pool_compact(object_memory_pool);}
long mac_constructor_storage_used(void){return memory_pool_get_used_size(object_memory_pool);}
int mac_constructor_storage_stop(void)
{
    int clean=object_header_data && object_header_data->actual_count==0 && memory_pool_get_used_size(object_memory_pool)==0;
    if(object_memory_pool)memory_pool_delete(object_memory_pool);object_memory_pool=NULL;
    free(object_header_data);object_header_data=NULL;return clean;
}
