//! Converting an image's pixels, as a cache stores them, to a texture the
//! GPU takes: unswizzled, DXT kept as BC, everything else widened to RGBA8
//! (P8 bump maps through the game's vector palette), with a full chain of
//! mipmaps made for an uncompressed texture stored without one.
//!
//! An Xbox cache stores each image as Direct3D lays it out: uncompressed
//! levels swizzled (Morton order, one level after another) unless linear,
//! DXT levels as plain rows of blocks, and a cube map as six faces, each
//! with its whole chain of levels, each but the last padded to 128 bytes.
//! Halo PC's levels are plain rows, a cube map's six faces within each
//! level.

use opence_content::{Texture, TextureFormat};

use crate::cache::CacheKind;
use crate::vector_palette::VECTOR_PALETTE;

const A8: i16 = 0;
const Y8: i16 = 1;
const AY8: i16 = 2;
const A8Y8: i16 = 3;
const R5G6B5: i16 = 6;
const A1R5G5B5: i16 = 8;
const A4R4G4B4: i16 = 9;
const X8R8G8B8: i16 = 10;
const A8R8G8B8: i16 = 11;
const DXT1: i16 = 14;
const DXT3: i16 = 15;
const DXT5: i16 = 16;
const P8_BUMP: i16 = 17;

const TEXTURE_2D: i16 = 0;
const TEXTURE_3D: i16 = 1;
const CUBE_MAP: i16 = 2;
const LINEAR_FLAG: u16 = 1 << 4;
const XBOX_CUBE_FACE_ALIGNMENT: usize = 128;
const XBOX_PITCH_ALIGNMENT: usize = 64;

/// An image as its tag gives it, with its pixels.
pub(crate) struct StoredImage<'a> {
    pub width: u16,
    pub height: u16,
    pub depth: u16,
    pub kind: i16,
    pub format: i16,
    pub flags: u16,
    pub mipmap_count: u16,
    pub pixels: &'a [u8],
}

#[derive(Clone, Copy)]
enum Texel {
    /// Kept: 4x4 blocks of this many bytes.
    Block(TextureFormat, usize),
    /// Widened to RGBA8 from this many bytes a texel.
    Bytes(usize),
}

fn texel(format: i16) -> Result<Texel, String> {
    Ok(match format {
        DXT1 => Texel::Block(TextureFormat::Bc1, 8),
        DXT3 => Texel::Block(TextureFormat::Bc2, 16),
        DXT5 => Texel::Block(TextureFormat::Bc3, 16),
        A8 | Y8 | AY8 | P8_BUMP => Texel::Bytes(1),
        A8Y8 | R5G6B5 | A1R5G5B5 | A4R4G4B4 => Texel::Bytes(2),
        X8R8G8B8 | A8R8G8B8 => Texel::Bytes(4),
        other => return Err(format!("is {other}, not a format the game draws")),
    })
}

fn expand5(value: u16) -> u8 {
    ((value << 3) | (value >> 2)) as u8
}

fn expand6(value: u16) -> u8 {
    ((value << 2) | (value >> 4)) as u8
}

fn expand4(value: u16) -> u8 {
    (value * 0x11) as u8
}

fn argb(value: u32) -> [u8; 4] {
    let [b, g, r, a] = value.to_le_bytes();
    [r, g, b, a]
}

/// One texel, as red, green, blue and alpha.
fn rgba(format: i16, texel: &[u8]) -> [u8; 4] {
    let byte = |at: usize| texel.get(at).copied().unwrap_or(0);
    let short = u16::from_le_bytes([byte(0), byte(1)]);
    match format {
        A8 => [255, 255, 255, byte(0)],
        Y8 => [byte(0), byte(0), byte(0), 255],
        AY8 => [byte(0); 4],
        A8Y8 => [byte(0), byte(0), byte(0), byte(1)],
        R5G6B5 => [
            expand5(short >> 11),
            expand6((short >> 5) & 0x3F),
            expand5(short & 0x1F),
            255,
        ],
        A1R5G5B5 => [
            expand5((short >> 10) & 0x1F),
            expand5((short >> 5) & 0x1F),
            expand5(short & 0x1F),
            if short & 0x8000 != 0 { 255 } else { 0 },
        ],
        A4R4G4B4 => [
            expand4((short >> 8) & 0xF),
            expand4((short >> 4) & 0xF),
            expand4(short & 0xF),
            expand4(short >> 12),
        ],
        X8R8G8B8 => [byte(2), byte(1), byte(0), 255],
        P8_BUMP => argb(VECTOR_PALETTE[usize::from(byte(0))]),
        _ => [byte(2), byte(1), byte(0), byte(3)],
    }
}

/// Which bits of a swizzled texel's index are x's, y's and z's: they take
/// turns, lowest first, until each dimension runs out.
fn swizzle_masks(width: usize, height: usize, depth: usize) -> [usize; 3] {
    let mut masks = [0; 3];
    let mut mask_bit = 1;
    let mut bit = 1;
    loop {
        let mut more = false;
        for (mask, size) in masks.iter_mut().zip([width, height, depth]) {
            if bit < size {
                *mask |= mask_bit;
                mask_bit <<= 1;
                more = true;
            }
        }
        if !more {
            return masks;
        }
        bit <<= 1;
    }
}

/// `value`'s bits, spread over the bits of `mask`.
fn spread(mask: usize, mut value: usize) -> usize {
    let mut result = 0;
    let mut bit = 1;
    while value != 0 && bit != 0 {
        if mask & bit != 0 {
            if value & 1 != 0 {
                result |= bit;
            }
            value >>= 1;
        }
        bit <<= 1;
    }
    result
}

fn level_size(image: &StoredImage<'_>, level: usize) -> [usize; 3] {
    let shrink = |size: u16| (usize::from(size) >> level).max(1);
    [
        shrink(image.width),
        shrink(image.height),
        shrink(image.depth),
    ]
}

fn floor_log2(value: usize) -> usize {
    (usize::BITS - 1 - value.max(1).leading_zeros()) as usize
}

/// Converts an image's pixels; the error says what about them is wrong.
pub(crate) fn convert(image: &StoredImage<'_>, layout: CacheKind) -> Result<Texture, String> {
    let texel = texel(image.format)?;
    let faces = match image.kind {
        TEXTURE_2D | TEXTURE_3D => 1,
        CUBE_MAP if image.width == image.height => 6,
        CUBE_MAP => return Err("is a cube map, not square".into()),
        other => return Err(format!("is of type {other}, not a 2D, 3D or cube texture")),
    };
    let linear = image.flags & LINEAR_FLAG != 0;
    if linear && (image.kind != TEXTURE_2D || matches!(texel, Texel::Block(..))) {
        return Err("is linear, but not an uncompressed 2D texture".into());
    }
    let longest = usize::from(image.width.max(image.height).max(image.depth));
    let full_chain = floor_log2(longest) + 1;
    let claimed = if linear {
        1
    } else {
        (usize::from(image.mipmap_count) + 1).min(full_chain)
    };

    // each level's bytes in the cache: a linear level's rows are padded on
    // the Xbox when the pixels have room for it
    let pitch = |width: usize| match texel {
        Texel::Block(_, bytes) => width.div_ceil(4) * bytes,
        Texel::Bytes(bytes) => width * bytes,
    };
    let rows = |height: usize| match texel {
        Texel::Block(..) => height.div_ceil(4),
        Texel::Bytes(_) => height,
    };
    let mut linear_pitch = pitch(usize::from(image.width));
    if linear && layout == CacheKind::Xbox {
        let padded = linear_pitch.next_multiple_of(XBOX_PITCH_ALIGNMENT);
        if padded * usize::from(image.height) <= image.pixels.len() {
            linear_pitch = padded;
        }
    }
    let stored_bytes: Vec<usize> = (0..claimed)
        .map(|level| {
            let [width, height, depth] = level_size(image, level);
            let row = if linear { linear_pitch } else { pitch(width) };
            row * rows(height) * depth
        })
        .collect();
    // where a face's level starts, in a chain of `levels`
    let offset = |levels: usize, face: usize, level: usize| -> usize {
        let before: usize = stored_bytes[..level].iter().sum();
        match layout {
            CacheKind::Xbox => {
                let chain: usize = stored_bytes[..levels].iter().sum();
                let face_size = if faces > 1 {
                    chain.next_multiple_of(XBOX_CUBE_FACE_ALIGNMENT)
                } else {
                    chain
                };
                face * face_size + before
            }
            CacheKind::CustomEdition => before * faces + face * stored_bytes[level],
        }
    };
    let needed = |levels: usize| offset(levels, faces - 1, levels - 1) + stored_bytes[levels - 1];
    // (the stock maps' non-square DXT bitmaps claim a level more than they
    // hold, their chains ending at 1x2: the levels held are kept)
    let mut levels = claimed;
    while levels > 1 && needed(levels) > image.pixels.len() {
        levels -= 1;
    }
    if needed(levels) > image.pixels.len() {
        return Err(format!(
            "are {} bytes, short of the {} its first level needs",
            image.pixels.len(),
            needed(levels)
        ));
    }

    let swizzled = layout == CacheKind::Xbox && !linear;
    let mut converted = Vec::with_capacity(levels);
    for (level, &level_bytes) in stored_bytes.iter().enumerate().take(levels) {
        let mut bytes = Vec::new();
        for face in 0..faces {
            let at = offset(levels, face, level);
            let stored = &image.pixels[at..at + level_bytes];
            match texel {
                Texel::Block(..) => bytes.extend_from_slice(stored),
                Texel::Bytes(size) => {
                    let [width, height, depth] = level_size(image, level);
                    let row = if linear { linear_pitch } else { width * size };
                    let masks = swizzle_masks(width, height, depth);
                    bytes.reserve(width * height * depth * 4);
                    for z in 0..depth {
                        for y in 0..height {
                            for x in 0..width {
                                let at = if swizzled {
                                    (spread(masks[0], x)
                                        | spread(masks[1], y)
                                        | spread(masks[2], z))
                                        * size
                                } else {
                                    (z * height + y) * row + x * size
                                };
                                let source = stored.get(at..at + size).unwrap_or(&[]);
                                bytes.extend_from_slice(&rgba(image.format, source));
                            }
                        }
                    }
                }
            }
        }
        converted.push(bytes);
    }

    let format = match texel {
        Texel::Block(format, _) => format,
        Texel::Bytes(_) => TextureFormat::Rgba8,
    };
    let mut texture = Texture {
        format,
        width: image.width,
        height: image.height,
        depth: image.depth,
        faces: faces as u8,
        levels: converted,
    };
    if format == TextureFormat::Rgba8 && levels == 1 && image.kind != TEXTURE_3D {
        make_mipmaps(&mut texture, full_chain);
    }
    Ok(texture)
}

/// Adds levels to an RGBA8 texture down to 1x1, each texel the mean of the
/// four above it (or two, or one, along a side already 1 texel long).
fn make_mipmaps(texture: &mut Texture, full_chain: usize) {
    for level in 1..full_chain {
        let [width, height, _] = texture.level_size(level).map(|size| size as usize);
        let [above_width, above_height, _] =
            texture.level_size(level - 1).map(|size| size as usize);
        let Some(above) = texture.levels.last() else {
            return;
        };
        let face_bytes = above_width * above_height * 4;
        let mut bytes = Vec::with_capacity(width * height * 4 * usize::from(texture.faces));
        for face in above.chunks_exact(face_bytes) {
            let texel = |x: usize, y: usize, channel: usize| {
                let x = x.min(above_width - 1);
                let y = y.min(above_height - 1);
                u32::from(face[(y * above_width + x) * 4 + channel])
            };
            for y in 0..height {
                for x in 0..width {
                    for channel in 0..4 {
                        let sum = texel(2 * x, 2 * y, channel)
                            + texel(2 * x + 1, 2 * y, channel)
                            + texel(2 * x, 2 * y + 1, channel)
                            + texel(2 * x + 1, 2 * y + 1, channel);
                        bytes.push(((sum + 2) / 4) as u8);
                    }
                }
            }
        }
        texture.levels.push(bytes);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn image(width: u16, height: u16, format: i16, pixels: &[u8]) -> StoredImage<'_> {
        StoredImage {
            width,
            height,
            depth: 1,
            kind: TEXTURE_2D,
            format,
            flags: 0,
            mipmap_count: 0,
            pixels,
        }
    }

    #[test]
    fn an_xbox_image_is_unswizzled() {
        // a 4x2 Y8 image: swizzled, texel (x, y) is at x's bits and y's
        // taking turns: (0,0) (1,0) (0,1) (1,1) (2,0) (3,0) (2,1) (3,1)
        let stored = [0, 1, 10, 11, 2, 3, 12, 13];
        let texture =
            convert(&image(4, 2, Y8, &stored), CacheKind::Xbox).unwrap_or_else(|e| panic!("{e}"));
        let reds: Vec<u8> = texture.levels[0].chunks(4).map(|texel| texel[0]).collect();
        assert_eq!(reds, [0, 1, 2, 3, 10, 11, 12, 13]);
        // and given its mipmaps, down to 1x1
        assert_eq!(texture.levels.len(), 3);
        assert_eq!(texture.levels[1].len(), 2 * 4);
        assert_eq!(texture.levels[2].len(), 4);
        for (level, bytes) in texture.levels.iter().enumerate() {
            assert_eq!(bytes.len(), texture.level_bytes(level));
        }
    }

    #[test]
    fn a_pc_image_is_read_in_rows() {
        let stored = [0, 1, 2, 3, 10, 11, 12, 13];
        let texture = convert(&image(4, 2, Y8, &stored), CacheKind::CustomEdition)
            .unwrap_or_else(|e| panic!("{e}"));
        let reds: Vec<u8> = texture.levels[0].chunks(4).map(|texel| texel[0]).collect();
        assert_eq!(reds, [0, 1, 2, 3, 10, 11, 12, 13]);
    }

    #[test]
    fn formats_widen_to_rgba() {
        assert_eq!(rgba(A8, &[7]), [255, 255, 255, 7]);
        assert_eq!(rgba(A8Y8, &[9, 7]), [9, 9, 9, 7]);
        assert_eq!(rgba(R5G6B5, &0xF800u16.to_le_bytes()), [255, 0, 0, 255]);
        assert_eq!(rgba(A1R5G5B5, &0x801Fu16.to_le_bytes()), [0, 0, 255, 255]);
        assert_eq!(rgba(A4R4G4B4, &0x0F00u16.to_le_bytes()), [255, 0, 0, 0]);
        assert_eq!(rgba(A8R8G8B8, &[1, 2, 3, 4]), [3, 2, 1, 4]);
        assert_eq!(rgba(X8R8G8B8, &[1, 2, 3, 4]), [3, 2, 1, 255]);
        // the palette's last entry is the flat normal, (0, 0, 1)
        assert_eq!(rgba(P8_BUMP, &[255]), [128, 128, 255, 0]);
    }

    #[test]
    fn dxt_is_kept_as_blocks() {
        // an 8x8 DXT1 image with a 4x4 mipmap: 4 blocks, then 1
        let stored: Vec<u8> = (0..40).collect();
        let mut stored_image = image(8, 8, DXT1, &stored);
        stored_image.mipmap_count = 1;
        let texture = convert(&stored_image, CacheKind::Xbox).unwrap_or_else(|e| panic!("{e}"));
        assert_eq!(texture.format, TextureFormat::Bc1);
        assert_eq!(
            texture.levels,
            [stored[..32].to_vec(), stored[32..].to_vec()]
        );
    }

    #[test]
    fn a_chain_shorter_than_claimed_keeps_the_levels_held() {
        // 4x16 DXT5 claims 5 levels (64, 32, 16, 16 and 16 bytes), holds 4
        let stored: Vec<u8> = (0..128).collect();
        let mut stored_image = image(4, 16, DXT5, &stored);
        stored_image.mipmap_count = 4;
        let texture = convert(&stored_image, CacheKind::Xbox).unwrap_or_else(|e| panic!("{e}"));
        assert_eq!(texture.levels.len(), 4);
        assert_eq!(texture.levels[3], stored[112..]);
    }

    #[test]
    fn an_xbox_cube_map_has_each_face_with_its_levels() {
        // 2x2 A8, one mipmap: each face 4 + 1 bytes, padded to 128
        let mut stored = vec![0u8; 5 * 128 + 5];
        for face in 0..6 {
            stored[face * 128..face * 128 + 5].fill(face as u8);
        }
        let mut cube = image(2, 2, A8, &stored);
        cube.kind = CUBE_MAP;
        cube.mipmap_count = 1;
        let texture = convert(&cube, CacheKind::Xbox).unwrap_or_else(|e| panic!("{e}"));
        assert_eq!(texture.faces, 6);
        let alphas: Vec<u8> = texture.levels[1].chunks(4).map(|texel| texel[3]).collect();
        assert_eq!(alphas, [0, 1, 2, 3, 4, 5]);
    }

    #[test]
    fn short_or_strange_images_are_refused() {
        let short = convert(&image(4, 4, A8, &[0; 15]), CacheKind::Xbox);
        assert_eq!(
            short.err().as_deref(),
            Some("are 15 bytes, short of the 16 its first level needs")
        );
        assert!(convert(&image(4, 4, 4, &[0; 16]), CacheKind::Xbox).is_err());
        let mut cube = image(4, 2, A8, &[0; 64]);
        cube.kind = CUBE_MAP;
        assert!(convert(&cube, CacheKind::Xbox).is_err());
    }
}
