// The bake's path tracer (PathTracer.swift): the light arriving at points
// of the structure, one thread a point. It is engine/crates/bake's
// trace.rs on the GPU, checked against it (BakeTests.swift): rays leave a
// point by the cosine; one that leaves the level sees the sky's dome, one
// that meets a surface takes its emission and, by its albedo, the light it
// receives in turn; the sky's lights are sampled at every point of the
// path within their cones. Compiled with safe maths: the hierarchy's slab
// test relies on infinities.
#include <metal_stdlib>
using namespace metal;

constant float SURFACE_OFFSET = 1e-3;
constant float RAY_LENGTH = 1.0e5;
constant uint SURE_BOUNCES = 2;
constant uint MAX_BOUNCES = 16;
constant uint MAX_PASSES = 8;
// (two a level of opence_bake's deepest hierarchy, 64, and two more)
constant uint STACK = 130;

// opence_bake's Bvh::node_bytes
struct BakeNode {
    packed_float3 min;
    uint first;
    packed_float3 max;
    uint count;
};

// opence_bake's Bvh::triangle_bytes
struct BakeTriangle {
    packed_float3 a;
    uint material;
    packed_float3 ab;
    uint pad0;
    packed_float3 ac;
    uint pad1;
};

// opence_bake's Scene::material_bytes
struct BakeMaterial {
    packed_float3 albedo;
    float coverage;
    packed_float3 emission;
    float pad;
};

// opence_bake's Scene::light_bytes
struct BakeLight {
    packed_float3 towards;
    float cos_half_angle;
    packed_float3 irradiance;
    float pad;
};

// PathTracer.swift's Parameters
struct BakeParameters {
    packed_float3 sky;
    uint light_count;
    uint triangle_count;
    uint samples;
    uint seed;
    float occlusion_distance;
    uint point_count;
    uint pad0;
    uint pad1;
    uint pad2;
};

// PathTracer.swift's BakePoint
struct BakePoint {
    packed_float3 position;
    float pad0;
    packed_float3 normal;
    float pad1;
};

struct BakeScene {
    device const BakeNode *nodes;
    device const BakeTriangle *triangles;
    device const BakeMaterial *materials;
    device const BakeLight *lights;
    constant BakeParameters *parameters;
};

// A PCG hash: the next random state.
static uint bake_hash(uint value) {
    uint state = value * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// A number from 0 up to 1.
static float bake_random(thread uint &state) {
    state = bake_hash(state);
    return float(state >> 8) * (1.0f / 16777216.0f);
}

// Two unit vectors at right angles to `normal` and each other.
static void bake_basis(float3 normal, thread float3 &tangent, thread float3 &bitangent) {
    float sign = copysign(1.0f, normal.z);
    float a = -1.0f / (sign + normal.z);
    float b = normal.x * normal.y * a;
    tangent = float3(1.0f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
    bitangent = float3(b, sign + normal.y * normal.y * a, -normal.y);
}

static float3 bake_cosine_direction(float3 normal, thread uint &state) {
    float u = bake_random(state);
    float v = bake_random(state);
    float radius = sqrt(u);
    float angle = 2.0f * M_PI_F * v;
    float3 tangent, bitangent;
    bake_basis(normal, tangent, bitangent);
    float z = sqrt(max(1.0f - u, 0.0f));
    return normalize(tangent * (radius * cos(angle)) + bitangent * (radius * sin(angle)) + normal * z);
}

static float3 bake_cone_direction(float3 axis, float cos_half_angle, thread uint &state) {
    cos_half_angle = clamp(cos_half_angle, -1.0f, 1.0f);
    if (cos_half_angle >= 1.0f) {
        return axis;
    }
    float u = bake_random(state);
    float v = bake_random(state);
    float cos_theta = 1.0f - u * (1.0f - cos_half_angle);
    float sin_theta = sqrt(max(1.0f - cos_theta * cos_theta, 0.0f));
    float angle = 2.0f * M_PI_F * v;
    float3 tangent, bitangent;
    bake_basis(axis, tangent, bitangent);
    return normalize(tangent * (sin_theta * cos(angle)) + bitangent * (sin_theta * sin(angle)) + axis * cos_theta);
}

static float bake_luminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// Where a ray enters a node's bounds before t_max, if it does (slab test;
// fmin and fmax keep the bound when a ray in a slab's plane gives NaN).
static bool bake_enter(BakeNode node, float3 origin, float3 inverse, float t_max, thread float &near) {
    float3 a = (float3(node.min) - origin) * inverse;
    float3 b = (float3(node.max) - origin) * inverse;
    near = 0.0f;
    float far = t_max;
    for (int axis = 0; axis < 3; axis++) {
        near = fmax(near, fmin(a[axis], b[axis]));
        far = fmin(far, fmax(a[axis], b[axis]));
    }
    return near <= far;
}

// Where the ray meets `triangle`, from either side, before t_max (Möller
// and Trumbore).
static bool bake_intersect(BakeTriangle triangle, float3 origin, float3 direction, float t_max,
                           thread float &t) {
    float3 ab = float3(triangle.ab);
    float3 ac = float3(triangle.ac);
    float3 p = cross(direction, ac);
    float determinant = dot(ab, p);
    if (abs(determinant) < 1e-12f) {
        return false;
    }
    float inverse = 1.0f / determinant;
    float3 to_origin = origin - float3(triangle.a);
    float u = dot(to_origin, p) * inverse;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    float3 q = cross(to_origin, ab);
    float v = dot(direction, q) * inverse;
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    t = dot(ac, q) * inverse;
    return t > 0.0f && t < t_max;
}

// The nearest triangle the ray meets, nearer nodes first.
static bool bake_nearest(BakeScene scene, float3 origin, float3 direction, float t_max,
                         thread float &hit_t, thread uint &hit_triangle) {
    if (scene.parameters->triangle_count == 0) {
        return false;
    }
    float3 inverse = 1.0f / direction;
    uint stack[STACK];
    uint depth = 1;
    stack[0] = 0;
    bool found = false;
    float best = t_max;
    while (depth > 0) {
        depth--;
        BakeNode node = scene.nodes[stack[depth]];
        if (node.count > 0) {
            for (uint index = node.first; index < node.first + node.count; index++) {
                float t;
                if (bake_intersect(scene.triangles[index], origin, direction, best, t)) {
                    best = t;
                    hit_t = t;
                    hit_triangle = index;
                    found = true;
                }
            }
            continue;
        }
        float near_left, near_right;
        bool left = bake_enter(scene.nodes[node.first], origin, inverse, best, near_left);
        bool right = bake_enter(scene.nodes[node.first + 1], origin, inverse, best, near_right);
        if (depth + 2 > STACK) {
            break;
        }
        // (the farther pushed first, so the nearer is visited first)
        if (left && right) {
            if (near_left <= near_right) {
                stack[depth++] = node.first + 1;
                stack[depth++] = node.first;
            } else {
                stack[depth++] = node.first;
                stack[depth++] = node.first + 1;
            }
        } else if (left) {
            stack[depth++] = node.first;
        } else if (right) {
            stack[depth++] = node.first + 1;
        }
    }
    return found;
}

// The first solid surface the ray meets: how far, the point, the normal
// facing back along the ray and the material. A ray passes through an
// alpha-tested surface by the share of it that is open.
static bool bake_meet(BakeScene scene, float3 origin, float3 direction, thread uint &state,
                      thread float &distance, thread float3 &point, thread float3 &normal,
                      thread BakeMaterial &material) {
    float t_max = RAY_LENGTH;
    float travelled = 0.0f;
    for (uint pass = 0; pass < MAX_PASSES; pass++) {
        float t;
        uint index;
        if (!bake_nearest(scene, origin, direction, t_max, t, index)) {
            return false;
        }
        BakeTriangle triangle = scene.triangles[index];
        material = scene.materials[triangle.material];
        point = origin + direction * t;
        normal = cross(float3(triangle.ab), float3(triangle.ac));
        float length_squared = dot(normal, normal);
        normal = length_squared > 0.0f ? normal * rsqrt(length_squared) : normal;
        if (dot(normal, direction) > 0.0f) {
            normal = -normal;
        }
        if (bake_random(state) < material.coverage) {
            distance = travelled + t;
            return true;
        }
        travelled += t;
        origin = point + direction * SURFACE_OFFSET;
        t_max -= t;
    }
    return false;
}

// The sky's lights on a surface at `origin` facing `normal`, each added
// with its sampled direction weighted by its brightness when given.
static float3 bake_direct(BakeScene scene, float3 origin, float3 normal, thread uint &state,
                          thread float3 *weighted) {
    float3 total = float3(0.0f);
    for (uint index = 0; index < scene.parameters->light_count; index++) {
        BakeLight light = scene.lights[index];
        float3 towards = bake_cone_direction(float3(light.towards), light.cos_half_angle, state);
        float facing = dot(normal, towards);
        float3 irradiance = float3(light.irradiance);
        if (facing <= 0.0f || all(irradiance <= 0.0f)) {
            continue;
        }
        float distance;
        float3 point, hit_normal;
        BakeMaterial material;
        if (bake_meet(scene, origin, towards, state, distance, point, hit_normal, material)) {
            continue;
        }
        float3 given = irradiance * facing;
        total += given;
        if (weighted) {
            *weighted += towards * bake_luminance(given);
        }
    }
    return total;
}

// The light coming back along a ray, and how far it went to the first
// surface (RAY_LENGTH when it left the level).
static float3 bake_radiance(BakeScene scene, float3 origin, float3 direction, thread uint &state,
                            thread float &first) {
    float3 light = float3(0.0f);
    float3 throughput = float3(1.0f);
    first = RAY_LENGTH;
    for (uint bounce = 0; bounce < MAX_BOUNCES; bounce++) {
        float distance;
        float3 point, normal;
        BakeMaterial material;
        if (!bake_meet(scene, origin, direction, state, distance, point, normal, material)) {
            light += throughput * float3(scene.parameters->sky);
            break;
        }
        if (bounce == 0) {
            first = distance;
        }
        light += throughput * float3(material.emission);
        origin = point + normal * SURFACE_OFFSET;
        float3 albedo = float3(material.albedo);
        light += throughput * albedo * M_1_PI_F * bake_direct(scene, origin, normal, state, nullptr);
        throughput *= albedo;
        if (bounce >= SURE_BOUNCES) {
            float survive = min(max(max(throughput.x, throughput.y), throughput.z), 1.0f);
            if (bake_random(state) >= survive) {
                break;
            }
            throughput /= survive;
        }
        if (all(throughput <= 0.0f)) {
            break;
        }
        direction = bake_cosine_direction(normal, state);
    }
    return light;
}

// Each point's light: irradiance (red, green, blue) and the open share of
// its hemisphere, and the direction most of the light comes from with how
// much of it does.
kernel void bake_irradiance(device const BakeNode *nodes [[buffer(0)]],
                            device const BakeTriangle *triangles [[buffer(1)]],
                            device const BakeMaterial *materials [[buffer(2)]],
                            device const BakeLight *lights [[buffer(3)]],
                            constant BakeParameters &parameters [[buffer(4)]],
                            device const BakePoint *points [[buffer(5)]],
                            device float4 *irradiance [[buffer(6)]],
                            device float4 *directions [[buffer(7)]],
                            uint index [[thread_position_in_grid]]) {
    if (index >= parameters.point_count) {
        return;
    }
    BakeScene scene = {nodes, triangles, materials, lights, &parameters};
    BakePoint sample_point = points[index];
    float3 normal = float3(sample_point.normal);
    float3 origin = float3(sample_point.position) + normal * SURFACE_OFFSET;
    uint state = bake_hash(index ^ bake_hash(parameters.seed));
    float3 total = float3(0.0f);
    float3 weighted = float3(0.0f);
    uint open = 0;
    uint samples = max(parameters.samples, 1u);
    for (uint sample = 0; sample < samples; sample++) {
        total += bake_direct(scene, origin, normal, state, &weighted);
        float3 direction = bake_cosine_direction(normal, state);
        float first;
        float3 radiance = bake_radiance(scene, origin, direction, state, first);
        if (first > parameters.occlusion_distance) {
            open++;
        }
        float3 light = radiance * M_PI_F;
        total += light;
        weighted += direction * bake_luminance(light);
    }
    float length_squared = dot(weighted, weighted);
    irradiance[index] = float4(total / float(samples), float(open) / float(samples));
    // (how much of the light comes from its direction: the length of the
    // weighted directions over the light's brightness)
    float brightness = bake_luminance(total);
    float directionality = brightness > 0.0f ? min(sqrt(length_squared) / brightness, 1.0f) : 0.0f;
    directions[index] = float4(length_squared > 0.0f ? weighted * rsqrt(length_squared) : float3(0.0f),
                               directionality);
}

// The light seen from each point looking along its "normal": the mean of
// the paths' radiance, for the reflection probes' texels (trace.rs's
// Tracer::radiance_along). The same buffers as bake_irradiance; the
// directions are not written.
kernel void bake_radiance_along(device const BakeNode *nodes [[buffer(0)]],
                                device const BakeTriangle *triangles [[buffer(1)]],
                                device const BakeMaterial *materials [[buffer(2)]],
                                device const BakeLight *lights [[buffer(3)]],
                                constant BakeParameters &parameters [[buffer(4)]],
                                device const BakePoint *points [[buffer(5)]],
                                device float4 *radiance [[buffer(6)]],
                                device float4 *directions [[buffer(7)]],
                                uint index [[thread_position_in_grid]]) {
    if (index >= parameters.point_count) {
        return;
    }
    BakeScene scene = {nodes, triangles, materials, lights, &parameters};
    BakePoint sample_point = points[index];
    float3 origin = float3(sample_point.position);
    float3 direction = float3(sample_point.normal);
    uint state = bake_hash(index ^ bake_hash(parameters.seed));
    float3 total = float3(0.0f);
    uint samples = max(parameters.samples, 1u);
    for (uint sample = 0; sample < samples; sample++) {
        float first;
        total += bake_radiance(scene, origin, direction, state, first);
    }
    radiance[index] = float4(total / float(samples), 1.0f);
}
