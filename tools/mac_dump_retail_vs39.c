#include "cseries.h"
#include "../source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.h"
#include "../port/mac/mac_nv2a_shader_api.h"
#undef malloc
#undef free
#undef memset
#undef memcpy
#undef memcmp
#undef strcmp
#undef strlen
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv)
{
    unsigned long index=39;
    const struct vertex_shader_entry *entry;
    if(argc>1)index=strtoul(argv[1],NULL,10);
    if(index>=NUMBER_OF_VERTEX_SHADERS)return 2;
    entry=&vertex_shader_table[index];
    const DWORD *function=(const DWORD *)entry->code;
    unsigned long instruction_count=function[0]>>16;
    char *source=nv2a_vertex_shader_to_glsl(function+1,instruction_count,0,NULL);
    if(!source)return 1;
    printf("table-index=%lu instructions=%lu bytes=%ld\n%s",index,instruction_count,entry->instruction_count,source);
    free(source);
    return 0;
}
