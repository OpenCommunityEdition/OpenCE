// The Halo UI's quads over the finished frame (UiPass.swift): the widgets'
// bitmaps and their text, whose glyphs are signed distance fields in one
// atlas (engine/crates/ffi/src/ui.rs).

#include <metal_stdlib>
using namespace metal;

struct UiInstance {
    /// Left, top, right and bottom, in clip space.
    float4 rect;
    /// u0, v0, u1, v1.
    float4 uv;
    /// The colour and its opacity, not premultiplied.
    float4 color;
    /// x: 1 for a glyph.
    float4 parameters;
};

struct UiVarying {
    float4 position [[position]];
    float2 uv;
    uint instance [[flat]];
};

vertex UiVarying ui_vertex(uint vertex_id [[vertex_id]], uint instance [[instance_id]],
                           const device UiInstance *instances [[buffer(0)]]) {
    // (two triangles: 0 1 2, 2 1 3 of the corners left top, right top,
    // left bottom, right bottom)
    const uint corners[6] = {0, 1, 2, 2, 1, 3};
    uint corner = corners[vertex_id];
    UiInstance quad = instances[instance];
    bool right = (corner & 1) != 0, bottom = (corner & 2) != 0;
    UiVarying out;
    out.position = float4(right ? quad.rect.z : quad.rect.x, bottom ? quad.rect.w : quad.rect.y, 0, 1);
    out.uv = float2(right ? quad.uv.z : quad.uv.x, bottom ? quad.uv.w : quad.uv.y);
    out.instance = instance;
    return out;
}

// premultiplied out
fragment float4 ui_fragment(UiVarying in [[stage_in]], texture2d<float> art [[texture(0)]],
                            sampler linear [[sampler(0)]],
                            const device UiInstance *instances [[buffer(0)]]) {
    UiInstance quad = instances[in.instance];
    float4 texel = art.sample(linear, in.uv);
    if (quad.parameters.x > 0.5) {
        // (128 on the edge, more inside: the distance in the field's
        // texels, its edge a pixel wide however large it is drawn)
        float distance = texel.r - 128.0 / 255.0;
        float width = max(fwidth(distance), 1e-5);
        float coverage = clamp(distance / width + 0.5, 0.0, 1.0);
        float alpha = coverage * quad.color.a;
        return float4(quad.color.rgb * alpha, alpha);
    }
    float alpha = texel.a * quad.color.a;
    return float4(texel.rgb * quad.color.rgb * alpha, alpha);
}
