//! What is drawn as sprites: the particles (`part`) effects' events emit,
//! the contrails (`cont`) objects carry, the weather (`rain`) a BSP's
//! clusters name and the detail objects (`dobc`) a BSP scatters.
//!
//! An event's particles (0xE8 bytes each) name their location at 0x08,
//! their direction (yaw, pitch) at 0x0C, their offset at 0x14, their
//! particle at 0x54, their flags at 0x64, their distribution at 0x68,
//! their count bounds at 0x6C, their distribution radius bounds at 0x70,
//! their velocity bounds at 0x84, its cone at 0x8C, their angular velocity
//! bounds at 0x90, their radius bounds at 0xA0 and their tints (alpha,
//! red, green, blue) at 0xB0 and 0xC0.
//!
//! A particle (0x164 bytes): its flags at 0x00, its bitmap at 0x04, its
//! point physics at 0x14, its lifespan bounds at 0x38, its fades at 0x40
//! and 0x44, its radius at birth and death at 0x74, its frames a second at
//! 0x80, its sequences at 0x98 (first, initial, looping, final), its
//! orientation at 0xAC and its shader's flags, blending and fading at
//! 0xD8.
//!
//! A contrail (0x144 bytes): its flags at 0x00, its point rate at 0x04,
//! its velocity bounds at 0x08, its cone at 0x10, its inherited velocity
//! at 0x14, its render type at 0x18, its texture repeats at 0x1C and
//! animation at 0x24, its bitmap at 0x30, its sequences at 0x40, its
//! shader at 0xAC and its point states (0x68 bytes each) a block at 0x138:
//! duration bounds at 0x00, transition bounds at 0x08, width at 0x40 and
//! colours at 0x44 and 0x54.
//!
//! Weather (0x30 bytes): its particle types (0x25C bytes each) a block at
//! 0x24. A type's flags at 0x00 of the type's data at 0x20, its fade
//! distances at 0x24, its density bounds at 0xA4, its point physics at
//! 0xAC, its acceleration at 0xCC, its radius at 0xFC, its animation rate
//! at 0x104, its rotation rate at 0x10C, its colours at 0x134 and 0x144,
//! its bitmap at 0x194, its render mode at 0x1A4 and its shader at 0x1D0.
//!
//! A point physics (0x40 bytes): its flags at 0x00, the mass and
//! buoyancies the tools work out at 0x04, its density at 0x20 and its
//! frictions at 0x24 and 0x28.

use std::collections::BTreeSet;

use opence_content::{
    Contrail, ContrailState, DetailObject, DetailObjectCollection, DetailObjectType, Effect,
    EffectParticles, ObjectDefinition, Particle, PointPhysics, SpriteShader, StructureBsp, TagRef,
    WeatherPalette, WeatherParticleType, WeatherSystem,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_index, tag_ref, Reached};
use crate::object::vector;
use crate::tags::{Element, GroupTag};

pub(crate) const PART: GroupTag = GroupTag::new(b"part");
pub(crate) const PARTICLE_SIZE: usize = 0x164;
pub(crate) const PPHY: GroupTag = GroupTag::new(b"pphy");
pub(crate) const POINT_PHYSICS_SIZE: usize = 0x40;
pub(crate) const CONT: GroupTag = GroupTag::new(b"cont");
pub(crate) const CONTRAIL_SIZE: usize = 0x144;
const CONTRAIL_STATES_OFFSET: usize = 0x138;
pub(crate) const CONTRAIL_STATE_SIZE: usize = 0x68;
const MAX_CONTRAIL_STATES: usize = 16;
pub(crate) const RAIN: GroupTag = GroupTag::new(b"rain");
pub(crate) const WEATHER_SIZE: usize = 0x30;
const WEATHER_TYPES_OFFSET: usize = 0x24;
pub(crate) const WEATHER_TYPE_SIZE: usize = 0x25C;
const MAX_WEATHER_TYPES: usize = 8;
pub(crate) const DOBC: GroupTag = GroupTag::new(b"dobc");
pub(crate) const COLLECTION_SIZE: usize = 0x80;
const COLLECTION_TYPES_OFFSET: usize = 0x44;
pub(crate) const COLLECTION_TYPE_SIZE: usize = 0x60;
const MAX_COLLECTION_TYPES: usize = 16;
/// The scenario's detail object collections
/// (`scenario.detail_object_collection_palette`).
pub(crate) const SCENARIO_DETAIL_PALETTE_OFFSET: usize = 0x3C0;
pub(crate) const DETAIL_PALETTE_SIZE: usize = 0x30;
const MAX_DETAIL_PALETTE: usize = 32;
/// A BSP's weather palette (`structure_bsp.weather_palette`).
pub(crate) const BSP_WEATHER_PALETTE_OFFSET: usize = 0x1B4;
pub(crate) const BSP_WEATHER_PALETTE_SIZE: usize = 0xF0;
const MAX_BSP_WEATHER_PALETTE: usize = 32;
/// A BSP's detail objects (`structure_bsp.detail_objects`, one element).
pub(crate) const BSP_DETAIL_OBJECTS_OFFSET: usize = 0x24C;
pub(crate) const BSP_DETAIL_OBJECTS_SIZE: usize = 0x40;
pub(crate) const DETAIL_CELL_SIZE: usize = 0x20;
pub(crate) const DETAIL_INSTANCE_SIZE: usize = 6;
pub(crate) const DETAIL_COUNT_SIZE: usize = 2;
pub(crate) const DETAIL_Z_REFERENCE_SIZE: usize = 0x10;
const MAX_DETAIL_CELLS: usize = 262_144;
const MAX_DETAIL_INSTANCES: usize = 2_097_152;
const MAX_DETAIL_COUNTS: usize = 8_388_608;
/// A detail object cell's side, world units.
const DETAIL_CELL_SIDE: f32 = 8.0;
/// Grams a cubic world unit in a gram a millilitre (the tools' factor).
const DENSITY_SCALE: f32 = 118_613.34;
/// Air's density, grams a millilitre.
const AIR_DENSITY: f32 = 0.0011;

fn argb(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 4], ImportError> {
    vector::<4>(element, at, field)
}

/// A sprite shader whose flags are at `at`, its blending and fading after.
fn sprite_shader(element: &Element<'_>, at: usize) -> Result<SpriteShader, ImportError> {
    Ok(SpriteShader {
        flags: element.u16(at, "shader flags")?,
        framebuffer_blend_function: element.i16(at + 2, "framebuffer blend function")?,
        fade_mode: element.i16(at + 4, "framebuffer fade mode")?,
    })
}

/// The point physics a reference at `at` names. A tag the tools left
/// unworked (no mass) is worked out from its density as they would.
fn point_physics(
    cache: &Cache,
    element: &Element<'_>,
    at: usize,
) -> Result<Option<PointPhysics>, ImportError> {
    let Some(index) = element.reference(at, "physics", &[PPHY])? else {
        return Ok(None);
    };
    let physics = cache.tag(index, PPHY, POINT_PHYSICS_SIZE)?;
    let mut read = PointPhysics {
        flags: physics.u32(0x00, "flags")?,
        mass_over_radius_cubed: physics.f32(0x04, "mass over radius cubed")?,
        water_buoyancy: physics.f32(0x08, "water gravity scale")?,
        air_buoyancy: physics.f32(0x0C, "air gravity scale")?,
        density: physics.f32(0x20, "density")?,
        air_friction: physics.f32(0x24, "air friction")?,
        water_friction: physics.f32(0x28, "water friction")?,
    };
    if read.mass_over_radius_cubed == 0.0 && read.density > 0.0 {
        read.mass_over_radius_cubed = read.density * DENSITY_SCALE;
        read.air_buoyancy = AIR_DENSITY / read.density - 1.0;
        read.water_buoyancy = 1.0 / read.density - 1.0;
    }
    Ok(Some(read))
}

/// An event's particles.
pub(crate) fn effect_particles(event: &Element<'_>) -> Result<Vec<EffectParticles>, ImportError> {
    let block = event.block(
        crate::effect::PARTICLES_OFFSET,
        "particles",
        crate::effect::PARTICLES_SIZE,
        crate::effect::MAX_PARTICLES,
    )?;
    let mut particles = Vec::with_capacity(block.len());
    for element in block.iter() {
        let Some(index) = element.reference(0x54, "particle type", &[PART])? else {
            continue;
        };
        particles.push(EffectParticles {
            particle: tag_ref(element.cache(), index),
            location: element.i16(0x08, "location")?,
            flags: element.u32(0x64, "flags")?,
            distribution: element.i16(0x68, "distribution function")?,
            count: [element.i16(0x6C, "count")?, element.i16(0x6E, "count")?],
            distribution_radius: vector(&element, 0x70, "distribution radius")?,
            direction: vector(&element, 0x0C, "relative direction")?,
            offset: vector(&element, 0x14, "relative offset")?,
            velocity: vector(&element, 0x84, "velocity")?,
            velocity_cone_angle: element.f32(0x8C, "velocity cone angle")?,
            angular_velocity: vector(&element, 0x90, "angular velocity")?,
            radius: vector(&element, 0xA0, "radius")?,
            tint: [
                argb(&element, 0xB0, "tint lower bound")?,
                argb(&element, 0xC0, "tint upper bound")?,
            ],
        });
    }
    Ok(particles)
}

/// A particle tag, its bitmap reached.
fn import_particle(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<Particle, ImportError> {
    let tag = cache.tag(index, PART, PARTICLE_SIZE)?;
    Ok(Particle {
        tag: tag_ref(cache, index),
        flags: tag.u32(0x00, "flags")?,
        bitmap: reached.bitmap(&tag, 0x04, "bitmap")?,
        physics: point_physics(cache, &tag, 0x14)?,
        lifespan: vector(&tag, 0x38, "lifespan")?,
        fade_in: tag.f32(0x40, "fade in time")?,
        fade_out: tag.f32(0x44, "fade out time")?,
        radius: vector(&tag, 0x74, "radius")?,
        fps: vector(&tag, 0x80, "animation rate")?,
        first_sequence: tag.i16(0x98, "first sequence index")?,
        initial_sequences: tag.i16(0x9A, "initial sequence count")?,
        looping_sequences: tag.i16(0x9C, "looping sequence count")?,
        final_sequences: tag.i16(0x9E, "final sequence count")?,
        orientation: tag.i16(0xAC, "orientation")?,
        shader: sprite_shader(&tag, 0xD8)?,
    })
}

/// Every particle the effects' events emit, each once, by path.
pub(crate) fn import_particles(
    cache: &Cache,
    effects: &[Effect],
    reached: &mut Reached,
) -> Result<Vec<Particle>, ImportError> {
    let tags: BTreeSet<&TagRef> = effects
        .iter()
        .flat_map(|effect| &effect.events)
        .flat_map(|event| &event.particles)
        .map(|particles| &particles.particle)
        .collect();
    let mut particles = Vec::with_capacity(tags.len());
    for tag in tags {
        if let Some(index) = tag_index(cache, tag) {
            particles.push(import_particle(cache, index, reached)?);
        }
    }
    Ok(particles)
}

/// A contrail tag, its bitmap reached.
fn import_contrail(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<Contrail, ImportError> {
    let tag = cache.tag(index, CONT, CONTRAIL_SIZE)?;
    let block = tag.block(
        CONTRAIL_STATES_OFFSET,
        "point states",
        CONTRAIL_STATE_SIZE,
        MAX_CONTRAIL_STATES,
    )?;
    let mut states = Vec::with_capacity(block.len());
    for state in block.iter() {
        states.push(ContrailState {
            duration: vector(&state, 0x00, "duration")?,
            transition: vector(&state, 0x08, "transition duration")?,
            width: state.f32(0x40, "width")?,
            color: [
                argb(&state, 0x44, "color lower bound")?,
                argb(&state, 0x54, "color upper bound")?,
            ],
        });
    }
    Ok(Contrail {
        tag: tag_ref(cache, index),
        flags: tag.u16(0x00, "flags")?,
        point_generation_rate: tag.f32(0x04, "point generation rate")?,
        velocity: vector(&tag, 0x08, "point velocity")?,
        velocity_cone_angle: tag.f32(0x10, "point velocity cone angle")?,
        inherited_velocity_fraction: tag.f32(0x14, "inherited velocity fraction")?,
        render_type: tag.i16(0x18, "render type")?,
        texture_repeats: vector(&tag, 0x1C, "texture repeats")?,
        texture_animation: vector(&tag, 0x24, "texture animation")?,
        bitmap: reached.bitmap(&tag, 0x30, "bitmap")?,
        sequences: [
            tag.i16(0x40, "sequence index")?,
            tag.i16(0x42, "sequence count")?,
        ],
        shader: sprite_shader(&tag, 0xAC)?,
        states,
    })
}

/// Every contrail the objects carry, each once, by path.
pub(crate) fn import_contrails(
    cache: &Cache,
    objects: &[ObjectDefinition],
    reached: &mut Reached,
) -> Result<Vec<Contrail>, ImportError> {
    let tags: BTreeSet<&TagRef> = objects
        .iter()
        .flat_map(|object| &object.attachments)
        .filter_map(|attachment| attachment.tag.as_ref())
        .filter(|tag| tag.group.0 == *b"cont")
        .collect();
    let mut contrails = Vec::with_capacity(tags.len());
    for tag in tags {
        if let Some(index) = tag_index(cache, tag) {
            contrails.push(import_contrail(cache, index, reached)?);
        }
    }
    Ok(contrails)
}

/// A BSP's weather palette.
pub(crate) fn weather_palette(bsp: &Element<'_>) -> Result<Vec<WeatherPalette>, ImportError> {
    let block = bsp.block(
        BSP_WEATHER_PALETTE_OFFSET,
        "weather palette",
        BSP_WEATHER_PALETTE_SIZE,
        MAX_BSP_WEATHER_PALETTE,
    )?;
    let mut palette = Vec::with_capacity(block.len());
    for element in block.iter() {
        let direction = vector::<3>(&element, 0x90, "wind direction")?;
        palette.push(WeatherPalette {
            system: crate::object::reference(&element, 0x20, "particle system", &[RAIN])?,
            wind: [
                direction[0],
                direction[1],
                direction[2],
                element.f32(0x9C, "wind magnitude")?,
            ],
        });
    }
    Ok(palette)
}

/// A colour of five, six and five bits, in eight bits each.
fn rgb565(color: u16) -> [u8; 3] {
    let red = (color >> 11) as u8 & 0x1F;
    let green = (color >> 5) as u8 & 0x3F;
    let blue = color as u8 & 0x1F;
    [
        (red << 3) | (red >> 2),
        (green << 2) | (green >> 4),
        (blue << 3) | (blue >> 2),
    ]
}

/// A BSP's detail objects, where they stand. Each cell (8 world units a
/// side) lists the layers it has, one bit each, in order; each layer's
/// objects follow the last's, counted by the counts from the cell's, and
/// lifted by the z reference of the same index.
pub(crate) fn detail_objects(bsp: &Element<'_>) -> Result<Vec<DetailObject>, ImportError> {
    let data = bsp.block(
        BSP_DETAIL_OBJECTS_OFFSET,
        "detail objects",
        BSP_DETAIL_OBJECTS_SIZE,
        1,
    )?;
    let Some(data) = data.get(0) else {
        return Ok(Vec::new());
    };
    let cells = data.block(0x00, "cells", DETAIL_CELL_SIZE, MAX_DETAIL_CELLS)?;
    let instances = data.block(
        0x0C,
        "instances",
        DETAIL_INSTANCE_SIZE,
        MAX_DETAIL_INSTANCES,
    )?;
    let counts = data.block(0x18, "counts", DETAIL_COUNT_SIZE, MAX_DETAIL_COUNTS)?;
    let z_references = data.block(
        0x24,
        "z reference vectors",
        DETAIL_Z_REFERENCE_SIZE,
        MAX_DETAIL_COUNTS,
    )?;
    let mut objects = Vec::with_capacity(instances.len());
    for cell in cells.iter() {
        let origin = [
            f32::from(cell.i16(0x00, "cell x")?),
            f32::from(cell.i16(0x02, "cell y")?),
            f32::from(cell.i16(0x04, "cell z")?) + f32::from(cell.i16(0x06, "offset z")?) / 255.0,
        ];
        let layers = cell.u32(0x08, "valid layers flags")?;
        let start = cell.i32(0x0C, "start index")?;
        let count_index = cell.i32(0x10, "count index")?;
        let (Ok(mut next), Ok(count_index)) =
            (usize::try_from(start), usize::try_from(count_index))
        else {
            return Err(cell.fault("start index", "is negative"));
        };
        for (ordinal, layer) in (0..32u8).filter(|bit| layers & (1 << bit) != 0).enumerate() {
            let index = count_index + ordinal;
            let count = counts
                .get(index)
                .ok_or_else(|| cell.fault("count index", "is past the counts"))?
                .u16(0, "count")?;
            let z_reference = match z_references.get(index) {
                Some(z) => vector::<4>(&z, 0, "z reference vector")?,
                None => [0.0, 0.0, 1.0, 0.0],
            };
            for _ in 0..count {
                let instance = instances
                    .get(next)
                    .ok_or_else(|| cell.fault("count", "is past the instances"))?;
                next += 1;
                let point = [
                    f32::from(instance.u8(0, "position")?),
                    f32::from(instance.u8(1, "position")?),
                    f32::from(instance.u8(2, "position")?),
                ];
                let lift = (point[0] * z_reference[0]
                    + point[1] * z_reference[1]
                    + point[2] * z_reference[2])
                    / 255.0
                    + z_reference[3];
                objects.push(DetailObject {
                    position: [
                        (origin[0] + point[0] / 255.0) * DETAIL_CELL_SIDE,
                        (origin[1] + point[1] / 255.0) * DETAIL_CELL_SIDE,
                        (origin[2] + lift) * DETAIL_CELL_SIDE,
                    ],
                    color: rgb565(instance.u16(4, "color")?),
                    collection: layer,
                    data: instance.u8(3, "data")?,
                });
            }
        }
    }
    Ok(objects)
}

/// A weather tag, its bitmaps reached.
fn import_weather_system(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<WeatherSystem, ImportError> {
    let tag = cache.tag(index, RAIN, WEATHER_SIZE)?;
    let block = tag.block(
        WEATHER_TYPES_OFFSET,
        "particle types",
        WEATHER_TYPE_SIZE,
        MAX_WEATHER_TYPES,
    )?;
    let mut types = Vec::with_capacity(block.len());
    for kind in block.iter() {
        types.push(WeatherParticleType {
            flags: kind.u32(0x20, "flags")?,
            distance_fade: vector(&kind, 0x24, "fade distances")?,
            density: vector(&kind, 0xA4, "particle count")?,
            physics: point_physics(cache, &kind, 0xAC)?,
            acceleration: vector(&kind, 0xCC, "acceleration")?,
            radius: vector(&kind, 0xFC, "particle radius")?,
            animation_rate: vector(&kind, 0x104, "animation rate")?,
            rotation_rate: vector(&kind, 0x10C, "rotation rate")?,
            color: [
                argb(&kind, 0x134, "color lower bound")?,
                argb(&kind, 0x144, "color upper bound")?,
            ],
            bitmap: reached.bitmap(&kind, 0x194, "sprite bitmap")?,
            render_mode: kind.i16(0x1A4, "render mode")?,
            shader: sprite_shader(&kind, 0x1D0)?,
        });
    }
    Ok(WeatherSystem {
        tag: tag_ref(cache, index),
        types,
    })
}

/// Every weather the BSPs' palettes name, each once, by path.
pub(crate) fn import_weather(
    cache: &Cache,
    structure_bsps: &[StructureBsp],
    reached: &mut Reached,
) -> Result<Vec<WeatherSystem>, ImportError> {
    let tags: BTreeSet<&TagRef> = structure_bsps
        .iter()
        .flat_map(|bsp| &bsp.weather_palette)
        .filter_map(|entry| entry.system.as_ref())
        .collect();
    let mut weather = Vec::with_capacity(tags.len());
    for tag in tags {
        if let Some(index) = tag_index(cache, tag) {
            weather.push(import_weather_system(cache, index, reached)?);
        }
    }
    Ok(weather)
}

/// A detail object collection, its bitmap reached.
fn import_collection(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<DetailObjectCollection, ImportError> {
    let tag = cache.tag(index, DOBC, COLLECTION_SIZE)?;
    let block = tag.block(
        COLLECTION_TYPES_OFFSET,
        "types",
        COLLECTION_TYPE_SIZE,
        MAX_COLLECTION_TYPES,
    )?;
    let mut types = Vec::with_capacity(block.len());
    for kind in block.iter() {
        types.push(DetailObjectType {
            sequence: kind.u8(0x20, "sequence index")?,
            sprites: [
                kind.u8(0x22, "first sprite index")?,
                kind.u8(0x23, "sprite count")?,
            ],
            color_override: kind.f32(0x24, "color override factor")?,
            fade: vector(&kind, 0x30, "fade distance")?,
            size: vector(&kind, 0x38, "size")?,
        });
    }
    Ok(DetailObjectCollection {
        tag: tag_ref(cache, index),
        kind: tag.i16(0x00, "collection type")?,
        bitmap: reached.bitmap(&tag, 0x34, "sprite plate")?,
        types,
    })
}

/// The scenario's detail object collections, by their palette's order
/// (the BSPs' layers index it); none where a BSP places no detail object.
pub(crate) fn import_detail_object_collections(
    cache: &Cache,
    scenario: &Element<'_>,
    structure_bsps: &[StructureBsp],
    reached: &mut Reached,
) -> Result<Vec<Option<DetailObjectCollection>>, ImportError> {
    if structure_bsps
        .iter()
        .all(|bsp| bsp.detail_objects.is_empty())
    {
        return Ok(Vec::new());
    }
    let block = scenario.block(
        SCENARIO_DETAIL_PALETTE_OFFSET,
        "detail object collection palette",
        DETAIL_PALETTE_SIZE,
        MAX_DETAIL_PALETTE,
    )?;
    let mut collections = Vec::with_capacity(block.len());
    for element in block.iter() {
        collections.push(match element.reference(0, "reference", &[DOBC])? {
            Some(index) => Some(import_collection(cache, index, reached)?),
            None => None,
        });
    }
    Ok(collections)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, empty, put, reference};
    use opence_content::object::Attachment;
    use opence_content::ObjectKind;
    use std::error::Error;

    fn floats(bytes: &mut [u8], at: usize, values: &[f32]) {
        for (index, value) in values.iter().enumerate() {
            put(bytes, at + index * 4, &value.to_le_bytes());
        }
    }

    #[test]
    fn colours_of_five_and_six_bits_are_widened() {
        assert_eq!(rgb565(0xFFFF), [255, 255, 255]);
        assert_eq!(rgb565(0), [0, 0, 0]);
        assert_eq!(rgb565(0xF800), [255, 0, 0]);
        assert_eq!(rgb565(0x07E0), [0, 255, 0]);
        assert_eq!(rgb565(0x8410), [132, 130, 132]);
    }

    /// A particle with point physics the tools did not work out, an effect
    /// whose one event emits it, and a contrail with one state.
    #[test]
    fn effects_particles_and_contrails_are_read() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let mut physics = [0u8; POINT_PHYSICS_SIZE];
        put(&mut physics, 0x00, &PointPhysics::NO_GRAVITY.to_le_bytes());
        floats(&mut physics, 0x20, &[0.5, 0.25, 0.75]);
        let physics = fake.add_tag(PPHY, "effects\\point physics\\smoke", &physics);
        let mut particle = [0u8; PARTICLE_SIZE];
        put(&mut particle, 0x00, &4u32.to_le_bytes());
        empty(&mut particle, 0x04);
        reference(&mut particle, 0x14, PPHY, fake.handle(physics));
        floats(&mut particle, 0x38, &[1.0, 2.0, 0.25, 0.5]);
        floats(&mut particle, 0x74, &[0.1, 0.4, 0.0, 10.0, 20.0]);
        put(&mut particle, 0x98, &[2, 0, 3, 0, 1, 0, 1, 0]);
        put(&mut particle, 0xAC, &1i16.to_le_bytes());
        put(&mut particle, 0xD8, &[1, 0, 3, 0, 2, 0]);
        let particle = fake.add_tag(PART, "effects\\particles\\smoke", &particle);

        let mut emitted = [0u8; crate::effect::PARTICLES_SIZE];
        put(&mut emitted, 0x08, &1i16.to_le_bytes());
        floats(&mut emitted, 0x0C, &[0.5, -0.5, 1.0, 2.0, 3.0]);
        reference(&mut emitted, 0x54, PART, fake.handle(particle));
        put(
            &mut emitted,
            0x64,
            &EffectParticles::STAY_ATTACHED.to_le_bytes(),
        );
        put(&mut emitted, 0x68, &EffectParticles::BUILDUP.to_le_bytes());
        put(&mut emitted, 0x6C, &[3, 0, 5, 0]);
        floats(&mut emitted, 0x70, &[0.0, 0.5]);
        floats(&mut emitted, 0x84, &[1.0, 2.0, 0.3, 4.0, 5.0]);
        floats(&mut emitted, 0xA0, &[1.0, 1.5]);
        floats(
            &mut emitted,
            0xB0,
            &[1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 1.0],
        );
        let mut unnamed = [0u8; crate::effect::PARTICLES_SIZE];
        empty(&mut unnamed, 0x54);
        let mut both = emitted.to_vec();
        both.extend_from_slice(&unnamed);
        let both = fake.add_data(&both);
        let mut event = [0u8; crate::effect::EVENT_SIZE];
        block(&mut event, crate::effect::PARTICLES_OFFSET, 2, both);
        let event = fake.add_data(&event);
        let mut effect = [0u8; crate::effect::EFFECT_SIZE];
        block(&mut effect, 0x34, 1, event);
        let effect = fake.add_tag(crate::effect::EFFE, "effects\\smoke", &effect);

        let mut state = [0u8; CONTRAIL_STATE_SIZE];
        floats(&mut state, 0x00, &[1.0, 2.0, 0.5, 0.5]);
        floats(
            &mut state,
            0x40,
            &[0.2, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0],
        );
        let state = fake.add_data(&state);
        let mut contrail = [0u8; CONTRAIL_SIZE];
        floats(&mut contrail, 0x04, &[30.0, 1.0, 2.0, 0.1, 0.5]);
        put(&mut contrail, 0x18, &2i16.to_le_bytes());
        floats(&mut contrail, 0x1C, &[1.0, 4.0, 0.0, 2.0]);
        empty(&mut contrail, 0x30);
        put(&mut contrail, 0x40, &[1, 0, 2, 0]);
        put(&mut contrail, 0xAC, &[0, 0, 3, 0, 1, 0]);
        block(&mut contrail, CONTRAIL_STATES_OFFSET, 1, state);
        let contrail = fake.add_tag(CONT, "effects\\contrails\\trail", &contrail);
        let cache = Cache::open(fake.build())?;

        let read = crate::effect::import_effect(&cache, effect)?;
        let emits = &read.events[0].particles;
        assert_eq!(emits.len(), 1, "the unnamed particles are left out");
        assert_eq!(emits[0].particle.path, "effects\\particles\\smoke");
        assert_eq!(emits[0].location, 1);
        assert_eq!(emits[0].direction, [0.5, -0.5]);
        assert_eq!(emits[0].offset, [1.0, 2.0, 3.0]);
        assert_eq!(emits[0].distribution, EffectParticles::BUILDUP);
        assert_eq!(emits[0].count, [3, 5]);
        assert_eq!(emits[0].velocity, [1.0, 2.0]);
        assert_eq!(emits[0].velocity_cone_angle, 0.3);
        assert_eq!(emits[0].angular_velocity, [4.0, 5.0]);
        assert_eq!(emits[0].radius, [1.0, 1.5]);
        assert_eq!(emits[0].tint[0], [1.0, 0.5, 0.5, 0.5]);

        let mut reached = Reached::default();
        let particles = import_particles(&cache, &[read.clone(), read], &mut reached)?;
        assert_eq!(particles.len(), 1, "each particle once");
        let smoke = &particles[0];
        assert_eq!(smoke.flags, 4);
        assert_eq!(smoke.lifespan, [1.0, 2.0]);
        assert_eq!((smoke.fade_in, smoke.fade_out), (0.25, 0.5));
        assert_eq!(smoke.radius, [0.1, 0.4]);
        assert_eq!(smoke.fps, [10.0, 20.0]);
        assert_eq!(
            [
                smoke.first_sequence,
                smoke.initial_sequences,
                smoke.looping_sequences,
                smoke.final_sequences
            ],
            [2, 3, 1, 1]
        );
        assert_eq!(smoke.orientation, 1);
        assert_eq!(smoke.shader.framebuffer_blend_function, 3);
        assert_eq!(smoke.shader.fade_mode, 2);
        let Some(physics) = smoke.physics else {
            panic!("the smoke has no physics");
        };
        assert_eq!(physics.flags, PointPhysics::NO_GRAVITY);
        assert_eq!(physics.mass_over_radius_cubed, 0.5 * DENSITY_SCALE);
        assert_eq!(physics.air_buoyancy, AIR_DENSITY / 0.5 - 1.0);
        assert_eq!((physics.air_friction, physics.water_friction), (0.25, 0.75));

        let carrier = ObjectDefinition {
            attachments: vec![
                Attachment {
                    tag: Some(tag_ref(&cache, contrail)),
                    marker: "trail".into(),
                    primary_scale: 0,
                    change_color: 0,
                },
                Attachment {
                    tag: Some(tag_ref(&cache, effect)),
                    marker: String::new(),
                    primary_scale: 0,
                    change_color: 0,
                },
            ],
            widgets: Vec::new(),
            tag: TagRef {
                group: opence_content::Group(*b"scen"),
                path: "rock".into(),
            },
            kind: ObjectKind::Scenery,
            flags: 0,
            bounding_radius: 1.0,
            bounding_offset: [0.0, 0.0, 0.0],
            acceleration_scale: 1.0,
            render_bounding_radius: 1.0,
            model: None,
            animation_graph: None,
            collision_model: None,
            physics: None,
            modifier_shader: None,
            creation_effect: None,
            function_modes: Default::default(),
            functions: Vec::new(),
            change_colors: Vec::new(),
            unit_hud: None,
            unit_melee_damage: None,
            biped: None,
            weapon: None,
            projectile: None,
            equipment: None,
            vehicle: None,
            device: None,
        };
        let contrails = import_contrails(&cache, &[carrier], &mut reached)?;
        assert_eq!(contrails.len(), 1);
        let trail = &contrails[0];
        assert_eq!(trail.point_generation_rate, 30.0);
        assert_eq!(trail.velocity, [1.0, 2.0]);
        assert_eq!(trail.inherited_velocity_fraction, 0.5);
        assert_eq!(trail.render_type, 2);
        assert_eq!(trail.texture_repeats, [1.0, 4.0]);
        assert_eq!(trail.texture_animation, [0.0, 2.0]);
        assert_eq!(trail.sequences, [1, 2]);
        assert_eq!(trail.shader.framebuffer_blend_function, 3);
        assert_eq!(trail.states.len(), 1);
        assert_eq!(trail.states[0].duration, [1.0, 2.0]);
        assert_eq!(trail.states[0].width, 0.2);
        assert_eq!(
            trail.states[0].color,
            [[1.0, 1.0, 0.0, 0.0], [1.0, 0.0, 1.0, 0.0]]
        );
        Ok(())
    }

    #[test]
    fn weather_is_read_from_a_bsps_palette() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let mut kind = [0u8; WEATHER_TYPE_SIZE];
        put(
            &mut kind,
            0x20,
            &WeatherParticleType::RANDOM_ROTATION.to_le_bytes(),
        );
        floats(&mut kind, 0x24, &[1.0, 2.0, 10.0, 20.0]);
        floats(&mut kind, 0xA4, &[0.01, 0.02]);
        empty(&mut kind, 0xAC);
        floats(&mut kind, 0xCC, &[1.0, 2.0, 0.5, 0.25]);
        floats(&mut kind, 0xFC, &[0.05, 0.1, 4.0, 8.0, 1.0, 2.0]);
        floats(&mut kind, 0x134, &[1.0; 8]);
        empty(&mut kind, 0x194);
        put(&mut kind, 0x1A4, &1i16.to_le_bytes());
        put(&mut kind, 0x1D0, &[0, 0, 3, 0, 0, 0]);
        let kind = fake.add_data(&kind);
        let mut rain = [0u8; WEATHER_SIZE];
        block(&mut rain, WEATHER_TYPES_OFFSET, 1, kind);
        let rain = fake.add_tag(RAIN, "weather\\snow", &rain);
        let mut entry = [0u8; BSP_WEATHER_PALETTE_SIZE];
        reference(&mut entry, 0x20, RAIN, fake.handle(rain));
        floats(&mut entry, 0x90, &[1.0, 0.0, 0.0, 3.0]);
        let mut unnamed = [0u8; BSP_WEATHER_PALETTE_SIZE];
        empty(&mut unnamed, 0x20);
        let mut entries = entry.to_vec();
        entries.extend_from_slice(&unnamed);
        let entries = fake.add_data(&entries);
        let mut bsp = vec![0u8; BSP_WEATHER_PALETTE_OFFSET + 12];
        block(&mut bsp, BSP_WEATHER_PALETTE_OFFSET, 2, entries);
        let bsp = fake.add_tag(GroupTag::new(b"sbsp"), "levels\\bsp", &bsp);
        let cache = Cache::open(fake.build())?;

        let element = cache.tag(bsp, GroupTag::new(b"sbsp"), BSP_WEATHER_PALETTE_OFFSET + 12)?;
        let palette = weather_palette(&element)?;
        assert_eq!(palette.len(), 2);
        assert_eq!(palette[0].wind, [1.0, 0.0, 0.0, 3.0]);
        assert_eq!(palette[1].system, None);
        let Some(system) = &palette[0].system else {
            panic!("the palette names no weather");
        };
        assert_eq!(system.path, "weather\\snow");
        let structure = StructureBsp {
            weather_palette: palette,
            ..opence_content::devmap::plane(1.0, 1)
        };
        let mut reached = Reached::default();
        let weather = import_weather(&cache, &[structure], &mut reached)?;
        assert_eq!(weather.len(), 1);
        let snow = &weather[0].types[0];
        assert_eq!(snow.flags, WeatherParticleType::RANDOM_ROTATION);
        assert_eq!(snow.distance_fade, [1.0, 2.0, 10.0, 20.0]);
        assert_eq!(snow.density, [0.01, 0.02]);
        assert_eq!(snow.physics, None);
        assert_eq!(snow.acceleration, [1.0, 2.0, 0.5, 0.25]);
        assert_eq!(snow.radius, [0.05, 0.1]);
        assert_eq!(snow.animation_rate, [4.0, 8.0]);
        assert_eq!(snow.rotation_rate, [1.0, 2.0]);
        assert_eq!(snow.color, [[1.0; 4]; 2]);
        assert_eq!(snow.render_mode, 1);
        assert_eq!(snow.shader.framebuffer_blend_function, 3);
        Ok(())
    }

    /// A BSP of one cell with two layers (0 and 2): one object in the
    /// first, lifted by its z reference, and two in the second, which has
    /// none.
    fn detail_bsp(fake: &mut FakeCache, count_index: i32) -> usize {
        let mut cell = [0u8; DETAIL_CELL_SIZE];
        put(&mut cell, 0x00, &[1, 0, 2, 0, 3, 0, 255, 0]);
        put(&mut cell, 0x08, &0b101u32.to_le_bytes());
        put(&mut cell, 0x0C, &0i32.to_le_bytes());
        put(&mut cell, 0x10, &count_index.to_le_bytes());
        let cell = fake.add_data(&cell);
        let mut instances = Vec::new();
        instances.extend_from_slice(&[255, 0, 255, 0x12, 0xFF, 0xFF]);
        instances.extend_from_slice(&[0, 255, 0, 0x03, 0x00, 0xF8]);
        instances.extend_from_slice(&[0, 0, 0, 0x00, 0x00, 0x00]);
        let instances = fake.add_data(&instances);
        let counts = fake.add_data(&[1, 0, 2, 0]);
        let mut z = [0u8; DETAIL_Z_REFERENCE_SIZE];
        floats(&mut z, 0, &[0.0, 0.0, 1.0, 0.5]);
        let z = fake.add_data(&z);
        let mut data = [0u8; BSP_DETAIL_OBJECTS_SIZE];
        block(&mut data, 0x00, 1, cell);
        block(&mut data, 0x0C, 3, instances);
        block(&mut data, 0x18, 2, counts);
        block(&mut data, 0x24, 1, z);
        let data = fake.add_data(&data);
        let mut bsp = vec![0u8; BSP_DETAIL_OBJECTS_OFFSET + 12];
        block(&mut bsp, BSP_DETAIL_OBJECTS_OFFSET, 1, data);
        fake.add_tag(GroupTag::new(b"sbsp"), "levels\\bsp", &bsp)
    }

    #[test]
    fn detail_objects_are_placed_by_their_cells_and_layers() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let bsp = detail_bsp(&mut fake, 0);
        let mut kind = [0u8; COLLECTION_TYPE_SIZE];
        put(&mut kind, 0x20, &[2, 0, 1, 3]);
        floats(&mut kind, 0x24, &[0.5, 0.0, 0.0, 4.0, 8.0, 0.02, 0.04]);
        let kind = fake.add_data(&kind);
        let mut collection = [0u8; COLLECTION_SIZE];
        put(&mut collection, 0x00, &1i16.to_le_bytes());
        empty(&mut collection, 0x34);
        block(&mut collection, COLLECTION_TYPES_OFFSET, 1, kind);
        let collection = fake.add_tag(DOBC, "detail\\grass", &collection);
        let mut entries = [0u8; DETAIL_PALETTE_SIZE * 2];
        reference(&mut entries, 0, DOBC, fake.handle(collection));
        empty(&mut entries, DETAIL_PALETTE_SIZE);
        let entries = fake.add_data(&entries);
        let mut scenario = vec![0u8; SCENARIO_DETAIL_PALETTE_OFFSET + 12];
        block(&mut scenario, SCENARIO_DETAIL_PALETTE_OFFSET, 2, entries);
        let scenario = fake.add_tag(GroupTag::new(b"scnr"), "levels\\scenario", &scenario);
        let cache = Cache::open(fake.build())?;

        let element = cache.tag(bsp, GroupTag::new(b"sbsp"), BSP_DETAIL_OBJECTS_OFFSET + 12)?;
        let objects = detail_objects(&element)?;
        assert_eq!(objects.len(), 3);
        // (cell (1, 2, 3 + 255/255), lifted a cell and a half by its
        // height and the z reference's)
        assert_eq!(objects[0].position, [16.0, 16.0, 44.0]);
        assert_eq!(objects[0].color, [255, 255, 255]);
        assert_eq!((objects[0].collection, objects[0].data), (0, 0x12));
        // (the second layer has no z reference: it is lifted by none)
        assert_eq!(objects[1].position, [8.0, 24.0, 32.0]);
        assert_eq!(objects[1].color, [255, 0, 0]);
        assert_eq!((objects[1].collection, objects[1].data), (2, 0x03));
        assert_eq!(objects[2].position, [8.0, 16.0, 32.0]);

        let structure = StructureBsp {
            detail_objects: objects,
            ..opence_content::devmap::plane(1.0, 1)
        };
        let element = cache.tag(
            scenario,
            GroupTag::new(b"scnr"),
            SCENARIO_DETAIL_PALETTE_OFFSET + 12,
        )?;
        let mut reached = Reached::default();
        let none = import_detail_object_collections(&cache, &element, &[], &mut reached)?;
        assert!(none.is_empty(), "no BSP places a detail object");
        let collections =
            import_detail_object_collections(&cache, &element, &[structure], &mut reached)?;
        assert_eq!(collections.len(), 2);
        assert_eq!(collections[1], None);
        let Some(grass) = &collections[0] else {
            panic!("the grass was not read");
        };
        assert_eq!(grass.kind, 1);
        assert_eq!(
            grass.types,
            [DetailObjectType {
                sequence: 2,
                sprites: [1, 3],
                color_override: 0.5,
                fade: [4.0, 8.0],
                size: [0.02, 0.04],
            }]
        );
        Ok(())
    }

    #[test]
    fn a_cells_counts_past_the_end_are_refused() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let bsp = detail_bsp(&mut fake, 1);
        let cache = Cache::open(fake.build())?;
        let element = cache.tag(bsp, GroupTag::new(b"sbsp"), BSP_DETAIL_OBJECTS_OFFSET + 12)?;
        assert!(detail_objects(&element).is_err());
        Ok(())
    }

    #[test]
    fn a_bsp_without_detail_objects_has_none() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let bsp = vec![0u8; BSP_DETAIL_OBJECTS_OFFSET + 12];
        let bsp = fake.add_tag(GroupTag::new(b"sbsp"), "levels\\bsp", &bsp);
        let cache = Cache::open(fake.build())?;
        let element = cache.tag(bsp, GroupTag::new(b"sbsp"), BSP_DETAIL_OBJECTS_OFFSET + 12)?;
        assert!(detail_objects(&element)?.is_empty());
        Ok(())
    }
}
