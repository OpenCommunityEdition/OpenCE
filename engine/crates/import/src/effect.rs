//! Effects (`effe`): their events and the parts each makes, the damage
//! effects, lights and decals among those parts, and the particles they
//! emit that are heavy enough to be bodies. An effect is 0x40 bytes: its
//! flags at 0x00, the event it loops from at 0x04, its locations (0x20
//! bytes each, a marker's name) a block at 0x28 and its events (0x44 bytes
//! each) a block at 0x34. An event's skip fraction is at 0x04, its delay
//! bounds at 0x08 and its duration bounds at 0x10, its parts (0x68 bytes
//! each) a block at 0x2C and its particles (0xE8 bytes each) one at 0x38.
//! A part's environment is at 0x00, its mode at 0x02, its location at 0x04,
//! its flags at 0x06, its tag a reference at 0x18, its velocity bounds at
//! 0x40, its cone at 0x48, its angular velocity bounds at 0x4C and its
//! radius modifier bounds at 0x54. A particle (`part`, 0x164 bytes) names
//! its point physics (`pphy`, 0x40 bytes) at 0x14 and its least radius at
//! 0x74; the point physics' density is at 0x20, its contact friction at
//! 0x2C and its elasticity at 0x30.

use opence_content::{Effect, EffectDebris, EffectEvent, EffectPart};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::object::vector;
use crate::particle::{PART, PARTICLE_SIZE, POINT_PHYSICS_SIZE, PPHY};
use crate::tags::{Element, GroupTag};
use crate::weapon::JPT;

pub(crate) const EFFE: GroupTag = GroupTag::new(b"effe");
pub(crate) const EFFECT_SIZE: usize = 0x40;
pub(crate) const EVENT_SIZE: usize = 0x44;
pub(crate) const PART_SIZE: usize = 0x68;
const EVENTS_OFFSET: usize = 0x34;
const PARTS_OFFSET: usize = 0x2C;
const PART_TAG_OFFSET: usize = 0x18;
const MAX_EVENTS: usize = 32;
const MAX_PARTS: usize = 32;
pub(crate) const PARTICLES_OFFSET: usize = 0x38;
pub(crate) const PARTICLES_SIZE: usize = 0xE8;
pub(crate) const MAX_PARTICLES: usize = 32;
const LOCATIONS_OFFSET: usize = 0x28;
pub(crate) const LOCATION_SIZE: usize = 0x20;
const MAX_LOCATIONS: usize = 32;
/// The least density a particle is a body at, grams a millilitre (the
/// C port's `PHYSICS_DEBRIS_DENSITY_MIN`: lighter ones are smoke and
/// sparks).
pub const DEBRIS_DENSITY_MIN: f32 = 0.05;

/// An effect: the damage effects its parts create, in order.
pub(crate) fn import_effect(cache: &Cache, index: usize) -> Result<Effect, ImportError> {
    let tag = cache.tag(index, EFFE, EFFECT_SIZE)?;
    let events = tag.block(EVENTS_OFFSET, "events", EVENT_SIZE, MAX_EVENTS)?;
    let mut damage = Vec::new();
    let mut debris = Vec::new();
    let mut lights = Vec::new();
    let mut decals = Vec::new();
    let mut timeline = Vec::new();
    let locations = tag
        .block(LOCATIONS_OFFSET, "locations", LOCATION_SIZE, MAX_LOCATIONS)?
        .iter()
        .map(|location| Ok(location.text(0, "marker name", 31)?.to_owned()))
        .collect::<Result<Vec<String>, ImportError>>()?;
    for event in events.iter() {
        let particles =
            event.block(PARTICLES_OFFSET, "particles", PARTICLES_SIZE, MAX_PARTICLES)?;
        for particles in particles.iter() {
            if let Some(piece) = import_debris(cache, &particles)? {
                debris.push(piece);
            }
        }
        let parts = event.block(PARTS_OFFSET, "parts", PART_SIZE, MAX_PARTS)?;
        let mut made = Vec::new();
        for part in parts.iter() {
            let Some(part_index) = part.reference(PART_TAG_OFFSET, "type", &[])? else {
                continue;
            };
            if cache.tags()[part_index].is(JPT) {
                damage.push(tag_ref(cache, part_index));
            } else if cache.tags()[part_index].is(crate::light::LIGH) {
                lights.push(tag_ref(cache, part_index));
            } else if cache.tags()[part_index].is(crate::decal::DECA) {
                decals.push(tag_ref(cache, part_index));
            }
            made.push(EffectPart {
                create_in: part.i16(0x00, "create in")?,
                create_in_mode: part.i16(0x02, "create in mode")?,
                location: part.i16(0x04, "location")?,
                flags: part.u16(0x06, "flags")?,
                tag: tag_ref(cache, part_index),
                velocity: vector(&part, 0x40, "velocity bounds")?,
                velocity_cone_angle: part.f32(0x48, "velocity cone angle")?,
                angular_velocity: vector(&part, 0x4C, "angular velocity bounds")?,
                radius_modifier: vector(&part, 0x54, "radius modifier bounds")?,
            });
        }
        timeline.push(EffectEvent {
            particles: crate::particle::effect_particles(&event)?,
            skip_fraction: event.f32(0x04, "skip fraction")?,
            delay: vector(&event, 0x08, "delay bounds")?,
            duration: vector(&event, 0x10, "duration bounds")?,
            parts: made,
        });
    }
    Ok(Effect {
        tag: tag_ref(cache, index),
        damage,
        debris,
        lights,
        decals,
        flags: tag.u32(0x00, "flags")?,
        loop_start_event: tag.i16(0x04, "loop start event")?,
        locations,
        events: timeline,
    })
}

/// An event's particles, if their point physics makes them bodies.
fn import_debris(
    cache: &Cache,
    particles: &Element<'_>,
) -> Result<Option<EffectDebris>, ImportError> {
    let Some(particle) = particles.reference(0x54, "particle type", &[PART])? else {
        return Ok(None);
    };
    let particle = cache.tag(particle, PART, PARTICLE_SIZE)?;
    let Some(physics) = particle.reference(0x14, "physics", &[PPHY])? else {
        return Ok(None);
    };
    let physics = cache.tag(physics, PPHY, POINT_PHYSICS_SIZE)?;
    let density = physics.f32(0x20, "density")?;
    // (written as `!(a >= b)` so a NaN density is no body either)
    if density.is_nan() || density < DEBRIS_DENSITY_MIN {
        return Ok(None);
    }
    // (the particle's radius at birth, by the event's radius modifier)
    let modifier = vector::<2>(particles, 0xA0, "radius")?;
    let radius = particle.f32(0x74, "radius")? * 0.5 * (modifier[0] + modifier[1]);
    Ok(Some(EffectDebris {
        count: [particles.i16(0x6C, "count")?, particles.i16(0x6E, "count")?],
        offset: vector(particles, 0x14, "relative offset")?,
        direction: vector(particles, 0x0C, "relative direction")?,
        velocity: vector(particles, 0x84, "velocity")?,
        velocity_cone_angle: particles.f32(0x8C, "velocity cone angle")?,
        radius,
        density,
        friction: physics.f32(0x2C, "contact friction")?,
        elasticity: physics.f32(0x30, "elasticity")?,
    }))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, reference};
    use crate::weapon::DAMAGE_EFFECT_SIZE;
    use std::error::Error;

    #[test]
    fn an_effect_names_the_damage_lights_and_decals_its_parts_create() -> Result<(), Box<dyn Error>>
    {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let damage = fake.add_tag(
            JPT,
            "weapons\\test\\explosion",
            &vec![0u8; DAMAGE_EFFECT_SIZE],
        );
        let other = fake.add_tag(EFFE, "weapons\\test\\smoke", &[0u8; EFFECT_SIZE]);
        let mut parts = vec![0u8; PART_SIZE * 4];
        reference(&mut parts, PART_TAG_OFFSET, EFFE, fake.handle(other));
        let light = fake.add_tag(
            crate::light::LIGH,
            "weapons\\test\\flash",
            &vec![0u8; crate::light::LIGHT_SIZE],
        );
        reference(
            &mut parts,
            PART_SIZE + PART_TAG_OFFSET,
            JPT,
            fake.handle(damage),
        );
        reference(
            &mut parts,
            PART_SIZE * 2 + PART_TAG_OFFSET,
            crate::light::LIGH,
            fake.handle(light),
        );
        let decal = fake.add_tag(
            crate::decal::DECA,
            "effects\\decals\\scorch",
            &vec![0u8; crate::decal::DECAL_SIZE],
        );
        reference(
            &mut parts,
            PART_SIZE * 3 + PART_TAG_OFFSET,
            crate::decal::DECA,
            fake.handle(decal),
        );
        // (the light is made at the second location, faster than 2 a
        // second, facing down)
        let light_part = PART_SIZE * 2;
        parts[light_part + 0x04..light_part + 0x06].copy_from_slice(&1i16.to_le_bytes());
        parts[light_part + 0x06..light_part + 0x08].copy_from_slice(&1u16.to_le_bytes());
        parts[light_part + 0x40..light_part + 0x44].copy_from_slice(&2.0f32.to_le_bytes());
        parts[light_part + 0x44..light_part + 0x48].copy_from_slice(&3.0f32.to_le_bytes());
        let parts = fake.add_data(&parts);
        let mut event = vec![0u8; EVENT_SIZE * 2];
        block(&mut event, PARTS_OFFSET, 4, parts);
        event[0x04..0x08].copy_from_slice(&0.25f32.to_le_bytes());
        event[0x08..0x0C].copy_from_slice(&0.5f32.to_le_bytes());
        event[0x0C..0x10].copy_from_slice(&1.0f32.to_le_bytes());
        event[0x10..0x14].copy_from_slice(&2.0f32.to_le_bytes());
        event[0x14..0x18].copy_from_slice(&4.0f32.to_le_bytes());
        let events = fake.add_data(&event);
        let mut locations = vec![0u8; LOCATION_SIZE * 2];
        locations[..6].copy_from_slice(b"origin");
        locations[LOCATION_SIZE..LOCATION_SIZE + 6].copy_from_slice(b"muzzle");
        let locations = fake.add_data(&locations);
        let mut effect = vec![0u8; EFFECT_SIZE];
        effect[0x04..0x06].copy_from_slice(&1i16.to_le_bytes());
        block(&mut effect, LOCATIONS_OFFSET, 2, locations);
        block(&mut effect, EVENTS_OFFSET, 2, events);
        let index = fake.add_tag(EFFE, "weapons\\test\\blast", &effect);
        let cache = Cache::open(fake.build())?;
        let effect = import_effect(&cache, index)?;
        assert_eq!(effect.tag.path, "weapons\\test\\blast");
        assert_eq!(effect.damage.len(), 1);
        assert_eq!(effect.damage[0].group.0, *b"jpt!");
        assert_eq!(effect.lights.len(), 1);
        assert_eq!(effect.lights[0].path, "weapons\\test\\flash");
        assert_eq!(effect.decals.len(), 1);
        assert_eq!(effect.decals[0].path, "effects\\decals\\scorch");
        assert_eq!(effect.loop_start_event, 1);
        assert_eq!(effect.locations, ["origin", "muzzle"]);
        assert_eq!(effect.events.len(), 2);
        let first = &effect.events[0];
        assert_eq!(first.skip_fraction, 0.25);
        assert_eq!(first.delay, [0.5, 1.0]);
        assert_eq!(first.duration, [2.0, 4.0]);
        assert_eq!(first.parts.len(), 4, "every part with a tag is kept");
        assert_eq!(first.parts[0].tag.path, "weapons\\test\\smoke");
        let light = &first.parts[2];
        assert_eq!(light.tag.group.0, *b"ligh");
        assert_eq!(light.location, 1);
        assert_eq!(light.flags, EffectPart::FACE_DOWN);
        assert_eq!(light.velocity, [2.0, 3.0]);
        assert!(effect.events[1].parts.is_empty());
        Ok(())
    }
}
