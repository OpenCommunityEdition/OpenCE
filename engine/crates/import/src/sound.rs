//! Sounds (`snd!`) and looping sounds (`lsnd`). A sound's samples are in
//! the file, outside the tag data, and are copied as they are encoded.

use opence_content::{
    LoopingSound, LoopingSoundDetail, LoopingSoundTrack, MaterialEffect, MaterialEffects, Sound,
    SoundCompression, SoundModifiers, SoundPermutation, SoundPitchRange,
};

use crate::cache::{Cache, CacheKind};
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::object::{reference, vector};
use crate::tags::{Element, GroupTag};

pub(crate) const SND: GroupTag = GroupTag::new(b"snd!");
pub(crate) const LSND: GroupTag = GroupTag::new(b"lsnd");
pub(crate) const FOOT: GroupTag = GroupTag::new(b"foot");
const EFFE: GroupTag = GroupTag::new(b"effe");
pub(crate) const MATERIAL_EFFECTS_SIZE: usize = 0x0C;
pub(crate) const MATERIAL_EVENT_SIZE: usize = 0x1C;
pub(crate) const MATERIAL_SIZE: usize = 0x30;
const MAX_MATERIAL_EVENTS: usize = 13;
const MAX_MATERIALS: usize = 33;
pub(crate) const SOUND_SIZE: usize = 0xA4;
pub(crate) const PITCH_RANGES_OFFSET: usize = 0x98;
pub(crate) const PITCH_RANGE_SIZE: usize = 0x48;
pub(crate) const PERMUTATIONS_OFFSET: usize = 0x3C;
pub(crate) const PERMUTATION_SIZE: usize = 0x7C;
pub(crate) const SAMPLES_OFFSET: usize = 0x40;
pub(crate) const MOUTH_OFFSET: usize = 0x54;
pub(crate) const LOOPING_SOUND_SIZE: usize = 0x54;
pub(crate) const TRACKS_OFFSET: usize = 0x3C;
pub(crate) const TRACK_SIZE: usize = 0xA0;
pub(crate) const DETAILS_OFFSET: usize = 0x48;
pub(crate) const DETAIL_SIZE: usize = 0x68;
const MAX_PITCH_RANGES: usize = 8;
const MAX_PERMUTATIONS: usize = 256;
const MAX_SAMPLES: usize = 0x40_0000;
const MAX_MOUTH: usize = 8192;
const MAX_TRACKS: usize = 4;
const MAX_DETAILS: usize = 32;
const NAME_LONGEST: usize = 31;
const NO_PERMUTATION: i16 = -1;

/// A sound tag, its samples copied from the file.
pub(crate) fn import_sound(cache: &Cache, index: usize) -> Result<Sound, ImportError> {
    let sound = cache.tag(index, SND, SOUND_SIZE)?;
    let sample_rate = match sound.i16(0x06, "sample rate")? {
        0 => 22_050,
        1 => 44_100,
        rate => return Err(sound.fault("sample rate", format!("is {rate}, not 0 or 1"))),
    };
    let channels = match sound.i16(0x6C, "encoding")? {
        0 => 1,
        1 => 2,
        encoding => return Err(sound.fault("encoding", format!("is {encoding}, not 0 or 1"))),
    };
    let modifiers = |at: usize| -> Result<SoundModifiers, ImportError> {
        Ok(SoundModifiers {
            skip_fraction: sound.f32(at, "skip fraction modifier")?,
            gain: sound.f32(at + 4, "gain modifier")?,
            pitch: sound.f32(at + 8, "pitch modifier")?,
        })
    };
    let class = sound.i16(0x04, "class")?;
    let pitch_ranges = sound.block(
        PITCH_RANGES_OFFSET,
        "pitch ranges",
        PITCH_RANGE_SIZE,
        MAX_PITCH_RANGES,
    )?;
    Ok(Sound {
        tag: tag_ref(cache, index),
        flags: sound.u32(0x00, "flags")?,
        class: u16::try_from(class).map_err(|_| sound.fault("class", format!("is {class}")))?,
        sample_rate,
        channels,
        distance: vector(&sound, 0x08, "distance")?,
        skip_fraction: sound.f32(0x10, "skip fraction")?,
        random_pitch_bounds: vector(&sound, 0x14, "random pitch bounds")?,
        cone_angles: vector(&sound, 0x1C, "cone angles")?,
        outer_cone_gain: sound.f32(0x24, "outer cone gain")?,
        gain_modifier: sound.f32(0x28, "gain modifier")?,
        maximum_bend_per_second: sound.f32(0x2C, "maximum bend per second")?,
        zero: modifiers(0x3C)?,
        one: modifiers(0x54)?,
        promotion: reference(&sound, 0x70, "promotion sound", &[SND])?,
        promotion_count: u16::try_from(sound.i16(0x80, "promotion count")?).unwrap_or(0),
        longest_permutation_ms: sound.u32(0x84, "longest permutation length")?,
        pitch_ranges: pitch_ranges
            .iter()
            .map(|range| import_pitch_range(&range))
            .collect::<Result<_, _>>()?,
    })
}

fn import_pitch_range(range: &Element<'_>) -> Result<SoundPitchRange, ImportError> {
    let permutations = range.block(
        PERMUTATIONS_OFFSET,
        "permutations",
        PERMUTATION_SIZE,
        MAX_PERMUTATIONS,
    )?;
    let count = permutations.len();
    let actual = range.i16(0x2C, "actual permutation count")?;
    let natural_pitch = range.f32(0x20, "natural pitch")?;
    let playback_rate = match range.f32(0x30, "playback rate")? {
        rate if rate > 0.0 && rate.is_finite() => rate,
        _ if natural_pitch > 0.0 => 1.0 / natural_pitch,
        _ => 1.0,
    };
    Ok(SoundPitchRange {
        name: range.text(0x00, "name", NAME_LONGEST)?.to_owned(),
        natural_pitch,
        bend_bounds: vector(range, 0x24, "bend bounds")?,
        actual_permutation_count: u16::try_from(actual).unwrap_or(0),
        playback_rate,
        permutations: permutations
            .iter()
            .map(|permutation| import_permutation(&permutation, count))
            .collect::<Result<_, _>>()?,
    })
}

fn import_permutation(
    permutation: &Element<'_>,
    count: usize,
) -> Result<SoundPermutation, ImportError> {
    let compression = permutation.i16(0x28, "compression")?;
    let compression = SoundCompression::from_tag(compression)
        .ok_or_else(|| permutation.fault("compression", format!("is {compression}, not 0 to 3")))?;
    let next = permutation.i16(0x2A, "next permutation index")?;
    let next = match next {
        NO_PERMUTATION => None,
        next => Some(
            u16::try_from(next)
                .ok()
                .filter(|&next| usize::from(next) < count)
                .ok_or_else(|| {
                    permutation.fault(
                        "next permutation index",
                        format!("is {next}, not one of the {count}"),
                    )
                })?,
        ),
    };
    let mut samples = permutation
        .file_data(SAMPLES_OFFSET, "samples", MAX_SAMPLES)?
        .to_vec();
    // Custom Edition's PCM is the PC's, big-endian; the Xbox's is
    // little-endian, as the pack keeps it.
    if compression == SoundCompression::Pcm
        && permutation.cache().header().kind == CacheKind::CustomEdition
    {
        for pair in samples.as_chunks_mut::<2>().0 {
            pair.swap(0, 1);
        }
    }
    Ok(SoundPermutation {
        name: permutation.text(0x00, "name", NAME_LONGEST)?.to_owned(),
        skip_fraction: permutation.f32(0x20, "skip fraction")?,
        gain: permutation.f32(0x24, "gain")?,
        compression,
        next,
        samples,
        mouth: permutation
            .data(MOUTH_OFFSET, "mouth data", MAX_MOUTH)?
            .to_vec(),
    })
}

/// A looping sound tag.
pub(crate) fn import_looping_sound(
    cache: &Cache,
    index: usize,
) -> Result<LoopingSound, ImportError> {
    let looping = cache.tag(index, LSND, LOOPING_SOUND_SIZE)?;
    let tracks = looping.block(TRACKS_OFFSET, "tracks", TRACK_SIZE, MAX_TRACKS)?;
    let details = looping.block(DETAILS_OFFSET, "detail sounds", DETAIL_SIZE, MAX_DETAILS)?;
    Ok(LoopingSound {
        tag: tag_ref(cache, index),
        flags: looping.u32(0x00, "flags")?,
        detail_period: [
            looping.f32(0x04, "scale lower bound detail period")?,
            looping.f32(0x10, "scale upper bound detail period")?,
        ],
        tracks: tracks
            .iter()
            .map(|track| {
                Ok(LoopingSoundTrack {
                    flags: track.u32(0x00, "flags")?,
                    gain: track.f32(0x04, "gain")?,
                    fade_in: track.f32(0x08, "fade in duration")?,
                    fade_out: track.f32(0x0C, "fade out duration")?,
                    start: reference(&track, 0x30, "start", &[SND])?,
                    looping: reference(&track, 0x40, "loop", &[SND])?,
                    stop: reference(&track, 0x50, "end", &[SND])?,
                    alternate_loop: reference(&track, 0x80, "alternate loop", &[SND])?,
                    alternate_stop: reference(&track, 0x90, "alternate end", &[SND])?,
                })
            })
            .collect::<Result<_, ImportError>>()?,
        details: details
            .iter()
            .map(|detail| {
                Ok(LoopingSoundDetail {
                    sound: reference(&detail, 0x00, "sound", &[SND])?,
                    period_bounds: vector(&detail, 0x10, "random period bounds")?,
                    gain: detail.f32(0x18, "gain")?,
                    flags: detail.u32(0x1C, "flags")?,
                    theta_bounds: vector(&detail, 0x50, "yaw bounds")?,
                    phi_bounds: vector(&detail, 0x58, "pitch bounds")?,
                    distance_bounds: vector(&detail, 0x60, "distance bounds")?,
                })
            })
            .collect::<Result<_, ImportError>>()?,
    })
}

/// Every sound and looping sound in the map's own tag data (Custom
/// Edition's in the resource maps are not here, and are not imported).
pub fn import_sounds(cache: &Cache) -> Result<(Vec<Sound>, Vec<LoopingSound>), ImportError> {
    let mut sounds = Vec::new();
    let mut looping = Vec::new();
    for (index, instance) in cache.tags().iter().enumerate() {
        if instance.in_resource_map {
            continue;
        }
        if instance.group == SND {
            sounds.push(import_sound(cache, index)?);
        } else if instance.group == LSND {
            looping.push(import_looping_sound(cache, index)?);
        }
    }
    Ok((sounds, looping))
}

/// The material effects (`foot`) of tag `index`.
fn import_material_effect_tag(cache: &Cache, index: usize) -> Result<MaterialEffects, ImportError> {
    let tag = cache.tag(index, FOOT, MATERIAL_EFFECTS_SIZE)?;
    let block = tag.block(0, "effects", MATERIAL_EVENT_SIZE, MAX_MATERIAL_EVENTS)?;
    let mut events = Vec::with_capacity(block.len());
    for event in block.iter() {
        let materials = event.block(0, "materials", MATERIAL_SIZE, MAX_MATERIALS)?;
        let mut made = Vec::with_capacity(materials.len());
        for material in materials.iter() {
            made.push(MaterialEffect {
                effect: reference(&material, 0x00, "effect", &[EFFE])?,
                sound: reference(&material, 0x10, "sound", &[SND])?,
            });
        }
        events.push(made);
    }
    Ok(MaterialEffects {
        tag: tag_ref(cache, index),
        events,
    })
}

/// Every material effects tag in the map.
pub(crate) fn import_material_effects(cache: &Cache) -> Result<Vec<MaterialEffects>, ImportError> {
    let mut effects = Vec::new();
    for (index, instance) in cache.tags().iter().enumerate() {
        if instance.group == FOOT && !instance.in_resource_map {
            effects.push(import_material_effect_tag(cache, index)?);
        }
    }
    Ok(effects)
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, empty, put, reference as put_reference};
    use std::error::Error;

    type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

    fn floats(values: &[f32]) -> Vec<u8> {
        values
            .iter()
            .flat_map(|value| value.to_le_bytes())
            .collect()
    }

    /// A data field at `at` of `size` bytes at `address` in the tag data,
    /// or at `offset` in the file.
    fn data(bytes: &mut [u8], at: usize, size: usize, offset: u32, address: u32) {
        put(bytes, at, &(size as i32).to_le_bytes());
        put(bytes, at + 8, &offset.to_le_bytes());
        put(bytes, at + 12, &address.to_le_bytes());
    }

    /// A sound of one pitch range and two linked permutations, its samples
    /// in the file.
    pub(crate) fn sound_cache(rate: i16, next: i16) -> Result<(Cache, usize)> {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let samples = fake.add_file_data(&[1, 2, 3, 4, 5, 6, 7, 8]);
        let mouth = fake.add_data(&[0, 128, 255]);
        let mut permutations = Vec::new();
        for (index, name) in ["first", "second"].iter().enumerate() {
            let mut permutation = vec![0u8; PERMUTATION_SIZE];
            put(&mut permutation, 0, name.as_bytes());
            put(&mut permutation, 0x20, &floats(&[0.25, 0.8]));
            put(&mut permutation, 0x28, &1i16.to_le_bytes());
            let next = if index == 0 { next } else { NO_PERMUTATION };
            put(&mut permutation, 0x2A, &next.to_le_bytes());
            data(&mut permutation, SAMPLES_OFFSET, 8, samples, 0);
            data(&mut permutation, MOUTH_OFFSET, 3, 0, mouth);
            permutations.extend_from_slice(&permutation);
        }
        let permutations = fake.add_data(&permutations);
        let mut range = vec![0u8; PITCH_RANGE_SIZE];
        put(&mut range, 0, b"default");
        put(&mut range, 0x20, &floats(&[1.0, 0.5, 2.0]));
        put(&mut range, 0x2C, &1i16.to_le_bytes());
        block(&mut range, PERMUTATIONS_OFFSET, 2, permutations);
        let range = fake.add_data(&range);
        let mut sound = vec![0u8; SOUND_SIZE];
        put(&mut sound, 0, &Sound::LINKED_PERMUTATIONS.to_le_bytes());
        put(&mut sound, 0x04, &19i16.to_le_bytes());
        put(&mut sound, 0x06, &rate.to_le_bytes());
        put(&mut sound, 0x08, &floats(&[1.5, 20.0, 0.1, 0.9, 1.1]));
        put(&mut sound, 0x1C, &floats(&[0.5, 1.0, 0.25, 1.0, 0.0]));
        put(&mut sound, 0x3C, &floats(&[0.0, 0.5, 1.0]));
        put(&mut sound, 0x54, &floats(&[0.0, 1.0, 1.25]));
        put(&mut sound, 0x6C, &1i16.to_le_bytes());
        empty(&mut sound, 0x70);
        put(&mut sound, 0x80, &3i16.to_le_bytes());
        put(&mut sound, 0x84, &1500u32.to_le_bytes());
        block(&mut sound, PITCH_RANGES_OFFSET, 1, range);
        let index = fake.add_tag(SND, "sound\\dialog\\test", &sound);
        Ok((Cache::open(fake.build())?, index))
    }

    #[test]
    fn a_sound_has_its_fields_ranges_and_permutations() -> Result {
        let (cache, index) = sound_cache(1, 1)?;
        let sound = import_sound(&cache, index)?;
        assert_eq!(sound.flags, Sound::LINKED_PERMUTATIONS);
        assert_eq!(
            (sound.class, sound.sample_rate, sound.channels),
            (19, 44_100, 2)
        );
        assert_eq!(sound.distance, [1.5, 20.0]);
        assert_eq!(sound.skip_fraction, 0.1);
        assert_eq!(sound.random_pitch_bounds, [0.9, 1.1]);
        assert_eq!(sound.cone_angles, [0.5, 1.0]);
        assert_eq!((sound.outer_cone_gain, sound.gain_modifier), (0.25, 1.0));
        assert_eq!(sound.zero.gain, 0.5);
        assert_eq!(sound.one.pitch, 1.25);
        assert_eq!(sound.promotion, None);
        assert_eq!(
            (sound.promotion_count, sound.longest_permutation_ms),
            (3, 1500)
        );
        let range = sound.pitch_ranges.first().ok_or("no pitch range")?;
        assert_eq!(range.name, "default");
        assert_eq!(range.bend_bounds, [0.5, 2.0]);
        assert_eq!(range.actual_permutation_count, 1);
        assert_eq!(range.playback_rate, 1.0);
        let [first, second] = range.permutations.as_slice() else {
            return Err("not two permutations".into());
        };
        assert_eq!(first.name, "first");
        assert_eq!((first.skip_fraction, first.gain), (0.25, 0.8));
        assert_eq!(first.compression, SoundCompression::XboxAdpcm);
        assert_eq!((first.next, second.next), (Some(1), None));
        assert_eq!(first.samples, [1, 2, 3, 4, 5, 6, 7, 8]);
        assert_eq!(first.mouth, [0, 128, 255]);
        Ok(())
    }

    #[test]
    fn a_bad_rate_or_link_is_refused() -> Result {
        let (cache, index) = sound_cache(2, 1)?;
        assert!(import_sound(&cache, index).is_err());
        let (cache, index) = sound_cache(0, 2)?;
        assert!(import_sound(&cache, index).is_err());
        let (cache, index) = sound_cache(0, NO_PERMUTATION)?;
        assert_eq!(import_sound(&cache, index)?.sample_rate, 22_050);
        Ok(())
    }

    #[test]
    fn material_effects_have_each_events_materials() -> Result {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let mut sound = vec![0u8; SOUND_SIZE];
        empty(&mut sound, 0x70);
        let step = fake.add_tag(SND, "sound\\step", &sound);
        let handle = fake.handle(step);
        // (the run's second material, dirt, plays the step; the first
        // makes nothing)
        let mut materials = vec![0u8; 2 * MATERIAL_SIZE];
        for at in [0x00, 0x10, MATERIAL_SIZE] {
            empty(&mut materials, at);
        }
        put_reference(&mut materials, MATERIAL_SIZE + 0x10, SND, handle);
        let materials = fake.add_data(&materials);
        let mut events = vec![0u8; 2 * MATERIAL_EVENT_SIZE];
        block(&mut events, MATERIAL_EVENT_SIZE, 2, materials);
        let events = fake.add_data(&events);
        let mut foot = vec![0u8; MATERIAL_EFFECTS_SIZE];
        block(&mut foot, 0, 2, events);
        fake.add_tag(FOOT, "effects\\material effects\\cyborg", &foot);
        let cache = Cache::open(fake.build())?;

        let effects = import_material_effects(&cache)?;
        let effects = effects.first().ok_or("no material effects")?;
        assert_eq!(effects.events.len(), 2);
        assert!(effects.events[MaterialEffects::WALK].is_empty());
        let dirt = effects.of(MaterialEffects::RUN, 1).ok_or("no dirt")?;
        assert_eq!(dirt.effect, None);
        assert_eq!(
            dirt.sound.as_ref().map(|sound| sound.path.as_str()),
            Some("sound\\step")
        );
        assert_eq!(
            effects.of(MaterialEffects::RUN, 0),
            Some(&MaterialEffect::default())
        );
        assert_eq!(effects.of(MaterialEffects::RUN, -1), None);
        assert_eq!(effects.of(MaterialEffects::RUN, 2), None);
        Ok(())
    }

    #[test]
    fn a_looping_sound_has_its_tracks_and_details() -> Result {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let mut sound = vec![0u8; SOUND_SIZE];
        empty(&mut sound, 0x70);
        let looped = fake.add_tag(SND, "sound\\loop", &sound);
        let handle = fake.handle(looped);
        let mut track = vec![0u8; TRACK_SIZE];
        put(
            &mut track,
            0,
            &LoopingSoundTrack::FADE_IN_AT_START.to_le_bytes(),
        );
        put(&mut track, 0x04, &floats(&[0.75, 2.0, 3.0]));
        for at in [0x30, 0x50, 0x80, 0x90] {
            empty(&mut track, at);
        }
        put_reference(&mut track, 0x40, SND, handle);
        let tracks = fake.add_data(&track);
        let mut detail = vec![0u8; DETAIL_SIZE];
        put_reference(&mut detail, 0, SND, handle);
        put(&mut detail, 0x10, &floats(&[4.0, 8.0, 0.5]));
        put(
            &mut detail,
            0x1C,
            &LoopingSoundDetail::NOT_WITH_ALTERNATE.to_le_bytes(),
        );
        put(
            &mut detail,
            0x50,
            &floats(&[0.0, 1.0, -0.5, 0.5, 2.0, 10.0]),
        );
        let details = fake.add_data(&detail);
        let mut looping = vec![0u8; LOOPING_SOUND_SIZE];
        put(&mut looping, 0, &LoopingSound::STOPS_MUSIC.to_le_bytes());
        put(&mut looping, 0x04, &0.5f32.to_le_bytes());
        put(&mut looping, 0x10, &2.0f32.to_le_bytes());
        block(&mut looping, TRACKS_OFFSET, 1, tracks);
        block(&mut looping, DETAILS_OFFSET, 1, details);
        fake.add_tag(LSND, "sound\\music\\test", &looping);
        let cache = Cache::open(fake.build())?;

        let (sounds, looping) = import_sounds(&cache)?;
        assert_eq!(sounds.len(), 1);
        let looping = looping.first().ok_or("no looping sound")?;
        assert_eq!(looping.flags, LoopingSound::STOPS_MUSIC);
        assert_eq!(looping.detail_period, [0.5, 2.0]);
        let track = looping.tracks.first().ok_or("no track")?;
        assert_eq!(
            (track.gain, track.fade_in, track.fade_out),
            (0.75, 2.0, 3.0)
        );
        assert_eq!(
            track.looping.as_ref().map(|t| t.path.as_str()),
            Some("sound\\loop")
        );
        assert_eq!((track.start.clone(), track.stop.clone()), (None, None));
        let detail = looping.details.first().ok_or("no detail")?;
        assert!(detail.sound.is_some());
        assert_eq!((detail.period_bounds, detail.gain), ([4.0, 8.0], 0.5));
        assert_eq!(detail.flags, LoopingSoundDetail::NOT_WITH_ALTERNATE);
        assert_eq!(detail.phi_bounds, [-0.5, 0.5]);
        assert_eq!(detail.distance_bounds, [2.0, 10.0]);
        Ok(())
    }
}
