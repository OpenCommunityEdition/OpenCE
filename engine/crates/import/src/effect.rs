//! Effects (`effe`), as far as the simulation reads them: the damage
//! effects their events' parts create, and the particles they emit that
//! are heavy enough to be bodies. An effect is 0x40 bytes, its events
//! (0x44 bytes each) a block at 0x34, an event's parts (0x68 bytes each) a
//! block at 0x2C and its particles (0xE8 bytes each) one at 0x38, and a
//! part's tag a reference at 0x18. A particle (`part`, 0x164 bytes) names
//! its point physics (`pphy`, 0x40 bytes) at 0x14 and its least radius at
//! 0x74; the point physics' density is at 0x20, its contact friction at
//! 0x2C and its elasticity at 0x30.

use opence_content::{Effect, EffectDebris};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::object::vector;
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
const PARTICLES_OFFSET: usize = 0x38;
pub(crate) const PARTICLES_SIZE: usize = 0xE8;
const MAX_PARTICLES: usize = 32;
const PART: GroupTag = GroupTag::new(b"part");
pub(crate) const PARTICLE_SIZE: usize = 0x164;
const PPHY: GroupTag = GroupTag::new(b"pphy");
pub(crate) const POINT_PHYSICS_SIZE: usize = 0x40;
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
    for event in events.iter() {
        let particles =
            event.block(PARTICLES_OFFSET, "particles", PARTICLES_SIZE, MAX_PARTICLES)?;
        for particles in particles.iter() {
            if let Some(piece) = import_debris(cache, &particles)? {
                debris.push(piece);
            }
        }
        let parts = event.block(PARTS_OFFSET, "parts", PART_SIZE, MAX_PARTS)?;
        for part in parts.iter() {
            // (a part creates any object or effect; only damage is read)
            if let Some(part_index) = part.reference(PART_TAG_OFFSET, "type", &[])? {
                if cache.tags()[part_index].is(JPT) {
                    damage.push(tag_ref(cache, part_index));
                }
            }
        }
    }
    Ok(Effect {
        tag: tag_ref(cache, index),
        damage,
        debris,
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
    fn an_effect_names_the_damage_its_parts_create() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let damage = fake.add_tag(
            JPT,
            "weapons\\test\\explosion",
            &vec![0u8; DAMAGE_EFFECT_SIZE],
        );
        let other = fake.add_tag(EFFE, "weapons\\test\\smoke", &[0u8; EFFECT_SIZE]);
        let mut parts = vec![0u8; PART_SIZE * 2];
        reference(&mut parts, PART_TAG_OFFSET, EFFE, fake.handle(other));
        reference(
            &mut parts,
            PART_SIZE + PART_TAG_OFFSET,
            JPT,
            fake.handle(damage),
        );
        let parts = fake.add_data(&parts);
        let mut event = vec![0u8; EVENT_SIZE];
        block(&mut event, PARTS_OFFSET, 2, parts);
        let events = fake.add_data(&event);
        let mut effect = vec![0u8; EFFECT_SIZE];
        block(&mut effect, EVENTS_OFFSET, 1, events);
        let index = fake.add_tag(EFFE, "weapons\\test\\blast", &effect);
        let cache = Cache::open(fake.build())?;
        let effect = import_effect(&cache, index)?;
        assert_eq!(effect.tag.path, "weapons\\test\\blast");
        assert_eq!(effect.damage.len(), 1);
        assert_eq!(effect.damage[0].group.0, *b"jpt!");
        Ok(())
    }
}
