//! What the camera sees of lights, as sprites for the renderer's camera
//! pass: the lens flares at the BSP's markers, at the dynamic lights and
//! at the sky's lights (`rasterizer_lens_flares_draw`), and the widgets
//! objects carry at their markers, light volumes and glows. Each flare
//! also gives a disk the renderer tests against the depth, whose share
//! seen scales its sprites.

use std::collections::BTreeMap;

use opence_content::{
    Glow, LensFlare, LensFlareReflection, Level, LightVolume, ObjectDefinition, StructureBsp,
    TagRef,
};
use opence_sim::animation::NodeMatrix;
use opence_sim::seats::default_pose;
use opence_sim::Simulation;

use crate::lights::marker_frames;
use crate::snapshot::LightSnapshot;

/// Its radius grows with the share of its flare seen.
pub const SPRITE_SCALED_BY_OCCLUSION: u32 = 1 << 0;
/// It is hidden by what is in front of it (a widget's sprite; a flare's
/// are hidden by its occlusion test).
pub const SPRITE_DEPTH_TESTED: u32 = 1 << 1;

/// The most sprites a frame draws.
pub const MAX_SPRITES: usize = 4096;
/// The most flares a frame tests.
pub const MAX_OCCLUDERS: usize = 512;
/// The most sprites one light volume or glow is drawn with.
const MAX_WIDGET_SPRITES: usize = 64;
/// How far ahead of the camera the sky's flares are (the original's
/// `1023.875`, just inside its far plane).
const SKY_DISTANCE: f32 = 1023.875;
/// Nearer than this a flare is not drawn.
const NEAREST: f32 = 0.0625;

/// The camera the sprites face: its position and its axes (Z up).
#[derive(Clone, Debug, Default, PartialEq, uniffi::Record)]
pub struct SpriteCamera {
    /// Where it is (x, y, z).
    pub position: Vec<f32>,
    /// The way it looks.
    pub forward: Vec<f32>,
    /// Its up.
    pub up: Vec<f32>,
}

/// A sprite facing the camera, drawn added to the frame.
#[derive(Clone, Debug, Default, PartialEq, uniffi::Record)]
pub struct CameraSprite {
    /// Its centre in the world.
    pub x: f32,
    /// (y)
    pub y: f32,
    /// (z)
    pub z: f32,
    /// Its half width, in world units.
    pub width: f32,
    /// Its half height.
    pub height: f32,
    /// Its turn on the screen, in radians, anticlockwise.
    pub rotation: f32,
    /// Its colour, linear, times its brightness.
    pub red: f32,
    /// (green)
    pub green: f32,
    /// (blue)
    pub blue: f32,
    /// Its picture, an index into `RenderLevel::camera_textures`
    /// (`u32::MAX`: a soft disk).
    pub image: u32,
    /// The occluder whose share seen it is multiplied by (`u32::MAX`:
    /// none).
    pub occluder: u32,
    /// `SPRITE_SCALED_BY_OCCLUSION` and the rest.
    pub flags: u32,
}

/// A disk the renderer tests against the depth: the share of it not
/// hidden scales its flare.
#[derive(Clone, Debug, Default, PartialEq, uniffi::Record)]
pub struct FlareOccluder {
    /// Its centre.
    pub x: f32,
    /// (y)
    pub y: f32,
    /// (z)
    pub z: f32,
    /// Its radius, in world units.
    pub radius: f32,
}

/// A frame's sprites and the disks their flares are tested with.
#[derive(Clone, Debug, Default, PartialEq, uniffi::Record)]
pub struct CameraSprites {
    /// The sprites.
    pub sprites: Vec<CameraSprite>,
    /// The disks.
    pub occluders: Vec<FlareOccluder>,
}

/// A bitmap image the sprites draw.
pub(crate) type Image = (TagRef, u16);

/// A widget an object definition carries at a marker.
#[derive(Clone, Debug, PartialEq)]
enum Widget {
    /// An index into the level's light volumes.
    Volume(usize),
    /// An index into the level's glows.
    Glow(usize),
}

/// What the level's lens flares and widgets draw, looked up once.
#[derive(Clone, Debug, Default)]
pub(crate) struct CameraLevel {
    /// The images, in the order `RenderLevel::camera_textures` gives them.
    images: Vec<Image>,
    /// Each lens flare's reflections' images.
    flare_images: Vec<Vec<u32>>,
    /// Each light volume's image.
    volume_images: Vec<u32>,
    /// Each glow's image.
    glow_images: Vec<u32>,
    /// Each BSP's markers: where, which way, and the flare (an index into
    /// the level's).
    markers: Vec<Vec<Marker>>,
    /// The skies' lights: the way towards each and its flare.
    sky: Vec<([f32; 3], usize)>,
    /// Each of the level's lights' flare.
    light_flares: Vec<Option<usize>>,
    /// The widgets each object definition carries, and its model.
    carried: BTreeMap<TagRef, (Vec<Widget>, Option<usize>)>,
}

/// A lens flare marker: where, which way, and its flare.
type Marker = ([f32; 3], [f32; 3], usize);

/// A widget placed this frame: its marker's frame and the object's
/// functions' values.
#[derive(Clone, Debug, PartialEq)]
struct Placed {
    widget: Widget,
    frame: NodeMatrix,
    functions: [f32; 4],
    /// The object it is on, which turns its glow's particles apart.
    id: u32,
}

/// What the last tick left for the sprites: the BSP and the widgets.
#[derive(Clone, Debug, Default)]
pub(crate) struct CameraSources {
    bsp: usize,
    placed: Vec<Placed>,
}

/// The images the level's lens flares, light volumes and glows draw,
/// each once, in order.
pub(crate) fn camera_images(level: &Level) -> Vec<Image> {
    CameraLevel::new(level).images
}

/// The image a sequence of a bitmap starts with.
fn first_image(level: &Level, bitmap: &TagRef, sequence: i16) -> u16 {
    level
        .bitmap(bitmap)
        .and_then(|bitmap| bitmap.sequences.get(usize::try_from(sequence).ok()?))
        .and_then(|sequence| {
            sequence
                .first_image
                .or_else(|| sequence.sprites.first().map(|sprite| sprite.image))
        })
        .unwrap_or(0)
}

impl CameraLevel {
    pub(crate) fn new(level: &Level) -> Self {
        let mut out = Self::default();
        let mut known = BTreeMap::new();
        let mut image = |bitmap: Option<&TagRef>, index: u16, out: &mut Vec<Image>| {
            let Some(bitmap) = bitmap else {
                return u32::MAX;
            };
            let key = (bitmap.clone(), index);
            *known.entry(key.clone()).or_insert_with(|| {
                out.push(key);
                out.len() as u32 - 1
            })
        };
        for flare in &level.lens_flares {
            let images = flare
                .reflections
                .iter()
                .map(|reflection| match u16::try_from(reflection.bitmap_index) {
                    Ok(index) => image(flare.bitmap.as_ref(), index, &mut out.images),
                    Err(_) => u32::MAX,
                })
                .collect();
            out.flare_images.push(images);
        }
        for volume in &level.light_volumes {
            let index = volume.bitmap.as_ref().map_or(0, |bitmap| {
                first_image(level, bitmap, volume.sequence_index)
            });
            out.volume_images
                .push(image(volume.bitmap.as_ref(), index, &mut out.images));
        }
        for glow in &level.glows {
            out.glow_images
                .push(image(glow.texture.as_ref(), 0, &mut out.images));
        }
        let flare = |tag: Option<&TagRef>| {
            let tag = tag?;
            level.lens_flares.iter().position(|flare| flare.tag == *tag)
        };
        for structure in &level.structure_bsps {
            let palette: Vec<Option<usize>> = structure
                .lens_flares
                .iter()
                .map(|tag| flare(tag.as_ref()))
                .collect();
            out.markers.push(
                structure
                    .lens_flare_markers
                    .iter()
                    .filter_map(|marker| {
                        let index = (*palette.get(usize::from(marker.lens_flare))?)?;
                        Some((marker.position, marker.direction, index))
                    })
                    .collect(),
            );
        }
        for sky in &level.skies {
            for light in &sky.lights {
                if let Some(index) = flare(light.lens_flare.as_ref()) {
                    out.sky.push((light.towards(), index));
                }
            }
        }
        out.light_flares = level
            .lights
            .iter()
            .map(|light| flare(light.lens_flare.as_ref()))
            .collect();
        for object in &level.objects {
            let widgets = widgets(level, object);
            if widgets.is_empty() {
                continue;
            }
            let model = object
                .model
                .as_ref()
                .and_then(|model| level.model_index(model));
            out.carried.insert(object.tag.clone(), (widgets, model));
        }
        out
    }

    /// The widgets the objects in `simulation` carry, and the BSP the
    /// camera is in, for the sprites of the frames until the next tick.
    pub(crate) fn gather(
        &self,
        level: &Level,
        simulation: &mut Simulation,
        bsp: usize,
        out: &mut CameraSources,
    ) {
        out.bsp = bsp;
        out.placed.clear();
        simulation.visit_lit(|object| {
            let Some((widgets, model)) = self.carried.get(object.tag) else {
                return;
            };
            let model = model.and_then(|index| level.models.get(index));
            let rest = match (object.nodes, model) {
                (None, Some(model)) => default_pose(model),
                _ => Vec::new(),
            };
            let functions = object.functions.map_or([0.0; 4], |values| {
                [0, 1, 2, 3].map(|i| {
                    if values.active[i] {
                        values.outgoing[i]
                    } else {
                        0.0
                    }
                })
            });
            for widget in widgets {
                let marker = match widget {
                    Widget::Volume(index) => &level.light_volumes[*index].marker,
                    Widget::Glow(index) => &level.glows[*index].marker,
                };
                let (frame, _) = marker_frames(model, &object, &rest, marker);
                out.placed.push(Placed {
                    widget: widget.clone(),
                    frame,
                    functions,
                    id: object.id.0,
                });
            }
        });
    }

    /// The frame's sprites seen from `camera` at `seconds`: the flares of
    /// the BSP's markers, of `lights` and of the sky (each when `flares`),
    /// and the widgets `sources` placed (when `widgets`).
    #[allow(clippy::too_many_arguments)]
    pub(crate) fn sprites(
        &self,
        level: &Level,
        sources: &CameraSources,
        lights: &[LightSnapshot],
        camera: &SpriteCamera,
        seconds: f32,
        flares: bool,
        widgets: bool,
    ) -> CameraSprites {
        let mut out = CameraSprites::default();
        let Some(view) = View::new(camera) else {
            return out;
        };
        if flares {
            let mut lit = Vec::new();
            for marker in self.markers.get(sources.bsp).into_iter().flatten() {
                lit.push((marker.0, Some(marker.1), [1.0; 4], 0.0, marker.2));
            }
            for light in lights {
                let Some(Some(flare)) = self.light_flares.get(light.light as usize) else {
                    continue;
                };
                let color = [light.color.x, light.color.y, light.color.z];
                let brightest = color.iter().fold(0.0f32, |a, &b| a.max(b));
                if brightest <= 0.0 {
                    continue;
                }
                let position = [light.position.x, light.position.y, light.position.z];
                let forward = [light.forward.x, light.forward.y, light.forward.z];
                // (the colour's hue at its brightest; its brightness the scale)
                let hue = color.map(|channel| channel / brightest);
                lit.push((
                    position,
                    Some(forward),
                    [hue[0], hue[1], hue[2], 1.0],
                    brightest.min(1.0),
                    *flare,
                ));
            }
            for (towards, flare) in &self.sky {
                let position = add(view.eye, scale(*towards, SKY_DISTANCE));
                lit.push((position, None, [1.0; 4], 1.0, *flare));
            }
            for (position, direction, color, light_scale, flare) in lit {
                if out.occluders.len() >= MAX_OCCLUDERS {
                    break;
                }
                let Some(definition) = level.lens_flares.get(flare) else {
                    continue;
                };
                flare_sprites(
                    definition,
                    &self.flare_images[flare],
                    &Lit {
                        position,
                        direction,
                        color,
                        light_scale,
                    },
                    &view,
                    seconds,
                    &mut out,
                );
            }
        }
        if widgets {
            for placed in &sources.placed {
                match placed.widget {
                    Widget::Volume(index) => volume_sprites(
                        &level.light_volumes[index],
                        self.volume_images[index],
                        placed,
                        &view,
                        &mut out.sprites,
                    ),
                    Widget::Glow(index) => glow_sprites(
                        &level.glows[index],
                        self.glow_images[index],
                        placed,
                        seconds,
                        &mut out.sprites,
                    ),
                }
            }
        }
        out.sprites.truncate(MAX_SPRITES);
        out
    }
}

/// The light volumes and glows `object` carries.
fn widgets(level: &Level, object: &ObjectDefinition) -> Vec<Widget> {
    object
        .widgets
        .iter()
        .filter_map(|tag| {
            if let Some(index) = level.light_volumes.iter().position(|v| v.tag == *tag) {
                return Some(Widget::Volume(index));
            }
            let index = level.glows.iter().position(|glow| glow.tag == *tag)?;
            Some(Widget::Glow(index))
        })
        .collect()
}

/// The camera's frame.
struct View {
    eye: [f32; 3],
    forward: [f32; 3],
    left: [f32; 3],
    up: [f32; 3],
}

impl View {
    fn new(camera: &SpriteCamera) -> Option<Self> {
        let vector = |values: &[f32]| -> Option<[f32; 3]> {
            let vector: [f32; 3] = values.get(..3)?.try_into().ok()?;
            vector.iter().all(|v| v.is_finite()).then_some(vector)
        };
        let eye = vector(&camera.position)?;
        let forward = normalized(vector(&camera.forward)?)?;
        let left = normalized(cross(vector(&camera.up)?, forward))?;
        let up = cross(forward, left);
        Some(Self {
            eye,
            forward,
            left,
            up,
        })
    }
}

/// A light a flare is drawn for.
struct Lit {
    position: [f32; 3],
    /// The way it faces (none for the sky's).
    direction: Option<[f32; 3]>,
    /// Its colour; the alpha scales its brightness.
    color: [f32; 4],
    /// Its intensity, 0 to 1, which its reflections' bounds are taken at.
    light_scale: f32,
}

/// `PIN(value, 0, 1)`.
fn pin(value: f32) -> f32 {
    if value.is_nan() {
        0.0
    } else {
        value.clamp(0.0, 1.0)
    }
}

fn lerp([lower, upper]: [f32; 2], t: f32) -> f32 {
    lower + (upper - lower) * t
}

/// The sprites of `flare` for `light` (`rasterizer_lens_flare_render`):
/// its occlusion disk, then each reflection along the line from the
/// light through the screen's centre.
fn flare_sprites(
    flare: &LensFlare,
    images: &[u32],
    light: &Lit,
    view: &View,
    seconds: f32,
    out: &mut CameraSprites,
) {
    let to_corona = sub(light.position, view.eye);
    let depth = dot(view.forward, to_corona);
    if depth <= NEAREST {
        return;
    }
    let Some(towards_camera) = normalized(to_corona) else {
        return;
    };
    let mut light_brightness = if flare.far_fade_distance > 0.0 {
        pin((depth - flare.far_fade_distance)
            / (flare.near_fade_distance - flare.far_fade_distance))
    } else {
        1.0
    } * light.color[3];
    if light_brightness <= 0.0 {
        return;
    }
    // (the disk: towards the viewer, along the light's forward, or at it)
    let occlusion = match (flare.occlusion_offset_direction, light.direction) {
        (0, _) => sub(light.position, scale(view.forward, flare.occlusion_radius)),
        (1, Some(direction)) => add(
            light.position,
            scale(direction, flare.occlusion_radius * std::f32::consts::SQRT_2),
        ),
        _ => light.position,
    };
    let occluder = out.occluders.len() as u32;
    out.occluders.push(FlareOccluder {
        x: occlusion[0],
        y: occlusion[1],
        z: occlusion[2],
        radius: flare.occlusion_radius.max(0.0),
    });
    let corona_axis = scale(sub(scale(view.forward, depth), to_corona), 2.0);
    let screen_rotation = libm::atan2f(dot(view.up, to_corona), dot(view.left, to_corona));
    let corona_rotation = match flare.corona_rotation_function {
        // (by the light's distance from the screen's centre, and by its angle
        // round it)
        1 => length(corona_axis) / (2.0 * depth),
        2 => screen_rotation / std::f32::consts::TAU,
        _ => 0.0,
    } * flare.corona_rotation_function_scale.to_radians();
    let cosine_scale = {
        let range = flare.cosine_falloff_angle - flare.cosine_cutoff_angle;
        if range.abs() > f32::EPSILON {
            1.0 / range
        } else {
            1.0
        }
    };
    let cosine_offset = -flare.cosine_cutoff_angle * cosine_scale;
    let by_angle = |function: i16| -> f32 {
        let cosine = match (function, light.direction) {
            (0, _) => return 1.0,
            (1, Some(direction)) => -dot(view.forward, direction),
            (2, Some(direction)) => -dot(direction, towards_camera),
            (3, _) => dot(view.forward, towards_camera),
            _ => return 0.0,
        };
        pin(cosine * cosine_scale + cosine_offset)
    };
    for (at, reflection) in flare.reflections.iter().enumerate() {
        let mut brightness = lerp(reflection.brightness, light.light_scale)
            * by_angle(reflection.brightness_scale_function)
            * light_brightness;
        if at == 0 {
            light_brightness = brightness;
        }
        if brightness <= 0.0 || out.sprites.len() >= MAX_SPRITES {
            continue;
        }
        let mut radius = lerp(reflection.radius, light.light_scale);
        if reflection.flags & LensFlareReflection::RADIUS_NOT_SCALED_BY_DISTANCE != 0 {
            radius *= depth;
        }
        let (mut rotation, size) = if at == 0 {
            (
                corona_rotation + reflection.rotation_offset.to_radians(),
                flare.corona_radius_scale,
            )
        } else {
            (reflection.rotation_offset.to_radians(), [1.0, 1.0])
        };
        if reflection.flags & LensFlareReflection::ROTATE_FROM_CENTER != 0 {
            rotation += screen_rotation;
        }
        let tint = tint(reflection, light.color, seconds);
        brightness = brightness.min(16.0);
        let point = add(light.position, scale(corona_axis, reflection.offset));
        let mut flags = 0;
        if reflection.flags & LensFlareReflection::RADIUS_SCALED_BY_OCCLUSION != 0 {
            flags |= SPRITE_SCALED_BY_OCCLUSION;
        }
        out.sprites.push(CameraSprite {
            x: point[0],
            y: point[1],
            z: point[2],
            width: radius * if size[0] > 0.0 { size[0] } else { 1.0 },
            height: radius * if size[1] > 0.0 { size[1] } else { 1.0 },
            rotation,
            red: tint[0] * brightness,
            green: tint[1] * brightness,
            blue: tint[2] * brightness,
            image: images.get(at).copied().unwrap_or(u32::MAX),
            occluder,
            flags,
        });
    }
}

/// A reflection's colour: its tint (the light's colour if it has none),
/// times its animated colour.
fn tint(reflection: &LensFlareReflection, light: [f32; 4], seconds: f32) -> [f32; 3] {
    let tint = reflection.tint;
    let mut color = if tint[..3].iter().all(|&channel| channel == 0.0) {
        [light[0], light[1], light[2]]
    } else {
        [tint[0], tint[1], tint[2]]
    };
    if reflection.animation_function > 1 && reflection.animation_period > 0.0 {
        let phase = (seconds + reflection.animation_phase) / reflection.animation_period;
        let t = 0.5 - 0.5 * libm::cosf(phase * std::f32::consts::TAU);
        let [lower, upper] = reflection.animation_color;
        for (axis, channel) in color.iter_mut().enumerate() {
            *channel *= lower[axis] + (upper[axis] - lower[axis]) * t;
        }
    }
    color
}

/// `t` to the power `exponent` (1 for an exponent that is not one).
fn shape(t: f32, exponent: f32) -> f32 {
    if exponent > 0.0 && exponent.is_finite() {
        libm::powf(t, exponent)
    } else {
        t
    }
}

/// A light volume's sprites: a row along its marker's forward, faded as
/// it is seen end on and far off.
fn volume_sprites(
    volume: &LightVolume,
    image: u32,
    placed: &Placed,
    view: &View,
    out: &mut Vec<CameraSprite>,
) {
    let Some(frame) = volume.frames.first() else {
        return;
    };
    let forward = placed.frame.forward;
    let origin = placed.frame.position;
    let distance = length(sub(origin, view.eye));
    let mut brightness = lerp(
        [
            volume.perpendicular_brightness_scale,
            volume.parallel_brightness_scale,
        ],
        dot(view.forward, forward).abs(),
    );
    if volume.far_fade_distance > 0.0 {
        brightness *= pin((distance - volume.far_fade_distance)
            / (volume.near_fade_distance - volume.far_fade_distance));
    }
    if let Ok(source @ 0..=3) = usize::try_from(volume.brightness_scale_source - 1) {
        brightness *= placed.functions[source];
    }
    if brightness <= 0.0 {
        return;
    }
    let count = usize::try_from(volume.count)
        .unwrap_or(0)
        .clamp(1, MAX_WIDGET_SPRITES);
    for at in 0..count {
        let t = if count > 1 {
            at as f32 / (count - 1) as f32
        } else {
            0.0
        };
        let along = frame.offset_from_marker + frame.length * shape(t, frame.offset_exponent);
        let radius = lerp(frame.radius, shape(t, frame.radius_exponent));
        let color_t = shape(t, frame.color_exponent);
        let [lower, upper] = frame.color;
        let alpha = lerp([lower[3], upper[3]], shape(t, frame.brightness_exponent)) * brightness;
        let color = [0, 1, 2].map(|axis| lerp([lower[axis], upper[axis]], color_t) * alpha);
        let point = add(origin, scale(forward, along));
        out.push(CameraSprite {
            x: point[0],
            y: point[1],
            z: point[2],
            width: radius,
            height: radius,
            rotation: 0.0,
            red: color[0],
            green: color[1],
            blue: color[2],
            image,
            occluder: u32::MAX,
            flags: SPRITE_DEPTH_TESTED,
        });
    }
}

/// A glow's particles: turning round its marker's forward, each at its
/// own distance, size and colour between the bounds.
fn glow_sprites(
    glow: &Glow,
    image: u32,
    placed: &Placed,
    seconds: f32,
    out: &mut Vec<CameraSprite>,
) {
    let count = usize::try_from(glow.number_of_particles)
        .unwrap_or(0)
        .min(MAX_WIDGET_SPRITES);
    let frame = &placed.frame;
    for at in 0..count {
        // (each particle's place between the bounds, the same every frame)
        let t = spread(placed.id, at as u32);
        let angle = (at as f32 + t) / count as f32 * std::f32::consts::TAU
            + glow.effect_rotational_velocity * seconds;
        let distance = lerp(glow.distance_to_object, t);
        let radius = lerp(glow.particle_size, t);
        let [lower, upper] = glow.color;
        let alpha = lerp([lower[3], upper[3]], t);
        let color = [0, 1, 2].map(|axis| lerp([lower[axis], upper[axis]], t) * alpha);
        let offset = add(
            scale(frame.left, libm::cosf(angle) * distance),
            scale(frame.up, libm::sinf(angle) * distance),
        );
        let point = add(frame.position, offset);
        out.push(CameraSprite {
            x: point[0],
            y: point[1],
            z: point[2],
            width: radius,
            height: radius,
            rotation: angle,
            red: color[0],
            green: color[1],
            blue: color[2],
            image,
            occluder: u32::MAX,
            flags: SPRITE_DEPTH_TESTED,
        });
    }
}

/// A number from 0 to 1 for the particle `at` of object `id`.
fn spread(id: u32, at: u32) -> f32 {
    let mut hash = id.wrapping_mul(0x9E37_79B9) ^ at.wrapping_mul(0x85EB_CA6B);
    hash ^= hash >> 15;
    hash = hash.wrapping_mul(0x2C1B_3C6D);
    hash ^= hash >> 12;
    (hash >> 8) as f32 / (1u32 << 24) as f32
}

fn add(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] + b[0], a[1] + b[1], a[2] + b[2]]
}

fn sub(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
}

fn scale(a: [f32; 3], s: f32) -> [f32; 3] {
    a.map(|v| v * s)
}

fn dot(a: [f32; 3], b: [f32; 3]) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

fn cross(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
}

fn length(a: [f32; 3]) -> f32 {
    libm::sqrtf(dot(a, a))
}

fn normalized(a: [f32; 3]) -> Option<[f32; 3]> {
    let length = length(a);
    (length > 1e-6).then(|| scale(a, 1.0 / length))
}

/// The exposure's limits outdoors, in stops from the level's own (the
/// sky's light is steady, so the view adapts little).
pub const EXTERIOR_EXPOSURE: [f32; 2] = [-0.5, 0.5];
/// The exposure's limits indoors, where the view adapts more to the dark.
pub const INTERIOR_EXPOSURE: [f32; 2] = [-1.0, 1.5];
/// The average brightness the exposure brings the view to.
const EXPOSURE_KEY: f32 = 0.18;
/// How fast the exposure adapts, per second: to a brighter view, and to a
/// darker one.
const ADAPT_BRIGHTER: f32 = 3.0;
const ADAPT_DARKER: f32 = 1.0;

/// The exposure's limits in a BSP, least and greatest, in stops darker
/// than the level's own light: an exterior's (one at least half of whose
/// clusters see a sky) or an interior's.
pub(crate) fn exposure_limits(bsp: &StructureBsp) -> [f32; 2] {
    let skies = bsp.cluster_skies.iter().filter(|sky| sky.is_some()).count();
    let sees_sky = !bsp.cluster_skies.is_empty() && skies * 2 >= bsp.cluster_skies.len();
    if sees_sky {
        EXTERIOR_EXPOSURE
    } else {
        INTERIOR_EXPOSURE
    }
}

/// The exposure `seconds` after it was `current` (in stops darker than
/// the level's own light), for a view whose average log2 brightness is
/// `log_luminance`: towards the stops that bring it to the key, within
/// `limits`, adapting faster to light than to the dark.
#[uniffi::export]
pub fn adapt_exposure(current: f32, log_luminance: f32, limits: Vec<f32>, seconds: f32) -> f32 {
    let (lower, upper) = match limits[..] {
        [lower, upper] if lower <= upper => (lower, upper),
        _ => (0.0, 0.0),
    };
    let current = if current.is_finite() {
        current.clamp(lower, upper)
    } else {
        0.0_f32.clamp(lower, upper)
    };
    if !log_luminance.is_finite() || !seconds.is_finite() || seconds <= 0.0 {
        return current;
    }
    let target = (log_luminance - libm::log2f(EXPOSURE_KEY)).clamp(lower, upper);
    let rate = if target > current {
        ADAPT_BRIGHTER
    } else {
        ADAPT_DARKER
    };
    current + (target - current) * (1.0 - libm::expf(-rate * seconds))
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Group, LensFlareMarker, LightVolumeFrame};

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn reflection(offset: f32) -> LensFlareReflection {
        LensFlareReflection {
            flags: 0,
            bitmap_index: 0,
            offset,
            rotation_offset: 0.0,
            radius: [1.0, 1.0],
            brightness: [1.0, 1.0],
            brightness_scale_function: 0,
            tint: [0.0; 4],
            animation_color: [[1.0; 4]; 2],
            animation_flags: 0,
            animation_function: 0,
            animation_period: 0.0,
            animation_phase: 0.0,
        }
    }

    fn flare() -> LensFlare {
        LensFlare {
            tag: tag(b"lens", "flare"),
            cosine_falloff_angle: 1.0,
            cosine_cutoff_angle: 0.0,
            occlusion_radius: 0.5,
            occlusion_offset_direction: 0,
            near_fade_distance: 0.0,
            far_fade_distance: 0.0,
            bitmap: Some(tag(b"bitm", "flare")),
            flags: 0,
            corona_rotation_function: 0,
            corona_rotation_function_scale: 0.0,
            corona_radius_scale: [2.0, 1.0],
            reflections: vec![reflection(0.0), reflection(1.0)],
        }
    }

    fn camera() -> SpriteCamera {
        SpriteCamera {
            position: vec![0.0, 0.0, 0.0],
            forward: vec![1.0, 0.0, 0.0],
            up: vec![0.0, 0.0, 1.0],
        }
    }

    /// A level with one BSP whose one marker shows `flare` at (10, 2, 1).
    fn level(flare: LensFlare) -> Level {
        let mut level = crate::debug_scene::tests::level();
        level.lens_flares = vec![flare];
        let mut bsp = level.structure_bsps.remove(0);
        bsp.lens_flares = vec![Some(tag(b"lens", "flare"))];
        bsp.lens_flare_markers = vec![LensFlareMarker {
            position: [10.0, 2.0, 1.0],
            direction: [-1.0, 0.0, 0.0],
            lens_flare: 0,
        }];
        level.structure_bsps = vec![bsp];
        level
    }

    #[test]
    fn a_marker_s_flare_is_drawn_along_the_line_through_the_centre() {
        let level = level(flare());
        let camera_level = CameraLevel::new(&level);
        let sprites = camera_level.sprites(
            &level,
            &CameraSources::default(),
            &[],
            &camera(),
            0.0,
            true,
            true,
        );
        assert_eq!(sprites.occluders.len(), 1);
        // (towards the viewer by its radius)
        assert_eq!(sprites.occluders[0].x, 9.5);
        assert_eq!(sprites.sprites.len(), 2);
        let corona = &sprites.sprites[0];
        assert_eq!([corona.x, corona.y, corona.z], [10.0, 2.0, 1.0]);
        assert_eq!([corona.width, corona.height], [2.0, 1.0]);
        // (the second is at the centre of the screen, at the same depth)
        let centre = &sprites.sprites[1];
        assert_eq!([centre.x, centre.y, centre.z], [10.0, -2.0, -1.0]);
        assert_eq!(corona.occluder, 0);
        assert_eq!(corona.image, 0);
        assert_eq!(camera_level.images, [(tag(b"bitm", "flare"), 0)]);
    }

    #[test]
    fn a_flare_behind_the_camera_or_switched_off_is_not_drawn() {
        let level = level(flare());
        let camera_level = CameraLevel::new(&level);
        let mut behind = camera();
        behind.forward = vec![-1.0, 0.0, 0.0];
        for (camera, flares) in [(behind, true), (camera(), false)] {
            let sprites = camera_level.sprites(
                &level,
                &CameraSources::default(),
                &[],
                &camera,
                0.0,
                flares,
                true,
            );
            assert!(sprites.sprites.is_empty());
            assert!(sprites.occluders.is_empty());
        }
    }

    #[test]
    fn a_flare_fades_with_distance_and_by_its_angle() {
        let mut faded = flare();
        faded.near_fade_distance = 5.0;
        faded.far_fade_distance = 15.0;
        // (seen from straight ahead, the light faces the camera: full)
        faded.reflections[1].brightness_scale_function = 1;
        let level = level(faded);
        let sprites = CameraLevel::new(&level).sprites(
            &level,
            &CameraSources::default(),
            &[],
            &camera(),
            0.0,
            true,
            false,
        );
        let corona = &sprites.sprites[0];
        assert!((corona.red - 0.5).abs() < 1e-5, "{}", corona.red);
        assert!((sprites.sprites[1].red - 0.5).abs() < 1e-5);
        // (beyond the far distance it is gone)
        let mut far = camera();
        far.position = vec![-20.0, 0.0, 0.0];
        let sprites = CameraLevel::new(&level).sprites(
            &level,
            &CameraSources::default(),
            &[],
            &far,
            0.0,
            true,
            false,
        );
        assert!(sprites.sprites.is_empty());
    }

    #[test]
    fn the_sky_s_flare_is_far_ahead_and_a_light_s_takes_its_colour() {
        let mut level = level(flare());
        level.structure_bsps[0].lens_flare_markers.clear();
        let fog = opence_content::level::AtmosphericFog {
            color: [0.0; 3],
            maximum_density: 0.0,
            start_distance: 0.0,
            opaque_distance: 0.0,
        };
        level.skies = vec![opence_content::Sky {
            lights: vec![opence_content::SkyLight {
                lens_flare: Some(tag(b"lens", "flare")),
                flags: 0,
                color: [1.0; 3],
                power: 1.0,
                test_distance: 0.0,
                direction: [0.0, 0.0],
                diameter: 0.0,
            }],
            tag: tag(b"sky ", "sky"),
            model: None,
            outdoor_fog: fog,
            indoor_fog: fog,
            indoor_ambient: Default::default(),
            outdoor_ambient: Default::default(),
        }];
        let mut light = crate::lights::tests::light(0);
        light.lens_flare = Some(tag(b"lens", "flare"));
        level.lights = vec![light];
        let lit = LightSnapshot {
            position: crate::snapshot::Vec3 {
                x: 4.0,
                y: 0.0,
                z: 0.0,
            },
            color: crate::snapshot::Vec3 {
                x: 0.5,
                y: 0.25,
                z: 0.0,
            },
            light: 0,
            ..LightSnapshot::default()
        };
        let sprites = CameraLevel::new(&level).sprites(
            &level,
            &CameraSources::default(),
            &[lit],
            &camera(),
            0.0,
            true,
            false,
        );
        assert_eq!(sprites.occluders.len(), 2);
        let light = &sprites.sprites[0];
        // (its hue; its brightness picks between the bounds, both 1 here)
        assert_eq!([light.red, light.green, light.blue], [1.0, 0.5, 0.0]);
        let sky = &sprites.sprites[2];
        assert_eq!(sky.x, SKY_DISTANCE);
    }

    #[test]
    fn a_light_volume_is_a_row_of_sprites_along_its_marker() {
        let volume = LightVolume {
            tag: tag(b"mgs2", "beam"),
            marker: String::new(),
            near_fade_distance: 0.0,
            far_fade_distance: 0.0,
            perpendicular_brightness_scale: 1.0,
            parallel_brightness_scale: 0.0,
            brightness_scale_source: 0,
            bitmap: None,
            sequence_index: 0,
            count: 3,
            frames: vec![LightVolumeFrame {
                offset_from_marker: 1.0,
                offset_exponent: 1.0,
                length: 4.0,
                radius: [0.5, 1.5],
                radius_exponent: 1.0,
                color: [[1.0, 0.0, 0.0, 1.0], [0.0, 0.0, 1.0, 0.0]],
                color_exponent: 1.0,
                brightness_exponent: 1.0,
            }],
        };
        let placed = Placed {
            widget: Widget::Volume(0),
            frame: NodeMatrix {
                forward: [0.0, 1.0, 0.0],
                left: [-1.0, 0.0, 0.0],
                up: [0.0, 0.0, 1.0],
                position: [5.0, 0.0, 0.0],
                scale: 1.0,
            },
            functions: [0.0; 4],
            id: 1,
        };
        let Some(view) = View::new(&camera()) else {
            panic!("the camera has no view");
        };
        let mut out = Vec::new();
        volume_sprites(&volume, u32::MAX, &placed, &view, &mut out);
        assert_eq!(out.len(), 3);
        assert_eq!([out[0].y, out[1].y, out[2].y], [1.0, 3.0, 5.0]);
        assert_eq!([out[0].width, out[2].width], [0.5, 1.5]);
        assert_eq!([out[0].red, out[2].red, out[2].blue], [1.0, 0.0, 0.0]);
        assert_eq!(out[0].flags, SPRITE_DEPTH_TESTED);
        // (seen end on it is gone)
        let mut end_on = placed.clone();
        end_on.frame.forward = [1.0, 0.0, 0.0];
        out.clear();
        volume_sprites(&volume, u32::MAX, &end_on, &view, &mut out);
        assert!(out.is_empty());
    }

    #[test]
    fn a_glow_s_particles_turn_round_its_marker() {
        let glow = Glow {
            tag: tag(b"glw!", "glow"),
            marker: String::new(),
            number_of_particles: 4,
            flags: 0,
            effect_rotational_velocity: 1.0,
            distance_to_object: [1.0, 1.0],
            particle_size: [0.25, 0.25],
            color: [[1.0; 4]; 2],
            percentage_edge_fade: 0.0,
            texture: None,
        };
        let placed = Placed {
            widget: Widget::Glow(0),
            frame: NodeMatrix {
                forward: [1.0, 0.0, 0.0],
                left: [0.0, 1.0, 0.0],
                up: [0.0, 0.0, 1.0],
                position: [5.0, 0.0, 0.0],
                scale: 1.0,
            },
            functions: [0.0; 4],
            id: 7,
        };
        let mut at_start = Vec::new();
        glow_sprites(&glow, 3, &placed, 0.0, &mut at_start);
        let mut later = Vec::new();
        glow_sprites(&glow, 3, &placed, 1.0, &mut later);
        assert_eq!(at_start.len(), 4);
        for (start, later) in at_start.iter().zip(&later) {
            let radius = |s: &CameraSprite| (s.y * s.y + s.z * s.z).sqrt();
            assert!((radius(start) - 1.0).abs() < 1e-5);
            assert_eq!(start.x, 5.0);
            assert!((start.rotation + 1.0 - later.rotation).abs() < 1e-5);
            assert_eq!(start.image, 3);
        }
    }

    #[test]
    fn a_camera_without_a_frame_draws_nothing() {
        let level = level(flare());
        let sprites = CameraLevel::new(&level).sprites(
            &level,
            &CameraSources::default(),
            &[],
            &SpriteCamera::default(),
            0.0,
            true,
            true,
        );
        assert_eq!(sprites, CameraSprites::default());
    }

    #[test]
    fn exposure_adapts_within_the_limits() {
        let limits = INTERIOR_EXPOSURE.to_vec();
        let bright = libm::log2f(EXPOSURE_KEY * 64.0);
        let dark = libm::log2f(EXPOSURE_KEY / 64.0);
        // (a bright view darkens, towards the greatest stops, and never past)
        let mut exposure = 0.0;
        for _ in 0..600 {
            exposure = adapt_exposure(exposure, bright, limits.clone(), 1.0 / 60.0);
        }
        assert!((exposure - INTERIOR_EXPOSURE[1]).abs() < 1e-3, "{exposure}");
        // (a dark one brightens, more slowly)
        let one = adapt_exposure(0.0, dark, limits.clone(), 0.1);
        let other = adapt_exposure(0.0, bright, limits.clone(), 0.1);
        assert!(one < 0.0 && other > 0.0 && other > -one, "{one} {other}");
        assert!(adapt_exposure(0.0, dark, limits.clone(), 100.0) >= INTERIOR_EXPOSURE[0]);
        // (a view at the key stays as it is; nothing is made of nonsense)
        assert_eq!(
            adapt_exposure(0.0, libm::log2f(EXPOSURE_KEY), limits.clone(), 1.0),
            0.0
        );
        assert_eq!(adapt_exposure(0.25, f32::NAN, limits.clone(), 1.0), 0.25);
        assert_eq!(adapt_exposure(f32::NAN, bright, limits.clone(), 0.0), 0.0);
        assert_eq!(
            adapt_exposure(9.0, bright, limits, -1.0),
            INTERIOR_EXPOSURE[1]
        );
        assert_eq!(adapt_exposure(1.0, bright, vec![1.0], 1.0), 0.0);
    }

    #[test]
    fn an_exterior_bsp_adapts_less_than_an_interior() {
        let mut level = crate::debug_scene::tests::level();
        let mut bsp = level.structure_bsps.remove(0);
        bsp.cluster_skies.clear();
        assert_eq!(exposure_limits(&bsp), INTERIOR_EXPOSURE);
        bsp.cluster_skies = vec![None, None, Some(0)];
        assert_eq!(exposure_limits(&bsp), INTERIOR_EXPOSURE);
        bsp.cluster_skies.push(Some(0));
        assert_eq!(exposure_limits(&bsp), EXTERIOR_EXPOSURE);
        let width = |[lower, upper]: [f32; 2]| upper - lower;
        assert!(width(EXTERIOR_EXPOSURE) < width(INTERIOR_EXPOSURE));
    }
}
