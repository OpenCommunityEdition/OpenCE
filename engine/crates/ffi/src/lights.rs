//! The dynamic lights of a frame: the lights objects carry at their
//! markers (a weapon's glow, a projectile's, a piece of scenery's, the
//! flashlight), scaled by the objects' functions, and the lights effects
//! make, which fade over their durations; then the cells of the view each
//! light reaches, and the lights that cast shadows.

use std::collections::BTreeMap;

use opence_content::{Level, Light, TagRef};
use opence_sim::animation::NodeMatrix;
use opence_sim::damage::transition;
use opence_sim::effects::FiredPart;
use opence_sim::seats::{default_pose, marker_matrix};
use opence_sim::{Quat, Simulation, Vec3};

use crate::snapshot::{
    self, LightSnapshot, LIGHT_FIRST_PERSON, LIGHT_NO_SPECULAR, LIGHT_SPOT, MAX_LIGHTS,
};

/// A light an object definition carries at a marker.
#[derive(Clone, Debug, PartialEq)]
struct Attached {
    /// Its tag's index in the level's lights.
    light: usize,
    /// The marker it is at ("" for the object's origin).
    marker: String,
    /// The function (1 to 4) its intensity is (0: none, which is 1).
    primary_scale: i16,
}

/// An object definition's lights and the model their markers are on.
#[derive(Clone, Debug, Default, PartialEq)]
struct Carried {
    lights: Vec<Attached>,
    model: Option<usize>,
}

/// A light an effect made, fading.
#[derive(Clone, Copy, Debug, PartialEq)]
struct Fading {
    light: usize,
    position: Vec3,
    rotation: Quat,
    /// The ticks it has shone.
    age: u32,
    /// The effect's number, which it is known by from frame to frame.
    number: u32,
}

/// The level's lights and the effects' lights that still shine.
#[derive(Clone, Debug, Default)]
pub(crate) struct Lights {
    carried: BTreeMap<TagRef, Carried>,
    fading: Vec<Fading>,
    /// The level's lens flares and widgets.
    pub(crate) camera: crate::camera::CameraLevel,
    /// The widgets the last frame placed, and its BSP.
    pub(crate) sources: crate::camera::CameraSources,
}

/// An effect's light's `source`: its own range, above the objects'.
pub(crate) const EFFECT_SOURCE: u32 = 1 << 31;

/// The ticks a light lasts: its duration, at least one.
fn ticks(light: &Light) -> u32 {
    if light.duration >= 1.0 {
        light.duration as u32
    } else {
        1
    }
}

/// Linear from a colour `0` (at 0) to `1` (at `t` 1), by hue, saturation
/// and value if `flags` say so, the long way round the hues for
/// `Light::MORE_COLORS`.
pub(crate) fn interpolate(flags: u32, lower: [f32; 3], upper: [f32; 3], t: f32) -> [f32; 3] {
    if flags & Light::HSV == 0 {
        return [0, 1, 2].map(|i| lower[i] + (upper[i] - lower[i]) * t);
    }
    let [h0, s0, v0] = hsv(lower);
    let [mut h1, s1, v1] = hsv(upper);
    let shorter = (h1 - h0).abs() <= 0.5;
    if shorter == (flags & Light::MORE_COLORS != 0) {
        h1 += if h1 > h0 { -1.0 } else { 1.0 };
    }
    let hue = h0 + (h1 - h0) * t;
    rgb([
        hue - libm::floorf(hue),
        s0 + (s1 - s0) * t,
        v0 + (v1 - v0) * t,
    ])
}

/// A colour's hue (0 to 1), saturation and value.
fn hsv([r, g, b]: [f32; 3]) -> [f32; 3] {
    let max = r.max(g).max(b);
    let range = max - r.min(g).min(b);
    if range <= 0.0 {
        return [0.0, 0.0, max];
    }
    let hue = if max == r {
        (g - b) / range
    } else if max == g {
        2.0 + (b - r) / range
    } else {
        4.0 + (r - g) / range
    } / 6.0;
    [hue - libm::floorf(hue), range / max, max]
}

/// The colour of a hue (0 to 1), saturation and value.
fn rgb([hue, saturation, value]: [f32; 3]) -> [f32; 3] {
    let sector = hue * 6.0;
    let channel = |n: f32| {
        let k = libm::fmodf(n + sector, 6.0);
        value - value * saturation * k.min(4.0 - k).clamp(0.0, 1.0)
    };
    [channel(5.0), channel(3.0), channel(1.0)]
}

/// A colour that the game gives as display values, linear.
fn linear(value: f32) -> f32 {
    libm::powf(value.max(0.0), 2.2)
}

/// `light` at `intensity`, its frame `matrix` (the way it faces on its x,
/// its gel's up on its z), at `previous` the tick before; none when it
/// gives no light.
fn shine(
    light: &Light,
    index: usize,
    intensity: f32,
    matrix: &NodeMatrix,
    previous: Vec3,
    source: u32,
) -> Option<LightSnapshot> {
    if light.flags & Light::DYNAMIC == 0 || intensity <= 0.0 {
        return None;
    }
    let [modifier_lower, modifier_upper] = light.radius_modifier;
    let scale = if modifier_lower == 0.0 && modifier_upper == 0.0 {
        1.0
    } else {
        modifier_lower + (modifier_upper - modifier_lower) * intensity
    };
    let radius = light.radius * scale;
    let lower = [
        light.color_lower[1],
        light.color_lower[2],
        light.color_lower[3],
    ];
    let upper = [
        light.color_upper[1],
        light.color_upper[2],
        light.color_upper[3],
    ];
    let color = interpolate(light.color_flags, lower, upper, intensity).map(linear);
    if radius <= 0.0 || color.iter().all(|&channel| channel <= 0.0) {
        return None;
    }
    let vector = |[x, y, z]: [f32; 3]| snapshot::Vec3 { x, y, z };
    let spot = light.cutoff_angle > 0.0 && light.cutoff_angle < std::f32::consts::PI - 0.01;
    let mut flags = 0;
    if spot {
        flags |= LIGHT_SPOT;
    }
    if light.flags & Light::NO_SPECULAR != 0 {
        flags |= LIGHT_NO_SPECULAR;
    }
    Some(LightSnapshot {
        position: vector(matrix.position),
        radius,
        previous: vector(previous.to_array()),
        cosine_cutoff: if spot {
            libm::cosf(light.cutoff_angle)
        } else {
            -1.0
        },
        forward: vector(matrix.forward),
        cosine_falloff: if spot {
            libm::cosf(light.falloff_angle.min(light.cutoff_angle))
        } else {
            -1.0
        },
        up: vector(matrix.up),
        specular_radius: radius * light.specular_radius_multiplier.max(1.0),
        color: vector(color),
        flags,
        light: index as u32,
        source,
    })
}

/// The frame of `marker` on `object` (its origin for "" or a marker its
/// model lacks) at the last tick and the tick before; `rest` is the
/// model's default pose, for an object that is not animated.
pub(crate) fn marker_frames(
    model: Option<&opence_content::Model>,
    object: &opence_sim::Lit<'_>,
    rest: &[NodeMatrix],
    marker: &str,
) -> (NodeMatrix, NodeMatrix) {
    let place = |nodes: &[NodeMatrix], transform: &opence_sim::Transform| {
        let origin = frame(transform.position, transform.rotation);
        let local = model
            .filter(|_| !marker.is_empty())
            .and_then(|model| marker_matrix(model, nodes, marker));
        match (local, object.nodes.is_some()) {
            // (an animated object's nodes are in the world)
            (Some(local), true) => local,
            (Some(local), false) => origin.multiply(&local),
            (None, _) => origin,
        }
    };
    match object.nodes {
        Some(nodes) => (
            place(&nodes.current, object.current),
            place(&nodes.previous, object.previous),
        ),
        None => (place(rest, object.current), place(rest, object.previous)),
    }
}

/// The frame of a point at `position` turned by `rotation`.
pub(crate) fn frame(position: Vec3, rotation: Quat) -> NodeMatrix {
    NodeMatrix {
        forward: (rotation * Vec3::X).to_array(),
        left: (rotation * Vec3::Y).to_array(),
        up: (rotation * Vec3::Z).to_array(),
        position: position.to_array(),
        scale: 1.0,
    }
}

impl Lights {
    /// The lights `level`'s object definitions carry.
    pub(crate) fn new(level: &Level) -> Self {
        let mut carried = BTreeMap::new();
        for object in &level.objects {
            let lights: Vec<Attached> = object
                .attachments
                .iter()
                .filter_map(|attachment| {
                    let tag = attachment.tag.as_ref()?;
                    let light = level.lights.iter().position(|light| light.tag == *tag)?;
                    Some(Attached {
                        light,
                        marker: attachment.marker.clone(),
                        primary_scale: attachment.primary_scale,
                    })
                })
                .collect();
            if lights.is_empty() {
                continue;
            }
            let model = object
                .model
                .as_ref()
                .and_then(|model| level.model_index(model));
            carried.insert(object.tag.clone(), Carried { lights, model });
        }
        Self {
            carried,
            fading: Vec::new(),
            camera: crate::camera::CameraLevel::new(level),
            sources: crate::camera::CameraSources::default(),
        }
    }

    /// One tick: the effects' lights age, and the light parts effects
    /// made this tick (`fired`) begin to shine.
    pub(crate) fn tick(&mut self, level: &Level, fired: &[FiredPart]) {
        self.fading.retain_mut(|fading| {
            fading.age += 1;
            level
                .lights
                .get(fading.light)
                .is_some_and(|light| fading.age < ticks(light))
        });
        for part in fired {
            let Some(light) = level.lights.iter().position(|light| light.tag == part.tag) else {
                continue;
            };
            self.fading.push(Fading {
                light,
                position: part.position,
                rotation: part.rotation,
                age: 0,
                number: part.effect,
            });
        }
        // (the oldest go first when there are too many)
        let excess = self.fading.len().saturating_sub(MAX_LIGHTS);
        self.fading.drain(..excess);
    }

    /// The frame's lights: the objects' (the player's flashlight at the
    /// `camera` in first person) and the effects'.
    pub(crate) fn gather(
        &self,
        level: &Level,
        simulation: &mut Simulation,
        camera: &opence_sim::Transform,
        out: &mut Vec<LightSnapshot>,
    ) {
        out.clear();
        simulation.visit_lit(|object| {
            let Some(carried) = self.carried.get(object.tag) else {
                return;
            };
            let model = carried.model.and_then(|index| level.models.get(index));
            let place =
                |transform: &opence_sim::Transform| frame(transform.position, transform.rotation);
            let rest = match (object.nodes, &model) {
                (None, Some(model)) => default_pose(model),
                _ => Vec::new(),
            };
            for (at, attached) in carried.lights.iter().enumerate() {
                let Some(light) = level.lights.get(attached.light) else {
                    continue;
                };
                let intensity = match usize::try_from(attached.primary_scale - 1) {
                    Ok(index) if index < 4 => object.functions.map_or(0.0, |values| {
                        if values.active[index] {
                            values.outgoing[index]
                        } else {
                            0.0
                        }
                    }),
                    _ => 1.0,
                };
                let first_person =
                    object.viewer && light.flags & Light::FIRST_PERSON_FLASHLIGHT != 0;
                let (matrix, previous) = if first_person {
                    (place(camera), camera.position)
                } else {
                    let (current, earlier) = marker_frames(model, &object, &rest, &attached.marker);
                    (current, Vec3::from_array(earlier.position))
                };
                let source = (object.id.0 << 3 | (at as u32 & 7)) & !EFFECT_SOURCE;
                if let Some(mut shone) =
                    shine(light, attached.light, intensity, &matrix, previous, source)
                {
                    if first_person {
                        shone.flags |= LIGHT_FIRST_PERSON;
                    }
                    out.push(shone);
                }
            }
        });
        for fading in &self.fading {
            let Some(light) = level.lights.get(fading.light) else {
                continue;
            };
            let t = fading.age as f32 / ticks(light) as f32;
            let intensity = 1.0 - transition(light.falloff_function, t);
            let matrix = frame(fading.position, fading.rotation);
            let source = EFFECT_SOURCE | fading.number;
            out.extend(shine(
                light,
                fading.light,
                intensity,
                &matrix,
                fading.position,
                source,
            ));
        }
        out.truncate(MAX_LIGHTS);
    }
}

/// The cells the view is cut into across, down and in depth.
pub const CLUSTER_X: u32 = 16;
/// The cells down.
pub const CLUSTER_Y: u32 = 9;
/// The cells in depth, from `near` to `far` at equal ratios.
pub const CLUSTER_Z: u32 = 24;
/// The cells.
pub const CLUSTER_CELLS: usize = (CLUSTER_X * CLUSTER_Y * CLUSTER_Z) as usize;
/// The most light indices the cells hold together.
pub const MAX_CLUSTER_INDICES: usize = 1 << 16;

/// The view the lights are sorted into cells of.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClusterView {
    /// The camera's position.
    pub position: snapshot::Vec3,
    /// The tangent of half the horizontal field of view.
    pub tangent_x: f32,
    /// The camera's orientation (its x forward, y left, z up).
    pub rotation: snapshot::Quat,
    /// The tangent of half the vertical field of view.
    pub tangent_y: f32,
    /// The nearest depth.
    pub near: f32,
    /// The farthest depth.
    pub far: f32,
    /// (zero)
    pub reserved: f32,
}

/// The cell `(x, y, z)`'s index: across first, then down, then in depth.
pub fn cell_index(x: u32, y: u32, z: u32) -> usize {
    ((z * CLUSTER_Y + y) * CLUSTER_X + x) as usize
}

/// The cells, by index, that a sphere at `centre` (in the camera's space:
/// x forward, y left, z up) of `radius` may reach: the columns, rows and
/// depth slices it is not wholly outside of.
fn cells_reached(
    view: &ClusterView,
    centre: Vec3,
    radius: f32,
) -> Option<([u32; 2], [u32; 2], [u32; 2])> {
    if centre.x + radius < view.near || centre.x - radius > view.far {
        return None;
    }
    // (a plane through the eye at a tile's edge, `across` from the
    // centre's axis (-1 to 1): the sphere's distance in front of it)
    let inside = |offset: f32, edge: f32, tangent: f32, forward: f32| {
        let slope = edge * tangent;
        (offset - slope * forward) / libm::sqrtf(1.0 + slope * slope)
    };
    let range = |count: u32, offset: f32, tangent: f32| -> Option<[u32; 2]> {
        let mut first = None;
        let mut last = 0;
        for cell in 0..count {
            let low = -1.0 + 2.0 * cell as f32 / count as f32;
            let high = -1.0 + 2.0 * (cell + 1) as f32 / count as f32;
            if inside(offset, low, tangent, centre.x) >= -radius
                && -inside(offset, high, tangent, centre.x) >= -radius
            {
                first.get_or_insert(cell);
                last = cell;
            }
        }
        first.map(|first| [first, last])
    };
    // (the screen's right is the camera's -y; its top is +z)
    let columns = range(CLUSTER_X, -centre.y, view.tangent_x)?;
    let rows = range(CLUSTER_Y, -centre.z, view.tangent_y)?;
    let slice = |depth: f32| {
        if depth <= view.near {
            return 0;
        }
        let ratio = libm::logf(depth / view.near) / libm::logf(view.far / view.near);
        ((ratio * CLUSTER_Z as f32) as u32).min(CLUSTER_Z - 1)
    };
    let slices = [slice(centre.x - radius), slice(centre.x + radius)];
    Some((columns, rows, slices))
}

/// Sorts `lights` into the view's cells: each cell's first index and
/// count in `cells` (two numbers a cell), the lights' indices in
/// `indices`, as many as fit. Returns the indices written.
pub fn cluster(
    lights: &[LightSnapshot],
    view: &ClusterView,
    cells: &mut [u32],
    indices: &mut [u16],
) -> usize {
    let length = (2 * CLUSTER_CELLS).min(cells.len());
    let cells = &mut cells[..length];
    cells.fill(0);
    let rotation = Quat::from_xyzw(
        view.rotation.x,
        view.rotation.y,
        view.rotation.z,
        view.rotation.w,
    )
    .normalize();
    let eye = Vec3::new(view.position.x, view.position.y, view.position.z);
    let reached: Vec<_> = lights
        .iter()
        .map(|light| {
            let at = Vec3::new(light.position.x, light.position.y, light.position.z);
            let radius = light.radius.max(light.specular_radius);
            cells_reached(view, rotation.inverse() * (at - eye), radius)
        })
        .collect();
    let each = |visit: &mut dyn FnMut(usize, usize)| {
        for (index, reach) in reached.iter().enumerate() {
            let Some(([x0, x1], [y0, y1], [z0, z1])) = *reach else {
                continue;
            };
            for z in z0..=z1 {
                for y in y0..=y1 {
                    for x in x0..=x1 {
                        visit(cell_index(x, y, z), index);
                    }
                }
            }
        }
    };
    // (counted, then each cell's first index found, then filled)
    let mut counts = vec![0u32; CLUSTER_CELLS];
    each(&mut |cell, _| counts[cell] += 1);
    let mut next = vec![0usize; CLUSTER_CELLS];
    let mut total = 0usize;
    for (cell, &count) in counts.iter().enumerate() {
        next[cell] = total;
        let fits = (count as usize).min(indices.len().saturating_sub(total));
        if let Some(pair) = cells.get_mut(2 * cell..2 * cell + 2) {
            pair[0] = total as u32;
            pair[1] = fits as u32;
        }
        total += fits;
    }
    let mut filled = vec![0u32; CLUSTER_CELLS];
    each(&mut |cell, index| {
        let fits = cells.get(2 * cell + 1).copied().unwrap_or(0);
        if filled[cell] < fits {
            if let Some(slot) = indices.get_mut(next[cell] + filled[cell] as usize) {
                *slot = index as u16;
            }
            filled[cell] += 1;
        }
    });
    total
}

/// The `count` lights that cast shadows: those nearest `eye` (the ones it
/// is inside of first), the larger first at a distance.
pub fn shadowed(lights: &[LightSnapshot], eye: snapshot::Vec3, count: usize) -> Vec<usize> {
    let eye = Vec3::new(eye.x, eye.y, eye.z);
    let mut order: Vec<(f32, usize)> = lights
        .iter()
        .enumerate()
        .map(|(index, light)| {
            let at = Vec3::new(light.position.x, light.position.y, light.position.z);
            (
                (at.distance(eye) - light.radius).max(0.0) - light.radius * 1e-3,
                index,
            )
        })
        .collect();
    order.sort_by(|a, b| a.0.total_cmp(&b.0).then(a.1.cmp(&b.1)));
    order
        .into_iter()
        .take(count)
        .map(|(_, index)| index)
        .collect()
}

/// Sorts `count` lights at `lights` into `view`'s cells (see [`cluster`]):
/// `2 * CLUSTER_CELLS` numbers at `cells`, at most `capacity` indices at
/// `indices`. Returns the indices written.
///
/// # Safety
/// Each pointer must point at as many values as it is said to, or be
/// null with its count 0.
#[no_mangle]
pub unsafe extern "C" fn opence_cluster_lights(
    lights: *const LightSnapshot,
    count: u32,
    view: *const ClusterView,
    cells: *mut u32,
    indices: *mut u16,
    capacity: u32,
) -> u32 {
    // SAFETY: the caller passes valid pointers and counts (above).
    let (lights, view, cells, indices) = unsafe {
        let lights = if lights.is_null() {
            &[][..]
        } else {
            std::slice::from_raw_parts(lights, count as usize)
        };
        let Some(view) = view.as_ref() else {
            return 0;
        };
        if cells.is_null() || indices.is_null() {
            return 0;
        }
        (
            lights,
            view,
            std::slice::from_raw_parts_mut(cells, 2 * CLUSTER_CELLS),
            std::slice::from_raw_parts_mut(indices, capacity as usize),
        )
    };
    cluster(lights, view, cells, indices) as u32
}

/// Writes the indices of the `capacity` lights of the `count` at `lights`
/// that cast shadows (see [`shadowed`]) to `out`. Returns how many.
///
/// # Safety
/// `lights` must point at `count` lights and `out` at `capacity` numbers.
#[no_mangle]
pub unsafe extern "C" fn opence_shadowed_lights(
    lights: *const LightSnapshot,
    count: u32,
    eye: snapshot::Vec3,
    out: *mut u32,
    capacity: u32,
) -> u32 {
    if lights.is_null() || out.is_null() {
        return 0;
    }
    // SAFETY: the caller passes valid pointers and counts (above).
    let (lights, out) = unsafe {
        (
            std::slice::from_raw_parts(lights, count as usize),
            std::slice::from_raw_parts_mut(out, capacity as usize),
        )
    };
    let chosen = shadowed(lights, eye, out.len());
    for (slot, index) in out.iter_mut().zip(&chosen) {
        *slot = *index as u32;
    }
    chosen.len() as u32
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use opence_content::Group;

    pub(crate) fn light(flags: u32) -> Light {
        Light {
            tag: TagRef {
                group: Group(*b"ligh"),
                path: "light".into(),
            },
            flags,
            radius: 2.0,
            radius_modifier: [0.0, 0.0],
            falloff_angle: 0.0,
            cutoff_angle: std::f32::consts::PI,
            lens_flare_radius: 0.0,
            cosine_falloff_angle: 0.0,
            cosine_cutoff_angle: 0.0,
            specular_radius_multiplier: 0.0,
            sine_cutoff_angle: 0.0,
            color_flags: 0,
            color_lower: [1.0, 0.0, 0.0, 0.0],
            color_upper: [1.0, 1.0, 0.5, 0.25],
            gel: None,
            lens_flare: None,
            duration: 4.0,
            falloff_function: 0,
        }
    }

    fn at(x: f32, y: f32, z: f32, radius: f32) -> LightSnapshot {
        LightSnapshot {
            position: snapshot::Vec3 { x, y, z },
            radius,
            ..Default::default()
        }
    }

    #[test]
    fn colours_are_interpolated_directly_or_round_the_hues() {
        let close = |a: [f32; 3], b: [f32; 3]| (0..3).all(|i| (a[i] - b[i]).abs() < 1e-4);
        assert!(close(
            interpolate(0, [0.0, 0.0, 0.0], [1.0, 0.5, 0.25], 0.5),
            [0.5, 0.25, 0.125]
        ));
        for colour in [[1.0, 0.0, 0.0], [0.2, 0.6, 0.4], [0.3, 0.3, 0.9], [0.5; 3]] {
            assert!(close(rgb(hsv(colour)), colour), "{colour:?}");
        }
        // (red to green: through yellow the short way, blue the long)
        let red = [1.0, 0.0, 0.0];
        let green = [0.0, 1.0, 0.0];
        assert!(close(
            interpolate(Light::HSV, red, green, 0.5),
            [1.0, 1.0, 0.0]
        ));
        let long = interpolate(Light::HSV | Light::MORE_COLORS, red, green, 0.5);
        assert!(close(long, [0.0, 0.0, 1.0]), "{long:?}");
    }

    #[test]
    fn a_light_shines_by_its_intensity_radius_and_cone() {
        let matrix = frame(Vec3::new(1.0, 2.0, 3.0), Quat::IDENTITY);
        assert_eq!(shine(&light(0), 0, 1.0, &matrix, Vec3::ZERO, 0), None);
        assert_eq!(
            shine(&light(Light::DYNAMIC), 0, 0.0, &matrix, Vec3::ZERO, 0),
            None
        );
        let Some(point) = shine(&light(Light::DYNAMIC), 3, 1.0, &matrix, Vec3::ONE, 9) else {
            panic!("no light");
        };
        assert_eq!(point.radius, 2.0);
        assert_eq!(point.specular_radius, 2.0);
        assert_eq!(point.flags, 0);
        assert_eq!(point.cosine_cutoff, -1.0);
        assert_eq!((point.light, point.source), (3, 9));
        assert_eq!(point.position.y, 2.0);
        assert_eq!(point.previous.x, 1.0);
        assert!((point.color.x - 1.0).abs() < 1e-5 && point.color.y < 0.5);
        let mut spot = light(Light::DYNAMIC | Light::NO_SPECULAR);
        spot.cutoff_angle = 0.5;
        spot.falloff_angle = 0.25;
        spot.radius_modifier = [0.5, 1.5];
        spot.specular_radius_multiplier = 2.0;
        let Some(cone) = shine(&spot, 0, 0.5, &matrix, Vec3::ZERO, 0) else {
            panic!("no spot");
        };
        assert_eq!(cone.flags, LIGHT_SPOT | LIGHT_NO_SPECULAR);
        assert_eq!(cone.radius, 2.0);
        assert_eq!(cone.specular_radius, 4.0);
        assert!((cone.cosine_cutoff - libm::cosf(0.5)).abs() < 1e-6);
        assert!(cone.cosine_falloff > cone.cosine_cutoff);
    }

    fn view() -> ClusterView {
        ClusterView {
            tangent_x: 1.0,
            tangent_y: 0.5625,
            rotation: snapshot::Quat {
                w: 1.0,
                ..Default::default()
            },
            near: 0.1,
            far: 100.0,
            ..Default::default()
        }
    }

    fn lights_in(cells: &[u32], indices: &[u16], x: u32, y: u32, z: u32) -> Vec<u16> {
        let cell = cell_index(x, y, z);
        let (first, count) = (cells[2 * cell] as usize, cells[2 * cell + 1] as usize);
        indices[first..first + count].to_vec()
    }

    #[test]
    fn lights_are_sorted_into_the_cells_they_reach() {
        let mut cells = vec![u32::MAX; 2 * CLUSTER_CELLS];
        let mut indices = vec![0u16; 64];
        // (ahead in the middle; to the right and up; behind the eye)
        let lights = [
            at(10.0, 0.0, 0.0, 0.5),
            at(10.0, -8.0, 4.0, 0.5),
            at(-5.0, 0.0, 0.0, 1.0),
        ];
        let written = cluster(&lights, &view(), &mut cells, &mut indices);
        assert!(written > 0);
        let slice =
            |depth: f32| (libm::logf(depth / 0.1) / libm::logf(1000.0) * CLUSTER_Z as f32) as u32;
        let z = slice(10.0);
        assert!(lights_in(&cells, &indices, 7, 4, z).contains(&0));
        assert!(lights_in(&cells, &indices, 8, 4, z).contains(&0));
        assert!(!lights_in(&cells, &indices, 0, 0, z).contains(&0));
        assert!(!lights_in(&cells, &indices, 8, 4, slice(50.0)).contains(&0));
        // (the screen's right is the camera's -y, its top +z: the top rows
        // are the first; 0.8 across and 0.71 up is column 14, row 1)
        assert!(lights_in(&cells, &indices, 14, 1, z).contains(&1));
        assert!(!lights_in(&cells, &indices, 14, 7, z).contains(&1));
        assert!(!lights_in(&cells, &indices, 1, 1, z).contains(&1));
        assert!(indices[..written].iter().all(|&index| index != 2));
        // (a turned camera: facing +y, the first light is out of view)
        let mut turned = view();
        let quarter = Quat::from_rotation_z(std::f32::consts::FRAC_PI_2);
        turned.rotation = snapshot::Quat {
            x: quarter.x,
            y: quarter.y,
            z: quarter.z,
            w: quarter.w,
        };
        let written = cluster(&lights[..1], &turned, &mut cells, &mut indices);
        assert_eq!(written, 0);
        // (the indices that do not fit are left out, the counts with them)
        let many: Vec<_> = (0..40).map(|_| at(10.0, 0.0, 0.0, 50.0)).collect();
        let written = cluster(&many, &view(), &mut cells, &mut indices);
        assert_eq!(written, 64);
        let counted: u32 = cells.chunks(2).map(|pair| pair[1]).sum();
        assert_eq!(counted, 64);
    }

    #[test]
    fn the_nearest_lights_cast_shadows() {
        let lights = [
            at(10.0, 0.0, 0.0, 1.0),
            at(3.0, 0.0, 0.0, 1.0),
            at(20.0, 0.0, 0.0, 30.0),
        ];
        let eye = snapshot::Vec3::default();
        assert_eq!(shadowed(&lights, eye, 2), vec![2, 1]);
        assert_eq!(shadowed(&lights, eye, 5), vec![2, 1, 0]);
        let mut out = [u32::MAX; 2];
        // SAFETY: three lights, two numbers.
        let count = unsafe { opence_shadowed_lights(lights.as_ptr(), 3, eye, out.as_mut_ptr(), 2) };
        assert_eq!((count, out), (2, [2, 1]));
        // SAFETY: null pointers are refused.
        unsafe {
            assert_eq!(
                opence_shadowed_lights(std::ptr::null(), 0, eye, out.as_mut_ptr(), 2),
                0
            );
        }
        let mut cells = vec![0u32; 2 * CLUSTER_CELLS];
        let mut indices = [0u16; 8];
        let view = view();
        // SAFETY: the cells and indices are as long as they are said to be.
        let written = unsafe {
            opence_cluster_lights(
                lights.as_ptr(),
                3,
                &view,
                cells.as_mut_ptr(),
                indices.as_mut_ptr(),
                8,
            )
        };
        assert!(written > 0 && written <= 8);
        // SAFETY: null pointers are refused.
        let refused = unsafe {
            opence_cluster_lights(
                lights.as_ptr(),
                3,
                std::ptr::null(),
                cells.as_mut_ptr(),
                indices.as_mut_ptr(),
                8,
            )
        };
        assert_eq!(refused, 0);
    }

    #[test]
    fn an_effects_light_fades_over_its_duration() {
        let mut level = crate::debug_scene::tests::level();
        level.lights.push(light(Light::DYNAMIC));
        let mut lights = Lights::new(&level);
        lights.fading.push(Fading {
            light: 0,
            position: Vec3::new(1.0, 0.0, 0.0),
            rotation: Quat::IDENTITY,
            age: 0,
            number: 1,
        });
        let mut simulation = Simulation::empty();
        let camera = opence_sim::Transform::default();
        let mut out = Vec::new();
        let mut radii = Vec::new();
        for _ in 0..5 {
            lights.gather(&level, &mut simulation, &camera, &mut out);
            radii.extend(out.iter().map(|light| (light.source, light.color.x)));
            lights.tick(&level, &[]);
        }
        // (four ticks, dimmer each, the first at full)
        assert_eq!(radii.len(), 4, "{radii:?}");
        assert!(radii.iter().all(|&(source, _)| source == EFFECT_SOURCE | 1));
        assert!(
            radii.windows(2).all(|pair| pair[1].1 < pair[0].1),
            "{radii:?}"
        );
        assert!((radii[0].1 - 1.0).abs() < 1e-5);
        assert_eq!(
            ticks(&Light {
                duration: 0.0,
                ..light(0)
            }),
            1
        );
    }
}
