// physics.cpp - the host's physics world on Jolt Physics (physics.h).
//
// One PhysicsSystem a world, stepped by the calling thread and a few of
// Jolt's worker threads (its thread pool). Each island of touching bodies is
// solved on its own job, and Jolt sorts contacts and constraints before it
// solves them (mDeterministicSimulation, on), so a step's result does not
// depend on the threads: a run repeats exactly. Bodies are of two layers: the level's mesh
// (static) and everything that moves (ragdoll parts, debris, kinematic
// bodies), which collides with the level and with itself. A ragdoll's parts
// share a collision group whose table keeps joined (and, at rest,
// overlapping) parts from colliding, until their joint breaks.

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Skeleton/Skeleton.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

#include "physics.h"

using namespace JPH;

namespace {

constexpr ObjectLayer LAYER_STATIC = 0;
constexpr ObjectLayer LAYER_MOVING = 1;
constexpr uint32 LAYER_COUNT = 2;

// what a body's user data says it is
constexpr uint64 KIND_LEVEL = 1;
constexpr uint64 KIND_BODY = 2;
constexpr uint64 KIND_RAGDOLL = 3;

// limits on what a caller may ask for
constexpr uint32 BODIES_MAX = 65536;
constexpr uint32 COLLISION_STEPS_MAX = 8;
constexpr uint32 LEVEL_TRIANGLES_MAX = 1u << 21;
constexpr float SIZE_MAX_METRES = 100.0f;
constexpr float MASS_MAX = 100000.0f;
constexpr float STEP_MAX = 1.0f;
// a single body's turning slowed by this (1/s), Jolt's stand-in for rolling
// resistance
constexpr float BODY_ANGULAR_DAMPING = 2.0f;
// how far bodies may sink into one another, and how near before they touch
constexpr float PENETRATION_SLOP_METRES = 0.005f;
constexpr float SPECULATIVE_CONTACT_METRES = 0.02f;
constexpr uint32 RAGDOLLS_MAX = 1024;
// a joint's motor, at most this torque (N m) a kilogram of its part
constexpr float MOTOR_TORQUE_PER_KILOGRAM = 40.0f;
// a limp joint's friction, the same way: enough that every stock biped's
// body comes to rest on flat ground within 8 seconds and sleeps
// (tools/test_ragdoll_physics.py); at 0.4 a jackal's shield arm and a flood
// elite's limbs crept on for longer
constexpr float FRICTION_TORQUE_PER_KILOGRAM = 2.0f;
// a ragdoll's solver iterations: a chain of joints needs more than Jolt's
// defaults (10 and 2) to stay within its limits as it hits the ground
constexpr uint RAGDOLL_VELOCITY_STEPS = 12;
constexpr uint RAGDOLL_POSITION_STEPS = 6;
// the step's worker threads, besides the calling one: thirty ragdolls
// tumbling at once took 6 ms a step on one thread of an M1 Max
constexpr uint PHYSICS_WORKER_THREADS = 3;

class BroadPhaseLayers final : public BroadPhaseLayerInterface {
public:
    uint GetNumBroadPhaseLayers() const override { return LAYER_COUNT; }
    BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer layer) const override
    {
        return BroadPhaseLayer(layer == LAYER_STATIC ? 0 : 1);
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *GetBroadPhaseLayerName(BroadPhaseLayer layer) const override
    {
        return layer.GetValue() == 0 ? "static" : "moving";
    }
#endif
};

class ObjectVsBroadPhase final : public ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(ObjectLayer layer, BroadPhaseLayer broad_phase) const override
    {
        return layer == LAYER_MOVING || broad_phase.GetValue() != 0;
    }
};

class ObjectPairs final : public ObjectLayerPairFilter {
public:
    bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override
    {
        return a == LAYER_MOVING || b == LAYER_MOVING;
    }
};

struct RagdollSlot {
    Ref<RagdollSettings> settings;
    Ref<Ragdoll> ragdoll;
    GroupFilterTable *filter = nullptr;
    // counted up each time the slot is used, so that an old handle names nothing
    uint32 generation = 0;
};

struct CharacterSlot {
    Ref<CharacterVirtual> character;
    float radius = 0.0f;
    float step_up = 0.0f;
    float stick_down = 0.0f;
    uint32_t layers = 0;
    uint32 generation = 0;
};

constexpr uint32 CHARACTERS_MAX = 1024;

void trace(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    std::fputs("halo_host: physics: ", stderr);
    std::vfprintf(stderr, format, arguments);
    std::fputc('\n', stderr);
    va_end(arguments);
}

void start_jolt()
{
    static std::once_flag once;
    std::call_once(once, [] {
        RegisterDefaultAllocator();
        Trace = trace;
        Factory::sInstance = new Factory();
        RegisterTypes();
    });
}

bool finite(const float *values, int count)
{
    for (int i = 0; i < count; i++)
        if (!std::isfinite(values[i]))
            return false;
    return true;
}

// a pose's position, and its rotation made unit; false for a pose that is
// not one (not finite, or a rotation far from unit)
bool read_pose(const physics_pose *pose, RVec3 &position, Quat &rotation)
{
    if (pose == nullptr || !finite(pose->position, 3) || !finite(pose->rotation, 4))
        return false;
    Vec4 q(pose->rotation[0], pose->rotation[1], pose->rotation[2], pose->rotation[3]);
    float length = q.Length();
    if (!(length > 0.9f && length < 1.1f))
        return false;
    position = RVec3(pose->position[0], pose->position[1], pose->position[2]);
    rotation = Quat(q / length);
    return true;
}

void write_pose(RVec3Arg position, QuatArg rotation, physics_pose *pose)
{
    pose->position[0] = float(position.GetX());
    pose->position[1] = float(position.GetY());
    pose->position[2] = float(position.GetZ());
    pose->rotation[0] = rotation.GetX();
    pose->rotation[1] = rotation.GetY();
    pose->rotation[2] = rotation.GetZ();
    pose->rotation[3] = rotation.GetW();
}

bool read_vector(const float *values, Vec3 &vector, float limit)
{
    if (values == nullptr || !finite(values, 3))
        return false;
    vector = Vec3(values[0], values[1], values[2]);
    return vector.LengthSq() <= limit * limit;
}

bool positive_size(float size) { return size > 0.0f && size <= SIZE_MAX_METRES; }

RefConst<Shape> make_shape(const physics_shape *shape)
{
    if (shape == nullptr)
        return nullptr;
    RVec3 offset_position;
    Quat offset_rotation;
    if (!read_pose(&shape->offset, offset_position, offset_rotation) || !finite(shape->size, 3))
        return nullptr;
    Ref<ShapeSettings> settings;
    Quat axis = Quat::sIdentity();
    switch (shape->type) {
    case PHYSICS_SHAPE_SPHERE:
        if (!positive_size(shape->size[0]))
            return nullptr;
        settings = new SphereShapeSettings(shape->size[0]);
        break;
    case PHYSICS_SHAPE_CAPSULE:
        if (!positive_size(shape->size[0]) || !positive_size(shape->size[1]))
            return nullptr;
        settings = new CapsuleShapeSettings(shape->size[1], shape->size[0]);
        // Jolt's capsules lie along y; ours along x
        axis = Quat::sRotation(Vec3::sAxisZ(), -0.5f * JPH_PI);
        break;
    case PHYSICS_SHAPE_BOX: {
        if (!positive_size(shape->size[0]) || !positive_size(shape->size[1]) || !positive_size(shape->size[2]))
            return nullptr;
        Vec3 half(shape->size[0], shape->size[1], shape->size[2]);
        settings = new BoxShapeSettings(half, std::min(cDefaultConvexRadius, 0.5f * half.ReduceMin()));
        break;
    }
    default:
        return nullptr;
    }
    Quat rotation = offset_rotation * axis;
    if (!offset_position.IsNearZero() || !rotation.IsClose(Quat::sIdentity()))
        settings = new RotatedTranslatedShapeSettings(Vec3(offset_position), rotation, settings);
    ShapeSettings::ShapeResult result = settings->Create();
    if (result.HasError())
        return nullptr;
    return result.Get();
}

bool read_material(const physics_material *material, float &friction, float &restitution)
{
    friction = 0.6f;
    restitution = 0.0f;
    if (material == nullptr)
        return true;
    if (!finite(&material->friction, 1) || !finite(&material->restitution, 1) || material->friction < 0.0f
        || material->friction > 10.0f || material->restitution < 0.0f || material->restitution > 1.0f)
        return false;
    friction = material->friction;
    restitution = material->restitution;
    return true;
}

bool valid_mass(float mass) { return std::isfinite(mass) && mass > 0.0f && mass <= MASS_MAX; }

} // namespace

struct physics_world {
    // a step's scratch memory, which Jolt sizes by the most bodies and
    // contacts: 16 MB and 4 KB a body (reserved, and touched only as used)
    explicit physics_world(uint32 bodies) : temp_allocator(16 * 1024 * 1024 + 4096 * bodies) {}

    BroadPhaseLayers broad_phase_layers;
    ObjectVsBroadPhase object_vs_broad_phase;
    ObjectPairs object_pairs;
    PhysicsSystem system;
    TempAllocatorImpl temp_allocator;
    JobSystemThreadPool job_system{cMaxPhysicsJobs, cMaxPhysicsBarriers, int(PHYSICS_WORKER_THREADS)};
    uint32 bodies_max = 0;
    uint32 collision_steps = 1;
    BodyID level;
    std::vector<RagdollSlot> ragdolls;
    std::vector<CharacterSlot> characters;

    BodyInterface &bodies() { return system.GetBodyInterfaceNoLock(); }
    const BodyInterface &bodies() const { return system.GetBodyInterfaceNoLock(); }

    // a body of a kind, by its handle; null for anything else
    const Body *find(uint32_t handle, uint64 kind) const
    {
        if (handle == PHYSICS_NONE || (handle & BodyID::cBroadPhaseBit) != 0)
            return nullptr;
        const Body *body = system.GetBodyLockInterfaceNoLock().TryGetBody(BodyID(handle));
        return body != nullptr && body->GetUserData() == kind ? body : nullptr;
    }

    RagdollSlot *find_ragdoll(uint32_t handle)
    {
        uint32 index = handle & 0xffff;
        if (handle == PHYSICS_NONE || index >= ragdolls.size())
            return nullptr;
        RagdollSlot &slot = ragdolls[index];
        return slot.ragdoll != nullptr && slot.generation == (handle >> 16) ? &slot : nullptr;
    }
    const RagdollSlot *find_ragdoll(uint32_t handle) const
    {
        return const_cast<physics_world *>(this)->find_ragdoll(handle);
    }

    CharacterSlot *find_character(uint32_t handle)
    {
        uint32 index = handle & 0xffff;
        if (handle == PHYSICS_NONE || index >= characters.size())
            return nullptr;
        CharacterSlot &slot = characters[index];
        return slot.character != nullptr && slot.generation == (handle >> 16) ? &slot : nullptr;
    }
    const CharacterSlot *find_character(uint32_t handle) const
    {
        return const_cast<physics_world *>(this)->find_character(handle);
    }

    uint32 room() const { return bodies_max - std::min(bodies_max, system.GetNumBodies()); }
};

// ---------- the world

extern "C" physics_world *physics_world_create(const physics_world_settings *settings)
{
    if (settings == nullptr || !finite(settings->gravity, 3) || settings->bodies_max == 0
        || settings->bodies_max > BODIES_MAX || settings->collision_steps == 0
        || settings->collision_steps > COLLISION_STEPS_MAX)
        return nullptr;
    start_jolt();
    // (one more body than asked for: the level's)
    physics_world *world = new physics_world(settings->bodies_max + 1);
    world->bodies_max = settings->bodies_max + 1;
    world->collision_steps = settings->collision_steps;
    world->system.Init(world->bodies_max, 0, 4 * world->bodies_max, 4 * world->bodies_max,
                       world->broad_phase_layers, world->object_vs_broad_phase, world->object_pairs);
    world->system.SetGravity(Vec3(settings->gravity[0], settings->gravity[1], settings->gravity[2]));
    // (a casing is 16 mm wide: Jolt's 20 mm of slop lets it sink into the
    // ground, and through it; and with less slop, a contact kept from the
    // step before pushes a body lying still across the ground, a few
    // centimetres a second, never asleep)
    PhysicsSettings physics;
    physics.mPenetrationSlop = PENETRATION_SLOP_METRES;
    physics.mSpeculativeContactDistance = SPECULATIVE_CONTACT_METRES;
    physics.mUseBodyPairContactCache = false;
    world->system.SetPhysicsSettings(physics);
    return world;
}

extern "C" void physics_world_destroy(physics_world *world)
{
    if (world == nullptr)
        return;
    for (RagdollSlot &slot : world->ragdolls)
        if (slot.ragdoll != nullptr)
            slot.ragdoll->RemoveFromPhysicsSystem(false);
    world->ragdolls.clear();
    world->characters.clear();
    BodyIDVector ids;
    world->system.GetBodies(ids);
    for (const BodyID &id : ids) {
        world->bodies().RemoveBody(id);
        world->bodies().DestroyBody(id);
    }
    delete world;
}

extern "C" void physics_world_clear_level(physics_world *world)
{
    if (world == nullptr || world->level.IsInvalid())
        return;
    world->bodies().RemoveBody(world->level);
    world->bodies().DestroyBody(world->level);
    world->level = BodyID();
}

extern "C" int physics_world_set_level(physics_world *world, const float *vertices, uint32_t vertex_count,
                                       const uint32_t *indices, uint32_t triangle_count)
{
    if (world == nullptr || vertices == nullptr || indices == nullptr || vertex_count == 0
        || triangle_count == 0 || triangle_count > LEVEL_TRIANGLES_MAX || vertex_count > 3 * LEVEL_TRIANGLES_MAX
        || !finite(vertices, int(3 * vertex_count)))
        return -1;
    VertexList points;
    points.reserve(vertex_count);
    for (uint32 i = 0; i < vertex_count; i++)
        points.push_back(Float3(vertices[3 * i], vertices[3 * i + 1], vertices[3 * i + 2]));
    IndexedTriangleList triangles;
    triangles.reserve(triangle_count);
    for (uint32 i = 0; i < triangle_count; i++) {
        uint32 a = indices[3 * i], b = indices[3 * i + 1], c = indices[3 * i + 2];
        if (a >= vertex_count || b >= vertex_count || c >= vertex_count)
            return -1;
        Vec3 pa(points[a]), pb(points[b]), pc(points[c]);
        // (degenerate: a corner repeated, or no area)
        if (a == b || b == c || a == c || (pb - pa).Cross(pc - pa).LengthSq() < 1.0e-12f)
            continue;
        triangles.push_back(IndexedTriangle(a, b, c));
    }
    if (triangles.empty())
        return -1;
    MeshShapeSettings mesh(std::move(points), std::move(triangles));
    ShapeSettings::ShapeResult result = mesh.Create();
    if (result.HasError()) {
        trace("the level's mesh is refused: %s", result.GetError().c_str());
        return -1;
    }
    physics_world_clear_level(world);
    BodyCreationSettings settings(result.Get(), RVec3::sZero(), Quat::sIdentity(), EMotionType::Static,
                                  LAYER_STATIC);
    settings.mUserData = KIND_LEVEL;
    settings.mFriction = 0.8f;
    Body *body = world->bodies().CreateBody(settings);
    if (body == nullptr)
        return -1;
    world->level = body->GetID();
    world->bodies().AddBody(world->level, EActivation::DontActivate);
    return 0;
}

extern "C" int physics_world_step(physics_world *world, float seconds)
{
    if (world == nullptr || !std::isfinite(seconds) || seconds <= 0.0f || seconds > STEP_MAX)
        return -1;
    EPhysicsUpdateError error = world->system.Update(seconds, int(world->collision_steps), &world->temp_allocator,
                                                     &world->job_system);
    if (error != EPhysicsUpdateError::None)
        trace("a step ran out of room (%u)", unsigned(error));
    return 0;
}

extern "C" uint32_t physics_world_active_bodies(const physics_world *world)
{
    return world == nullptr ? 0 : world->system.GetNumActiveBodies(EBodyType::RigidBody);
}

extern "C" uint32_t physics_world_bodies(const physics_world *world)
{
    if (world == nullptr)
        return 0;
    return world->system.GetNumBodies() - (world->level.IsInvalid() ? 0 : 1);
}

// ---------- single bodies

extern "C" uint32_t physics_body_add(physics_world *world, const physics_shape *shape, const physics_pose *pose,
                                     int32_t motion, float mass, const physics_material *material)
{
    RVec3 position;
    Quat rotation;
    float friction, restitution;
    if (world == nullptr || !read_pose(pose, position, rotation) || !valid_mass(mass)
        || (motion != PHYSICS_MOTION_KINEMATIC && motion != PHYSICS_MOTION_DYNAMIC)
        || !read_material(material, friction, restitution) || world->room() <= 1)
        return PHYSICS_NONE;
    RefConst<Shape> made = make_shape(shape);
    if (made == nullptr)
        return PHYSICS_NONE;
    BodyCreationSettings settings(made, position, rotation,
                                  motion == PHYSICS_MOTION_KINEMATIC ? EMotionType::Kinematic : EMotionType::Dynamic,
                                  LAYER_MOVING);
    settings.mUserData = KIND_BODY;
    settings.mFriction = friction;
    settings.mRestitution = restitution;
    settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = mass;
    // (a dynamic body is small and fast, a casing thrown from a gun: cast
    // along its step so that it does not pass through the level's thin
    // triangles), and it slows its turning as it goes: Jolt has no
    // rolling resistance, and a capsule would roll across level ground
    // forever, never slow enough to sleep
    if (motion == PHYSICS_MOTION_DYNAMIC) {
        settings.mMotionQuality = EMotionQuality::LinearCast;
        settings.mAngularDamping = BODY_ANGULAR_DAMPING;
    }
    Body *body = world->bodies().CreateBody(settings);
    if (body == nullptr)
        return PHYSICS_NONE;
    world->bodies().AddBody(body->GetID(), EActivation::Activate);
    return body->GetID().GetIndexAndSequenceNumber();
}

extern "C" void physics_body_remove(physics_world *world, uint32_t body)
{
    if (world == nullptr || world->find(body, KIND_BODY) == nullptr)
        return;
    world->bodies().RemoveBody(BodyID(body));
    world->bodies().DestroyBody(BodyID(body));
}

extern "C" int physics_body_get_pose(const physics_world *world, uint32_t body, physics_pose *pose)
{
    const Body *found = world == nullptr ? nullptr : world->find(body, KIND_BODY);
    if (found == nullptr || pose == nullptr)
        return -1;
    write_pose(found->GetPosition(), found->GetRotation(), pose);
    return 0;
}

extern "C" int physics_body_get_velocity(const physics_world *world, uint32_t body, float linear[3],
                                         float angular[3])
{
    const Body *found = world == nullptr ? nullptr : world->find(body, KIND_BODY);
    if (found == nullptr || linear == nullptr || angular == nullptr)
        return -1;
    found->GetLinearVelocity().StoreFloat3(reinterpret_cast<Float3 *>(linear));
    found->GetAngularVelocity().StoreFloat3(reinterpret_cast<Float3 *>(angular));
    return 0;
}

extern "C" int physics_body_move(physics_world *world, uint32_t body, const physics_pose *pose, float seconds)
{
    RVec3 position;
    Quat rotation;
    const Body *found = world == nullptr ? nullptr : world->find(body, KIND_BODY);
    if (found == nullptr || !found->IsKinematic() || !read_pose(pose, position, rotation)
        || !std::isfinite(seconds) || seconds <= 0.0f || seconds > STEP_MAX)
        return -1;
    world->bodies().MoveKinematic(BodyID(body), position, rotation, seconds);
    return 0;
}

extern "C" int physics_body_add_impulse(physics_world *world, uint32_t body, const float impulse[3],
                                        const float point[3])
{
    Vec3 push, at;
    const Body *found = world == nullptr ? nullptr : world->find(body, KIND_BODY);
    if (found == nullptr || !found->IsDynamic() || !read_vector(impulse, push, 1.0e6f)
        || !read_vector(point, at, 1.0e6f))
        return -1;
    world->bodies().AddImpulse(BodyID(body), push, RVec3(at));
    return 0;
}

extern "C" int physics_body_set_velocity(physics_world *world, uint32_t body, const float linear[3],
                                         const float angular[3])
{
    Vec3 velocity, spin;
    const Body *found = world == nullptr ? nullptr : world->find(body, KIND_BODY);
    if (found == nullptr || found->IsStatic() || !read_vector(linear, velocity, 1000.0f)
        || !read_vector(angular, spin, 1000.0f))
        return -1;
    world->bodies().SetLinearAndAngularVelocity(BodyID(body), velocity, spin);
    return 0;
}

extern "C" int physics_body_active(const physics_world *world, uint32_t body)
{
    const Body *found = world == nullptr ? nullptr : world->find(body, KIND_BODY);
    return found == nullptr ? -1 : found->IsActive() ? 1 : 0;
}

// ---------- ragdolls

namespace {

// a part's joint axes in the world at rest; false for axes that are not
// (zero, not finite, or parallel)
bool joint_axes(const physics_part &part, QuatArg rest, Vec3 &twist, Vec3 &plane)
{
    Vec3 local_twist, local_plane;
    if (!read_vector(part.twist_axis, local_twist, 1.0e3f) || !read_vector(part.plane_axis, local_plane, 1.0e3f)
        || local_twist.LengthSq() < 1.0e-6f || local_plane.LengthSq() < 1.0e-6f)
        return false;
    twist = (rest * local_twist).Normalized();
    plane = rest * local_plane;
    plane -= twist * twist.Dot(plane);
    if (plane.LengthSq() < 1.0e-6f)
        return false;
    plane = plane.Normalized();
    return true;
}

bool valid_limits(const physics_part &part)
{
    const float values[] = {part.twist_min, part.twist_max, part.swing_normal, part.swing_plane};
    return finite(values, 4) && part.twist_min >= -JPH_PI && part.twist_min <= 0.0f && part.twist_max >= 0.0f
           && part.twist_max <= JPH_PI && part.swing_normal >= 0.0f && part.swing_normal <= JPH_PI
           && part.swing_plane >= 0.0f && part.swing_plane <= JPH_PI;
}

SwingTwistConstraint *joint(const RagdollSlot &slot, int part)
{
    int index = slot.settings->GetConstraintIndexForBodyIndex(part);
    if (index < 0)
        return nullptr;
    return static_cast<SwingTwistConstraint *>(slot.ragdoll->GetConstraint(index));
}

} // namespace

extern "C" uint32_t physics_ragdoll_add(physics_world *world, const physics_part *parts, uint32_t count,
                                        const physics_pose *rest, const physics_pose *pose,
                                        const physics_material *material)
{
    float friction, restitution;
    if (world == nullptr || parts == nullptr || rest == nullptr || pose == nullptr || count == 0
        || count > PHYSICS_RAGDOLL_PARTS_MAX || !read_material(material, friction, restitution)
        || world->room() <= count)
        return PHYSICS_NONE;

    Ref<RagdollSettings> settings = new RagdollSettings();
    settings->mSkeleton = new Skeleton();
    settings->mParts.resize(count);
    std::vector<Mat44> rest_matrices(count), pose_matrices(count);
    for (uint32 i = 0; i < count; i++) {
        const physics_part &part = parts[i];
        RVec3 rest_position, position;
        Quat rest_rotation, rotation;
        if ((i == 0) != (part.parent < 0) || (i > 0 && uint32(part.parent) >= i) || !valid_mass(part.mass)
            || !read_pose(&rest[i], rest_position, rest_rotation) || !read_pose(&pose[i], position, rotation))
            return PHYSICS_NONE;
        RefConst<Shape> shape = make_shape(&part.shape);
        if (shape == nullptr)
            return PHYSICS_NONE;
        char name[8];
        std::snprintf(name, sizeof(name), "%u", unsigned(i));
        settings->mSkeleton->AddJoint(name, part.parent);
        rest_matrices[i] = Mat44::sRotationTranslation(rest_rotation, Vec3(rest_position));
        pose_matrices[i] = Mat44::sRotationTranslation(rotation, Vec3(position));

        RagdollSettings::Part &made = settings->mParts[i];
        made.SetShape(shape);
        made.mPosition = rest_position;
        made.mRotation = rest_rotation;
        made.mMotionType = EMotionType::Dynamic;
        made.mObjectLayer = LAYER_MOVING;
        made.mUserData = KIND_RAGDOLL;
        made.mFriction = friction;
        made.mRestitution = restitution;
        made.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
        made.mMassPropertiesOverride.mMass = part.mass;
        made.mNumVelocityStepsOverride = RAGDOLL_VELOCITY_STEPS;
        made.mNumPositionStepsOverride = RAGDOLL_POSITION_STEPS;
        if (i == 0)
            continue;
        Vec3 twist, plane;
        if (!valid_limits(part) || !joint_axes(part, rest_rotation, twist, plane))
            return PHYSICS_NONE;
        Ref<SwingTwistConstraintSettings> constraint = new SwingTwistConstraintSettings();
        constraint->mSpace = EConstraintSpace::WorldSpace;
        constraint->mPosition1 = constraint->mPosition2 = rest_position;
        constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist;
        constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = plane;
        constraint->mTwistMinAngle = part.twist_min;
        constraint->mTwistMaxAngle = part.twist_max;
        constraint->mNormalHalfConeAngle = part.swing_normal;
        constraint->mPlaneHalfConeAngle = part.swing_plane;
        constraint->mNumVelocityStepsOverride = RAGDOLL_VELOCITY_STEPS;
        constraint->mNumPositionStepsOverride = RAGDOLL_POSITION_STEPS;
        constraint->mMaxFrictionTorque = FRICTION_TORQUE_PER_KILOGRAM * part.mass;
        float torque = MOTOR_TORQUE_PER_KILOGRAM * part.mass;
        constraint->mSwingMotorSettings = MotorSettings(10.0f, 1.0f, 0.0f, torque);
        constraint->mTwistMotorSettings = MotorSettings(10.0f, 1.0f, 0.0f, torque);
        made.mToParent = constraint;
    }
    settings->mSkeleton->CalculateParentJointIndices();
    if (!settings->Stabilize())
        return PHYSICS_NONE;
    // (joined parts, and those that overlap at rest, do not collide)
    settings->DisableParentChildCollisions(rest_matrices.data(), 0.0f);
    settings->CalculateBodyIndexToConstraintIndex();
    settings->CalculateConstraintIndexToBodyIdxPair();

    uint32 index = 0;
    while (index < world->ragdolls.size() && world->ragdolls[index].ragdoll != nullptr)
        index++;
    if (index == RAGDOLLS_MAX)
        return PHYSICS_NONE;
    if (index == world->ragdolls.size())
        world->ragdolls.emplace_back();
    RagdollSlot &slot = world->ragdolls[index];
    Ragdoll *ragdoll = settings->CreateRagdoll(CollisionGroup::GroupID(index), KIND_RAGDOLL, &world->system);
    if (ragdoll == nullptr)
        return PHYSICS_NONE;
    slot.settings = settings;
    slot.ragdoll = ragdoll;
    slot.filter = const_cast<GroupFilterTable *>(
        static_cast<const GroupFilterTable *>(settings->mParts[0].mCollisionGroup.GetGroupFilter()));
    slot.generation = (slot.generation + 1) & 0xffff;
    if (slot.generation == 0xffff)
        slot.generation = 0;
    ragdoll->SetPose(RVec3::sZero(), pose_matrices.data(), false);
    ragdoll->AddToPhysicsSystem(EActivation::Activate, false);
    return index | (slot.generation << 16);
}

extern "C" void physics_ragdoll_remove(physics_world *world, uint32_t ragdoll)
{
    RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr)
        return;
    slot->ragdoll->RemoveFromPhysicsSystem(false);
    slot->ragdoll = nullptr;
    slot->settings = nullptr;
    slot->filter = nullptr;
}

extern "C" int physics_ragdoll_get_poses(const physics_world *world, uint32_t ragdoll, physics_pose *poses,
                                         uint32_t count)
{
    const RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr || poses == nullptr || count != slot->ragdoll->GetBodyCount())
        return -1;
    for (uint32 i = 0; i < count; i++) {
        const Body *body = world->system.GetBodyLockInterfaceNoLock().TryGetBody(slot->ragdoll->GetBodyID(int(i)));
        if (body == nullptr)
            return -1;
        write_pose(body->GetPosition(), body->GetRotation(), &poses[i]);
    }
    return 0;
}

extern "C" int physics_ragdoll_drive(physics_world *world, uint32_t ragdoll, const physics_pose *target,
                                     uint32_t count, float frequency)
{
    RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr || target == nullptr || count != slot->ragdoll->GetBodyCount() || !std::isfinite(frequency)
        || frequency <= 0.0f || frequency > 100.0f)
        return -1;
    std::vector<Quat> rotations(count);
    for (uint32 i = 0; i < count; i++) {
        RVec3 position;
        if (!read_pose(&target[i], position, rotations[i]))
            return -1;
    }
    for (uint32 i = 1; i < count; i++) {
        SwingTwistConstraint *constraint = joint(*slot, int(i));
        if (constraint == nullptr || !constraint->GetEnabled())
            continue;
        int parent = slot->settings->mSkeleton->GetJoint(int(i)).mParentJointIndex;
        constraint->GetSwingMotorSettings().mSpringSettings.mFrequency = frequency;
        constraint->GetTwistMotorSettings().mSpringSettings.mFrequency = frequency;
        constraint->SetSwingMotorState(EMotorState::Position);
        constraint->SetTwistMotorState(EMotorState::Position);
        constraint->SetTargetOrientationBS(rotations[parent].Conjugated() * rotations[i]);
    }
    slot->ragdoll->Activate(false);
    return 0;
}

extern "C" int physics_ragdoll_go_limp(physics_world *world, uint32_t ragdoll)
{
    RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr)
        return -1;
    for (uint32 i = 1; i < slot->ragdoll->GetBodyCount(); i++) {
        SwingTwistConstraint *constraint = joint(*slot, int(i));
        if (constraint == nullptr)
            continue;
        constraint->SetSwingMotorState(EMotorState::Off);
        constraint->SetTwistMotorState(EMotorState::Off);
    }
    return 0;
}

extern "C" int physics_ragdoll_add_impulse(physics_world *world, uint32_t ragdoll, uint32_t part,
                                           const float impulse[3], const float point[3])
{
    Vec3 push, at;
    RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr || part >= slot->ragdoll->GetBodyCount() || !read_vector(impulse, push, 1.0e6f)
        || !read_vector(point, at, 1.0e6f))
        return -1;
    world->bodies().AddImpulse(slot->ragdoll->GetBodyID(int(part)), push, RVec3(at));
    return 0;
}

extern "C" int physics_ragdoll_set_velocity(physics_world *world, uint32_t ragdoll, const float linear[3])
{
    Vec3 velocity;
    RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr || !read_vector(linear, velocity, 1000.0f))
        return -1;
    slot->ragdoll->SetLinearVelocity(velocity, false);
    return 0;
}

extern "C" int physics_ragdoll_break_joint(physics_world *world, uint32_t ragdoll, uint32_t part)
{
    RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr || part == 0 || part >= slot->ragdoll->GetBodyCount())
        return -1;
    SwingTwistConstraint *constraint = joint(*slot, int(part));
    if (constraint == nullptr || !constraint->GetEnabled())
        return -1;
    constraint->SetEnabled(false);
    int parent = slot->settings->mSkeleton->GetJoint(int(part)).mParentJointIndex;
    slot->filter->EnableCollision(CollisionGroup::SubGroupID(part), CollisionGroup::SubGroupID(parent));
    // (torn off, the limb rolls on its own as a single body does, and is
    // slowed as one is, or it rolls across level ground forever)
    {
        BodyLockWrite lock(world->system.GetBodyLockInterface(), slot->ragdoll->GetBodyID(int(part)));
        if (lock.Succeeded())
            lock.GetBody().GetMotionProperties()->SetAngularDamping(BODY_ANGULAR_DAMPING);
    }
    slot->ragdoll->Activate(false);
    return 0;
}

extern "C" int physics_ragdoll_joint_broken(const physics_world *world, uint32_t ragdoll, uint32_t part)
{
    const RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr || part >= slot->ragdoll->GetBodyCount())
        return -1;
    if (part == 0)
        return 0;
    const SwingTwistConstraint *constraint = joint(*slot, int(part));
    return constraint != nullptr && !constraint->GetEnabled() ? 1 : 0;
}

extern "C" int physics_ragdoll_active(const physics_world *world, uint32_t ragdoll)
{
    const RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr)
        return -1;
    for (const BodyID &id : slot->ragdoll->GetBodyIDs())
        if (world->bodies().IsActive(id))
            return 1;
    return 0;
}

extern "C" float physics_ragdoll_limit_error(const physics_world *world, uint32_t ragdoll)
{
    const RagdollSlot *slot = world == nullptr ? nullptr : world->find_ragdoll(ragdoll);
    if (slot == nullptr)
        return -1.0f;
    float worst = 0.0f;
    for (uint32 i = 1; i < slot->ragdoll->GetBodyCount(); i++) {
        const SwingTwistConstraint *constraint = joint(*slot, int(i));
        if (constraint == nullptr || !constraint->GetEnabled())
            continue;
        Quat rotation = constraint->GetRotationInConstraintSpace().EnsureWPositive();
        Quat swing, twist;
        rotation.GetSwingTwist(swing, twist);
        twist = twist.EnsureWPositive();
        swing = swing.EnsureWPositive();
        float twist_angle = 2.0f * std::atan2(twist.GetX(), twist.GetW());
        worst = std::max(worst, std::max(constraint->GetTwistMinAngle() - twist_angle,
                                         twist_angle - constraint->GetTwistMaxAngle()));
        // the swing's angles about the plane axis (y) and the normal (z),
        // against the cone's ellipse in that direction
        float about_plane = 2.0f * std::atan2(swing.GetY(), swing.GetW());
        float about_normal = 2.0f * std::atan2(swing.GetZ(), swing.GetW());
        float angle = std::sqrt(about_plane * about_plane + about_normal * about_normal);
        if (angle < 1.0e-6f)
            continue;
        float plane = std::max(constraint->GetPlaneHalfConeAngle(), 1.0e-4f);
        float normal = std::max(constraint->GetNormalHalfConeAngle(), 1.0e-4f);
        float c = about_plane / angle / plane, s = about_normal / angle / normal;
        worst = std::max(worst, angle - 1.0f / std::sqrt(c * c + s * s));
    }
    return worst;
}

// ---------- casts

namespace {

// a cast's layers (physics_cast_layers), for the broad phase and the bodies
class CastBroadPhase final : public BroadPhaseLayerFilter {
public:
    explicit CastBroadPhase(uint32_t layers) : layers(layers) {}
    bool ShouldCollide(BroadPhaseLayer layer) const override
    {
        return (layers & (layer == BroadPhaseLayer(0) ? PHYSICS_CAST_LEVEL : PHYSICS_CAST_BODIES)) != 0;
    }

private:
    uint32_t layers;
};

class CastObjects final : public ObjectLayerFilter {
public:
    explicit CastObjects(uint32_t layers) : layers(layers) {}
    bool ShouldCollide(ObjectLayer layer) const override
    {
        return (layers & (layer == LAYER_STATIC ? PHYSICS_CAST_LEVEL : PHYSICS_CAST_BODIES)) != 0;
    }

private:
    uint32_t layers;
};

bool valid_layers(uint32_t layers)
{
    return layers != 0 && (layers & ~uint32_t(PHYSICS_CAST_LEVEL | PHYSICS_CAST_BODIES)) == 0;
}

void write_hit(const physics_world *world, const BodyID &id, float fraction, RVec3Arg point, Vec3Arg normal,
               physics_hit *hit)
{
    hit->fraction = fraction;
    hit->point[0] = float(point.GetX());
    hit->point[1] = float(point.GetY());
    hit->point[2] = float(point.GetZ());
    hit->normal[0] = normal.GetX();
    hit->normal[1] = normal.GetY();
    hit->normal[2] = normal.GetZ();
    hit->level = id == world->level ? 1 : 0;
    hit->body = world->find(id.GetIndexAndSequenceNumber(), KIND_BODY) != nullptr ? id.GetIndexAndSequenceNumber()
                                                                                    : PHYSICS_NONE;
}

} // namespace

extern "C" int physics_world_cast_ray(const physics_world *world, const float origin[3], const float direction[3],
                                      uint32_t layers, physics_hit *hit)
{
    if (world == nullptr || origin == nullptr || direction == nullptr || hit == nullptr || !finite(origin, 3)
        || !finite(direction, 3) || !valid_layers(layers))
        return -1;
    RRayCast ray{RVec3(origin[0], origin[1], origin[2]), Vec3(direction[0], direction[1], direction[2])};
    if (ray.mDirection.LengthSq() == 0.0f)
        return -1;
    RayCastResult result;
    CastBroadPhase broad_phase(layers);
    CastObjects objects(layers);
    if (!world->system.GetNarrowPhaseQueryNoLock().CastRay(ray, result, broad_phase, objects))
        return 0;
    RVec3 point = ray.GetPointOnRay(result.mFraction);
    const Body *body = world->system.GetBodyLockInterfaceNoLock().TryGetBody(result.mBodyID);
    if (body == nullptr)
        return 0;
    Vec3 normal = body->GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point);
    // (a two-sided or back face: the normal toward the ray's start)
    if (normal.Dot(ray.mDirection) > 0.0f)
        normal = -normal;
    write_hit(world, result.mBodyID, result.mFraction, point, normal, hit);
    return 1;
}

extern "C" int physics_world_cast_shape(const physics_world *world, const physics_shape *shape,
                                        const physics_pose *pose, const float direction[3], uint32_t layers,
                                        physics_hit *hit)
{
    RVec3 position;
    Quat rotation;
    if (world == nullptr || direction == nullptr || hit == nullptr || !read_pose(pose, position, rotation)
        || !finite(direction, 3) || !valid_layers(layers))
        return -1;
    RefConst<Shape> made = make_shape(shape);
    if (made == nullptr)
        return -1;
    RShapeCast cast = RShapeCast::sFromWorldTransform(made, Vec3::sOne(),
                                                      RMat44::sRotationTranslation(rotation, position),
                                                      Vec3(direction[0], direction[1], direction[2]));
    ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = true;
    ClosestHitCollisionCollector<CastShapeCollector> collector;
    CastBroadPhase broad_phase(layers);
    CastObjects objects(layers);
    world->system.GetNarrowPhaseQueryNoLock().CastShape(cast, settings, RVec3::sZero(), collector, broad_phase,
                                                        objects);
    if (!collector.HadHit())
        return 0;
    const ShapeCastResult &result = collector.mHit;
    Vec3 axis = result.mPenetrationAxis;
    Vec3 normal = axis.LengthSq() > 0.0f ? -axis.Normalized() : Vec3::sAxisZ();
    write_hit(world, result.mBodyID2, result.mFraction, RVec3(result.mContactPointOn2), normal, hit);
    return 1;
}

// ---------- characters

namespace {

// A character's pill: Jolt's capsules stand along y, ours along z, with
// the character's position at its feet.
RefConst<Shape> character_shape(float radius, float height)
{
    if (!positive_size(radius) || !positive_size(height) || !(height > 2.0f * radius))
        return nullptr;
    Ref<ShapeSettings> settings = new CapsuleShapeSettings(0.5f * height - radius, radius);
    settings = new RotatedTranslatedShapeSettings(Vec3(0.0f, 0.0f, 0.5f * height),
                                                  Quat::sRotation(Vec3::sAxisX(), 0.5f * JPH_PI), settings);
    ShapeSettings::ShapeResult result = settings->Create();
    if (result.HasError())
        return nullptr;
    return result.Get();
}

void write_character(const CharacterSlot &slot, physics_character_state *state)
{
    RVec3 position = slot.character->GetPosition();
    Vec3 velocity = slot.character->GetLinearVelocity();
    CharacterBase::EGroundState ground = slot.character->GetGroundState();
    Vec3 normal = ground == CharacterBase::EGroundState::InAir ? Vec3::sAxisZ() : slot.character->GetGroundNormal();
    for (int axis = 0; axis < 3; axis++) {
        state->position[axis] = float(position[axis]);
        state->velocity[axis] = velocity[axis];
        state->ground_normal[axis] = normal[axis];
    }
    switch (ground) {
    case CharacterBase::EGroundState::OnGround:
        state->ground = PHYSICS_GROUND_STANDING;
        break;
    case CharacterBase::EGroundState::OnSteepGround:
        state->ground = PHYSICS_GROUND_STEEP;
        break;
    case CharacterBase::EGroundState::NotSupported:
        state->ground = PHYSICS_GROUND_UNSUPPORTED;
        break;
    default:
        state->ground = PHYSICS_GROUND_AIRBORNE;
        break;
    }
}

} // namespace

extern "C" uint32_t physics_character_add(physics_world *world, const physics_character_settings *settings,
                                          const float position[3])
{
    if (world == nullptr || settings == nullptr || position == nullptr || !finite(position, 3)
        || !std::isfinite(settings->maximum_slope) || settings->maximum_slope <= 0.0f
        || settings->maximum_slope >= 0.5f * JPH_PI || !std::isfinite(settings->step_up)
        || settings->step_up < 0.0f || settings->step_up > SIZE_MAX_METRES || !std::isfinite(settings->stick_down)
        || settings->stick_down < 0.0f || settings->stick_down > SIZE_MAX_METRES
        || !valid_layers(settings->layers))
        return PHYSICS_NONE;
    RefConst<Shape> shape = character_shape(settings->radius, settings->height);
    if (shape == nullptr)
        return PHYSICS_NONE;
    uint32 index = 0;
    while (index < world->characters.size() && world->characters[index].character != nullptr)
        index++;
    if (index >= CHARACTERS_MAX)
        return PHYSICS_NONE;
    if (index == world->characters.size())
        world->characters.emplace_back();
    Ref<CharacterVirtualSettings> made = new CharacterVirtualSettings;
    made->mShape = shape;
    made->mUp = Vec3::sAxisZ();
    made->mMaxSlopeAngle = settings->maximum_slope;
    // (only contacts below the lower sphere's centre hold it up)
    made->mSupportingVolume = Plane(Vec3::sAxisZ(), -settings->radius);
    CharacterSlot &slot = world->characters[index];
    slot.character = new CharacterVirtual(made, RVec3(position[0], position[1], position[2]), Quat::sIdentity(), 0,
                                          &world->system);
    slot.radius = settings->radius;
    slot.step_up = settings->step_up;
    slot.stick_down = settings->stick_down;
    slot.layers = settings->layers;
    CastBroadPhase broad_phase(slot.layers);
    CastObjects objects(slot.layers);
    slot.character->RefreshContacts(broad_phase, objects, BodyFilter(), ShapeFilter(), world->temp_allocator);
    return index | (slot.generation << 16);
}

extern "C" void physics_character_remove(physics_world *world, uint32_t character)
{
    CharacterSlot *slot = world == nullptr ? nullptr : world->find_character(character);
    if (slot == nullptr)
        return;
    slot->character = nullptr;
    slot->generation = (slot->generation + 1) & 0xffff;
}

extern "C" int physics_character_move(physics_world *world, uint32_t character, const float velocity[3],
                                      float seconds, physics_character_state *state)
{
    CharacterSlot *slot = world == nullptr ? nullptr : world->find_character(character);
    if (slot == nullptr || velocity == nullptr || state == nullptr || !finite(velocity, 3)
        || !std::isfinite(seconds) || seconds <= 0.0f || seconds > STEP_MAX)
        return -1;
    slot->character->SetLinearVelocity(Vec3(velocity[0], velocity[1], velocity[2]));
    CharacterVirtual::ExtendedUpdateSettings update;
    update.mStickToFloorStepDown = Vec3(0.0f, 0.0f, -slot->stick_down);
    update.mWalkStairsStepUp = Vec3(0.0f, 0.0f, slot->step_up);
    CastBroadPhase broad_phase(slot->layers);
    CastObjects objects(slot->layers);
    slot->character->ExtendedUpdate(seconds, world->system.GetGravity(), update, broad_phase, objects, BodyFilter(),
                                    ShapeFilter(), world->temp_allocator);
    write_character(*slot, state);
    return 0;
}

extern "C" int physics_character_set_position(physics_world *world, uint32_t character, const float position[3])
{
    CharacterSlot *slot = world == nullptr ? nullptr : world->find_character(character);
    if (slot == nullptr || position == nullptr || !finite(position, 3))
        return -1;
    slot->character->SetPosition(RVec3(position[0], position[1], position[2]));
    slot->character->SetLinearVelocity(Vec3::sZero());
    CastBroadPhase broad_phase(slot->layers);
    CastObjects objects(slot->layers);
    slot->character->RefreshContacts(broad_phase, objects, BodyFilter(), ShapeFilter(), world->temp_allocator);
    return 0;
}

extern "C" int physics_character_get_state(const physics_world *world, uint32_t character,
                                           physics_character_state *state)
{
    const CharacterSlot *slot = world == nullptr ? nullptr : world->find_character(character);
    if (slot == nullptr || state == nullptr)
        return -1;
    write_character(*slot, state);
    return 0;
}

extern "C" int physics_character_set_height(physics_world *world, uint32_t character, float height)
{
    CharacterSlot *slot = world == nullptr ? nullptr : world->find_character(character);
    if (slot == nullptr)
        return -1;
    RefConst<Shape> shape = character_shape(slot->radius, height);
    if (shape == nullptr)
        return -1;
    CastBroadPhase broad_phase(slot->layers);
    CastObjects objects(slot->layers);
    // (refused where the new pill would overlap the level: no room to stand)
    return slot->character->SetShape(shape, 1.5f * PENETRATION_SLOP_METRES, broad_phase, objects, BodyFilter(),
                                     ShapeFilter(), world->temp_allocator)
               ? 1
               : 0;
}
