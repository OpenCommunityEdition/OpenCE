// The level's and its objects' Metal shaders (LevelRenderer.swift,
// ModelPass.swift), compiled from source when a level loads, and again
// when this file changes in a development build (ShaderSources.swift).
//
// A surface is its base map, its detail and micro detail maps combined as
// its environment shader says, times its light: the lightmap page, bumped
// by its bump map toward the light's incident direction, times its
// material colour, plus its self-illumination; then its specular
// highlight and its cube map's reflection added, as the stock game's
// combiners draw them, in display values (the game's pipeline was not
// linear), into a half-float target that the tone map then brings into
// the drawable.
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
    float3 tangent [[attribute(4)]];
    float3 binormal [[attribute(5)]];
    // toward the light that reaches it, its length how much of the light
    // comes from there (zero: unlit, or no direction)
    float3 incident [[attribute(6)]];
};

// (as LevelRenderer's FrameUniforms)
struct Frame {
    float4x4 view_projection;
    // the view's forward over the fog's range, and where it starts
    float4 fog_plane;
    // the fog's color, and its maximum density
    float4 fog_color;
    // the camera's position, and the mipmap bias: how many levels sharper
    // than its own size a frame drawn small for the temporal scaler samples
    float4 eye;
    // the baked probe grid's first probe and one over its spacing; how
    // many probes each way, and 1 when there are baked probes
    float4 probe_origin;
    float4 probe_counts;
    // the way to the bake's sun, and 1 when there is one; its irradiance
    // (the probes leave its direct light out for the objects to add)
    float4 sun_towards;
    float4 sun_color;
    // the developer's view of the level's surfaces (0 the frame, 1 unlit,
    // 2 their light alone), and 1 when the lightmaps are off (lit flat)
    uint4 debug;
};

constant uint VIEW_ALBEDO = 1;
constant uint VIEW_LIGHT = 2;

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
    // (faded to white), 3 double-multiplied (faded to grey), 4 tinted and
    // added (the colour faded out, the tint to white)
    uint fog_mode;
    // a transparent chicago shader's maps (0: not one), each one's color
    // function, alpha function << 8 and alpha replicate << 16, and its
    // scale and offset
    uint chicago_count;
    uint4 chicago_functions;
    float4 chicago_transforms[4];
    // this frame's animation: the primary self-illumination colour and
    // the plasma's value, the secondary colour, the maps' scroll in u, v
    float4 animation[3];
    // an environment shader's material colour, its plasma's colours
    float4 material_color;
    float4 plasma_on;
    float4 plasma_off;
    // the specular colour along the normal and the brightness; across it
    // and the lightmap's brightness scale
    float4 specular_perpendicular;
    float4 specular_parallel;
    // the micro detail's, bump map's and self-illumination's scales, the
    // reflection's brightness along the normal; then across it
    float4 environment_scales;
    float4 reflection_parallel;
    // the micro detail function, the specular flags, the reflection's
    // kind and the ENVIRONMENT bits (0: not an environment shader)
    uint4 environment;
    // a model shader's MODEL bits, its detail mask, its change colour's
    // source and its self-illumination colour's (0: not a model shader);
    // its reflection's tint and brightness along the normal and across it
    // are specular_perpendicular and specular_parallel, its reflection's
    // falloff and cutoff distances environment_scales.xy
    uint4 model;
    // a glass, meter, plasma or water shader's class (TRANSPARENT_*), its
    // flags and its TRANSPARENT_MAP bits (0: none of them); its tint is
    // material_color, its colours along the normal and across it
    // specular_perpendicular and specular_parallel, its maps' scales and
    // offsets chicago_transforms
    uint4 transparent;
};

constant uint TRANSPARENT_GLASS = 1;
constant uint TRANSPARENT_METER = 2;
constant uint TRANSPARENT_PLASMA = 3;
constant uint TRANSPARENT_WATER = 4;

constant uint MODEL = 1;
constant uint MODEL_REFLECTION = 2;
constant uint DETAIL_AFTER_REFLECTION = 4;

constant uint ENVIRONMENT = 1;
constant uint MICRO = 2;
constant uint BUMP = 4;
constant uint SPECULAR_MASK = 8;
constant uint SELF_ILLUMINATION = 16;
constant uint UNFILTERED = 32;
constant uint REFLECTION = 64;

constant uint OVERBRIGHT = 1;
constant uint EXTRA_SHINY = 2;
constant uint LIGHTMAP_IS_SPECULAR = 4;

constant uint BASE = 1;
constant uint PRIMARY = 2;
constant uint SECONDARY = 4;
constant uint LIGHTMAP = 8;
constant uint UNBAKED = 16;
constant uint BAKED = 32;

struct VertexOut {
    float4 position [[position]];
    float3 world;
    float3 normal;
    float3 tangent;
    float3 binormal;
    float3 incident;
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
    case 1: case 4: color.rgb *= 1 - fog; break;
    case 2: color.rgb = mix(color.rgb, float3(1), fog); break;
    case 3: color.rgb = mix(color.rgb, float3(0.5), fog); break;
    default: color.rgb = mix(color.rgb, frame.fog_color.rgb, fog); break;
    }
    return color;
}

// what the opaque passes write beside the colour, for the screen's passes
// (screen.metal): the surface's shading normal and what it is, the
// reflection it added and the sun's share of its light, and the weight a
// reflection takes on it (the colour the cube map was multiplied by)
struct Opaque {
    float4 color [[color(0)]];
    float4 surface [[color(1)]];
    float4 reflection [[color(2)]];
    float4 reflectance [[color(3)]];
};

// (Opaque's surface.w: what the screen's passes take it for)
constant float SURFACE_LEVEL = 1;
constant float SURFACE_OBJECT = 2;
// (unlit, or the first-person weapon in the nearer half of the depth
// range: left as drawn)
constant float SURFACE_KEPT = 3;

Opaque opaque(float4 color, float4 position, float kind, float3 normal, float sun, float3 reflected,
              float3 weight, float fog) {
    Opaque out;
    out.color = color;
    // (what the fog hides of the reflection, the screen's passes do not
    // bring back)
    float seen = 1 - saturate(fog);
    out.surface = float4(normal, position.z > 0.5 ? SURFACE_KEPT : kind);
    out.reflection = float4(reflected * seen, saturate(sun));
    out.reflectance = float4(weight * seen, 1);
    return out;
}

// the share of `irradiance` the sun's `sun` is
float sun_share(float3 sun, float3 irradiance) {
    return saturate(dot(sun, float3(1.0 / 3)) / max(dot(irradiance, float3(1.0 / 3)), 1e-4));
}

vertex VertexOut level_vertex(VertexIn in [[stage_in]], constant Frame &frame [[buffer(1)]]) {
    VertexOut out;
    out.position = frame.view_projection * float4(in.position, 1);
    out.world = in.position;
    out.normal = in.normal;
    out.tangent = in.tangent;
    out.binormal = in.binormal;
    out.incident = in.incident;
    out.texcoord = in.texcoord;
    out.lightmap = in.lightmap;
    out.fog = fog_amount(frame, in.position);
    return out;
}

// the light on a surface of a BSP shipped with no lightmaps, from its
// normal: the colors the game's no_lightmap mode gives its vertex shader
// (a sky above, a ground below, a light from straight up, and ambient)
float3 unbaked_light(float3 normal) {
    float3 n = normalize(normal);
    float3 sky = float3(0.5, 0.6, 0.6);
    float3 ground = float3(0.1, 0.1, 0.3);
    float3 overhead = float3(0.9, 0.9, 0.8);
    return mix(ground, sky, n.z * 0.5 + 0.5) + overhead * saturate(n.z) * 0.5;
}

// a colour and a detail map's, as a detail function combines them:
// double-biased multiply, multiply or double-biased add
float3 detailed(uint function, float3 color, float3 detail) {
    switch (function) {
    case 1: return color * detail;
    case 2: return saturate(color + 2 * detail - 1);
    default: return saturate(color * 2 * detail);
    }
}

// a surface's base and detail maps, as its material combines them, their
// mipmaps chosen `lod` levels sharper than the drawn size's
float4 surface(float2 texcoord, constant Material &material, texture2d<float> base, texture2d<float> primary,
               texture2d<float> secondary, sampler wrap, float lod) {
    float4 color = float4(1);
    if (material.maps & BASE) {
        color = base.sample(wrap, texcoord * material.base_scale + material.base_offset, bias(lod));
    }
    if (material.alpha_tested != 0 && color.a < 0.5) {
        discard_fragment();
    }
    uint details = material.maps & (PRIMARY | SECONDARY);
    if (details != 0) {
        float4 first = (details & PRIMARY) ? primary.sample(wrap, texcoord * material.primary_scale, bias(lod))
                                           : secondary.sample(wrap, texcoord * material.secondary_scale, bias(lod));
        float4 second = (details & SECONDARY) ? secondary.sample(wrap, texcoord * material.secondary_scale, bias(lod))
                                              : first;
        // normal: the secondary map's alpha mixes them; blended: the
        // base's; either way, at one the primary
        float mask = material.environment_kind == 0 ? second.a : color.a;
        color.rgb = detailed(material.detail_function, color.rgb, mix(second.rgb, first.rgb, mask));
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
float4 chicago(float2 texcoord, constant Material &material, array<texture2d<float>, 4> maps, sampler wrap,
               float lod) {
    float4 current = float4(1);
    for (uint stage = 0; stage < min(material.chicago_count, 4u); stage++) {
        float4 transform = material.chicago_transforms[stage];
        float4 next = maps[stage].sample(wrap, texcoord * transform.xy + transform.zw, bias(lod));
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

// a vector's direction, or `otherwise` for one of no length
float3 direction(float3 v, float3 otherwise) {
    return length_squared(v) > 1e-12 ? normalize(v) : otherwise;
}

// the self-illumination map's light: its red channel the primary colour,
// its green the secondary, its blue the plasma's, where its alpha is near
// the plasma's value (a band that moves with it)
float3 self_illumination(float4 map, constant Material &material) {
    float s = max(0.5 - abs(map.a - material.animation[0].w), 0.0);
    float y = min(4 * s * s, 1.0);
    float x = y * y;
    float band = (2 * x - 1) * (2 * x - 1);
    float3 plasma = material.plasma_on.rgb * band + material.plasma_off.rgb;
    return map.r * material.animation[0].rgb + map.g * material.animation[1].rgb + map.b * plasma;
}

// the bake's irradiance as the stock lightmaps hold their light: over pi
// (the light a white surface sends back, linear), as a display value by
// the sRGB curve carried on past 1 (opence_bake::mood::shown)
float3 baked_shown(float3 irradiance) {
    float3 linear = max(irradiance, 0) / M_PI_F;
    return select(1.055 * pow(linear, 1.0 / 2.4) - 0.055, linear * 12.92, linear <= 0.0031308);
}

// the baked probes' ambient cube at `world` for a surface facing
// `normal`: the irradiance from the six axes, weighed by how much the
// normal faces each, and the share of the sun the probes see
float4 probe_cube(constant Frame &frame, float3 world, float3 normal, sampler clamp,
                  texture3d<float> px, texture3d<float> nx, texture3d<float> py,
                  texture3d<float> ny, texture3d<float> pz, texture3d<float> nz) {
    float3 n = direction(normal, float3(0, 0, 1));
    float3 at = ((world - frame.probe_origin.xyz) * frame.probe_origin.w + 0.5) / frame.probe_counts.xyz;
    float3 weight = n * n;
    float4 x = n.x >= 0 ? px.sample(clamp, at) : nx.sample(clamp, at);
    float4 y = n.y >= 0 ? py.sample(clamp, at) : ny.sample(clamp, at);
    float4 z = n.z >= 0 ? pz.sample(clamp, at) : nz.sample(clamp, at);
    return weight.x * x + weight.y * y + weight.z * z;
}

// the light on a surface facing `normal` at `world` from the baked
// probes around it, shown as the lightmaps' light is (baked_shown)
float3 probe_light(constant Frame &frame, float3 world, float3 normal, sampler clamp,
                   texture3d<float> px, texture3d<float> nx, texture3d<float> py,
                   texture3d<float> ny, texture3d<float> pz, texture3d<float> nz) {
    return baked_shown(probe_cube(frame, world, normal, clamp, px, nx, py, ny, pz, nz).rgb);
}

// (as LevelRenderer's ShadowUniforms) the sun's cascaded shadow maps
struct Shadows {
    // world to each cascade's map: x and y across it, depth from the sun
    float4x4 cascades[4];
    // where each cascade ends, ahead of the camera
    float4 far;
    // the view's forward, and 1 when the sun's shadows are drawn
    float4 forward;
    // the width of a texel of each cascade, in world units
    float4 texel;
};

// how much of the sun reaches `world` on a surface facing `normal` in the
// shadow maps `map` (four taps of their filtered comparison), and how far
// a cascade holds it (zero past the last, fading out over its last tenth)
float2 sun_seen(constant Shadows &shadows, constant Frame &frame, float3 world, float3 normal,
                depth2d_array<float> map) {
    if (shadows.forward.w <= 0) {
        return float2(1, 0);
    }
    float ahead = dot(world - frame.eye.xyz, shadows.forward.xyz);
    uint index = 0;
    while (index < 4 && ahead >= shadows.far[index]) {
        index++;
    }
    if (index >= 4 || ahead < 0) {
        return float2(1, 0);
    }
    constexpr sampler compare(filter::linear, address::clamp_to_edge, compare_func::less_equal);
    // (pushed off the surface by a texel and a half, so that it does not
    // shadow itself)
    float4 at = shadows.cascades[index] * float4(world + normal * (shadows.texel[index] * 1.5), 1);
    float2 uv = float2(at.x * 0.5 + 0.5, 0.5 - at.y * 0.5);
    float2 texel = float2(0.5 / float(max(map.get_width(), 1u)));
    float seen = 0;
    for (int tap = 0; tap < 4; tap++) {
        float2 offset = float2((tap & 1) ? texel.x : -texel.x, (tap & 2) ? texel.y : -texel.y);
        seen += map.sample_compare(compare, uv + offset, index, at.z);
    }
    float last = shadows.far[3];
    return float2(seen * 0.25, saturate((last - ahead) / max(last * 0.1, 1e-4)));
}

// the light on an object's surface: the probes' (the world's light
// bounced to it, and the sky's), and the sun's where the shadow maps (or,
// beyond them, the probes) see it, shown as the lightmaps' light is;
// without a bake the probes are white and there is no sun, an even light
// (w: the sun's share of it)
float4 object_light(constant Frame &frame, float3 world, float3 normal, sampler clamp,
                    texture3d<float> px, texture3d<float> nx, texture3d<float> py,
                    texture3d<float> ny, texture3d<float> pz, texture3d<float> nz,
                    constant Shadows &shadows, depth2d_array<float> shadow_map) {
    float3 n = direction(normal, float3(0, 0, 1));
    float4 cube = probe_cube(frame, world, n, clamp, px, nx, py, ny, pz, nz);
    float3 irradiance = cube.rgb;
    float3 sun = float3(0);
    if (frame.sun_towards.w > 0) {
        float facing = saturate(dot(n, frame.sun_towards.xyz));
        float2 seen = facing > 0 ? sun_seen(shadows, frame, world, n, shadow_map) : float2(0);
        sun = frame.sun_color.rgb * facing * mix(saturate(cube.w), seen.x, seen.y);
        irradiance += sun;
    }
    return float4(baked_shown(irradiance), sun_share(sun, irradiance));
}

// (as LightPass's GPULight) a dynamic light: the flashlight's, a shot's,
// an effect's, the scenery's
struct DynamicLight {
    // where it is, and how far its light reaches
    float4 position_radius;
    // the way it faces, and the cosine its cone ends at (a spot's)
    float4 forward_cutoff;
    // its gel's up, and the cosine its cone starts to fall off at
    float4 up_falloff;
    // its colour (linear), and how far its highlights reach
    float4 color_specular;
    // its flags, its gel's slot and its shadow's cube (none: all ones)
    uint4 flags;
};

// (as LightPass's LightClusterUniforms) the view the cells cut
struct LightCluster {
    // the eye, and the number of lights
    float4 eye_count;
    float4 forward;
    float4 left;
    float4 up;
    // the tangents of half the fields of view across and up, and the
    // cells' nearest and farthest depths
    float4 tangents;
};

constant uint LIGHT_SPOT = 1;
constant uint LIGHT_NO_SPECULAR = 2;
constant uint CLUSTER_X = 16;
constant uint CLUSTER_Y = 9;
constant uint CLUSTER_Z = 24;
constant uint NO_LIGHT = 0xFFFFFFFF;
// (as LightPass.shadowNear) the nearest depth of a light's shadow cube
constant float LIGHT_SHADOW_NEAR = 0.05;

// a display value's light (the sRGB curve undone, carried on past 1),
// and back: the dynamic lights are added to the baked light between them
float3 to_linear(float3 shown) {
    shown = max(shown, 0);
    return select(pow((shown + 0.055) / 1.055, 2.4), shown / 12.92, shown <= 0.04045);
}

float3 to_shown(float3 linear) {
    linear = max(linear, 0);
    return select(1.055 * pow(linear, 1.0 / 2.4) - 0.055, linear * 12.92, linear <= 0.0031308);
}

// the lights' bindings, which the level's and the objects' shaders take
#define DYNAMIC_LIGHT_ARGUMENTS \
    constant DynamicLight *lights [[buffer(5)]], \
    constant uint2 *light_cells [[buffer(6)]], \
    constant ushort *light_indices [[buffer(7)]], \
    constant LightCluster &cluster [[buffer(8)]], \
    array<texturecube<float>, 8> gels [[texture(18)]], \
    depthcube_array<float> light_shadows [[texture(26)]]
#define DYNAMIC_LIGHTS lights, light_cells, light_indices, cluster, gels, light_shadows

// the cell `world` is in (as lights.rs cuts them; NO_LIGHT outside them)
uint light_cell(constant LightCluster &cluster, float3 world) {
    float3 v = world - cluster.eye_count.xyz;
    float x = dot(v, cluster.forward.xyz);
    if (x <= 0) {
        return NO_LIGHT;
    }
    float across = (-dot(v, cluster.left.xyz) / x) / cluster.tangents.x;
    float down = (-dot(v, cluster.up.xyz) / x) / cluster.tangents.y;
    uint column = uint(clamp(floor((across + 1) * 0.5 * float(CLUSTER_X)), 0.0, float(CLUSTER_X - 1)));
    uint row = uint(clamp(floor((down + 1) * 0.5 * float(CLUSTER_Y)), 0.0, float(CLUSTER_Y - 1)));
    uint slice = 0;
    if (x > cluster.tangents.z) {
        float ratio = log(x / cluster.tangents.z) / log(cluster.tangents.w / cluster.tangents.z);
        if (ratio >= 1) {
            return NO_LIGHT;
        }
        slice = min(uint(ratio * float(CLUSTER_Z)), CLUSTER_Z - 1);
    }
    return (slice * CLUSTER_Y + row) * CLUSTER_X + column;
}

// a light's gel the way `direction` leaves it (in its frame)
float3 gel_color(array<texturecube<float>, 8> gels, uint slot, float3 direction) {
    constexpr sampler gel(filter::linear, mip_filter::linear);
    switch (slot) {
    case 0: return gels[0].sample(gel, direction).rgb;
    case 1: return gels[1].sample(gel, direction).rgb;
    case 2: return gels[2].sample(gel, direction).rgb;
    case 3: return gels[3].sample(gel, direction).rgb;
    case 4: return gels[4].sample(gel, direction).rgb;
    case 5: return gels[5].sample(gel, direction).rgb;
    case 6: return gels[6].sample(gel, direction).rgb;
    case 7: return gels[7].sample(gel, direction).rgb;
    default: return float3(1);
    }
}

// how much of a light reaches the point `from_light` away from it, by its
// shadow cube `cube` (drawn as LightPass.faceMatrix draws it)
float light_seen(depthcube_array<float> shadows, uint cube, float3 from_light, float far) {
    constexpr sampler compare(filter::linear, compare_func::less_equal);
    float near = LIGHT_SHADOW_NEAR;
    far = max(far, near * 2);
    float3 extent = abs(from_light);
    float major = max(max(extent.x, extent.y), extent.z);
    if (major <= near) {
        return 1;
    }
    float depth = far / (far - near) - near * far / ((far - near) * major);
    return shadows.sample_compare(compare, from_light, cube, depth);
}

// the dynamic lights at `world` on a surface facing `normal`: the light a
// white surface sends back (linear), and the highlights toward `eye`
// (none with an exponent of 0)
struct DynamicLit {
    float3 diffuse;
    float3 specular;
};

float square(float x) {
    return x * x;
}

DynamicLit dynamic_light(constant DynamicLight *lights, constant uint2 *light_cells,
                         constant ushort *light_indices, constant LightCluster &cluster,
                         array<texturecube<float>, 8> gels, depthcube_array<float> light_shadows,
                         float3 world, float3 normal, float3 eye, float exponent) {
    DynamicLit lit = {float3(0), float3(0)};
    uint count = uint(cluster.eye_count.w);
    if (count == 0) {
        return lit;
    }
    uint cell = light_cell(cluster, world);
    if (cell == NO_LIGHT) {
        return lit;
    }
    uint2 range = light_cells[cell];
    for (uint i = 0; i < range.y; i++) {
        uint index = light_indices[range.x + i];
        if (index >= count) {
            continue;
        }
        DynamicLight light = lights[index];
        float3 to = light.position_radius.xyz - world;
        float distance = length(to);
        float radius = light.position_radius.w;
        float reach = max(radius, light.color_specular.w);
        if (distance >= reach || distance < 1e-4) {
            continue;
        }
        float3 l = to / distance;
        float facing = saturate(dot(normal, l));
        if (facing <= 0) {
            continue;
        }
        float3 color = light.color_specular.rgb;
        if (light.flags.x & LIGHT_SPOT) {
            float along = dot(-l, light.forward_cutoff.xyz);
            float width = max(light.up_falloff.w - light.forward_cutoff.w, 1e-4);
            float cone = saturate((along - light.forward_cutoff.w) / width);
            color *= cone * cone * (3 - 2 * cone);
        }
        if (light.flags.y != NO_LIGHT) {
            float3 forward = light.forward_cutoff.xyz;
            float3 up = light.up_falloff.xyz;
            float3 away = -l;
            color *= gel_color(gels, light.flags.y, float3(dot(away, forward), dot(away, cross(up, forward)),
                                                           dot(away, up)));
        }
        if (light.flags.z != NO_LIGHT) {
            color *= light_seen(light_shadows, light.flags.z, world + normal * 0.02 - light.position_radius.xyz,
                                reach);
        }
        if (distance < radius) {
            lit.diffuse += color * facing * square(saturate(1 - square(distance / radius)));
        }
        float specular_radius = light.color_specular.w;
        if (exponent > 0 && !(light.flags.x & LIGHT_NO_SPECULAR) && distance < specular_radius) {
            float3 half_way = normalize(l + eye);
            lit.specular += color * pow(saturate(dot(normal, half_way)), exponent) * facing
                          * square(saturate(1 - square(distance / specular_radius)));
        }
    }
    return lit;
}

fragment Opaque level_fragment(VertexOut in [[stage_in]],
                               constant Material &material [[buffer(0)]],
                               constant Frame &frame [[buffer(1)]],
                               texture2d<float> base [[texture(0)]],
                               texture2d<float> primary [[texture(1)]],
                               texture2d<float> secondary [[texture(2)]],
                               texture2d<float> lightmap [[texture(3)]],
                               texture2d<float> micro [[texture(4)]],
                               texture2d<float> bump [[texture(5)]],
                               texture2d<float> glow [[texture(6)]],
                               texturecube<float> reflection [[texture(7)]],
                               texture2d<float> baked_directions [[texture(8)]],
                               texture3d<float> probe_px [[texture(9)]],
                               texture3d<float> probe_nx [[texture(10)]],
                               texture3d<float> probe_py [[texture(11)]],
                               texture3d<float> probe_ny [[texture(12)]],
                               texture3d<float> probe_pz [[texture(13)]],
                               texture3d<float> probe_nz [[texture(14)]],
                               depth2d_array<float> level_shadows [[texture(16)]],
                               depth2d_array<float> shadows_map [[texture(17)]],
                               constant Shadows &shadows [[buffer(4)]],
                               DYNAMIC_LIGHT_ARGUMENTS,
                               sampler wrap [[sampler(0)]],
                               sampler clamp [[sampler(1)]],
                               sampler nearest [[sampler(2)]]) {
    if (material.chicago_count != 0) {
        // (unlit, as the game draws them)
        array<texture2d<float>, 4> maps = {base, primary, secondary, lightmap};
        return opaque(fogged(chicago(in.texcoord, material, maps, wrap, frame.eye.w), in.fog, frame, material),
                      in.position, SURFACE_KEPT, float3(0), 0, float3(0), float3(0), in.fog);
    }
    uint bits = material.environment.w;
    // the maps scrolled as the shader animates them
    float2 texcoord = in.texcoord + material.animation[2].xy;
    float lod = frame.eye.w;
    float4 color = surface(texcoord, material, base, primary, secondary, wrap, lod);
    if (bits & MICRO) {
        float4 detail = micro.sample(wrap, texcoord * material.environment_scales.x, bias(lod));
        color.rgb = detailed(material.environment.x, color.rgb, detail.rgb);
    }
    float3 normal = direction(in.normal, float3(0, 0, 1));
    if (frame.debug.x == VIEW_ALBEDO) {
        return opaque(color, in.position, SURFACE_KEPT, normal, 0, float3(0), float3(0), 0);
    }
    // the bumped normal, from the bump map in the surface's frame
    float3 bumped = normal;
    float4 bump_texel = float4(0.5, 0.5, 1, 1);
    // how far the bump map's mipmaps averaged its normals apart: one where
    // they agree, less where a minified map holds several directions
    float agreement = 1;
    if (bits & (BUMP | SPECULAR_MASK)) {
        bump_texel = bump.sample(wrap, texcoord * material.environment_scales.y, bias(lod));
    }
    if (bits & BUMP) {
        float3 n = bump_texel.xyz * 2 - 1;
        agreement = saturate(length(n));
        bumped = direction(in.tangent * n.x + in.binormal * n.y + normal * n.z, normal);
    }
    float3 light = float3(1);
    float3 lightmapped = float3(1);
    // the light's direction, as long as its share of the light (the
    // stock lightmaps' from the vertices, the bake's from its texels)
    float3 incident_light = in.incident;
    // (the sun's share of the light, where the bake knows the sun)
    float sun = 0;
    if (material.maps & LIGHTMAP) {
        if (material.maps & BAKED) {
            float3 irradiance = lightmap.sample(clamp, in.lightmap).rgb;
            // the objects' shadows: the sun's light taken out where the
            // level's own surfaces let it through (as the bake saw) but an
            // object stops it
            float facing = saturate(dot(normal, frame.sun_towards.xyz));
            if (frame.sun_towards.w > 0 && shadows.forward.w > 0 && facing > 0) {
                float2 open = sun_seen(shadows, frame, in.world, normal, level_shadows);
                float2 seen = sun_seen(shadows, frame, in.world, normal, shadows_map);
                float hidden = max(open.x - seen.x, 0.0) * seen.y;
                irradiance = max(irradiance - frame.sun_color.rgb * facing * hidden, float3(0));
                sun = sun_share(frame.sun_color.rgb * facing * (seen.y > 0 ? seen.x : open.x), irradiance);
            }
            lightmapped = baked_shown(irradiance);
            float4 baked = baked_directions.sample(clamp, in.lightmap);
            incident_light = (baked.xyz * 2 - 1) * baked.w;
        } else {
            lightmapped = lightmap.sample(clamp, in.lightmap).rgb;
        }
        light = lightmapped;
    } else if (material.maps & UNBAKED) {
        lightmapped = frame.probe_counts.w > 0
            ? probe_light(frame, in.world, in.normal, clamp, probe_px, probe_nx, probe_py, probe_ny, probe_pz,
                          probe_nz)
            : unbaked_light(in.normal);
        light = lightmapped;
    }
    if (frame.debug.y != 0) {
        light = float3(1);
        lightmapped = float3(1);
        incident_light = float3(0);
    }
    if (frame.debug.x == VIEW_LIGHT) {
        return opaque(float4(light, 1), in.position, SURFACE_KEPT, normal, 0, float3(0), float3(0), 0);
    }
    float3 eye = direction(frame.eye.xyz - in.world, normal);
    if (!(bits & ENVIRONMENT)) {
        DynamicLit lit = dynamic_light(DYNAMIC_LIGHTS, in.world, normal, eye, 0);
        color.rgb *= to_shown(to_linear(light) + lit.diffuse);
        return opaque(fogged(color, in.fog, frame, material), in.position, SURFACE_LEVEL, normal, sun, float3(0),
                      float3(0), in.fog);
    }
    uint flags = material.environment.y;
    DynamicLit lit = dynamic_light(DYNAMIC_LIGHTS, in.world, bumped, eye,
                                   material.specular_perpendicular.w > 0 ? ((flags & EXTRA_SHINY) ? 64.0 : 16.0) : 0);
    // the bump's shading: toward the light by as much as the light has a
    // direction, flat where it has none
    float directed = saturate(length(incident_light));
    float3 incident = direction(incident_light, normal);
    float shading = (1 - directed) + directed * saturate(dot(bumped, incident));
    light = to_shown(to_linear(light * shading) + lit.diffuse) * material.material_color.rgb;
    if (bits & SELF_ILLUMINATION) {
        float2 at = texcoord * material.environment_scales.z;
        float4 map = (bits & UNFILTERED) ? glow.sample(nearest, at, bias(lod)) : glow.sample(wrap, at, bias(lod));
        light += self_illumination(map, material);
    }
    color.rgb *= light;

    // the specular mask: the bump map's alpha where it is one, else the
    // base map's alpha (normal and blended-base-specular) or the bump
    // map's (blended)
    float mask;
    if (bits & SPECULAR_MASK) {
        mask = bump_texel.a;
    } else if (material.environment_kind == 1) {
        mask = (bits & BUMP) ? bump_texel.a : 1.0;
    } else {
        mask = (material.maps & BASE) ? base.sample(wrap, texcoord * material.base_scale + material.base_offset, bias(lod)).a
                                      : 1.0;
    }
    float3 facing = (material.environment.z == 1) ? normal : bumped;
    float along = saturate(dot(facing, eye));
    float fresnel = along * along;
    float3 tint = mix(material.specular_parallel.rgb, material.specular_perpendicular.rgb, fresnel);

    // the dynamic lights' highlights
    if (material.specular_perpendicular.w > 0) {
        float3 specular = lit.specular * tint * material.specular_perpendicular.w * mask;
        color.rgb = to_shown(to_linear(color.rgb) + ((flags & OVERBRIGHT) ? 2 * specular : specular));
    }

    // the lightmap's highlight: its light, from its incident direction,
    // reflected toward the eye
    if (material.specular_perpendicular.w > 0 && directed > 0) {
        float exponent = (flags & EXTRA_SHINY) ? 64.0 : 16.0;
        // (Toksvig's correction: a highlight as broad as the normals the
        // texel averages, so a distant bumped surface does not sparkle)
        exponent *= agreement / max(agreement + exponent * (1 - agreement), 1e-4);
        float highlight = pow(saturate(dot(reflect(-eye, bumped), incident)), exponent) * directed;
        float3 specular = highlight * tint * material.specular_perpendicular.w * mask;
        specular *= (flags & LIGHTMAP_IS_SPECULAR) ? lightmapped : float3(dot(lightmapped, float3(1.0 / 3)));
        color.rgb += (flags & OVERBRIGHT) ? 2 * specular : specular;
    }

    // the cube map's reflection, brighter across the surface than along it,
    // dimmed by the lightmap's darkness as far as its scale says
    float3 reflected = float3(0);
    float3 weight = float3(0);
    if (bits & REFLECTION) {
        float brightness = mix(material.reflection_parallel.x, material.environment_scales.w, fresnel);
        float unmasked = saturate(material.specular_parallel.w);
        float lit = mix(dot(lightmapped, float3(1.0 / 3)), 1.0, unmasked);
        weight = tint * brightness * mask * lit;
        reflected = reflection.sample(clamp, reflect(-eye, facing)).rgb * weight;
        color.rgb += reflected;
    }
    return opaque(fogged(color, in.fog, frame, material), in.position, SURFACE_LEVEL, bumped, sun, reflected, weight,
                  in.fog);
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
    // the reflection probe nearest the object, -1 for none
    int reflection_probe;
};

struct ModelOut {
    float4 position [[position]];
    float3 world;
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
    out.world = world.xyz;
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

// the sun's shadow casters: the level's surfaces and the objects' parts,
// their depth from the sun in a cascade's map (`light`), the alpha-tested
// ones cut out by their base maps
struct ShadowOut {
    float4 position [[position]];
    float2 texcoord;
};

vertex ShadowOut shadow_level_vertex(VertexIn in [[stage_in]], constant float4x4 &light [[buffer(1)]]) {
    ShadowOut out;
    out.position = light * float4(in.position, 1);
    out.texcoord = in.texcoord;
    return out;
}

vertex ShadowOut shadow_model_vertex(ModelIn in [[stage_in]], constant float4x4 &light [[buffer(1)]],
                                     const device float4x4 *skins [[buffer(2)]],
                                     constant ModelObject &object [[buffer(3)]]) {
    uint last = max(object.node_count, 1u) - 1;
    float4x4 skin = skins[object.first_node + min(uint(in.nodes.x), last)] * in.weights.x
                  + skins[object.first_node + min(uint(in.nodes.y), last)] * in.weights.y;
    ShadowOut out;
    out.position = light * (skin * float4(in.position, 1));
    out.texcoord = in.texcoord;
    return out;
}

fragment void shadow_cutout_fragment(ShadowOut in [[stage_in]], constant Material &material [[buffer(0)]],
                                     texture2d<float> base [[texture(0)]], sampler wrap [[sampler(0)]]) {
    if ((material.maps & BASE)
        && base.sample(wrap, in.texcoord * material.base_scale + material.base_offset).a < 0.5) {
        discard_fragment();
    }
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
        return chicago(in.texcoord, material, maps, wrap, 0);
    }
    return surface(in.texcoord, material, base, primary, secondary, wrap, 0);
}

// a multipurpose map's channel the detail mask chooses (1-2 red, 3-4
// green, 5-6 blue, 7-8 alpha; odd ones inverted), one for none
float detail_mask(uint mask, float4 multipurpose) {
    if (mask == 0 || mask > 8) {
        return 1;
    }
    float channel = multipurpose[(mask - 1) / 2];
    return (mask & 1) ? 1 - channel : channel;
}

// lit by the world (object_light), then the model combiner's
// self-illumination, change colour, reflection (its cube by the light of
// the nearest reflection probe) and detail
fragment Opaque model_fragment(ModelOut in [[stage_in]],
                               constant Material &material [[buffer(0)]],
                               constant Frame &frame [[buffer(1)]],
                               constant float4 *change_colors [[buffer(2)]],
                               constant ModelObject &object [[buffer(3)]],
                               texture2d<float> base [[texture(0)]],
                               texture2d<float> primary [[texture(1)]],
                               texture2d<float> secondary [[texture(2)]],
                               texture2d<float> fourth [[texture(3)]],
                               texturecube<float> reflection [[texture(4)]],
                               texture3d<float> probe_px [[texture(9)]],
                               texture3d<float> probe_nx [[texture(10)]],
                               texture3d<float> probe_py [[texture(11)]],
                               texture3d<float> probe_ny [[texture(12)]],
                               texture3d<float> probe_pz [[texture(13)]],
                               texture3d<float> probe_nz [[texture(14)]],
                               texturecube_array<float> reflections [[texture(15)]],
                               depth2d_array<float> shadows_map [[texture(17)]],
                               constant Shadows &shadows [[buffer(4)]],
                               DYNAMIC_LIGHT_ARGUMENTS,
                               sampler wrap [[sampler(0)]],
                               sampler clamp [[sampler(1)]]) {
    if (material.chicago_count != 0) {
        array<texture2d<float>, 4> maps = {base, primary, secondary, fourth};
        return opaque(fogged(chicago(in.texcoord, material, maps, wrap, frame.eye.w), in.fog, frame, material),
                      in.position, SURFACE_KEPT, float3(0), 0, float3(0), float3(0), in.fog);
    }
    float3 normal = direction(in.normal, float3(0, 0, 1));
    float4 world_light = object_light(frame, in.world, normal, clamp, probe_px, probe_nx, probe_py, probe_ny,
                                      probe_pz, probe_nz, shadows, shadows_map);
    float3 light = world_light.rgb;
    DynamicLit lit = dynamic_light(DYNAMIC_LIGHTS, in.world, normal, float3(0, 0, 1), 0);
    light = to_shown(to_linear(light) + lit.diffuse);
    uint bits = material.model.x;
    if (!(bits & MODEL)) {
        float4 color = surface(in.texcoord, material, base, primary, secondary, wrap, frame.eye.w);
        color.rgb *= light;
        return opaque(fogged(color, in.fog, frame, material), in.position, SURFACE_OBJECT, normal, world_light.w,
                      float3(0), float3(0), in.fog);
    }
    float lod = frame.eye.w;
    // the maps scrolled as the shader animates them
    float2 texcoord = in.texcoord + material.animation[2].xy;
    float4 color = float4(1);
    if (material.maps & BASE) {
        color = base.sample(wrap, texcoord * material.base_scale + material.base_offset, bias(lod));
    }
    if (material.alpha_tested != 0 && color.a < 0.5) {
        discard_fragment();
    }
    // the multipurpose map: red the reflection's mask, green the
    // self-illumination's, blue the change colour's, alpha auxiliary
    float4 multipurpose = secondary.sample(wrap, texcoord * material.base_scale + material.base_offset, bias(lod));
    float3 self_color = material.animation[0].rgb;
    uint self_source = material.model.w;
    if (self_source >= 1 && self_source <= 4) {
        self_color *= change_colors[self_source - 1].rgb;
    }
    light += multipurpose.g * self_color;
    uint change_source = material.model.z;
    if (change_source >= 1 && change_source <= 4) {
        light *= mix(float3(1), change_colors[change_source - 1].rgb, multipurpose.b);
    }
    // the detail map, toward its neutral value where the mask hides it
    float neutral = material.detail_function == 1 ? 1.0 : 0.5;
    float3 detail = float3(neutral);
    if (material.maps & PRIMARY) {
        float3 sampled = primary.sample(wrap, texcoord * material.primary_scale, bias(lod)).rgb;
        detail = mix(detail, sampled, detail_mask(material.model.y, multipurpose));
    }
    // the cube map's reflection, by the fresnel term between its tints,
    // faded in from its cutoff distance to its falloff distance
    float3 reflected = float3(0);
    float3 weight = float3(0);
    if (bits & MODEL_REFLECTION) {
        float3 toward = frame.eye.xyz - in.world;
        float3 eye = direction(toward, normal);
        float along = saturate(dot(normal, eye));
        float fresnel = along * along;
        float4 tint = mix(material.specular_parallel, material.specular_perpendicular, fresnel);
        float falloff = material.environment_scales.x;
        float cutoff = material.environment_scales.y;
        float fraction = 1;
        if (cutoff > 0 && falloff != cutoff) {
            fraction = saturate((length(toward) - cutoff) / (falloff - cutoff));
        }
        float3 toward_reflected = reflect(-eye, normal);
        weight = tint.rgb * (multipurpose.r * tint.a * fraction);
        reflected = reflection.sample(clamp, toward_reflected).rgb * weight;
        // the radiance the probe sees that way, shown as a white surface
        // lit by it would be (pi times it is the irradiance of that)
        if (object.reflection_probe >= 0) {
            float3 radiance = reflections.sample(clamp, toward_reflected, uint(object.reflection_probe)).rgb;
            reflected *= baked_shown(radiance * M_PI_F);
        }
    }
    // (what the reflection adds after the detail map, which scales it)
    float3 detail_scale = float3(1);
    if (bits & DETAIL_AFTER_REFLECTION) {
        float3 before = color.rgb * light;
        color.rgb = detailed(material.detail_function, before + reflected, detail);
        detail_scale = color.rgb / max(before + reflected, float3(1e-4));
    } else {
        color.rgb = detailed(material.detail_function, color.rgb, detail) * light + reflected;
    }
    return opaque(fogged(color, in.fog, frame, material), in.position, SURFACE_OBJECT, normal, world_light.w,
                  reflected * detail_scale, weight * detail_scale, in.fog);
}

// a transparent shader's output: what is added to the frame, and what
// the frame is multiplied by first (the blend's second source)
struct Tinted {
    float4 color [[color(0), index(0)]];
    float4 tint [[color(0), index(1)]];
};

// one of a transparent shader's maps, at its slot's scale and offset
float4 transparent_map(texture2d<float> map, uint slot, float2 texcoord, constant Material &material,
                       sampler wrap, float lod, float4 otherwise) {
    if (!((material.transparent.z >> slot) & 1)) {
        return otherwise;
    }
    float4 transform = material.chicago_transforms[slot];
    return map.sample(wrap, texcoord * transform.xy + transform.zw, bias(lod));
}

// glass, a meter, plasma or water, as the original's passes for it
// combine: the frame behind tinted, then a reflection or a glow added
Tinted transparent(float2 texcoord, float3 normal, float3 world, float fog, constant Frame &frame,
                   constant Material &material, array<texture2d<float>, 4> maps, texturecube<float> cube,
                   sampler wrap, sampler clamp) {
    float lod = frame.eye.w;
    float3 eye = direction(frame.eye.xyz - world, normal);
    // (both faces drawn: the normal toward the eye)
    float3 n = dot(normal, eye) < 0 ? -normal : normal;
    float facing = saturate(dot(n, eye));
    float4 fresnel = mix(material.specular_parallel, material.specular_perpendicular, facing);
    float3 reflected = cube.sample(clamp, reflect(-eye, n)).rgb;
    uint flags = material.transparent.y;
    Tinted out;
    out.color = float4(0, 0, 0, 1);
    out.tint = float4(1);
    switch (material.transparent.x) {
    case TRANSPARENT_GLASS: {
        // the diffuse map over; under it the frame tinted and the reflection
        float4 diffuse = transparent_map(maps[0], 0, texcoord, material, wrap, lod, float4(0));
        float4 detail = transparent_map(maps[3], 3, texcoord, material, wrap, lod, float4(0.5));
        diffuse.rgb = saturate(diffuse.rgb * 2 * detail.rgb);
        float3 tint = material.material_color.rgb
                    * transparent_map(maps[1], 1, texcoord, material, wrap, lod, float4(1)).rgb;
        float mask = 1;
        if (flags & 8) {
            mask = transparent_map(maps[2], 2, texcoord, material, wrap, lod, float4(1)).a;
        }
        float under = 1 - diffuse.a;
        out.tint.rgb = tint * under;
        out.color.rgb = reflected * fresnel.rgb * fresnel.a * mask * under + diffuse.rgb * diffuse.a;
        break;
    }
    case TRANSPARENT_METER: {
        // (at its full value: the objects' functions do not reach it yet)
        float4 map = transparent_map(maps[0], 0, texcoord, material, wrap, lod, float4(1));
        float3 gradient = mix(material.specular_perpendicular.rgb, material.specular_parallel.rgb, map.a);
        out.color.rgb = gradient * material.material_color.rgb * map.rgb * material.specular_perpendicular.a;
        break;
    }
    case TRANSPARENT_PLASMA: {
        // its two noise maps scrolled along their directions, their
        // difference's nearness to zero the glow
        float4 first = transparent_map(maps[0], 0, texcoord + material.animation[0].xy, material, wrap, lod,
                                       float4(0.5));
        float4 second = transparent_map(maps[1], 1, texcoord + material.animation[1].xy, material, wrap, lod,
                                        float4(0.5));
        float intensity = (material.transparent.z & 3) ? saturate(1 - 2 * abs(first.a - second.a)) : 0.5;
        out.color.rgb = fresnel.rgb * fresnel.a * intensity;
        break;
    }
    case TRANSPARENT_WATER: {
        float4 base = transparent_map(maps[0], 0, texcoord, material, wrap, lod, float4(1));
        if (flags & 2) {
            out.tint.rgb = base.rgb;
        }
        float mask = (flags & 1) ? base.a : 1;
        out.color.rgb = reflected * fresnel.rgb * fresnel.a * mask;
        break;
    }
    default: break;
    }
    out.color.rgb *= 1 - fog;
    out.tint.rgb = mix(out.tint.rgb, float3(1), fog);
    return out;
}

fragment Tinted transparent_fragment(VertexOut in [[stage_in]],
                                     constant Material &material [[buffer(0)]],
                                     constant Frame &frame [[buffer(1)]],
                                     texture2d<float> first [[texture(0)]],
                                     texture2d<float> second [[texture(1)]],
                                     texture2d<float> third [[texture(2)]],
                                     texture2d<float> fourth [[texture(3)]],
                                     texturecube<float> reflection [[texture(7)]],
                                     sampler wrap [[sampler(0)]],
                                     sampler clamp [[sampler(1)]]) {
    array<texture2d<float>, 4> maps = {first, second, third, fourth};
    return transparent(in.texcoord, direction(in.normal, float3(0, 0, 1)), in.world, in.fog, frame, material,
                       maps, reflection, wrap, clamp);
}

fragment Tinted model_transparent_fragment(ModelOut in [[stage_in]],
                                           constant Material &material [[buffer(0)]],
                                           constant Frame &frame [[buffer(1)]],
                                           texture2d<float> first [[texture(0)]],
                                           texture2d<float> second [[texture(1)]],
                                           texture2d<float> third [[texture(2)]],
                                           texture2d<float> fourth [[texture(3)]],
                                           texturecube<float> reflection [[texture(4)]],
                                           sampler wrap [[sampler(0)]],
                                           sampler clamp [[sampler(1)]]) {
    array<texture2d<float>, 4> maps = {first, second, third, fourth};
    return transparent(in.texcoord, direction(in.normal, float3(0, 0, 1)), in.world, in.fog, frame, material,
                       maps, reflection, wrap, clamp);
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
// (as LevelRenderer's ToneUniforms)
struct Tone {
    // the exposure's multiplier, and the bloom's strength
    float exposure;
    float bloom;
    // 0 the frame, 1 its brightness, 2 the bloom alone
    uint view;
};

fragment float4 tone_fragment(ToneOut in [[stage_in]], texture2d<float> frame [[texture(0)]],
                              texture2d<float> bloom [[texture(1)]], constant Tone &tone [[buffer(0)]]) {
    constexpr sampler nearest(filter::nearest, address::clamp_to_edge);
    constexpr sampler linear(filter::linear, address::clamp_to_edge);
    float3 bloomed = max(bloom.sample(linear, in.texcoord).rgb, 0);
    if (tone.view == 2) {
        return float4(bloomed, 1);
    }
    float3 color = max(frame.sample(nearest, in.texcoord).rgb, 0);
    if (tone.view == 1) {
        float brightness = dot(color, float3(0.2126, 0.7152, 0.0722)) * tone.exposure;
        return float4(float3(saturate(log2(max(brightness, 1e-4)) / 8 + 0.5)), 1);
    }
    color = color * tone.exposure + bloomed * tone.bloom;
    float3 over = max(color - 0.8, 0);
    float3 eased = 0.8 + 0.2 * (1 - exp(-over / 0.2));
    return float4(select(color, eased, color > 0.8), 1);
}
