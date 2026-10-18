#include "cseries.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "engine_animation.h"
#undef memset
#undef memcpy
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define WITNESS(name, expression) typedef char name[(expression) ? 1 : -1]
WITNESS(animation_size, sizeof(struct animation) == 0xb4);
WITNESS(animation_type, offsetof(struct animation, type) == 0x20);
WITNESS(animation_count, offsetof(struct animation, frame_count) == 0x22);
WITNESS(animation_frame_size, offsetof(struct animation, frame_size) == 0x24);
WITNESS(animation_checksum, offsetof(struct animation, node_list_checksum) == 0x28);
WITNESS(animation_nodes, offsetof(struct animation, node_count) == 0x2c);
WITNESS(animation_flags, offsetof(struct animation, flags) == 0x3a);
WITNESS(animation_frame_info, offsetof(struct animation, frame_info) == 0x48);
WITNESS(animation_translation_flags, offsetof(struct animation, nodes_with_translation_flags) == 0x5c);
WITNESS(animation_rotation_flags, offsetof(struct animation, nodes_with_rotation_flags) == 0x6c);
WITNESS(animation_scale_flags, offsetof(struct animation, nodes_with_scale_flags) == 0x7c);
WITNESS(animation_compressed_offset, offsetof(struct animation, compressed_data_offset) == 0x88);
WITNESS(animation_defaults, offsetof(struct animation, default_data) == 0x8c);
WITNESS(animation_data, offsetof(struct animation, data) == 0xa0);
WITNESS(model_node_size, sizeof(struct model_node) == 0x9c);
WITNESS(model_node_translation, offsetof(struct model_node, default_translation) == 0x28);
WITNESS(model_node_rotation, offsetof(struct model_node, default_rotation) == 0x34);
WITNESS(model_node_inverse, offsetof(struct model_node, runtime_default_inverse_matrix) == 0x68);
WITNESS(model_nodes, offsetof(struct model, nodes) == 0xb8);

int mac_engine_animation_sample(const struct mac_retail_animation *decoded,
    const struct mac_retail_skeleton *skeleton,
    uint32_t frame, struct mac_engine_orientation *out, uint32_t capacity,
    const char **error)
{
    struct animation animation;
    struct model model;
    struct model_node model_nodes[MAXIMUM_NODES_PER_ANIMATION];
    long empty_stream = 0;
    struct real_orientation orientations[MAXIMUM_NODES_PER_ANIMATION];
    unsigned i, frame_bytes = 0, default_bytes = 0;
    if (error) *error = NULL;
    if (!decoded || !skeleton || !out || decoded->type != _animation_base ||
        decoded->flags & FLAG(_animation_compressed_bit) ||
        decoded->node_count < 1 || decoded->node_count > MAXIMUM_NODES_PER_ANIMATION ||
        capacity < (unsigned)decoded->node_count || decoded->frame_count < 1 || decoded->frame_count > MAXIMUM_FRAMES_PER_ANIMATION ||
        frame >= (unsigned)decoded->frame_count) {
        if (error) *error = "engine sampler requires an in-range native base clip";
        return 0;
    }
    if (skeleton->count != (unsigned)decoded->node_count ||
        (skeleton->checksum && decoded->node_list_checksum &&
         skeleton->checksum != decoded->node_list_checksum)) {
        if (error) *error = "engine animation/model node count or checksum mismatch";
        return 0;
    }
    for (i = 0; i < (unsigned)decoded->node_count; ++i) {
        unsigned mask = 1U << (i & 31), bank = i >> 5;
        frame_bytes += (decoded->rotation_flags[bank] & mask) ? 8 : 0;
        frame_bytes += (decoded->translation_flags[bank] & mask) ? 12 : 0;
        frame_bytes += (decoded->scale_flags[bank] & mask) ? 4 : 0;
        default_bytes += (decoded->rotation_flags[bank] & mask) ? 0 : 8;
        default_bytes += (decoded->translation_flags[bank] & mask) ? 0 : 12;
        default_bytes += (decoded->scale_flags[bank] & mask) ? 0 : 4;
    }
    if (decoded->frame_size != (int)frame_bytes || decoded->default_size != default_bytes ||
        decoded->data_size != frame_bytes * (unsigned)decoded->frame_count ||
        (default_bytes && !decoded->defaults) || (decoded->data_size && !decoded->data)) {
        if (error) *error = "engine sampler native streams disagree with node masks";
        return 0;
    }
    memset(&animation, 0, sizeof(animation));
    memcpy(animation.name, decoded->name, sizeof(animation.name));
    animation.type = decoded->type;
    animation.frame_count = decoded->frame_count;
    animation.frame_size = decoded->frame_size;
    animation.frame_info_type = decoded->frame_info_type;
    animation.node_list_checksum = decoded->node_list_checksum;
    animation.node_count = decoded->node_count;
    animation.private_loop_frame_index = decoded->loop_frame_index;
    animation.weight = decoded->weight;
    animation.private_key_frame_index = decoded->key_frame_index;
    animation.private_second_key_frame_index = decoded->second_key_frame_index;
    animation.next_animation_index = decoded->next_animation_index;
    animation.flags = decoded->flags;
    animation.sound_index = decoded->sound_index;
    animation.private_sound_frame_index = decoded->sound_frame_index;
    animation.private_left_foot_frame_index = decoded->left_foot_frame_index;
    animation.private_right_foot_frame_index = decoded->right_foot_frame_index;
    animation.runtime_parent_animation_index = decoded->parent_animation_index;
    animation.runtime_normalized_weight = decoded->normalized_weight;
    animation.frame_info.size = decoded->frame_info_size;
    animation.frame_info.address = decoded->frame_info_size ? decoded->frame_info : (void *)&empty_stream;
    animation.default_data.size = decoded->default_size;
    animation.default_data.address = decoded->default_size ? decoded->defaults : (void *)&empty_stream;
    animation.data.size = decoded->data_size;
    animation.data.address = decoded->data_size ? decoded->data : (void *)&empty_stream;
    for (i = 0; i < 2; ++i) {
        animation.nodes_with_translation_flags[i] = decoded->translation_flags[i];
        animation.nodes_with_rotation_flags[i] = decoded->rotation_flags[i];
        animation.nodes_with_scale_flags[i] = decoded->scale_flags[i];
    }
    memset(&model, 0, sizeof(model));
    memset(model_nodes, 0, sizeof(model_nodes));
    model.node_list_checksum = skeleton->checksum;
    model.nodes.count = skeleton->count;
    model.nodes.address = model_nodes;
    for (i = 0; i < skeleton->count; ++i) {
        const struct mac_retail_model_node *source = &skeleton->nodes[i];
        struct model_node *node = &model_nodes[i];
        real_matrix4x3 *inverse = &node->runtime_default_inverse_matrix;
        memcpy(node->name, source->name, sizeof(node->name));
        node->next_sibling_node_index = source->sibling;
        node->first_child_node_index = source->child;
        node->parent_node_index = source->parent;
        node->default_translation.x = source->translation[0];
        node->default_translation.y = source->translation[1];
        node->default_translation.z = source->translation[2];
        node->default_rotation.v.i = source->rotation[0];
        node->default_rotation.v.j = source->rotation[1];
        node->default_rotation.v.k = source->rotation[2];
        node->default_rotation.w = source->rotation[3];
        node->node_distance_from_parent = source->distance;
        inverse->scale = source->inverse[0];
        inverse->forward.i = source->inverse[1];
        inverse->forward.j = source->inverse[2];
        inverse->forward.k = source->inverse[3];
        inverse->left.i = source->inverse[4];
        inverse->left.j = source->inverse[5];
        inverse->left.k = source->inverse[6];
        inverse->up.i = source->inverse[7];
        inverse->up.j = source->inverse[8];
        inverse->up.k = source->inverse[9];
        inverse->position.x = source->inverse[10];
        inverse->position.y = source->inverse[11];
        inverse->position.z = source->inverse[12];
    }
    animation_get_node_orientations(&model, &animation, (short)frame, orientations);
    for (i = 0; i < (unsigned)decoded->node_count; ++i) {
        out[i].rotation[0] = orientations[i].rotation.v.i;
        out[i].rotation[1] = orientations[i].rotation.v.j;
        out[i].rotation[2] = orientations[i].rotation.v.k;
        out[i].rotation[3] = orientations[i].rotation.w;
        out[i].translation[0] = orientations[i].translation.x;
        out[i].translation[1] = orientations[i].translation.y;
        out[i].translation[2] = orientations[i].translation.z;
        out[i].scale = orientations[i].scale;
    }
    return 1;
}

/* Native owned tag-data service for this isolated engine slice. A full native
   tag registry will own this boundary. It deliberately fails invalid access;
   it never supplies fake empty data or pretends a CE cache is active. */
void *tag_data_get_pointer(const struct tag_data *data, long offset, long size)
{
    if (!data || size < 0 || offset < 0 || data->size < 0 ||
        offset > data->size || size > data->size - offset ||
        (!data->address && (offset || size))) {
        fprintf(stderr, "invalid native animation tag-data access\n");
        abort();
    }
    return data->address ? (unsigned char *)data->address + offset : NULL;
}
