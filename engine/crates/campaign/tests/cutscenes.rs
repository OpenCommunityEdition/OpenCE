//! The cutscenes' animations on the campaign's maps: a custom animation
//! starts at the frame asked, blends in from the pose before it when asked
//! to, and plays where its unit was put at a cutscene flag; the camera
//! follows a camera animation's root node to its end (skipped where there
//! is no game data).

use std::error::Error;
use std::path::Path;
use std::sync::Arc;

use opence_campaign::Mission;
use opence_content::{AnimationKind, Level};
use opence_import::testing::stock_level_unheld;
use opence_script::Value;
use opence_sim::animation::{
    base_orientations, AnimationPlayer, NodeMatrices, NodeMatrix, Orientation,
};
use opence_sim::input::TickInput;
use opence_sim::objects::Difficulty;
use opence_sim::{Simulation, Vec3};

const MAPS: [&str; 4] = ["a10", "a30", "b30", "c10"];

/// The mission of `map`, if the map is here.
fn mission(map: &str) -> Result<Option<Mission>, Box<dyn Error>> {
    let Some(stock) = stock_level_unheld(map)? else {
        return Ok(None);
    };
    let level = Arc::new(stock);
    let mut sim = Simulation::empty();
    sim.spawn_player(&level, 0)?;
    Ok(Some(Mission::new(sim, level, Difficulty::Normal)))
}

/// Two base animations of the player's biped's graph longer than ten
/// frames: the graph's tag and their names.
fn biped_animations(mission: &Mission) -> Option<(opence_content::TagRef, String, String)> {
    let unit = mission.sim().player()?.unit().clone();
    let level: &Level = mission.level();
    let graph_tag = level
        .objects
        .iter()
        .find(|d| d.tag == unit)?
        .animation_graph
        .clone()?;
    let graph = level.animation_graphs.iter().find(|g| g.tag == graph_tag)?;
    let mut long = graph
        .animations
        .iter()
        .filter(|a| a.kind == AnimationKind::Base && a.frame_count > 10)
        .map(|a| a.name.clone());
    let first = long.next()?;
    let second = long.find(|name| *name != first)?;
    Some((graph_tag, first, second))
}

#[test]
fn a_custom_animation_starts_at_its_frame_and_blends_in() -> Result<(), Box<dyn Error>> {
    let mut played = 0;
    for map in MAPS {
        let Some(mut mission) = mission(map)? else {
            continue;
        };
        mission.step(TickInput::default());
        let Some((graph, first, second)) = biped_animations(&mission) else {
            continue;
        };
        let Some(handle) = mission.sim().player().map(|p| p.handle()) else {
            continue;
        };
        let unit = Value::Index(mission.index_of(handle));
        let started = mission.call(
            "unit_custom_animation_at_frame",
            &[
                unit.clone(),
                Value::Tag(Some(graph.clone())),
                Value::String(first.clone()),
                Value::Boolean(false),
                Value::Short(5),
            ],
        );
        assert_eq!(started, Some(Value::Boolean(true)), "{map}: {first}");
        let player = mission
            .sim()
            .component::<AnimationPlayer>(handle)
            .ok_or("no animation player")?;
        assert_eq!(player.state.frame, 5, "{map}: started at another frame");
        assert_eq!(player.pose(), player.blended_pose(), "{map}: a cut blended");

        let started = mission.call(
            "custom_animation",
            &[
                unit,
                Value::Tag(Some(graph)),
                Value::String(second.clone()),
                Value::Boolean(true),
            ],
        );
        assert_eq!(started, Some(Value::Boolean(true)), "{map}: {second}");
        let player = mission
            .sim()
            .component::<AnimationPlayer>(handle)
            .ok_or("no animation player")?;
        assert_eq!(player.state.frame, 0);
        assert_ne!(
            player.pose(),
            player.blended_pose(),
            "{map}: an interpolated animation did not blend in"
        );
        played += 1;
    }
    let here = MAPS.iter().any(|map| {
        Path::new(env!("CARGO_MANIFEST_DIR"))
            .join(format!("../../../assets/maps/{map}.map"))
            .exists()
    });
    assert!(!here || played > 0, "no map played a custom animation");
    Ok(())
}

#[test]
fn a_custom_animation_plays_where_its_unit_stands_at_a_flag() -> Result<(), Box<dyn Error>> {
    for map in MAPS {
        let Some(mut mission) = mission(map)? else {
            continue;
        };
        mission.step(TickInput::default());
        let Some((graph, first, _)) = biped_animations(&mission) else {
            continue;
        };
        let Some(handle) = mission.sim().player().map(|p| p.handle()) else {
            continue;
        };
        let Some(flag) = mission.level().scenario.campaign.cutscene_flags.first() else {
            continue;
        };
        let at = Vec3::from_array(flag.position);
        let unit = Value::Index(mission.index_of(handle));
        mission.call("object_teleport", &[unit.clone(), Value::Index(0)]);
        let standing = mission.sim().transform(handle).ok_or("no transform")?;
        assert!(
            standing.position.distance(at) < 0.01,
            "{map}: the unit is not at the flag"
        );
        mission.call(
            "custom_animation",
            &[
                unit,
                Value::Tag(Some(graph)),
                Value::String(first),
                Value::Boolean(false),
            ],
        );
        let matrices = mission
            .sim()
            .component::<NodeMatrices>(handle)
            .ok_or("no node matrices")?;
        let root = matrices.current.first().ok_or("no nodes")?;
        assert!(
            Vec3::from_array(root.position).distance(at) < 2.0,
            "{map}: the animation's root is {:?}, far from the flag at {at:?}",
            root.position
        );
    }
    Ok(())
}

/// Steps the mission through one game tick (the simulation's ticks are
/// finer).
fn game_tick(mission: &mut Mission) {
    let next = mission.game_tick() + 1;
    while mission.game_tick() < next {
        mission.step(TickInput::default());
    }
}

#[test]
fn the_camera_follows_its_animation_to_the_end() -> Result<(), Box<dyn Error>> {
    let mut ended = 0;
    for map in MAPS {
        let Some(mut mission) = mission(map)? else {
            continue;
        };
        let camera = mission
            .level()
            .animation_graphs
            .iter()
            .filter(|g| g.nodes.len() == 1)
            .find_map(|g| {
                let animation = g
                    .animations
                    .iter()
                    .find(|a| a.kind == AnimationKind::Base && a.frame_count >= 45)?;
                Some((g.tag.clone(), animation.clone()))
            });
        let Some((graph, animation)) = camera else {
            continue;
        };
        mission.call("camera_control", &[Value::Boolean(true)]);
        mission.call(
            "camera_set_animation",
            &[
                Value::Tag(Some(graph)),
                Value::String(animation.name.clone()),
            ],
        );
        let seconds = animation.frame_count / 30;
        assert_eq!(
            mission.call("camera_time", &[]),
            Some(Value::Short(i16::try_from(seconds * 30)?)),
            "{map}: the camera's time"
        );
        let root = |frame: u16| {
            let mut node = [Orientation::REST];
            base_orientations(
                None,
                &animation,
                i16::try_from(frame).unwrap_or(0),
                &mut node,
            );
            Vec3::from_array(NodeMatrix::from_orientation(&node[0]).position)
        };
        let start = animation.frame_count - seconds * 30;
        let (position, _, field_of_view) =
            mission.cinematic_camera().ok_or("the camera is not held")?;
        assert!(
            position.distance(root(start)) < 1e-5,
            "{map}: the first frame"
        );
        assert!((field_of_view - 70f32.to_radians()).abs() < 1e-5);
        for _ in 0..10 {
            game_tick(&mut mission);
        }
        // Ten ticks of its timer, in seconds as the game keeps it, and the
        // frame they truncate to (`scripted_camera_update`).
        let mut timer = f32::from(seconds);
        for _ in 0..10 {
            timer = (timer - 1.0 / 30.0).max(0.0);
        }
        #[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
        let frame = (f32::from(animation.frame_count) - timer * 30.0) as u16;
        assert!(frame.abs_diff(start + 10) <= 1);
        let (position, _, _) = mission.cinematic_camera().ok_or("the camera is not held")?;
        assert!(position.distance(root(frame)) < 1e-5, "{map}: ten ticks on");
        // The map's own scripts run as it steps: a map whose scripts move
        // the camera elsewhere first proves nothing about the end.
        let mut taken = false;
        for _ in 0..animation.frame_count {
            game_tick(&mut mission);
            let ours = mission
                .cinematic_camera()
                .is_some_and(|(_, _, fov)| (fov - 70f32.to_radians()).abs() < 1e-5);
            taken |= !ours;
        }
        if taken {
            continue;
        }
        let (position, _, _) = mission.cinematic_camera().ok_or("the camera is not held")?;
        assert!(
            position.distance(root(animation.frame_count - 1)) < 1e-5,
            "{map}: the camera did not stop at its last frame"
        );
        assert_eq!(mission.call("camera_time", &[]), Some(Value::Short(0)));
        ended += 1;
    }
    let here = MAPS.iter().any(|map| {
        Path::new(env!("CARGO_MANIFEST_DIR"))
            .join(format!("../../../assets/maps/{map}.map"))
            .exists()
    });
    assert!(
        !here || ended > 0,
        "no map's camera animation played to its end"
    );
    Ok(())
}
