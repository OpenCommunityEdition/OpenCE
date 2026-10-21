// The level's Metal shaders, compiled from source once when a level
// loads (swift build does not compile .metal files).
//
// A surface is its base map, its detail maps combined as its environment
// shader says, times its lightmap page: as the stock game draws it, in
// display values (the game's pipeline was not linear), into a
// half-float target that the tone map then brings into the drawable.

let levelShaderSource = """
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texcoord [[attribute(2)]];
    float2 lightmap [[attribute(3)]];
};

struct Frame {
    float4x4 view_projection;
};

// (as LevelRenderer's MaterialUniforms)
struct Material {
    float2 base_scale;
    float2 base_offset;
    float primary_scale;
    float secondary_scale;
    uint maps;
    uint detail_function;
    uint environment_kind;
    uint alpha_tested;
};

constant uint BASE = 1;
constant uint PRIMARY = 2;
constant uint SECONDARY = 4;
constant uint LIGHTMAP = 8;

struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
    float2 lightmap;
};

vertex VertexOut level_vertex(VertexIn in [[stage_in]], constant Frame &frame [[buffer(1)]]) {
    VertexOut out;
    out.position = frame.view_projection * float4(in.position, 1);
    out.texcoord = in.texcoord;
    out.lightmap = in.lightmap;
    return out;
}

fragment float4 level_fragment(VertexOut in [[stage_in]],
                               constant Material &material [[buffer(0)]],
                               texture2d<float> base [[texture(0)]],
                               texture2d<float> primary [[texture(1)]],
                               texture2d<float> secondary [[texture(2)]],
                               texture2d<float> lightmap [[texture(3)]],
                               sampler wrap [[sampler(0)]],
                               sampler clamp [[sampler(1)]]) {
    float4 color = float4(1);
    if (material.maps & BASE) {
        color = base.sample(wrap, in.texcoord * material.base_scale + material.base_offset);
    }
    if (material.alpha_tested != 0 && color.a < 0.5) {
        discard_fragment();
    }
    uint details = material.maps & (PRIMARY | SECONDARY);
    if (details != 0) {
        float4 first = (details & PRIMARY) ? primary.sample(wrap, in.texcoord * material.primary_scale)
                                           : secondary.sample(wrap, in.texcoord * material.secondary_scale);
        float4 second = (details & SECONDARY) ? secondary.sample(wrap, in.texcoord * material.secondary_scale)
                                              : first;
        // normal: the secondary map's alpha mixes them; blended: the base's
        float mask = material.environment_kind == 0 ? second.a : color.a;
        float3 detail = material.environment_kind == 0 ? mix(first.rgb, second.rgb, mask)
                                                       : mix(second.rgb, first.rgb, mask);
        switch (material.detail_function) {
        case 1: color.rgb *= detail; break;
        case 2: color.rgb = saturate(color.rgb + 2 * detail - 1); break;
        default: color.rgb = saturate(color.rgb * 2 * detail); break;
        }
    }
    if (material.maps & LIGHTMAP) {
        color.rgb *= lightmap.sample(clamp, in.lightmap).rgb;
    }
    return color;
}

struct ToneOut {
    float4 position [[position]];
    float2 texcoord;
};

// one triangle over the whole target
vertex ToneOut tone_vertex(uint index [[vertex_id]]) {
    float2 corner = float2((index << 1) & 2, index & 2);
    ToneOut out;
    out.position = float4(corner * float2(2, -2) + float2(-1, 1), 0, 1);
    out.texcoord = corner;
    return out;
}

struct Motion {
    // the last frame's view-projection times the inverse of this one's
    float4x4 reprojection;
    float2 size;
};

// each texel's move from where it was in the last frame, in uv units:
// the level is still, so from its depth and the camera's move (the sky,
// at depth 0, is a direction and turns with the camera)
fragment float2 motion_fragment(ToneOut in [[stage_in]], depth2d<float> depth [[texture(0)]],
                                constant Motion &motion [[buffer(0)]]) {
    uint2 texel = uint2(in.position.xy);
    float2 uv = (float2(texel) + 0.5) / motion.size;
    float4 here = float4(uv.x * 2 - 1, 1 - uv.y * 2, depth.read(texel), 1);
    float4 before = motion.reprojection * here;
    if (before.w <= 1e-6) {
        return float2(0);
    }
    float2 was = float2(before.x / before.w * 0.5 + 0.5, 0.5 - before.y / before.w * 0.5);
    return was - uv;
}

// below 0.8 as it is; above, eased into the last fifth, so that a light
// brighter than white keeps its shape instead of flattening
fragment float4 tone_fragment(ToneOut in [[stage_in]], texture2d<float> frame [[texture(0)]]) {
    constexpr sampler nearest(filter::nearest, address::clamp_to_edge);
    float3 color = max(frame.sample(nearest, in.texcoord).rgb, 0);
    float3 over = max(color - 0.8, 0);
    float3 eased = 0.8 + 0.2 * (1 - exp(-over / 0.2));
    return float4(select(color, eased, color > 0.8), 1);
}
"""
