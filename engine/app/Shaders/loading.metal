// The loading screen (LoadingPass.swift, engine/crates/ui/src/loading.rs,
// after source/interface/progress_bar.c): each frame the last one is drawn
// again, blurred, a little larger and fainter, then the band of light over
// the loading picture; the result is laid over the mission as it fades in.

#include <metal_stdlib>
using namespace metal;

struct LoadingUniforms {
    /// x: the light's brightness, y: the band's left edge in the screen's
    /// 640 pixels, z: the trail's alpha, w: the screen's alpha.
    float4 frame;
    /// x: the trail's enlargement, y: its blur in uv, across; z: down.
    float4 trail;
};

struct LoadingVarying {
    float4 position [[position]];
    float2 uv;
};

// a quad over the target, scaled about its centre by `scale`
static LoadingVarying corner(uint vertex_id, float scale) {
    const float2 corners[4] = {float2(-1, 1), float2(1, 1), float2(-1, -1), float2(1, -1)};
    float2 at = corners[vertex_id];
    LoadingVarying out;
    out.position = float4(at * scale, 0, 1);
    out.uv = float2(at.x * 0.5 + 0.5, 0.5 - at.y * 0.5);
    return out;
}

vertex LoadingVarying loading_vertex(uint vertex_id [[vertex_id]]) {
    return corner(vertex_id, 1.0);
}

vertex LoadingVarying loading_trail_vertex(uint vertex_id [[vertex_id]],
                                           constant LoadingUniforms &uniforms [[buffer(0)]]) {
    return corner(vertex_id, 1.0 + uniforms.trail.x);
}

// the last frame, blurred by four taps, at the trail's alpha (added to black)
fragment float4 loading_trail(LoadingVarying in [[stage_in]], texture2d<float> last [[texture(0)]],
                              sampler linear [[sampler(0)]],
                              constant LoadingUniforms &uniforms [[buffer(0)]]) {
    float2 across = float2(uniforms.trail.y, 0), down = float2(0, uniforms.trail.z);
    float4 sum = last.sample(linear, in.uv + across) + last.sample(linear, in.uv - across)
        + last.sample(linear, in.uv + down) + last.sample(linear, in.uv - down);
    return float4(sum.rgb * 0.25 * uniforms.frame.z, 1);
}

// the band of light over the picture (added)
fragment float4 loading_light(LoadingVarying in [[stage_in]], texture2d<float> picture [[texture(0)]],
                              texture2d<float> mask [[texture(1)]], sampler linear [[sampler(0)]],
                              sampler border [[sampler(1)]],
                              constant LoadingUniforms &uniforms [[buffer(0)]]) {
    float x = in.uv.x * 640.0;
    float2 band = float2((x - uniforms.frame.y) / 128.0, in.uv.y);
    float light = mask.sample(border, band).r;
    float3 lit = picture.sample(linear, in.uv).rgb * uniforms.frame.x * light;
    return float4(lit, 0);
}

// premultiplied out: the screen over the mission
fragment float4 loading_composite(LoadingVarying in [[stage_in]], texture2d<float> screen [[texture(0)]],
                                  sampler linear [[sampler(0)]],
                                  constant LoadingUniforms &uniforms [[buffer(0)]]) {
    float alpha = uniforms.frame.w;
    return float4(screen.sample(linear, in.uv).rgb * alpha, alpha);
}
