//! Lights (`ligh`): the dynamic lights objects carry and effects make.

use opence_content::Light;

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::object::{reference, vector};
use crate::tags::GroupTag;

pub(crate) const LIGH: GroupTag = GroupTag::new(b"ligh");
pub(crate) const LIGHT_SIZE: usize = 0x160;
const BITM: GroupTag = GroupTag::new(b"bitm");
use crate::lens::LENS;

/// A light tag.
pub(crate) fn import_light(cache: &Cache, index: usize) -> Result<Light, ImportError> {
    let light = cache.tag(index, LIGH, LIGHT_SIZE)?;
    Ok(Light {
        tag: tag_ref(cache, index),
        flags: light.u32(0x00, "flags")?,
        radius: light.f32(0x04, "radius")?,
        radius_modifier: vector(&light, 0x08, "radius modifer")?,
        falloff_angle: light.f32(0x10, "falloff angle")?,
        cutoff_angle: light.f32(0x14, "cutoff angle")?,
        lens_flare_radius: light.f32(0x18, "lens flare only radius")?,
        cosine_falloff_angle: light.f32(0x1C, "cos falloff angle")?,
        cosine_cutoff_angle: light.f32(0x20, "cos cutoff angle")?,
        specular_radius_multiplier: light.f32(0x24, "specular radius multiplier")?,
        sine_cutoff_angle: light.f32(0x28, "sin cutoff angle")?,
        color_flags: light.u32(0x34, "interpolation flags")?,
        color_lower: vector(&light, 0x38, "color lower bound")?,
        color_upper: vector(&light, 0x48, "color upper bound")?,
        gel: reference(&light, 0x64, "primary cube map", &[BITM])?,
        lens_flare: reference(&light, 0xAC, "lens flare", &[LENS])?,
        duration: light.f32(0xF4, "duration")?,
        falloff_function: light.i16(0xFA, "falloff function")?,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{empty, put, reference};
    use std::error::Error;

    #[test]
    fn a_light_reads_its_fields() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let gel = fake.add_tag(BITM, "effects\\gel", &[0u8; 0x6C]);
        let mut tag = vec![0u8; LIGHT_SIZE];
        put(
            &mut tag,
            0x00,
            &(Light::DYNAMIC | Light::NO_SPECULAR).to_le_bytes(),
        );
        put(&mut tag, 0x04, &3.0f32.to_le_bytes());
        put(&mut tag, 0x08, &0.5f32.to_le_bytes());
        put(&mut tag, 0x0C, &1.5f32.to_le_bytes());
        put(&mut tag, 0x20, &0.25f32.to_le_bytes());
        put(&mut tag, 0x34, &Light::HSV.to_le_bytes());
        for (i, v) in [1.0f32, 0.2, 0.4, 0.6].iter().enumerate() {
            put(&mut tag, 0x48 + i * 4, &v.to_le_bytes());
        }
        reference(&mut tag, 0x64, BITM, fake.handle(gel));
        empty(&mut tag, 0xAC);
        put(&mut tag, 0xF4, &30.0f32.to_le_bytes());
        put(&mut tag, 0xFA, &3i16.to_le_bytes());
        let index = fake.add_tag(LIGH, "effects\\lights\\flash", &tag);
        let cache = Cache::open(fake.build())?;
        let light = import_light(&cache, index)?;
        assert_eq!(light.tag.path, "effects\\lights\\flash");
        assert_eq!(light.flags, Light::DYNAMIC | Light::NO_SPECULAR);
        assert_eq!(light.radius, 3.0);
        assert_eq!(light.radius_modifier, [0.5, 1.5]);
        assert_eq!(light.cosine_cutoff_angle, 0.25);
        assert_eq!(light.color_flags, Light::HSV);
        assert_eq!(light.color_upper, [1.0, 0.2, 0.4, 0.6]);
        assert_eq!(light.gel.map(|g| g.path), Some("effects\\gel".to_owned()));
        assert_eq!(light.lens_flare, None);
        assert_eq!(light.duration, 30.0);
        assert_eq!(light.falloff_function, 3);
        Ok(())
    }
}
