//! What the level sounds like around the camera: the looping sounds its
//! objects carry and its rooms' backgrounds, each heard through the walls
//! between it and the camera (`sound_source_obstruction`), and the reverb
//! of the room the camera is in, glided to as it moves from room to room
//! (`sound_environment_update`).

use std::collections::BTreeMap;

use opence_audio::Environment;
use opence_content::{Level, StructureBsp, TagRef, MAXIMUM_CLUSTER_SOUND_DISTANCE};
use opence_sim::{Simulation, Vec3};

/// The key the background sound of the camera's room loops under.
pub(crate) const BACKGROUND_KEY: u64 = u64::MAX;
/// The game's ticks a second.
const TICKS_PER_SECOND: f32 = 30.0;
/// A source behind a wall: its direct path is so much blocked, and it is
/// so much shut away (`sound_source_obstruction`).
const OBSTRUCTED: f32 = 0.6;
const OCCLUDED: f32 = 1.0;
/// A source behind a wall in a room near the camera's is less blocked.
const OBSTRUCTED_NEARBY: f32 = 0.45;
/// How much more shut away a source is than its distance alone says.
const OCCLUSION_SCALE: f32 = 1.4;

/// A looping sound to keep playing this frame.
#[derive(Clone, Debug, PartialEq)]
pub(crate) struct Looping {
    /// The key it plays under: its object's and attachment's, or the
    /// background's.
    pub key: u64,
    /// The looping sound.
    pub tag: TagRef,
    /// Where it is; `None` for one heard everywhere alike.
    pub position: Option<Vec3>,
    /// Its velocity, in world units a second.
    pub velocity: Vec3,
    /// How much of its direct path is blocked, 0 to 1.
    pub obstruction: f32,
    /// How much it is shut away, 0 to 1.
    pub occlusion: f32,
}

/// The level's sound around the camera.
#[derive(Clone, Debug)]
pub(crate) struct SoundWorld {
    /// Each object definition's looping sounds, by their attachments'
    /// indices.
    carried: BTreeMap<TagRef, Vec<(usize, TagRef)>>,
    /// The environment heard, gliding toward the camera room's.
    heard: Environment,
    /// Where the camera was at the last update.
    camera: Option<Vec3>,
    /// The camera's velocity.
    velocity: Vec3,
}

impl Default for SoundWorld {
    fn default() -> Self {
        Self {
            carried: BTreeMap::new(),
            heard: Environment::DEFAULT,
            camera: None,
            velocity: Vec3::ZERO,
        }
    }
}

/// The cluster of `bsp` that `point` is in.
fn cluster_at(bsp: &StructureBsp, point: Vec3) -> Option<u16> {
    bsp.collision
        .leaf_at(point.to_array())
        .and_then(|leaf| *bsp.leaf_clusters.get(leaf as usize)?)
}

/// How blocked and shut away a source `distance` from the listener is,
/// `cluster_distance` away by the BSP's sound paths, when the line
/// between them is `clear` or not: (obstruction, occlusion).
pub(crate) fn obstruction(clear: bool, cluster_distance: f32, distance: f32) -> (f32, f32) {
    if clear {
        return (0.0, 0.0);
    }
    let obstruction = if cluster_distance < MAXIMUM_CLUSTER_SOUND_DISTANCE {
        OBSTRUCTED_NEARBY
    } else {
        OBSTRUCTED
    };
    let total = cluster_distance + distance;
    let occlusion = if total > 0.0 {
        ((1.0 - distance / total) * OCCLUSION_SCALE).clamp(0.0, 1.0)
    } else {
        OCCLUDED
    };
    (obstruction, occlusion)
}

impl SoundWorld {
    /// The looping sounds `level`'s object definitions carry.
    pub(crate) fn new(level: &Level) -> Self {
        let mut carried = BTreeMap::new();
        for object in &level.objects {
            let loops: Vec<(usize, TagRef)> = object
                .attachments
                .iter()
                .enumerate()
                .filter_map(|(slot, attachment)| {
                    let tag = attachment.tag.as_ref()?;
                    (tag.group.0 == *b"lsnd").then(|| (slot, tag.clone()))
                })
                .collect();
            if !loops.is_empty() {
                carried.insert(object.tag.clone(), loops);
            }
        }
        Self {
            carried,
            ..Self::default()
        }
    }

    /// The environment heard.
    pub(crate) fn environment(&self) -> &Environment {
        &self.heard
    }

    /// The camera's velocity, in world units a second.
    pub(crate) fn listener_velocity(&self) -> Vec3 {
        self.velocity
    }

    /// Moves on by `ticks` ticks with the camera at `camera` in `structure`
    /// (the BSP the level plays in, none past its last):
    /// the environment glides toward the camera room's, and `loops` is
    /// filled with the looping sounds to play, each heard through what
    /// `clear` says stands between it and the camera.
    pub(crate) fn update(
        &mut self,
        structure: Option<&StructureBsp>,
        simulation: &mut Simulation,
        camera: Vec3,
        ticks: u32,
        mut clear: impl FnMut(Vec3, Vec3) -> bool,
        loops: &mut Vec<Looping>,
    ) {
        loops.clear();
        if ticks > 0 {
            if let Some(last) = self.camera {
                self.velocity = (camera - last) * (TICKS_PER_SECOND / ticks as f32);
            }
            self.camera = Some(camera);
        }
        let listener = structure.and_then(|structure| cluster_at(structure, camera));
        let room = structure
            .zip(listener)
            .and_then(|(structure, cluster)| structure.cluster_sounds.get(usize::from(cluster)));
        let target = room
            .and_then(|room| room.environment)
            .and_then(|index| {
                structure?
                    .sound_environments
                    .get(usize::from(index))
                    .copied()
                    .flatten()
            })
            .map_or(Environment::DEFAULT, |environment| {
                Environment::from_values(environment.values)
            });
        // (every value has arrived after so many updates)
        for _ in 0..ticks.min(16) {
            if self.heard.glide_towards(&target) {
                break;
            }
        }
        if let Some(tag) = room.and_then(|room| room.background).and_then(|index| {
            structure?
                .background_sounds
                .get(usize::from(index))
                .cloned()
                .flatten()
        }) {
            loops.push(Looping {
                key: BACKGROUND_KEY,
                tag,
                position: None,
                velocity: Vec3::ZERO,
                obstruction: 0.0,
                occlusion: 0.0,
            });
        }
        if self.carried.is_empty() {
            return;
        }
        let carried = &self.carried;
        simulation.visit_lit(|object| {
            let Some(attached) = carried.get(object.tag) else {
                return;
            };
            let position = object.current.position;
            let velocity = (position - object.previous.position) * TICKS_PER_SECOND;
            let (obstruction, occlusion) = match structure {
                Some(structure) => {
                    let between = match (listener, cluster_at(structure, position)) {
                        (Some(from), Some(to)) => structure.cluster_sound_distance(from, to),
                        _ => MAXIMUM_CLUSTER_SOUND_DISTANCE,
                    };
                    self::obstruction(clear(camera, position), between, position.distance(camera))
                }
                None => (0.0, 0.0),
            };
            for (slot, tag) in attached {
                loops.push(Looping {
                    key: u64::from(object.id.0) << 8 | (*slot as u64 & 0xFF),
                    tag: tag.clone(),
                    position: Some(position),
                    velocity,
                    obstruction,
                    occlusion,
                });
            }
        });
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Attachment, ClusterSound, Group, ObjectKind, SoundEnvironment};
    use opence_sim::objects::ObjectSpawn;
    use opence_sim::Transform;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    /// The devmap, whose one room hums `hum` and rings as a hall, with a
    /// generator carrying the loop `engine`; and a point in the room.
    fn level() -> (Level, Vec3) {
        let mut level = opence_content::devmap::level(None);
        let mut generator =
            crate::debug_scene::tests::test_object(ObjectKind::Scenery, "generator");
        let attachment = |group: &[u8; 4], path: &str| Attachment {
            tag: Some(tag(group, path)),
            marker: String::new(),
            primary_scale: 0,
            change_color: 0,
        };
        generator.attachments = vec![attachment(b"effe", "smoke"), attachment(b"lsnd", "engine")];
        level.objects.push(generator);
        let bsp = &mut level.structure_bsps[0];
        bsp.leaf_clusters = vec![Some(0); bsp.collision.leaves.len()];
        bsp.background_sounds = vec![None, Some(tag(b"lsnd", "hum"))];
        let mut values = Environment::DEFAULT.values();
        values[0] = 0.5;
        values[5] = 0.25;
        bsp.sound_environments = vec![Some(SoundEnvironment {
            priority: 1,
            values,
        })];
        bsp.cluster_sounds = vec![ClusterSound {
            background: Some(1),
            environment: Some(0),
        }];
        let room = [1.0, -1.0, 10.0, -10.0]
            .into_iter()
            .map(|z| Vec3::new(0.5, 0.5, z))
            .find(|point| bsp.collision.leaf_at(point.to_array()).is_some());
        let Some(room) = room else {
            panic!("the devmap has no leaf");
        };
        (level, room)
    }

    #[test]
    fn a_wall_blocks_a_source_and_its_distance_shuts_it_away() {
        assert_eq!(obstruction(true, 0.0, 10.0), (0.0, 0.0));
        let (blocked, shut) = obstruction(false, 0.0, 10.0);
        assert_eq!((blocked, shut), (OBSTRUCTED_NEARBY, 0.0));
        let (blocked, shut) = obstruction(false, 30.0, 10.0);
        assert_eq!(blocked, OBSTRUCTED_NEARBY);
        assert!((shut - 0.75 * OCCLUSION_SCALE).abs() < 1e-6 || shut == 1.0);
        let (blocked, shut) = obstruction(false, 10.0, 30.0);
        assert!((shut - 0.25 * OCCLUSION_SCALE).abs() < 1e-6);
        assert_eq!(blocked, OBSTRUCTED_NEARBY);
        let (blocked, shut) = obstruction(false, MAXIMUM_CLUSTER_SOUND_DISTANCE, 1.0);
        assert_eq!((blocked, shut), (OBSTRUCTED, 1.0));
        assert_eq!(obstruction(false, 0.0, 0.0), (OBSTRUCTED_NEARBY, OCCLUDED));
    }

    #[test]
    fn the_rooms_background_hums_and_its_reverb_is_glided_to() {
        let (level, room) = level();
        let mut world = SoundWorld::new(&level);
        assert_eq!(world.carried.len(), 1);
        let mut simulation = Simulation::empty();
        let mut loops = Vec::new();
        world.update(
            level.structure_bsps.first(),
            &mut simulation,
            room,
            1,
            |_, _| true,
            &mut loops,
        );
        assert_eq!(
            loops,
            [Looping {
                key: BACKGROUND_KEY,
                tag: tag(b"lsnd", "hum"),
                position: None,
                velocity: Vec3::ZERO,
                obstruction: 0.0,
                occlusion: 0.0,
            }]
        );
        assert!((world.environment().room_intensity - 0.03).abs() < 1e-6);
        world.update(
            level.structure_bsps.first(),
            &mut simulation,
            room + Vec3::X,
            30,
            |_, _| true,
            &mut loops,
        );
        assert!((world.environment().room_intensity - 0.5).abs() < 1e-6);
        assert!(world.environment().is_audible());
        assert_eq!(world.listener_velocity(), Vec3::X);

        // (outside every room, or in another BSP, the default is glided to)
        for _ in 0..2 {
            world.update(
                level.structure_bsps.get(3),
                &mut simulation,
                room,
                30,
                |_, _| true,
                &mut loops,
            );
        }
        assert!(loops.is_empty());
        assert_eq!(*world.environment(), Environment::DEFAULT);
    }

    #[test]
    fn an_objects_loops_follow_it_and_are_heard_through_walls(
    ) -> Result<(), Box<dyn std::error::Error>> {
        let (level, room) = level();
        let mut world = SoundWorld::new(&level);
        let mut simulation = Simulation::empty();
        let spawned = simulation.spawn(ObjectSpawn {
            kind: ObjectKind::Scenery,
            tag: tag(b"scen", "generator"),
            transform: Transform {
                position: room + Vec3::Y,
                ..Default::default()
            },
            name: None,
            permutation: None,
            team: None,
        });
        assert!(spawned.is_ok(), "the generator was not made");
        let mut loops = Vec::new();
        let mut cast = Vec::new();
        world.update(
            level.structure_bsps.first(),
            &mut simulation,
            room,
            1,
            |from, to| {
                cast.push((from, to));
                false
            },
            &mut loops,
        );
        assert_eq!(cast, [(room, room + Vec3::Y)]);
        let engine = loops
            .iter()
            .find(|looping| looping.tag.path == "engine")
            .ok_or("the generator's loop is not heard")?;
        assert_eq!(
            engine.key & 0xFF,
            1,
            "the loop is the generator's second attachment"
        );
        assert_eq!(engine.position, Some(room + Vec3::Y));
        // (in the camera's own room, behind something: blocked, not shut away)
        assert_eq!(
            (engine.obstruction, engine.occlusion),
            (OBSTRUCTED_NEARBY, 0.0)
        );

        let mut empty = SoundWorld::default();
        empty.update(
            level.structure_bsps.first(),
            &mut simulation,
            room,
            1,
            |_, _| false,
            &mut loops,
        );
        assert!(loops.iter().all(|looping| looping.key == BACKGROUND_KEY));
        Ok(())
    }
}
