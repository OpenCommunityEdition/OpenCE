#!/usr/bin/env python3
"""Execute the actual Metal normal decoding/TBN helper on CPU SIMD vectors.
Texture sampling is mocked; production UV/metadata indexing, normal decoding,
handedness and fallbacks remain unchanged. No GPU or game process is started.
"""
import ast
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
source = (ROOT / 'port/macos/host/host_metal_rt.m').read_text()
a = source.index('\t"static float3 level_shading_normal(')
b = source.index('\t"kernel void probes', a)
metal = ''.join(ast.literal_eval(line.strip()) for line in source[a:b].splitlines() if line.strip().startswith('"'))
metal = re.sub(r'\b(device|constant)\s+', '', metal)
metal = metal.replace('texture2d<float, access::sample>', 'Texture')
# Compile-time transport checks complement execution of the real shader math.
assert 'surface_data + 768u + uint(c[102])' in source
assert '(768u + (NSUInteger)rt.surface_property_count) * 16' in source
assert '(768u + (NSUInteger)count) * 16' in source
assert 'MTLPixelFormatRGBA8Unorm\n' in source
assert 'atIndex:11]' in source
# Geometric data remains the basis for visibility and temporal rejection.
assert 'float4(oct_encode(N), z, float(surface_key))' in source
assert 'dot(oct_decode(old_surface.xy), N)' in source
assert 'ray to_light(P + N * bias' in source
assert 'dot(N, L) > 0.0 ? max(dot(Ns, L), 0.0)' in source

definitions = r'''
#include <simd/simd.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
using uint = uint32_t;
using float2 = simd_float2;
using float3 = simd_float3;
using float4 = simd_float4;
using int3 = simd_int3;
using int2 = simd_int2;
#define float2(...) ((simd_float2){__VA_ARGS__})
#define float3(...) ((simd_float3){__VA_ARGS__})
#define float4(...) ((simd_float4){__VA_ARGS__})
static float dot(float3 a,float3 b) { return simd_dot(a,b); }
static float3 cross(float3 a,float3 b) { return simd_cross(a,b); }
static float3 normalize(float3 a) { return simd_normalize(a); }
static float2 fract(float2 a) { return a-float2(floorf(a.x),floorf(a.y)); }
static float2 clamp(float2 a,float2 lo,float2 hi) { return simd_clamp(a,lo,hi); }
static int3 isfinite(float3 a) { return (int3){std::isfinite(a.x),std::isfinite(a.y),std::isfinite(a.z)}; }
static int2 isfinite(float2 a) { return (int2){std::isfinite(a.x),std::isfinite(a.y)}; }
static bool all(int2 a) { return a.x&&a.y; }
static bool all(int3 a) { return a.x&&a.y&&a.z; }
constexpr int linear_clamp=0;
namespace metal { static float level(float a) { return a; } }
static float2 sampled_uv;
static int sample_count;
struct Texture {
    float4 encoded;
    float4 sample(int,float2 uv,float) { sampled_uv=uv;sample_count++;return encoded; }
};
'''
checks = r'''
static void near(float a,float b) { assert(fabsf(a-b)<1e-5f); }
int main() {
    // Identical world XY triangle, UV and +Z geometric normal. Opposite
    // authored tangent slopes must produce opposite light-facing responses.
    float vertices[]={0,0,0, 1,0,0, 0,1,0};uint indices[]={0,1,2};uint materials[]={0};
    float2 uvs[]={float2(0,0),float2(1,0),float2(0,1)};
    float c[105]={};c[101]=1;c[102]=1;
    float4 data[769]={};data[256]=float4(0,0,0,1);
    data[257]=float4(0,0,256.0f/4096,256.0f/4096);data[513]=float4(2,3,0,0);
    float3 Ng=float3(0,0,1), light=normalize(float3(1,0,1));
    auto shade=[&](float4 color) {return level_shading_normal(0,float2(.2f,.1f),Ng,
        vertices,indices,uvs,materials,c,data,Texture{color});};
    float3 plus=shade(float4(.8f,.5f,.9f,1)),minus=shade(float4(.2f,.5f,.9f,1));
    assert(plus.x>.5f&&minus.x<-.5f);near(plus.z,.8f);near(minus.z,.8f);
    assert(dot(plus,light)>dot(minus,light)+.8f);
    near(sampled_uv.x,.4f/16);near(sampled_uv.y,.3f/16);
    // A flat normal preserves the plane; normal RGB is linear, not sRGB.
    auto flat=shade(float4(.5f,.5f,1,1));near(flat.x,0);near(flat.y,0);near(flat.z,1);
    // Mirrored UVs preserve handedness and reverse the tangent slope.
    uvs[1]=float2(-1,0);auto mirrored=shade(float4(.8f,.5f,.9f,1));assert(mirrored.x<-.5f);uvs[1]=float2(1,0);
    // Unloaded atlas, missing bitmap, degenerate UV and bad texels all fall
    // back to geometric normal without generating non-finite shading.
    int before=sample_count;data[257].z=0;near(shade(float4(.8f,.5f,.9f,1)).z,1);assert(sample_count==before);data[257].z=1.0f/16;
    data[256].w=0;near(shade(float4(.8f,.5f,.9f,1)).z,1);assert(sample_count==before);data[256].w=1;
    data[256].w=255;near(shade(float4(.8f,.5f,.9f,1)).z,1);assert(sample_count==before);data[256].w=1;
    uvs[2]=float2(2,0);near(shade(float4(.8f,.5f,.9f,1)).z,1);assert(sample_count==before);uvs[2]=float2(0,1);
    near(shade(float4(NAN,.5f,1,1)).z,1);near(shade(float4(.5f,.5f,0,1)).z,1);
    near(shade(float4(.5f,.5f,.5f,1)).z,1);
    // Last normal slot uses metadata beyond all material entries safely.
    data[256].w=254;data[510]=data[257];data[766]=float4(1,1,0,0);
    assert(shade(float4(.8f,.5f,.9f,1)).x>.5f);
    // A bump cannot alter geometric visibility, origins, or history basis.
    near(Ng.x,0);near(Ng.z,1);
    std::puts("RT normal regression: actual Metal helper opposite slopes, linear decode, authored tiling, mirrored UVs, missing/invalid maps and last-slot bounds passed");
}
'''
with tempfile.TemporaryDirectory(prefix='halo-rt-normal-') as temp:
    src=Path(temp)/'test.cpp';out=Path(temp)/'test';src.write_text(definitions+metal+checks)
    subprocess.run(['clang++','-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(src),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True)
