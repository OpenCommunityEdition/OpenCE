// The particles, contrails, weather and detail objects (ParticlePass.swift):
// each row a quad facing the screen, standing on its axis, spanning a
// strip's segment, or between four corners (a contrail's piece), blended
// with the frame as its framebuffer blend function says (read from the
// frame: Apple GPUs blend in the shader).

#include <metal_stdlib>
using namespace metal;

struct ParticleFrame {
    float4x4 worldToClip;
    float4 eye;
    float4 right;
    float4 up;
};

// (as snapshot.rs's ParticleSnapshot)
struct Particle {
    // (facing 3, a quad: position, axis and shape's xyz are three corners,
    // their ws the fourth; modes.w the far corners' colour, packed)
    float4 position;   // xyz, then the half width
    float4 axis;       // the axis or the segment's end; w the screen turn
    float4 color;      // rgba
    float4 uv;         // the picture's rectangle: u0, v0, u1, v1
    float4 shape;      // half height, fade begins, fade ends, 0
    uint4 modes;       // picture, blend, facing, 0
};

struct ParticleVertex {
    float4 position [[position]];
    float2 uv;
    float4 color;
    uint blend [[flat]];
};

constant float2 corners[6] = {
    float2(-1, -1), float2(1, -1), float2(1, 1),
    float2(-1, -1), float2(1, 1), float2(-1, 1),
};

vertex ParticleVertex particle_vertex(uint vid [[vertex_id]], uint instance [[instance_id]],
                                      constant ParticleFrame &frame [[buffer(0)]],
                                      const device Particle *particles [[buffer(1)]],
                                      constant uint &first [[buffer(2)]]) {
    Particle p = particles[first + instance];
    float2 c = corners[vid];
    if (p.modes.z == 3) {
        // A quad: the near edge's two corners, then the far edge's.
        float3 far = float3(p.position.w, p.axis.w, p.shape.w);
        float3 world = c.x < 0 ? (c.y < 0 ? p.position.xyz : p.axis.xyz)
                               : (c.y < 0 ? p.shape.xyz : far);
        ParticleVertex out;
        out.position = frame.worldToClip * float4(world, 1);
        out.uv = float2(c.x < 0 ? p.uv.x : p.uv.z, c.y < 0 ? p.uv.y : p.uv.w);
        out.color = c.x < 0 ? p.color : unpack_unorm4x8_to_float(p.modes.w);
        out.blend = p.modes.y;
        return out;
    }
    float3 centre = p.position.xyz;
    float3 side;
    float3 up;
    float width = p.position.w;
    float height = p.shape.x > 0 ? p.shape.x : width;
    if (p.modes.z == 2) {
        // A segment: from its position to its axis, across the view.
        float3 along = p.axis.xyz - centre;
        float3 view = normalize(frame.eye.xyz - centre);
        side = normalize(cross(along, view) + float3(0, 0, 1e-6)) * width;
        centre += along * 0.5;
        up = along * 0.5;
    } else if (p.modes.z == 1) {
        // On its axis: up the axis, across the view.
        float3 axis = normalize(p.axis.xyz + float3(0, 0, 1e-6));
        float3 view = normalize(frame.eye.xyz - centre);
        side = normalize(cross(axis, view) + float3(1e-6, 0, 0)) * width;
        up = axis * height;
    } else {
        float s = sin(p.axis.w);
        float k = cos(p.axis.w);
        side = (frame.right.xyz * k + frame.up.xyz * s) * width;
        up = (frame.up.xyz * k - frame.right.xyz * s) * height;
    }
    float3 world = centre + side * c.x + up * c.y;
    float fade = 1.0;
    if (p.shape.z > p.shape.y) {
        float d = distance(frame.eye.xyz, p.position.xyz);
        fade = 1.0 - saturate((d - p.shape.y) / (p.shape.z - p.shape.y));
    }
    ParticleVertex out;
    out.position = frame.worldToClip * float4(world, 1);
    float2 t = c * 0.5 + 0.5;
    out.uv = float2(mix(p.uv.x, p.uv.z, t.x), mix(p.uv.w, p.uv.y, t.y));
    out.color = float4(p.color.rgb, p.color.a * fade);
    out.blend = p.modes.y;
    return out;
}

fragment float4 particle_fragment(ParticleVertex in [[stage_in]], float4 dst [[color(0)]],
                                  texture2d<float> picture [[texture(0)]],
                                  sampler linear [[sampler(0)]]) {
    float4 s = picture.sample(linear, in.uv) * in.color;
    switch (in.blend) {
    case 1: return float4(dst.rgb * mix(float3(1), s.rgb, s.a), dst.a);
    case 2: return float4(dst.rgb * mix(float3(1), 2 * s.rgb, s.a), dst.a);
    case 3: return float4(dst.rgb + s.rgb * s.a, dst.a);
    case 4: return float4(max(dst.rgb - s.rgb * s.a, 0), dst.a);
    case 5: return float4(min(dst.rgb, mix(dst.rgb, s.rgb, s.a)), dst.a);
    case 6: return float4(max(dst.rgb, mix(dst.rgb, s.rgb, s.a)), dst.a);
    case 7: return float4(dst.rgb * (1 - s.a) + s.rgb, dst.a);
    default: return float4(mix(dst.rgb, s.rgb, s.a), dst.a);
    }
}
