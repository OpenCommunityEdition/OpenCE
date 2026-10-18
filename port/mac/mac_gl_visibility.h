#ifndef HALO_MAC_GL_VISIBILITY_H
#define HALO_MAC_GL_VISIBILITY_H

#include <stdint.h>

enum mac_gl_visibility_result_code {
    MAC_GL_VISIBILITY_OK=0,
    MAC_GL_VISIBILITY_INVALID=-1,
    MAC_GL_VISIBILITY_UNAVAILABLE=-2,
    MAC_GL_VISIBILITY_GL_ERROR=-3
};

/* Bounded GL_ARB_occlusion_query owner for the engine's 12-bit visibility
   indices. Call on the context-owning thread while that context is current. */
int mac_gl_visibility_start(void);
void mac_gl_visibility_stop(void);
int mac_gl_visibility_begin(void);
int mac_gl_visibility_end(uint32_t index, float samples_per_game_pixel);
int mac_gl_visibility_result(uint32_t index, uint32_t *result, uint64_t *time_stamp);

#ifdef MAC_GL_VISIBILITY_TEST
int mac_gl_visibility_test_query_ids(unsigned int *ids,unsigned capacity);
#endif

#endif
