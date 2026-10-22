// The decals (DecalPass.swift): each a box about its surface, drawn by its
// back faces, projecting its picture onto the surfaces of its kind (the
// level's or the objects') inside it that face its way, blended with the frame as its
// framebuffer blend function says (read from the frame: Apple GPUs blend
// in the shader).

#include <metal_stdlib>
using namespace metal;

struct DecalFrame {
    float4x4 clipToWorld;
    float4x4 worldToClip;
    float4 eye;
    float4 size;
};

// (as DecalPass.swift's GPUDecal)
struct Decal {
    float4 center;   // xyz, and how far above and below the surface
    float4 forward;  // xyz, and the least reach along it
    float4 left;     // xyz, and the least reach along it
    float4 up;       // xyz, and 0
    float4 reach;    // the greatest reach along forward and left, then 0, 0
    float4 uv;       // u0, v0, u1, v1
    float4 color;    // linear rgb, and its brightness
    uint4 modes;     // its blend function, its layer, its picture (NO_PICTURE: none),
                     // the kind of surface it is laid on (level.metal's surface.w)
};

constant uint NO_PICTURE = 0xFFFFFFFF;
constant uint LAYER_ALPHA_TESTED = 3;
// a surface turned further than this from the decal's up is not drawn on
constant float LEAST_FACING = 0.3;

struct DecalVertex {
    float4 position [[position]];
    uint decal [[flat]];
    // whether this face looks away from the eye: only those are drawn, so
    // that each pixel is blended once (inside the box too)
    float away [[flat]];
};

// the box's corner `corner` (of 8), in -1..1 on each of its axes
static float3 corner_of(uint corner) {
    return float3(corner & 1 ? 1.0 : -1.0, corner & 2 ? 1.0 : -1.0, corner & 4 ? 1.0 : -1.0);
}

// the box's 12 triangles, as its corners: two each of its faces at -z,
// +z, -y, +y, -x, +x
constant ushort box_corners[36] = {
    0, 2, 1, 1, 2, 3,  4, 5, 6, 5, 7, 6,
    0, 1, 4, 1, 5, 4,  2, 6, 3, 3, 6, 7,
    0, 4, 2, 2, 4, 6,  1, 3, 5, 3, 7, 5,
};

vertex DecalVertex decal_vertex(uint vid [[vertex_id]], uint instance [[instance_id]],
                                constant DecalFrame &frame [[buffer(0)]],
                                const device Decal *decals [[buffer(1)]],
                                constant uint &first [[buffer(2)]]) {
    const device Decal &decal = decals[first + instance];
    float3 c = corner_of(box_corners[vid]);
    // (a little larger than the decal, so that nothing it covers is cut)
    float across = c.x > 0 ? decal.reach.x : decal.forward.w;
    float along = c.y > 0 ? decal.reach.y : decal.left.w;
    float3 world = decal.center.xyz + decal.forward.xyz * across * 1.01 + decal.left.xyz * along * 1.01
        + decal.up.xyz * c.z * decal.center.w;
    uint face = vid / 6;
    float side = face & 1 ? 1.0 : -1.0;
    float3 outward = side * (face < 2 ? decal.up.xyz : face < 4 ? decal.left.xyz : decal.forward.xyz);
    DecalVertex out;
    out.position = frame.worldToClip * float4(world, 1);
    out.decal = first + instance;
    // (every corner of a face gives the same: they share its plane)
    out.away = dot(outward, world - frame.eye.xyz) > 0 ? 1.0 : 0.0;
    return out;
}

// the frame `dst` with the decal's colour `color` and its opacity `a`
// blended as `function` blends (decals.c's framebuffer blend functions)
static float3 blended(uint function, float3 dst, float3 color, float a) {
    switch (function) {
    case 0: return mix(dst, color, a);                      // alpha blend
    case 1: return dst * mix(float3(1), color, a);          // multiply
    case 2: return dst * 2 * mix(float3(0.5), color, a);    // double multiply
    case 3: return dst + color * a;                         // add
    case 4: return max(dst - color * a, float3(0));         // subtract
    case 5: return min(dst, mix(float3(65504), color, a));  // least
    case 6: return max(dst, color * a);                     // greatest
    default: return dst * (1 - a) + color * a;              // alpha multiply add
    }
}

fragment float4 decal_fragment(DecalVertex in [[stage_in]], float4 dst [[color(0)]],
                               constant DecalFrame &frame [[buffer(0)]],
                               const device Decal *decals [[buffer(1)]],
                               depth2d<float, access::read> depth [[texture(0)]],
                               texture2d<float, access::read> surfaces [[texture(1)]],
                               texture2d<float> picture [[texture(2)]],
                               sampler linear [[sampler(0)]]) {
    if (in.away < 0.5) {
        discard_fragment();
    }
    const device Decal &decal = decals[in.decal];
    uint2 pixel = uint2(in.position.xy);
    float4 surface = surfaces.read(pixel);
    if (surface.w != float(decal.modes.w) || dot(surface.xyz, decal.up.xyz) < LEAST_FACING) {
        discard_fragment();
    }
    float z = depth.read(pixel);
    float2 ndc = (float2(pixel) + 0.5) / frame.size.xy * float2(2, -2) + float2(-1, 1);
    float4 world = frame.clipToWorld * float4(ndc, z, 1);
    float3 offset = world.xyz / world.w - decal.center.xyz;
    float3 local = float3(dot(offset, decal.forward.xyz), dot(offset, decal.left.xyz), dot(offset, decal.up.xyz));
    if (local.x < decal.forward.w || local.x > decal.reach.x || local.y < decal.left.w || local.y > decal.reach.y
        || abs(local.z) > decal.center.w) {
        discard_fragment();
    }
    float2 t = float2((local.x - decal.forward.w) / max(decal.reach.x - decal.forward.w, 1e-6),
                      (local.y - decal.left.w) / max(decal.reach.y - decal.left.w, 1e-6));
    float2 uv = mix(decal.uv.xy, decal.uv.zw, t);
    float4 texel = decal.modes.z != NO_PICTURE ? picture.sample(linear, uv) : float4(1);
    if (decal.modes.y == LAYER_ALPHA_TESTED && texel.a < 0.5) {
        discard_fragment();
    }
    float a = saturate(texel.a * decal.color.a);
    return float4(blended(decal.modes.x, dst.rgb, texel.rgb * decal.color.rgb, a), dst.a);
}
