//! The debug drawing's world layers: what the level and the world hold,
//! drawn again at each publish while a layer is on.

use opence_content::{devmap, Level};
use opence_dev::draw::{Color, DebugDraw, Layer, MAX_LINES};
use opence_sim::{Simulation, Vec3};

/// How far from the camera the collision's edges are drawn.
pub const COLLISION_REACH: f32 = 30.0;

/// The radius a firing position is drawn with.
const FIRING_POSITION_RADIUS: f32 = 0.25;

/// The length of an object's axes.
const AXES_LENGTH: f32 = 0.5;

/// How far from the camera the test map's objects are named.
pub const LABEL_REACH: f32 = 25.0;

/// How far above an object's slot its name is.
const LABEL_LIFT: f32 = 0.3;

/// What the world layers that are on show, from `camera`, into `scene`
/// (emptied first; a shape lasts one tick).
pub(crate) fn fill(
    scene: &mut DebugDraw,
    level: Option<&Level>,
    bsp: usize,
    simulation: &mut Simulation,
    camera: Vec3,
) {
    scene.clear();
    if let Some(level) = level {
        if scene.is_on(Layer::Collision) {
            collision(scene, level, bsp, camera);
        }
        if scene.is_on(Layer::Triggers) {
            triggers(scene, level);
        }
        if scene.is_on(Layer::Ai) {
            ai(scene, level);
        }
        if scene.is_on(Layer::Labels) && level.scenario.path == devmap::NAME {
            labels(scene, level, camera);
        }
    }
    if scene.is_on(Layer::Markers) || scene.is_on(Layer::Bodies) {
        objects(scene, simulation);
    }
}

fn collision(scene: &mut DebugDraw, level: &Level, bsp: usize, camera: Vec3) {
    let Some(structure) = level.structure_bsps.get(bsp) else {
        return;
    };
    let collision = &structure.collision;
    let point = |index: u32| {
        collision
            .vertices
            .get(index as usize)
            .map(|vertex| vertex.point)
    };
    let near =
        |at: [f32; 3]| Vec3::from(at).distance_squared(camera) <= COLLISION_REACH * COLLISION_REACH;
    for edge in &collision.edges {
        let (Some(from), Some(to)) = (point(edge.vertices[0]), point(edge.vertices[1])) else {
            continue;
        };
        if near(from) || near(to) {
            scene.line(Layer::Collision, from, to, Layer::Collision.color(), 1);
            if scene.lines().len() >= MAX_LINES {
                return;
            }
        }
    }
}

fn triggers(scene: &mut DebugDraw, level: &Level) {
    let color = Layer::Triggers.color();
    for volume in &level.scenario.campaign.trigger_volumes {
        if volume.oriented {
            let forward = Vec3::from(volume.forward).normalize_or_zero();
            let up = Vec3::from(volume.up).normalize_or_zero();
            let left = up.cross(forward);
            let half = Vec3::from(volume.extents) * 0.5;
            // (its corner is `position`: the middle is half its extents along each axis)
            let center =
                Vec3::from(volume.position) + forward * half.x + left * half.y + up * half.z;
            scene.oriented_box(
                Layer::Triggers,
                center.to_array(),
                [forward.to_array(), left.to_array(), up.to_array()],
                half.to_array(),
                color,
                1,
            );
        } else {
            let [x0, x1, y0] = volume.position;
            let [y1, z0, z1] = volume.extents;
            scene.bounds(
                Layer::Triggers,
                [x0.min(x1), y0.min(y1), z0.min(z1)],
                [x0.max(x1), y0.max(y1), z0.max(z1)],
                color,
                1,
            );
        }
    }
}

fn ai(scene: &mut DebugDraw, level: &Level) {
    let campaign = &level.scenario.campaign;
    let color = Layer::Ai.color();
    for encounter in &campaign.encounters {
        for firing in &encounter.firing_positions {
            scene.sphere(Layer::Ai, firing.position, FIRING_POSITION_RADIUS, color, 1);
        }
    }
    for list in &campaign.command_lists {
        for pair in list.points.windows(2) {
            scene.line(Layer::Ai, pair[0], pair[1], Color::WHITE, 1);
        }
    }
}

/// The test map's galleries: each one's heading, and near the camera, each
/// object's slot outlined on the floor (its rack, bay or pedestal) and
/// named above it, nearest first.
fn labels(scene: &mut DebugDraw, level: &Level, camera: Vec3) {
    let color = Layer::Labels.color();
    for (heading, at) in devmap::headings(&level.scenario.placements) {
        scene.text(Layer::Labels, at, heading, color, 1);
    }
    let mut near: Vec<(f32, &opence_content::ObjectPlacement)> = level
        .scenario
        .placements
        .iter()
        .map(|placement| {
            (
                Vec3::from(placement.position).distance_squared(camera),
                placement,
            )
        })
        .filter(|(distance, _)| *distance <= LABEL_REACH * LABEL_REACH)
        .collect();
    near.sort_by(|a, b| a.0.total_cmp(&b.0));
    for (_, placement) in near {
        let Some(object) = level.object(&placement.tag) else {
            continue;
        };
        let radius = object.bounding_radius.max(devmap::MIN_SLOT_RADIUS);
        let [x, y, z] = placement.position;
        // (the slot's middle: the origin less its sphere's offset, turned to face the start)
        let (middle_x, middle_y) = (x - object.bounding_offset[0], y - object.bounding_offset[1]);
        let corners = [
            [middle_x - radius, middle_y - radius, 0.01],
            [middle_x + radius, middle_y - radius, 0.01],
            [middle_x + radius, middle_y + radius, 0.01],
            [middle_x - radius, middle_y + radius, 0.01],
        ];
        for at in 0..4 {
            scene.line(Layer::Labels, corners[at], corners[(at + 1) % 4], color, 1);
        }
        let name = placement.tag.path.rsplit('\\').next().unwrap_or("");
        let top = z.max(0.0) + 2.0 * radius + LABEL_LIFT;
        scene.text(Layer::Labels, [middle_x, middle_y, top], name, color, 1);
        if scene.labels().len() >= opence_dev::draw::MAX_LABELS {
            return;
        }
    }
}

fn objects(scene: &mut DebugDraw, simulation: &mut Simulation) {
    let markers = scene.is_on(Layer::Markers);
    let bodies = scene.is_on(Layer::Bodies);
    let mut shapes = Vec::new();
    simulation.visit_drawn(|object| {
        let radius = object
            .appearance
            .map_or(0.0, |look| f32::from_bits(look.radius_bits));
        shapes.push((object.current.position, object.current.rotation, radius));
    });
    for (position, rotation, radius) in shapes {
        if markers {
            let axes = [
                (rotation * Vec3::X).to_array(),
                (rotation * Vec3::Y).to_array(),
                (rotation * Vec3::Z).to_array(),
            ];
            scene.axes(Layer::Markers, position.to_array(), axes, AXES_LENGTH, 1);
        }
        if bodies && radius.is_finite() && radius > 0.0 {
            scene.sphere(
                Layer::Bodies,
                position.to_array(),
                radius,
                Layer::Bodies.color(),
                1,
            );
        }
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use opence_content::campaign::{CommandList, Encounter, FiringPosition, TriggerVolume};
    use opence_content::level::{
        CollisionBsp, CollisionEdge, CollisionVertex, Scenario, StructureBsp,
    };

    /// A level with nothing in it but a few shapes.
    pub(crate) fn level() -> Level {
        let mut level = Level {
            scenario: Scenario {
                path: "test".into(),
                structure_bsps: Vec::new(),
                player_starts: Vec::new(),
                placements: Vec::new(),
                netgame_flags: Vec::new(),
                netgame_equipment: Vec::new(),
                scripts: Default::default(),
                campaign: Default::default(),
            },
            structure_bsps: Vec::new(),
            objects: Vec::new(),
            models: Vec::new(),
            animation_graphs: Vec::new(),
            shaders: Vec::new(),
            bitmaps: Vec::new(),
            resource_bitmaps: Vec::new(),
            skies: Vec::new(),
            globals: Default::default(),
            damage_effects: Vec::new(),
            collision_models: Vec::new(),
            physics: Vec::new(),
            effects: Vec::new(),
            lights: Vec::new(),
            unit_huds: Vec::new(),
            weapon_huds: Vec::new(),
            grenade_huds: Vec::new(),
            hud_digits: None,
            hud_globals: None,
        };
        let mut structure = StructureBsp {
            path: "test".into(),
            lightmap_bitmap: None,
            world_bounds: [[0.0; 2]; 3],
            collision: CollisionBsp::default(),
            collision_materials: Vec::new(),
            surfaces: Vec::new(),
            lightmaps: Vec::new(),
        };
        structure.collision.vertices = vec![
            CollisionVertex {
                point: [0.0, 0.0, 0.0],
                first_edge: 0,
            },
            CollisionVertex {
                point: [1.0, 0.0, 0.0],
                first_edge: 0,
            },
            CollisionVertex {
                point: [100.0, 0.0, 0.0],
                first_edge: 0,
            },
            CollisionVertex {
                point: [101.0, 0.0, 0.0],
                first_edge: 0,
            },
        ];
        let edge = |a, b| CollisionEdge {
            vertices: [a, b],
            edges: [0, 0],
            surfaces: [None, None],
        };
        // (the near one, the far one, and one whose vertex is missing)
        structure.collision.edges = vec![edge(0, 1), edge(2, 3), edge(0, 9)];
        level.structure_bsps.push(structure);
        let campaign = &mut level.scenario.campaign;
        campaign.trigger_volumes = vec![
            TriggerVolume {
                name: "box".into(),
                oriented: true,
                position: [0.0, 0.0, 0.0],
                forward: [1.0, 0.0, 0.0],
                up: [0.0, 0.0, 1.0],
                extents: [2.0, 2.0, 2.0],
            },
            TriggerVolume {
                name: "corners".into(),
                oriented: false,
                position: [1.0, -1.0, 0.0],
                forward: [0.0; 3],
                up: [0.0; 3],
                extents: [1.0, 0.0, 2.0],
            },
        ];
        campaign.encounters = vec![Encounter {
            name: "fight".into(),
            flags: 0,
            team: 0,
            squads: Vec::new(),
            platoons: Vec::new(),
            firing_positions: vec![FiringPosition {
                position: [3.0, 0.0, 0.0],
                group: 0,
            }],
        }];
        campaign.command_lists = vec![CommandList {
            name: "walk".into(),
            flags: 0,
            commands: Vec::new(),
            points: vec![[0.0; 3], [1.0, 0.0, 0.0], [1.0, 1.0, 0.0]],
        }];
        level
    }

    fn scene(layer: Layer) -> DebugDraw {
        let mut scene = DebugDraw::new();
        scene.set_layer(Layer::General, false);
        scene.set_layer(layer, true);
        scene
    }

    #[test]
    fn the_test_maps_galleries_are_labelled_near_the_camera() {
        let mut level = opence_content::devmap::level(None);
        let near = test_object(opence_content::ObjectKind::Weapon, r"weapons\rifle\rifle");
        let mut far = test_object(opence_content::ObjectKind::Scenery, r"scenery\far");
        far.bounding_radius = 1.0;
        level.objects = vec![near, far];
        level.scenario.placements = devmap::galleries(&level.objects).placements;
        level.scenario.placements[1].position = [200.0, 0.0, 0.0];
        let mut scene = scene(Layer::Labels);
        let camera = Vec3::from(level.scenario.placements[0].position);
        fill(
            &mut scene,
            Some(&level),
            0,
            &mut Simulation::empty(),
            camera,
        );
        let texts: Vec<&str> = scene
            .labels()
            .iter()
            .map(|label| label.text.as_str())
            .collect();
        assert_eq!(texts, vec!["weapons", "scenery", "rifle"]);
        // (its slot, outlined on the floor)
        assert_eq!(scene.lines().len(), 4);
        assert!(scene.labels()[2].at[2] > 0.5);

        // another map's placements are not named
        level.scenario.path = "other".into();
        fill(
            &mut scene,
            Some(&level),
            0,
            &mut Simulation::empty(),
            camera,
        );
        assert!(scene.labels().is_empty());
    }

    fn test_object(
        kind: opence_content::ObjectKind,
        path: &str,
    ) -> opence_content::ObjectDefinition {
        opence_content::ObjectDefinition {
            tag: opence_content::TagRef {
                group: opence_content::Group(kind.group()),
                path: path.into(),
            },
            kind,
            flags: 0,
            bounding_radius: 0.25,
            bounding_offset: [0.0, 0.0, 0.1],
            acceleration_scale: 1.0,
            render_bounding_radius: 0.25,
            model: None,
            animation_graph: None,
            collision_model: None,
            physics: None,
            modifier_shader: None,
            creation_effect: None,
            function_modes: Default::default(),
            functions: Vec::new(),
            attachments: Vec::new(),
            change_colors: Vec::new(),
            unit_hud: None,
            unit_melee_damage: None,
            biped: None,
            weapon: None,
            projectile: None,
            equipment: None,
            vehicle: None,
        }
    }

    #[test]
    fn the_collision_near_the_camera_is_drawn() {
        let mut scene = scene(Layer::Collision);
        fill(
            &mut scene,
            Some(&level()),
            0,
            &mut Simulation::empty(),
            Vec3::ZERO,
        );
        assert_eq!(scene.lines().len(), 1);
        assert_eq!(scene.lines()[0].to, [1.0, 0.0, 0.0]);
        // (a structure the level has not draws nothing)
        fill(
            &mut scene,
            Some(&level()),
            5,
            &mut Simulation::empty(),
            Vec3::ZERO,
        );
        assert!(scene.lines().is_empty());
    }

    #[test]
    fn the_trigger_volumes_are_boxes() {
        let mut scene = scene(Layer::Triggers);
        fill(
            &mut scene,
            Some(&level()),
            0,
            &mut Simulation::empty(),
            Vec3::ZERO,
        );
        // (two boxes of twelve edges)
        assert_eq!(scene.lines().len(), 24);
        let xs: Vec<f32> = scene.lines()[..12]
            .iter()
            .flat_map(|line| [line.from[0], line.to[0]])
            .collect();
        assert!(xs.iter().all(|&x| x == 0.0 || x == 2.0));
        let corners: Vec<[f32; 3]> = scene.lines()[12..]
            .iter()
            .flat_map(|line| [line.from, line.to])
            .collect();
        assert!(corners.contains(&[-1.0, 0.0, 0.0]));
        assert!(corners.contains(&[1.0, 1.0, 2.0]));
    }

    #[test]
    fn the_ai_shows_its_firing_positions_and_paths() {
        let mut scene = scene(Layer::Ai);
        fill(
            &mut scene,
            Some(&level()),
            0,
            &mut Simulation::empty(),
            Vec3::ZERO,
        );
        let paths = scene
            .lines()
            .iter()
            .filter(|line| line.color == Color::WHITE)
            .count();
        assert_eq!(paths, 2);
        assert!(scene.lines().len() > paths);
    }

    #[test]
    fn layers_that_are_off_draw_nothing() {
        let mut scene = DebugDraw::new();
        fill(
            &mut scene,
            Some(&level()),
            0,
            &mut Simulation::empty(),
            Vec3::ZERO,
        );
        assert!(scene.lines().is_empty());
        let mut scene = self::scene(Layer::Markers);
        fill(&mut scene, None, 0, &mut Simulation::empty(), Vec3::ZERO);
        assert!(scene.lines().is_empty());
    }
}
