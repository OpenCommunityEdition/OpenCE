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
    /// Each of the palette's actor variants, as its actors fight.
    pub actor_variants: Vec<Option<ActorVariant>>,
    /// Its encounters.
    pub encounters: Vec<Encounter>,
    /// Its AI command lists.
    pub command_lists: Vec<CommandList>,
    /// Its AI conversations.
    pub conversations: Vec<Conversation>,
    /// Its starting profiles' names.
    pub starting_profiles: Vec<String>,
    /// The trigger volumes that switch the structure a player stands in.
    pub bsp_switches: Vec<BspSwitch>,
    /// The length of each sound of the map, which its scripts wait on.
    pub sound_lengths: Vec<SoundLength>,
}

/// How long a sound plays: its longest permutation's length
/// (`sound_definition`'s `longest_permutation_length`).
#[derive(Clone, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct SoundLength {
    /// The sound's path.
    pub path: String,
    /// Its length in milliseconds.
    pub milliseconds: u32,
    /// Its class (`sound_class`), which tells the scripts' dialogue from
    /// other sounds.
    pub class: u16,
}

/// An AI conversation (`ai_conversation`): lines its participants speak in
/// turn.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Conversation {
    /// Its name.
    pub name: String,
    /// Who speaks.
    pub participants: Vec<ConversationParticipant>,
    /// What they say, in order.
    pub lines: Vec<ConversationLine>,
}

/// One who speaks in a conversation.
#[derive(Clone, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct ConversationParticipant {
    /// How the speaker is chosen: 1 a voice without a body, 6 and 7 the
    /// radio (heard everywhere alike); others an actor.
    pub selection: i16,
    /// The object name of the speaker already in the world; -1 for none.
    pub object_name: i16,
    /// The unit variants each of a line's six dialogue variants is for (0
    /// any, -1 none).
    pub variants: [i16; 6],
}

/// A line of a conversation.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ConversationLine {
    /// Its flags: [`ConversationLine::WAIT_UNTIL_TOLD`] and the looks.
    pub flags: u16,
    /// The participant who speaks it.
    pub participant: i16,
    /// The seconds waited after it before the next.
    pub delay: f32,
    /// The sound of each of its six variants.
    pub dialogue: Vec<Option<TagRef>>,
}

impl ConversationLine {
    /// The conversation waits after this line until a script advances it
    /// (`ai_conversation_advance`).
    pub const WAIT_UNTIL_TOLD: u16 = 1 << 3;
}

/// A trigger volume that switches the structure when a player enters it
/// while the structure it leaves is the one loaded
/// (`scenario_bsp_switch_trigger_volume`). Each index is the game's, `-1`
/// for none.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct BspSwitch {
    /// The trigger volume.
    pub trigger_volume: i16,
    /// The structure it switches from.
    pub source: i16,
    /// The structure it switches to.
    pub destination: i16,
    /// The cutscene flag the other players are moved to.
    pub cutscene_flag: i16,
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

/// An actor variant (`actv`) and its actor (`actr`): the unit its actors
/// are, the weapon they carry, and how they see and fight
/// (`actor_definitions.h`). Angles are in radians, distances in world
/// units, times in seconds.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ActorVariant {
    /// The variant.
    pub tag: TagRef,
    /// The unit its actors are.
    pub unit: Option<TagRef>,
    /// The weapon they carry.
    pub weapon: Option<TagRef>,
    /// The actor's type (elite, jackal, grunt, ..., marine, ...).
    pub kind: i16,
    /// How far it sees.
    pub vision_distance: f32,
    /// The widest angle from where it looks that it sees.
    pub vision_angle: f32,
    /// How far it hears.
    pub hearing_distance: f32,
    /// The farthest it fires at a target.
    pub firing_range: f32,
    /// Its shots a second while it fires.
    pub rate_of_fire: f32,
    /// How far from its aim its shots go.
    pub error_angle: f32,
    /// The nearest and farthest it keeps from its target.
    pub combat_range: [f32; 2],
    /// The shortest and longest bursts it fires.
    pub burst_duration: [f32; 2],
    /// The shortest and longest waits between its bursts.
    pub burst_separation: [f32; 2],
    /// The chance it panics when a friend is killed.
    pub panic_chance: f32,
    /// The fraction of its shield below which it hides.
    pub shield_fraction_hide: f32,
    /// The fraction its shield recharges to before it fights again.
    pub shield_fraction_emerge: f32,
    /// The actor's flags (`_actor_definition_*_bit`:
    /// [`ActorVariant::PANIC_ON_SURPRISE`] and the rest).
    pub actor_flags: u32,
    /// How near an enemy must be when it is first seen to surprise it.
    pub surprise_distance: f32,
    /// The damage in a moment, and the vitality below which, that send it
    /// berserk.
    pub berserk_damage: [f32; 2],
    /// How near an enemy sends it berserk.
    pub berserk_distance: f32,
}

impl ActorVariant {
    /// The actor panics when it is surprised.
    pub const PANIC_ON_SURPRISE: u32 = 1 << 10;
    /// The actor goes berserk whenever it attacks.
    pub const ALWAYS_BERSERK_IN_ATTACKING_MODE: u32 = 1 << 17;
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
