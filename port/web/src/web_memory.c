/* The ordinary heap ends below CE's linked tags. MCC's accepted windows and
 * the Xbox arena lie above it, inside the already allocated Wasm memory. */
#include "web_memory.h"
#include "port_config.h"

static void *custom_edition_tags;

size_t web_memory_bytes(void)
{
    return (size_t)__builtin_wasm_memory_size(0) << 16;
}

size_t emscripten_get_heap_size(void)
{
    size_t bytes = web_memory_bytes();
    return bytes > WEB_HEAP_LIMIT ? WEB_HEAP_LIMIT : bytes;
}

void web_memory_initialize(void)
{
    /* The page's arguments and OPFS/configuration paths must be in place
     * before reading settings; native constructors run too early here. */
    custom_edition_tags = NULL;
    if (config_boolean("game.custom_edition") &&
        web_memory_bytes() >= CUSTOM_EDITION_TAG_CACHE_ADDRESS + CUSTOM_EDITION_TAG_CACHE_BYTES)
        custom_edition_tags = (void *)CUSTOM_EDITION_TAG_CACHE_ADDRESS;
}

void *web_custom_edition_tag_cache(void)
{
    return custom_edition_tags;
}
