#ifndef HALO_MAC_RETAIL_ANIMATION_H
#define HALO_MAC_RETAIL_ANIMATION_H
#include "retail_cache.h"
/* One uncompressed animation, not a native animation graph. Streams contain
   native int16 quaternion components and float words in engine mask order. */
struct mac_retail_animation {
    char name[32];
    int16_t type,frame_count,frame_size,frame_info_type,node_count;
    int16_t loop_frame_index,key_frame_index,second_key_frame_index;
    int16_t next_animation_index,sound_index,sound_frame_index,parent_animation_index;
    uint16_t flags;
    uint8_t left_foot_frame_index,right_foot_frame_index;
    uint32_t node_list_checksum,translation_flags[2],rotation_flags[2],scale_flags[2];
    float weight,normalized_weight;
    uint32_t frame_info_size,default_size,data_size;
    unsigned char *frame_info,*defaults,*data;
};
int mac_retail_animation_count(const struct mac_retail_cache *,uint32_t graph_handle,
    uint32_t *count,const char **error);
/* On failure output is empty. Free before reuse. Source remains unchanged. */
int mac_retail_animation_decode(const struct mac_retail_cache *,uint32_t graph_handle,
    uint32_t animation_index,struct mac_retail_animation *,const char **error);
void mac_retail_animation_free(struct mac_retail_animation *);
#endif
