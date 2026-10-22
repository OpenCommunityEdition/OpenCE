//! Lens flares (`lens`), light volumes (`mgs2`) and glows (`glw!`): what
//! the camera sees of lights, at the level's markers, its lights, its
//! skies and its objects' widgets.

use opence_content::{Glow, LensFlare, LensFlareReflection, LightVolume, LightVolumeFrame};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_ref, Reached};
use crate::tags::{Element, GroupTag};

pub(crate) const LENS: GroupTag = GroupTag::new(b"lens");
pub(crate) const MGS2: GroupTag = GroupTag::new(b"mgs2");
pub(crate) const GLOW: GroupTag = GroupTag::new(b"glw!");
pub(crate) const LENS_FLARE_SIZE: usize = 0xF0;
const REFLECTION_SIZE: usize = 0x80;
const MAX_REFLECTIONS: usize = 32;
pub(crate) const LIGHT_VOLUME_SIZE: usize = 0x14C;
const LIGHT_VOLUME_FRAME_SIZE: usize = 0xB0;
const MAX_LIGHT_VOLUME_FRAMES: usize = 2;
pub(crate) const GLOW_SIZE: usize = 0x154;
const MARKER_LONGEST: usize = 31;

/// An `argb` colour at `at`, alpha moved last.
fn argb(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 4], ImportError> {
    let mut color = [0.0f32; 4];
    for (channel, value) in color.iter_mut().enumerate() {
        *value = element.f32(at + channel * 4, field)?;
    }
    let [alpha, red, green, blue] = color;
    Ok([red, green, blue, alpha])
}

fn pair(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 2], ImportError> {
    Ok([element.f32(at, field)?, element.f32(at + 4, field)?])
}

/// A lens flare tag, its bitmap reached.
pub(crate) fn import_lens_flare(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<LensFlare, ImportError> {
    let flare = cache.tag(index, LENS, LENS_FLARE_SIZE)?;
    let block = flare.block(0xC4, "reflections", REFLECTION_SIZE, MAX_REFLECTIONS)?;
    let mut reflections = Vec::with_capacity(block.len());
    for reflection in block.iter() {
        reflections.push(LensFlareReflection {
            flags: reflection.u16(0x00, "flags")?,
            bitmap_index: reflection.i16(0x04, "bitmap index")?,
            offset: reflection.f32(0x1C, "position")?,
            rotation_offset: reflection.f32(0x20, "rotation offset")?,
            radius: pair(&reflection, 0x28, "radius")?,
            brightness: pair(&reflection, 0x34, "brightness")?,
            brightness_scale_function: reflection.i16(0x3C, "brightness scaled by")?,
            tint: argb(&reflection, 0x40, "tint color")?,
            animation_color: [
                argb(&reflection, 0x50, "color lower bound")?,
                argb(&reflection, 0x60, "color upper bound")?,
            ],
            animation_flags: reflection.u16(0x70, "animation flags")?,
            animation_function: reflection.i16(0x72, "animation function")?,
            animation_period: reflection.f32(0x74, "animation period")?,
            animation_phase: reflection.f32(0x78, "animation phase")?,
        });
    }
    Ok(LensFlare {
        tag: tag_ref(cache, index),
        cosine_falloff_angle: flare.f32(0x08, "cos falloff angle")?,
        cosine_cutoff_angle: flare.f32(0x0C, "cos cutoff angle")?,
        occlusion_radius: flare.f32(0x10, "occlusion radius")?,
        occlusion_offset_direction: flare.i16(0x14, "occlusion offset direction")?,
        near_fade_distance: flare.f32(0x18, "near fade distance")?,
        far_fade_distance: flare.f32(0x1C, "far fade distance")?,
        bitmap: reached.bitmap(&flare, 0x20, "bitmap")?,
        flags: flare.u16(0x30, "flags")?,
        corona_rotation_function: flare.i16(0x80, "corona rotation function")?,
        corona_rotation_function_scale: flare.f32(0x84, "corona rotation function scale")?,
        corona_radius_scale: pair(&flare, 0xA0, "horizontal scale")?,
        reflections,
    })
}

/// A light volume tag, its bitmap reached.
pub(crate) fn import_light_volume(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<LightVolume, ImportError> {
    let volume = cache.tag(index, MGS2, LIGHT_VOLUME_SIZE)?;
    let block = volume.block(
        0x120,
        "frames",
        LIGHT_VOLUME_FRAME_SIZE,
        MAX_LIGHT_VOLUME_FRAMES,
    )?;
    let mut frames = Vec::with_capacity(block.len());
    for frame in block.iter() {
        frames.push(LightVolumeFrame {
            offset_from_marker: frame.f32(0x10, "offset from marker")?,
            offset_exponent: frame.f32(0x14, "offset exponent")?,
            length: frame.f32(0x18, "length")?,
            radius: pair(&frame, 0x3C, "radius")?,
            radius_exponent: frame.f32(0x44, "radius exponent")?,
            color: [
                argb(&frame, 0x68, "tint color hither")?,
                argb(&frame, 0x78, "tint color yon")?,
            ],
            color_exponent: frame.f32(0x88, "tint color exponent")?,
            brightness_exponent: frame.f32(0x8C, "brightness exponent")?,
        });
    }
    Ok(LightVolume {
        tag: tag_ref(cache, index),
        marker: volume
            .text(0x00, "attachment marker", MARKER_LONGEST)?
            .to_owned(),
        near_fade_distance: volume.f32(0x34, "near fade distance")?,
        far_fade_distance: volume.f32(0x38, "far fade distance")?,
        perpendicular_brightness_scale: volume.f32(0x3C, "perpendicular brightness scale")?,
        parallel_brightness_scale: volume.f32(0x40, "parallel brightness scale")?,
        brightness_scale_source: volume.i16(0x44, "brightness scale source")?,
        bitmap: reached.bitmap(&volume, 0x5C, "map")?,
        sequence_index: volume.i16(0x6C, "sequence index")?,
        count: volume.i16(0x6E, "count")?,
        frames,
    })
}

/// A glow tag, its texture reached.
pub(crate) fn import_glow(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<Glow, ImportError> {
    let glow = cache.tag(index, GLOW, GLOW_SIZE)?;
    Ok(Glow {
        tag: tag_ref(cache, index),
        marker: glow
            .text(0x00, "attachment marker", MARKER_LONGEST)?
            .to_owned(),
        number_of_particles: glow.i16(0x20, "number of particles")?,
        flags: glow.u32(0x28, "flags")?,
        effect_rotational_velocity: glow.f32(0x64, "effect rotational velocity")?,
        distance_to_object: pair(&glow, 0x84, "distance to object")?,
        particle_size: pair(&glow, 0xA0, "particle size")?,
        color: [
            argb(&glow, 0xB4, "color lower bound")?,
            argb(&glow, 0xC4, "color upper bound")?,
        ],
        percentage_edge_fade: glow.f32(0xF8, "percentage of edge fade")?,
        texture: reached.bitmap(&glow, 0x144, "texture")?,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, put, reference};
    use opence_content::Group;
    use std::error::Error;

    const BITM: GroupTag = GroupTag::new(b"bitm");

    fn floats(tag: &mut [u8], at: usize, values: &[f32]) {
        for (i, value) in values.iter().enumerate() {
            put(tag, at + i * 4, &value.to_le_bytes());
        }
    }

    #[test]
    fn a_lens_flare_reads_its_fields_and_reflections() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let bitmap = fake.add_tag(BITM, "effects\\flares", &[0u8; 0x6C]);
        let mut tag = vec![0u8; LENS_FLARE_SIZE];
        let mut reflections = vec![0u8; 2 * REFLECTION_SIZE];
        floats(&mut tag, 0x08, &[0.9, 0.5, 0.25]);
        put(&mut tag, 0x14, &1i16.to_le_bytes());
        floats(&mut tag, 0x18, &[10.0, 40.0]);
        reference(&mut tag, 0x20, BITM, fake.handle(bitmap));
        put(&mut tag, 0x30, &LensFlare::SUN.to_le_bytes());
        put(&mut tag, 0x80, &2i16.to_le_bytes());
        floats(&mut tag, 0x84, &[45.0]);
        floats(&mut tag, 0xA0, &[2.0, 0.5]);
        let second = REFLECTION_SIZE;
        put(
            &mut reflections,
            second,
            &(LensFlareReflection::ROTATE_FROM_CENTER | LensFlareReflection::ZBUFFER).to_le_bytes(),
        );
        put(&mut reflections, second + 0x04, &3i16.to_le_bytes());
        floats(&mut reflections, second + 0x1C, &[1.5, 90.0]);
        floats(&mut reflections, second + 0x28, &[0.1, 0.2]);
        floats(&mut reflections, second + 0x34, &[0.3, 0.6]);
        put(&mut reflections, second + 0x3C, &2i16.to_le_bytes());
        floats(&mut reflections, second + 0x40, &[1.0, 0.2, 0.4, 0.8]);
        put(&mut reflections, second + 0x72, &4i16.to_le_bytes());
        floats(&mut reflections, second + 0x74, &[2.0, 0.5]);
        let address = fake.add_data(&reflections);
        block(&mut tag, 0xC4, 2, address);
        let index = fake.add_tag(LENS, "effects\\lens flares\\sun", &tag);
        let cache = Cache::open(fake.build())?;
        let mut reached = Reached::default();
        let flare = import_lens_flare(&cache, index, &mut reached)?;
        assert_eq!(flare.tag.group, Group(*b"lens"));
        assert_eq!(flare.cosine_falloff_angle, 0.9);
        assert_eq!(flare.cosine_cutoff_angle, 0.5);
        assert_eq!(flare.occlusion_radius, 0.25);
        assert_eq!(flare.occlusion_offset_direction, 1);
        assert_eq!(
            [flare.near_fade_distance, flare.far_fade_distance],
            [10.0, 40.0]
        );
        assert_eq!(
            flare.bitmap.map(|b| b.path),
            Some("effects\\flares".to_owned())
        );
        assert!(reached.bitmaps.contains(&bitmap));
        assert_eq!(flare.flags, LensFlare::SUN);
        assert_eq!(flare.corona_rotation_function, 2);
        assert_eq!(flare.corona_rotation_function_scale, 45.0);
        assert_eq!(flare.corona_radius_scale, [2.0, 0.5]);
        assert_eq!(flare.reflections.len(), 2);
        let reflection = &flare.reflections[1];
        assert_eq!(
            reflection.flags,
            LensFlareReflection::ROTATE_FROM_CENTER | LensFlareReflection::ZBUFFER
        );
        assert_eq!(reflection.bitmap_index, 3);
        assert_eq!([reflection.offset, reflection.rotation_offset], [1.5, 90.0]);
        assert_eq!(reflection.radius, [0.1, 0.2]);
        assert_eq!(reflection.brightness, [0.3, 0.6]);
        assert_eq!(reflection.brightness_scale_function, 2);
        assert_eq!(reflection.tint, [0.2, 0.4, 0.8, 1.0]);
        assert_eq!(reflection.animation_function, 4);
        assert_eq!(
            [reflection.animation_period, reflection.animation_phase],
            [2.0, 0.5]
        );
        Ok(())
    }

    #[test]
    fn a_light_volume_reads_its_frame() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let mut tag = vec![0u8; LIGHT_VOLUME_SIZE];
        let mut frame = vec![0u8; LIGHT_VOLUME_FRAME_SIZE];
        put(&mut tag, 0x00, b"beam\0");
        floats(&mut tag, 0x34, &[5.0, 20.0, 1.0, 0.25]);
        put(&mut tag, 0x44, &2i16.to_le_bytes());
        put(&mut tag, 0x6C, &1i16.to_le_bytes());
        put(&mut tag, 0x6E, &8i16.to_le_bytes());
        crate::level::tests::empty(&mut tag, 0x5C);
        floats(&mut frame, 0x10, &[0.5, 1.0, 4.0]);
        floats(&mut frame, 0x3C, &[0.1, 0.4, 2.0]);
        floats(&mut frame, 0x68, &[1.0, 1.0, 0.5, 0.0]);
        floats(&mut frame, 0x78, &[0.0, 0.0, 0.5, 1.0]);
        floats(&mut frame, 0x88, &[1.0, 3.0]);
        let address = fake.add_data(&frame);
        block(&mut tag, 0x120, 1, address);
        let index = fake.add_tag(MGS2, "effects\\beam", &tag);
        let cache = Cache::open(fake.build())?;
        let volume = import_light_volume(&cache, index, &mut Reached::default())?;
        assert_eq!(volume.marker, "beam");
        assert_eq!(
            [volume.near_fade_distance, volume.far_fade_distance],
            [5.0, 20.0]
        );
        assert_eq!(
            [
                volume.perpendicular_brightness_scale,
                volume.parallel_brightness_scale
            ],
            [1.0, 0.25]
        );
        assert_eq!(volume.brightness_scale_source, 2);
        assert_eq!(volume.bitmap, None);
        assert_eq!([volume.sequence_index, volume.count], [1, 8]);
        let frame = volume.frames[0];
        assert_eq!(frame.length, 4.0);
        assert_eq!(frame.radius, [0.1, 0.4]);
        assert_eq!(frame.radius_exponent, 2.0);
        assert_eq!(frame.color, [[1.0, 0.5, 0.0, 1.0], [0.0, 0.5, 1.0, 0.0]]);
        assert_eq!(frame.brightness_exponent, 3.0);
        Ok(())
    }

    #[test]
    fn a_glow_reads_its_fields() -> Result<(), Box<dyn Error>> {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        let texture = fake.add_tag(BITM, "effects\\glow", &[0u8; 0x6C]);
        let mut tag = vec![0u8; GLOW_SIZE];
        put(&mut tag, 0x00, b"glow\0");
        put(&mut tag, 0x20, &12i16.to_le_bytes());
        put(&mut tag, 0x28, &5u32.to_le_bytes());
        floats(&mut tag, 0x64, &[3.0]);
        floats(&mut tag, 0x84, &[0.1, 0.3]);
        floats(&mut tag, 0xA0, &[0.02, 0.05]);
        floats(&mut tag, 0xB4, &[1.0, 0.1, 0.2, 0.3]);
        floats(&mut tag, 0xC4, &[0.5, 0.4, 0.5, 0.6]);
        floats(&mut tag, 0xF8, &[0.25]);
        reference(&mut tag, 0x144, BITM, fake.handle(texture));
        let index = fake.add_tag(GLOW, "effects\\glow", &tag);
        let cache = Cache::open(fake.build())?;
        let mut reached = Reached::default();
        let glow = import_glow(&cache, index, &mut reached)?;
        assert_eq!(glow.marker, "glow");
        assert_eq!(glow.number_of_particles, 12);
        assert_eq!(glow.flags, 5);
        assert_eq!(glow.effect_rotational_velocity, 3.0);
        assert_eq!(glow.distance_to_object, [0.1, 0.3]);
        assert_eq!(glow.particle_size, [0.02, 0.05]);
        assert_eq!(glow.color, [[0.1, 0.2, 0.3, 1.0], [0.4, 0.5, 0.6, 0.5]]);
        assert_eq!(glow.percentage_edge_fade, 0.25);
        assert!(reached.bitmaps.contains(&texture));
        Ok(())
    }
}
