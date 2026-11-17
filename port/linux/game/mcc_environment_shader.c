#include "cseries.h"
#include "mcc_cache.h"
#include "mcc_environment_shader.h"

void mcc_environment_shader_begin(unsigned short flags)
{
    /* 0x8 is the MCC environment tag's alternate bump attenuation flag.
     * A specular-mask-only texture does not carry a surface normal. */
    mcc_environment_shader_set(mcc_cache_tags_loaded() && (flags&8) && !(flags&2));
}

void mcc_environment_shader_end(void)
{
    mcc_environment_shader_set(0);
}
