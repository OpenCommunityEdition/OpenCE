//! What a scenario gives its campaign: the things its scripts name (trigger
//! volumes, cutscene flags and camera points, device groups, recordings,
//! conversations) and its AI's encounters and command lists. A script names
//! each by its index in its list here.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// A scenario's campaign data.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Campaign {
    /// The names its objects are given, in the scenario's order.
    pub object_names: Vec<String>,
    /// Its trigger volumes.
    pub trigger_volumes: Vec<TriggerVolume>,
    /// Its cutscene flags.
    pub cutscene_flags: Vec<CutsceneFlag>,
    /// Its cutscene camera points.
    pub camera_points: Vec<CameraPoint>,
    /// Its cutscene titles' names.
    pub cutscene_titles: Vec<String>,
    /// Its device groups.
    pub device_groups: Vec<DeviceGroup>,
    /// Its recorded animations.
    pub recordings: Vec<Recording>,
    /// The actor variants its encounters place.
    pub actor_palette: Vec<Option<TagRef>>,
    /// Its encounters.
    pub encounters: Vec<Encounter>,
    /// Its AI command lists.
    pub command_lists: Vec<CommandList>,
    /// Its AI conversations' names.
    pub conversations: Vec<String>,
    /// Its starting profiles' names.
    pub starting_profiles: Vec<String>,
}

/// A box a script tests objects against.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct TriggerVolume {
    /// Its name.
    pub name: String,
    /// Whether it is turned by its axes; otherwise it lies along the
    /// world's, its corners `position` and `extents` read as x0, x1, y0
    /// and y1, z0, z1.
    pub oriented: bool,
    /// Its corner.
    pub position: [f32; 3],
    /// Its forward axis.
    pub forward: [f32; 3],
    /// Its up axis.
    pub up: [f32; 3],
    /// How far it reaches along its forward, left and up axes.
    pub extents: [f32; 3],
}

impl TriggerVolume {
    /// Whether `point` is inside it (`scenario_trigger_volume_test_point`):
    /// between its world-aligned bounds, or, measured from its corner along
    /// its forward, left and up axes, between nothing and its extents.
    pub fn contains(&self, point: [f32; 3]) -> bool {
        if !self.oriented {
            let [x0, x1, y0] = self.position;
            let [y1, z0, z1] = self.extents;
            return point[0] > x0
                && point[1] > y0
                && point[2] > z0
                && point[0] < x1
                && point[1] < y1
                && point[2] < z1;
        }
        let [f, u] = [self.forward, self.up];
        // (left: up across forward)
        let left = [
            u[1] * f[2] - u[2] * f[1],
            u[2] * f[0] - u[0] * f[2],
            u[0] * f[1] - u[1] * f[0],
        ];
        let d = [
            point[0] - self.position[0],
            point[1] - self.position[1],
            point[2] - self.position[2],
        ];
        let dot = |a: [f32; 3]| a[0] * d[0] + a[1] * d[1] + a[2] * d[2];
        [dot(f), dot(left), dot(u)]
            .iter()
            .zip(self.extents)
            .all(|(&along, extent)| along > 0.0 && along < extent)
    }
}

/// A point and facing a cutscene names.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CutsceneFlag {
    /// Its name.
    pub name: String,
    /// Where it is.
    pub position: [f32; 3],
    /// Its yaw and pitch, in radians.
    pub facing: [f32; 2],
}

/// A place a cutscene's camera stands.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CameraPoint {
    /// Its name.
    pub name: String,
    /// Where it is.
    pub position: [f32; 3],
    /// Its yaw, pitch and roll, in radians.
    pub orientation: [f32; 3],
    /// Its field of view, in radians.
    pub field_of_view: f32,
}

/// A value devices share: their power or their position.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct DeviceGroup {
    /// Its name.
    pub name: String,
    /// The value it starts as.
    pub initial_value: f32,
    /// Its flags: bit 0, only a script changes it.
    pub flags: u32,
}

/// A unit's recorded movements, which a script plays on it.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Recording {
    /// Its name.
    pub name: String,
    /// How long it plays, in ticks.
    pub length_in_ticks: i16,
}

/// A group of actors the scenario places together.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Encounter {
    /// Its name.
    pub name: String,
    /// Its flags.
    pub flags: u32,
    /// Its team.
    pub team: i16,
    /// Its squads.
    pub squads: Vec<Squad>,
    /// Its platoons' names.
    pub platoons: Vec<String>,
    /// Where its actors stand to fight.
    pub firing_positions: Vec<FiringPosition>,
}

/// A squad of an encounter.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Squad {
    /// Its name.
    pub name: String,
    /// Its actor variant, from the scenario's actor palette.
    pub actor: i16,
    /// Its platoon, or `-1`.
    pub platoon: i16,
    /// The state its actors start in.
    pub initial_state: i16,
    /// The state they return to.
    pub default_state: i16,
    /// Its flags.
    pub flags: u32,
    /// The firing position groups it uses, a bit each, in each of its
    /// eight situations.
    pub firing_position_groups: [u32; 8],
    /// The fewest actors it places, and the most.
    pub count: [i16; 2],
    /// Where its actors start.
    pub starting_locations: Vec<StartingLocation>,
}

/// Where one of a squad's actors starts.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct StartingLocation {
    /// Where it stands.
    pub position: [f32; 3],
    /// Its yaw, in radians.
    pub facing: f32,
    /// Its own actor variant, or `-1` for its squad's.
    pub actor: i16,
    /// Its command list, or `-1`.
    pub command_list: i16,
    /// Its initial state, or `0` for its squad's.
    pub initial_state: i16,
}

/// A place an encounter's actors fight from.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct FiringPosition {
    /// Where it is.
    pub position: [f32; 3],
    /// Its group, a letter from 0.
    pub group: i16,
}

/// A list of commands a script gives actors.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CommandList {
    /// Its name.
    pub name: String,
    /// Its flags.
    pub flags: u32,
    /// Its commands, in order.
    pub commands: Vec<Command>,
    /// The points its commands name.
    pub points: Vec<[f32; 3]>,
}

/// A command of a command list: an atom and what it takes.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Command {
    /// What it does (Halo's `ai_atom` numbers: pause, go to, go to and
    /// face, move in direction, look, animation mode, crouch, shoot,
    /// grenade, vehicle, running jump, targeted jump, script, animate,
    /// recording, action, vocalize, targeting, initiative, wait, loop,
    /// die, move immediate, look random, look player, look object, set
    /// radius, teleport).
    pub atom: i16,
    /// Its modifier.
    pub modifier: i16,
    /// Its two numbers.
    pub parameters: [f32; 2],
    /// Its two points, from the list's points, or `-1` (stale where its
    /// atom takes none).
    pub points: [i16; 2],
    /// The object name it names, or `-1`.
    pub object_name: i16,
}
