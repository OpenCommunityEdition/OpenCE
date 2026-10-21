//! Bitmaps: a tag's images, and how they are grouped into sequences.

use crate::TagRef;

/// A bitmap tag.
#[derive(Clone, Debug, PartialEq)]
pub struct Bitmap {
    /// Its tag.
    pub tag: TagRef,
    /// 2D textures, 3D textures, cube maps, sprites or interface bitmaps.
    pub kind: i16,
    /// The format it was imported to.
    pub format: i16,
    /// What it is for (alpha blend, default, height map, ...).
    pub usage: i16,
    /// Its flags.
    pub flags: u16,
    /// Its height map's height, for a bump map.
    pub bump_height: f32,
    /// Its sequences of images (an animation, a font's glyphs).
    pub sequences: Vec<Sequence>,
    /// Its images.
    pub images: Vec<BitmapImage>,
}

/// A sequence of a bitmap's images.
#[derive(Clone, Debug, PartialEq)]
pub struct Sequence {
    /// Its name.
    pub name: String,
    /// Its first image.
    pub first_image: Option<u16>,
    /// How many images, from the first.
    pub image_count: u16,
    /// Its sprites, for a sprite bitmap.
    pub sprites: Vec<Sprite>,
}

/// A sprite: part of an image.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Sprite {
    /// The image it is in.
    pub image: u16,
    /// Its left, right, top and bottom, from 0 to 1.
    pub bounds: [f32; 4],
    /// Its registration point.
    pub registration_point: [f32; 2],
}

/// One image of a bitmap, with its mipmaps.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BitmapImage {
    /// Its width.
    pub width: u16,
    /// Its height.
    pub height: u16,
    /// Its depth (1 but for 3D textures).
    pub depth: u16,
    /// 2D texture, 3D texture or cube map.
    pub kind: i16,
    /// Its pixels' format (A8, R5G6B5, DXT1, P8, ...).
    pub format: i16,
    /// Its flags: power-of-two dimensions, compressed, palettized,
    /// swizzled, linear, in the resource map.
    pub flags: u16,
    /// Its registration point.
    pub registration_point: [i16; 2],
    /// How many mipmaps it has below the first.
    pub mipmap_count: u16,
    /// Its pixels.
    pub pixels: Pixels,
}

/// Where an image's pixels are.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Pixels {
    /// Read from the map, as the cache stores them (an Xbox cache's
    /// swizzled, unless linear).
    Here(Vec<u8>),
    /// In Custom Edition's `bitmaps.map`.
    ResourceMap {
        /// Where, in that file.
        offset: u32,
        /// How many bytes.
        size: u32,
    },
}
