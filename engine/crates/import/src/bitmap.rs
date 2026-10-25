//! Bitmaps: a tag's sequences and images, each image's pixels converted
//! to a texture (`texture.rs`).
//!
//! Each image gives its pixels' offset into the file (an Xbox cache's are
//! after the tag data), or a Halo PC image flagged so into `bitmaps.map`.

use opence_content::{Bitmap, BitmapImage, Pixels, ResourceRange, Sequence, Sprite};

use crate::cache::{Cache, CacheKind};
use crate::error::ImportError;
use crate::level::{tag_ref, BITM};
use crate::reader;
use crate::tags::Element;
use crate::texture::{self, StoredImage};

pub(crate) const BITMAP_SIZE: usize = 0x6C;
pub(crate) const SEQUENCES_OFFSET: usize = 0x54;
pub(crate) const SEQUENCE_SIZE: usize = 0x40;
pub(crate) const SEQUENCE_SPRITES_OFFSET: usize = 0x34;
pub(crate) const SPRITE_SIZE: usize = 0x20;
pub(crate) const IMAGES_OFFSET: usize = 0x60;
pub(crate) const IMAGE_SIZE: usize = 0x30;
const MAX_SEQUENCES: usize = 256;
const MAX_SPRITES: usize = 2048;
const MAX_IMAGES: usize = 2048;
const MAX_PIXEL_DATA: usize = 64 << 20;
const LONGEST_DIMENSION: u16 = 8192;
const LONGEST_DEPTH: u16 = 512;
const SEQUENCE_NAME_LONGEST: usize = 31;
/// An image whose pixels are in Custom Edition's `bitmaps.map`.
pub(crate) const IN_RESOURCE_MAP_FLAG: u16 = 1 << 8;

/// Reads a bitmap tag, its sequences, its images and their pixels.
pub fn import_bitmap(cache: &Cache, index: usize) -> Result<Bitmap, ImportError> {
    let bitmap = cache.tag(index, BITM, BITMAP_SIZE)?;
    let block = bitmap.block(IMAGES_OFFSET, "bitmaps", IMAGE_SIZE, MAX_IMAGES)?;
    let mut images = Vec::with_capacity(block.len());
    for image in block.iter() {
        images.push(import_image(&image, cache)?);
    }

    let block = bitmap.block(SEQUENCES_OFFSET, "sequences", SEQUENCE_SIZE, MAX_SEQUENCES)?;
    let mut sequences = Vec::with_capacity(block.len());
    for sequence in block.iter() {
        sequences.push(import_sequence(&sequence, images.len())?);
    }

    Ok(Bitmap {
        tag: tag_ref(cache, index),
        kind: bitmap.i16(0, "type")?,
        format: bitmap.i16(2, "format")?,
        usage: bitmap.i16(4, "usage")?,
        flags: bitmap.u16(6, "flags")?,
        bump_height: bitmap.f32(0x10, "bump height")?,
        sequences,
        images,
    })
}

fn image_index(
    element: &Element<'_>,
    at: usize,
    field: &str,
    images: usize,
) -> Result<Option<u16>, ImportError> {
    match element.i16(at, field)? {
        -1 => Ok(None),
        index if index >= 0 && (index as usize) < images => Ok(Some(index as u16)),
        index => Err(element.fault(field, format!("is {index}, past the {images} bitmaps"))),
    }
}

fn import_sequence(sequence: &Element<'_>, images: usize) -> Result<Sequence, ImportError> {
    let first_image = image_index(sequence, 0x20, "first bitmap index", images)?;
    let image_count = sequence.i16(0x22, "bitmap count")?;
    let fits = image_count >= 0
        && match first_image {
            Some(first) => usize::from(first) + image_count as usize <= images,
            None => image_count == 0,
        };
    if !fits {
        return Err(sequence.fault(
            "bitmap count",
            format!("is {image_count} from {first_image:?}, past the {images} bitmaps"),
        ));
    }
    let block = sequence.block(SEQUENCE_SPRITES_OFFSET, "sprites", SPRITE_SIZE, MAX_SPRITES)?;
    let mut sprites = Vec::with_capacity(block.len());
    for sprite in block.iter() {
        let image = image_index(&sprite, 0, "bitmap index", images)?
            .ok_or_else(|| sprite.fault("bitmap index", "is none"))?;
        sprites.push(Sprite {
            image,
            bounds: [
                sprite.f32(0x08, "left")?,
                sprite.f32(0x0C, "right")?,
                sprite.f32(0x10, "top")?,
                sprite.f32(0x14, "bottom")?,
            ],
            registration_point: [
                sprite.f32(0x18, "registration point")?,
                sprite.f32(0x1C, "registration point")?,
            ],
        });
    }
    Ok(Sequence {
        name: sequence.text(0, "name", SEQUENCE_NAME_LONGEST)?.to_owned(),
        first_image,
        image_count: image_count as u16,
        sprites,
    })
}

fn import_image(image: &Element<'_>, cache: &Cache) -> Result<BitmapImage, ImportError> {
    let dimension = |at: usize, field: &str, longest: u16| {
        let value = image.u16(at, field)?;
        if (1..=longest).contains(&value) {
            Ok(value)
        } else {
            Err(image.fault(field, format!("is {value}, not 1 to {longest}")))
        }
    };
    let flags = image.u16(0x0E, "flags")?;
    let offset = image.u32(0x18, "pixels offset")?;
    let size = image.u32(0x1C, "pixels size")?;
    if size as usize > MAX_PIXEL_DATA {
        return Err(image.fault(
            "pixels size",
            format!("is {size} bytes, past {MAX_PIXEL_DATA}"),
        ));
    }
    let width = dimension(0x04, "width", LONGEST_DIMENSION)?;
    let height = dimension(0x06, "height", LONGEST_DIMENSION)?;
    let depth = dimension(0x08, "depth", LONGEST_DEPTH)?;
    let kind = image.i16(0x0A, "type")?;
    let format = image.i16(0x0C, "format")?;
    let mipmap_count = image.u16(0x14, "mipmap count")?;
    let layout = cache.header().kind;
    let pixels = if layout == CacheKind::CustomEdition && flags & IN_RESOURCE_MAP_FLAG != 0 {
        Pixels::ResourceMap(ResourceRange { offset, size })
    } else {
        // (the stock maps' offsets are into the file, not from the tag's
        // pixel data, whose offset field a cache does not use)
        let at = usize::try_from(offset).unwrap_or(usize::MAX);
        let stored = reader::slice(cache.file(), at, size as usize)
            .map_err(|fault| image.fault("pixels", fault))?;
        let stored = StoredImage {
            width,
            height,
            depth,
            kind,
            format,
            flags,
            mipmap_count,
            pixels: stored,
        };
        Pixels::Texture(
            texture::convert(&stored, layout).map_err(|fault| image.fault("pixels", fault))?,
        )
    };
    Ok(BitmapImage {
        width,
        height,
        depth,
        kind,
        format,
        flags,
        registration_point: [
            image.i16(0x10, "registration point")?,
            image.i16(0x12, "registration point")?,
        ],
        mipmap_count,
        pixels,
    })
}
