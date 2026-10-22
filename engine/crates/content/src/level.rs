//! A level: its scenario, its structure BSPs and what they are drawn with.

use rkyv::{Archive, Deserialize, Serialize};

use crate::{
    AnimationGraph, Bitmap, Campaign, CollisionModel, DamageEffect, Effect, Grenade, GrenadeHud,
    HudDigits, HudGlobals, Model, ObjectDefinition, ObjectKind, Physics, Scripts, Shader, TagRef,
    UnitHud, WeaponHud,
};

/// Everything a level's structure is drawn and collided with.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Level {
    /// The scenario.
    pub scenario: Scenario,
    /// Its structure BSPs, in the scenario's order.
    pub structure_bsps: Vec<StructureBsp>,
    /// The definitions of the objects the scenario places, by path.
    pub objects: Vec<ObjectDefinition>,
    /// Their models, by path.
    pub models: Vec<Model>,
    /// Their animation graphs, by path.
    pub animation_graphs: Vec<AnimationGraph>,
    /// Every shader the BSPs and models use, and the shaders those layer
    /// on, by path.
    pub shaders: Vec<Shader>,
    /// Every bitmap the BSPs and shaders use, by path.
    pub bitmaps: Vec<Bitmap>,
    /// Bitmaps whose definitions are in Custom Edition's `bitmaps.map`,
    /// not in the map: read with the shared resource maps.
    pub resource_bitmaps: Vec<TagRef>,
    /// The scenario's skies, in its order (the first is the one outside).
    pub skies: Vec<Sky>,
    /// The game's globals: the player's speeds and falling.
    pub globals: Globals,
    /// Every damage effect its objects and globals do, by path.
    pub damage_effects: Vec<DamageEffect>,
    /// Its objects' collision models, by path.
    pub collision_models: Vec<CollisionModel>,
    /// Its objects' physics, by path.
    pub physics: Vec<Physics>,
    /// The effects its projectiles detonate with and its weapons' shots
    /// make, by path.
    pub effects: Vec<Effect>,
    /// Its units' HUDs, by path.
    pub unit_huds: Vec<UnitHud>,
    /// Its weapons' HUDs, and the HUDs they add to, by path.
    pub weapon_huds: Vec<WeaponHud>,
    /// The grenades' HUDs, by path.
    pub grenade_huds: Vec<GrenadeHud>,
    /// The digits numbers are drawn with.
    pub hud_digits: Option<HudDigits>,
    /// The HUD's globals.
    pub hud_globals: Option<HudGlobals>,
}

/// The game's globals (the `matg` tag) that the simulation reads.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Globals {
    /// The player's speeds.
    pub player: PlayerPhysics,
    /// What falling does.
    pub falling: FallingDamage,
    /// The player's biped in the campaign.
    pub player_unit: Option<TagRef>,
    /// The player's biped in multiplayer.
    pub multiplayer_unit: Option<TagRef>,
    /// The weapon that is a team's flag in Capture the Flag.
    pub multiplayer_flag: Option<TagRef>,
    /// The grenades, frag then plasma.
    pub grenades: Vec<Grenade>,
    /// The weapons a game may give, in the game engine's order (the
    /// assault rifle first).
    pub weapons: Vec<TagRef>,
    /// The HUD's globals (`hudg`).
    pub hud_globals: Option<TagRef>,
    /// The digits the HUD's numbers are drawn with (`hud#`).
    pub hud_digits: Option<TagRef>,
    /// The motion sensor's blip.
    pub motion_sensor_blip: Option<TagRef>,
    /// The first-person view's hands (a `mod2`).
    pub first_person_hands: Option<TagRef>,
    /// The campaign's difficulty values (`game_globals.c`'s order: enemy
    /// damage, vitality and shield first), each at easy, normal, hard and
    /// legendary; none if the globals have none.
    pub difficulty: Vec<[f32; 4]>,
}

/// How a biped that uses player physics moves. Speeds are in world units a
/// second, accelerations in world units a second squared.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct PlayerPhysics {
    /// Walking (unused by the keyboard: the game walks at a run).
    pub walking_speed: f32,
    /// Running forward.
    pub run_forward: f32,
    /// Running backward.
    pub run_backward: f32,
    /// Running sideways.
    pub run_sideways: f32,
    /// How fast it reaches its running speed.
    pub run_acceleration: f32,
    /// Crouching, forward.
    pub sneak_forward: f32,
    /// Crouching, backward.
    pub sneak_backward: f32,
    /// Crouching, sideways.
    pub sneak_sideways: f32,
    /// How fast it reaches its crouching speed.
    pub sneak_acceleration: f32,
    /// How fast it changes its speed in the air.
    pub airborne_acceleration: f32,
    /// How much being stunned slows it.
    pub stun_movement_penalty: f32,
    /// How much being stunned slows its turning.
    pub stun_turning_penalty: f32,
    /// How much being stunned lowers its jumps.
    pub stun_jumping_penalty: f32,
    /// Where a grenade is thrown from, from the camera: forward, up and
    /// left, in world units.
    pub grenade_origin: [f32; 3],
}

impl Default for PlayerPhysics {
    /// The game's old player speeds (its code's override), for maps
    /// without globals.
    fn default() -> Self {
        Self {
            walking_speed: 0.512,
            run_forward: 2.25,
            run_backward: 2.0,
            run_sideways: 2.0,
            run_acceleration: 0.32 * 30.0,
            sneak_forward: 0.512,
            sneak_backward: 0.512,
            sneak_sideways: 0.512,
            sneak_acceleration: 0.32 * 30.0,
            airborne_acceleration: 0.32 * 30.0,
            stun_movement_penalty: 0.0,
            stun_turning_penalty: 0.0,
            stun_jumping_penalty: 0.0,
            grenade_origin: [0.0; 3],
        }
    }
}

/// What a fall does to a biped. Velocities are in world units a second.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct FallingDamage {
    /// A landing slower than this does no harm.
    pub minimum_damage_velocity: f32,
    /// A landing at this does the falling damage in full.
    pub maximum_damage_velocity: f32,
    /// A landing faster than this does the deadly damage.
    pub maximum_falling_velocity: f32,
    /// The damage a harmful landing does, scaled.
    pub falling_damage: Option<TagRef>,
    /// The damage a deadly landing does.
    pub maximum_distance_damage: Option<TagRef>,
}

impl Level {
    /// A shader, by its tag.
    pub fn shader(&self, tag: &TagRef) -> Option<&Shader> {
        self.shaders.iter().find(|shader| shader.tag == *tag)
    }

    /// An object definition, by its tag.
    pub fn object(&self, tag: &TagRef) -> Option<&ObjectDefinition> {
        self.objects.iter().find(|object| object.tag == *tag)
    }

    /// A model's index in [`Level::models`], by its tag.
    pub fn model_index(&self, tag: &TagRef) -> Option<usize> {
        self.models.iter().position(|model| model.tag == *tag)
    }

    /// An animation graph, by its tag.
    pub fn animation_graph(&self, tag: &TagRef) -> Option<&AnimationGraph> {
        self.animation_graphs.iter().find(|graph| graph.tag == *tag)
    }

    /// A damage effect, by its tag.
    pub fn damage_effect(&self, tag: &TagRef) -> Option<&DamageEffect> {
        self.damage_effects.iter().find(|effect| effect.tag == *tag)
    }

    /// An effect, by its tag.
    pub fn effect(&self, tag: &TagRef) -> Option<&Effect> {
        self.effects.iter().find(|effect| effect.tag == *tag)
    }

    /// A collision model, by its tag.
    pub fn collision_model(&self, tag: &TagRef) -> Option<&CollisionModel> {
        self.collision_models.iter().find(|model| model.tag == *tag)
    }

    /// A bitmap, by its tag.
    pub fn bitmap(&self, tag: &TagRef) -> Option<&Bitmap> {
        self.bitmaps.iter().find(|bitmap| bitmap.tag == *tag)
    }
}

/// A sky: the model drawn behind the level, and the fog of the air
/// outside and inside.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Sky {
    /// Its path.
    pub tag: TagRef,
    /// Its model, among the level's models.
    pub model: Option<TagRef>,
    /// The fog outside.
    pub outdoor_fog: AtmosphericFog,
    /// The fog inside.
    pub indoor_fog: AtmosphericFog,
}

/// A sky's fog: from its start distance to its opaque distance, the view
/// fades to its color, as far as its maximum density.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct AtmosphericFog {
    /// Its color.
    pub color: [f32; 3],
    /// How much of the view it hides, at most (0: all of it).
    pub maximum_density: f32,
    /// Where it starts, in world units.
    pub start_distance: f32,
    /// Where it is densest, in world units (0: there is none).
    pub opaque_distance: f32,
}

/// A scenario, as far as its structure goes.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Scenario {
    /// Its path.
    pub path: String,
    /// Its structure BSPs.
    pub structure_bsps: Vec<TagRef>,
    /// Where players start.
    pub player_starts: Vec<PlayerStart>,
    /// The objects it places, kind by kind in the scenario's order.
    pub placements: Vec<ObjectPlacement>,
    /// Its multiplayer flags: the flags' stands, the hills, the race's
    /// checkpoints, the teleporters.
    pub netgame_flags: Vec<NetgameFlag>,
    /// The weapons and equipment a multiplayer game places, and places
    /// again when they are taken.
    pub netgame_equipment: Vec<NetgameEquipment>,
    /// Its scripts.
    pub scripts: Scripts,
    /// What its scripts name and its AI places.
    pub campaign: Campaign,
}

/// The game types something in a scenario is for, as Halo gives them: four
/// entries, each a game type, all of them (12), all but CTF (13) or all but
/// CTF and Race (14).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct GameTypes(pub [i16; 4]);

/// A multiplayer game type, numbered as Halo numbers its game engines.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum GameType {
    /// Capture the Flag.
    CaptureTheFlag = 1,
    /// Slayer.
    Slayer = 2,
    /// Oddball.
    Oddball = 3,
    /// King of the Hill.
    King = 4,
    /// Race.
    Race = 5,
}

impl GameTypes {
    /// Whether it is for `game` (Halo's `match_game_type`).
    pub fn matches(&self, game: GameType) -> bool {
        let game = game as i16;
        self.0.iter().any(|&entry| match entry {
            12 => true,
            13 => game != GameType::CaptureTheFlag as i16,
            14 => game != GameType::CaptureTheFlag as i16 && game != GameType::Race as i16,
            entry => entry == game,
        })
    }
}

/// What a multiplayer flag marks.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub enum NetgameFlagKind {
    /// A team's flag stand.
    CtfFlag,
    /// Where a team's vehicle stands in CTF.
    CtfVehicle,
    /// Where the ball starts.
    Oddball,
    /// A checkpoint of the race's track.
    RaceTrack,
    /// Where a vehicle stands in Race.
    RaceVehicle,
    /// A bank (unused by the stock game types).
    VegasBank,
    /// A teleporter's entrance.
    TeleporterSource,
    /// A teleporter's exit.
    TeleporterTarget,
    /// A corner of a hill.
    Hill,
}

/// A multiplayer flag, as the scenario gives it.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct NetgameFlag {
    /// What it marks.
    pub kind: NetgameFlagKind,
    /// Its team (for a teleporter, its channel; for the race, its order).
    pub team: u16,
    /// Where it is.
    pub position: [f32; 3],
    /// The way it faces: radians left from +x.
    pub facing: f32,
}

/// An item a multiplayer game places, chosen from its item collection
/// each time.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct NetgameEquipment {
    /// The game types it is for.
    pub game_types: GameTypes,
    /// The team it is for (0: either).
    pub team: u16,
    /// Whether it floats where it is placed rather than falling.
    pub levitate: bool,
    /// Seconds before it is placed again once taken (none: its collection's,
    /// or the game's default).
    pub spawn_seconds: Option<u16>,
    /// Where it is.
    pub position: [f32; 3],
    /// The way it faces: radians left from +x.
    pub facing: f32,
    /// The items it is chosen from, each with its weight.
    pub items: Vec<ItemChoice>,
}

/// One of the items of an item collection.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ItemChoice {
    /// The weapon or equipment.
    pub item: TagRef,
    /// How likely it is, against the others' weights.
    pub weight: f32,
}

/// An object the scenario places.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ObjectPlacement {
    /// Its kind: the scenario's block it is in.
    pub kind: ObjectKind,
    /// Its object tag, from the kind's palette.
    pub tag: TagRef,
    /// Its name, from the scenario's object names.
    pub name: Option<String>,
    /// When it is placed: bit 0, not automatically; bits 1 to 3, not on
    /// easy, normal or hard.
    pub placement_flags: u16,
    /// Where it is (an item that neither levitates nor is made at rest
    /// raised 5 cm, as the game places it).
    pub position: [f32; 3],
    /// Its yaw, pitch and roll, in radians.
    pub rotation: [f32; 3],
    /// The structure BSPs it is in, a bit each.
    pub structure_bsps: u16,
    /// Its colors and regions' permutations (scenery, units and weapons).
    pub permutation: Option<PlacementPermutation>,
    /// Its device groups and flags (machines and controls).
    pub device: Option<PlacementDevice>,
}

impl ObjectPlacement {
    /// Whether it is placed when the level starts.
    pub fn is_automatic(&self) -> bool {
        self.placement_flags & 1 == 0
    }
}

/// A placed device's groups and flags (`scenario_device_datum`, and a
/// machine's or a control's own flags after it).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct PlacementDevice {
    /// The device group its power follows, or `-1`.
    pub power_group: i16,
    /// The device group its position follows, or `-1`.
    pub position_group: i16,
    /// Its flags: bit 0, starts open; 1, starts off; 2, changes only once;
    /// 3, its position reversed; 4, not usable from any side.
    pub flags: u32,
    /// A machine's flags (bit 0, does not operate by itself; 1, one sided;
    /// 2, never appears locked; 3, opened by a melee attack), or a
    /// control's.
    pub kind_flags: u32,
}

/// A placed object's appearance.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct PlacementPermutation {
    /// Its four change colors, each 0xAARRGGBB (zero: the object's own).
    pub change_colors: [u32; 4],
    /// The permutation of each of its model's regions.
    pub region_permutations: [u8; 8],
}

/// A place a player starts, as the scenario gives it.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct PlayerStart {
    /// Where its feet are.
    pub position: [f32; 3],
    /// The way it faces: radians left from +x.
    pub facing: f32,
    /// The team it is for (multiplayer).
    pub team: u16,
    /// The structure BSP it is in (none: any).
    pub structure_bsp: Option<u16>,
    /// The game types it is for.
    pub game_types: GameTypes,
}

/// A structure BSP: the level's fixed geometry, its lightmaps and its
/// collision.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct StructureBsp {
    /// Its path.
    pub path: String,
    /// The bitmap its lightmaps' pages are in.
    pub lightmap_bitmap: Option<TagRef>,
    /// Its bounds: x, y and z, each least and greatest.
    pub world_bounds: [[f32; 2]; 3],
    /// Its collision.
    pub collision: CollisionBsp,
    /// What each collision surface is made of.
    pub collision_materials: Vec<CollisionMaterial>,
    /// Triangles, as indices into their material's vertices.
    pub surfaces: Vec<[u16; 3]>,
    /// Its geometry, grouped by lightmap page.
    pub lightmaps: Vec<Lightmap>,
}

/// What a collision surface is made of.
#[derive(Clone, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct CollisionMaterial {
    /// The shader drawn there.
    pub shader: Option<TagRef>,
    /// The physics material type (what a footstep or a bullet does).
    pub material_type: i16,
}

/// The geometry that shares one lightmap page.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Lightmap {
    /// The page's index in the BSP's lightmap bitmap, if it has one.
    pub bitmap_index: Option<u16>,
    /// Its materials.
    pub materials: Vec<Material>,
}

/// The triangles of a lightmap page that share a shader.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Material {
    /// The shader.
    pub shader: Option<TagRef>,
    /// Its first triangle, in the BSP's surfaces.
    pub first_surface: u32,
    /// How many triangles, from the first.
    pub surface_count: u32,
    /// Its vertices, which its triangles index.
    pub vertices: Vec<EnvironmentVertex>,
    /// Each vertex's lightmap coordinates and light direction, if it is
    /// lightmapped (as many as the vertices).
    pub lightmap_vertices: Vec<LightmapVertex>,
}

/// A vertex of the structure.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct EnvironmentVertex {
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
}

/// A vertex's light: where on the lightmap page it is, and the direction
/// most of its light comes from.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LightmapVertex {
    /// The incident light's direction.
    pub incident_direction: [f32; 3],
    /// Where on the page it is.
    pub texcoord: [f32; 2],
}

/// A collision BSP, in the game's encoding, its indices checked.
///
/// A 3D node's child is a node if it is not negative; otherwise its low
/// 31 bits are a leaf, all ones for none. A 2D node's child, and a 2D
/// reference's node, are a node if not negative, otherwise a surface in
/// the low 31 bits. A plane designator's high bit flips the plane.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct CollisionBsp {
    /// The 3D nodes.
    pub bsp3d_nodes: Vec<Bsp3dNode>,
    /// The planes: normal, then distance.
    pub planes: Vec<[f32; 4]>,
    /// The leaves.
    pub leaves: Vec<CollisionLeaf>,
    /// The 2D references.
    pub bsp2d_references: Vec<Bsp2dReference>,
    /// The 2D nodes.
    pub bsp2d_nodes: Vec<Bsp2dNode>,
    /// The surfaces.
    pub surfaces: Vec<CollisionSurface>,
    /// The edges.
    pub edges: Vec<CollisionEdge>,
    /// The vertices.
    pub vertices: Vec<CollisionVertex>,
}

/// How deep [`CollisionBsp::leaf_at`] goes before it takes a point to be
/// in solid (`MAXIMUM_BSP3D_TRAVERSAL_DEPTH`).
pub const MAXIMUM_BSP3D_DEPTH: usize = 128;

impl CollisionBsp {
    /// The leaf `point` is in, or none if it is in solid (or the tree is
    /// broken: a node that is not one, or one too deep), as the game tests
    /// it (`bsp3d_test_point`): a point on a plane is in front of it.
    pub fn leaf_at(&self, point: [f32; 3]) -> Option<u32> {
        let mut node = 0i32;
        for _ in 0..MAXIMUM_BSP3D_DEPTH {
            let this = self.bsp3d_nodes.get(usize::try_from(node).ok()?)?;
            let index = usize::try_from(this.plane & i32::MAX).ok()?;
            let [i, j, k, d] = *self.planes.get(index)?;
            let mut distance = i * point[0] + j * point[1] + k * point[2] - d;
            if this.plane < 0 {
                distance = -distance;
            }
            node = this.children[usize::from(distance >= 0.0)];
            if node < 0 {
                return (node != -1).then_some((node & i32::MAX) as u32);
            }
        }
        None
    }
}

/// A 3D node of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct Bsp3dNode {
    /// Its plane designator.
    pub plane: i32,
    /// Behind and in front of the plane.
    pub children: [i32; 2],
}

/// A leaf of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct CollisionLeaf {
    /// Its flags.
    pub flags: u16,
    /// How many 2D references it has.
    pub bsp2d_reference_count: u16,
    /// Its first 2D reference.
    pub first_bsp2d_reference: u32,
}

/// A 2D reference of a collision BSP: a plane and the 2D tree in it.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct Bsp2dReference {
    /// Its plane designator.
    pub plane: i32,
    /// Its tree's root.
    pub node: i32,
}

/// A 2D node of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Bsp2dNode {
    /// Its line: normal, then distance.
    pub plane: [f32; 3],
    /// Behind and in front of the line.
    pub children: [i32; 2],
}

/// A surface of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct CollisionSurface {
    /// Its plane designator.
    pub plane: i32,
    /// One of its edges.
    pub first_edge: u32,
    /// Its flags (two-sided, invisible, climbable, breakable).
    pub flags: u8,
    /// Its breakable surface, if it is breakable.
    pub breakable_surface: u8,
    /// Its material, in the BSP's collision materials.
    pub material: Option<u16>,
}

/// An edge of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct CollisionEdge {
    /// Its start and end.
    pub vertices: [u32; 2],
    /// The next edge of each of its surfaces.
    pub edges: [u32; 2],
    /// The surfaces on each side, if any.
    pub surfaces: [Option<u32>; 2],
}

/// A vertex of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CollisionVertex {
    /// Where it is.
    pub point: [f32; 3],
    /// One of its edges.
    pub first_edge: u32,
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A plane z = 1 with the leaf above it; below is solid.
    fn floor() -> CollisionBsp {
        CollisionBsp {
            bsp3d_nodes: vec![Bsp3dNode {
                plane: 0,
                children: [-1, i32::MIN | 3],
            }],
            planes: vec![[0.0, 0.0, 1.0, 1.0]],
            ..CollisionBsp::default()
        }
    }

    #[test]
    fn a_point_is_in_the_leaf_in_front_of_its_planes() {
        let floor = floor();
        assert_eq!(floor.leaf_at([5.0, -2.0, 1.5]), Some(3));
        // (on the plane is in front)
        assert_eq!(floor.leaf_at([0.0, 0.0, 1.0]), Some(3));
        assert_eq!(floor.leaf_at([0.0, 0.0, 0.5]), None);
        // a flipped designator turns the plane round
        let ceiling = CollisionBsp {
            bsp3d_nodes: vec![Bsp3dNode {
                plane: i32::MIN,
                ..floor.bsp3d_nodes[0]
            }],
            ..floor
        };
        assert_eq!(ceiling.leaf_at([0.0, 0.0, 0.5]), Some(3));
        assert_eq!(ceiling.leaf_at([0.0, 0.0, 1.5]), None);
    }

    #[test]
    fn a_broken_tree_is_solid() {
        // no nodes, a plane it does not have, a node that is not one, and
        // a loop deeper than the game goes
        assert_eq!(CollisionBsp::default().leaf_at([0.0; 3]), None);
        let mut floor = floor();
        floor.planes.clear();
        assert_eq!(floor.leaf_at([0.0, 0.0, 2.0]), None);
        let mut floor = self::floor();
        floor.bsp3d_nodes[0].children[1] = 7;
        assert_eq!(floor.leaf_at([0.0, 0.0, 2.0]), None);
        floor.bsp3d_nodes[0].children[1] = 0;
        assert_eq!(floor.leaf_at([0.0, 0.0, 2.0]), None);
    }
}
