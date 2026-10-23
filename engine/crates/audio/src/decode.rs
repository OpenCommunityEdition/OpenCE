//! The decoders of a permutation's samples: Xbox ADPCM (the stock maps'),
//! 16-bit PCM, and Ogg Vorbis (Custom Edition's), to interleaved 16-bit
//! samples.

use std::io::Cursor;

use lewton::inside_ogg::OggStreamReader;
use opence_content::SoundCompression;

/// The bytes of one Xbox ADPCM block, a channel.
pub const ADPCM_BLOCK_BYTES: usize = 36;
/// The samples of one Xbox ADPCM block, a channel.
pub const ADPCM_BLOCK_SAMPLES: usize = 64;

/// A permutation's samples, decoded.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Decoded {
    /// The samples' rate.
    pub sample_rate: u32,
    /// The channels, 1 or 2.
    pub channels: u8,
    /// The samples, the channels interleaved.
    pub samples: Vec<i16>,
}

impl Decoded {
    /// The number of frames (samples a channel).
    pub fn frames(&self) -> usize {
        self.samples.len() / usize::from(self.channels.max(1))
    }
}

/// Why samples could not be decoded.
#[derive(Debug, thiserror::Error)]
pub enum DecodeError {
    /// A compression the decoders do not read.
    #[error("{0:?} samples are not decoded")]
    Unsupported(SoundCompression),
    /// A channel count other than 1 or 2.
    #[error("{0} channels")]
    Channels(u8),
    /// An Ogg Vorbis stream that does not decode.
    #[error("the Ogg Vorbis stream: {0}")]
    Ogg(String),
}

/// Decodes `bytes`, of `channels` channels at `sample_rate`, as
/// `compression` encodes them. An Ogg Vorbis stream gives its own rate and
/// channels.
pub fn decode(
    compression: SoundCompression,
    sample_rate: u32,
    channels: u8,
    bytes: &[u8],
) -> Result<Decoded, DecodeError> {
    if compression == SoundCompression::Ogg {
        return decode_ogg(bytes);
    }
    if !(1..=2).contains(&channels) {
        return Err(DecodeError::Channels(channels));
    }
    let samples = match compression {
        SoundCompression::Pcm => decode_pcm(bytes),
        SoundCompression::XboxAdpcm => decode_adpcm(bytes, usize::from(channels)),
        SoundCompression::ImaAdpcm | SoundCompression::Ogg => {
            return Err(DecodeError::Unsupported(compression))
        }
    };
    Ok(Decoded {
        sample_rate,
        channels,
        samples,
    })
}

/// Little-endian 16-bit samples; a last odd byte is dropped.
fn decode_pcm(bytes: &[u8]) -> Vec<i16> {
    bytes
        .as_chunks::<2>()
        .0
        .iter()
        .map(|pair| i16::from_le_bytes(*pair))
        .collect()
}

const IMA_INDEX: [i32; 16] = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8];

const IMA_STEP: [i32; 89] = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272,
    2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
];

/// The IMA ADPCM decoder's state, a channel.
struct Ima {
    predictor: i32,
    index: usize,
}

impl Ima {
    /// The sample a nibble codes.
    fn expand(&mut self, nibble: u8) -> i16 {
        let step = IMA_STEP[self.index];
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
        self.predictor = (self.predictor + difference).clamp(-32768, 32767);
        let index = i32::try_from(self.index).unwrap_or(0) + IMA_INDEX[usize::from(nibble & 15)];
        self.index = usize::try_from(index.clamp(0, 88)).unwrap_or(0);
        i16::try_from(self.predictor).unwrap_or(0)
    }
}

/// Xbox ADPCM: each block has, a channel, a 4-byte header (the block's
/// first sample and the step index), then 4-byte groups of eight nibbles,
/// low nibble first, the channels' groups alternating. The nibbles code the
/// 63 samples after the header's; the 64th only pads the block. A last
/// partial block is dropped.
fn decode_adpcm(bytes: &[u8], channels: usize) -> Vec<i16> {
    let block_bytes = ADPCM_BLOCK_BYTES * channels;
    let blocks = bytes.len() / block_bytes;
    let mut samples = vec![0i16; blocks * ADPCM_BLOCK_SAMPLES * channels];
    for (data, output) in bytes
        .chunks_exact(block_bytes)
        .zip(samples.chunks_exact_mut(ADPCM_BLOCK_SAMPLES * channels))
    {
        for channel in 0..channels {
            let header = &data[channel * 4..channel * 4 + 4];
            let first = i16::from_le_bytes([header[0], header[1]]);
            let mut ima = Ima {
                predictor: i32::from(first),
                index: usize::from(header[2].min(88)),
            };
            output[channel] = first;
            for group in 0..8 {
                let at = 4 * channels + (group * channels + channel) * 4;
                for (byte, &nibbles) in data[at..at + 4].iter().enumerate() {
                    // nibble n codes sample n + 1
                    let sample = group * 8 + byte * 2 + 1;
                    output[sample * channels + channel] = ima.expand(nibbles & 15);
                    if sample + 1 < ADPCM_BLOCK_SAMPLES {
                        output[(sample + 1) * channels + channel] = ima.expand(nibbles >> 4);
                    }
                }
            }
        }
    }
    samples
}

/// An Ogg Vorbis stream, at its own rate and channels.
fn decode_ogg(bytes: &[u8]) -> Result<Decoded, DecodeError> {
    let mut reader =
        OggStreamReader::new(Cursor::new(bytes)).map_err(|e| DecodeError::Ogg(e.to_string()))?;
    let channels = reader.ident_hdr.audio_channels;
    if !(1..=2).contains(&channels) {
        return Err(DecodeError::Channels(channels));
    }
    let sample_rate = reader.ident_hdr.audio_sample_rate;
    let mut samples = Vec::new();
    while let Some(packet) = reader
        .read_dec_packet_itl()
        .map_err(|e| DecodeError::Ogg(e.to_string()))?
    {
        samples.extend_from_slice(&packet);
    }
    // The last page's granule position is the stream's length: the decoder
    // gives the last packet whole, padding and all, which would put a gap
    // in a loop.
    if let Some(length) = reader
        .get_last_absgp()
        .and_then(|frames| usize::try_from(frames).ok())
    {
        samples.truncate(length.saturating_mul(usize::from(channels)));
    }
    Ok(Decoded {
        sample_rate,
        channels,
        samples,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::error::Error;

    #[test]
    fn pcm_is_little_endian() -> Result<(), Box<dyn Error>> {
        let decoded = decode(
            SoundCompression::Pcm,
            22050,
            2,
            &[1, 0, 0xff, 0xff, 0, 0x80, 9],
        )?;
        assert_eq!(decoded.samples, vec![1, -1, -32768]);
        assert_eq!(decoded.sample_rate, 22050);
        assert_eq!(decoded.frames(), 1);
        Ok(())
    }

    #[test]
    fn an_adpcm_block_decodes_to_known_samples() -> Result<(), Box<dyn Error>> {
        // One mono block: the first sample 1000 at step index 0, then
        // nibbles of 7 (up a step and more) and of 15 (down).
        let mut block = vec![0u8; ADPCM_BLOCK_BYTES];
        block[0..2].copy_from_slice(&1000i16.to_le_bytes());
        block[2] = 0;
        block[4] = 0x77;
        block[5] = 0xff;
        let decoded = decode(SoundCompression::XboxAdpcm, 22050, 1, &block)?;
        assert_eq!(decoded.frames(), 64);
        // Step 7: 7>>3 + 7>>2 + 7>>1 + 7 = 11 up, and a 7 climbs the
        // index by 8: steps 7, 16, 34 and 73, the last two down.
        assert_eq!(&decoded.samples[..5], &[1000, 1011, 1041, 978, 842]);
        Ok(())
    }

    #[test]
    fn adpcm_channels_alternate_by_group() -> Result<(), Box<dyn Error>> {
        let mut block = vec![0u8; ADPCM_BLOCK_BYTES * 2];
        block[0..2].copy_from_slice(&100i16.to_le_bytes());
        block[4..6].copy_from_slice(&(-100i16).to_le_bytes());
        // The right channel's first group follows the left's.
        block[12] = 0x01;
        let decoded = decode(SoundCompression::XboxAdpcm, 44100, 2, &block)?;
        assert_eq!(decoded.frames(), 64);
        assert_eq!(&decoded.samples[..4], &[100, -100, 100, -99]);
        // A partial block is dropped.
        let short = decode(SoundCompression::XboxAdpcm, 44100, 2, &block[..70])?;
        assert!(short.samples.is_empty());
        Ok(())
    }

    #[test]
    fn what_is_not_decoded_is_refused() {
        assert!(matches!(
            decode(SoundCompression::ImaAdpcm, 22050, 1, &[]),
            Err(DecodeError::Unsupported(SoundCompression::ImaAdpcm))
        ));
        assert!(matches!(
            decode(SoundCompression::Pcm, 22050, 3, &[]),
            Err(DecodeError::Channels(3))
        ));
        assert!(matches!(
            decode(SoundCompression::Ogg, 0, 0, b"not ogg"),
            Err(DecodeError::Ogg(_))
        ));
    }

    #[test]
    fn an_ogg_stream_decodes_to_its_sine() -> Result<(), Box<dyn Error>> {
        let bytes = include_bytes!("../tests/fixtures/sine.ogg");
        let decoded = decode(SoundCompression::Ogg, 0, 0, bytes)?;
        assert_eq!(decoded.sample_rate, 22050);
        assert_eq!(decoded.channels, 1);
        // A tenth of a second of a 441 Hz sine at half scale.
        assert!(decoded.frames().abs_diff(2205) < 64, "{}", decoded.frames());
        let peak = decoded
            .samples
            .iter()
            .map(|s| s.unsigned_abs())
            .max()
            .unwrap_or(0);
        assert!((14000..19000).contains(&peak), "peak {peak}");
        let crossings = decoded
            .samples
            .windows(2)
            .filter(|w| (w[0] < 0) != (w[1] < 0))
            .count();
        // 44 cycles cross zero twice each.
        assert!((84..=92).contains(&crossings), "{crossings} crossings");
        Ok(())
    }
}
