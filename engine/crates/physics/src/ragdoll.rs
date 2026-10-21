//! Ragdolls: a dead biped's parts, each a body joined to an earlier one
//! by a swing-twist joint, through physics.h's `physics_ragdoll_*`.

use glam::{Quat, Vec3};

use crate::{sys, units_to_metres, Material, PhysicsError, Pose, Shape, World, METRES_PER_UNIT};

/// The most parts a ragdoll has.
pub const RAGDOLL_PARTS_MAX: usize = sys::RAGDOLL_PARTS_MAX;

/// A ragdoll's part: one body, joined to an earlier part (its parent) at
/// its own frame's origin. Its joint twists about `twist_axis` and swings
/// about `plane_axis` and the normal (their cross product), each in the
/// part's frame; the angles are in radians.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct RagdollPart {
    /// The part it hangs from, earlier in the list; none for the first.
    pub parent: Option<usize>,
    /// Its shape.
    pub shape: Shape,
    /// Where the shape sits in the part's frame.
    pub offset: Pose,
    /// Its mass, in kilograms.
    pub mass: f32,
    /// The axis its joint twists about.
    pub twist_axis: Vec3,
    /// The axis its joint swings in the plane of.
    pub plane_axis: Vec3,
    /// The least and most twist, -π to 0 and 0 to π.
    pub twist: [f32; 2],
    /// Half the swing cone's angle about the normal, 0 to π.
    pub swing_normal: f32,
    /// Half the swing cone's angle about the plane axis, 0 to π.
    pub swing_plane: f32,
}

impl RagdollPart {
    fn raw(&self) -> sys::Part {
        let mut shape = self.shape.raw();
        // (the shape's own turn, then the part's offset)
        let inner = Quat::from_array(shape.offset.rotation);
        shape.offset = Pose {
            position: self.offset.position,
            rotation: self.offset.rotation * inner,
        }
        .raw();
        sys::Part {
            parent: self
                .parent
                .and_then(|parent| i32::try_from(parent).ok())
                .unwrap_or(-1),
            shape,
            mass: self.mass,
            twist_axis: self.twist_axis.to_array(),
            plane_axis: self.plane_axis.to_array(),
            twist_min: self.twist[0],
            twist_max: self.twist[1],
            swing_normal: self.swing_normal,
            swing_plane: self.swing_plane,
        }
    }
}

/// A ragdoll, by its handle.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct RagdollId(u32);

fn raw_poses(poses: &[Pose]) -> Vec<sys::Pose> {
    poses.iter().map(Pose::raw).collect()
}

impl World {
    /// Adds a ragdoll of `parts`, built at `rest` (each part's frame with
    /// its joints at rest, which the limits centre on) and placed at `pose`
    /// (each part's frame now). A part does not collide with its parent.
    pub fn add_ragdoll(
        &mut self,
        parts: &[RagdollPart],
        rest: &[Pose],
        pose: &[Pose],
        material: Material,
    ) -> Result<RagdollId, PhysicsError> {
        let Ok(count) = u32::try_from(parts.len()) else {
            return Err(PhysicsError::Body);
        };
        if rest.len() != parts.len() || pose.len() != parts.len() {
            return Err(PhysicsError::Body);
        }
        let raw: Vec<sys::Part> = parts.iter().map(RagdollPart::raw).collect();
        let rest = raw_poses(rest);
        let pose = raw_poses(pose);
        let material = sys::Material {
            friction: material.friction,
            restitution: material.restitution,
        };
        // SAFETY: the arrays hold `count` each, and the call copies them.
        let ragdoll = unsafe {
            sys::physics_ragdoll_add(
                self.raw.as_ptr(),
                raw.as_ptr(),
                count,
                rest.as_ptr(),
                pose.as_ptr(),
                &material,
            )
        };
        if ragdoll == sys::NONE {
            Err(PhysicsError::Body)
        } else {
            Ok(RagdollId(ragdoll))
        }
    }

    /// Removes a ragdoll (nothing, if it names none).
    pub fn remove_ragdoll(&mut self, ragdoll: RagdollId) {
        // SAFETY: the world is live; the handle is checked.
        unsafe { sys::physics_ragdoll_remove(self.raw.as_ptr(), ragdoll.0) }
    }

    /// Each of a ragdoll's `count` parts' frames in the world.
    pub fn ragdoll_poses(
        &self,
        ragdoll: RagdollId,
        count: usize,
    ) -> Result<Vec<Pose>, PhysicsError> {
        let Ok(raw_count) = u32::try_from(count) else {
            return Err(PhysicsError::Refused);
        };
        let mut poses = vec![sys::Pose::default(); count];
        // SAFETY: the array holds `count`.
        let result = unsafe {
            sys::physics_ragdoll_get_poses(
                self.raw.as_ptr(),
                ragdoll.0,
                poses.as_mut_ptr(),
                raw_count,
            )
        };
        if result == 0 {
            Ok(poses.iter().map(Pose::from_raw).collect())
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// Drives each joint's motor toward its part's rotation in `target`
    /// (world frames, as [`World::ragdoll_poses`] gives), springing at
    /// `frequency` (Hz), until [`World::ragdoll_go_limp`].
    pub fn drive_ragdoll(
        &mut self,
        ragdoll: RagdollId,
        target: &[Pose],
        frequency: f32,
    ) -> Result<(), PhysicsError> {
        let Ok(count) = u32::try_from(target.len()) else {
            return Err(PhysicsError::Refused);
        };
        let target = raw_poses(target);
        // SAFETY: the array holds `count`.
        let result = unsafe {
            sys::physics_ragdoll_drive(
                self.raw.as_ptr(),
                ragdoll.0,
                target.as_ptr(),
                count,
                frequency,
            )
        };
        refused_unless_zero(result)
    }

    /// Turns a ragdoll's motors off: only its joints' friction holds it.
    pub fn ragdoll_go_limp(&mut self, ragdoll: RagdollId) -> Result<(), PhysicsError> {
        // SAFETY: the world is live; the handle is checked.
        refused_unless_zero(unsafe { sys::physics_ragdoll_go_limp(self.raw.as_ptr(), ragdoll.0) })
    }

    /// Pushes a ragdoll's part: an impulse in kilogram world units a
    /// second, at a point in the world.
    pub fn push_ragdoll(
        &mut self,
        ragdoll: RagdollId,
        part: usize,
        impulse: Vec3,
        point: Vec3,
    ) -> Result<(), PhysicsError> {
        let Ok(part) = u32::try_from(part) else {
            return Err(PhysicsError::Refused);
        };
        let impulse = units_to_metres(impulse);
        let point = units_to_metres(point);
        // SAFETY: the arrays hold three each.
        let result = unsafe {
            sys::physics_ragdoll_add_impulse(
                self.raw.as_ptr(),
                ragdoll.0,
                part,
                impulse.as_ptr(),
                point.as_ptr(),
            )
        };
        refused_unless_zero(result)
    }

    /// Sets every part of a ragdoll moving at `linear`, in world units a
    /// second.
    pub fn set_ragdoll_velocity(
        &mut self,
        ragdoll: RagdollId,
        linear: Vec3,
    ) -> Result<(), PhysicsError> {
        let linear = units_to_metres(linear);
        // SAFETY: the array holds three.
        let result = unsafe {
            sys::physics_ragdoll_set_velocity(self.raw.as_ptr(), ragdoll.0, linear.as_ptr())
        };
        refused_unless_zero(result)
    }

    /// Breaks the joint between a part and its parent: the part and its
    /// children go on as a limb of their own. Refused for the first part,
    /// one out of range, or a joint already broken.
    pub fn break_ragdoll_joint(
        &mut self,
        ragdoll: RagdollId,
        part: usize,
    ) -> Result<(), PhysicsError> {
        let Ok(part) = u32::try_from(part) else {
            return Err(PhysicsError::Refused);
        };
        // SAFETY: the world is live; the handle and part are checked.
        refused_unless_zero(unsafe {
            sys::physics_ragdoll_break_joint(self.raw.as_ptr(), ragdoll.0, part)
        })
    }

    /// Whether the joint between a part and its parent is broken.
    pub fn ragdoll_joint_broken(
        &self,
        ragdoll: RagdollId,
        part: usize,
    ) -> Result<bool, PhysicsError> {
        let Ok(part) = u32::try_from(part) else {
            return Err(PhysicsError::Refused);
        };
        // SAFETY: the world is live; the handle and part are checked.
        match unsafe { sys::physics_ragdoll_joint_broken(self.raw.as_ptr(), ragdoll.0, part) } {
            1 => Ok(true),
            0 => Ok(false),
            _ => Err(PhysicsError::Refused),
        }
    }

    /// Whether any part of a ragdoll is awake.
    pub fn ragdoll_awake(&self, ragdoll: RagdollId) -> Result<bool, PhysicsError> {
        // SAFETY: the world is live; the handle is checked.
        match unsafe { sys::physics_ragdoll_active(self.raw.as_ptr(), ragdoll.0) } {
            1 => Ok(true),
            0 => Ok(false),
            _ => Err(PhysicsError::Refused),
        }
    }

    /// The furthest any whole joint of a ragdoll is past its limits, in
    /// radians (0 inside them).
    pub fn ragdoll_limit_error(&self, ragdoll: RagdollId) -> Result<f32, PhysicsError> {
        // SAFETY: the world is live; the handle is checked.
        let error = unsafe { sys::physics_ragdoll_limit_error(self.raw.as_ptr(), ragdoll.0) };
        if error < 0.0 {
            Err(PhysicsError::Refused)
        } else {
            Ok(error)
        }
    }
}

fn refused_unless_zero(result: i32) -> Result<(), PhysicsError> {
    if result == 0 {
        Ok(())
    } else {
        Err(PhysicsError::Refused)
    }
}

/// A shape's volume, in cubic world units.
pub fn shape_volume(shape: &Shape) -> f32 {
    let pi = std::f32::consts::PI;
    match *shape {
        Shape::Sphere { radius } => 4.0 / 3.0 * pi * radius * radius * radius,
        Shape::Capsule { radius, height } => pi * radius * radius * (4.0 / 3.0 * radius + height),
        Shape::Box { half_extents } => 8.0 * half_extents.x * half_extents.y * half_extents.z,
    }
}

/// Metres in a world unit, for masses worked out from volumes.
pub const METRES_PER_UNIT_CUBED: f32 = METRES_PER_UNIT * METRES_PER_UNIT * METRES_PER_UNIT;
