//! Blood Gulch's textures, imported from the stock map, are pictures: a
//! block's colour is near its neighbour's, as it is not when the pixels are
//! read from the wrong place in the file. Skipped without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{Pixels, Texture, TextureFormat};
use opence_import::{import_level, Cache};

/// The mean difference, over a channel's 0 to 255, between each block's
/// first colour and the next block's along its row (each texel's, for an
/// uncompressed texture).
fn roughness(texture: &Texture) -> Option<f64> {
    let [width, height, _] = texture.level_size(0);
    let pixels = texture.levels.first()?;
    let (step, at, columns) = match texture.format {
        TextureFormat::Bc1 => (8, 0, width.div_ceil(4)),
        TextureFormat::Bc2 | TextureFormat::Bc3 => (16, 8, width.div_ceil(4)),
        TextureFormat::Rgba8 => (4, 0, width),
    };
    let rows = match texture.format {
        TextureFormat::Rgba8 => height,
        _ => height.div_ceil(4),
    };
    if columns < 8 || rows < 8 {
        return None;
    }
    let colour = |index: usize| -> Option<[i32; 3]> {
        let bytes = pixels.get(index * step + at..index * step + at + 4)?;
        Some(match texture.format {
            TextureFormat::Rgba8 => [bytes[0].into(), bytes[1].into(), bytes[2].into()],
            _ => {
                let packed = i32::from(u16::from_le_bytes([bytes[0], bytes[1]]));
                [
                    (packed >> 11) * 255 / 31,
                    (packed >> 5 & 63) * 255 / 63,
                    (packed & 31) * 255 / 31,
                ]
            }
        })
    };
    let (mut total, mut count) = (0i64, 0i64);
    for row in 0..rows as usize {
        for column in 0..columns as usize - 1 {
            let index = row * columns as usize + column;
            let (here, next) = (colour(index)?, colour(index + 1)?);
            total += here
                .iter()
                .zip(next)
                .map(|(a, b)| i64::from((a - b).abs()))
                .sum::<i64>();
            count += 3;
        }
    }
    Some(total as f64 / count as f64)
}

#[test]
fn blood_gulchs_textures_are_pictures() -> Result<(), Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(()); // (no game data here)
    };
    let level = import_level(&Cache::open(file)?)?;
    let mut measured: Vec<f64> = level
        .bitmaps
        .iter()
        .flat_map(|bitmap| &bitmap.images)
        .filter_map(|image| match &image.pixels {
            Pixels::Texture(texture) => roughness(texture),
            Pixels::ResourceMap(_) => None,
        })
        .collect();
    assert!(measured.len() > 10, "{} textures measured", measured.len());
    measured.sort_by(f64::total_cmp);
    // (random bytes differ by about 85; the stock textures by under 20)
    let median = measured[measured.len() / 2];
    assert!(
        median < 30.0,
        "the median texture differs by {median:.1} between blocks"
    );
    Ok(())
}
