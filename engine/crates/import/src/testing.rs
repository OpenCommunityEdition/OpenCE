//! The levels the tests load from the user's maps: each loaded through
//! the cache folder the game uses (its pack, or the map imported once and
//! its pack written), and held once per process, so that a suite's tests
//! share one level and the game's packs and the tests' are the same.

use std::collections::BTreeMap;
use std::env;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex, OnceLock, PoisonError};

use opence_content::Level;

use crate::level_cache::{load_level, LoadError, LoadedFrom};

/// Where a held level came from.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Held {
    /// Held already by this process: no file was read.
    Memory,
    /// Its pack in the cache folder.
    Pack,
    /// The map, imported now.
    Map,
}

/// The repository's root: `OPENCE_ROOT`, or the checkout the tests were
/// built in.
pub fn root() -> PathBuf {
    root_from(env::var_os("OPENCE_ROOT"))
}

/// The root named by `variable` (`OPENCE_ROOT`'s value), or the checkout's.
fn root_from(variable: Option<std::ffi::OsString>) -> PathBuf {
    variable
        .map(PathBuf::from)
        .unwrap_or_else(|| Path::new(env!("CARGO_MANIFEST_DIR")).join("../../.."))
}

/// The user's maps: `assets/maps`.
pub fn maps_folder() -> PathBuf {
    root().join("assets/maps")
}

/// The cache folder: `OPENCE_CACHE`, or the app's `build/engine/cache`.
pub fn cache_folder() -> PathBuf {
    env::var_os("OPENCE_CACHE")
        .map(PathBuf::from)
        .unwrap_or_else(|| root().join("build/engine/cache"))
}

fn held() -> &'static Mutex<BTreeMap<PathBuf, Arc<Level>>> {
    static HELD: OnceLock<Mutex<BTreeMap<PathBuf, Arc<Level>>>> = OnceLock::new();
    HELD.get_or_init(|| Mutex::new(BTreeMap::new()))
}

/// `map`'s level, held by this process after its first load, which goes
/// through `cache_folder`. (Tests that load at once wait for one load.)
pub fn held_level(map: &Path, cache_folder: &Path) -> Result<(Arc<Level>, Held), LoadError> {
    // (a test that panicked while holding the lock left the map whole)
    let mut held = held().lock().unwrap_or_else(PoisonError::into_inner);
    if let Some(level) = held.get(map) {
        return Ok((Arc::clone(level), Held::Memory));
    }
    let loaded = load_level(map, cache_folder)?;
    let level = Arc::new(loaded.level);
    held.insert(map.to_owned(), Arc::clone(&level));
    let from = match loaded.from {
        LoadedFrom::Pack => Held::Pack,
        LoadedFrom::Map => Held::Map,
    };
    Ok((level, from))
}

/// The user's `assets/maps/<name>.map`, held, or `None` when there is no
/// such map (the tests that need it then pass without it).
pub fn stock_level(name: &str) -> Result<Option<Arc<Level>>, LoadError> {
    let map = maps_folder().join(format!("{name}.map"));
    if !map.exists() {
        return Ok(None);
    }
    Ok(Some(held_level(&map, &cache_folder())?.0))
}

/// The user's `assets/maps/<name>.map`, loaded through `cache_folder` but
/// not held, or `None` when there is no such map: for the suites that go
/// through many maps, which held would keep all in memory at once (a
/// campaign level is some 300 MB).
pub fn stock_level_unheld(name: &str) -> Result<Option<Level>, LoadError> {
    let map = maps_folder().join(format!("{name}.map"));
    if !map.exists() {
        return Ok(None);
    }
    Ok(Some(load_level(&map, &cache_folder())?.level))
}

#[cfg(test)]
mod tests {
    use std::fs;

    use super::*;
    use crate::cache::CacheKind;
    use crate::level::tests::{level, Breaks};
    use crate::{import_level, Cache};

    fn folder(name: &str) -> PathBuf {
        let folder = env::temp_dir()
            .join(format!("opence_testing_{}", std::process::id()))
            .join(name);
        let _ = fs::remove_dir_all(&folder);
        if let Err(error) = fs::create_dir_all(&folder) {
            panic!("{}: {error}", folder.display());
        }
        folder
    }

    fn held_or_panic(map: &Path, cache: &Path) -> (Arc<Level>, Held) {
        held_level(map, cache).unwrap_or_else(|error| panic!("{error}"))
    }

    #[test]
    fn the_held_level_is_the_imported_one() {
        let folder = folder("same");
        let map = folder.join("box.map");
        let file = level(CacheKind::Xbox, &Breaks::default()).build();
        if let Err(error) = fs::write(&map, &file) {
            panic!("{error}");
        }
        let imported = Cache::open(file)
            .and_then(|cache| import_level(&cache))
            .unwrap_or_else(|error| panic!("{error}"));
        let (held, from) = held_or_panic(&map, &folder.join("cache"));
        assert_eq!(from, Held::Map);
        assert_eq!(*held, imported);
    }

    #[test]
    fn a_second_load_reads_no_file() {
        let folder = folder("twice");
        let map = folder.join("box.map");
        let cache = folder.join("cache");
        if let Err(error) = fs::write(
            &map,
            level(CacheKind::CustomEdition, &Breaks::default()).build(),
        ) {
            panic!("{error}");
        }
        let (first, _) = held_or_panic(&map, &cache);
        // (with the map and its pack gone, only the held level is left)
        let _ = fs::remove_file(&map);
        let _ = fs::remove_dir_all(&cache);
        let (again, from) = held_or_panic(&map, &cache);
        assert_eq!(from, Held::Memory);
        assert!(Arc::ptr_eq(&first, &again));
    }

    #[test]
    fn a_pack_in_the_folder_is_read_not_imported() {
        let folder = folder("pack");
        let map = folder.join("box.map");
        let cache = folder.join("cache");
        if let Err(error) = fs::write(&map, level(CacheKind::Xbox, &Breaks::default()).build()) {
            panic!("{error}");
        }
        // (another process, or the game, wrote the pack)
        let written = load_level(&map, &cache).unwrap_or_else(|error| panic!("{error}"));
        assert_eq!(written.from, LoadedFrom::Map);
        let (held, from) = held_or_panic(&map, &cache);
        assert_eq!(from, Held::Pack);
        assert_eq!(*held, written.level);
    }

    #[test]
    fn opence_root_names_the_repository_when_set() {
        // (cargo-mutants tests a copy of the workspace made outside the
        // repository, which has no assets beside it)
        assert_eq!(
            root_from(Some("/elsewhere/OpenCE".into())),
            PathBuf::from("/elsewhere/OpenCE")
        );
        assert_eq!(
            root_from(None),
            Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..")
        );
    }

    #[test]
    fn a_missing_stock_map_is_none() {
        assert!(matches!(stock_level("no_such_map"), Ok(None)));
    }
}
