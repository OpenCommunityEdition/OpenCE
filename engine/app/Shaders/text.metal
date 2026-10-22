// The monospace text pass's glyph quads over the finished frame (TextPass.swift).

#include <metal_stdlib>
using namespace metal;

struct TextInstance {
    float4 rect;
    float4 uv;
    float4 color;
};

struct TextVarying {
    float4 position [[position]];
    float2 uv;
    float4 color [[flat]];
};

vertex TextVarying text_vertex(uint vertex_id [[vertex_id]], uint instance [[instance_id]],
                               const device TextInstance *instances [[buffer(0)]]) {
    const uint corners[6] = {0, 1, 2, 2, 1, 3};
    uint corner = corners[vertex_id];
    TextInstance quad = instances[instance];
    bool right = (corner & 1) != 0, bottom = (corner & 2) != 0;
    TextVarying out;
    out.position = float4(right ? quad.rect.z : quad.rect.x, bottom ? quad.rect.w : quad.rect.y, 0, 1);
    out.uv = float2(right ? quad.uv.z : quad.uv.x, bottom ? quad.uv.w : quad.uv.y);
    out.color = quad.color;
    return out;
}

// premultiplied out
fragment float4 text_fragment(TextVarying in [[stage_in]], texture2d<float> atlas [[texture(0)]],
                              sampler nearest [[sampler(0)]]) {
    float alpha = atlas.sample(nearest, in.uv).r * in.color.a;
    return float4(in.color.rgb * alpha, alpha);
}
