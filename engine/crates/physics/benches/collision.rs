//! The collision the simulation asks for each tick: a ray across a room
//! and a character walking into its wall (`cargo bench -p opence_physics`).

use divan::Bencher;
use glam::Vec3;
use opence_content::level::{CollisionEdge, CollisionSurface, CollisionVertex};
use opence_content::CollisionBsp;
use opence_physics::{CharacterSettings, Layers, LevelMesh, World, WorldSettings};

fn main() {
    divan::main();
}

/// Adds a surface whose ring is `points`, in order, with no neighbours.
fn add_surface(bsp: &mut CollisionBsp, points: &[[f32; 3]], plane: [f32; 4]) {
    let surface = bsp.surfaces.len() as u32;
    let first_vertex = bsp.vertices.len() as u32;
    let first_edge = bsp.edges.len() as u32;
    let count = points.len() as u32;
    bsp.planes.push(plane);
    for (index, &point) in points.iter().enumerate() {
        let index = index as u32;
        bsp.vertices.push(CollisionVertex {
            point,
            first_edge: first_edge + index,
        });
        bsp.edges.push(CollisionEdge {
            vertices: [first_vertex + index, first_vertex + (index + 1) % count],
            edges: [first_edge + (index + 1) % count, 0],
            surfaces: [Some(surface), None],
        });
    }
    bsp.surfaces.push(CollisionSurface {
        plane: (bsp.planes.len() - 1) as i32,
        first_edge,
        flags: 0,
        breakable_surface: 0,
        material: None,
    });
}

/// A floor 20 units square at z = 0 and a wall across it at x = 5.
fn room() -> Option<World> {
    let mut bsp = CollisionBsp::default();
    add_surface(
        &mut bsp,
        &[
            [-10.0, -10.0, 0.0],
            [10.0, -10.0, 0.0],
            [10.0, 10.0, 0.0],
            [-10.0, 10.0, 0.0],
        ],
        [0.0, 0.0, 1.0, 0.0],
    );
    add_surface(
        &mut bsp,
        &[
            [5.0, -10.0, 0.0],
            [5.0, 10.0, 0.0],
            [5.0, 10.0, 10.0],
            [5.0, -10.0, 10.0],
        ],
        [-1.0, 0.0, 0.0, -5.0],
    );
    let mut world = World::new(WorldSettings::default()).ok()?;
    world.set_level(&LevelMesh::from_collision(&bsp)).ok()?;
    Some(world)
}

#[divan::bench]
fn a_ray_to_the_wall(bencher: Bencher) {
    let Some(world) = room() else { return };
    bencher.bench_local(|| {
        world.cast_ray(
            divan::black_box(Vec3::new(0.0, 0.0, 1.0)),
            Vec3::new(10.0, 0.0, 0.0),
            Layers::ALL,
        )
    });
}

#[divan::bench]
fn a_character_walks_into_the_wall(bencher: Bencher) {
    let Some(mut world) = room() else { return };
    let settings = CharacterSettings {
        radius: 0.2,
        height: 0.7,
        maximum_slope: 0.8,
        step_up: 0.1,
        stick_down: 0.1,
        layers: Layers::GROUND,
    };
    let Ok(character) = world.add_character(&settings, Vec3::new(4.0, 0.0, 0.0)) else {
        return;
    };
    bencher.bench_local(|| world.move_character(character, Vec3::new(4.0, 0.0, -1.0), 1.0 / 60.0));
}
