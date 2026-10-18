#ifndef HALO_MAC_NATIVE_MODEL_BUFFERS_H
#define HALO_MAC_NATIVE_MODEL_BUFFERS_H
#include "native_model_tags.h"
/* Genuine CPU-backed Xbox resource records for native headless loading.
   Vertex storage uses non-clobbering Mach allocation in the Xbox contiguous
   window; index storage is native memory. GPU upload/drawing is not provided.
   The callbacks own buffers until model destruction; context is unused. */
void mac_native_model_buffer_callbacks(struct mac_native_model_buffers *);
/* The model index copy holding [p, p+bytes): 1 with its serial (never
   reused), p's offset in it, its base and its size. */
int mac_native_model_index_span(const void *p,unsigned long bytes,unsigned long *serial,
    unsigned long *offset,const void **base,unsigned long *total);
/* An Xbox map's models: their parts' vertex and index buffers are the map's own, in
   the tag cache (registered by cache_files.c). Certify each (an owned range and a
   certificate for the vertices, an index span for the indices), as create does for
   the native v7 session's, so the draw path keeps decoded copies of them. Call
   once the map's tags are loaded and before they are let go. */
void mac_native_model_buffers_certify_xbox(void);
void mac_native_model_buffers_release_xbox(void);
#endif
