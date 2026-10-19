#include "mac_render_timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct mac_render_timing_total {
    mac_render_timing_stamp microseconds;
    unsigned long calls;
};

static int timing_enabled;
static int timing_detail;
static struct mac_render_timing_total timing_totals[mac_render_timing_bucket_count];
#define MAC_RENDER_TIMING_VERTEX_IDS 136
static struct mac_render_timing_total vertex_totals[MAC_RENDER_TIMING_VERTEX_IDS];

static const char *const timing_names[mac_render_timing_bucket_count] = {
    "texture_bind",
    "shader_sources",
    "program_lookup",
    "draw_submit_restore",
    "present_readback",
    "visibility_results",
    "draw_state_setup",
    "draw_uniforms",
    "draw_call",
    "draw_restore",
    "bone_gather",
    "stream_decode",
    "dc_attributes",
    "dc_vertex_program",
    "dc_fragment_program",
    "dc_gl_draw"
};

static int timing_bucket_valid(enum mac_render_timing_bucket bucket)
{
    return (unsigned)bucket < (unsigned)mac_render_timing_bucket_count;
}

static mac_render_timing_stamp timing_now(void)
{
    struct timeval now;
    if(gettimeofday(&now,NULL)!=0)return 0;
    return (mac_render_timing_stamp)now.tv_sec*1000000ULL+
        (mac_render_timing_stamp)now.tv_usec;
}

void mac_render_timing_init(void)
{
    const char *enabled;
    memset(timing_totals,0,sizeof(timing_totals));
    memset(vertex_totals,0,sizeof(vertex_totals));
    enabled=getenv("HALO_MAC_RENDER_TIMING");
    timing_enabled=enabled&&(enabled[0]=='1'||enabled[0]=='2')&&enabled[1]=='\0';
    /* port: "2" also times the parts of a draw call (the dc_ buckets); "1" leaves them out so that it costs what it did */
    timing_detail=timing_enabled&&enabled[0]=='2';
}

int mac_render_timing_enabled(void)
{
    return timing_enabled;
}

mac_render_timing_stamp mac_render_timing_begin(
    enum mac_render_timing_bucket bucket)
{
    if(!timing_enabled||!timing_bucket_valid(bucket))return 0;
    if(bucket>=mac_render_timing_dc_attributes&&!timing_detail)return 0;
    return timing_now();
}

static void timing_accumulate(struct mac_render_timing_total *total,
    mac_render_timing_stamp elapsed)
{
    if((mac_render_timing_stamp)-1-total->microseconds<elapsed)
        total->microseconds=(mac_render_timing_stamp)-1;
    else
        total->microseconds+=elapsed;
    if(total->calls!=(unsigned long)-1)++total->calls;
}

void mac_render_timing_end(enum mac_render_timing_bucket bucket,
    mac_render_timing_stamp started_at)
{
    mac_render_timing_stamp ended_at;
    if(!timing_enabled||!started_at||!timing_bucket_valid(bucket))return;
    ended_at=timing_now();
    if(!ended_at||ended_at<started_at)return;
    timing_accumulate(&timing_totals[bucket],ended_at-started_at);
}

void mac_render_timing_draw_end(unsigned long vertex_id,
    mac_render_timing_stamp started_at)
{
    mac_render_timing_stamp ended_at,elapsed;
    if(!timing_enabled||!started_at)return;
    ended_at=timing_now();
    if(!ended_at||ended_at<started_at)return;
    elapsed=ended_at-started_at;
    timing_accumulate(&timing_totals[mac_render_timing_draw_call],elapsed);
    if(vertex_id<MAC_RENDER_TIMING_VERTEX_IDS)
        timing_accumulate(&vertex_totals[vertex_id],elapsed);
}

void mac_render_timing_get(enum mac_render_timing_bucket bucket,
    mac_render_timing_stamp *microseconds,unsigned long *calls)
{
    if(microseconds)*microseconds=0;
    if(calls)*calls=0;
    if(!timing_bucket_valid(bucket))return;
    if(microseconds)*microseconds=timing_totals[bucket].microseconds;
    if(calls)*calls=timing_totals[bucket].calls;
}

void mac_render_timing_report(void)
{
    unsigned i;
    if(!timing_enabled)return;
    for(i=0;i<(unsigned)mac_render_timing_bucket_count;++i) {
        fprintf(stderr,"mac render timing %s: %llu us, %lu calls\n",
            timing_names[i],
            (unsigned long long)timing_totals[i].microseconds,
            timing_totals[i].calls);
    }
    for(i=0;i<MAC_RENDER_TIMING_VERTEX_IDS;++i)if(vertex_totals[i].calls)
        fprintf(stderr,"mac render CPU draw submission vertex %u: %llu us, %lu calls\n",
            i,(unsigned long long)vertex_totals[i].microseconds,vertex_totals[i].calls);
}
