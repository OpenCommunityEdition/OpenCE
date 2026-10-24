// The screen's passes (ScreenPass.swift), between the opaque draws and the
// blended ones: ambient occlusion and the sun's contact shadows from the
// depth and the surfaces' normals, and reflections traced through the
// depth over the opaque frame; then a resolve that applies them to the
// frame, and the views that show one buffer alone (`render.view`).
//
// The bake already holds the level's own occlusion and its shadows of the
// sun, so on the level only objects occlude and shadow; on an object,
// everything does.

#include <metal_stdlib>
using namespace metal;

// (as level.metal's Opaque.surface.w)
constant float SURFACE_LEVEL = 1;
constant float SURFACE_OBJECT = 2;

// (as ScreenPass's ScreenUniforms)
struct Screen {
    // clip to world space, and back
    float4x4 clip_to_world;
    float4x4 world_to_clip;
    // the eye, and the near plane's distance (depth is near over the
    // distance ahead)
    float4 eye;
    // toward the sun, and 1 when there is one
    float4 sun;
    // the width and height in texels, and the pixels a world unit spans
    // one unit ahead
    float4 size;
    // the passes on (gtao, contact shadows, reflections) and the view shown
    uint4 passes;
    // the occlusion's resolution (0 full, 1 half)
    uint4 quality;
    // how much the occlusion darkens, scaled (the lighting controls)
    float4 tune;
};

constant uint VIEW_DEPTH = 1;
constant uint VIEW_NORMALS = 2;
constant uint VIEW_AO = 3;
constant uint VIEW_CONTACT = 4;
constant uint VIEW_SSR = 5;

// how far the occlusion looks, and the contact shadows march toward the
// sun, in world units (one is about 3 m)
constant float OCCLUSION_RADIUS = 0.3;
constant float CONTACT_LENGTH = 0.15;
// how thick a surface in the depth is taken to be behind its front
constant float CONTACT_THICKNESS = 0.08;
constant uint SLICES = 2;
constant uint SLICE_STEPS = 4;
constant uint CONTACT_STEPS = 8;
constant uint REFLECTION_STEPS = 32;

// a value in 0..1 that changes from texel to texel without a pattern the
// eye follows (interleaved gradient noise)
float noise(uint2 texel) {
    return fract(52.9829189 * fract(0.06711056 * float(texel.x) + 0.00583715 * float(texel.y)));
}

float3 world_at(constant Screen &screen, float2 uv, float depth) {
    float4 at = screen.clip_to_world * float4(uv.x * 2 - 1, 1 - uv.y * 2, depth, 1);
    return at.xyz / at.w;
}

// whether a surface of `kind` darkens one of `receiver`'s: on the level,
// only objects do (the bake holds the level's own)
bool occludes(float receiver, float kind) {
    return kind == SURFACE_OBJECT || (receiver == SURFACE_OBJECT && kind == SURFACE_LEVEL);
}

bool screened(float kind, float depth) {
    return (kind == SURFACE_LEVEL || kind == SURFACE_OBJECT) && depth > 0 && depth <= 0.5;
}

// a texel's ambient occlusion (r) and contact shadow (g), for one that is
// screened at depth `d`, `ahead` of the eye
float2 occlusion_at(depth2d<float> depth, texture2d<float> surface, constant Screen &screen, uint2 texel,
                    float d, float4 here, float ahead) {
    float2 uv = (float2(texel) + 0.5) / screen.size.xy;
    float3 position = world_at(screen, uv, d);
    float3 normal = normalize(here.xyz);
    float jitter = noise(texel);

    // the occlusion: in slices about the texel, each side's highest
    // horizon toward the normal, nearer ones counting more
    float ao = 1;
    float reach = OCCLUSION_RADIUS * screen.size.w / ahead;
    if (screen.passes.x != 0 && reach >= 1.5) {
        reach = min(reach, 64.0);
        float occlusion = 0;
        for (uint slice = 0; slice < SLICES; slice++) {
            float angle = (float(slice) + jitter) * M_PI_F / float(SLICES);
            float2 along = float2(cos(angle), sin(angle));
            for (int side = -1; side <= 1; side += 2) {
                float highest = 0;
                for (uint step = 1; step <= SLICE_STEPS; step++) {
                    float2 at = float2(texel) + along * float(side) * reach * (float(step) - 0.5 + jitter * 0.5)
                                / float(SLICE_STEPS);
                    if (any(at < 0) || at.x >= screen.size.x || at.y >= screen.size.y) {
                        break;
                    }
                    uint2 sample = uint2(at);
                    float sd = depth.read(sample);
                    if (sd <= 0 || !occludes(here.w, surface.read(sample).w)) {
                        continue;
                    }
                    float3 toward = world_at(screen, (float2(sample) + 0.5) / screen.size.xy, sd) - position;
                    float distance = length(toward);
                    if (distance < 1e-4) {
                        continue;
                    }
                    float falloff = saturate(1 - distance / OCCLUSION_RADIUS);
                    highest = max(highest, saturate(dot(toward / distance, normal) - 0.1) * falloff);
                }
                occlusion += highest;
            }
        }
        ao = saturate(1 - occlusion / float(SLICES * 2) * 1.5);
    }

    // the contact shadow: a short march toward the sun, stopped by a
    // surface in front of the ray but not far in front
    float contact = 0;
    if (screen.passes.y != 0 && screen.sun.w > 0 && dot(normal, screen.sun.xyz) > 0) {
        float3 start = position + normal * 0.004;
        for (uint step = 1; step <= CONTACT_STEPS; step++) {
            float3 at = start + screen.sun.xyz * CONTACT_LENGTH * (float(step) - 1 + jitter) / float(CONTACT_STEPS);
            float4 clip = screen.world_to_clip * float4(at, 1);
            if (clip.w <= screen.eye.w) {
                break;
            }
            float2 seen = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
            if (any(seen < 0) || any(seen >= 1)) {
                break;
            }
            uint2 sample = uint2(seen * screen.size.xy);
            float sd = depth.read(sample);
            if (sd <= 0 || sd > 0.5) {
                continue;
            }
            float behind = clip.w - screen.eye.w / sd;
            if (behind > 0.002 && behind < CONTACT_THICKNESS && occludes(here.w, surface.read(sample).w)) {
                // (softer toward the march's end)
                contact = 1 - float(step - 1) / float(CONTACT_STEPS);
                break;
            }
        }
    }
    return float2(ao, contact);
}

// the occlusion at half the resolution (`render.ao_quality` half): each
// texel's of the even rows and columns, with its distance ahead (b) and
// whether it is screened (a)
kernel void screen_occlusion_half(depth2d<float> depth [[texture(0)]],
                                  texture2d<float> surface [[texture(1)]],
                                  texture2d<float, access::write> out [[texture(2)]],
                                  constant Screen &screen [[buffer(0)]],
                                  uint2 coarse [[thread_position_in_grid]]) {
    uint2 size = uint2(screen.size.xy);
    if (coarse.x >= (size.x + 1) / 2 || coarse.y >= (size.y + 1) / 2) {
        return;
    }
    uint2 texel = min(coarse * 2, size - 1);
    float d = depth.read(texel);
    float4 here = surface.read(texel);
    float ahead = d > 0 ? screen.eye.w / d : 65000.0;
    if (!screened(here.w, d)) {
        out.write(float4(1, 0, ahead, 0), coarse);
        return;
    }
    out.write(float4(occlusion_at(depth, surface, screen, texel, d, here, ahead), ahead, 1), coarse);
}

// each texel's ambient occlusion (r), contact shadow (g) and a shade of
// its depth for the views (b): computed here, or taken from the half
// resolution's four nearest of about the same depth
kernel void screen_occlusion(depth2d<float> depth [[texture(0)]],
                             texture2d<float> surface [[texture(1)]],
                             texture2d<float, access::write> out [[texture(2)]],
                             texture2d<float> half_occlusion [[texture(3)]],
                             constant Screen &screen [[buffer(0)]],
                             uint2 texel [[thread_position_in_grid]]) {
    uint2 size = uint2(screen.size.xy);
    if (texel.x >= size.x || texel.y >= size.y) {
        return;
    }
    float d = depth.read(texel);
    float4 here = surface.read(texel);
    float ahead = d > 0 ? screen.eye.w / d : INFINITY;
    float shade = d > 0 ? exp(-ahead / 20) : 0;
    if (!screened(here.w, d)) {
        out.write(float4(1, 0, shade, 1), texel);
        return;
    }
    if (screen.quality.x == 0) {
        out.write(float4(occlusion_at(depth, surface, screen, texel, d, here, ahead), shade, 1), texel);
        return;
    }
    // (bilinear weights, each scaled down as its depth differs from this
    // texel's, so that an edge does not bleed across)
    float2 at = float2(texel) * 0.5;
    int2 base = int2(floor(at));
    float2 f = at - float2(base);
    int2 last = int2(half_occlusion.get_width(), half_occlusion.get_height()) - 1;
    float2 sum = 0;
    float total = 0;
    float nearest = INFINITY;
    float2 fallback = float2(1, 0);
    for (int y = 0; y <= 1; y++) {
        for (int x = 0; x <= 1; x++) {
            float4 c = half_occlusion.read(uint2(clamp(base + int2(x, y), int2(0), last)));
            if (c.a == 0) {
                continue;
            }
            float differs = abs(c.b - ahead) / ahead;
            if (differs < nearest) {
                nearest = differs;
                fallback = c.rg;
            }
            float w = (x == 0 ? 1 - f.x : f.x) * (y == 0 ? 1 - f.y : f.y) * saturate(1 - differs * 20);
            sum += c.rg * w;
            total += w;
        }
    }
    out.write(float4(total > 1e-4 ? sum / total : fallback, shade, 1), texel);
}

// each reflecting texel's reflection traced through the depth: the colour
// of the opaque frame where the ray meets a surface (rgb), and how far it
// is trusted (a; none off the screen's edges or past the march)
kernel void screen_reflection(depth2d<float> depth [[texture(0)]],
                              texture2d<float> surface [[texture(1)]],
                              texture2d<float> reflectance [[texture(2)]],
                              texture2d<float> frame [[texture(3)]],
                              texture2d<float, access::write> out [[texture(4)]],
                              constant Screen &screen [[buffer(0)]],
                              uint2 texel [[thread_position_in_grid]]) {
    uint2 size = uint2(screen.size.xy);
    if (texel.x >= size.x || texel.y >= size.y) {
        return;
    }
    float d = depth.read(texel);
    float4 here = surface.read(texel);
    float3 weight = reflectance.read(texel).rgb;
    if (screen.passes.z == 0 || !screened(here.w, d) || max3(weight.r, weight.g, weight.b) <= 1e-3) {
        out.write(float4(0), texel);
        return;
    }
    float2 uv = (float2(texel) + 0.5) / screen.size.xy;
    float3 position = world_at(screen, uv, d);
    float3 normal = normalize(here.xyz);
    float3 eye = normalize(screen.eye.xyz - position);
    float3 ray = reflect(-eye, normal);
    float jitter = noise(texel);
    float3 start = position + normal * 0.01;
    float before = 0;
    float4 found = float4(0);
    for (uint step = 1; step <= REFLECTION_STEPS; step++) {
        // (steps that grow with the distance, as the texels they cross do)
        float t = 0.05 * pow(float(step) + jitter, 1.7);
        float4 clip = screen.world_to_clip * float4(start + ray * t, 1);
        if (clip.w <= screen.eye.w) {
            break;
        }
        float2 seen = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
        if (any(seen < 0) || any(seen >= 1)) {
            break;
        }
        float sd = depth.read(uint2(seen * screen.size.xy));
        float behind = sd > 0 ? clip.w - screen.eye.w / sd : -1;
        if (behind > 0 && behind < 0.2 + t * 0.1) {
            // halved between the last step in front and this one
            float low = before, high = t;
            for (uint refine = 0; refine < 4; refine++) {
                float middle = (low + high) * 0.5;
                float4 c = screen.world_to_clip * float4(start + ray * middle, 1);
                float2 s = float2(c.x / c.w * 0.5 + 0.5, 0.5 - c.y / c.w * 0.5);
                float md = depth.read(uint2(clamp(s, 0.0, 0.9999) * screen.size.xy));
                if (md > 0 && c.w - screen.eye.w / md > 0) {
                    high = middle;
                } else {
                    low = middle;
                }
                seen = s;
            }
            uint2 hit = uint2(clamp(seen, 0.0, 0.9999) * screen.size.xy);
            float4 met = surface.read(hit);
            float met_depth = depth.read(hit);
            if (!screened(met.w, met_depth) || dot(normalize(met.xyz), ray) > 0.2) {
                break;
            }
            float2 edge = min(seen, 1 - seen);
            float trust = saturate(min(edge.x, edge.y) * 10) * (1 - float(step) / float(REFLECTION_STEPS));
            found = float4(frame.read(hit).rgb, trust);
            break;
        }
        before = t;
    }
    out.write(found, texel);
}

struct ScreenOut {
    float4 position [[position]];
};

// one triangle over the whole target
vertex ScreenOut screen_vertex(uint index [[vertex_id]]) {
    float2 corner = float2((index << 1) & 2, index & 2);
    ScreenOut out;
    out.position = float4(corner * float2(2, -2) + float2(-1, 1), 0, 1);
    return out;
}

// the passes applied to the opaque frame (read where it is drawn): the
// traced reflection in place of the one the surface added, as far as it is
// trusted; the occlusion over the light that is not the sun's; the
// contact shadow over the sun's
fragment float4 screen_resolve(ScreenOut in [[stage_in]], float4 current [[color(0)]],
                               texture2d<float> surface [[texture(0)]],
                               texture2d<float> reflection [[texture(1)]],
                               texture2d<float> reflectance [[texture(2)]],
                               texture2d<float> occlusion [[texture(3)]],
                               texture2d<float> reflected [[texture(4)]],
                               constant Screen &screen [[buffer(0)]]) {
    uint2 texel = uint2(in.position.xy);
    float4 here = surface.read(texel);
    if (here.w != SURFACE_LEVEL && here.w != SURFACE_OBJECT) {
        return current;
    }
    float3 color = current.rgb;
    float4 added = reflection.read(texel);
    if (screen.passes.z != 0) {
        float4 traced = reflected.read(texel);
        color = max(color + traced.a * (traced.rgb * reflectance.read(texel).rgb - added.rgb), float3(0));
    }
    float4 occluded = occlusion.read(texel);
    if (screen.passes.x != 0) {
        // (held to 0: the control above 1 darkens past the occlusion)
        color *= saturate(mix(1.0, occluded.r, (1 - added.a) * screen.tune.x));
    }
    if (screen.passes.y != 0) {
        color *= 1 - added.a * occluded.g;
    }
    return float4(color, current.a);
}

// one buffer alone, over the finished frame
fragment float4 screen_view(ScreenOut in [[stage_in]],
                            texture2d<float> surface [[texture(0)]],
                            texture2d<float> occlusion [[texture(3)]],
                            texture2d<float> reflected [[texture(4)]],
                            constant Screen &screen [[buffer(0)]]) {
    uint2 texel = uint2(in.position.xy);
    float4 here = surface.read(texel);
    float4 occluded = occlusion.read(texel);
    switch (screen.passes.w) {
    case VIEW_DEPTH: return float4(float3(occluded.b), 1);
    case VIEW_NORMALS:
        return float4(here.w == SURFACE_LEVEL || here.w == SURFACE_OBJECT ? normalize(here.xyz) * 0.5 + 0.5 : float3(0), 1);
    case VIEW_AO: return float4(float3(occluded.r), 1);
    case VIEW_CONTACT: return float4(float3(1 - occluded.g), 1);
    case VIEW_SSR: {
        float4 traced = reflected.read(texel);
        return float4(traced.rgb * traced.a, 1);
    }
    default: return float4(0, 0, 0, 1);
    }
}
