// The HUD's quads over the finished frame (HudPass.swift).

#include <metal_stdlib>
using namespace metal;

struct HudInstance {
    float4 rect;
    float4 uv;
    float4 color;
    float4 min_color;
    float4 max_color;
    float4 empty_color;
    float4 parameters;
};

struct HudVarying {
    float4 position [[position]];
    float2 uv;
    uint instance [[flat]];
};

vertex HudVarying hud_vertex(uint vertex_id [[vertex_id]], uint instance [[instance_id]],
                             const device HudInstance *instances [[buffer(0)]]) {
    // (two triangles: 0 1 2, 2 1 3 of the corners left top, right top,
    // left bottom, right bottom)
    const uint corners[6] = {0, 1, 2, 2, 1, 3};
    uint corner = corners[vertex_id];
    HudInstance quad = instances[instance];
    bool right = (corner & 1) != 0, bottom = (corner & 2) != 0;
    HudVarying out;
    out.position = float4(right ? quad.rect.z : quad.rect.x, bottom ? quad.rect.w : quad.rect.y, 0, 1);
    out.uv = float2(right ? quad.uv.z : quad.uv.x, bottom ? quad.uv.w : quad.uv.y);
    out.instance = instance;
    return out;
}

// premultiplied out
fragment float4 hud_fragment(HudVarying in [[stage_in]], texture2d<float> art [[texture(0)]],
                             sampler linear [[sampler(0)]],
                             const device HudInstance *instances [[buffer(0)]]) {
    HudInstance quad = instances[in.instance];
    float4 texel = art.sample(linear, in.uv);
    float threshold = quad.parameters.x;
    float kind = quad.parameters.y;
    if (kind == 2.0 && threshold >= 0.0) {
        float along = texel.b;
        uint2 size = uint2(art.get_width(), art.get_height());
        uint2 pixel = uint2(clamp(floor(in.uv * float2(size)), float2(0), float2(size) - 1));
        float segment = art.read(pixel).r;
        if (segment > 0.0) along = segment;
        float4 colour = along <= threshold ? mix(quad.min_color, quad.max_color, threshold) : quad.empty_color;
        float alpha = colour.a * texel.g * quad.color.a;
        return float4(colour.rgb * alpha, alpha);
    }
    // (the game's "alpha multiply add", one and one less the source's
    // alpha: the colour's alpha darkens what is behind but doesn't fade
    // the colour, so a crosshair of no alpha is added to the frame)
    if (kind == 1.0) {
        return float4(texel.rgb * quad.color.rgb, texel.a * quad.color.a);
    }
    return float4(texel.rgb * texel.a * quad.color.rgb, texel.a * quad.color.a);
}
