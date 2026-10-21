//! Effects (`effe`), as far as the simulation reads them: the damage
//! effects their events' parts create. An effect is 0x40 bytes, its
//! events (0x44 bytes each) a block at 0x34, an event's parts (0x68 bytes
//! each) a block at 0x2C, and a part's tag a reference at 0x18.

use opence_content::Effect;

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::tags::GroupTag;
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

/// An effect: the damage effects its parts create, in order.
pub(crate) fn import_effect(cache: &Cache, index: usize) -> Result<Effect, ImportError> {
    let tag = cache.tag(index, EFFE, EFFECT_SIZE)?;
    let events = tag.block(EVENTS_OFFSET, "events", EVENT_SIZE, MAX_EVENTS)?;
    let mut damage = Vec::new();
    for event in events.iter() {
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
    })
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
