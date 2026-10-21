//! The cache folder: each map imported once, kept as a pack
//! (`<map>-<fingerprint>.ocpak`), and imported again when the map or the
//! importer changes.

use std::fs;
use std::io;
use std::path::{Path, PathBuf};

use opence_content::pack::{self, PackHeader};
use opence_content::Level;

use crate::{import_level, Cache, ImportError};

/// The importer's version, which a pack records: raise it with any change
/// to what the importer writes, so that packs written before are imported
/// again.
pub const IMPORTER_VERSION: u32 = 9;

/// Why a map could not be loaded.
#[derive(Debug, thiserror::Error)]
pub enum LoadError {
    /// The map could not be read.
    #[error("{path}: {error}")]
    Read {
        /// The map.
        path: PathBuf,
        /// Why.
        error: io::Error,
    },
    /// The map was refused.
    #[error("{path}: {error}")]
    Import {
        /// The map.
        path: PathBuf,
        /// Why.
        error: ImportError,
    },
}

/// Where a loaded level came from.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LoadedFrom {
    /// Its pack in the cache folder.
    Pack,
    /// The map, imported now (and its pack written, if it could be).
    Map,
}

/// A loaded level.
#[derive(Debug)]
pub struct LoadedLevel {
    /// The level.
    pub level: Level,
    /// Where from.
    pub from: LoadedFrom,
    /// The pack that holds it, or why it could not be written.
    pub pack: Result<PathBuf, String>,
}

/// Loads a map's level from its pack in `cache_folder`, importing it (and
/// writing the pack) if there is none from this map and this importer.
pub fn load_level(map: &Path, cache_folder: &Path) -> Result<LoadedLevel, LoadError> {
    load_level_with(map, cache_folder, IMPORTER_VERSION)
}

fn pack_path(map: &Path, cache_folder: &Path, source: u64) -> PathBuf {
    let name = map
        .file_stem()
        .map(|stem| stem.to_string_lossy().into_owned())
        .unwrap_or_else(|| "map".into());
    cache_folder.join(format!("{name}-{source:016x}.ocpak"))
}

pub(crate) fn load_level_with(
    map: &Path,
    cache_folder: &Path,
    importer_version: u32,
) -> Result<LoadedLevel, LoadError> {
    let file = fs::read(map).map_err(|error| LoadError::Read {
        path: map.to_owned(),
        error,
    })?;
    let source = pack::fingerprint(&file);
    let path = pack_path(map, cache_folder, source);
    let expected = PackHeader {
        importer_version,
        source,
    };
    if let Ok(bytes) = fs::read(&path) {
        // (a pack from another importer, or damaged, is written again)
        if let Ok((header, level)) = pack::read_pack(&bytes) {
            if header == expected {
                return Ok(LoadedLevel {
                    level,
                    from: LoadedFrom::Pack,
                    pack: Ok(path),
                });
            }
        }
    }

    let import_error = |error| LoadError::Import {
        path: map.to_owned(),
        error,
    };
    let cache = Cache::open(file).map_err(import_error)?;
    let level = import_level(&cache).map_err(import_error)?;
    let pack = write(&level, expected, cache_folder, &path);
    Ok(LoadedLevel {
        level,
        from: LoadedFrom::Map,
        pack,
    })
}

/// Writes a pack beside where it goes, then moves it there, so that a
/// pack is never seen half written.
fn write(
    level: &Level,
    header: PackHeader,
    cache_folder: &Path,
    path: &Path,
) -> Result<PathBuf, String> {
    let bytes = pack::write_pack(level, header).map_err(|error| error.to_string())?;
    let partial = path.with_extension("ocpak.partial");
    fs::create_dir_all(cache_folder)
        .and_then(|()| fs::write(&partial, &bytes))
        .and_then(|()| fs::rename(&partial, path))
        .map_err(|error| format!("{}: {error}", path.display()))?;
    Ok(path.to_owned())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::level::tests::{level, Breaks};

    fn folder(name: &str) -> PathBuf {
        let folder = std::env::temp_dir()
            .join(format!("opence_level_cache_{}", std::process::id()))
            .join(name);
        let _ = fs::remove_dir_all(&folder);
        if let Err(error) = fs::create_dir_all(&folder) {
            panic!("{}: {error}", folder.display());
        }
        folder
    }

    fn load(map: &Path, cache: &Path, version: u32) -> LoadedLevel {
        match load_level_with(map, cache, version) {
            Ok(loaded) => loaded,
            Err(error) => panic!("{error}"),
        }
    }

    #[test]
    fn a_map_is_imported_once_and_again_when_the_importer_changes() {
        let folder = folder("versions");
        let map = folder.join("box.map");
        let cache = folder.join("cache");
        if let Err(error) = fs::write(&map, level(CacheKind::Xbox, &Breaks::default()).build()) {
            panic!("{error}");
        }

        let first = load(&map, &cache, 1);
        assert_eq!(first.from, LoadedFrom::Map);
        let pack = first.pack.clone().unwrap_or_else(|error| panic!("{error}"));
        assert!(pack.starts_with(&cache));

        let again = load(&map, &cache, 1);
        assert_eq!(again.from, LoadedFrom::Pack);
        assert_eq!(again.level, first.level);

        let newer = load(&map, &cache, 2);
        assert_eq!(newer.from, LoadedFrom::Map);
        assert_eq!(load(&map, &cache, 2).from, LoadedFrom::Pack);
    }

    #[test]
    fn a_damaged_pack_is_written_again() {
        let folder = folder("damaged");
        let map = folder.join("box.map");
        let cache = folder.join("cache");
        if let Err(error) = fs::write(
            &map,
            level(CacheKind::CustomEdition, &Breaks::default()).build(),
        ) {
            panic!("{error}");
        }
        let first = load(&map, &cache, 1);
        let pack = first.pack.unwrap_or_else(|error| panic!("{error}"));
        let mut bytes = fs::read(&pack).unwrap_or_default();
        let middle = bytes.len() / 2;
        bytes[middle] ^= 1;
        if let Err(error) = fs::write(&pack, &bytes) {
            panic!("{error}");
        }
        let again = load(&map, &cache, 1);
        assert_eq!(again.from, LoadedFrom::Map);
        assert_eq!(again.level, first.level);
        assert_eq!(load(&map, &cache, 1).from, LoadedFrom::Pack);
    }

    #[test]
    fn a_changed_map_has_its_own_pack() {
        let folder = folder("changed");
        let map = folder.join("box.map");
        let cache = folder.join("cache");
        let _ = fs::write(&map, level(CacheKind::Xbox, &Breaks::default()).build());
        let first = load(&map, &cache, 1);
        let mut file = fs::read(&map).unwrap_or_default();
        file.push(0);
        let _ = fs::write(&map, file);
        let changed = load(&map, &cache, 1);
        assert_eq!(changed.from, LoadedFrom::Map);
        assert_ne!(changed.pack.ok(), first.pack.ok());
    }

    #[test]
    fn a_missing_or_broken_map_is_an_error() {
        let folder = folder("missing");
        let missing = load_level_with(&folder.join("none.map"), &folder, 1);
        assert!(matches!(missing, Err(LoadError::Read { .. })));
        let broken = folder.join("broken.map");
        let _ = fs::write(&broken, b"not a map");
        assert!(matches!(
            load_level_with(&broken, &folder, 1),
            Err(LoadError::Import { .. })
        ));
    }
}
