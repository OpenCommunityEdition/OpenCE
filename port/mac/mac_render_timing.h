#ifndef MAC_RENDER_TIMING_H
#define MAC_RENDER_TIMING_H

/* Optional renderer cost accounting. Call init once at device start, then
   bracket named renderer phases. With HALO_MAC_RENDER_TIMING unset, begin
   returns zero and no wall-clock reads are made. */
enum mac_render_timing_bucket {
    mac_render_timing_texture_bind = 0,
    mac_render_timing_shader_sources,
    mac_render_timing_program_lookup,
    mac_render_timing_draw_submit_restore,
    mac_render_timing_present_readback,
    mac_render_timing_visibility_results,
    mac_render_timing_draw_state_setup,
    mac_render_timing_draw_uniforms,
    mac_render_timing_draw_call,
    mac_render_timing_draw_restore,
    mac_render_timing_bone_gather,
    mac_render_timing_stream_decode,
    mac_render_timing_bucket_count
};

typedef unsigned long long mac_render_timing_stamp;

void mac_render_timing_init(void);
int mac_render_timing_enabled(void);
mac_render_timing_stamp mac_render_timing_begin(
    enum mac_render_timing_bucket bucket);
void mac_render_timing_end(enum mac_render_timing_bucket bucket,
    mac_render_timing_stamp started_at);
/* Attribute CPU draw submission time to the bounded retail shader ID.
   This does not insert a GPU synchronization or measure GPU execution time. */
void mac_render_timing_draw_end(unsigned long vertex_id,
    mac_render_timing_stamp started_at);
void mac_render_timing_get(enum mac_render_timing_bucket bucket,
    mac_render_timing_stamp *microseconds,unsigned long *calls);
void mac_render_timing_report(void);

#endif
