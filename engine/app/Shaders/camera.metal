// The camera's passes (CameraPass.swift): the lens flares' occlusion test
// against the depth, the sprites of the flares and of the objects' light
// volumes and glows added to the frame, the frame's average brightness for
// the exposure, and the bloom the tone map adds.

#include <metal_stdlib>
using namespace metal;

// (as CameraPass's CameraUniforms)
struct Camera {
    float4x4 world_to_clip;
    // the camera's right and up in the world
    float4 right;
    float4 up;
    // the width and height in texels, the occluders, and the first sprite
    // of the draw
    float4 size;
    uint4 counts;
};

// (as CameraPass's GPUSprite)
struct Sprite {
    // its centre, and its turn on the screen
    float4 center;
    // its half width and half height
    float4 extent;
    // its colour times its brightness
    float4 color;
    // its occluder, its flags, and whether it is a soft disk
    uint4 refs;
};

constant uint NO_OCCLUDER = 0xffffffff;
// (as camera.rs's SPRITE_SCALED_BY_OCCLUSION)
constant uint SCALED_BY_OCCLUSION = 1;
// The samples across an occluder's disk, each way.
constant int OCCLUSION_STEPS = 3;

// the share of each flare's disk not hidden by what is nearer (reversed
// depth: the scene hides a sample where its depth is greater); samples off
// the screen are hidden
kernel void camera_occlusion(device const float4 *occluders [[buffer(0)]],
                             device float *seen [[buffer(1)]],
                             constant Camera &camera [[buffer(2)]],
                             depth2d<float> depth [[texture(0)]],
                             uint index [[thread_position_in_grid]]) {
    if (index >= camera.counts.x) {
        return;
    }
    float4 occluder = occluders[index];
    float4 clip = camera.world_to_clip * float4(occluder.xyz, 1);
    if (clip.w <= 1e-4) {
        seen[index] = 0;
        return;
    }
    float2 centre = clip.xy / clip.w;
    float disk = clip.z / clip.w;
    float4 edge = camera.world_to_clip * float4(occluder.xyz + camera.up.xyz * occluder.w, 1);
    float2 size = camera.size.xy;
    float radius = clamp(length((edge.xy / max(edge.w, 1e-4) - centre) * size * 0.5), 1.0, 64.0);
    float2 pixel = float2(centre.x * 0.5 + 0.5, 0.5 - centre.y * 0.5) * size;
    int visible = 0;
    int total = 0;
    for (int y = -OCCLUSION_STEPS; y <= OCCLUSION_STEPS; y++) {
        for (int x = -OCCLUSION_STEPS; x <= OCCLUSION_STEPS; x++) {
            float2 offset = float2(x, y) / float(OCCLUSION_STEPS);
            if (dot(offset, offset) > 1) {
                continue;
            }
            total++;
            float2 at = pixel + offset * radius;
            if (any(at < 0) || any(at >= size)) {
                continue;
            }
            if (depth.read(uint2(at)) <= disk) {
                visible++;
            }
        }
    }
    seen[index] = float(visible) / float(max(total, 1));
}

struct SpriteOut {
    float4 position [[position]];
    float2 texcoord;
    float3 color;
    uint soft [[flat]];
};

// a quad facing the camera, turned on the screen, its brightness (and,
// for some, its size) its flare's share seen
vertex SpriteOut camera_sprite_vertex(uint vertex_index [[vertex_id]], uint instance [[instance_id]],
                                      device const Sprite *sprites [[buffer(20)]],
                                      device const float *seen [[buffer(21)]],
                                      constant Camera &camera [[buffer(22)]]) {
    const float2 corners[6] = {float2(-1, -1), float2(1, -1), float2(-1, 1),
                               float2(-1, 1), float2(1, -1), float2(1, 1)};
    Sprite sprite = sprites[camera.counts.y + instance];
    float share = sprite.refs.x == NO_OCCLUDER ? 1.0 : seen[sprite.refs.x];
    float scale = (sprite.refs.y & SCALED_BY_OCCLUSION) != 0 ? (share + 1) * 0.5 : 1.0;
    float2 corner = corners[vertex_index];
    float2 extent = corner * sprite.extent.xy * scale;
    float turn = sprite.center.w;
    float2 turned = float2(extent.x * cos(turn) - extent.y * sin(turn),
                           extent.x * sin(turn) + extent.y * cos(turn));
    float3 world = sprite.center.xyz + camera.right.xyz * turned.x + camera.up.xyz * turned.y;
    SpriteOut out;
    out.position = camera.world_to_clip * float4(world, 1);
    out.texcoord = float2(corner.x * 0.5 + 0.5, 0.5 - corner.y * 0.5);
    out.color = sprite.color.rgb * share;
    out.soft = sprite.refs.z;
    if (share <= 0) {
        out.position = float4(0, 0, -1, 1);
    }
    return out;
}

// added to the frame: its picture times its colour (a soft disk without one)
fragment float4 camera_sprite_fragment(SpriteOut in [[stage_in]], texture2d<float> image [[texture(30)]]) {
    constexpr sampler linear(filter::linear, mip_filter::linear, address::clamp_to_edge);
    float3 shade;
    if (in.soft != 0) {
        float2 from = in.texcoord * 2 - 1;
        float fall = saturate(1 - dot(from, from));
        shade = float3(fall * fall);
    } else {
        float4 texel = image.sample(linear, in.texcoord);
        shade = texel.rgb * texel.a;
    }
    return float4(shade * in.color, 0);
}

constant uint LUMINANCE_SIDE = 16;
constant uint LUMINANCE_SAMPLES = 4;

float luminance(float3 color) {
    return dot(max(color, 0), float3(0.2126, 0.7152, 0.0722));
}

// the average of the log2 brightness of 64 by 64 samples of the frame, in
// one threadgroup of 16 by 16, into `average[slot]`
kernel void camera_luminance(texture2d<float> frame [[texture(0)]],
                             device float *average [[buffer(0)]],
                             constant uint &slot [[buffer(1)]],
                             uint2 local [[thread_position_in_threadgroup]],
                             uint place [[thread_index_in_threadgroup]]) {
    threadgroup float sums[LUMINANCE_SIDE * LUMINANCE_SIDE];
    constexpr sampler linear(filter::linear, address::clamp_to_edge);
    float sum = 0;
    float side = float(LUMINANCE_SIDE * LUMINANCE_SAMPLES);
    for (uint y = 0; y < LUMINANCE_SAMPLES; y++) {
        for (uint x = 0; x < LUMINANCE_SAMPLES; x++) {
            float2 at = (float2(local * LUMINANCE_SAMPLES + uint2(x, y)) + 0.5) / side;
            sum += log2(max(luminance(frame.sample(linear, at, level(0)).rgb), 1e-4));
        }
    }
    sums[place] = sum;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = LUMINANCE_SIDE * LUMINANCE_SIDE / 2; stride > 0; stride /= 2) {
        if (place < stride) {
            sums[place] += sums[place + stride];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    if (place == 0) {
        average[slot] = sums[0] / (side * side);
    }
}

// The brightness above which light blooms.
constant float BLOOM_THRESHOLD = 1.0;

// a level of the bloom's chain: four filtered samples of the level above
// it (from the frame, only what is brighter than the threshold)
kernel void camera_bloom_down(texture2d<float> above [[texture(0)]],
                              texture2d<float, access::write> result [[texture(1)]],
                              constant uint &first [[buffer(0)]],
                              uint2 texel [[thread_position_in_grid]]) {
    if (texel.x >= result.get_width() || texel.y >= result.get_height()) {
        return;
    }
    constexpr sampler linear(filter::linear, address::clamp_to_edge);
    float2 size = float2(result.get_width(), result.get_height());
    float2 at = (float2(texel) + 0.5) / size;
    float2 step = 0.5 / size;
    float3 sum = 0;
    for (int corner = 0; corner < 4; corner++) {
        float2 offset = float2(corner & 1 ? 1 : -1, corner & 2 ? 1 : -1) * step;
        float3 color = max(above.sample(linear, at + offset, level(0)).rgb, 0);
        if (first != 0) {
            color = max(color - BLOOM_THRESHOLD, 0);
        }
        sum += color;
    }
    result.write(float4(sum / 4, 1), texel);
}

// a level of the bloom going up: its own and the level below's, widened
kernel void camera_bloom_up(texture2d<float> down [[texture(0)]],
                            texture2d<float> below [[texture(1)]],
                            texture2d<float, access::write> result [[texture(2)]],
                            uint2 texel [[thread_position_in_grid]]) {
    if (texel.x >= result.get_width() || texel.y >= result.get_height()) {
        return;
    }
    constexpr sampler linear(filter::linear, address::clamp_to_edge);
    float2 size = float2(result.get_width(), result.get_height());
    float2 at = (float2(texel) + 0.5) / size;
    float2 step = 1.0 / size;
    // (a tent of nine samples)
    float3 wide = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float weight = (x == 0 ? 2.0 : 1.0) * (y == 0 ? 2.0 : 1.0);
            wide += below.sample(linear, at + float2(x, y) * step, level(0)).rgb * weight;
        }
    }
    result.write(float4(down.read(texel).rgb + wide / 16, 1), texel);
}
