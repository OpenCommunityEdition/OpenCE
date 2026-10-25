//! Platforms: the machines (lifts, doors, gates) as kinematic bodies in the
//! player's physics world, a box for each node of their collision models,
//! moved each tick to where the machine's animation put that node. What
//! stands on one is carried with it, and what one meets is pushed ahead.

use bevy_ecs::prelude::{With, World as Ecs};
use glam::{Mat3, Quat, Vec3};
use opence_content::{Level, ObjectKind, TagRef};
use opence_physics::{BodyId, Material, Motion, Pose, Shape, World as PhysicsWorld};

use crate::animation::{NodeMatrices, NodeMatrix};
use crate::cached::Cached;
use crate::clock;
use crate::objects::{Device, Object, ObjectHandle};
use crate::ragdoll::node_bounds;
use crate::Transform;

/// A platform's mass, in kilograms; the physics moves it whatever it
/// meets, so this only weighs it against what it pushes.
const MASS: f32 = 1000.0;
/// Half a platform's size at the least, along each axis, in world units.
const HALF_MIN: f32 = 0.01;

/// A box of a machine's collision model.
#[derive(Clone, Debug, PartialEq)]
struct Piece {
    /// The model's node it moves with (its index in the node matrices).
    node: usize,
    /// Its centre, in that node's space.
    centre: Vec3,
    /// Half its size along each of the node's axes.
    half_extents: Vec3,
}

/// A piece of a machine in the physics world.
#[derive(Clone, Debug, PartialEq)]
struct Platform {
    object: ObjectHandle,
    piece: usize,
    body: BodyId,
    pose: Pose,
}

/// The machines of a game as platforms.
#[derive(Clone, Debug, Default)]
pub struct Platforms {
    /// Each machine definition's pieces.
    pieces: Vec<(TagRef, Vec<Piece>)>,
    platforms: Vec<Platform>,
    /// Whether any moved this tick, or the last (when they came to rest,
    /// and the physics still takes their speed away).
    moved: [bool; 2],
    /// The machines (a kept query).
    machines: Cached<Machines, With<Device>>,
}

/// What [`Platforms::update`] reads of each machine.
type Machines = (
    &'static ObjectHandle,
    &'static Object,
    &'static Transform,
    Option<&'static NodeMatrices>,
);

impl Platforms {
    /// `level`'s machines' pieces, and none in the physics yet.
    pub fn for_level(level: &Level) -> Platforms {
        let pieces = level
            .objects
            .iter()
            .filter(|object| object.kind == ObjectKind::Machine)
            .filter_map(|object| {
                let collision = level.collision_model(object.collision_model.as_ref()?)?;
                let model = object
                    .model
                    .as_ref()
                    .and_then(|tag| level.model_index(tag))
                    .and_then(|index| level.models.get(index));
                let pieces: Vec<Piece> = collision
                    .nodes
                    .iter()
                    .enumerate()
                    .filter_map(|(index, node)| {
                        let (minimum, maximum) = node_bounds(node)?;
                        // (its model's node of the same name, else the
                        // same index)
                        let node_index = model
                            .and_then(|model| {
                                model.nodes.iter().position(|other| other.name == node.name)
                            })
                            .unwrap_or(index);
                        Some(Piece {
                            node: node_index,
                            centre: 0.5 * (minimum + maximum),
                            half_extents: (0.5 * (maximum - minimum)).max(Vec3::splat(HALF_MIN)),
                        })
                    })
                    .collect();
                (!pieces.is_empty()).then(|| (object.tag.clone(), pieces))
            })
            .collect();
        Platforms {
            pieces,
            platforms: Vec::new(),
            moved: [false; 2],
            machines: Cached::default(),
        }
    }

    /// The platforms in the physics.
    pub fn len(&self) -> usize {
        self.platforms.len()
    }

    /// Whether there are none.
    pub fn is_empty(&self) -> bool {
        self.platforms.is_empty()
    }

    /// Whether any moved this tick or the last (the physics steps for it).
    pub fn moving(&self) -> bool {
        self.moved.iter().any(|&moved| moved)
    }

    /// Where each platform is, its body and its pose.
    pub fn poses(&self) -> impl Iterator<Item = (BodyId, Pose)> + '_ {
        self.platforms
            .iter()
            .map(|platform| (platform.body, platform.pose))
    }

    /// Before the physics steps: adds a platform for each new machine's
    /// pieces, moves each to where its machine's node is now, and removes
    /// those whose machine is gone. In the order of the machines' handles,
    /// so that every machine adds its bodies in the same order.
    pub fn update(&mut self, world: &mut Ecs, physics: &mut PhysicsWorld) {
        let mut machines: Vec<(ObjectHandle, TagRef, Transform, Option<Vec<NodeMatrix>>)> = self
            .machines
            .get(world)
            .iter(world)
            .filter(|(_, object, _, _)| object.kind == ObjectKind::Machine)
            .map(|(&handle, object, &transform, nodes)| {
                (
                    handle,
                    object.tag.clone(),
                    transform,
                    nodes.map(|nodes| nodes.current.clone()),
                )
            })
            .collect();
        machines.sort_by_key(|(handle, ..)| *handle);
        let mut moved = false;
        let mut kept = Vec::with_capacity(self.platforms.len());
        for (handle, tag, transform, nodes) in &machines {
            let Some((_, pieces)) = self.pieces.iter().find(|(other, _)| other == tag) else {
                continue;
            };
            for (index, piece) in pieces.iter().enumerate() {
                let Some(pose) = place(piece, transform, nodes.as_deref()) else {
                    continue;
                };
                let earlier = self
                    .platforms
                    .iter()
                    .position(|platform| platform.object == *handle && platform.piece == index);
                match earlier {
                    Some(earlier) => {
                        let mut platform = self.platforms.swap_remove(earlier);
                        if platform.pose != pose {
                            moved = true;
                        }
                        // (moved every tick, so that one come to rest has
                        // its speed taken away)
                        if physics
                            .move_body(platform.body, pose, clock::TICK.0)
                            .is_ok()
                        {
                            platform.pose = pose;
                        }
                        kept.push(platform);
                    }
                    None => {
                        let scale = transform.scale.max(HALF_MIN);
                        let Ok(body) = physics.add_body(
                            Shape::Box {
                                half_extents: piece.half_extents * scale,
                            },
                            pose,
                            Motion::Kinematic,
                            MASS,
                            Material {
                                friction: 0.8,
                                restitution: 0.0,
                            },
                        ) else {
                            continue;
                        };
                        kept.push(Platform {
                            object: *handle,
                            piece: index,
                            body,
                            pose,
                        });
                    }
                }
            }
        }
        // (the machines gone, or no longer placed)
        for gone in std::mem::replace(&mut self.platforms, kept) {
            physics.remove_body(gone.body);
        }
        self.moved = [moved, self.moved[0]];
    }

    /// Removes every platform.
    pub fn clear(&mut self, physics: &mut PhysicsWorld) {
        for one in self.platforms.drain(..) {
            physics.remove_body(one.body);
        }
        self.moved = [false; 2];
    }

    /// Forgets every platform, whose bodies went with their physics world.
    pub fn forget(&mut self) {
        self.platforms.clear();
        self.moved = [false; 2];
    }
}

/// Where `piece` is: by its node's matrix if the machine is posed, else, for
/// its root node's pieces only, by the machine's transform; none for a node
/// it does not have or a pose that is not a number.
fn place(piece: &Piece, transform: &Transform, nodes: Option<&[NodeMatrix]>) -> Option<Pose> {
    let (position, rotation, scale) = match nodes {
        Some(nodes) => {
            let matrix = nodes.get(piece.node)?;
            let scale = matrix.scale;
            if scale.is_nan() || scale <= 0.0 {
                return None;
            }
            let axes = Mat3::from_cols(
                Vec3::from_array(matrix.forward),
                Vec3::from_array(matrix.left),
                Vec3::from_array(matrix.up),
            ) * (1.0 / scale);
            (
                Vec3::from_array(matrix.position),
                Quat::from_mat3(&axes).normalize(),
                scale,
            )
        }
        None if piece.node == 0 => (transform.position, transform.rotation, transform.scale),
        None => return None,
    };
    let pose = Pose {
        position: position + rotation * (piece.centre * scale),
        rotation,
    };
    (pose.position.is_finite() && pose.rotation.is_finite()).then_some(pose)
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::level::{CollisionEdge, CollisionSurface, CollisionVertex};
    use opence_content::{CollisionBsp, CollisionModel, CollisionNode, Group, ObjectDefinition};
    use opence_physics::{CharacterSettings, Ground, Layers, LevelMesh, WorldSettings};

    use crate::objects::{ObjectKinds, ObjectSpawn, ObjectTable};

    type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

    const LIFT: &str = "levels\\test\\lift";
    /// Half the lift's size.
    const HALF: Vec3 = Vec3::new(1.5, 1.5, 0.1);
    /// How fast the lift rises, in world units a second.
    const RISE: f32 = 1.0;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    /// A collision bsp of `points` (only its vertices are read).
    fn bsp(points: &[[f32; 3]]) -> CollisionBsp {
        let mut bsp = CollisionBsp::default();
        for (index, &point) in points.iter().enumerate() {
            bsp.vertices.push(CollisionVertex {
                point,
                first_edge: index as u32,
            });
        }
        bsp
    }

    /// A floor 40 units square at z = 0.
    fn floor() -> CollisionBsp {
        let mut bsp = bsp(&[
            [-20.0, -20.0, 0.0],
            [20.0, -20.0, 0.0],
            [20.0, 20.0, 0.0],
            [-20.0, 20.0, 0.0],
        ]);
        bsp.planes.push([0.0, 0.0, 1.0, 0.0]);
        for index in 0..4u32 {
            bsp.edges.push(CollisionEdge {
                vertices: [index, (index + 1) % 4],
                edges: [(index + 1) % 4, 0],
                surfaces: [Some(0), None],
            });
        }
        bsp.surfaces.push(CollisionSurface {
            plane: 0,
            first_edge: 0,
            flags: 0,
            breakable_surface: 0,
            material: None,
        });
        bsp
    }

    fn collision_model() -> CollisionModel {
        CollisionModel {
            tag: tag(b"coll", LIFT),
            flags: 0,
            indirect_damage_material: None,
            friendly_damage_resistance: 0.0,
            body_damaged_threshold: 0.0,
            body_destroyed_threshold: 0.0,
            shield_material_type: 0,
            shield_failure_function: 0,
            shield_failure_threshold: 0.0,
            failing_shield_leak_fraction: 0.0,
            minimum_stun_damage: 0.0,
            stun_time: 0.0,
            recharge_time: 0.0,
            shield_damaged_threshold: 0.0,
            maximum_body_vitality: 1.0,
            maximum_shield_vitality: 0.0,
            materials: Vec::new(),
            regions: Vec::new(),
            pathfinding_box: Default::default(),
            pathfinding_spheres: Vec::new(),
            nodes: vec![CollisionNode {
                name: "frame".into(),
                region: None,
                parent: None,
                next_sibling: None,
                first_child: None,
                // (a slab, its top at the node's origin)
                bsps: vec![bsp(&[
                    [-HALF.x, -HALF.y, -2.0 * HALF.z],
                    [HALF.x, HALF.y, 0.0],
                ])],
            }],
        }
    }

    fn definition(kind: ObjectKind, path: &str, collision: bool) -> ObjectDefinition {
        ObjectDefinition {
            widgets: Vec::new(),
            tag: tag(&kind.group(), path),
            kind,
            flags: 0,
            bounding_radius: 2.0,
            bounding_offset: [0.0; 3],
            acceleration_scale: 1.0,
            render_bounding_radius: 2.0,
            icon_text_index: -1,
            model: None,
            animation_graph: None,
            collision_model: collision.then(|| tag(b"coll", path)),
            physics: None,
            modifier_shader: None,
            creation_effect: None,
            function_modes: Default::default(),
            functions: Vec::new(),
            attachments: Vec::new(),
            change_colors: Vec::new(),
            unit_hud: None,
            material_effects: None,
            unit_melee_damage: None,
            biped: None,
            weapon: None,
            projectile: None,
            equipment: None,
            vehicle: None,
            device: None,
        }
    }

    /// The devmap with a lift (a machine with a slab for its collision)
    /// and a control of the same model, which is no platform.
    fn level() -> Level {
        let mut level = opence_content::devmap::level(None);
        level
            .objects
            .push(definition(ObjectKind::Machine, LIFT, true));
        level
            .objects
            .push(definition(ObjectKind::Control, LIFT, true));
        level.collision_models.push(collision_model());
        level
    }

    struct Scene {
        ecs: Ecs,
        physics: PhysicsWorld,
        platforms: Platforms,
        lift: ObjectHandle,
    }

    fn scene(at: Vec3) -> Result<Scene> {
        let level = level();
        let mut ecs = Ecs::new();
        ecs.insert_resource(ObjectTable::default());
        ecs.insert_resource(ObjectKinds::default());
        let mut spawn = |kind: ObjectKind, position: Vec3| {
            crate::objects::spawn(
                &mut ecs,
                ObjectSpawn {
                    kind,
                    tag: tag(&kind.group(), LIFT),
                    transform: Transform {
                        position,
                        ..Transform::default()
                    },
                    name: None,
                    permutation: None,
                    team: None,
                },
            )
        };
        let lift = spawn(ObjectKind::Machine, at)?;
        spawn(ObjectKind::Control, at + Vec3::new(10.0, 0.0, 0.0))?;
        let mut physics = PhysicsWorld::new(WorldSettings::default())?;
        physics.set_level(&LevelMesh::from_collision(&floor()))?;
        Ok(Scene {
            ecs,
            physics,
            platforms: Platforms::for_level(&level),
            lift,
        })
    }

    impl Scene {
        fn move_lift(&mut self, position: Vec3) -> Result {
            let entity = self
                .ecs
                .resource::<ObjectTable>()
                .entity(self.lift)
                .ok_or("no lift")?;
            let mut transform = self
                .ecs
                .get_mut::<Transform>(entity)
                .ok_or("no transform")?;
            transform.position = position;
            Ok(())
        }

        fn tick(&mut self) -> Result {
            self.platforms.update(&mut self.ecs, &mut self.physics);
            self.physics.step(clock::TICK.0)?;
            Ok(())
        }
    }

    #[test]
    fn only_machines_with_a_collision_model_are_platforms() -> Result {
        let mut scene = scene(Vec3::new(0.0, 0.0, 1.0))?;
        assert!(scene.platforms.is_empty());
        scene.tick()?;
        assert_eq!(scene.platforms.len(), 1);
        let (body, pose) = scene.platforms.poses().next().ok_or("no platform")?;
        assert!(pose
            .position
            .abs_diff_eq(Vec3::new(0.0, 0.0, 1.0 - HALF.z), 1e-5));
        let hit = scene
            .physics
            .cast_ray(
                Vec3::new(0.0, 0.0, 3.0),
                Vec3::new(0.0, 0.0, -5.0),
                Layers::PLATFORMS,
            )?
            .ok_or("the lift is not in the physics")?;
        assert_eq!(hit.struck, opence_physics::Struck::Body(body));
        // (standing still, after a tick for its speed to go)
        assert!(!scene.platforms.moving());
        Ok(())
    }

    #[test]
    fn a_rising_lift_carries_a_character_and_a_box() -> Result {
        let start = Vec3::new(0.0, 0.0, 0.5);
        let mut scene = scene(start)?;
        scene.tick()?;
        let character = scene.physics.add_character(
            &CharacterSettings {
                radius: 0.2,
                height: 0.7,
                maximum_slope: 0.8,
                step_up: 0.1,
                stick_down: 0.1,
                layers: Layers::GROUND,
            },
            Vec3::new(-0.7, 0.0, start.z + 0.01),
        )?;
        let box_ = scene.physics.add_body(
            Shape::Box {
                half_extents: Vec3::splat(0.2),
            },
            Pose::at(Vec3::new(0.7, 0.0, start.z + 0.21)),
            Motion::Dynamic,
            20.0,
            Material {
                friction: 0.8,
                restitution: 0.0,
            },
        )?;
        let mut state = scene.physics.character_state(character)?;
        let mut at = start;
        let ticks = 2 * clock::TICKS_PER_SECOND;
        for tick in 0..clock::TICKS_PER_SECOND + ticks {
            if tick >= clock::TICKS_PER_SECOND {
                at.z += RISE * clock::TICK.0;
                scene.move_lift(at)?;
            }
            let fall = if state.ground == Ground::Standing {
                Vec3::ZERO
            } else {
                Vec3::new(0.0, 0.0, -3.0)
            };
            state = scene
                .physics
                .move_character(character, fall, clock::TICK.0)?;
            scene.tick()?;
            if tick >= clock::TICKS_PER_SECOND {
                assert!(scene.platforms.moving());
            }
        }
        assert_eq!(state.ground, Ground::Standing);
        assert!(
            (state.position.z - at.z).abs() < 0.1,
            "the character is at {} on a lift at {at}",
            state.position
        );
        let box_at = scene.physics.body_pose(box_)?.position;
        assert!(
            (box_at.z - (at.z + 0.2)).abs() < 0.1,
            "the box is at {box_at} on a lift at {at}"
        );
        Ok(())
    }

    #[test]
    fn a_removed_machine_takes_its_platform_away() -> Result {
        let mut scene = scene(Vec3::new(0.0, 0.0, 1.0))?;
        scene.tick()?;
        assert_eq!(scene.platforms.len(), 1);
        assert!(crate::objects::remove(&mut scene.ecs, scene.lift));
        scene.tick()?;
        assert!(scene.platforms.is_empty());
        assert_eq!(
            scene.physics.cast_ray(
                Vec3::new(0.0, 0.0, 3.0),
                Vec3::new(0.0, 0.0, -5.0),
                Layers::PLATFORMS
            )?,
            None
        );
        Ok(())
    }

    #[test]
    fn a_posed_machine_places_its_pieces_by_their_nodes() {
        let piece = Piece {
            node: 1,
            centre: Vec3::new(1.0, 0.0, 0.0),
            half_extents: Vec3::ONE,
        };
        let turned = NodeMatrix {
            forward: [0.0, 2.0, 0.0],
            left: [-2.0, 0.0, 0.0],
            up: [0.0, 0.0, 2.0],
            position: [5.0, 0.0, 0.0],
            scale: 2.0,
        };
        let nodes = [NodeMatrix::IDENTITY, turned];
        let pose = place(&piece, &Transform::default(), Some(&nodes));
        let pose = pose.unwrap_or(Pose::at(Vec3::NAN));
        assert!(
            pose.position.abs_diff_eq(Vec3::new(5.0, 2.0, 0.0), 1e-5),
            "{}",
            pose.position
        );
        assert!((pose.rotation * Vec3::X).abs_diff_eq(Vec3::Y, 1e-5));
        // (a node the machine does not have, or a root-less piece unposed)
        let far = Piece {
            node: 7,
            ..piece.clone()
        };
        assert_eq!(place(&far, &Transform::default(), Some(&nodes)), None);
        assert_eq!(place(&piece, &Transform::default(), None), None);
    }
}
