//! The engine's content: the typed data a map is made of, as the importer
//! writes it and the engine reads it. Nothing here points into a cache:
//! every index is into one of these types' own lists, and every list is
//! one the importer checked.

pub mod bitmap;
pub mod campaign;
pub mod decal;
pub mod devmap;
pub mod hud;
pub mod lens;
pub mod level;
pub mod light;
pub mod material;
pub mod object;
pub mod pack;
pub mod script;
pub mod shader;
pub mod vehicle;
pub mod weapon;

pub use bitmap::{
    Bitmap, BitmapImage, Pixels, ResourceRange, Sequence, Sprite, Texture, TextureFormat,
};
pub use campaign::{
    ActorVariant, BspSwitch, CameraPoint, Campaign, Command, CommandList, CutsceneFlag,
    DeviceGroup, Encounter, FiringPosition, Recording, SoundLength, Squad, StartingLocation,
    TriggerVolume,
};
pub use decal::{Decal, LevelDecal};
pub use hud::{
    CrosshairItem, CrosshairKind, GrenadeHud, HudAnchor, HudDigits, HudGlobals, HudMeter,
    HudNumber, HudPlacement, HudStatic, UnitHud, WeaponHud, WeaponHudCrosshair, WeaponHudElement,
    WeaponHudMeter, WeaponHudNumber, WeaponHudState, WeaponHudStatic,
};
pub use lens::{
    Glow, LensFlare, LensFlareMarker, LensFlareReflection, LightVolume, LightVolumeFrame,
};
pub use level::{
    AtmosphericFog, CollisionBsp, CollisionMaterial, EnvironmentVertex, FallingDamage, GameType,
    GameTypes, Globals, ItemChoice, Level, Lightmap, LightmapVertex, Material, NetgameEquipment,
    NetgameFlag, NetgameFlagKind, ObjectPlacement, PlacementDevice, PlacementPermutation,
    PlayerPhysics, PlayerStart, Scenario, Sky, SkyAmbient, SkyLight, StructureBsp,
};
pub use light::{FunctionModes, Light, ObjectFunction};
pub use material::SurfaceMaterial;
pub use object::{
    Animation, AnimationGraph, AnimationKind, AnimationNode, Attachment, Biped, ChangeColor,
    CollisionModel, CollisionModelMaterial, CollisionNode, CollisionRegion, ColorPermutation,
    MarkerInstance, Model, ModelGeometry, ModelMarker, ModelNode, ModelPart, ModelPermutation,
    ModelRegion, ModelShader, ModelVertex, ObjectDefinition, ObjectKind, PathfindingSphere,
    UnitSeat, WeaponClass, WeaponType,
};
pub use script::{
    Call, Constant, GlobalRef, Named, NodeKind, Script, ScriptGlobal, ScriptKind, ScriptNode,
    Scripts, ValueType,
};
pub use shader::{
    AnimatedColor, ChicagoMap, EnvironmentAnimation, EnvironmentShader, GenericMap, GenericStage,
    GlassShader, MeterShader, ModelShaderClass, PlasmaNoise, PlasmaShader, Shader, ShaderClass,
    ShaderRadiosity, TextureAnimation, TransparentChicagoShader, TransparentGenericShader,
    WaterShader,
};
pub use vehicle::{
    FrictionType, MassPoint, Physics, PoweredMassPoint, PoweredSeat, Vehicle, VehicleKind,
    VehicleSeat,
};
pub use weapon::{
    AimAssist, Ammunition, DamageEffect, Effect, EffectDebris, EffectEvent, EffectPart, Equipment,
    Grenade, Item, Magazine, MaterialResponse, Projectile, Trigger, Weapon,
};

use rkyv::{Archive, Deserialize, Serialize};
use std::fmt;

/// A tag group's four-character code (`sbsp`), first character first.
#[derive(Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord, Archive, Serialize, Deserialize)]
pub struct Group(pub [u8; 4]);

impl fmt::Display for Group {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0
            .iter()
            .try_for_each(|&byte| write!(f, "{}", char::from(byte).escape_default()))
    }
}

impl fmt::Debug for Group {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "'{self}'")
    }
}

/// A tag, named by its group and path (`bitm`,
/// `levels\test\bloodgulch\bitmaps\grass`).
#[derive(Clone, Debug, PartialEq, Eq, Hash, PartialOrd, Ord, Archive, Serialize, Deserialize)]
pub struct TagRef {
    /// Its group.
    pub group: Group,
    /// Its path.
    pub path: String,
}
