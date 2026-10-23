//! The chant: an executable's sections are read and checked, the tone is
//! found by the shape of its blocks and known by its SHA-256, its ADPCM
//! decodes as an encoder made it, and without the executable there is
//! none. With the player's `Haloce_dump/default.xbe`, the retail tone is
//! found in it.

use opence_ui::chant::{
    block_runs, decode_nibble, decode_xbox_adpcm, find_tone, find_tone_by, gain, pitch_rate,
    sections, wave_file, BLOCK_BYTES, SAMPLES_PER_BLOCK, SAMPLE_RATE, STEPS, TONE_BLOCKS,
    TONE_BYTES, TONE_SHA256, VOICE_PITCHES,
};
use sha2::{Digest, Sha256};

/// One sample's nibble, as an IMA encoder chooses it.
fn encode_nibble(sample: i32, predictor: &mut i32, index: &mut i32) -> u8 {
    let step = STEPS[*index as usize];
    let mut difference = sample - *predictor;
    let mut nibble = 0u8;
    if difference < 0 {
        nibble = 8;
        difference = -difference;
    }
    let mut part = step;
    for bit in [4u8, 2, 1] {
        if difference >= part {
            nibble |= bit;
            difference -= part;
        }
        part >>= 1;
    }
    // (the decoder's own step, so that both carry the same state)
    decode_nibble(nibble, predictor, index);
    nibble
}

/// `samples` as Xbox ADPCM blocks of 64.
fn encode(samples: &[i16]) -> Vec<u8> {
    let mut out = Vec::new();
    let mut predictor = samples.first().map_or(0, |&s| i32::from(s));
    let mut index = 0i32;
    for block in samples.chunks(SAMPLES_PER_BLOCK) {
        out.extend_from_slice(&(predictor as i16).to_le_bytes());
        out.push(index as u8);
        out.push(0);
        let mut nibbles = block
            .iter()
            .map(|&sample| encode_nibble(i32::from(sample), &mut predictor, &mut index));
        for _ in 0..32 {
            let low = nibbles.next().unwrap_or(0);
            let high = nibbles.next().unwrap_or(0);
            out.push(low | high << 4);
        }
    }
    out
}

fn sine(count: usize) -> Vec<i16> {
    (0..count)
        .map(|n| (libm::sinf(n as f32 * 0.05) * 12_000.0 + 400.0) as i16)
        .collect()
}

/// An executable with one section, `.data`, holding `data`.
fn xbe(data: &[u8]) -> Vec<u8> {
    let base = 0x10000u32;
    let mut file = vec![0u8; 0x1000];
    file[..4].copy_from_slice(b"XBEH");
    file[0x104..0x108].copy_from_slice(&base.to_le_bytes());
    file[0x11C..0x120].copy_from_slice(&1u32.to_le_bytes());
    file[0x120..0x124].copy_from_slice(&(base + 0x200).to_le_bytes());
    let header = 0x200;
    file[header + 4..header + 8].copy_from_slice(&0x20000u32.to_le_bytes());
    file[header + 12..header + 16].copy_from_slice(&0x1000u32.to_le_bytes());
    file[header + 16..header + 20].copy_from_slice(&(data.len() as u32).to_le_bytes());
    file[header + 20..header + 24].copy_from_slice(&(base + 0x300).to_le_bytes());
    file[0x300..0x306].copy_from_slice(b".data\0");
    file.extend_from_slice(data);
    file
}

#[test]
fn an_executables_sections_are_read_and_checked() {
    let file = xbe(&[1, 2, 3, 4]);
    let read = sections(&file).unwrap_or_default();
    assert_eq!(read.len(), 1);
    assert_eq!(read[0].name, ".data");
    assert_eq!(
        (
            read[0].virtual_address,
            read[0].raw_address,
            read[0].raw_size
        ),
        (0x20000, 0x1000, 4)
    );
    assert_eq!(sections(b"MZ\0\0"), None, "not an executable");
    assert_eq!(
        sections(&file[..0x1002]),
        None,
        "a section past the file's end"
    );
    let mut many = file.clone();
    many[0x11C..0x120].copy_from_slice(&1000u32.to_le_bytes());
    assert_eq!(sections(&many), None);
    assert_eq!(sections(&file[..0x110]), None, "cut short");
}

#[test]
fn adpcm_decodes_as_it_was_encoded() {
    let samples = sine(SAMPLES_PER_BLOCK * 20);
    let blocks = encode(&samples);
    assert_eq!(blocks.len(), 20 * BLOCK_BYTES);
    let decoded = decode_xbox_adpcm(&blocks);
    assert_eq!(decoded.len(), samples.len());
    // (after the first few samples, while the step grows)
    let worst = samples
        .iter()
        .zip(&decoded)
        .skip(32)
        .map(|(&a, &b)| (i32::from(a) - i32::from(b)).abs())
        .max()
        .unwrap_or(i32::MAX);
    assert!(worst < 1500, "{worst}");
    assert_eq!(
        decode_xbox_adpcm(&blocks[..BLOCK_BYTES + 10]).len(),
        SAMPLES_PER_BLOCK,
        "a short block is dropped"
    );
    // a predictor and index past their ends are held in range
    let mut predictor = 32_000;
    let mut index = 88;
    assert_eq!(decode_nibble(7, &mut predictor, &mut index), 32_767);
    assert_eq!(index, 88);
    let mut low = -32_000;
    let mut first = 0;
    assert_eq!(decode_nibble(15, &mut low, &mut first), -32_011);
    assert_eq!(first, 8);
}

#[test]
fn the_tone_is_found_by_its_blocks_and_its_hash() {
    let tone = encode(&sine(TONE_BLOCKS * SAMPLES_PER_BLOCK));
    assert_eq!(tone.len(), TONE_BYTES);
    let mut data = vec![0u8; 4000];
    data.extend_from_slice(b"some other data, \xff\xff\xff\xff");
    let at = data.len();
    data.extend_from_slice(&tone);
    data.extend(vec![0u8; 100]);
    assert!(block_runs(&data, TONE_BLOCKS).contains(&at));
    assert!(
        !block_runs(&data, TONE_BLOCKS).contains(&0),
        "zeroed memory is not a tone"
    );
    assert!(block_runs(&data, 0).is_empty());
    assert!(block_runs(&data[..100], TONE_BLOCKS).is_empty());
    let file = xbe(&data);
    let digest: [u8; 32] = Sha256::digest(&tone).into();
    assert_eq!(find_tone_by(&file, &digest), Some(&tone[..]));
    assert_eq!(find_tone(&file), None, "not the retail tone");
    assert_eq!(find_tone(b"not an executable"), None);
}

#[test]
fn the_samples_are_a_wave_file() {
    let file = wave_file(&[1, -2, 3], SAMPLE_RATE);
    assert_eq!(file.len(), 44 + 6);
    assert_eq!(&file[..4], b"RIFF");
    assert_eq!(&file[8..16], b"WAVEfmt ");
    assert_eq!(
        u32::from_le_bytes([file[24], file[25], file[26], file[27]]),
        22_050
    );
    assert_eq!(&file[36..40], b"data");
    assert_eq!(&file[44..], &[1, 0, 0xfe, 0xff, 3, 0]);
}

#[test]
fn the_voices_pitches_and_volumes() {
    assert!((pitch_rate(0) - 1.0).abs() < 1e-6);
    assert!((pitch_rate(-4096) - 0.5).abs() < 1e-6);
    let rates: Vec<f32> = VOICE_PITCHES.iter().map(|&p| pitch_rate(p)).collect();
    assert!(
        rates.iter().all(|&rate| rate > 0.2 && rate < 0.6),
        "{rates:?}"
    );
    assert_eq!(gain(0.0), 1.0);
    assert!((gain(-2000.0) - 0.1).abs() < 1e-6);
    assert_eq!(gain(-10_000.0), 0.0);
    assert_eq!(gain(500.0), 1.0, "no louder than full");
}

#[test]
fn the_players_executable_has_the_retail_tone() {
    let path =
        std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../Haloce_dump/default.xbe");
    let Ok(file) = std::fs::read(path) else {
        return; // (no executable here)
    };
    let Some(tone) = find_tone(&file) else {
        panic!("the tone is not in default.xbe");
    };
    assert_eq!(Sha256::digest(tone).as_slice(), TONE_SHA256);
    assert_eq!(&tone[..4], &[0xe2, 0x0c, 0, 0]);
    let samples = decode_xbox_adpcm(tone);
    assert_eq!(samples.len(), TONE_BLOCKS * SAMPLES_PER_BLOCK);
    assert!(
        samples.iter().any(|&s| s.unsigned_abs() > 1000),
        "not silence"
    );
}
