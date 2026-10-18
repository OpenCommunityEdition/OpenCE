#include <stdio.h>
#include <stdlib.h>
#include "engine_device_state.h"
#include "engine_physical_memory.h"
#include "native_resource_memory.h"
#include "engine_cache_resources.h"

#undef fprintf
#undef printf

void mac_engine_cache_resource_validate(D3DResource *resource,
    int registered_data)
{
    unsigned long type;
    /* port: the hidden-GPU and window runs register an Xbox map's buffers too */
    if (!mac_device_headless_identity() && !mac_device_gpu_identity()) {
        fprintf(stderr, "cache resource operation requires an active native device\n");
        abort();
    }
    if (!resource || !mac_engine_physical_memory_contains(resource, sizeof(*resource))) {
        fprintf(stderr, "cache resource descriptor is outside live physical allocation\n");
        abort();
    }
    type = resource->Common & D3DCOMMON_TYPE_MASK;
    if ((type != D3DCOMMON_TYPE_VERTEXBUFFER && type != D3DCOMMON_TYPE_INDEXBUFFER) ||
        (resource->Common & D3DCOMMON_D3DCREATED)) {
        fprintf(stderr, "cache resource is not a CPU-owned vertex or index buffer\n");
        abort();
    }
    if (resource->Data) {
        void *data = registered_data ?
            PLATFORM_PHYSICAL_TO_VIRTUAL(resource->Data) :
            (void *)resource->Data;
        if (!mac_engine_physical_memory_contains(data, 1)) {
            fprintf(stderr, "cache resource data is outside live physical allocation\n");
            abort();
        }
    }
}
