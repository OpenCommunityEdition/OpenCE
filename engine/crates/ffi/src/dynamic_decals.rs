//! Decals laid while the game runs: where projectiles strike and grenades
//! detonate, each effect's decals laid on the level or on the object
//! struck, fading at the end of their lives and recycled oldest first
//! (`decals.c`'s `decal_new_from_collision` and `decals_update`).

use std::collections::HashMap;

use opence_content::{Bitmap, Level, Projectile, TagRef};
use opence_sim::objects::ObjectHandle;
use opence_sim::{Simulation, Transform, Vec3};

use crate::decals::{first_hit, lay, seed, triangles, Laid, RenderDecal, Triangle};
use crate::snapshot::{
    DecalSnapshot, DECAL_NO_PICTURE, DECAL_ON_LEVEL, DECAL_ON_OBJECT, MAX_DECALS,
};

/// The game's ticks a second.
const TICKS_PER_SECOND: f32 = 30.0;
/// How far above a surface struck its material is looked for.
const MATERIAL_REACH: f32 = 0.25;
/// How far above a detonation the ground under it is looked for, and how
/// far below.
const GROUND_ABOVE: f32 = 0.5;
const GROUND_BELOW: f32 = 0.5;

/// A decal laid, with its age.
#[derive(Clone, Debug, PartialEq)]
struct Mark {
    /// The decal, in the world, or in the frame of the object it is on.
    decal: RenderDecal,
    /// The object it is on, if it is on one.
    on: Option<ObjectHandle>,
    /// The ticks it has been laid.
    age: u32,
    /// The ticks it lasts before it fades (with `decay` 0 too: until it is
    /// recycled).
    lifetime: u32,
    /// The ticks it fades over.
    decay: u32,
}

impl Mark {
    fn permanent(&self) -> bool {
        self.lifetime == 0 && self.decay == 0
    }

    /// Its opacity now, of its own.
    fn fade(&self) -> f32 {
        if self.permanent() || self.age <= self.lifetime {
            return 1.0;
        }
        let into = (self.age - self.lifetime) as f32;
        (1.0 - into / self.decay.max(1) as f32).clamp(0.0, 1.0)
    }

    fn gone(&self) -> bool {
        !self.permanent() && self.age >= self.lifetime + self.decay
    }
}

/// The images the level's decals draw, each once, in the order the
/// snapshot's decal pictures number them.
pub(crate) fn decal_images(level: &Level) -> Vec<(TagRef, u16)> {
    let mut images = Vec::new();
    let mut seen = Vec::new();
    for decal in &level.decals {
        let Some(bitmap) = decal.map.as_ref().and_then(|map| level.bitmap(map)) else {
            continue;
        };
        if seen.contains(&&bitmap.tag) {
            continue;
        }
        seen.push(&bitmap.tag);
        let count = u16::try_from(bitmap.images.len()).unwrap_or(u16::MAX);
        images.extend((0..count).map(|image| (bitmap.tag.clone(), image)));
    }
    images
}

/// The effect a projectile makes where it strikes a surface of
/// `material` (-1: one not known): its response's, else its detonation's.
pub(crate) fn response_effect(projectile: &Projectile, material: i16) -> Option<&TagRef> {
    let response = projectile
        .material_responses
        .get(usize::try_from(material).ok()?)?;
    response
        .default_effect
        .as_ref()
        .or(response.detonation_effect.as_ref())
}

/// The decals laid while the game runs.
#[derive(Debug, Default)]
pub(crate) struct DynamicDecals {
    marks: Vec<Mark>,
    /// The snapshot's picture of each decal image.
    pictures: HashMap<(TagRef, u16), u32>,
    /// Each BSP's triangles, found when a decal is first laid on it.
    triangles: Vec<Option<Vec<Triangle>>>,
}

impl DynamicDecals {
    pub(crate) fn new(level: &Level) -> Self {
        let pictures = decal_images(level)
            .into_iter()
            .enumerate()
            .map(|(index, image)| (image, u32::try_from(index).unwrap_or(DECAL_NO_PICTURE)))
            .collect();
        Self {
            marks: Vec::new(),
            pictures,
            triangles: Vec::new(),
        }
    }

    /// The BSP `bsp`'s triangles.
    fn bsp(&mut self, level: &Level, bsp: usize) -> &[Triangle] {
        if self.triangles.len() <= bsp {
            self.triangles.resize_with(bsp + 1, || None);
        }
        self.triangles[bsp].get_or_insert_with(|| {
            level
                .structure_bsps
                .get(bsp)
                .map_or_else(Vec::new, |structure| triangles(level, structure))
        })
    }

    /// One tick: the decals age, those past their lives go, and this
    /// tick's impacts and detonations lay theirs on BSP `bsp` and the
    /// objects they struck.
    pub(crate) fn tick(&mut self, level: &Level, simulation: &Simulation, bsp: usize) {
        self.age(|handle| simulation.transform(handle).is_some());
        for impact in simulation.impacts() {
            let Some(projectile) = level
                .object(&impact.projectile)
                .and_then(|object| object.projectile.as_ref())
            else {
                continue;
            };
            let point = impact.point.to_array();
            let normal = impact.normal.to_array();
            let on = impact
                .object
                .and_then(|handle| Some((handle, simulation.transform(handle)?)));
            let material = match (impact.object, &on) {
                (Some(handle), _) => simulation
                    .object(handle)
                    .and_then(|object| level.object(&object.tag)?.collision_model.clone())
                    .and_then(|model| level.collision_model(&model))
                    .and_then(|model| model.materials.first())
                    .map_or(-1, |material| material.material_type),
                (None, _) => {
                    let origin = add(point, scale(normal, MATERIAL_REACH));
                    first_hit(
                        self.bsp(level, bsp),
                        origin,
                        scale(normal, -1.0),
                        MATERIAL_REACH * 2.0,
                    )
                    .map_or(-1, |hit| hit.material)
                }
            };
            if let Some(effect) = response_effect(projectile, material) {
                let direction = impact.direction.to_array();
                self.lay_effect(level, effect, point, normal, direction, on);
            }
        }
        for detonation in simulation.detonations() {
            let Some(effect) = detonation.effect.as_ref() else {
                continue;
            };
            let point = detonation.point.to_array();
            let down = [0.0, 0.0, -1.0];
            let origin = add(point, [0.0, 0.0, GROUND_ABOVE]);
            let Some(hit) = first_hit(
                self.bsp(level, bsp),
                origin,
                down,
                GROUND_ABOVE + GROUND_BELOW,
            ) else {
                continue;
            };
            self.lay_effect(level, effect, hit.point, hit.normal, down, None);
        }
    }

    /// The decals age; those past their lives, and those on objects that
    /// are gone (by `exists`), are removed.
    fn age(&mut self, exists: impl Fn(ObjectHandle) -> bool) {
        self.marks.retain_mut(|mark| {
            mark.age = mark.age.saturating_add(1);
            !mark.gone() && mark.on.is_none_or(&exists)
        });
    }

    /// Lays `effect`'s decals at `point` on a surface facing `normal`,
    /// struck going `velocity`; on the object `on` (with its transform),
    /// if one was struck.
    pub(crate) fn lay_effect(
        &mut self,
        level: &Level,
        effect: &TagRef,
        point: [f32; 3],
        normal: [f32; 3],
        velocity: [f32; 3],
        on: Option<(ObjectHandle, Transform)>,
    ) {
        let Some(effect) = level.effect(effect) else {
            return;
        };
        let pictures = &self.pictures;
        let mut texture =
            |bitmap: &Bitmap, image: u16| pictures.get(&(bitmap.tag.clone(), image)).copied();
        let mut seed = seed(point);
        let mut laid = Vec::new();
        for decal in effect.decals.iter().filter_map(|tag| level.decal(tag)) {
            laid.extend(lay(
                level,
                decal,
                point,
                normal,
                velocity,
                &mut seed,
                &mut texture,
            ));
        }
        for Laid {
            mut decal,
            lifetime,
            decay,
        } in laid
        {
            if let Some((_, transform)) = &on {
                into_frame(&mut decal, transform);
            }
            self.marks.push(Mark {
                decal,
                on: on.as_ref().map(|(handle, _)| *handle),
                age: 0,
                lifetime: ticks(lifetime),
                decay: ticks(decay),
            });
        }
        // (the oldest are recycled when there are too many)
        let excess = self.marks.len().saturating_sub(MAX_DECALS);
        self.marks.drain(..excess);
    }

    /// The frame's decals, faded, those on objects placed where `transform`
    /// says their objects are; layer by layer, and by picture within each.
    pub(crate) fn gather(
        &self,
        transform: impl Fn(ObjectHandle) -> Option<Transform>,
        out: &mut Vec<DecalSnapshot>,
    ) {
        out.clear();
        for mark in &self.marks {
            let mut decal = mark.decal.clone();
            let surface = match mark.on {
                Some(handle) => {
                    let Some(transform) = transform(handle) else {
                        continue;
                    };
                    out_of_frame(&mut decal, &transform);
                    DECAL_ON_OBJECT
                }
                None => DECAL_ON_LEVEL,
            };
            decal.alpha *= mark.fade();
            out.push(snapshot(&decal, surface));
        }
        out.sort_by_key(|decal| (decal.modes[1], decal.modes[2]));
    }
}

/// The ticks in `seconds`.
fn ticks(seconds: f32) -> u32 {
    libm::roundf(seconds.max(0.0) * TICKS_PER_SECOND) as u32
}

fn add(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] + b[0], a[1] + b[1], a[2] + b[2]]
}

fn scale(a: [f32; 3], s: f32) -> [f32; 3] {
    [a[0] * s, a[1] * s, a[2] * s]
}

/// The decal's centre and axes, each through `map` (the centre by
/// `point`, the axes by `axis`).
fn remap(decal: &mut RenderDecal, point: impl Fn(Vec3) -> Vec3, axis: impl Fn(Vec3) -> Vec3) {
    let centre = point(Vec3::new(decal.x, decal.y, decal.z));
    let up = axis(Vec3::new(decal.up_x, decal.up_y, decal.up_z));
    let forward = axis(Vec3::new(decal.forward_x, decal.forward_y, decal.forward_z));
    let left = axis(Vec3::new(decal.left_x, decal.left_y, decal.left_z));
    [decal.x, decal.y, decal.z] = centre.to_array();
    [decal.up_x, decal.up_y, decal.up_z] = up.to_array();
    [decal.forward_x, decal.forward_y, decal.forward_z] = forward.to_array();
    [decal.left_x, decal.left_y, decal.left_z] = left.to_array();
}

/// The decal, from the world into the frame of an object at `transform`.
fn into_frame(decal: &mut RenderDecal, transform: &Transform) {
    let inverse = transform.rotation.inverse();
    let size = if transform.scale > 0.0 {
        transform.scale
    } else {
        1.0
    };
    remap(
        decal,
        |point| inverse * (point - transform.position) / size,
        |axis| inverse * axis,
    );
}

/// The decal, from the frame of an object at `transform` into the world.
fn out_of_frame(decal: &mut RenderDecal, transform: &Transform) {
    let size = if transform.scale > 0.0 {
        transform.scale
    } else {
        1.0
    };
    remap(
        decal,
        |point| transform.position + transform.rotation * (point * size),
        |axis| transform.rotation * axis,
    );
}

/// The decal as the snapshot gives it, laid on `surface`.
pub(crate) fn snapshot(decal: &RenderDecal, surface: u32) -> DecalSnapshot {
    DecalSnapshot {
        center: [decal.x, decal.y, decal.z, decal.depth],
        forward: [
            decal.forward_x,
            decal.forward_y,
            decal.forward_z,
            decal.forward_least,
        ],
        left: [decal.left_x, decal.left_y, decal.left_z, decal.left_least],
        up: [decal.up_x, decal.up_y, decal.up_z, 0.0],
        reach: [decal.forward_greatest, decal.left_greatest, 0.0, 0.0],
        uv: [decal.u0, decal.v0, decal.u1, decal.v1],
        color: [decal.red, decal.green, decal.blue, decal.alpha],
        modes: [decal.blend, decal.layer, decal.texture, surface],
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Decal, Effect, Group, MaterialResponse};
    use opence_sim::Quat;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn decal(path: &str, lifetime: f32, decay: f32) -> Decal {
        Decal {
            tag: tag(b"deca", path),
            flags: 0,
            kind: 0,
            layer: Decal::PRIMARY,
            next: None,
            radius: [0.25, 0.75],
            intensity: [1.0, 1.0],
            color: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
            animation_loop_frame: 0,
            animation_speed: 1,
            lifetime: [lifetime, lifetime],
            decay_time: [decay, decay],
            framebuffer_blend_function: 1,
            map: None,
            maximum_sprite_extent: 0.0,
        }
    }

    fn effect(path: &str, decals: &[&Decal]) -> Effect {
        Effect {
            tag: tag(b"effe", path),
            damage: Vec::new(),
            debris: Vec::new(),
            lights: Vec::new(),
            decals: decals.iter().map(|decal| decal.tag.clone()).collect(),
        }
    }

    /// A level whose effect `hole` lays a decal lasting a second and
    /// fading over another, and whose effect `chain` lays a decal and the
    /// one it chains to.
    fn level() -> Level {
        let mut level = crate::render_level::tests::level();
        let hole = decal("hole", 1.0, 1.0);
        let mut first = decal("first", 0.0, 0.0);
        first.next = Some(tag(b"deca", "second"));
        let mut second = decal("second", 0.0, 0.0);
        second.layer = Decal::SECONDARY;
        let mut pictured = decal("pictured", 0.0, 0.0);
        pictured.map = Some(tag(b"bitm", "decals"));
        level.bitmaps = vec![crate::render_level::tests::bitmap("decals", 3, 1)];
        level.effects = vec![
            effect("hole", &[&hole]),
            effect("chain", &[&first]),
            effect("pictured", &[&pictured]),
        ];
        level.decals = vec![hole, first, second, pictured];
        level
    }

    fn lay(decals: &mut DynamicDecals, level: &Level, effect: &str) {
        decals.lay_effect(
            level,
            &tag(b"effe", effect),
            [1.0, 2.0, 0.0],
            [0.0, 0.0, 1.0],
            [0.0, 0.0, -1.0],
            None,
        );
    }

    fn gathered(decals: &DynamicDecals) -> Vec<DecalSnapshot> {
        let mut out = Vec::new();
        decals.gather(|_| None, &mut out);
        out
    }

    #[test]
    fn a_decal_lasts_its_lifetime_then_fades_over_its_decay() {
        let level = level();
        let mut decals = DynamicDecals::new(&level);
        lay(&mut decals, &level, "hole");
        let laid = gathered(&decals);
        assert_eq!(laid.len(), 1);
        assert_eq!(laid[0].center[..3], [1.0, 2.0, 0.0]);
        assert_eq!(laid[0].color[3], 1.0);
        assert_eq!(
            laid[0].modes,
            [1, Decal::PRIMARY as u32, DECAL_NO_PICTURE, DECAL_ON_LEVEL]
        );
        // (its size is drawn from its range)
        let reach = laid[0].reach[0];
        assert!((0.25..=0.75).contains(&reach), "{reach}");
        for _ in 0..30 {
            decals.age(|_| true);
        }
        assert_eq!(gathered(&decals)[0].color[3], 1.0);
        for _ in 0..15 {
            decals.age(|_| true);
        }
        let half = gathered(&decals)[0].color[3];
        assert!((half - 0.5).abs() < 1e-4, "{half}");
        for _ in 0..15 {
            decals.age(|_| true);
        }
        assert!(gathered(&decals).is_empty(), "the decal outlived its decay");
    }

    #[test]
    fn a_decal_without_a_lifetime_stays_and_its_chain_follows_layer_by_layer() {
        let level = level();
        let mut decals = DynamicDecals::new(&level);
        lay(&mut decals, &level, "chain");
        lay(&mut decals, &level, "nothing");
        for _ in 0..1000 {
            decals.age(|_| true);
        }
        let laid = gathered(&decals);
        assert_eq!(laid.len(), 2);
        assert_eq!(laid[0].modes[1], Decal::PRIMARY as u32);
        assert_eq!(laid[1].modes[1], Decal::SECONDARY as u32);
    }

    #[test]
    fn the_oldest_decals_are_recycled_past_the_limit() {
        let level = level();
        let mut decals = DynamicDecals::new(&level);
        lay(&mut decals, &level, "hole");
        decals.age(|_| true);
        for _ in 0..MAX_DECALS {
            lay(&mut decals, &level, "chain");
        }
        assert_eq!(decals.marks.len(), MAX_DECALS);
        // (the hole, the oldest, went first)
        assert!(decals.marks.iter().all(|mark| mark.lifetime == 0));
    }

    #[test]
    fn a_pictured_decal_names_its_image_among_the_levels_decal_pictures() {
        let level = level();
        assert_eq!(
            decal_images(&level),
            (0..3)
                .map(|image| (tag(b"bitm", "decals"), image))
                .collect::<Vec<_>>()
        );
        let mut decals = DynamicDecals::new(&level);
        lay(&mut decals, &level, "pictured");
        let picture = gathered(&decals)[0].modes[2];
        assert!(picture < 3, "{picture}");
    }

    #[test]
    fn a_decal_on_an_object_moves_with_it_and_goes_with_it() {
        let level = level();
        let mut decals = DynamicDecals::new(&level);
        let handle = ObjectHandle { index: 4, salt: 1 };
        let at = Transform {
            position: Vec3::new(1.0, 2.0, 0.0),
            rotation: Quat::IDENTITY,
            scale: 1.0,
        };
        decals.lay_effect(
            &level,
            &tag(b"effe", "hole"),
            [1.0, 2.5, 0.0],
            [0.0, 1.0, 0.0],
            [0.0, -1.0, 0.0],
            Some((handle, at)),
        );
        let moved = Transform {
            position: Vec3::new(5.0, 0.0, 0.0),
            rotation: Quat::from_rotation_z(core::f32::consts::FRAC_PI_2),
            scale: 1.0,
        };
        let mut out = Vec::new();
        decals.gather(|_| Some(moved), &mut out);
        assert_eq!(out.len(), 1);
        let [x, y, z, _] = out[0].center;
        // (half a unit along the object's y, turned to its -x)
        assert!(
            (x - 4.5).abs() < 1e-4 && y.abs() < 1e-4 && z.abs() < 1e-4,
            "{x} {y} {z}"
        );
        assert!((out[0].up[0] + 1.0).abs() < 1e-4);
        assert_eq!(out[0].modes[3], DECAL_ON_OBJECT);
        decals.gather(|_| None, &mut out);
        assert!(out.is_empty());
        decals.age(|_| false);
        assert_eq!(decals.marks.len(), 0, "the decal outlived its object");
    }

    #[test]
    fn the_effect_struck_is_the_materials_response() {
        let response = |effect: Option<&str>, detonation: Option<&str>| MaterialResponse {
            default_effect: effect.map(|path| tag(b"effe", path)),
            detonation_effect: detonation.map(|path| tag(b"effe", path)),
            ..MaterialResponse::default()
        };
        let projectile = Projectile {
            material_responses: vec![
                response(Some("dirt"), None),
                response(None, Some("boom")),
                response(None, None),
            ],
            ..Projectile::default()
        };
        assert_eq!(response_effect(&projectile, 0), Some(&tag(b"effe", "dirt")));
        assert_eq!(response_effect(&projectile, 1), Some(&tag(b"effe", "boom")));
        assert_eq!(response_effect(&projectile, 2), None);
        assert_eq!(response_effect(&projectile, 3), None);
        assert_eq!(response_effect(&projectile, -1), None);
    }

    #[test]
    fn the_material_struck_is_read_from_the_level() {
        let mut level = level();
        let structure = &level.structure_bsps[0];
        let any = structure
            .lightmaps
            .iter()
            .flat_map(|page| &page.materials)
            .find(|material| material.shader.is_some())
            .and_then(|material| material.shader.clone());
        let Some(shader) = any else {
            return;
        };
        if let Some(found) = level.shaders.iter_mut().find(|s| s.tag == shader) {
            found.material_type = 7;
        }
        let mut decals = DynamicDecals::new(&level);
        let triangles = decals.bsp(&level, 0);
        assert!(triangles.iter().any(|triangle| triangle.material == 7));
        assert!(decals.bsp(&level, 9).is_empty());
    }
}
