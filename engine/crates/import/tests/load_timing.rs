//! How long a map's level takes to load from its pack, and where the time
//! goes: reading the map, its fingerprint, reading the pack and checking
//! it. It prints the times (`--nocapture`) and checks only that a pack is
//! read back as the level imported. Skipped without assets/maps.

use std::error::Error;
use std::fs;
use std::path::Path;
use std::time::Instant;

use opence_content::pack;
use opence_import::{load_level, Cache, LoadedFrom};

fn milliseconds(start: Instant) -> f64 {
    start.elapsed().as_secs_f64() * 1000.0
}

#[test]
fn a_pack_loads_as_the_level_imported_and_its_times_are_printed() -> Result<(), Box<dyn Error>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let cache = std::env::temp_dir().join(format!("opence_load_timing_{}", std::process::id()));
    let mut timed = 0;
    for name in ["bloodgulch", "a10"] {
        let map = maps.join(format!("{name}.map"));
        if !map.exists() {
            continue;
        }
        let start = Instant::now();
        let file = fs::read(&map)?;
        let read = milliseconds(start);
        let start = Instant::now();
        let source = pack::fingerprint(&file);
        let fingerprint = milliseconds(start);

        // (the header and the tag index, and an Xbox cache's inflating: the
        // part of the import a declarative reader would speed)
        let start = Instant::now();
        let opened = Cache::open(file.clone())?;
        let open = milliseconds(start);
        let tags = opened.tags().len();

        let start = Instant::now();
        let imported = load_level(&map, &cache)?;
        let import = milliseconds(start);
        assert_eq!(imported.from, LoadedFrom::Map);
        let pack_path = imported.pack.clone()?;
        let pack_bytes = fs::read(&pack_path)?;

        let start = Instant::now();
        let loaded = load_level(&map, &cache)?;
        let load = milliseconds(start);
        assert_eq!(loaded.from, LoadedFrom::Pack);
        assert!(
            loaded.level == imported.level,
            "{name}: the pack is not the level imported"
        );

        let start = Instant::now();
        let (header, _) = pack::read_pack(&pack_bytes)?;
        let check = milliseconds(start);
        assert_eq!(header.source, source);
        println!(
            "{name}: map {:.1} MB read {read:.1} ms, fingerprint {fingerprint:.1} ms; \
             open {open:.1} ms ({tags} tags); import {import:.0} ms; pack {:.1} MB, load {load:.1} ms (of it, check and \
             unpack {check:.1} ms)",
            file.len() as f64 / 1.0e6,
            pack_bytes.len() as f64 / 1.0e6,
        );
        timed += 1;
    }
    let _ = fs::remove_dir_all(&cache);
    if timed == 0 {
        println!("skipped: no assets/maps/bloodgulch.map or a10.map");
    }
    Ok(())
}
