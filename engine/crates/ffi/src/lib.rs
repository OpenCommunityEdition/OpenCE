//! The contract between the engine and the Swift app.
//!
//! Calls (load a map, start a game, input, settings) go through UniFFI;
//! the per-frame data goes through the `#[repr(C)]` types of [`snapshot`],
//! in the header cbindgen writes, which Swift reads in place.
// (the UniFFI scaffolding and the snapshot's reader are unsafe code; this
// crate and physics are the only ones allowed it)
#![allow(unsafe_code)]

pub mod bake;
pub mod camera;
pub mod campaign;
pub mod control;
mod debug_scene;
pub mod decals;
pub mod devtools;
mod dynamic_decals;
mod effects;
pub mod hud;
pub mod lights;
pub mod loading;
pub mod network;
mod particles;
pub mod render_level;
pub mod render_model;
pub mod render_shadow;
pub mod render_sky;
pub mod render_transparent;
pub mod script;
pub mod settings;
pub mod snapshot;
mod stations;
pub mod ui;
mod ui_host;

use std::collections::HashMap;
use std::path::Path;
use std::sync::{Arc, Mutex, MutexGuard};

use opence_content::ObjectKind;
use opence_sim::animation::{self, AnimationPlayer};
use opence_sim::clock::Clock;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{Appearance, Difficulty, Object, ObjectHandle, Permutation};
use opence_sim::player::View;
use opence_sim::units::Radians;
use opence_sim::{FreeCamera, Simulation};
use render_level::{RenderDraw, RenderLevel, RenderMaterial, RenderTexture, VERTEX_STRIDE};
use render_model::{RenderModel, MODEL_VERTEX_STRIDE};
use render_sky::RenderSky;
use snapshot::{CameraSnapshot, NodeSnapshot, ObjectSnapshot, SnapshotBuffers, SnapshotWriter};

uniffi::setup_scaffolding!();

/// The engine's version, as the app shows it.
#[uniffi::export]
pub fn engine_version() -> String {
    env!("CARGO_PKG_VERSION").to_owned()
}

/// What a call to the engine can fail with.
#[derive(Debug, PartialEq, Eq, thiserror::Error, uniffi::Error)]
pub enum EngineError {
    /// A call that needs a world came before one was started.
    #[error("no world is running")]
    NoWorld,
    /// A map could not be loaded.
    #[error("{message}")]
    Load {
        /// Why.
        message: String,
    },
    /// A network game could not be started.
    #[error("{message}")]
    Network {
        /// Why.
        message: String,
    },
    /// The settings could not be read or saved.
    #[error("{message}")]
    Settings {
        /// Why.
        message: String,
    },
    /// The level has no structure BSP of that index.
    #[error("the level has no BSP {index}")]
    NoBsp {
        /// The index asked for.
        index: u32,
    },
}

/// Where the free camera is put and which way it faces (Z up; a yaw of
/// zero looks along +X; radians).
#[derive(Clone, Copy, Debug, Default, PartialEq, uniffi::Record)]
pub struct CameraPlacement {
    /// Where, in world units.
    pub x: f32,
    /// Where, in world units.
    pub y: f32,
    /// Where, in world units.
    pub z: f32,
    /// The turn to the left from +X.
    pub yaw: f32,
    /// The turn up from level.
    pub pitch: f32,
}

/// A level's structure BSP, ready for the GPU: loaded once, through the
/// cache folder, and read by the app into its buffers and textures.
#[derive(uniffi::Object)]
pub struct LevelGeometry {
    /// The map's name (its file's, without .map).
    name: String,
    level: RenderLevel,
    /// The structure BSP drawn, and its scenery placed.
    bsp: usize,
    content: Arc<opence_content::Level>,
}

#[uniffi::export]
impl LevelGeometry {
    /// The BSP `bsp` of the map at `map`, its pack kept in `cache_folder`.
    #[uniffi::constructor]
    pub fn load(map: String, cache_folder: String, bsp: u32) -> Result<Arc<Self>, EngineError> {
        let path = Path::new(&map);
        if !path.exists()
            && path.file_stem().and_then(|stem| stem.to_str()) == Some(opence_content::devmap::NAME)
        {
            if bsp != 0 {
                return Err(EngineError::NoBsp { index: bsp });
            }
            let folder = path
                .parent()
                .map_or_else(String::new, |folder| folder.to_string_lossy().into_owned());
            return Self::devmap_gathered(folder, cache_folder);
        }
        let loaded = opence_import::load_level(Path::new(&map), Path::new(&cache_folder)).map_err(
            |error| EngineError::Load {
                message: error.to_string(),
            },
        )?;
        let level = RenderLevel::build(&loaded.level, bsp as usize)
            .ok_or(EngineError::NoBsp { index: bsp })?;
        let name = Path::new(&map)
            .file_stem()
            .map_or_else(String::new, |stem| stem.to_string_lossy().into_owned());
        Ok(Arc::new(Self {
            name,
            level,
            bsp: bsp as usize,
            content: Arc::new(loaded.level),
        }))
    }

    /// The test map, `devmap`: the engine's plane, made over the level of
    /// the map at `donor` (its pack kept in `cache_folder`) for its
    /// definitions and its sky, or, without one, with nothing on it under
    /// a generated sky.
    #[uniffi::constructor]
    pub fn devmap(donor: Option<String>, cache_folder: String) -> Result<Arc<Self>, EngineError> {
        let donor = donor
            .map(|map| {
                opence_import::load_level(Path::new(&map), Path::new(&cache_folder))
                    .map(|loaded| loaded.level)
                    .map_err(|error| EngineError::Load {
                        message: error.to_string(),
                    })
            })
            .transpose()?;
        let level = opence_content::devmap::level(donor.as_ref());
        let render = RenderLevel::build(&level, 0).ok_or(EngineError::NoBsp { index: 0 })?;
        Ok(Arc::new(Self {
            name: opence_content::devmap::NAME.into(),
            level: render,
            bsp: 0,
            content: Arc::new(level),
        }))
    }

    /// The test map with its galleries: every object of every map in
    /// `maps_folder` (in name order; a map that does not load is left out)
    /// stood in rows by kind, on a plane made large enough to hold them.
    /// `map devmap` and `--map devmap` load it.
    #[uniffi::constructor]
    pub fn devmap_gathered(
        maps_folder: String,
        cache_folder: String,
    ) -> Result<Arc<Self>, EngineError> {
        let folder = if maps_folder.is_empty() {
            Path::new(".")
        } else {
            Path::new(&maps_folder)
        };
        let mut maps: Vec<_> = std::fs::read_dir(folder)
            .map_err(|error| EngineError::Load {
                message: format!("{}: {error}", folder.display()),
            })?
            .filter_map(|entry| entry.ok().map(|entry| entry.path()))
            .filter(|path| path.extension().is_some_and(|extension| extension == "map"))
            .collect();
        maps.sort();
        let levels: Vec<_> = maps
            .iter()
            .filter_map(
                |map| match opence_import::load_level(map, Path::new(&cache_folder)) {
                    Ok(loaded) => Some(loaded.level),
                    Err(error) => {
                        let text = format!("{} left out: {error}", map.display());
                        opence_dev::logbook::record(
                            opence_dev::logbook::Level::Warn,
                            "devmap",
                            &text,
                        );
                        None
                    }
                },
            )
            .collect();
        let (level, galleries) =
            opence_content::devmap::gallery_level(&levels).ok_or_else(|| EngineError::Load {
                message: format!("no map in {} loaded", folder.display()),
            })?;
        if !galleries.left_out.is_empty() {
            let text = format!(
                "{} objects too wide for a gallery",
                galleries.left_out.len()
            );
            opence_dev::logbook::record(opence_dev::logbook::Level::Info, "devmap", &text);
        }
        let render = RenderLevel::build(&level, 0).ok_or(EngineError::NoBsp { index: 0 })?;
        Ok(Arc::new(Self {
            name: opence_content::devmap::NAME.into(),
            level: render,
            bsp: 0,
            content: Arc::new(level),
        }))
    }

    /// The map's name (its file's, without .map).
    pub fn name(&self) -> String {
        self.name.clone()
    }

    /// The bytes of one vertex.
    pub fn vertex_stride(&self) -> u32 {
        VERTEX_STRIDE as u32
    }

    /// The vertices: position, normal, texture and lightmap coordinates,
    /// tangent, binormal and incident light direction, as little-endian
    /// floats.
    pub fn vertices(&self) -> Vec<u8> {
        self.level.vertices.clone()
    }

    /// The indices, as little-endian 32-bit integers.
    pub fn indices(&self) -> Vec<u8> {
        self.level
            .indices
            .iter()
            .flat_map(|index| index.to_le_bytes())
            .collect()
    }

    /// The draws: opaque and alpha-tested, then blended.
    pub fn draws(&self) -> Vec<RenderDraw> {
        self.level.draws.clone()
    }

    /// The materials the draws use.
    pub fn materials(&self) -> Vec<RenderMaterial> {
        self.level.materials.clone()
    }

    /// Every material's animation at `seconds` of game time, twelve floats
    /// each: the self-illumination's primary colour and plasma value, its
    /// secondary colour and a zero, then the maps' offset in u and v and
    /// two zeros.
    pub fn material_animation(&self, seconds: f32) -> Vec<f32> {
        self.level.animation(seconds)
    }

    /// The textures the materials and lightmaps use.
    pub fn textures(&self) -> Vec<RenderTexture> {
        self.level
            .textures
            .iter()
            .map(|(texture, _)| texture.clone())
            .collect()
    }

    /// A texture's level's bytes (none for one it does not have).
    pub fn texture_level(&self, texture: u32, level: u32) -> Vec<u8> {
        self.level
            .textures
            .get(texture as usize)
            .and_then(|(_, levels)| levels.get(level as usize))
            .cloned()
            .unwrap_or_default()
    }

    /// The bytes of one model vertex.
    pub fn model_vertex_stride(&self) -> u32 {
        MODEL_VERTEX_STRIDE as u32
    }

    /// The object models' vertices: position, normal and texture
    /// coordinates as little-endian floats, then the two nodes each moves
    /// with (16-bit) and their weights (floats).
    pub fn model_vertices(&self) -> Vec<u8> {
        self.level.model_vertices.clone()
    }

    /// The object models' indices, as little-endian 32-bit integers.
    pub fn model_indices(&self) -> Vec<u8> {
        self.level
            .model_indices
            .iter()
            .flat_map(|index| index.to_le_bytes())
            .collect()
    }

    /// The object models, which the snapshot's objects name by index.
    pub fn models(&self) -> Vec<RenderModel> {
        self.level.models.clone()
    }

    /// The level's sky: its model and its fog.
    pub fn sky(&self) -> Option<RenderSky> {
        self.level.sky.clone()
    }

    /// Each of the level's lights' gel, a cube texture, by the index the
    /// snapshot's lights give (`u32::MAX`: none).
    pub fn light_gels(&self) -> Vec<u32> {
        self.level.light_gels.clone()
    }

    /// The textures the camera's sprites draw, by `CameraSprite::image`.
    pub fn camera_textures(&self) -> Vec<u32> {
        self.level.camera_textures.clone()
    }

    /// The decals the BSP lays when it is loaded, layer by layer.
    pub fn decals(&self) -> Vec<decals::RenderDecal> {
        self.level.decals.clone()
    }

    /// The textures the snapshot's particles draw, by their picture.
    pub fn particle_textures(&self) -> Vec<u32> {
        self.level.particle_textures.clone()
    }

    /// The BSP's detail objects, as the snapshot's particles are laid out
    /// (`ParticleSnapshot`'s bytes, one after another).
    pub fn detail_objects(&self) -> Vec<u8> {
        self.level
            .detail_objects
            .iter()
            .flat_map(|row| {
                let words: [[u32; 4]; 6] = [
                    row.position.map(f32::to_bits),
                    row.axis.map(f32::to_bits),
                    row.color.map(f32::to_bits),
                    row.uv.map(f32::to_bits),
                    row.shape.map(f32::to_bits),
                    row.modes,
                ];
                words.into_iter().flatten().flat_map(u32::to_ne_bytes)
            })
            .collect()
    }

    /// The textures the snapshot's decals draw, by their picture.
    pub fn decal_textures(&self) -> Vec<u32> {
        self.level.decal_textures.clone()
    }

    /// The exposure's limits in the BSP, least and greatest, in stops
    /// (`adapt_exposure`).
    pub fn exposure_limits(&self) -> Vec<f32> {
        self.content
            .structure_bsps
            .get(self.bsp)
            .map_or(camera::INTERIOR_EXPOSURE, camera::exposure_limits)
            .to_vec()
    }

    /// The BSP's bounds: least and greatest x, then y, then z.
    pub fn bounds(&self) -> Vec<f32> {
        self.level.bounds.iter().flatten().copied().collect()
    }

    /// Where a player would first see the BSP from: its first player
    /// start, at a standing player's eyes, facing its way; or, if it has
    /// none, a view over it from its bounds.
    pub fn start_view(&self) -> CameraPlacement {
        match self.level.player_start {
            Some((position, facing)) => player_view(position, facing),
            None => start_view(self.level.bounds),
        }
    }
}

/// The permutation each region of `model` is drawn with: the placement's,
/// if the region has it, else its first.
fn region_permutations(model: &opence_content::Model, placed: [u8; 8]) -> [u8; 8] {
    let mut regions = [0u8; 8];
    for (at, region) in model.regions.iter().take(8).enumerate() {
        if usize::from(placed[at]) < region.permutations.len() {
            regions[at] = placed[at];
        }
    }
    regions
}

/// An object's four change colours, as `0xFFRRGGBB`, as the original's
/// `object_choose_random_change_colors` chooses them: each starts as its
/// placement's (white where that is zero), and one its definition gives
/// takes the first of its permutations whose weight reaches a value
/// hashed from the object's position, at a point between the
/// permutation's bounds hashed from its height. The colours the
/// original then scales or darkens by the object's functions are left as
/// chosen: objects have no functions yet.
fn change_colors(
    definitions: &[opence_content::ChangeColor],
    placed: [u32; 4],
    position: [f32; 3],
) -> [u32; 4] {
    let [x, y, z] = position;
    let mut colors = [0u32; 4];
    for (index, color) in colors.iter_mut().enumerate() {
        let mut rgb = if placed[index] == 0 {
            [1.0; 3]
        } else {
            unpack_rgb(placed[index])
        };
        if let Some(definition) = definitions.get(index) {
            let cc = index as f32;
            let weight = libm::fmodf(
                libm::fabsf(x * 315.893_13 + y * 587.129_46 + z * 744.124_15 + cc * 431.128_94),
                1.0,
            );
            if let Some(permutation) = definition
                .permutations
                .iter()
                .find(|permutation| weight <= permutation.weight)
            {
                let along = libm::fmodf(libm::fabsf(y) + cc * 0.712_11, 1.0);
                for (channel, value) in rgb.iter_mut().enumerate() {
                    *value = permutation.lower[channel]
                        + (permutation.upper[channel] - permutation.lower[channel]) * along;
                }
            }
        }
        *color = pack_rgb(rgb);
    }
    colors
}

/// `0x__RRGGBB` as red, green and blue from 0 to 1.
fn unpack_rgb(color: u32) -> [f32; 3] {
    let channel = |shift: u32| f32::from(((color >> shift) & 0xFF) as u8) / 255.0;
    [channel(16), channel(8), channel(0)]
}

/// Red, green and blue, clamped to 0 to 1, as `0xFFRRGGBB`.
fn pack_rgb(rgb: [f32; 3]) -> u32 {
    let channel = |value: f32| {
        let value = if value.is_nan() {
            0.0
        } else {
            value.clamp(0.0, 1.0)
        };
        libm::roundf(value * 255.0) as u32
    };
    0xFF00_0000 | channel(rgb[0]) << 16 | channel(rgb[1]) << 8 | channel(rgb[2])
}

/// How far a standing player's eyes are above their feet, in world units
/// (the stock cyborg's camera).
const EYE_HEIGHT: f32 = 0.62;

/// The view from a player start at `position` facing `facing`.
fn player_view(position: [f32; 3], facing: f32) -> CameraPlacement {
    let [x, y, z] = position;
    CameraPlacement {
        x,
        y,
        z: z + EYE_HEIGHT,
        yaw: facing,
        pitch: 0.0,
    }
}

/// A view over a BSP of `bounds`: high over one corner, looking at its
/// middle, a little above its floor.
fn start_view(bounds: [[f32; 2]; 3]) -> CameraPlacement {
    let at = |axis: usize, part: f32| {
        let [least, greatest] = bounds[axis];
        least + (greatest - least) * part
    };
    let (x, y, z) = (at(0, 0.2), at(1, 0.2), at(2, 0.7));
    let (dx, dy, dz) = (at(0, 0.5) - x, at(1, 0.5) - y, at(2, 0.3) - z);
    // (libm's, as the simulation's: the camera's start is part of its state)
    let across = libm::sqrtf(dx * dx + dy * dy);
    let finite = |value: f32| if value.is_finite() { value } else { 0.0 };
    CameraPlacement {
        x: finite(x),
        y: finite(y),
        z: finite(z),
        yaw: finite(libm::atan2f(dy, dx)),
        pitch: finite(libm::atan2f(dz, across)),
    }
}

/// The player's input as the app reads it from the keyboard and mouse.
#[derive(Clone, Copy, Debug, Default, PartialEq, uniffi::Record)]
pub struct InputState {
    /// The buttons held (`Buttons` of the simulation).
    pub buttons: u32,
    /// Forward (127) or back (-127).
    pub forward: i8,
    /// Right (127) or left (-127).
    pub right: i8,
    /// Up (127) or down (-127), for the free camera.
    pub up: i8,
    /// The turn to the left since the last input, in radians.
    pub yaw: f32,
    /// The turn up since the last input, in radians.
    pub pitch: f32,
}

impl InputState {
    /// The input as the simulation takes it: unknown buttons dropped, the
    /// axes kept within ±127 and the turns finite and within a half turn.
    fn tick_input(self) -> TickInput {
        let axis = |value: i8| value.max(-127);
        let angle = |value: f32| {
            if value.is_finite() {
                Radians(value.clamp(-std::f32::consts::PI, std::f32::consts::PI))
            } else {
                Radians(0.0)
            }
        };
        TickInput {
            buttons: Buttons(self.buttons & Buttons::ALL.0),
            forward: axis(self.forward),
            right: axis(self.right),
            up: axis(self.up),
            yaw: angle(self.yaw),
            pitch: angle(self.pitch),
        }
    }
}

/// The bits of [`InputState::buttons`], as the simulation's `Buttons`.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Record)]
pub struct InputButtons {
    /// Jump.
    pub jump: u32,
    /// Crouch.
    pub crouch: u32,
    /// Fire.
    pub fire: u32,
    /// Sprint (the free camera's faster speed).
    pub sprint: u32,
    /// Reload.
    pub reload: u32,
    /// Change weapons.
    pub swap: u32,
    /// The action (pick up a weapon).
    pub action: u32,
    /// Melee.
    pub melee: u32,
    /// Throw a grenade.
    pub grenade: u32,
    /// Zoom.
    pub zoom: u32,
    /// Change grenades.
    pub switch_grenade: u32,
    /// The flashlight.
    pub flashlight: u32,
}

/// The buttons' bits, so that the app need not repeat them.
#[uniffi::export]
pub fn input_buttons() -> InputButtons {
    InputButtons {
        jump: Buttons::JUMP.0,
        crouch: Buttons::CROUCH.0,
        fire: Buttons::FIRE.0,
        sprint: Buttons::SPRINT.0,
        reload: Buttons::RELOAD.0,
        swap: Buttons::SWAP.0,
        action: Buttons::ACTION.0,
        melee: Buttons::MELEE.0,
        grenade: Buttons::GRENADE.0,
        zoom: Buttons::ZOOM.0,
        switch_grenade: Buttons::SWITCH_GRENADE.0,
        flashlight: Buttons::FLASHLIGHT.0,
    }
}

pub(crate) struct Running {
    simulation: Simulation,
    previous_camera: snapshot::Transform,
    clock: Clock,
    /// The level it was started with, if any.
    level: Option<Arc<opence_content::Level>>,
    /// The network game it plays, if any.
    network: Option<network::Network>,
    /// The players that have been dressed (the network adds them).
    dressed: usize,
    /// The mission it plays, if any.
    campaign: Option<campaign::Campaign>,
    /// The lights objects carry and effects make.
    lights: lights::Lights,
    /// The frame's lights (kept for their memory).
    lit: Vec<snapshot::LightSnapshot>,
    /// The decals projectiles and detonations lay.
    decals: dynamic_decals::DynamicDecals,
    /// The frame's laid decals (kept for their memory).
    marks: Vec<snapshot::DecalSnapshot>,
    /// The effects shots, impacts, detonations and objects make.
    effects: effects::LevelEffects,
    /// The frame's running effects (kept for their memory).
    running_effects: Vec<snapshot::EffectSnapshot>,
    /// The particles, contrails and weather.
    particles: particles::LevelParticles,
    /// The frame's particles (kept for their memory).
    particle_rows: Vec<snapshot::ParticleSnapshot>,
}

/// The models and animation graphs objects are drawn and posed with,
/// shared by every object of a tag.
#[derive(Default)]
struct Wardrobe {
    models: HashMap<usize, Arc<opence_content::Model>>,
    graphs: HashMap<opence_content::TagRef, Option<Arc<opence_content::AnimationGraph>>>,
}

impl Wardrobe {
    /// Gives an object its model and its standing animation, if its tag
    /// has them.
    fn dress(
        &mut self,
        simulation: &mut Simulation,
        level: &opence_content::Level,
        handle: ObjectHandle,
    ) {
        let Some(object) = simulation.component::<Object>(handle) else {
            return;
        };
        let Some(definition) = level.object(&object.tag) else {
            return;
        };
        let model = definition
            .model
            .as_ref()
            .and_then(|model| level.model_index(model));
        if let Some(model) = model {
            let permutation = simulation
                .component::<Permutation>(handle)
                .map(|permutation| permutation.0.region_permutations)
                .unwrap_or_default();
            let placed = simulation
                .component::<Permutation>(handle)
                .map(|permutation| permutation.0.change_colors)
                .unwrap_or_default();
            let position = simulation
                .transform(handle)
                .map_or([0.0; 3], |transform| transform.position.to_array());
            simulation.set_appearance(
                handle,
                Appearance {
                    model: model as u32,
                    radius_bits: definition.bounding_radius.to_bits(),
                    regions: region_permutations(&level.models[model], permutation),
                    change_colors: change_colors(&definition.change_colors, placed, position),
                },
            );
        }
        let Some(graph) = definition.animation_graph.as_ref() else {
            return;
        };
        let graph = self
            .graphs
            .entry(graph.clone())
            .or_insert_with(|| level.animation_graph(graph).cloned().map(Arc::new))
            .clone();
        if let Some(graph) = graph {
            let unit = matches!(object.kind, ObjectKind::Biped | ObjectKind::Vehicle);
            let animation = animation::standing_animation(&graph, unit);
            let model = model.map(|model| {
                self.models
                    .entry(model)
                    .or_insert_with(|| Arc::new(level.models[model].clone()))
                    .clone()
            });
            simulation.animate(handle, AnimationPlayer::new(graph, model, animation));
        }
    }
}

struct State {
    running: Option<Running>,
    writer: SnapshotWriter,
    input: InputState,
    shell: devtools::Shell,
}

/// The engine: one per app.
#[derive(uniffi::Object)]
pub struct Engine {
    // (always locked before the state)
    registry: Mutex<opence_dev::Registry<State>>,
    state: Mutex<State>,
    snapshots: Arc<SnapshotBuffers>,
    control: Arc<control::Control>,
}

impl Drop for Engine {
    fn drop(&mut self) {
        self.control.stop();
    }
}

#[uniffi::export]
impl Engine {
    /// An engine with no world.
    #[uniffi::constructor]
    pub fn new() -> Arc<Self> {
        let (buffers, writer) = SnapshotBuffers::new();
        // (the log book is the process's: a second engine finds it set)
        opence_dev::logbook::install();
        Arc::new(Self {
            registry: Mutex::new(devtools::registry()),
            state: Mutex::new(State {
                running: None,
                writer,
                input: InputState::default(),
                shell: devtools::Shell::default(),
            }),
            snapshots: Arc::new(buffers),
            control: Arc::default(),
        })
    }

    /// Starts a world with nothing in it, and publishes its first snapshot.
    pub fn start_empty_world(&self) {
        let mut state = self.lock();
        let simulation = Simulation::empty();
        let previous_camera = transform(&simulation.camera().transform);
        state.running = Some(Running {
            simulation,
            previous_camera,
            clock: Clock::default(),
            level: None,
            network: None,
            dressed: 0,
            campaign: None,
            lights: lights::Lights::default(),
            lit: Vec::new(),
            decals: dynamic_decals::DynamicDecals::default(),
            marks: Vec::new(),
            effects: effects::LevelEffects::default(),
            running_effects: Vec::new(),
            particles: particles::LevelParticles::default(),
            particle_rows: Vec::new(),
        });
        publish(&mut state, &self.snapshots, 0.0);
    }

    /// Starts a world of `geometry`'s level: every object its scenario
    /// places in its structure BSP (at any `difficulty`, 0 easy to 3
    /// legendary, as the game reads none of the placements' flags for
    /// them), each drawn with its
    /// model and standing still in its idle animation; publishes its first
    /// snapshot. Returns the objects made.
    pub fn start_level(
        &self,
        geometry: Arc<LevelGeometry>,
        difficulty: u8,
    ) -> Result<u32, EngineError> {
        let level = &geometry.content;
        // (the game places a level's objects at every difficulty alike)
        let _ = difficulty;
        let mut simulation = Simulation::empty();
        let handles = simulation
            .spawn_scenario(level, geometry.bsp)
            .map_err(|_| EngineError::Load {
                message: "the scenario places more objects than the world holds".into(),
            })?;
        let mut wardrobe = Wardrobe::default();
        for &handle in &handles {
            wardrobe.dress(&mut simulation, level, handle);
        }
        let count = handles.len() as u32;
        let mut state = self.lock();
        let previous_camera = transform(&simulation.camera().transform);
        state.running = Some(Running {
            simulation,
            previous_camera,
            clock: Clock::default(),
            level: Some(geometry.content.clone()),
            network: None,
            dressed: 0,
            campaign: None,
            lights: lights::Lights::new(&geometry.content),
            lit: Vec::new(),
            decals: dynamic_decals::DynamicDecals::new(&geometry.content),
            marks: Vec::new(),
            effects: effects::LevelEffects::new(&geometry.content),
            running_effects: Vec::new(),
            particles: particles::LevelParticles::new(&geometry.content),
            particle_rows: Vec::new(),
        });
        publish(&mut state, &self.snapshots, 0.0);
        Ok(count)
    }

    /// Makes the player at the level's player start `start` (the first is
    /// 0), drawn and standing, and sees from its eyes; a player there was
    /// before is removed. Fails without a level, or for a start the
    /// scenario does not have.
    pub fn spawn_player(&self, start: u32) -> Result<(), EngineError> {
        let mut state = self.lock();
        let running = state.running.as_mut().ok_or(EngineError::NoWorld)?;
        let level = running.level.clone().ok_or(EngineError::NoWorld)?;
        let handle = running
            .simulation
            .spawn_player(&level, start as usize)
            .map_err(|error| EngineError::Load {
                message: error.to_string(),
            })?;
        Wardrobe::default().dress(&mut running.simulation, &level, handle);
        running.previous_camera = transform(&running.simulation.camera().transform);
        publish(&mut state, &self.snapshots, 0.0);
        Ok(())
    }

    /// Sees the player from behind (true) or from its eyes; false without
    /// a player.
    pub fn set_third_person(&self, third_person: bool) -> bool {
        let mut state = self.lock();
        let Some(running) = state.running.as_mut() else {
            return false;
        };
        let view = if third_person {
            View::ThirdPerson
        } else {
            View::FirstPerson
        };
        if !running.simulation.set_player_view(view) {
            return false;
        }
        running.previous_camera = transform(&running.simulation.camera().transform);
        publish(&mut state, &self.snapshots, 0.0);
        true
    }

    /// Whether there is a player, and the camera is its.
    pub fn playing(&self) -> bool {
        self.lock()
            .running
            .as_ref()
            .is_some_and(|running| running.simulation.player().is_some())
    }

    /// Puts the free camera at `placement`, as if it had always been there
    /// (no interpolation from where it was).
    pub fn place_camera(&self, placement: CameraPlacement) -> Result<(), EngineError> {
        let mut state = self.lock();
        let running = state.running.as_mut().ok_or(EngineError::NoWorld)?;
        let finite = |value: f32| if value.is_finite() { value } else { 0.0 };
        running.simulation.place_free_camera(FreeCamera {
            position: opence_sim::Vec3::new(
                finite(placement.x),
                finite(placement.y),
                finite(placement.z),
            ),
            yaw: Radians(finite(placement.yaw)),
            pitch: Radians(finite(placement.pitch)),
        });
        running.previous_camera = transform(&running.simulation.camera().transform);
        publish(&mut state, &self.snapshots, 0.0);
        Ok(())
    }

    /// Sets the input the next ticks run with. The turns are used once,
    /// by the next tick; the axes and buttons stay until the next call.
    pub fn set_input(&self, input: InputState) {
        let mut state = self.lock();
        let turn = |earlier: f32, later: f32| {
            if (earlier + later).is_finite() {
                earlier + later
            } else {
                earlier
            }
        };
        let yaw = turn(state.input.yaw, input.yaw);
        let pitch = turn(state.input.pitch, input.pitch);
        state.input = InputState {
            yaw,
            pitch,
            ..input
        };
    }

    /// Runs one tick and publishes its snapshot.
    pub fn step(&self) -> Result<u64, EngineError> {
        let mut state = self.lock();
        run_ticks(&mut state, 1)?;
        let tick = state
            .running
            .as_ref()
            .map_or(0, |running| running.simulation.tick().0);
        publish(&mut state, &self.snapshots, 1.0);
        Ok(tick)
    }

    /// Opens a profiling scope `name` on the calling thread, which
    /// [`Engine::profile_end`] closes: false (and nothing kept) unless
    /// `stat frame` or a trace is on.
    pub fn profile_begin(&self, name: String) -> bool {
        opence_dev::profile::profiler().begin(&name)
    }

    /// Closes the calling thread's last profiling scope.
    pub fn profile_end(&self) {
        opence_dev::profile::profiler().end();
    }

    /// Adds a displayed frame's time: runs the ticks it covers and
    /// publishes a snapshot with the interpolation factor between the last
    /// two. Returns the ticks run.
    pub fn advance(&self, frame_seconds: f64) -> Result<u32, EngineError> {
        let mut registry = self.lock_registry();
        let mut state = self.lock();
        // (a frame is its advance and the app's drawing after it)
        end_profiled_frame(&mut state);
        opence_dev::profile_scope!("advance");
        let scale = if state.shell.paused {
            0.0
        } else {
            state.shell.time_scale
        };
        let mut frame = None;
        if let Some(running) = state.running.as_mut() {
            let advanced = running.clock.advance(frame_seconds * scale);
            run_ticks(&mut state, advanced.ticks)?;
            frame = Some(advanced);
        }
        // (a script in the window runs a line a frame, with or without a world)
        script::poll_window(&mut registry, &mut state, &self.control);
        let frame = frame.ok_or(EngineError::NoWorld)?;
        publish(&mut state, &self.snapshots, frame.alpha);
        Ok(frame.ticks)
    }

    /// The HUD to draw over a window `width` by `height` pixels: nothing
    /// without a level and a player.
    pub fn hud(&self, width: u32, height: u32) -> Vec<hud::HudQuad> {
        let state = self.lock();
        let Some(running) = &state.running else {
            return Vec::new();
        };
        let (Some(level), Some(readout)) = (&running.level, running.simulation.hud()) else {
            return Vec::new();
        };
        let seconds =
            running.simulation.tick().0 as f32 / opence_sim::clock::TICKS_PER_SECOND as f32;
        hud::hud_quads(level, &readout, width, height, seconds)
    }

    /// What the camera sees of lights from `camera` this frame: the lens
    /// flares (when `flares`) and the light volumes and glows (when
    /// `widgets`), as sprites, with the disks the flares are tested with.
    pub fn camera_sprites(
        &self,
        camera: camera::SpriteCamera,
        flares: bool,
        widgets: bool,
    ) -> camera::CameraSprites {
        let state = self.lock();
        let Some(running) = &state.running else {
            return camera::CameraSprites::default();
        };
        let Some(level) = &running.level else {
            return camera::CameraSprites::default();
        };
        let seconds =
            running.simulation.tick().0 as f32 / opence_sim::clock::TICKS_PER_SECOND as f32;
        running.lights.camera.sprites(
            level,
            &running.lights.sources,
            &running.lit,
            &camera,
            seconds,
            flares,
            widgets,
        )
    }

    /// Runs a line of the developer shell: commands split by `;`, each a
    /// command, a variable, or HaloScript for the running mission; then
    /// publishes the world as the line left it.
    pub fn run_command(&self, line: String) -> devtools::ShellOutput {
        let mut registry = self.lock_registry();
        let mut state = self.lock();
        let output = devtools::run(&mut registry, &mut state, &line);
        for (path, width, height) in std::mem::take(&mut state.shell.frames) {
            self.control.queue_unclaimed_frame(path, width, height);
        }
        publish(&mut state, &self.snapshots, 1.0);
        output
    }

    /// Runs the command bound to `key`, if one is.
    pub fn press_key(&self, key: String) -> devtools::ShellOutput {
        let line = self.lock_registry().binding(&key).map(str::to_owned);
        match line {
            Some(line) => self.run_command(line),
            None => devtools::ShellOutput {
                ok: true,
                json: "null".into(),
                ..Default::default()
            },
        }
    }

    /// The watch line's values (empty when nothing is watched).
    pub fn watch_values(&self) -> Vec<devtools::WatchValue> {
        let registry = self.lock_registry();
        let mut state = self.lock();
        devtools::watch_values(&registry, &mut state)
    }

    /// The ways `line` could go on: whole lines, in order.
    pub fn complete_command(&self, line: String) -> Vec<String> {
        self.lock_registry().complete(&line)
    }

    /// The shell's variables.
    pub fn shell_variables(&self) -> Vec<devtools::ShellVariable> {
        devtools::variables(&self.lock_registry())
    }

    /// A shell variable as a boolean (false if it is not one).
    pub fn shell_flag(&self, name: String) -> bool {
        self.lock_registry().get_bool(&name)
    }

    /// A shell variable's value as text (empty if there is none).
    pub fn shell_value(&self, name: String) -> String {
        self.lock_registry()
            .get(&name)
            .map(ToString::to_string)
            .unwrap_or_default()
    }

    /// Sets a shell variable from text read as its kind.
    pub fn set_shell_variable(&self, name: String, value: String) -> Result<(), EngineError> {
        let mut registry = self.lock_registry();
        let mut state = self.lock();
        registry
            .set_text(&mut state, &name, &value)
            .map_err(|error| EngineError::Load {
                message: error.to_string(),
            })
    }

    /// The saved shell variables not at their default, as text, for the
    /// settings file.
    pub fn saved_shell_variables(&self) -> HashMap<String, String> {
        self.lock_registry().saved().into_iter().collect()
    }

    /// Sets the saved shell variables from the settings file; returns the
    /// names it refused.
    pub fn load_shell_variables(&self, saved: HashMap<String, String>) -> Vec<String> {
        self.lock_registry()
            .load_saved(&saved.into_iter().collect())
    }

    /// Where `map` finds a map by its name.
    pub fn set_maps_folder(&self, folder: String) {
        self.lock().shell.maps_folder = Some(folder.into());
    }

    /// Whether the shell paused the world.
    pub fn paused(&self) -> bool {
        self.lock().shell.paused
    }

    /// The address to pass to `opence_snapshot_acquire`, valid while this
    /// engine is alive.
    pub fn snapshot_buffers(&self) -> u64 {
        Arc::as_ptr(&self.snapshots) as u64
    }
}

impl Engine {
    pub(crate) fn lock_registry(&self) -> MutexGuard<'_, opence_dev::Registry<State>> {
        self.registry
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
    }

    pub(crate) fn lock(&self) -> MutexGuard<'_, State> {
        // (a panic aborts, so the lock is never poisoned by one)
        self.state
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
    }
}

/// The difficulty of the app's number for it: 0 easy to 3 legendary.
fn difficulty(level: u8) -> Difficulty {
    match level {
        0 => Difficulty::Easy,
        1 => Difficulty::Normal,
        2 => Difficulty::Hard,
        _ => Difficulty::Legendary,
    }
}

fn transform(from: &opence_sim::Transform) -> snapshot::Transform {
    snapshot::Transform {
        position: snapshot::Vec3 {
            x: from.position.x,
            y: from.position.y,
            z: from.position.z,
        },
        scale: from.scale,
        rotation: snapshot::Quat {
            x: from.rotation.x,
            y: from.rotation.y,
            z: from.rotation.z,
            w: from.rotation.w,
        },
    }
}

fn node_matrix(from: &animation::NodeMatrix) -> snapshot::NodeMatrix {
    let vector = |[x, y, z]: [f32; 3]| snapshot::Vec3 { x, y, z };
    snapshot::NodeMatrix {
        forward: vector(from.forward),
        left: vector(from.left),
        up: vector(from.up),
        position: vector(from.position),
        scale: from.scale,
    }
}

/// Ends the profiler's frame, writing the trace it finished, if it did.
fn end_profiled_frame(state: &mut State) {
    let Some(spans) = opence_dev::profile::profiler().frame_end() else {
        return;
    };
    let path = state.shell.trace.take().unwrap_or_default();
    let (level, text) = match devtools::write_trace(&path, &spans) {
        Ok(text) => (opence_dev::logbook::Level::Info, text),
        Err(error) => (opence_dev::logbook::Level::Error, error.to_string()),
    };
    opence_dev::logbook::record(level, "profile", &text);
}

pub(crate) fn run_ticks(state: &mut State, ticks: u32) -> Result<(), EngineError> {
    opence_dev::profile_scope!("ticks");
    let State {
        running,
        input,
        shell,
        ..
    } = state;
    let running = running.as_mut().ok_or(EngineError::NoWorld)?;
    for _ in 0..ticks {
        opence_dev::profile_scope!("tick");
        running.previous_camera = transform(&campaign::camera(running).transform);
        match (&mut running.network, &running.level, &mut running.campaign) {
            (Some(network), Some(level), _) => {
                network.step(&mut running.simulation, level, input.tick_input());
            }
            (_, _, Some(campaign)) => campaign.step(&mut running.simulation, input.tick_input()),
            _ => {
                let tick = shell
                    .station
                    .as_mut()
                    .and_then(|run| run.input(&running.simulation))
                    .unwrap_or_else(|| input.tick_input());
                running.simulation.step(tick);
                if let (Some(run), Some(level)) = (&mut shell.station, &running.level) {
                    run.after_tick(&mut running.simulation, level, &mut shell.draw);
                }
            }
        }
        if let Some(level) = &running.level {
            let bsp = running
                .campaign
                .as_ref()
                .map_or(shell.bsp, campaign::Campaign::bsp);
            running
                .effects
                .tick(level, &mut running.simulation, &mut running.decals, bsp);
            let fired = running.effects.fired();
            running.lights.tick(level, fired);
            running.decals.tick(level, &running.simulation, fired);
            running.particles.tick(
                level,
                &mut running.simulation,
                running.effects.running(),
                bsp,
            );
        }
        // (the mouse's turn is used once)
        input.yaw = 0.0;
        input.pitch = 0.0;
        shell.draw.advance(running.simulation.tick().0);
    }
    if let (Some(_), Some(level)) = (&running.network, &running.level) {
        running.dressed = network::dress_players(&mut running.simulation, level, running.dressed);
    }
    if let Some(campaign) = &mut running.campaign {
        campaign.dress(&mut running.simulation);
    }
    Ok(())
}

pub(crate) fn publish(state: &mut State, buffers: &SnapshotBuffers, alpha: f32) {
    opence_dev::profile_scope!("publish");
    let State {
        running,
        writer,
        shell,
        ..
    } = state;
    let Some(running) = running else { return };
    let camera = campaign::camera(running);
    let bsp = running
        .campaign
        .as_ref()
        .map_or(shell.bsp, campaign::Campaign::bsp);
    if shell.scene.any_world_layer() {
        debug_scene::fill(
            &mut shell.scene,
            running.level.as_deref(),
            bsp,
            &mut running.simulation,
            camera.transform.position,
        );
    }
    // (a cinematic's camera is not the player's: no weapon in its hands)
    let cinematic = running
        .campaign
        .as_ref()
        .is_some_and(|campaign| campaign.camera().is_some());
    let tick = running.simulation.tick().0;
    let previous = running.previous_camera;
    let mut lit = std::mem::take(&mut running.lit);
    lit.clear();
    if let Some(level) = &running.level {
        running
            .lights
            .gather(level, &mut running.simulation, &camera.transform, &mut lit);
        let lights = &mut running.lights;
        lights
            .camera
            .gather(level, &mut running.simulation, bsp, &mut lights.sources);
    }
    let mut marks = std::mem::take(&mut running.marks);
    running
        .decals
        .gather(|handle| running.simulation.transform(handle), &mut marks);
    let mut running_effects = std::mem::take(&mut running.running_effects);
    running.effects.gather(&mut running_effects);
    let mut particle_rows = std::mem::take(&mut running.particle_rows);
    particle_rows.clear();
    running.particles.camera = camera.transform.position;
    if let Some(level) = &running.level {
        running
            .particles
            .gather(level, camera.transform.position, &mut particle_rows);
    }
    let simulation = &mut running.simulation;
    writer.publish(buffers, tick, alpha, |frame| {
        frame.camera(CameraSnapshot {
            previous,
            current: transform(&camera.transform),
            vertical_fov_radians: camera.vertical_fov.0,
            reserved: [0; 3],
        });
        let mut nodes = Vec::new();
        simulation.visit_drawn(|object| {
            nodes.clear();
            if let Some(matrices) = object.nodes {
                nodes.extend(matrices.current.iter().enumerate().map(|(at, current)| {
                    NodeSnapshot {
                        previous: node_matrix(matrices.previous.get(at).unwrap_or(current)),
                        current: node_matrix(current),
                    }
                }));
            }
            // (a full snapshot draws the objects that fit)
            let _ = frame.push_animated(
                ObjectSnapshot {
                    id: object.id.0,
                    model: object.appearance.map_or(u32::MAX, |look| look.model),
                    previous: transform(object.previous),
                    current: transform(object.current),
                    bounding_radius: object
                        .appearance
                        .map_or(0.0, |look| f32::from_bits(look.radius_bits)),
                    regions: object.appearance.map_or([0; 8], |look| look.regions),
                    change_colors: object
                        .appearance
                        .map_or([0xFFFF_FFFF; 4], |look| look.change_colors),
                    ..Default::default()
                },
                &nodes,
            );
        });
        // (the first-person weapon and hands, from the camera: their ids
        // from the top, apart from the world's)
        let first_person = if cinematic {
            &[][..]
        } else {
            simulation.first_person().models()
        };
        for (at, model) in first_person.iter().enumerate() {
            nodes.clear();
            nodes.extend(
                model
                    .current
                    .iter()
                    .zip(&model.previous)
                    .map(|(current, previous)| NodeSnapshot {
                        previous: node_matrix(previous),
                        current: node_matrix(current),
                    }),
            );
            let _ = frame.push_animated(
                ObjectSnapshot {
                    id: u32::MAX - u32::try_from(at).unwrap_or(0),
                    model: u32::try_from(model.model).unwrap_or(u32::MAX),
                    flags: snapshot::OBJECT_FIRST_PERSON,
                    // (white: the original takes the weapon's and the
                    // unit's, which are white until players have colours)
                    change_colors: [0xFFFF_FFFF; 4],
                    previous,
                    current: transform(&camera.transform),
                    bounding_radius: 1.0,
                    ..Default::default()
                },
                &nodes,
            );
        }
        frame.lights(&lit);
        frame.decals(&marks);
        frame.effects(&running_effects);
        frame.particles(&particle_rows);
        frame.debug(&shell.draw);
        frame.debug(&shell.scene);
    });
    running.lit = lit;
    running.marks = marks;
    running.running_effects = running_effects;
    running.particle_rows = particle_rows;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn change_colors_are_chosen_from_the_position_as_the_originals() {
        use opence_content::{ChangeColor, ColorPermutation};
        let color = |permutations: Vec<ColorPermutation>| ChangeColor {
            darken_by: 0,
            scale_by: 0,
            scale_flags: 0,
            lower: [0.0; 3],
            upper: [0.0; 3],
            permutations,
        };
        let permutation = |weight, upper| ColorPermutation {
            weight,
            lower: [0.0; 3],
            upper,
        };
        let definitions = [
            color(vec![
                permutation(0.4, [1.0, 0.0, 0.0]),
                permutation(1.0, [0.0, 1.0, 0.0]),
            ]),
            color(vec![
                permutation(0.4, [1.0, 0.0, 0.0]),
                permutation(1.0, [0.0, 0.0, 2.0]),
            ]),
        ];
        let colors = change_colors(&definitions, [0, 0, 0, 0x0010_2030], [1.0, 2.25, 3.0]);
        // the first: weight 0.307 takes the first permutation, a quarter
        // of the way along (|y| = 2.25)
        assert_eq!(colors[0], 0xFF40_0000);
        // the second: weight 0.436 takes the second, 0.962 along, clamped
        assert_eq!(colors[1], 0xFF00_00FF);
        // the others have no definition: the placement's, white for none
        assert_eq!(colors[2], 0xFFFF_FFFF);
        assert_eq!(colors[3], 0xFF10_2030);
        let unreached = [color(vec![permutation(0.1, [1.0, 0.0, 0.0])])];
        let colors = change_colors(&unreached, [0x0000_8000, 0, 0, 0], [1.0, 2.25, 3.0]);
        assert_eq!(colors[0], 0xFF00_8000);
        assert_eq!(pack_rgb([f32::NAN, -1.0, 0.5]), 0xFF00_0080);
    }

    #[test]
    fn the_version_is_the_crates() {
        assert_eq!(engine_version(), "0.1.0");
    }

    #[test]
    fn stepping_without_a_world_fails() {
        assert_eq!(Engine::new().step(), Err(EngineError::NoWorld));
    }

    #[test]
    fn an_empty_world_publishes_its_ticks() {
        let engine = Engine::new();
        engine.start_empty_world();
        assert_eq!(engine.step(), Ok(1));
        assert_eq!(engine.step(), Ok(2));
        let buffers = engine.snapshot_buffers() as *const SnapshotBuffers;
        // SAFETY: the engine is alive and this is the only reader.
        let frame = unsafe { &*snapshot::opence_snapshot_acquire(buffers) };
        assert_eq!(frame.tick, 2);
        assert_eq!(frame.object_count, 0);
        assert_eq!(frame.camera.current.scale, 1.0);
        assert!(frame.camera.vertical_fov_radians > 1.0);
    }

    fn acquire(engine: &Engine) -> &FrameSnapshotRef {
        // SAFETY: the engine is alive and each test is its only reader.
        unsafe {
            &*snapshot::opence_snapshot_acquire(engine.snapshot_buffers() as *const SnapshotBuffers)
        }
    }
    type FrameSnapshotRef = snapshot::FrameSnapshot;

    #[test]
    fn the_debug_shapes_reach_the_snapshot_and_expire() {
        let engine = Engine::new();
        engine.start_empty_world();
        assert!(engine.run_command("debug_line 0 0 0 1 2 3 2".into()).ok);
        let frame = acquire(&engine);
        assert_eq!(frame.debug_line_count, 1);
        // SAFETY: the frame holds a line at its lines' address.
        let line = unsafe { *frame.debug_lines };
        assert_eq!((line.to.x, line.to.y, line.to.z), (1.0, 2.0, 3.0));
        for _ in 0..2 {
            assert!(engine.step().is_ok());
        }
        assert_eq!(acquire(&engine).debug_line_count, 0);

        assert!(engine.run_command("debug_sphere 0 0 0 1".into()).ok);
        assert!(acquire(&engine).debug_line_count > 1);
        assert!(!engine.run_command("debug_sphere 0 0 0 -1".into()).ok);
        assert!(!engine.run_command("debug_line 0 0 0 1 nan 3".into()).ok);
        assert!(engine.run_command("debug_text 1 2 3 hello there".into()).ok);
        let frame = acquire(&engine);
        assert_eq!(frame.debug_label_count, 1);
        // SAFETY: the frame holds a label at its labels' address.
        let label = unsafe { *frame.debug_labels };
        assert!(label.text.starts_with(b"hello there\0"));
        assert!(engine.run_command("debug_box 1 1 1 0 0 0 5".into()).ok);
        assert!(engine.run_command("debug_clear".into()).ok);
        let frame = acquire(&engine);
        assert_eq!((frame.debug_line_count, frame.debug_label_count), (0, 0));

        // (a layer switched off draws nothing, and its variable says so)
        assert!(engine.run_command("set debug.draw.general false".into()).ok);
        assert!(engine.run_command("debug_line 0 0 0 1 1 1".into()).ok);
        assert_eq!(acquire(&engine).debug_line_count, 0);
        assert!(engine.run_command("set debug.draw.markers true".into()).ok);
        assert!(
            engine
                .run_command("set debug.draw.collision true".into())
                .ok
        );
        assert!(engine.step().is_ok());
        assert_eq!(acquire(&engine).debug_line_count, 0);
    }

    #[test]
    fn advancing_runs_whole_ticks_and_publishes_alpha() {
        let engine = Engine::new();
        assert_eq!(engine.advance(0.1), Err(EngineError::NoWorld));
        engine.start_empty_world();
        // (a 144 Hz display: a tick every 2.4 frames)
        let mut ticks = 0;
        for _ in 0..144 {
            ticks += engine.advance(1.0 / 144.0).unwrap_or(0);
            let frame = acquire(&engine);
            assert!((0.0..1.0).contains(&frame.alpha), "{}", frame.alpha);
            assert_eq!(frame.tick, u64::from(ticks));
        }
        assert!((59..=60).contains(&ticks), "{ticks}");
    }

    #[test]
    fn the_mouse_turns_once_and_the_keys_hold() {
        let engine = Engine::new();
        engine.start_empty_world();
        engine.set_input(InputState {
            forward: 127,
            yaw: 0.25,
            ..Default::default()
        });
        for _ in 0..3 {
            let _ = engine.step();
        }
        let frame = acquire(&engine);
        let turned = opence_sim::FreeCamera {
            yaw: Radians(0.25),
            ..Default::default()
        }
        .rotation();
        assert!((frame.camera.current.rotation.z - turned.z).abs() < 1e-6);
        assert!(frame.camera.current.position.x > 0.0 && frame.camera.current.position.y > 0.0);
    }

    #[test]
    fn a_placed_camera_is_published_without_a_streak() {
        let engine = Engine::new();
        let placement = CameraPlacement {
            x: 10.0,
            y: -4.0,
            z: 2.0,
            yaw: 1.0,
            pitch: 0.5,
        };
        assert_eq!(engine.place_camera(placement), Err(EngineError::NoWorld));
        engine.start_empty_world();
        assert_eq!(engine.place_camera(placement), Ok(()));
        let frame = acquire(&engine);
        assert_eq!(frame.camera.current.position.x, 10.0);
        assert_eq!(frame.camera.previous.position.y, -4.0);
        let _ = engine.step();
        let frame = acquire(&engine);
        assert_eq!(frame.camera.current.position.z, 2.0);
    }

    #[test]
    fn the_start_view_looks_into_the_bounds() {
        let view = start_view([[0.0, 100.0], [0.0, 100.0], [0.0, 10.0]]);
        assert_eq!((view.x, view.y, view.z), (20.0, 20.0, 7.0));
        assert!((view.yaw - std::f32::consts::FRAC_PI_4).abs() < 1e-6);
        assert!(view.pitch < 0.0 && view.pitch > -0.2);
        let nonsense = start_view([[f32::NAN, 1.0], [0.0, f32::INFINITY], [0.0, 0.0]]);
        assert!([nonsense.x, nonsense.y, nonsense.yaw, nonsense.pitch]
            .iter()
            .all(|value| value.is_finite()));
    }

    #[test]
    fn a_player_start_is_seen_from_its_eyes() {
        let view = player_view([1.0, 2.0, 3.0], 0.5);
        assert_eq!((view.x, view.y, view.z), (1.0, 2.0, 3.0 + EYE_HEIGHT));
        assert_eq!((view.yaw, view.pitch), (0.5, 0.0));
    }

    #[test]
    fn a_missing_map_is_a_load_error() {
        let result = LevelGeometry::load("/nonexistent/none.map".into(), "/nonexistent".into(), 0);
        assert!(matches!(result, Err(EngineError::Load { .. })));
    }

    #[test]
    fn nonsense_input_is_made_safe() {
        let input = InputState {
            buttons: u32::MAX,
            forward: i8::MIN,
            yaw: f32::NAN,
            pitch: 100.0,
            ..Default::default()
        };
        let tick = input.tick_input();
        assert_eq!(tick.buttons, Buttons::ALL);
        assert_eq!(tick.forward, -127);
        assert_eq!(tick.yaw, Radians(0.0));
        assert_eq!(tick.pitch, Radians(std::f32::consts::PI));
    }

    /// A stock map's first BSP and its objects, or None without game data.
    fn stock(name: &str) -> Option<Arc<LevelGeometry>> {
        let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
        let map = maps.join(format!("{name}.map"));
        if !map.exists() {
            return None;
        }
        let cache = std::env::temp_dir().join("opence-ffi-test-cache");
        let loaded = LevelGeometry::load(map.display().to_string(), cache.display().to_string(), 0);
        assert!(loaded.is_ok(), "{name}: {:?}", loaded.as_ref().err());
        loaded.ok()
    }

    /// A level's models are whole: every node has its default pose, every
    /// draw's triangles are in the model buffers, and every permutation's
    /// geometries are the model's.
    fn check_models(name: &str, geometry: &LevelGeometry) {
        let level = &geometry.level;
        assert!(!level.models.is_empty(), "{name} has no models");
        let vertex_count = (level.model_vertices.len() / MODEL_VERTEX_STRIDE) as u32;
        assert_eq!(level.model_vertices.len() % MODEL_VERTEX_STRIDE, 0);
        assert!(level
            .model_indices
            .iter()
            .all(|&index| index < vertex_count));
        for (number, model) in level.models.iter().enumerate() {
            let label = &geometry.content.models[number].tag.path;
            assert_eq!(model.default_inverses.len(), model.node_count as usize * 13);
            // (each node's scale is one, and its axes are of length one)
            for node in model.default_inverses.as_chunks::<13>().0 {
                assert!((node[0] - 1.0).abs() < 0.01, "{name} {label}: {node:?}");
                let forward = (node[1] * node[1] + node[2] * node[2] + node[3] * node[3]).sqrt();
                assert!((forward - 1.0).abs() < 0.01, "{name} {label}: {node:?}");
            }
            for geometry in &model.geometries {
                for part in &geometry.parts {
                    let end = part.first_index + part.index_count;
                    assert!(end as usize <= level.model_indices.len());
                    assert!((part.material as usize) < level.materials.len());
                }
            }
            let drawn = model.regions.iter().all(|region| {
                region.permutations.iter().all(|permutation| {
                    permutation
                        .geometries
                        .iter()
                        .flatten()
                        .all(|&index| (index as usize) < model.geometries.len())
                })
            });
            assert!(drawn, "{name} {label}");
        }
    }

    /// A level started in the engine publishes its objects with their
    /// models, and its units with their nodes.
    fn check_started(name: &str, geometry: &Arc<LevelGeometry>) {
        let engine = Engine::new();
        let made = engine.start_level(geometry.clone(), 3);
        assert!(matches!(made, Ok(count) if count > 0), "{name}: {made:?}");
        let _ = engine.step();
        let frame = acquire(&engine);
        // SAFETY: the snapshot's objects and nodes are valid while it is.
        let objects =
            unsafe { std::slice::from_raw_parts(frame.objects, frame.object_count as usize) };
        let modelled = objects
            .iter()
            .filter(|object| (object.model as usize) < geometry.level.models.len())
            .count();
        assert!(modelled > 0, "{name}: no object has a model");
        // (a radius of zero is the tag's: its model's lowest cutoff is
        // zero too, and it is drawn at its highest level)
        assert!(
            objects
                .iter()
                .all(|object| object.bounding_radius.is_finite() && object.bounding_radius >= 0.0),
            "{name}"
        );
        // (a campaign level places bipeds; a multiplayer one may place
        // nothing with more than its one node)
        let campaign = name.len() == 3;
        let animated = objects
            .iter()
            .filter(|object| object.node_count > 1)
            .count();
        assert!(!campaign || animated > 0, "{name}: nothing is animated");
    }

    #[test]
    fn blood_gulch_and_the_pillar_of_autumn_draw_their_objects() {
        for name in ["bloodgulch", "a10"] {
            let Some(geometry) = stock(name) else {
                return; // (no game data here)
            };
            check_models(name, &geometry);
            check_started(name, &geometry);
        }
    }

    #[test]
    fn the_player_is_spawned_and_played() {
        let engine = Engine::new();
        assert_eq!(engine.spawn_player(0), Err(EngineError::NoWorld));
        engine.start_empty_world();
        assert_eq!(engine.spawn_player(0), Err(EngineError::NoWorld));
        assert!(!engine.set_third_person(true));
        let Some(geometry) = stock("bloodgulch") else {
            return; // (no game data here)
        };
        assert!(engine.start_level(geometry.clone(), 1).is_ok());
        assert!(!engine.playing());
        assert!(matches!(
            engine.spawn_player(u32::MAX),
            Err(EngineError::Load { .. })
        ));
        assert_eq!(engine.spawn_player(0), Ok(()));
        assert!(engine.playing());
        let start = geometry.content.scenario.player_starts[0].position;
        let frame = acquire(&engine);
        let eyes = frame.camera.current.position;
        assert!((eyes.x - start[0]).abs() < 1e-3 && (eyes.y - start[1]).abs() < 1e-3);
        // (seen from its eyes, its own body is not in the snapshot)
        let count = frame.object_count;
        assert!(engine.set_third_person(true));
        assert_eq!(acquire(&engine).object_count, count + 1);
        assert!(engine.set_third_person(false));
        engine.set_input(InputState {
            forward: 127,
            ..Default::default()
        });
        for _ in 0..60 {
            let _ = engine.step();
        }
        let frame = acquire(&engine);
        let moved = libm::hypotf(
            frame.camera.current.position.x - eyes.x,
            frame.camera.current.position.y - eyes.y,
        );
        assert!(moved > 1.0, "{moved}");
    }

    #[test]
    fn the_flashlight_and_the_shots_light_the_frame() {
        let Some(geometry) = stock("bloodgulch") else {
            return; // (no game data here)
        };
        let engine = Engine::new();
        assert!(engine.start_level(geometry, 1).is_ok());
        assert_eq!(engine.spawn_player(0), Ok(()));
        let lights = |engine: &Engine| {
            let frame = acquire(engine);
            // SAFETY: the snapshot holds `light_count` lights.
            unsafe { std::slice::from_raw_parts(frame.lights, frame.light_count as usize) }.to_vec()
        };
        let _ = engine.step();
        let dark = lights(&engine);
        assert!(dark
            .iter()
            .all(|light| light.flags & snapshot::LIGHT_FIRST_PERSON == 0));
        let press = |buttons: u32| {
            engine.set_input(InputState {
                buttons,
                ..Default::default()
            });
            let _ = engine.step();
        };
        press(Buttons::FLASHLIGHT.0);
        press(0);
        for _ in 0..10 {
            let _ = engine.step();
        }
        let lit = lights(&engine);
        let flashlight = lit
            .iter()
            .find(|light| light.flags & snapshot::LIGHT_FIRST_PERSON != 0);
        let Some(flashlight) = flashlight else {
            panic!("no flashlight: {lit:?}");
        };
        let eyes = acquire(&engine).camera.current.position;
        assert!((flashlight.position.z - eyes.z).abs() < 1e-3);
        assert!(flashlight.flags & snapshot::LIGHT_SPOT != 0);
        // (a grenade's explosion lights the frame, and its light fades)
        for _ in 0..60 {
            let _ = engine.step();
        }
        let given = engine
            .lock()
            .running
            .as_mut()
            .is_some_and(|running| running.simulation.give_grenades(0, 1));
        assert!(given);
        press(Buttons::GRENADE.0);
        let mut shone = Vec::new();
        for _ in 0..240 {
            press(0);
            let effects: Vec<f32> = lights(&engine)
                .iter()
                .filter(|light| light.source & (1 << 31) != 0)
                .map(|light| light.color.x + light.color.y + light.color.z)
                .collect();
            if let Some(&brightest) = effects.iter().max_by(|a, b| a.total_cmp(b)) {
                shone.push(brightest);
            }
        }
        assert!(shone.len() > 10, "no explosion lit the frame: {shone:?}");
        assert!(shone.last() < shone.first(), "{shone:?}");
    }

    /// Every stock map (slow: `cargo test --release -- --ignored`).
    #[test]
    #[ignore]
    fn every_stock_map_draws_its_objects() {
        let names = [
            "a10",
            "a30",
            "a50",
            "b30",
            "b40",
            "c10",
            "c20",
            "c40",
            "d20",
            "d40",
            "beavercreek",
            "bloodgulch",
            "boardingaction",
            "carousel",
            "chillout",
            "damnation",
            "hangemhigh",
            "longest",
            "prisoner",
            "putput",
            "ratrace",
            "sidewinder",
            "wizard",
        ];
        for name in names {
            let Some(geometry) = stock(name) else {
                continue;
            };
            check_models(name, &geometry);
            check_started(name, &geometry);
        }
    }

    #[test]
    fn a_mission_plays_its_opening_and_loads_its_save() {
        let engine = Engine::new();
        assert!(engine.campaign_state().is_none());
        assert_eq!(engine.campaign_save(), Err(EngineError::NoWorld));
        let Some(geometry) = stock("a10") else {
            return; // (no game data here)
        };
        assert_eq!(engine.start_campaign(geometry.clone(), 1), Ok(()));
        let mut cinematic = false;
        for _ in 0..20 * opence_sim::clock::TICKS_PER_SECOND {
            let _ = engine.step();
            cinematic |= engine.campaign_state().is_some_and(|state| state.cinematic);
        }
        assert!(cinematic, "a10's opening cinematic never played");
        // (every object the scripts made is drawn with its model or none)
        assert!(acquire(&engine).object_count > 0);
        let Ok(saved) = engine.campaign_save() else {
            panic!("no save");
        };
        assert_eq!(
            engine.load_campaign(geometry.clone(), saved.clone()),
            Ok(())
        );
        assert!(engine.campaign_state().is_some());
        assert!(matches!(
            engine.load_campaign(geometry, saved[..saved.len() - 1].to_vec()),
            Err(EngineError::Load { .. })
        ));
    }
}
