//! Cinematics and their camera: the camera put at the scenario's camera
//! points, moving from one to the next over the ticks asked
//! (`hs_library_cinematics.c`, `director.c`).

use glam::{Mat3, Quat};
use opence_content::CameraPoint;
use opence_script::{Native, Value};
use opence_sim::animation::{base_orientations, NodeMatrix, Orientation};
use opence_sim::objects::{rotation_from_angles, ObjectHandle};
use opence_sim::Vec3;

use crate::{boolean, element, index, short, tag, text, Mission};

/// The ticks of a second.
const TICKS_PER_SECOND: f32 = 30.0;

/// The field of view an animated camera has (`DEGREES_TO_RADIANS(70)`).
const ANIMATION_FIELD_OF_VIEW: f32 = 70.0 * core::f32::consts::PI / 180.0;

/// A camera animation the scripts play: the root node of an animation of a
/// graph of one node (`camera_set_animation`).
#[derive(Clone, Debug, PartialEq)]
pub struct CameraAnimation {
    /// Its graph, in the level's.
    pub graph: usize,
    /// The animation, in its graph's.
    pub animation: u16,
    /// The seconds of it left.
    pub timer: f32,
}

/// Where the scripts put the cinematic camera.
#[derive(Clone, Debug, PartialEq)]
pub struct CameraShot {
    /// The camera point it moves to.
    pub point: i32,
    /// The point it moves from (`-1` for none: it is there at once).
    pub from: i32,
    /// The object the points are measured from, if any.
    pub relative: Option<ObjectHandle>,
    /// The ticks the move takes.
    pub ticks: u16,
    /// The ticks of it left.
    pub left: u16,
}

/// The cinematic's state.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Cinematic {
    /// Whether a cinematic is playing.
    pub playing: bool,
    /// Whether the letterbox is shown.
    pub letterbox: bool,
    /// Whether the scripts hold the camera.
    pub camera_control: bool,
    /// The camera's shot, if the scripts placed it.
    pub shot: Option<CameraShot>,
    /// The camera's animation, if the scripts play one instead.
    pub animation: Option<CameraAnimation>,
    /// The title shown last, if any.
    pub title: Option<i32>,
    /// Whether the player may skip what plays.
    pub skippable: bool,
}

impl Cinematic {
    pub(crate) fn advance(&mut self) {
        if let Some(shot) = &mut self.shot {
            shot.left = shot.left.saturating_sub(1);
        }
        if let Some(animation) = &mut self.animation {
            animation.timer = (animation.timer - 1.0 / TICKS_PER_SECOND).max(0.0);
        }
    }

    /// Puts the camera at a point (or follows the first person or the
    /// dead): no animation plays.
    fn set_mode(&mut self) {
        self.shot = None;
        self.animation = None;
    }
}

impl Mission {
    /// Where the cinematic camera stands, how it is turned and its field
    /// of view, if the scripts hold it: along its animation, else between
    /// its shot's points, as far along as the move has gone.
    pub fn cinematic_camera(&self) -> Option<(Vec3, Quat, f32)> {
        if !self.cinematic.camera_control {
            return None;
        }
        if let Some(animation) = &self.cinematic.animation {
            return self.animated_camera(animation);
        }
        let shot = self.cinematic.shot.as_ref()?;
        let points = &self.level.scenario.campaign.camera_points;
        let to: &CameraPoint = element(points, shot.point)?;
        let origin = shot
            .relative
            .and_then(|h| self.position(h))
            .unwrap_or(Vec3::ZERO);
        let at = |p: &CameraPoint| origin + Vec3::from_array(p.position);
        let Some(from) = element(points, shot.from).filter(|_| shot.left > 0) else {
            return Some((
                at(to),
                rotation_from_angles(to.orientation),
                to.field_of_view,
            ));
        };
        let along = 1.0 - f32::from(shot.left) / f32::from(shot.ticks.max(1));
        let orientation = std::array::from_fn(|i| {
            from.orientation[i] + (to.orientation[i] - from.orientation[i]) * along
        });
        Some((
            at(from).lerp(at(to), along),
            rotation_from_angles(orientation),
            from.field_of_view + (to.field_of_view - from.field_of_view) * along,
        ))
    }

    /// The animated camera: its root node at the frame as far along as
    /// its timer has gone (`scripted_camera_update`).
    fn animated_camera(&self, camera: &CameraAnimation) -> Option<(Vec3, Quat, f32)> {
        let graph = self.level.animation_graphs.get(camera.graph)?;
        let animation = graph.animations.get(usize::from(camera.animation))?;
        let frames = i32::from(animation.frame_count);
        #[allow(clippy::cast_possible_truncation)]
        let frame = (frames as f32 - camera.timer * TICKS_PER_SECOND) as i32;
        let frame = frame.min(frames - 1).max(0);
        let mut root = [Orientation::REST];
        base_orientations(
            None,
            animation,
            i16::try_from(frame).unwrap_or(0),
            &mut root,
        );
        let matrix = NodeMatrix::from_orientation(&root[0]);
        let rotation = Quat::from_mat3(&Mat3::from_cols(
            Vec3::from_array(matrix.forward),
            Vec3::from_array(matrix.left),
            Vec3::from_array(matrix.up),
        ))
        .normalize();
        Some((
            Vec3::from_array(matrix.position),
            rotation,
            ANIMATION_FIELD_OF_VIEW,
        ))
    }

    fn set_shot(&mut self, point: i32, ticks: i16, relative: Option<ObjectHandle>) {
        self.cinematic.animation = None;
        let from = self.cinematic.shot.as_ref().map_or(-1, |s| s.point);
        let ticks = u16::try_from(ticks).unwrap_or(0);
        self.cinematic.shot = Some(CameraShot {
            point,
            from,
            relative,
            ticks,
            left: ticks,
        });
    }
}

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("camera_control", camera_control),
    ("camera_set", camera_set),
    ("camera_set_animation", camera_set_animation),
    ("camera_set_relative", camera_set_relative),
    ("camera_set_first_person", camera_set_first_person),
    ("camera_set_dead", camera_set_dead),
    ("camera_time", camera_time),
    ("cinematic_start", cinematic_start),
    ("cinematic_stop", cinematic_stop),
    ("cinematic_show_letterbox", cinematic_show_letterbox),
    ("cinematic_set_title", cinematic_set_title),
    (
        "cinematic_skip_start_internal",
        cinematic_skip_start_internal,
    ),
    ("cinematic_skip_stop_internal", cinematic_skip_stop_internal),
];

fn camera_control(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.cinematic.camera_control = boolean(arguments, 0);
    if !mission.cinematic.camera_control {
        mission.cinematic.set_mode();
    }
    Value::Void
}

fn camera_set(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.set_shot(index(arguments, 0), short(arguments, 1), None);
    Value::Void
}

/// Plays a camera animation: an animation of a graph of one node, from as
/// many frames before its end as its whole seconds (the original divides
/// its frames by the ticks of a second in integers, so it starts up to 29
/// frames in; preserved).
fn camera_set_animation(mission: &mut Mission, arguments: &[Value]) -> Value {
    let Some(graph) = tag(arguments, 0) else {
        return Value::Void;
    };
    let name = text(arguments, 1);
    let found = mission
        .level
        .animation_graphs
        .iter()
        .enumerate()
        .find(|(_, g)| &g.tag == graph)
        .filter(|(_, g)| g.nodes.len() == 1)
        .and_then(|(place, g)| {
            let (index, animation) = g
                .animations
                .iter()
                .enumerate()
                .find(|(_, a)| a.name.eq_ignore_ascii_case(name))?;
            Some((place, u16::try_from(index).ok()?, animation.frame_count))
        });
    if let Some((graph, animation, frames)) = found {
        mission.cinematic.set_mode();
        mission.cinematic.animation = Some(CameraAnimation {
            graph,
            animation,
            timer: f32::from(frames / 30),
        });
    }
    Value::Void
}

fn camera_set_relative(mission: &mut Mission, arguments: &[Value]) -> Value {
    let relative = mission.handle(index(arguments, 2));
    mission.set_shot(index(arguments, 0), short(arguments, 1), relative);
    Value::Void
}

fn camera_set_first_person(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.cinematic.set_mode();
    mission.present("camera_set_first_person", arguments);
    Value::Void
}

fn camera_set_dead(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.cinematic.set_mode();
    mission.present("camera_set_dead", arguments);
    Value::Void
}

fn camera_time(mission: &mut Mission, _: &[Value]) -> Value {
    if let Some(animation) = &mission.cinematic.animation {
        #[allow(clippy::cast_possible_truncation)]
        return Value::Short((animation.timer * TICKS_PER_SECOND) as i16);
    }
    let left = mission.cinematic.shot.as_ref().map_or(0, |s| s.left);
    Value::Short(i16::try_from(left).unwrap_or(i16::MAX))
}

fn cinematic_start(mission: &mut Mission, _: &[Value]) -> Value {
    mission.cinematic.playing = true;
    mission.cinematic.letterbox = true;
    Value::Void
}

fn cinematic_stop(mission: &mut Mission, _: &[Value]) -> Value {
    mission.cinematic.playing = false;
    mission.cinematic.letterbox = false;
    mission.cinematic.title = None;
    Value::Void
}

fn cinematic_show_letterbox(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.cinematic.letterbox = boolean(arguments, 0);
    Value::Void
}

fn cinematic_set_title(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.cinematic.title = Some(index(arguments, 0));
    mission.present("cinematic_set_title", arguments);
    Value::Void
}

fn cinematic_skip_start_internal(mission: &mut Mission, _: &[Value]) -> Value {
    mission.cinematic.skippable = true;
    Value::Void
}

fn cinematic_skip_stop_internal(mission: &mut Mission, _: &[Value]) -> Value {
    mission.cinematic.skippable = false;
    Value::Void
}
