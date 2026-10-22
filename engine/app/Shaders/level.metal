// The level's and its objects' Metal shaders (LevelRenderer.swift,
// ModelPass.swift), compiled from source when a level loads, and again
// when this file changes in a development build (ShaderSources.swift).
//
// A surface is its base map, its detail maps combined as its environment
// shader says, times its lightmap page: as the stock game draws it, in
// display values (the game's pipeline was not linear), into a
// half-float target that the tone map then brings into the drawable.
// The sky's atmospheric fog mixes each surface toward its color by its
// distance along the view, as the game's vertex shaders' fog constants
// do; the sky itself is drawn first, at infinity, unfogged.

#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texcoord [[attribute(2)]];
    float2 lightmap [[attribute(3)]];
};

// (as LevelRenderer's FrameUniforms)
struct Frame {
    float4x4 view_projection;
    // the view's forward over the fog's range, and where it starts
    float4 fog_plane;
    // the fog's color, and its maximum density
    float4 fog_color;
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
    // 0 mixed toward the fog's color, 1 added (faded out), 2 multiplied
    // (faded to white), 3 double-multiplied (faded to grey)
    uint fog_mode;
    // a transparent chicago shader's maps (0: not one), each one's color
    // function, alpha function << 8 and alpha replicate << 16, and its
    // scale and offset
    uint chicago_count;
    uint4 chicago_functions;
    float4 chicago_transforms[4];
};

constant uint BASE = 1;
constant uint PRIMARY = 2;
constant uint SECONDARY = 4;
constant uint LIGHTMAP = 8;

struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
    float2 lightmap;
    float fog;
};

// how much of a point in the world the fog hides
float fog_amount(constant Frame &frame, float3 world) {
    return saturate(dot(frame.fog_plane.xyz, world) + frame.fog_plane.w) * frame.fog_color.a;
}

// a fragment's color behind `fog` of the fog, as its blend takes it
float4 fogged(float4 color, float fog, constant Frame &frame, constant Material &material) {
    switch (material.fog_mode) {
    case 1: color.rgb *= 1 - fog; break;
    case 2: color.rgb = mix(color.rgb, float3(1), fog); break;
    case 3: color.rgb = mix(color.rgb, float3(0.5), fog); break;
    default: color.rgb = mix(color.rgb, frame.fog_color.rgb, fog); break;
    }
    return color;
}

vertex VertexOut level_vertex(VertexIn in [[stage_in]], constant Frame &frame [[buffer(1)]]) {
    VertexOut out;
    out.position = frame.view_projection * float4(in.position, 1);
    out.texcoord = in.texcoord;
    out.lightmap = in.lightmap;
    out.fog = fog_amount(frame, in.position);
    return out;
}

// a surface's base and detail maps, as its material combines them
float4 surface(float2 texcoord, constant Material &material, texture2d<float> base, texture2d<float> primary,
               texture2d<float> secondary, sampler wrap) {
    float4 color = float4(1);
    if (material.maps & BASE) {
        color = base.sample(wrap, texcoord * material.base_scale + material.base_offset);
    }
    if (material.alpha_tested != 0 && color.a < 0.5) {
        discard_fragment();
    }
    uint details = material.maps & (PRIMARY | SECONDARY);
    if (details != 0) {
        float4 first = (details & PRIMARY) ? primary.sample(wrap, texcoord * material.primary_scale)
                                           : secondary.sample(wrap, texcoord * material.secondary_scale);
        float4 second = (details & SECONDARY) ? secondary.sample(wrap, texcoord * material.secondary_scale)
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
    return color;
}

// one stage's function of the result so far and the next map
float3 chicago_color(uint function, float4 current, float4 next) {
    switch (function) {
    case 1: return next.rgb;
    case 2: return current.rgb * next.rgb;
    case 3: return 2 * current.rgb * next.rgb;
    case 4: return current.rgb + next.rgb;
    case 5: case 6: return current.rgb + next.rgb - 0.5;
    case 7: return current.rgb - next.rgb;
    case 8: return next.rgb - current.rgb;
    case 9: return mix(next.rgb, current.rgb, current.a);
    case 10: return mix(current.rgb, next.rgb, current.a);
    case 11: return mix(current.rgb, next.rgb, next.a);
    case 12: return mix(next.rgb, current.rgb, next.a);
    default: return current.rgb;
    }
}

float chicago_alpha(uint function, float current, float next) {
    switch (function) {
    case 1: return next;
    case 2: return current * next;
    case 3: return 2 * current * next;
    case 4: return current + next;
    case 5: case 6: return current + next - 0.5;
    case 7: return current - next;
    case 8: return next - current;
    case 9: case 11: return mix(next, current, current);
    case 10: case 12: return mix(current, next, current);
    default: return current;
    }
}

// a transparent chicago shader's maps, each stage's result combined with
// the next map by the stage's functions
float4 chicago(float2 texcoord, constant Material &material, array<texture2d<float>, 4> maps, sampler wrap) {
    float4 current = float4(1);
    for (uint stage = 0; stage < min(material.chicago_count, 4u); stage++) {
        float4 transform = material.chicago_transforms[stage];
        float4 next = maps[stage].sample(wrap, texcoord * transform.xy + transform.zw);
        if ((material.chicago_functions[stage] >> 16) & 1) {
            next.rgb = next.a;
        }
        if (stage == 0) {
            current = next;
        } else {
            uint functions = material.chicago_functions[stage - 1];
            current = saturate(float4(chicago_color(functions & 0xFF, current, next),
                                      chicago_alpha((functions >> 8) & 0xFF, current.a, next.a)));
        }
    }
    if (material.alpha_tested != 0 && current.a < 0.5) {
        discard_fragment();
    }
    return current;
}

fragment float4 level_fragment(VertexOut in [[stage_in]],
                               constant Material &material [[buffer(0)]],
                               constant Frame &frame [[buffer(1)]],
                               texture2d<float> base [[texture(0)]],
                               texture2d<float> primary [[texture(1)]],
                               texture2d<float> secondary [[texture(2)]],
                               texture2d<float> lightmap [[texture(3)]],
                               sampler wrap [[sampler(0)]],
                               sampler clamp [[sampler(1)]]) {
    if (material.chicago_count != 0) {
        // (unlit, as the game draws them)
        array<texture2d<float>, 4> maps = {base, primary, secondary, lightmap};
        return fogged(chicago(in.texcoord, material, maps, wrap), in.fog, frame, material);
    }
    float4 color = surface(in.texcoord, material, base, primary, secondary, wrap);
    if (material.maps & LIGHTMAP) {
        color.rgb *= lightmap.sample(clamp, in.lightmap).rgb;
    }
    return fogged(color, in.fog, frame, material);
}

struct ModelIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texcoord [[attribute(2)]];
    ushort2 nodes [[attribute(3)]];
    float2 weights [[attribute(4)]];
};

// (as ModelPass's ModelObjectUniforms)
struct ModelObject {
    uint first_node;
    uint node_count;
};

struct ModelOut {
    float4 position [[position]];
    float3 normal;
    float2 texcoord;
    float fog;
};

// a model's vertex moved by its two nodes' skin matrices, by their weights
ModelOut skinned(ModelIn in, constant Frame &frame, const device float4x4 *skins, constant ModelObject &object) {
    uint last = max(object.node_count, 1u) - 1;
    float4x4 skin = skins[object.first_node + min(uint(in.nodes.x), last)] * in.weights.x
                  + skins[object.first_node + min(uint(in.nodes.y), last)] * in.weights.y;
    float4 world = skin * float4(in.position, 1);
    ModelOut out;
    out.position = frame.view_projection * world;
    out.normal = (skin * float4(in.normal, 0)).xyz;
    out.texcoord = in.texcoord;
    out.fog = fog_amount(frame, world.xyz);
    return out;
}

vertex ModelOut model_vertex(ModelIn in [[stage_in]], constant Frame &frame [[buffer(1)]],
                             const device float4x4 *skins [[buffer(2)]],
                             constant ModelObject &object [[buffer(3)]]) {
    return skinned(in, frame, skins, object);
}

// the sky's model, around the camera: at depth 0, infinitely far
vertex ModelOut sky_vertex(ModelIn in [[stage_in]], constant Frame &frame [[buffer(1)]],
                           const device float4x4 *skins [[buffer(2)]],
                           constant ModelObject &object [[buffer(3)]]) {
    ModelOut out = skinned(in, frame, skins, object);
    out.position.z = 0;
    out.fog = 0;
    return out;
}

// the sky as its maps are: it lights itself
fragment float4 sky_fragment(ModelOut in [[stage_in]],
                             constant Material &material [[buffer(0)]],
                             texture2d<float> base [[texture(0)]],
                             texture2d<float> primary [[texture(1)]],
                             texture2d<float> secondary [[texture(2)]],
                             texture2d<float> fourth [[texture(3)]],
                             sampler wrap [[sampler(0)]]) {
    if (material.chicago_count != 0) {
        array<texture2d<float>, 4> maps = {base, primary, secondary, fourth};
        return chicago(in.texcoord, material, maps, wrap);
    }
    return surface(in.texcoord, material, base, primary, secondary, wrap);
}

// until the level's lights reach objects: a sky light from above and a
// little from the side, and some ambient
fragment float4 model_fragment(ModelOut in [[stage_in]],
                               constant Material &material [[buffer(0)]],
                               constant Frame &frame [[buffer(1)]],
                               texture2d<float> base [[texture(0)]],
                               texture2d<float> primary [[texture(1)]],
                               texture2d<float> secondary [[texture(2)]],
                               texture2d<float> fourth [[texture(3)]],
                               sampler wrap [[sampler(0)]]) {
    if (material.chicago_count != 0) {
        array<texture2d<float>, 4> maps = {base, primary, secondary, fourth};
        return fogged(chicago(in.texcoord, material, maps, wrap), in.fog, frame, material);
    }
    float4 color = surface(in.texcoord, material, base, primary, secondary, wrap);
    float3 normal = length_squared(in.normal) > 1e-12 ? normalize(in.normal) : float3(0, 0, 1);
    float light = 0.45 + 0.65 * saturate(dot(normal, normalize(float3(0.35, 0.25, 0.9))));
    color.rgb *= light;
    return fogged(color, in.fog, frame, material);
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
