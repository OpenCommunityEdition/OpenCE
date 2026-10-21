//! Imports maps and says what each holds, a line of JSON per map:
//!
//! ```text
//! opence-import [--cache FOLDER] MAP...
//! ```
//!
//! With `--cache`, each level is loaded through the cache folder (its
//! pack, or the map imported and its pack written), and the line says
//! which. It exits with 1 if any map was refused.

use std::fmt::Write as _;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use opence_content::{Level, Pixels};
use opence_import::{import_level, load_level, Cache, CacheKind, LoadedFrom};

fn quoted(text: &str) -> String {
    let mut quoted = String::from("\"");
    for character in text.chars() {
        match character {
            '"' => quoted.push_str("\\\""),
            '\\' => quoted.push_str("\\\\"),
            character if u32::from(character) < 0x20 => {
                let _ = write!(quoted, "\\u{:04x}", u32::from(character));
            }
            character => quoted.push(character),
        }
    }
    quoted.push('"');
    quoted
}

fn counts(level: &Level) -> String {
    let images: Vec<_> = level
        .bitmaps
        .iter()
        .flat_map(|bitmap| &bitmap.images)
        .collect();
    let textures = images
        .iter()
        .filter(|image| matches!(image.pixels, Pixels::Texture(_)))
        .count();
    format!(
        "\"scenario\": {}, \"bsps\": {}, \"shaders\": {}, \"bitmaps\": {}, \"images\": {}, \"textures\": {}",
        quoted(&level.scenario.path),
        level.structure_bsps.len(),
        level.shaders.len(),
        level.bitmaps.len(),
        images.len(),
        textures,
    )
}

/// One map's line, or its refusal.
fn report(map: &Path, cache_folder: Option<&Path>) -> Result<String, String> {
    let file = std::fs::read(map).map_err(|error| error.to_string())?;
    let cache = Cache::open(file).map_err(|error| error.to_string())?;
    let kind = match cache.header().kind {
        CacheKind::Xbox => "xbox",
        CacheKind::CustomEdition => "custom_edition",
    };
    let tags = cache.tags().len();
    let (level, from) = match cache_folder {
        None => (
            import_level(&cache).map_err(|error| error.to_string())?,
            None,
        ),
        Some(folder) => {
            drop(cache);
            let loaded = load_level(map, folder).map_err(|error| error.to_string())?;
            let from = match loaded.from {
                LoadedFrom::Pack => "pack",
                LoadedFrom::Map => "map",
            };
            if let Err(error) = &loaded.pack {
                eprintln!("{}: the pack was not written: {error}", map.display());
            }
            (loaded.level, Some(from))
        }
    };
    let mut line = format!("\"kind\": \"{kind}\", \"tags\": {tags}, {}", counts(&level));
    if let Some(from) = from {
        let _ = write!(line, ", \"from\": \"{from}\"");
    }
    Ok(line)
}

fn main() -> ExitCode {
    let mut cache_folder = None;
    let mut maps = Vec::new();
    let mut arguments = std::env::args_os().skip(1);
    while let Some(argument) = arguments.next() {
        if argument == "--cache" {
            cache_folder = arguments.next().map(PathBuf::from);
        } else {
            maps.push(PathBuf::from(argument));
        }
    }
    if maps.is_empty() {
        eprintln!("usage: opence-import [--cache FOLDER] MAP...");
        return ExitCode::from(2);
    }
    let mut refused = false;
    for map in &maps {
        let name = map
            .file_stem()
            .map(|stem| stem.to_string_lossy().into_owned())
            .unwrap_or_default();
        match report(map, cache_folder.as_deref()) {
            Ok(line) => println!("{{\"map\": {}, {line}}}", quoted(&name)),
            Err(error) => {
                refused = true;
                println!(
                    "{{\"map\": {}, \"error\": {}}}",
                    quoted(&name),
                    quoted(&error)
                );
            }
        }
    }
    if refused {
        ExitCode::FAILURE
    } else {
        ExitCode::SUCCESS
    }
}
