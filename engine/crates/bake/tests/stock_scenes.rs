//! The stock maps' scenes: each BSP's triangles, materials and sky read
//! into the bake's scene and its hierarchy, and Blood Gulch's sun up in
//! its sky (the six BSPs with no lightmaps aside). Skipped without assets/maps.

use std::error::Error;
use std::path::Path;

use opence_bake::{Bvh, Ray, Scene};
use opence_import::testing::stock_level_unheld;

#[test]
fn every_stock_bsp_makes_a_scene() -> Result<(), Box<dyn Error>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let mut names: Vec<String> = ["bloodgulch", "a10", "a30", "b30", "c10", "d40"]
        .into_iter()
        .filter(|name| maps.join(format!("{name}.map")).exists())
        .map(String::from)
        .collect();
    names.sort();
    for name in &names {
        let level = stock_level_unheld(name)?.ok_or_else(|| format!("{name}: no map"))?;
        for (bsp, structure) in level.structure_bsps.iter().enumerate() {
            // (the six BSPs shipped with no lightmaps have no lit geometry)
            if structure.lightmap_bitmap.is_none() {
                continue;
            }
            let scene = Scene::of(&level, bsp).ok_or("no BSP")?;
            assert!(scene.triangles.len() > 100, "{name} bsp {bsp}");
            let bvh = Bvh::build(&scene.triangles);
            assert_eq!(bvh.triangles.len(), scene.triangles.len());
            // a ray down from above the middle meets the ground
            let [low, high] = scene.bounds;
            let middle: [f32; 3] = std::array::from_fn(|axis| (low[axis] + high[axis]) / 2.0);
            let down = Ray {
                origin: [middle[0], middle[1], high[2] + 1.0],
                direction: [0.0, 0.0, -1.0],
                t_max: high[2] - low[2] + 2.0,
            };
            if name == "bloodgulch" {
                assert!(bvh.occluded(&down), "{name}: the canyon's floor");
                let sun = scene.sun().ok_or("no sun")?;
                assert!(sun.towards[2] > 0.0, "{sun:?}");
                assert!(scene.sky.iter().sum::<f32>() > 0.0, "{:?}", scene.sky);
            }
        }
    }
    Ok(())
}
