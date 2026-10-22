//! A texture's texels decoded, for a material's mean colour.
//!
//! The bake needs only how much light a surface reflects on average, so it
//! decodes a texture's smallest whole level (a few texels, already the
//! mean of the largest) rather than every level. A compressed texture's
//! levels below one block are not kept in the maps (their bytes are
//! zero), so the smallest level it reads is a block or larger.

use opence_content::material::linear;
use opence_content::{Texture, TextureFormat};

/// A texel: red, green, blue and alpha, as stored (display values).
pub type Texel = [u8; 4];

/// The mean colour of `texture`'s first face at its smallest level of a
/// block or more (any level for an uncompressed format): red,
/// green and blue linear, alpha as stored, each 0 to 1. None if its bytes
/// are too few for its size.
pub fn mean_color(texture: &Texture) -> Option<[f32; 4]> {
    let whole = |level: &usize| {
        let [width, height, _] = texture.level_size(*level);
        texture.format == TextureFormat::Rgba8 || (width >= 4 && height >= 4)
    };
    let level = (0..texture.levels.len()).rev().find(whole).unwrap_or(0);
    let [width, height, _] = texture.level_size(level);
    let texels = decode(texture.format, width, height, texture.levels.get(level)?)?;
    let mut sum = [0.0f64; 4];
    for texel in &texels {
        for (channel, value) in texel.iter().enumerate() {
            let value = f32::from(*value) / 255.0;
            let value = if channel < 3 { linear(value) } else { value };
            sum[channel] += f64::from(value);
        }
    }
    let count = texels.len().max(1) as f64;
    Some(sum.map(|sum| (sum / count) as f32))
}

/// The texels of a `width` by `height` image of `format`, row by row; None
/// if `bytes` are too few.
pub fn decode(format: TextureFormat, width: u32, height: u32, bytes: &[u8]) -> Option<Vec<Texel>> {
    let (width, height) = (width as usize, height as usize);
    let mut texels = Vec::with_capacity(width * height);
    if format == TextureFormat::Rgba8 {
        texels.extend_from_slice(bytes.get(..width * height * 4)?.as_chunks::<4>().0);
        return Some(texels);
    }
    let block_bytes = if format == TextureFormat::Bc1 { 8 } else { 16 };
    let blocks_across = width.div_ceil(4);
    let mut blocks = Vec::with_capacity(blocks_across);
    for row in 0..height.div_ceil(4) {
        blocks.clear();
        for column in 0..blocks_across {
            let at = (row * blocks_across + column) * block_bytes;
            blocks.push(decode_block(format, bytes.get(at..at + block_bytes)?));
        }
        for y in row * 4..(row * 4 + 4).min(height) {
            for x in 0..width {
                texels.push(blocks[x / 4][(y % 4) * 4 + x % 4]);
            }
        }
    }
    Some(texels)
}

/// A 4x4 block's sixteen texels, row by row. `block` is the format's 8 or
/// 16 bytes.
fn decode_block(format: TextureFormat, block: &[u8]) -> [Texel; 16] {
    match format {
        TextureFormat::Bc1 => colors(block, true),
        TextureFormat::Bc2 => {
            let mut texels = colors(&block[8..], false);
            let alphas = u64::from_le_bytes(word(block, 0));
            for (index, texel) in texels.iter_mut().enumerate() {
                // (four bits each, scaled to eight)
                texel[3] = ((alphas >> (index * 4)) & 0xF) as u8 * 17;
            }
            texels
        }
        TextureFormat::Bc3 => {
            let mut texels = colors(&block[8..], false);
            let alphas = interpolated_alphas(block[0], block[1]);
            let mut indices = [0u8; 8];
            indices[..6].copy_from_slice(&block[2..8]);
            let indices = u64::from_le_bytes(indices);
            for (index, texel) in texels.iter_mut().enumerate() {
                texel[3] = alphas[((indices >> (index * 3)) & 0b111) as usize];
            }
            texels
        }
        TextureFormat::Rgba8 => [[0; 4]; 16],
    }
}

/// The eight bytes of `bytes` from `at`.
fn word(bytes: &[u8], at: usize) -> [u8; 8] {
    let mut word = [0; 8];
    word.copy_from_slice(&bytes[at..at + 8]);
    word
}

/// A BC3 block's eight alphas, from its two end points.
fn interpolated_alphas(first: u8, second: u8) -> [u8; 8] {
    let (a, b) = (u32::from(first), u32::from(second));
    let mut alphas = [first, second, 0, 0, 0, 0, 0, 255];
    if first > second {
        for step in 1..7u32 {
            alphas[step as usize + 1] = (((7 - step) * a + step * b) / 7) as u8;
        }
    } else {
        for step in 1..5u32 {
            alphas[step as usize + 1] = (((5 - step) * a + step * b) / 5) as u8;
        }
    }
    alphas
}

/// A BC1 colour block's sixteen texels (opaque but for BC1's transparent
/// black, which only `one_bit_alpha` blocks have).
fn colors(block: &[u8], one_bit_alpha: bool) -> [Texel; 16] {
    let first = u16::from_le_bytes([block[0], block[1]]);
    let second = u16::from_le_bytes([block[2], block[3]]);
    let (a, b) = (expand(first), expand(second));
    let mix = |left: u32, right: u32, of: u32| -> [u8; 4] {
        let channel = |c: usize| ((left * u32::from(a[c]) + right * u32::from(b[c])) / of) as u8;
        [channel(0), channel(1), channel(2), 255]
    };
    let palette = if first > second || !one_bit_alpha {
        [a, b, mix(2, 1, 3), mix(1, 2, 3)]
    } else {
        [a, b, mix(1, 1, 2), [0, 0, 0, 0]]
    };
    let indices = u32::from_le_bytes([block[4], block[5], block[6], block[7]]);
    std::array::from_fn(|index| palette[((indices >> (index * 2)) & 0b11) as usize])
}

/// A 5:6:5 colour as eight bits a channel, opaque.
fn expand(color: u16) -> Texel {
    let red = u32::from(color >> 11) & 0x1F;
    let green = u32::from(color >> 5) & 0x3F;
    let blue = u32::from(color) & 0x1F;
    [
        ((red * 255 + 15) / 31) as u8,
        ((green * 255 + 31) / 63) as u8,
        ((blue * 255 + 15) / 31) as u8,
        255,
    ]
}

#[cfg(test)]
mod tests {
    use super::*;

    fn texture(format: TextureFormat, width: u16, height: u16, levels: Vec<Vec<u8>>) -> Texture {
        Texture {
            format,
            width,
            height,
            depth: 1,
            faces: 1,
            levels,
        }
    }

    #[test]
    fn rgba8_texels_are_read_as_they_are() {
        let bytes = [255, 0, 0, 255, 0, 0, 255, 0];
        assert_eq!(
            decode(TextureFormat::Rgba8, 2, 1, &bytes),
            Some(vec![[255, 0, 0, 255], [0, 0, 255, 0]])
        );
        assert_eq!(decode(TextureFormat::Rgba8, 3, 1, &bytes), None);
    }

    #[test]
    fn bc1_blocks_interpolate_their_end_points() {
        // white and black, four-colour: indices 0, 1, 2, 3 then all 0
        let mut block = vec![0xFF, 0xFF, 0x00, 0x00];
        block.extend_from_slice(&0b1110_0100u32.to_le_bytes());
        let texels = decode(TextureFormat::Bc1, 4, 4, &block).unwrap_or_default();
        assert_eq!(texels[0], [255; 4]);
        assert_eq!(texels[1], [0, 0, 0, 255]);
        assert_eq!(texels[2], [170, 170, 170, 255]);
        assert_eq!(texels[3], [85, 85, 85, 255]);
        assert_eq!(texels[4], [255; 4]);
        // black then white: three colours and transparent black
        let mut block = vec![0x00, 0x00, 0xFF, 0xFF];
        block.extend_from_slice(&0b1110_0100u32.to_le_bytes());
        let texels = decode(TextureFormat::Bc1, 4, 4, &block).unwrap_or_default();
        assert_eq!(texels[2], [127, 127, 127, 255]);
        assert_eq!(texels[3], [0; 4]);
    }

    #[test]
    fn bc2_and_bc3_carry_their_alpha() {
        let color = [0x1F, 0x00, 0x1F, 0x00, 0, 0, 0, 0];
        let mut bc2 = 0xF0u64.to_le_bytes().to_vec();
        bc2.extend_from_slice(&color);
        let texels = decode(TextureFormat::Bc2, 4, 4, &bc2).unwrap_or_default();
        assert_eq!(texels[0], [0, 0, 255, 0]);
        assert_eq!(texels[1], [0, 0, 255, 255]);
        // alphas 255 and 0, eight steps: indices 0, 1, 2 then 0
        let mut bc3 = vec![255, 0, 0b1000_1000, 0, 0, 0, 0, 0];
        bc3.extend_from_slice(&color);
        let texels = decode(TextureFormat::Bc3, 4, 4, &bc3).unwrap_or_default();
        assert_eq!([texels[0][3], texels[1][3], texels[2][3]], [255, 0, 218]);
        // (six steps, then 0 and 255)
        assert_eq!(
            interpolated_alphas(0, 255),
            [0, 255, 51, 102, 153, 204, 0, 255]
        );
    }

    #[test]
    fn a_partial_block_gives_only_the_image_s_texels() {
        let mut block = vec![0xFF, 0xFF, 0x00, 0x00];
        block.extend_from_slice(&0u32.to_le_bytes());
        let texels = decode(TextureFormat::Bc1, 2, 1, &block).unwrap_or_default();
        assert_eq!(texels, vec![[255; 4]; 2]);
        assert_eq!(decode(TextureFormat::Bc1, 8, 4, &block), None);
    }

    #[test]
    fn the_mean_colour_is_linear_and_from_the_smallest_level() {
        // a 2x1 level of red and black over a 1x1 level of mid grey
        let large = vec![255, 0, 0, 255, 0, 0, 0, 255];
        let small = vec![128, 128, 128, 64];
        let mean = mean_color(&texture(
            TextureFormat::Rgba8,
            2,
            1,
            vec![large.clone(), small],
        ));
        let grey = linear(128.0 / 255.0);
        assert_eq!(mean, Some([grey, grey, grey, 64.0 / 255.0]));
        let mean = mean_color(&texture(TextureFormat::Rgba8, 2, 1, vec![large]));
        assert_eq!(mean, Some([0.5, 0.0, 0.0, 1.0]));
        assert_eq!(
            mean_color(&texture(TextureFormat::Rgba8, 2, 1, Vec::new())),
            None
        );
    }

    #[test]
    fn a_compressed_texture_s_mean_skips_its_unkept_levels() {
        // a white 4x4 block, then the 2x2 and 1x1 levels as the maps keep
        // them: zero
        let mut white = vec![0xFF, 0xFF, 0xFF, 0xFF];
        white.extend_from_slice(&0u32.to_le_bytes());
        let mean = mean_color(&texture(
            TextureFormat::Bc1,
            4,
            4,
            vec![white, vec![0; 8], vec![0; 8]],
        ));
        assert_eq!(mean, Some([1.0; 4]));
    }
}
