//! A scenario's scripts: its compiled HaloScript, as a tree of typed nodes
//! in one list, each child named by its index in the list.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// A scenario's scripts and globals, and the nodes their expressions are
/// made of.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Scripts {
    /// Every node of every expression.
    pub nodes: Vec<ScriptNode>,
    /// The scripts, in the scenario's order.
    pub scripts: Vec<Script>,
    /// The scenario's globals, in its order.
    pub globals: Vec<ScriptGlobal>,
}

/// A script.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Script {
    /// Its name.
    pub name: String,
    /// When it runs.
    pub kind: ScriptKind,
    /// What it returns (a static script's value).
    pub return_type: ValueType,
    /// Its expression: a node's index.
    pub root: u32,
}

/// When a script runs.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub enum ScriptKind {
    /// Once, when the level starts.
    Startup,
    /// When another script wakes it.
    Dormant,
    /// Again and again, every tick it is not asleep.
    Continuous,
    /// When another script calls it, returning its value.
    Static,
    /// A static script with no body yet.
    Stub,
}

impl ScriptKind {
    /// The kind of Halo's number for it.
    pub fn from_index(index: i16) -> Option<Self> {
        Some(match index {
            0 => Self::Startup,
            1 => Self::Dormant,
            2 => Self::Continuous,
            3 => Self::Static,
            4 => Self::Stub,
            _ => return None,
        })
    }
}

/// A global of the scenario's.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ScriptGlobal {
    /// Its name.
    pub name: String,
    /// Its type.
    pub value_type: ValueType,
    /// The expression it starts as, if any: a node's index.
    pub initial: Option<u32>,
}

/// A node of an expression, and the type its parent takes its value as.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ScriptNode {
    /// The type of its value.
    pub value_type: ValueType,
    /// What it is.
    pub kind: NodeKind,
}

/// What a node is.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub enum NodeKind {
    /// A value written in the script, and the type it was written as (an
    /// object's name where an object is taken, say).
    Constant(
        #[doc = "The type it was written as."] ValueType,
        #[doc = "The value."] Constant,
    ),
    /// A global's value.
    Global(#[doc = "The global."] GlobalRef),
    /// A call of a function (`begin`, `if` and `set` among them).
    Call(#[doc = "The call."] Call),
    /// A call of one of the scenario's static scripts.
    ScriptCall(#[doc = "The script's index."] u32),
}

/// A global a node names.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct GlobalRef {
    /// Its name.
    pub name: String,
    /// Whether it is one of the engine's, not the scenario's.
    pub external: bool,
    /// Its index: in the scenario's globals, or the engine's.
    pub index: u16,
}

/// A call of a function.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Call {
    /// The function's name.
    pub function: String,
    /// Its arguments: nodes' indices.
    pub arguments: Vec<u32>,
}

/// A value written in a script.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub enum Constant {
    /// A boolean.
    Boolean(#[doc = "The value."] bool),
    /// A real number.
    Real(#[doc = "The value."] f32),
    /// A short integer.
    Short(#[doc = "The value."] i16),
    /// A long integer.
    Long(#[doc = "The value."] i32),
    /// A string.
    String(#[doc = "The value."] String),
    /// Something of the scenario's or the game's named in the script: a
    /// trigger volume, an object's name, an encounter, a team, a
    /// difficulty, and so on.
    Named(#[doc = "The name and its index."] Named),
    /// A tag, or none.
    Tag(#[doc = "The value."] Option<TagRef>),
}

/// Something a script names, and its index in its list.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Named {
    /// The name written.
    pub name: String,
    /// Its index in its list (`-1` for none).
    pub index: i32,
}

/// The type of a script's value, numbered as Halo numbers them.
#[allow(missing_docs)]
#[derive(
    Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Archive, Serialize, Deserialize,
)]
pub enum ValueType {
    Unparsed,
    SpecialForm,
    FunctionName,
    Passthrough,
    Void,
    Boolean,
    Real,
    Short,
    Long,
    String,
    Script,
    TriggerVolume,
    CutsceneFlag,
    CutsceneCameraPoint,
    CutsceneTitle,
    CutsceneRecording,
    DeviceGroup,
    Ai,
    AiCommandList,
    StartingProfile,
    Conversation,
    Navpoint,
    HudMessage,
    ObjectList,
    Sound,
    Effect,
    Damage,
    LoopingSound,
    AnimationGraph,
    ActorVariant,
    DamageEffect,
    ObjectDefinition,
    GameDifficulty,
    Team,
    AiDefaultState,
    ActorType,
    HudCorner,
    Object,
    Unit,
    Vehicle,
    Weapon,
    Device,
    Scenery,
    ObjectName,
    UnitName,
    VehicleName,
    WeaponName,
    DeviceName,
    SceneryName,
}

impl ValueType {
    /// Every type, in Halo's order.
    pub const ALL: [Self; 49] = [
        Self::Unparsed,
        Self::SpecialForm,
        Self::FunctionName,
        Self::Passthrough,
        Self::Void,
        Self::Boolean,
        Self::Real,
        Self::Short,
        Self::Long,
        Self::String,
        Self::Script,
        Self::TriggerVolume,
        Self::CutsceneFlag,
        Self::CutsceneCameraPoint,
        Self::CutsceneTitle,
        Self::CutsceneRecording,
        Self::DeviceGroup,
        Self::Ai,
        Self::AiCommandList,
        Self::StartingProfile,
        Self::Conversation,
        Self::Navpoint,
        Self::HudMessage,
        Self::ObjectList,
        Self::Sound,
        Self::Effect,
        Self::Damage,
        Self::LoopingSound,
        Self::AnimationGraph,
        Self::ActorVariant,
        Self::DamageEffect,
        Self::ObjectDefinition,
        Self::GameDifficulty,
        Self::Team,
        Self::AiDefaultState,
        Self::ActorType,
        Self::HudCorner,
        Self::Object,
        Self::Unit,
        Self::Vehicle,
        Self::Weapon,
        Self::Device,
        Self::Scenery,
        Self::ObjectName,
        Self::UnitName,
        Self::VehicleName,
        Self::WeaponName,
        Self::DeviceName,
        Self::SceneryName,
    ];

    /// The type of Halo's number for it.
    pub fn from_index(index: i16) -> Option<Self> {
        usize::try_from(index)
            .ok()
            .and_then(|index| Self::ALL.get(index).copied())
    }

    /// Whether it is a number: a real, a short or a long.
    pub fn is_number(self) -> bool {
        matches!(self, Self::Real | Self::Short | Self::Long)
    }

    /// Whether its values are tags.
    pub fn is_tag(self) -> bool {
        (Self::Sound..=Self::ObjectDefinition).contains(&self)
    }

    /// Whether its values are objects.
    pub fn is_object(self) -> bool {
        (Self::Object..=Self::Scenery).contains(&self)
    }

    /// Whether its values are objects' names.
    pub fn is_object_name(self) -> bool {
        (Self::ObjectName..=Self::SceneryName).contains(&self)
    }
}
