/* Compile the unchanged NV2A pixel translator with platform-default values
   for its three optional GPU-debug settings (empty/false as port_config.h). */
#include "mac_nv2a_shader_api.h"
#undef strcmp
#undef strlen
#include <string.h>
static const char *mac_shader_config_string(const char *name)
{
    if (!strcmp(name,"debug.gpu_debug_expression")) return "";
    fprintf(stderr,"unsupported native shader setting: %s\n",name); abort(); return "";
}
static int mac_shader_config_boolean(const char *name)
{
    if (!strcmp(name,"debug.gpu_debug_texture0") || !strcmp(name,"debug.gpu_debug_flat")) return 0;
    fprintf(stderr,"unsupported native shader setting: %s\n",name); abort(); return 0;
}
#define config_string mac_shader_config_string
#define config_boolean mac_shader_config_boolean
#include "../linux/src/nv2a_psh.c"
#undef config_string
#undef config_boolean
