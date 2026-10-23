//! The loading screen's chant (source/interface/progress_bar.c): a tone of
//! 511 blocks of Xbox ADPCM that the game carries in its executable, which
//! four voices play at once, each at its own pitch.
//!
//! The engine ships none of it. The tone is found in the player's own
//! `default.xbe`, by the shape of its blocks, and known by its SHA-256; the
//! samples go into the cache only. Without the executable there is no
//! chant, and the loading screen is silent.

use sha2::{Digest, Sha256};

/// How many blocks the tone has.
pub const TONE_BLOCKS: usize = 511;
/// The bytes of a block: a 4-byte header and 32 bytes of nibbles.
pub const BLOCK_BYTES: usize = 36;
/// The samples a block decodes to (the header's sample is not one).
pub const SAMPLES_PER_BLOCK: usize = 64;
/// The tone's rate, in samples a second.
pub const SAMPLE_RATE: u32 = 22_050;
/// The tone's bytes in the executable.
pub const TONE_BYTES: usize = TONE_BLOCKS * BLOCK_BYTES;
/// The SHA-256 of the tone's bytes in the retail executable.
pub const TONE_SHA256: [u8; 32] = [
    0x4f, 0x61, 0xe8, 0x27, 0xbe, 0x9b, 0x54, 0xb7, 0x1e, 0x3d, 0x37, 0xa1, 0x45, 0xc9, 0xb8, 0x8f,
    0xb3, 0x2e, 0xaf, 0x51, 0x7e, 0x23, 0x74, 0x6a, 0xf8, 0x52, 0x36, 0xca, 0x41, 0xba, 0x8f, 0xc9,
];

/// The voices' pitches, in DirectSound's 1/4096 of an octave.
pub const VOICE_PITCHES: [i32; 4] = [-4681, -6386, -3999, -8773];

/// A section of an executable.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Section {
    /// Its name, such as `.data`.
    pub name: String,
    /// Where it is loaded.
    pub virtual_address: u32,
    /// Where its bytes are in the file.
    pub raw_address: u32,
    /// How many bytes it has in the file.
    pub raw_size: u32,
}

const XBE_MAGIC: &[u8; 4] = b"XBEH";
const BASE_ADDRESS: usize = 0x104;
const SECTION_COUNT: usize = 0x11C;
const SECTION_HEADERS: usize = 0x120;
const SECTION_HEADER_BYTES: usize = 56;
const MAXIMUM_SECTIONS: usize = 256;

fn u32_at(bytes: &[u8], at: usize) -> Option<u32> {
    let word = bytes.get(at..at.checked_add(4)?)?;
    Some(u32::from_le_bytes([word[0], word[1], word[2], word[3]]))
}

fn name_at(bytes: &[u8], at: usize) -> Option<String> {
    let rest = bytes.get(at..)?;
    let end = rest.iter().take(64).position(|&b| b == 0)?;
    Some(String::from_utf8_lossy(&rest[..end]).into_owned())
}

/// The sections of an Xbox executable, each checked to lie in the file;
/// none for a file that is not one.
pub fn sections(xbe: &[u8]) -> Option<Vec<Section>> {
    if xbe.get(..4)? != XBE_MAGIC {
        return None;
    }
    let base = u32_at(xbe, BASE_ADDRESS)? as usize;
    let count = u32_at(xbe, SECTION_COUNT)? as usize;
    let headers = (u32_at(xbe, SECTION_HEADERS)? as usize).checked_sub(base)?;
    if count > MAXIMUM_SECTIONS {
        return None;
    }
    let mut sections = Vec::with_capacity(count);
    for index in 0..count {
        let header = headers.checked_add(index * SECTION_HEADER_BYTES)?;
        let virtual_address = u32_at(xbe, header + 4)?;
        let raw_address = u32_at(xbe, header + 12)?;
        let raw_size = u32_at(xbe, header + 16)?;
        let name = name_at(xbe, (u32_at(xbe, header + 20)? as usize).checked_sub(base)?)?;
        let end = (raw_address as usize).checked_add(raw_size as usize)?;
        if end > xbe.len() {
            return None;
        }
        sections.push(Section {
            name,
            virtual_address,
            raw_address,
            raw_size,
        });
    }
    Some(sections)
}

fn header_is_valid(header: &[u8]) -> bool {
    header.len() >= 4 && header[2] <= 88 && header[3] == 0
}

/// Where in `data` runs of `blocks` blocks of ADPCM start: each block's
/// header has a step index of at most 88 and a zero reserved byte, and the
/// first one's sample is not zero (zeroed memory has the shape too).
pub fn block_runs(data: &[u8], blocks: usize) -> Vec<usize> {
    let span = blocks * BLOCK_BYTES;
    if blocks == 0 || data.len() < span {
        return Vec::new();
    }
    // run[k]: how many valid headers follow one another from k
    let mut run = vec![0u32; data.len() + BLOCK_BYTES];
    for at in (0..data.len()).rev() {
        if header_is_valid(&data[at..]) {
            run[at] = 1 + run[at + BLOCK_BYTES];
        }
    }
    (0..=data.len() - span)
        .filter(|&at| run[at] as usize >= blocks && (data[at] != 0 || data[at + 1] != 0))
        .collect()
}

/// The tone's bytes in an executable: the run of 511 blocks in one of its
/// sections whose SHA-256 is the retail tone's.
pub fn find_tone(xbe: &[u8]) -> Option<&[u8]> {
    find_tone_by(xbe, &TONE_SHA256)
}

/// The tone with the SHA-256 `digest` (a test's own).
pub fn find_tone_by<'a>(xbe: &'a [u8], digest: &[u8; 32]) -> Option<&'a [u8]> {
    for section in sections(xbe)? {
        let start = section.raw_address as usize;
        let data = &xbe[start..start + section.raw_size as usize];
        for at in block_runs(data, TONE_BLOCKS) {
            let tone = &data[at..at + TONE_BYTES];
            if Sha256::digest(tone).as_slice() == digest {
                return Some(tone);
            }
        }
    }
    None
}

/// IMA ADPCM's step sizes.
pub const STEPS: [i32; 89] = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272,
    2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
];

/// How a nibble moves the step index.
pub const INDEX_STEPS: [i32; 8] = [-1, -1, -1, -1, 2, 4, 6, 8];

/// One nibble's sample: the predictor and the step index carried on.
pub fn decode_nibble(nibble: u8, predictor: &mut i32, index: &mut i32) -> i16 {
    let step = STEPS[*index as usize];
    let mut difference = step >> 3;
    if nibble & 1 != 0 {
        difference += step >> 2;
    }
    if nibble & 2 != 0 {
        difference += step >> 1;
    }
    if nibble & 4 != 0 {
        difference += step;
    }
    if nibble & 8 != 0 {
        difference = -difference;
    }
    *predictor = (*predictor + difference).clamp(-32768, 32767);
    *index = (*index + INDEX_STEPS[(nibble & 7) as usize]).clamp(0, 88);
    *predictor as i16
}

/// Mono Xbox ADPCM's samples: each 36-byte block seeds the predictor and
/// the step index from its header, then gives 64 samples, the low nibble
/// of each byte first. A short last block is dropped.
pub fn decode_xbox_adpcm(data: &[u8]) -> Vec<i16> {
    let mut samples = Vec::with_capacity(data.len() / BLOCK_BYTES * SAMPLES_PER_BLOCK);
    for block in data.as_chunks::<BLOCK_BYTES>().0 {
        let mut predictor = i32::from(i16::from_le_bytes([block[0], block[1]]));
        let mut index = i32::from(block[2]).min(88);
        for &byte in &block[4..] {
            samples.push(decode_nibble(byte & 0x0f, &mut predictor, &mut index));
            samples.push(decode_nibble(byte >> 4, &mut predictor, &mut index));
        }
    }
    samples
}

/// The samples as a 16-bit mono WAVE file.
pub fn wave_file(samples: &[i16], rate: u32) -> Vec<u8> {
    let data = (samples.len() * 2) as u32;
    let mut file = Vec::with_capacity(44 + data as usize);
    file.extend_from_slice(b"RIFF");
    file.extend_from_slice(&(36 + data).to_le_bytes());
    file.extend_from_slice(b"WAVEfmt ");
    file.extend_from_slice(&16u32.to_le_bytes());
    file.extend_from_slice(&1u16.to_le_bytes()); // PCM
    file.extend_from_slice(&1u16.to_le_bytes()); // mono
    file.extend_from_slice(&rate.to_le_bytes());
    file.extend_from_slice(&(rate * 2).to_le_bytes());
    file.extend_from_slice(&2u16.to_le_bytes());
    file.extend_from_slice(&16u16.to_le_bytes());
    file.extend_from_slice(b"data");
    file.extend_from_slice(&data.to_le_bytes());
    for sample in samples {
        file.extend_from_slice(&sample.to_le_bytes());
    }
    file
}

/// A voice's rate against the tone's: 2 to the power of its pitch in
/// 1/4096 of an octave.
pub fn pitch_rate(pitch: i32) -> f32 {
    libm::exp2f(pitch as f32 / 4096.0)
}

/// A gain for a volume in hundredths of a decibel (0 is full, -10000
/// silence).
pub fn gain(millibels: f32) -> f32 {
    if millibels <= -10_000.0 {
        0.0
    } else {
        libm::powf(10.0, millibels.min(0.0) / 2000.0)
    }
}
