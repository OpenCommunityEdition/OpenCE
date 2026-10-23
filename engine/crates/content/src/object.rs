//! Objects: their definitions, and the models, animation graphs and
//! collision models they are drawn, moved and hit with.

use rkyv::{Archive, Deserialize, Serialize};

use crate::{
    CollisionBsp, Equipment, FunctionModes, ObjectFunction, Projectile, TagRef, Vehicle, Weapon,
};

/// What kind of object a definition is, as its type field and its tag
/// group say.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Archive, Serialize, Deserialize)]
pub enum ObjectKind {
    /// A biped (`bipd`).
    Biped,
    /// A vehicle (`vehi`).
    Vehicle,
    /// A weapon (`weap`).
    Weapon,
    /// Equipment (`eqip`).
    Equipment,
    /// Garbage (`garb`).
    Garbage,
    /// A projectile (`proj`).
    Projectile,
    /// Scenery (`scen`).
    Scenery,
    /// A machine (`mach`).
    Machine,
    /// A control (`ctrl`).
    Control,
    /// A light fixture (`lifi`).
    LightFixture,
    /// A placeholder (`plac`).
    Placeholder,
    /// Sound scenery (`ssce`).
    SoundScenery,
}

impl ObjectKind {
    /// Every kind, in the order of the game's type field.
    pub const ALL: [ObjectKind; 12] = [
        ObjectKind::Biped,
        ObjectKind::Vehicle,
        ObjectKind::Weapon,
        ObjectKind::Equipment,
        ObjectKind::Garbage,
        ObjectKind::Projectile,
        ObjectKind::Scenery,
        ObjectKind::Machine,
        ObjectKind::Control,
        ObjectKind::LightFixture,
        ObjectKind::Placeholder,
        ObjectKind::SoundScenery,
    ];

    /// The kind's tag group.
    pub fn group(self) -> [u8; 4] {
        *match self {
            ObjectKind::Biped => b"bipd",
            ObjectKind::Vehicle => b"vehi",
            ObjectKind::Weapon => b"weap",
            ObjectKind::Equipment => b"eqip",
            ObjectKind::Garbage => b"garb",
            ObjectKind::Projectile => b"proj",
            ObjectKind::Scenery => b"scen",
            ObjectKind::Machine => b"mach",
            ObjectKind::Control => b"ctrl",
            ObjectKind::LightFixture => b"lifi",
            ObjectKind::Placeholder => b"plac",
            ObjectKind::SoundScenery => b"ssce",
        }
    }
}

/// What every object's definition has, whatever its kind.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ObjectDefinition {
    /// Its tag.
    pub tag: TagRef,
    /// Its kind.
    pub kind: ObjectKind,
    /// Its flags.
    pub flags: u16,
    /// The radius of the sphere it fits in.
    pub bounding_radius: f32,
    /// The sphere's centre, from the object's origin.
    pub bounding_offset: [f32; 3],
    /// How much a damage effect's push moves it.
    pub acceleration_scale: f32,
    /// The radius it is drawn within (0: the bounding radius).
    pub render_bounding_radius: f32,
    /// Its model.
    pub model: Option<TagRef>,
    /// Its animation graph.
    pub animation_graph: Option<TagRef>,
    /// Its collision model.
    pub collision_model: Option<TagRef>,
    /// Its physics.
    pub physics: Option<TagRef>,
    /// The shader its model's shaders are modified with.
    pub modifier_shader: Option<TagRef>,
    /// The effect played where it is created.
    pub creation_effect: Option<TagRef>,
    /// Where its incoming values come from.
    pub function_modes: FunctionModes,
    /// Its functions (the first four are computed).
    pub functions: Vec<ObjectFunction>,
    /// What is attached to its markers.
    pub attachments: Vec<Attachment>,
    /// Its change colours.
    pub change_colors: Vec<ChangeColor>,
    /// Its widgets (light volumes and glows, among the level's), drawn at
    /// its markers.
    pub widgets: Vec<TagRef>,
    /// A unit's HUD (`unhi`): a biped's or a vehicle's.
    pub unit_hud: Option<TagRef>,
    /// The damage a biped's own melee does, without a weapon (the flood's
    /// forms have one; bipeds only).
    pub unit_melee_damage: Option<TagRef>,
    /// The effects and sounds of a biped's footsteps (`foot`), by the
    /// ground's material (bipeds only).
    pub material_effects: Option<TagRef>,
    /// A biped's movement (bipeds only).
    pub biped: Option<Biped>,
    /// A weapon's own part (weapons only).
    pub weapon: Option<Weapon>,
    /// A projectile's own part (projectiles only).
    pub projectile: Option<Projectile>,
    /// Equipment's own part (equipment only).
    pub equipment: Option<Equipment>,
    /// A vehicle's own part and its seats (vehicles only).
    pub vehicle: Option<Vehicle>,
    /// A device's own part (machines, controls and light fixtures only).
    pub device: Option<Device>,
}

/// A device's own part of its definition (`_device_definition`): how its
/// power and position move, in fractions a tick as the cache bakes them,
/// what it exports to its functions and the effects it plays.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Device {
    /// Its flags ([`Device::POSITION_LOOPS`] and the other).
    pub flags: u32,
    /// What it exports into each incoming value (`_device_function_*`: 0
    /// none, 1 power, 2 change in power, 3 position, 4 change in position,
    /// 5 locked, 6 delay).
    pub function_modes: [i16; 4],
    /// The effects it plays as its position starts moving up and down.
    pub start_effects: [Option<TagRef>; 2],
    /// The effects it plays as its position stops moving up and down.
    pub stop_effects: [Option<TagRef>; 2],
    /// The effect it plays as its delay begins.
    pub delay_effect: Option<TagRef>,
    /// How near a biped opens a door by itself (0: its bounding radius).
    pub automatic_activation_radius: f32,
    /// How much its power's speed changes in a tick.
    pub maximum_power_acceleration: f32,
    /// Its power's top speed, a tick.
    pub maximum_power_velocity: f32,
    /// How much its position's speed changes in a tick, without power.
    pub depowered_position_acceleration: f32,
    /// Its position's top speed, a tick, without power.
    pub depowered_position_velocity: f32,
    /// How much its position's speed changes in a tick, powered.
    pub powered_position_acceleration: f32,
    /// Its position's top speed, a tick, powered.
    pub powered_position_velocity: f32,
    /// The ticks it waits, closed, before it opens.
    pub delay_ticks: f32,
    /// A machine's own part (machines only).
    pub machine: Option<Machine>,
}

impl Device {
    /// Its position wraps from open back to closed.
    pub const POSITION_LOOPS: u32 = 1 << 0;
    /// Its position animation is posed by whole frames.
    pub const POSITION_NOT_INTERPOLATED: u32 = 1 << 1;
}

/// A machine's own part of its definition (`_machine_definition`).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct Machine {
    /// Its type: [`Machine::DOOR`], 1 a platform (it moves only as told),
    /// or [`Machine::GEAR`].
    pub machine_type: i16,
    /// Its flags (bit 0 a pathfinding obstacle, 1 not one when open, 2 an
    /// elevator).
    pub flags: u16,
    /// The ticks a door stays open before it closes by itself.
    pub door_open_ticks: i32,
    /// The node an elevator carries its riders by, or `-1`.
    pub elevator_node: i16,
}

impl Machine {
    /// A door: it opens as a biped comes near, and closes again.
    pub const DOOR: i16 = 0;
    /// A gear: it turns without end, by its power (the decompilation's
    /// `_machine_type_platform`).
    pub const GEAR: i16 = 2;
}

/// A biped's movement and stance, from the biped part of its tag. Speeds
/// are in world units a second, angles in radians, times in seconds.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Biped {
    /// Its flags ([`Biped::USES_PLAYER_PHYSICS`] and the rest).
    pub flags: u32,
    /// How fast it turns while moving.
    pub moving_turning_speed: f32,
    /// The angle it turns to before its feet follow, standing.
    pub stationary_turning_threshold: f32,
    /// The steepest ground it stands on.
    pub maximum_slope_angle: f32,
    /// Where going downhill starts to change its speed.
    pub downhill_falloff_angle: f32,
    /// Where going downhill changes its speed fully.
    pub downhill_cutoff_angle: f32,
    /// Its speed downhill, past the cutoff, as a scale.
    pub downhill_velocity_scale: f32,
    /// Where going uphill starts to change its speed.
    pub uphill_falloff_angle: f32,
    /// Where going uphill changes its speed fully.
    pub uphill_cutoff_angle: f32,
    /// Its speed uphill, past the cutoff, as a scale.
    pub uphill_velocity_scale: f32,
    /// Its speed up, jumping.
    pub jump_velocity: f32,
    /// The longest it recovers from a soft landing.
    pub maximum_soft_landing_time: f32,
    /// The longest it recovers from a hard landing.
    pub maximum_hard_landing_time: f32,
    /// The slowest landing that is soft.
    pub minimum_soft_landing_velocity: f32,
    /// The slowest landing that is hard.
    pub minimum_hard_landing_velocity: f32,
    /// The landing at which a hard landing's recovery is longest.
    pub maximum_hard_landing_velocity: f32,
    /// A landing that kills it.
    pub death_hard_landing_velocity: f32,
    /// Its camera's height, standing.
    pub standing_camera_height: f32,
    /// Its camera's height, crouching.
    pub crouching_camera_height: f32,
    /// How long it takes to crouch or stand.
    pub crouch_transition_time: f32,
    /// How fast it throws a grenade (from its unit part).
    pub grenade_velocity: f32,
    /// Its height, standing.
    pub collision_height_standing: f32,
    /// Its height, crouching.
    pub collision_height_crouching: f32,
    /// Its radius.
    pub collision_radius: f32,
    /// The cosine of the steepest ground it stands on (worked out by the
    /// tools that built the map, as the rest below).
    pub minimum_normal_k: f32,
    /// The slope at which going downhill starts to change its speed.
    pub downhill_k0: f32,
    /// The slope at which going downhill changes its speed fully.
    pub downhill_k1: f32,
    /// The slope at which going uphill starts to change its speed.
    pub uphill_k0: f32,
    /// The slope at which going uphill changes its speed fully.
    pub uphill_k1: f32,
}

impl Biped {
    /// It moves as a player does, by the globals' speeds.
    pub const USES_PLAYER_PHYSICS: u32 = 1 << 1;
    /// It flies.
    pub const FLYING: u32 = 1 << 2;
    /// Its collision pill's centre is its origin, not its feet.
    pub const PILL_CENTERED_AT_ORIGIN: u32 = 1 << 3;
    /// It is a sphere, not a pill.
    pub const SPHERICAL: u32 = 1 << 4;
    /// Falls do not harm it.
    pub const IMMUNE_TO_FALLING_DAMAGE: u32 = 1 << 7;
    /// It moves by the old player speeds (the override in the game's code).
    pub const USES_OLD_PLAYER_PHYSICS: u32 = 1 << 12;
}

/// Something attached to an object's marker (a light, an effect).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Attachment {
    /// What is attached.
    pub tag: Option<TagRef>,
    /// The marker it is attached to.
    pub marker: String,
    /// The outgoing function (1 to 4, A to D) a light's intensity is
    /// (0: none, which is 1).
    pub primary_scale: i16,
    /// The change colour (1 to 4, A to D) a light's colour is scaled by
    /// (0: none, which is white).
    pub change_color: i16,
}

/// A change colour: the colours an object of the definition may be
/// tinted with.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ChangeColor {
    /// The function its colour is darkened by.
    pub darken_by: i16,
    /// The function its colour is scaled by.
    pub scale_by: i16,
    /// How the scale is applied.
    pub scale_flags: u32,
    /// The colour at the scale's least.
    pub lower: [f32; 3],
    /// The colour at the scale's greatest.
    pub upper: [f32; 3],
    /// The colours chosen from at random.
    pub permutations: Vec<ColorPermutation>,
}

/// One of a change colour's colours, chosen by its weight.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ColorPermutation {
    /// How likely it is.
    pub weight: f32,
    /// The colour at the scale's least.
    pub lower: [f32; 3],
    /// The colour at the scale's greatest.
    pub upper: [f32; 3],
}

/// A model: its skeleton, markers, regions and the geometry each
/// region's permutations draw.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Model {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags.
    pub flags: u32,
    /// The checksum of its nodes, which an animation made for them has.
    pub node_list_checksum: i32,
    /// The pixel sizes below which each level of detail is drawn, from
    /// super low to super high.
    pub detail_cutoffs: [f32; 5],
    /// The base map's scale, u and v.
    pub base_map_scale: [f32; 2],
    /// Its nodes: the skeleton, each after its parent.
    pub nodes: Vec<ModelNode>,
    /// Its markers.
    pub markers: Vec<ModelMarker>,
    /// Its regions.
    pub regions: Vec<ModelRegion>,
    /// Its geometries, which the permutations index.
    pub geometries: Vec<ModelGeometry>,
    /// Its shaders, which the parts index.
    pub shaders: Vec<ModelShader>,
}

/// A node of a model's skeleton, at its default pose.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelNode {
    /// Its name.
    pub name: String,
    /// Its parent.
    pub parent: Option<u16>,
    /// Its first child.
    pub first_child: Option<u16>,
    /// Its next sibling.
    pub next_sibling: Option<u16>,
    /// Where it is, from its parent.
    pub translation: [f32; 3],
    /// How it is turned, from its parent: i, j, k, w.
    pub rotation: [f32; 4],
    /// How far it is from its parent.
    pub distance_from_parent: f32,
    /// The scale of the inverse of its matrix at the default pose, which
    /// takes the model's vertices into the node's space.
    pub default_inverse_scale: f32,
    /// That inverse's forward, left and up vectors and its position.
    pub default_inverse: [[f32; 3]; 4],
}

/// A named place on a model, once in each region and permutation it is in.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelMarker {
    /// Its name.
    pub name: String,
    /// Where it is.
    pub instances: Vec<MarkerInstance>,
}

/// A marker in one region's permutation.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct MarkerInstance {
    /// Its region.
    pub region: u8,
    /// Its permutation in the region.
    pub permutation: u8,
    /// The node it moves with.
    pub node: u8,
    /// Where it is, from the node.
    pub translation: [f32; 3],
    /// How it is turned, from the node: i, j, k, w.
    pub rotation: [f32; 4],
}

/// A part of a model that has permutations (a head, a visor).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelRegion {
    /// Its name.
    pub name: String,
    /// Its permutations.
    pub permutations: Vec<ModelPermutation>,
}

/// One way a region may look.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelPermutation {
    /// Its name.
    pub name: String,
    /// Its flags (bit 0: not chosen at random).
    pub flags: u32,
    /// The geometry of each level of detail, super low to super high.
    pub geometries: [Option<u16>; 5],
}

/// Geometry: the parts drawn together.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelGeometry {
    /// Its parts.
    pub parts: Vec<ModelPart>,
}

/// A part of a geometry: one shader's triangle strip.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelPart {
    /// Its flags.
    pub flags: u32,
    /// Its shader, in the model's shaders.
    pub shader: u16,
    /// Its centroid.
    pub centroid: [f32; 3],
    /// Its vertices.
    pub vertices: Vec<ModelVertex>,
    /// Its triangle strip, as indices into its vertices.
    pub strip: Vec<u16>,
}

/// A vertex of a model, skinned to at most two of its nodes.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelVertex {
    /// Where it is.
    pub position: [f32; 3],
    /// Its normal.
    pub normal: [f32; 3],
    /// Its binormal.
    pub binormal: [f32; 3],
    /// Its tangent.
    pub tangent: [f32; 3],
    /// Its texture coordinates.
    pub texcoord: [f32; 2],
    /// The model's nodes it moves with.
    pub nodes: [Option<u16>; 2],
    /// How much it moves with each.
    pub weights: [f32; 2],
}

/// A shader a model's parts are drawn with.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelShader {
    /// The shader.
    pub shader: Option<TagRef>,
    /// Its permutation.
    pub permutation: i16,
}

/// An animation graph: the animations of a model's nodes, and which an
/// object, a unit's seat and its weapon play.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct AnimationGraph {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags.
    pub flags: u16,
    /// The radius a limp body's nodes keep from each other.
    pub limp_body_node_radius: f32,
    /// The nodes it animates.
    pub nodes: Vec<AnimationNode>,
    /// Its animations.
    pub animations: Vec<Animation>,
    /// The animations an object plays over its others.
    pub object_overlays: Vec<ObjectOverlay>,
    /// The animations of a unit, by seat.
    pub unit_seats: Vec<UnitSeat>,
    /// A weapon's own animations, in the game's order (idle, ready,
    /// put-away, reload-1, ...).
    pub weapon: Vec<Option<u16>>,
    /// A vehicle's own animations: its overlays and its suspension.
    pub vehicle: Option<VehicleAnimations>,
    /// A device's animations, in the game's order (position, power).
    pub device: Vec<Option<u16>>,
    /// A unit's damage animations, by
    /// [`AnimationGraph::damage_index`].
    pub unit_damage: Vec<Option<u16>>,
    /// A first-person weapon's animations, in the game's order (idle,
    /// posing, fire-1, moving, overlays, ...): the hands and weapon the
    /// player sees.
    pub first_person_weapon: Vec<Option<u16>>,
    /// The sounds its animations play (none for an empty reference).
    pub sound_references: Vec<Option<TagRef>>,
}

impl AnimationGraph {
    /// How many damage animations a graph may have: four kinds (soft and
    /// hard ping, soft and hard kill), four directions and eleven parts.
    pub const DAMAGE_ANIMATIONS: usize = 4 * 4 * 11;

    /// Where a damage animation is in [`AnimationGraph::unit_damage`]
    /// (`build_damage_animation_index`): none for a kind, direction or part
    /// out of range.
    pub fn damage_index(kind: usize, direction: usize, part: usize) -> Option<usize> {
        (kind < 4 && direction < 4 && part < 11).then_some((kind * 4 + direction) * 11 + part)
    }

    /// The animation an index of the graph names, if it has it.
    pub fn animation(&self, index: Option<u16>) -> Option<&Animation> {
        self.animations.get(usize::from(index?))
    }
}

/// An animation an object plays over its others, driven by one of its
/// functions.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct ObjectOverlay {
    /// The animation.
    pub animation: Option<u16>,
    /// The object's function that drives it.
    pub function: i16,
    /// Whether the function picks its frame (0) or scales it (1).
    pub mode: i16,
}

/// How far an aiming or looking animation turns: the yaw and pitch each of
/// its frames turns (radians), and the frames on each side of its centre.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct ScreenBounds {
    /// The yaw of each frame to the left.
    pub negative_yaw: f32,
    /// The yaw of each frame to the right.
    pub positive_yaw: f32,
    /// The frames from the centre to the left edge.
    pub negative_yaw_frames: i16,
    /// The frames from the centre to the right edge.
    pub positive_yaw_frames: i16,
    /// The pitch of each frame down.
    pub negative_pitch: f32,
    /// The pitch of each frame up.
    pub positive_pitch: f32,
    /// The frames from the centre to the bottom edge.
    pub negative_pitch_frames: i16,
    /// The frames from the centre to the top edge.
    pub positive_pitch_frames: i16,
}

/// A marker a unit's hand is drawn to, on what it holds or rides.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct IkPoint {
    /// The unit's marker (`left hand`).
    pub marker: String,
    /// The marker of the weapon or vehicle it is drawn to.
    pub attached_to: String,
}

/// A vehicle's animations.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct VehicleAnimations {
    /// How far its steering animation turns.
    pub steering_bounds: ScreenBounds,
    /// Its overlays, in the game's order (steering, roll, throttle,
    /// velocity, braking, ground-speed, occupied, unoccupied).
    pub animations: Vec<Option<u16>>,
    /// Its wheels' suspension.
    pub suspensions: Vec<Suspension>,
}

/// A wheel's suspension: the overlay a mass point's compression plays.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Suspension {
    /// The vehicle's mass point.
    pub mass_point: i16,
    /// The overlay.
    pub animation: Option<u16>,
    /// The compression at the overlay's first frame.
    pub full_extension: f32,
    /// The compression at its last.
    pub full_compression: f32,
}

/// A node an animation graph animates.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct AnimationNode {
    /// Its name.
    pub name: String,
    /// Its parent.
    pub parent: Option<u16>,
    /// Its first child.
    pub first_child: Option<u16>,
    /// Its next sibling.
    pub next_sibling: Option<u16>,
}

/// What an animation does to the nodes it moves.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub enum AnimationKind {
    /// It poses the nodes.
    Base,
    /// It is added to their pose.
    Overlay,
    /// It replaces their pose.
    Replacement,
}

/// An animation, its frames as the game stores them.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Animation {
    /// Its name.
    pub name: String,
    /// What it does to the nodes.
    pub kind: AnimationKind,
    /// How many frames it has.
    pub frame_count: u16,
    /// The bytes of each frame of its data.
    pub frame_size: u16,
    /// What its frame info holds (none, dx dy, dx dy yaw, dx dy dz yaw).
    pub frame_info_type: i16,
    /// The checksum of the nodes it was made for.
    pub node_list_checksum: i32,
    /// How many nodes it moves.
    pub node_count: u16,
    /// The frame it loops to.
    pub loop_frame: u16,
    /// How likely it is to be chosen from its siblings.
    pub weight: f32,
    /// Its key frame.
    pub key_frame: i16,
    /// Its second key frame.
    pub second_key_frame: i16,
    /// The next of its permutations, in the graph's.
    pub next_animation: Option<u16>,
    /// The first of its permutations, which one is chosen from when it
    /// ends.
    pub parent_animation: Option<u16>,
    /// Where in its permutations' weights its own ends, from 0 to 1.
    pub normalized_weight: f32,
    /// Its flags (bit 0 compressed, bit 1 world relative, bit 2 at 25 Hz).
    pub flags: u16,
    /// The sound it plays, in the graph's sound references.
    pub sound: Option<u16>,
    /// The frame it plays the sound on.
    pub sound_frame: i16,
    /// The frame the left foot lands on.
    pub left_foot_frame: i8,
    /// The frame the right foot lands on.
    pub right_foot_frame: i8,
    /// Each frame's motion of the object.
    pub frame_info: Vec<u8>,
    /// Which nodes it translates, one bit a node.
    pub translation_flags: [u32; 2],
    /// Which nodes it rotates.
    pub rotation_flags: [u32; 2],
    /// Which nodes it scales.
    pub scale_flags: [u32; 2],
    /// Where its compressed frames start in its data.
    pub compressed_data_offset: u32,
    /// The pose of the nodes it does not move.
    pub default_data: Vec<u8>,
    /// Its frames.
    pub data: Vec<u8>,
}

impl Animation {
    /// Whether its frames are compressed.
    pub fn is_compressed(&self) -> bool {
        self.flags & 1 != 0
    }
}

/// The animations of a unit in one kind of seat.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct UnitSeat {
    /// Its label (`stand`, `warthog-driver`).
    pub label: String,
    /// How far its looking animation turns.
    pub looking_bounds: ScreenBounds,
    /// Its animations, in the game's order for a seat.
    pub animations: Vec<Option<u16>>,
    /// The markers its hands are drawn to.
    pub ik_points: Vec<IkPoint>,
    /// Its animations with each class of weapon.
    pub weapon_classes: Vec<WeaponClass>,
}

/// A unit's animations with a class of weapon (`pistol`, `rifle`).
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponClass {
    /// Its label.
    pub label: String,
    /// The weapon's marker the unit holds.
    pub grip_marker: String,
    /// The unit's marker that holds it.
    pub hand_marker: String,
    /// How far its aiming animations turn.
    pub aiming_bounds: ScreenBounds,
    /// Its animations, in the game's order for a weapon.
    pub animations: Vec<Option<u16>>,
    /// The markers the unit's hands are drawn to.
    pub ik_points: Vec<IkPoint>,
    /// Its animations with each type of the class.
    pub weapon_types: Vec<WeaponType>,
}

/// A unit's animations with a type of weapon of a class.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponType {
    /// Its label.
    pub label: String,
    /// Its animations, in the game's order for a weapon type.
    pub animations: Vec<Option<u16>>,
}

/// A collision model: what an object is hit with, what it is made of and
/// how much damage it takes.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CollisionModel {
    /// Its tag.
    pub tag: TagRef,
    /// How it takes damage ([`CollisionModel::TAKES_SHIELD_FOR_CHILDREN`]
    /// and the others).
    pub flags: u32,
    /// The material that damage without a surface (an explosion's) hits.
    pub indirect_damage_material: Option<u16>,
    /// The body's vitality.
    pub maximum_body_vitality: f32,
    /// How much less damage its own side does it.
    pub friendly_damage_resistance: f32,
    /// The body's vitality below which it shows its damage.
    pub body_damaged_threshold: f32,
    /// The body's vitality below which it is destroyed, not killed (below
    /// 0, when it is below 0).
    pub body_destroyed_threshold: f32,
    /// The shield's vitality.
    pub maximum_shield_vitality: f32,
    /// The physics material of the shield.
    pub shield_material_type: i16,
    /// How the shield's failure grows: a transition function (linear,
    /// early, very early, late, very late, cosine).
    pub shield_failure_function: i16,
    /// The shield's vitality below which it starts to fail.
    pub shield_failure_threshold: f32,
    /// The most damage that leaks through a failing shield.
    pub failing_shield_leak_fraction: f32,
    /// The least damage to the shield that stuns it.
    pub minimum_stun_damage: f32,
    /// How long a stunned shield waits to recharge, in seconds.
    pub stun_time: f32,
    /// How long the shield takes to recharge fully, in seconds.
    pub recharge_time: f32,
    /// The shield's vitality below which it shows its damage.
    pub shield_damaged_threshold: f32,
    /// What its surfaces are made of.
    pub materials: Vec<CollisionModelMaterial>,
    /// Its regions, each with its permutations' names.
    pub regions: Vec<CollisionRegion>,
    /// The box pathfinding keeps out of: x, y and z, each least and
    /// greatest.
    pub pathfinding_box: [[f32; 2]; 3],
    /// The spheres pathfinding keeps out of.
    pub pathfinding_spheres: Vec<PathfindingSphere>,
    /// Its nodes, each with its collision.
    pub nodes: Vec<CollisionNode>,
}

impl CollisionModel {
    /// Its children's damage goes to its shield.
    pub const TAKES_SHIELD_FOR_CHILDREN: u32 = 1 << 0;
    /// Its children's damage goes to its body.
    pub const TAKES_BODY_FOR_CHILDREN: u32 = 1 << 1;
    /// Its shield always takes its own side's damage.
    pub const ALWAYS_SHIELDS_FRIENDLY_DAMAGE: u32 = 1 << 2;
    /// Only explosions hurt it.
    pub const ONLY_HURT_BY_EXPLOSIVES: u32 = 1 << 5;

    /// The material of its node `node`: the one most of its first BSP's
    /// surfaces are made of.
    pub fn node_material(&self, node: usize) -> Option<u16> {
        let bsp = self.nodes.get(node)?.bsps.first()?;
        let mut counts = vec![0u32; self.materials.len()];
        for surface in &bsp.surfaces {
            if let Some(count) = surface
                .material
                .and_then(|material| counts.get_mut(usize::from(material)))
            {
                *count += 1;
            }
        }
        let (most, &count) = counts
            .iter()
            .enumerate()
            .max_by_key(|&(index, count)| (*count, std::cmp::Reverse(index)))?;
        (count > 0).then_some(most as u16)
    }
}

/// What a collision model's surface is made of.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CollisionModelMaterial {
    /// Its name.
    pub name: String,
    /// Its flags ([`CollisionModelMaterial::HEAD`]).
    pub flags: u32,
    /// The physics material type.
    pub material_type: i16,
    /// How much damage leaks through the shield.
    pub shield_leak_percentage: f32,
    /// The shield's damage multiplier.
    pub shield_damage_multiplier: f32,
    /// The body's damage multiplier.
    pub body_damage_multiplier: f32,
}

impl CollisionModelMaterial {
    /// It is a head: headshots kill.
    pub const HEAD: u32 = 1 << 0;
}

/// A region of a collision model.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CollisionRegion {
    /// Its name.
    pub name: String,
    /// What its destruction does ([`CollisionRegion::FORCES_OBJECT_TO_DIE`]
    /// and the others).
    pub flags: u32,
    /// The damage that destroys it (0: it is not destroyed).
    pub damage_threshold: f32,
    /// Its permutations' names.
    pub permutations: Vec<String>,
}

impl CollisionRegion {
    /// Its destruction kills the object.
    pub const FORCES_OBJECT_TO_DIE: u32 = 1 << 1;
    /// It is destroyed when the object dies.
    pub const DIES_WHEN_OBJECT_DIES: u32 = 1 << 2;
}

/// A sphere pathfinding keeps out of.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct PathfindingSphere {
    /// The node it moves with.
    pub node: Option<u16>,
    /// Its centre, from the node.
    pub center: [f32; 3],
    /// Its radius.
    pub radius: f32,
}

/// A node of a collision model, with a BSP for each of its region's
/// permutations.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CollisionNode {
    /// Its name.
    pub name: String,
    /// Its region.
    pub region: Option<u16>,
    /// Its parent.
    pub parent: Option<u16>,
    /// Its next sibling.
    pub next_sibling: Option<u16>,
    /// Its first child.
    pub first_child: Option<u16>,
    /// Its BSPs, whose surfaces' materials are the model's.
    pub bsps: Vec<CollisionBsp>,
}
