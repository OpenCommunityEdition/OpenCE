/*
RAGDOLL_PHYSICS.CPP

The ragdolls' physics engine: Jolt Physics (port/third_party/JoltPhysics,
github.com/jrouwe/JoltPhysics, MIT licence) behind ragdoll_physics.h's C
interface, for port/linux/game/ragdoll.c.

One PhysicsSystem holds every ragdoll and the level around each: a ragdoll
is a Jolt Ragdoll (a body a bone, capsules, swing-twist and hinge
constraints, its overlapping parts kept from colliding with each other,
stabilised mass ratios), and the level around it static meshes of the
triangles ragdoll.c gives (the game's collision surfaces near it). Ragdolls
collide with each other.

Halo's units are converted here: world units to metres (3.048), ticks to
seconds (1/30). One thread (the game's): no job threads to share the game
with.
*/

#include <Jolt/Jolt.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/HashCombine.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>

#include "ragdoll_physics.h"

using namespace JPH;

namespace
{

/* ---------- units */

const float k_metres = 3.048f; // a world unit
const float k_tick = 1.0f / 30.0f; // seconds

inline Vec3 metres(const float *v)
{
	return Vec3(v[0] * k_metres, v[1] * k_metres, v[2] * k_metres);
}

inline void world_units(Vec3Arg v, float *out)
{
	out[0] = v.GetX() / k_metres;
	out[1] = v.GetY() / k_metres;
	out[2] = v.GetZ() / k_metres;
}

inline bool finite3(Vec3Arg v)
{
	return std::isfinite(v.GetX()) && std::isfinite(v.GetY()) && std::isfinite(v.GetZ());
}

/* ---------- layers: the level (static) and the ragdolls (moving) */

namespace layers
{
	const ObjectLayer level = 0;
	const ObjectLayer ragdoll = 1;
}

class object_layer_pair_filter final : public ObjectLayerPairFilter
{
public:
	bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override
	{
		return a == layers::ragdoll || b == layers::ragdoll;
	}
};

class broad_phase_layers final : public BroadPhaseLayerInterface
{
public:
	uint GetNumBroadPhaseLayers() const override { return 2; }
	BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer layer) const override { return BroadPhaseLayer((BroadPhaseLayer::Type)layer); }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	const char *GetBroadPhaseLayerName(BroadPhaseLayer layer) const override
	{
		return layer.GetValue() == layers::level ? "level" : "ragdoll";
	}
#endif
};

class object_versus_broad_phase_filter final : public ObjectVsBroadPhaseLayerFilter
{
public:
	bool ShouldCollide(ObjectLayer layer, BroadPhaseLayer broad_phase_layer) const override
	{
		return layer == layers::ragdoll || broad_phase_layer.GetValue() == layers::ragdoll;
	}
};

/* ---------- the world */

struct ragdoll_slot
{
	Ref<Ragdoll> ragdoll;
	float mass = 0.f;
	BodyID environment[RAGDOLL_PHYSICS_ENVIRONMENT_LAYERS];
	uint64 environment_hash[RAGDOLL_PHYSICS_ENVIRONMENT_LAYERS] = {};
};

/* what a body is, in its user data */
const uint64 k_user_ragdoll = 1ull << 32;
const uint64 k_user_vehicle = 2ull << 32;
const uint64 k_user_kind = 0xffffffff00000000ull;

const int k_maximum_contacts = 64;

/* a vehicle a ragdoll passes through (its own, sat in), key + 1; 0: none */
class vehicle_contact_listener final : public ContactListener
{
public:
	ragdoll_physics_contact contacts[k_maximum_contacts];
	int count = 0;

	ValidateResult OnContactValidate(const Body &a, const Body &b, RVec3Arg, const CollideShapeResult &) override
	{
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void OnContactAdded(const Body &a, const Body &b, const ContactManifold &manifold, ContactSettings &) override
	{
		note(a, b, manifold);
	}

	void OnContactPersisted(const Body &a, const Body &b, const ContactManifold &manifold, ContactSettings &) override
	{
		note(a, b, manifold);
	}

private:
	void note(const Body &a, const Body &b, const ContactManifold &manifold)
	{
		uint64 kind_a = a.GetUserData() & k_user_kind;
		uint64 kind_b = b.GetUserData() & k_user_kind;
		const Body *vehicle;
		const Body *ragdoll;
		bool vehicle_first;

		if (kind_a == k_user_vehicle && kind_b == k_user_ragdoll)
		{
			vehicle = &a;
			ragdoll = &b;
			vehicle_first = true;
		}
		else if (kind_b == k_user_vehicle && kind_a == k_user_ragdoll)
		{
			vehicle = &b;
			ragdoll = &a;
			vehicle_first = false;
		}
		else
		{
			return;
		}
		if (manifold.mRelativeContactPointsOn1.empty())
			return;

		RVec3 point = vehicle_first ? manifold.GetWorldSpaceContactPointOn1(0) : manifold.GetWorldSpaceContactPointOn2(0);
		// (the normal moves body 2 out of body 1: out of the vehicle when it is body 1)
		Vec3 normal = vehicle_first ? manifold.mWorldSpaceNormal : -manifold.mWorldSpaceNormal;
		Vec3 relative = vehicle->GetPointVelocity(point) - ragdoll->GetPointVelocity(point);
		float speed = relative.Length() * k_tick / k_metres;
		int slot = (int)(ragdoll->GetUserData() & 0xffffffffull);
		long key = (long)(uint32)(vehicle->GetUserData() & 0xffffffffull);

		for (int index = 0; index < count; ++index)
		{
			ragdoll_physics_contact &contact = contacts[index];
			if (contact.slot == slot && contact.vehicle == key)
			{
				if (speed > contact.speed)
					set(contact, slot, key, point, normal, speed);
				return;
			}
		}
		if (count < k_maximum_contacts)
			set(contacts[count++], slot, key, point, normal, speed);
	}

	static void set(ragdoll_physics_contact &contact, int slot, long key, RVec3Arg point, Vec3Arg normal, float speed)
	{
		contact.slot = slot;
		contact.vehicle = key;
		world_units(Vec3(point), contact.point);
		contact.normal[0] = normal.GetX();
		contact.normal[1] = normal.GetY();
		contact.normal[2] = normal.GetZ();
		contact.speed = speed;
	}
};

struct vehicle_slot
{
	long key = -1; // (the game's object index; -1: free)
	BodyID id;
	uint64 hash = 0; // its points', for its shape
	bool given = false; // this tick
};

const int k_maximum_vehicles = 24;

struct world
{
	bool started = false;
	bool failed = false;
	TempAllocatorImpl *temporary = nullptr;
	JobSystemSingleThreaded *jobs = nullptr;
	PhysicsSystem *system = nullptr;
	broad_phase_layers broad_phase;
	object_versus_broad_phase_filter object_versus_broad_phase;
	object_layer_pair_filter object_pairs;
	ragdoll_slot slots[RAGDOLL_PHYSICS_MAXIMUM_RAGDOLLS];
	vehicle_slot vehicles[k_maximum_vehicles];
	vehicle_contact_listener contacts;
	float fastest_vehicle = 0.f; // metres a tick, this tick
};

// (never destroyed: the game may end with ragdolls in the world, and the
// engine's objects are not to be torn down after it at exit)
world &g_world = *new world();

void trace(const char *format, ...)
{
	char text[512];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);
	fprintf(stderr, "ragdolls (Jolt): %s\n", text);
}

#ifdef JPH_ENABLE_ASSERTS
bool assert_failed(const char *expression, const char *message, const char *file, uint line)
{
	trace("assertion %s (%s) at %s:%u", expression, message ? message : "", file, line);
	return false; // (carry on: the game is not to stop for its ragdolls)
}
#endif

ragdoll_slot *slot_get(int slot)
{
	if (!g_world.started || slot < 0 || slot >= RAGDOLL_PHYSICS_MAXIMUM_RAGDOLLS)
		return nullptr;
	return &g_world.slots[slot];
}

BodyInterface &bodies()
{
	return g_world.system->GetBodyInterfaceNoLock();
}

void environment_remove(ragdoll_slot &slot, int layer)
{
	BodyID &id = slot.environment[layer];

	if (!id.IsInvalid())
	{
		bodies().RemoveBody(id);
		bodies().DestroyBody(id);
		id = BodyID();
	}
	slot.environment_hash[layer] = 0;
}

Quat quaternion(const float *q)
{
	Quat result(q[0], q[1], q[2], q[3]);

	if (!std::isfinite(result.LengthSq()) || result.LengthSq() < 1.0e-12f)
		return Quat::sIdentity();
	return result.Normalized();
}

/* a unit vector square to v */
Vec3 perpendicular(Vec3Arg v)
{
	return v.GetNormalizedPerpendicular();
}

} // namespace

/* ---------- the interface */

extern "C" int ragdoll_physics_initialize(void)
{
	if (g_world.started)
		return 1;
	if (g_world.failed)
		return 0;

	RegisterDefaultAllocator();
	Trace = trace;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = assert_failed;)
	if (Factory::sInstance == nullptr)
		Factory::sInstance = new Factory();
	RegisterTypes();

	g_world.temporary = new TempAllocatorImpl(8 * 1024 * 1024);
	g_world.jobs = new JobSystemSingleThreaded(cMaxPhysicsJobs);
	g_world.system = new PhysicsSystem();
	// (bodies: 32 ragdolls of up to 64 bones and their levels)
	g_world.system->Init(
		4096, // bodies
		0, // body mutexes: the default
		8192, // body pairs
		8192, // contact constraints
		g_world.broad_phase,
		g_world.object_versus_broad_phase,
		g_world.object_pairs);

	PhysicsSettings settings = g_world.system->GetPhysicsSettings();
	settings.mNumVelocitySteps = 12;
	settings.mNumPositionSteps = 4;
	// (a body at rest sleeps soon, rather than creeping)
	settings.mPointVelocitySleepThreshold = 0.06f;
	settings.mTimeBeforeSleep = 0.4f;
	g_world.system->SetPhysicsSettings(settings);
	g_world.system->SetContactListener(&g_world.contacts);
	g_world.started = true;
	return 1;
}

extern "C" void ragdoll_physics_reset(void)
{
	if (!g_world.started)
		return;
	for (int slot = 0; slot < RAGDOLL_PHYSICS_MAXIMUM_RAGDOLLS; ++slot)
		ragdoll_physics_destroy(slot);
	ragdoll_physics_vehicles_begin();
	ragdoll_physics_vehicles_end();
	g_world.system->OptimizeBroadPhase();
}

extern "C" void ragdoll_physics_destroy(int index)
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot)
		return;
	if (slot->ragdoll)
	{
		// (its bodies go with it: Ragdoll's destructor)
		slot->ragdoll->RemoveFromPhysicsSystem(false);
		slot->ragdoll = nullptr;
	}
	for (int layer = 0; layer < RAGDOLL_PHYSICS_ENVIRONMENT_LAYERS; ++layer)
		environment_remove(*slot, layer);
	slot->mass = 0.f;
}

extern "C" int ragdoll_physics_exists(int index)
{
	ragdoll_slot *slot = slot_get(index);

	return slot && slot->ragdoll != nullptr;
}

extern "C" int ragdoll_physics_create(int index, struct ragdoll_physics_bone const *bones, int bone_count,
	struct ragdoll_physics_settings const *settings)
{
	if (!ragdoll_physics_initialize())
		return 0;
	ragdoll_slot *slot = slot_get(index);
	if (!slot || bone_count < 1 || bone_count > RAGDOLL_PHYSICS_MAXIMUM_BONES)
		return 0;
	ragdoll_physics_destroy(index);

	Ref<Skeleton> skeleton = new Skeleton();
	Ref<RagdollSettings> ragdoll_settings = new RagdollSettings();
	ragdoll_settings->mSkeleton = skeleton;
	ragdoll_settings->mParts.resize(bone_count);
	Array<Mat44> matrices((size_t)bone_count);

	// each bone's volume, for its share of the mass
	Array<float> volumes((size_t)bone_count);
	float total_volume = 0.f;
	for (int b = 0; b < bone_count; ++b)
	{
		const ragdoll_physics_bone &bone = bones[b];
		float radius = std::max(bone.radius * k_metres, 0.01f);
		float length = (metres(bone.capsule_end) - metres(bone.capsule_start)).Length();
		volumes[b] = JPH_PI * radius * radius * length + 4.f / 3.f * JPH_PI * radius * radius * radius;
		if (bone.radius2 > 0.f)
		{
			float radius2 = std::max(bone.radius2 * k_metres, 0.01f);
			float length2 = (metres(bone.capsule2_end) - metres(bone.capsule2_start)).Length();
			volumes[b] += JPH_PI * radius2 * radius2 * length2 + 4.f / 3.f * JPH_PI * radius2 * radius2 * radius2;
		}
		total_volume += volumes[b];
	}
	if (!(total_volume > 0.f))
		return 0;
	float mass = std::max(settings->mass, 1.f);

	// each bone's mass and its centre (the middle of its capsule, in the
	// world), and the weight's torque the limbs beyond each joint put on it,
	// held out level: what its friction is a share of
	Array<float> masses((size_t)bone_count);
	Array<Vec3> centres((size_t)bone_count);
	Array<float> friction((size_t)bone_count, 0.f);
	float total_share = 0.f;
	bool shares = true;
	for (int b = 0; b < bone_count; ++b)
	{
		total_share += std::max(bones[b].mass_share, 0.f);
		shares = shares && bones[b].mass_share > 0.f;
	}
	for (int b = 0; b < bone_count; ++b)
	{
		const ragdoll_physics_bone &bone = bones[b];
		masses[b] = std::max(shares ? mass * bone.mass_share / total_share : mass * volumes[b] / total_volume, 0.05f);
		centres[b] = metres(bone.position) + quaternion(bone.rotation) * (0.5f * (metres(bone.capsule_start) + metres(bone.capsule_end)));
	}
	{
		float g = std::max(settings->gravity, 0.f) * k_metres / (k_tick * k_tick);
		for (int b = 1; b < bone_count; ++b)
		{
			Vec3 joint = metres(bones[b].position);
			float torque = 0.f;
			// (the bones beyond it: those whose chain of parents reaches it)
			for (int d = b; d < bone_count; ++d)
			{
				int up = d;
				while (up > b)
					up = bones[up].parent;
				if (up == b)
					torque += masses[d] * g * (centres[d] - joint).Length();
			}
			friction[b] = std::max(settings->joint_friction, 0.f) * std::max(bones[b].friction_scale, 0.f) * torque;
		}
	}

	for (int b = 0; b < bone_count; ++b)
	{
		const ragdoll_physics_bone &bone = bones[b];
		if (bone.parent >= b || (b > 0 && bone.parent < 0) || (b == 0 && bone.parent >= 0))
			return 0;

		char name[16];
		snprintf(name, sizeof(name), "%d", b);
		skeleton->AddJoint(name, bone.parent);

		Vec3 position = metres(bone.position);
		Quat rotation = quaternion(bone.rotation);
		if (!finite3(position))
			return 0;

		// the shape, in the bone's frame: a capsule along its bone, or a ball
		// (and a second capsule, a foot)
		auto capsule = [](const float *from, const float *to, float radius_wu) -> Ref<ShapeSettings>
		{
			float radius = std::max(radius_wu * k_metres, 0.01f);
			Vec3 start = metres(from);
			Vec3 end = metres(to);
			Vec3 along = end - start;
			float length = along.Length();
			if (length > 1.0e-3f)
			{
				// (Jolt's capsules lie along Y)
				Quat turn = Quat::sFromTo(Vec3::sAxisY(), along / length);
				return new RotatedTranslatedShapeSettings(0.5f * (start + end), turn, new CapsuleShapeSettings(0.5f * length, radius));
			}
			return new RotatedTranslatedShapeSettings(start, Quat::sIdentity(), new SphereShapeSettings(radius));
		};
		RefConst<Shape> shape;
		{
			Ref<ShapeSettings> main = capsule(bone.capsule_start, bone.capsule_end, bone.radius);
			if (bone.radius2 > 0.f)
			{
				Ref<StaticCompoundShapeSettings> both = new StaticCompoundShapeSettings();
				both->AddShape(Vec3::sZero(), Quat::sIdentity(), main);
				both->AddShape(Vec3::sZero(), Quat::sIdentity(), capsule(bone.capsule2_start, bone.capsule2_end, bone.radius2));
				ShapeSettings::ShapeResult result = both->Create();
				if (!result.HasError())
					shape = result.Get();
			}
			if (shape == nullptr)
			{
				ShapeSettings::ShapeResult result = main->Create();
				if (!result.HasError())
					shape = result.Get();
			}
		}
		if (shape == nullptr)
			return 0;

		RagdollSettings::Part &part = ragdoll_settings->mParts[b];
		part.SetShape(shape);
		part.mPosition = RVec3(position);
		part.mRotation = rotation;
		part.mMotionType = EMotionType::Dynamic;
		part.mObjectLayer = layers::ragdoll;
		part.mMotionQuality = EMotionQuality::LinearCast; // (no passing through thin walls at speed)
		part.mFriction = settings->friction;
		part.mRestitution = settings->restitution;
		part.mLinearDamping = settings->linear_damping;
		part.mAngularDamping = settings->angular_damping;
		part.mMaxLinearVelocity = 60.f; // (metres a second: more than anything throws a body)
		part.mAllowSleeping = true;
		part.mEnhancedInternalEdgeRemoval = true; // (slides over the level's seams)
		part.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
		part.mMassPropertiesOverride.mMass = masses[b];
		{
			Vec3 velocity = Vec3(settings->velocity[0] + bone.velocity[0], settings->velocity[1] + bone.velocity[1],
				settings->velocity[2] + bone.velocity[2]) * (k_metres / k_tick);
			Vec3 spin = Vec3(bone.angular_velocity[0], bone.angular_velocity[1], bone.angular_velocity[2]) / k_tick;
			part.mLinearVelocity = finite3(velocity) ? velocity : Vec3::sZero();
			part.mAngularVelocity = finite3(spin) ? spin : Vec3::sZero();
		}
		matrices[b] = Mat44::sRotationTranslation(rotation, position);

		// its joint, in the world as the ragdoll is made (the pose it died in
		// is the joint's middle)
		if (bone.parent >= 0)
		{
			Vec3 axis = Vec3(bone.joint_axis[0], bone.joint_axis[1], bone.joint_axis[2]);
			Vec3 normal = Vec3(bone.joint_normal[0], bone.joint_normal[1], bone.joint_normal[2]);
			if (!finite3(axis) || axis.LengthSq() < 1.0e-8f)
				axis = rotation.RotateAxisX();
			axis = axis.Normalized();
			normal = normal - normal.Dot(axis) * axis;
			if (!finite3(normal) || normal.LengthSq() < 1.0e-8f)
				normal = perpendicular(axis);
			normal = normal.Normalized();

			if (bone.joint == _ragdoll_physics_joint_hinge)
			{
				Ref<HingeConstraintSettings> hinge = new HingeConstraintSettings();
				hinge->mSpace = EConstraintSpace::WorldSpace;
				hinge->mPoint1 = hinge->mPoint2 = RVec3(position);
				hinge->mHingeAxis1 = hinge->mHingeAxis2 = axis;
				hinge->mNormalAxis1 = hinge->mNormalAxis2 = normal;
				hinge->mLimitsMin = Clamp(bone.hinge_minimum, -JPH_PI, 0.f);
				hinge->mLimitsMax = Clamp(bone.hinge_maximum, 0.f, JPH_PI);
				hinge->mMaxFrictionTorque = friction[b];
				part.mToParent = hinge;
			}
			else
			{
				Ref<SwingTwistConstraintSettings> cone = new SwingTwistConstraintSettings();
				cone->mSpace = EConstraintSpace::WorldSpace;
				cone->mPosition1 = cone->mPosition2 = RVec3(position);
				cone->mTwistAxis1 = cone->mTwistAxis2 = axis;
				cone->mPlaneAxis1 = cone->mPlaneAxis2 = normal;
				cone->mNormalHalfConeAngle = cone->mPlaneHalfConeAngle = Clamp(bone.swing_limit, 0.f, JPH_PI);
				cone->mTwistMinAngle = -Clamp(bone.twist_limit, 0.f, JPH_PI);
				cone->mTwistMaxAngle = Clamp(bone.twist_limit, 0.f, JPH_PI);
				cone->mMaxFrictionTorque = friction[b];
				part.mToParent = cone;
			}
		}
	}

	skeleton->CalculateParentJointIndices();
	if (!skeleton->AreJointsCorrectlyOrdered())
		return 0;
	ragdoll_settings->Stabilize();
	// (a part touching another as it dies, a shoulder in the chest, never
	// collides with it; nor does any part with its parent; nor, without
	// self collision, any with any)
	ragdoll_settings->DisableParentChildCollisions(matrices.data(), 0.01f);
	if (!settings->self_collision)
	{
		GroupFilterTable *table = static_cast<GroupFilterTable *>(
			const_cast<GroupFilter *>(ragdoll_settings->mParts[0].mCollisionGroup.GetGroupFilter()));
		for (int a = 0; a < bone_count; ++a)
			for (int b = a + 1; b < bone_count; ++b)
				table->DisableCollision(a, b);
	}
	ragdoll_settings->CalculateBodyIndexToConstraintIndex();
	ragdoll_settings->CalculateConstraintIndexToBodyIdxPair();

	Ragdoll *ragdoll = ragdoll_settings->CreateRagdoll((CollisionGroup::GroupID)index, k_user_ragdoll | (uint64)index, g_world.system);
	if (ragdoll == nullptr)
	{
		trace("out of bodies for a ragdoll");
		return 0;
	}
	slot->ragdoll = ragdoll;
	slot->mass = 0.f;
	for (BodyID id : ragdoll->GetBodyIDs())
	{
		BodyLockRead lock(g_world.system->GetBodyLockInterfaceNoLock(), id);
		if (lock.Succeeded())
			slot->mass += 1.f / lock.GetBody().GetMotionProperties()->GetInverseMass();
	}
	ragdoll->AddToPhysicsSystem(EActivation::Activate, false);
	return 1;
}

extern "C" void ragdoll_physics_set_environment(int index, int layer, float const *triangles, int triangle_count)
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || layer < 0 || layer >= RAGDOLL_PHYSICS_ENVIRONMENT_LAYERS)
		return;
	// (the same triangles as it has: kept, so that bodies resting on them
	// sleep, which a level made again under them would wake)
	uint64 hash = triangle_count > 0 && triangles ?
		HashBytes(triangles, (size_t)triangle_count * 9 * sizeof(float), (uint64)triangle_count + 1) : 0;
	if (hash != 0 && hash == slot->environment_hash[layer] && !slot->environment[layer].IsInvalid())
		return;
	environment_remove(*slot, layer);
	if (triangle_count <= 0 || !triangles)
		return;

	// (about the first point: the mesh's numbers small, where they are exact)
	Vec3 origin = metres(&triangles[0]);
	TriangleList list;
	list.reserve((size_t)triangle_count);
	for (int t = 0; t < triangle_count; ++t)
	{
		Vec3 a = metres(&triangles[t * 9 + 0]) - origin;
		Vec3 b = metres(&triangles[t * 9 + 3]) - origin;
		Vec3 c = metres(&triangles[t * 9 + 6]) - origin;
		if (finite3(a) && finite3(b) && finite3(c))
			list.push_back(Triangle(Float3(a.GetX(), a.GetY(), a.GetZ()), Float3(b.GetX(), b.GetY(), b.GetZ()),
				Float3(c.GetX(), c.GetY(), c.GetZ())));
	}
	if (list.empty())
		return;

	MeshShapeSettings mesh(list);
	mesh.mActiveEdgeCosThresholdAngle = std::cos(DegreesToRadians(5.f));
	ShapeSettings::ShapeResult result = mesh.Create();
	if (result.HasError())
		return; // (nothing left of it: all its triangles degenerate)

	BodyCreationSettings level(result.Get(), RVec3(origin), Quat::sIdentity(), EMotionType::Static, layers::level);
	level.mFriction = 1.f; // (the ragdoll's own friction decides: the geometric mean)
	level.mRestitution = 0.f;
	Body *body = bodies().CreateBody(level);
	if (body == nullptr)
	{
		trace("out of bodies for the level");
		return;
	}
	slot->environment[layer] = body->GetID();
	slot->environment_hash[layer] = hash;
	bodies().AddBody(body->GetID(), EActivation::DontActivate);
}

extern "C" void ragdoll_physics_step(int collision_steps, float gravity)
{
	if (!g_world.started)
		return;
	g_world.contacts.count = 0;

	bool any = false;
	for (const ragdoll_slot &slot : g_world.slots)
		any = any || slot.ragdoll != nullptr;
	if (!any)
		return;

	// (a fast vehicle moves no more than about 10 cm a step, so that it
	// meets a body rather than jumping past it)
	int vehicle_steps = (int)std::ceil(g_world.fastest_vehicle / 0.1f);
	if (collision_steps < vehicle_steps)
		collision_steps = vehicle_steps;
	if (collision_steps < 1)
		collision_steps = 1;
	if (collision_steps > 8)
		collision_steps = 8;
	g_world.system->SetGravity(Vec3(0.f, 0.f, -gravity * k_metres / (k_tick * k_tick)));
	g_world.system->Update(k_tick, collision_steps, g_world.temporary, g_world.jobs);
}

extern "C" void ragdoll_physics_get_bone(int index, int bone, float position[3], float rotation[4])
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || !slot->ragdoll || bone < 0 || bone >= (int)slot->ragdoll->GetBodyCount())
		return;
	RVec3 where;
	Quat turn;
	bodies().GetPositionAndRotation(slot->ragdoll->GetBodyID(bone), where, turn);
	world_units(Vec3(where), position);
	rotation[0] = turn.GetX();
	rotation[1] = turn.GetY();
	rotation[2] = turn.GetZ();
	rotation[3] = turn.GetW();
}

extern "C" void ragdoll_physics_get_bone_velocity(int index, int bone, float velocity[3])
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || !slot->ragdoll || bone < 0 || bone >= (int)slot->ragdoll->GetBodyCount())
		return;
	world_units(bodies().GetLinearVelocity(slot->ragdoll->GetBodyID(bone)) * k_tick, velocity);
}

extern "C" int ragdoll_physics_active(int index)
{
	ragdoll_slot *slot = slot_get(index);

	return slot && slot->ragdoll && slot->ragdoll->IsActive(false);
}

extern "C" void ragdoll_physics_hit(int index, int bone, float const point[3], float const velocity[3], float share)
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || !slot->ragdoll || bone < 0 || bone >= (int)slot->ragdoll->GetBodyCount())
		return;
	Vec3 change = metres(velocity) / k_tick;
	Vec3 at = metres(point);
	if (!finite3(change) || !finite3(at))
		return;
	share = Clamp(share, 0.f, 1.f);
	slot->ragdoll->Activate(false);
	// (the share at the point: the bone it hit swings; the rest to the whole
	// body, as its joints would hand it on)
	bodies().AddImpulse(slot->ragdoll->GetBodyID(bone), change * (slot->mass * share), RVec3(at));
	slot->ragdoll->AddLinearVelocity(change * (1.f - share), false);
}

extern "C" void ragdoll_physics_add_velocity(int index, float const velocity[3])
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || !slot->ragdoll)
		return;
	Vec3 change = metres(velocity) / k_tick;
	if (!finite3(change))
		return;
	slot->ragdoll->Activate(false);
	slot->ragdoll->AddLinearVelocity(change, false);
}

extern "C" void ragdoll_physics_add_spin(int index, float const spin[3])
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || !slot->ragdoll)
		return;
	Vec3 omega(spin[0] / k_tick, spin[1] / k_tick, spin[2] / k_tick);
	if (!finite3(omega))
		return;
	// (its centre of mass, the bodies' weighted)
	RVec3 centre = RVec3::sZero();
	float total = 0.f;
	for (BodyID id : slot->ragdoll->GetBodyIDs())
	{
		BodyLockRead lock(g_world.system->GetBodyLockInterfaceNoLock(), id);
		if (!lock.Succeeded())
			continue;
		const Body &body = lock.GetBody();
		float mass = body.GetMotionProperties()->GetInverseMass() > 0.f ? 1.f / body.GetMotionProperties()->GetInverseMass() : 0.f;
		centre += body.GetCenterOfMassPosition() * mass;
		total += mass;
	}
	if (total <= 0.f)
		return;
	centre /= total;
	slot->ragdoll->Activate(false);
	for (BodyID id : slot->ragdoll->GetBodyIDs())
	{
		RVec3 where = bodies().GetCenterOfMassPosition(id);
		Vec3 linear = bodies().GetLinearVelocity(id) + omega.Cross(Vec3(where - centre));
		Vec3 angular = bodies().GetAngularVelocity(id) + omega;
		bodies().SetLinearAndAngularVelocity(id, linear, angular);
	}
}

extern "C" void ragdoll_physics_translate(int index, float const offset[3])
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || !slot->ragdoll)
		return;
	Vec3 move = metres(offset);
	if (!finite3(move))
		return;
	for (BodyID id : slot->ragdoll->GetBodyIDs())
	{
		RVec3 where = bodies().GetPosition(id);
		bodies().SetPosition(id, where + move, EActivation::DontActivate);
	}
}

extern "C" void ragdoll_physics_bounds(int index, float center[3], float *radius)
{
	ragdoll_slot *slot = slot_get(index);

	if (!slot || !slot->ragdoll)
		return;
	AABox box = slot->ragdoll->GetWorldSpaceBounds(false);
	world_units(Vec3(box.GetCenter()), center);
	*radius = 0.5f * box.GetSize().Length() / k_metres;
}

/* ---------- vehicles */

extern "C" void ragdoll_physics_vehicles_begin(void)
{
	if (!g_world.started)
		return;
	for (vehicle_slot &vehicle : g_world.vehicles)
		vehicle.given = false;
	g_world.fastest_vehicle = 0.f;
}

extern "C" void ragdoll_physics_vehicles_end(void)
{
	if (!g_world.started)
		return;
	for (vehicle_slot &vehicle : g_world.vehicles)
	{
		if (vehicle.key != -1 && !vehicle.given)
		{
			if (!vehicle.id.IsInvalid())
			{
				bodies().RemoveBody(vehicle.id);
				bodies().DestroyBody(vehicle.id);
			}
			vehicle = vehicle_slot();
		}
	}
}

extern "C" void ragdoll_physics_vehicle(long key, float const *points, int point_count, float const position[3],
	float const rotation[4], float const velocity[3], float const angular_velocity[3])
{
	if (!ragdoll_physics_initialize() || key == -1 || !points || point_count < 4)
		return;

	vehicle_slot *vehicle = nullptr;
	for (vehicle_slot &each : g_world.vehicles)
		if (each.key == key)
			vehicle = &each;
	if (!vehicle)
		for (vehicle_slot &each : g_world.vehicles)
			if (each.key == -1)
			{
				vehicle = &each;
				break;
			}
	if (!vehicle)
		return; // (more vehicles near the ragdolls than it keeps)

	Vec3 where = metres(position);
	Quat turn = quaternion(rotation);
	Vec3 moving = metres(velocity) / k_tick;
	Vec3 spinning = Vec3(angular_velocity[0], angular_velocity[1], angular_velocity[2]) / k_tick;
	if (!finite3(where) || !finite3(moving) || !finite3(spinning))
		return;

	// its shape: made again only when its points change (a turret turned)
	uint64 hash = HashBytes(points, (size_t)point_count * 3 * sizeof(float), (uint64)point_count + 7);
	if (vehicle->id.IsInvalid() || hash != vehicle->hash)
	{
		Array<Vec3> hull;
		hull.reserve((size_t)point_count);
		for (int p = 0; p < point_count; ++p)
		{
			Vec3 point = metres(&points[p * 3]);
			if (finite3(point))
				hull.push_back(point);
		}
		ConvexHullShapeSettings settings(hull, 0.02f);
		settings.mHullTolerance = 0.02f;
		ShapeSettings::ShapeResult result = settings.Create();
		if (result.HasError())
			return;
		if (!vehicle->id.IsInvalid())
		{
			bodies().RemoveBody(vehicle->id);
			bodies().DestroyBody(vehicle->id);
			vehicle->id = BodyID();
		}
		BodyCreationSettings creation(result.Get(), RVec3(where), turn, EMotionType::Kinematic, layers::ragdoll);
		creation.mFriction = 0.6f;
		creation.mRestitution = 0.1f;
		creation.mAllowSleeping = false;
		creation.mUserData = k_user_vehicle | (uint64)(uint32)key;
		Body *body = bodies().CreateBody(creation);
		if (body == nullptr)
			return;
		vehicle->id = body->GetID();
		vehicle->hash = hash;
		bodies().AddBody(vehicle->id, EActivation::Activate);
	}
	vehicle->key = key;
	vehicle->given = true;

	// where the game has it now, moving as the game moves it: Jolt carries
	// it on through the tick, and the next puts it where the game has it then
	bodies().SetPositionRotationAndVelocity(vehicle->id, RVec3(where), turn, moving, spinning);
	if (moving.LengthSq() > 1.0e-6f || spinning.LengthSq() > 1.0e-6f)
		bodies().ActivateBody(vehicle->id);
	g_world.fastest_vehicle = std::max(g_world.fastest_vehicle, moving.Length() * k_tick);
}

static vehicle_slot *vehicle_find(long key)
{
	for (vehicle_slot &each : g_world.vehicles)
		if (each.key == key && !each.id.IsInvalid())
			return &each;
	return nullptr;
}

extern "C" int ragdoll_physics_vehicle_contacts(struct ragdoll_physics_contact *contacts, int maximum)
{
	if (!g_world.started || !contacts || maximum <= 0)
		return 0;
	int count = std::min(g_world.contacts.count, maximum);
	for (int index = 0; index < count; ++index)
		contacts[index] = g_world.contacts.contacts[index];
	return count;
}
