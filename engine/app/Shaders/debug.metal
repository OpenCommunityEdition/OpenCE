// The debug drawing's lines in the level's frame (DebugPass.swift).

#include <metal_stdlib>
using namespace metal;

struct DebugVarying {
    float4 position [[position]];
    float4 color;
};

// (a vertex is its point and its packed colour's bits)
vertex DebugVarying debug_vertex(uint vertex_id [[vertex_id]], const device float4 *vertices [[buffer(0)]],
                                 constant float4x4 &view_projection [[buffer(1)]]) {
    float4 vertex_in = vertices[vertex_id];
    uint packed = as_type<uint>(vertex_in.w);
    DebugVarying out;
    out.position = view_projection * float4(vertex_in.xyz, 1);
    out.color = float4((packed >> 24) & 0xff, (packed >> 16) & 0xff, (packed >> 8) & 0xff, packed & 0xff) / 255.0;
    return out;
}

fragment float4 debug_fragment(DebugVarying in [[stage_in]]) {
    return float4(in.color.rgb, 1);
}
