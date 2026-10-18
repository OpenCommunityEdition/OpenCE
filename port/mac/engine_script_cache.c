/* Native cache membership uses actual allocations, never raw PC addresses. */
#include "cseries.h"
#include "cache/cache_files.h"
#include "engine_script_cache.h"
#include "engine_cache_files.h"
boolean cache_file_tag_cache_contains_retail_legacy(void const *address,long size);
static const struct mac_native_scenario_scripts *active_scripts;
int mac_engine_script_cache_bind(const struct mac_native_scenario_scripts *scripts)
{
    if(active_scripts || !scripts || !scripts->state)return 0;
    active_scripts=scripts;return 1;
}
int mac_engine_script_cache_unbind(const struct mac_native_scenario_scripts *scripts)
{
    if(!scripts || active_scripts!=scripts)return 0;
    active_scripts=NULL;return 1;
}
boolean cache_file_tag_cache_contains(const void *pointer,long bytes)
{
    if(mac_cache_files_xbox_active())return cache_file_tag_cache_contains_retail_legacy(pointer,bytes);
    return bytes>0 && mac_native_scenario_scripts_contains(active_scripts,pointer,(size_t)bytes);
}
