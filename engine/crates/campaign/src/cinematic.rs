//! Cinematics and their camera: the camera put at the scenario's camera
//! points, moving from one to the next over the ticks asked
//! (`hs_library_cinematics.c`, `director.c`).

use opence_content::CameraPoint;
use opence_script::{Native, Value};
use opence_sim::objects::ObjectHandle;
use opence_sim::Vec3;

use crate::{boolean, element, index, short, Mission};

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
    }
}

impl Mission {
    /// Where the cinematic camera stands and how it is turned (yaw, pitch
    /// and roll), and its field of view, if the scripts hold it: between
    /// its shot's points, as far along as the move has gone.
    pub fn cinematic_camera(&self) -> Option<(Vec3, [f32; 3], f32)> {
        let shot = self
            .cinematic
            .shot
            .as_ref()
            .filter(|_| self.cinematic.camera_control)?;
        let points = &self.level.scenario.campaign.camera_points;
        let to: &CameraPoint = element(points, shot.point)?;
        let origin = shot
            .relative
            .and_then(|h| self.position(h))
            .unwrap_or(Vec3::ZERO);
        let at = |p: &CameraPoint| origin + Vec3::from_array(p.position);
        let Some(from) = element(points, shot.from).filter(|_| shot.left > 0) else {
            return Some((at(to), to.orientation, to.field_of_view));
        };
        let along = 1.0 - f32::from(shot.left) / f32::from(shot.ticks.max(1));
        let orientation = std::array::from_fn(|i| {
            from.orientation[i] + (to.orientation[i] - from.orientation[i]) * along
        });
        Some((
            at(from).lerp(at(to), along),
            orientation,
            from.field_of_view + (to.field_of_view - from.field_of_view) * along,
        ))
    }

    fn set_shot(&mut self, point: i32, ticks: i16, relative: Option<ObjectHandle>) {
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
        mission.cinematic.shot = None;
    }
    Value::Void
}

fn camera_set(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.set_shot(index(arguments, 0), short(arguments, 1), None);
    Value::Void
}

fn camera_set_relative(mission: &mut Mission, arguments: &[Value]) -> Value {
    let relative = mission.handle(index(arguments, 2));
    mission.set_shot(index(arguments, 0), short(arguments, 1), relative);
    Value::Void
}

fn camera_set_first_person(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.cinematic.shot = None;
    mission.present("camera_set_first_person", arguments);
    Value::Void
}

fn camera_set_dead(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.cinematic.shot = None;
    mission.present("camera_set_dead", arguments);
    Value::Void
}

fn camera_time(mission: &mut Mission, _: &[Value]) -> Value {
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
