#ifndef HALO_MAC_ENGINE_CACHE_RESOURCES_H
#define HALO_MAC_ENGINE_CACHE_RESOURCES_H

/* Validate an original cache vertex/index descriptor before the retail cache
   registration and busy-query bodies touch it. registered_data is true only
   for vertex buffers after D3DResource_Register converted Data to an offset. */
void mac_engine_cache_resource_validate(D3DResource *resource,
    int registered_data);

#endif
