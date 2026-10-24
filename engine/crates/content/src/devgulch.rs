//! The development playground, `devgulch`: a copy of Blood Gulch the
//! engine makes from the player's own `bloodgulch.map` when it loads, never
//! a file of its own. It is Blood Gulch's ground, sky and scenario, holding
//! every definition of every other map of its folder ([`level`]), so that
//! any object can be put on it.
//!
//! On its floor are the test stations ([`STATIONS`]): spots on the flat
//! ground of the two bases' sides, clear of the hills and of the map's
//! own vehicles, that the `station` command runs a test from. Each stands
//! on the ground found under it ([`ground`]).

use crate::devmap::{self, Station};
use crate::Level;

/// The map's name: `--map devgulch`, `map devgulch`.
pub const NAME: &str = "devgulch";
/// The map it is made from (its file's name, without .map).
pub const DONOR: &str = "bloodgulch";
/// The height the ground is looked for under: under Blood Gulch's sky,
/// whose ceiling is at 68, and over its highest ground (26).
pub const CEILING: f32 = 40.0;

/// The stations, on the flat floor in front of each base.
pub const STATIONS: [Station; 9] = [
    Station {
        name: "weapons",
        at: [56.0, -128.0],
        facing: 0.0,
        help: "fires each weapon at targets: its rate of fire, damage and heat; with the equipment beside it",
    },
    Station {
        name: "vehicles",
        at: [84.0, -166.0],
        facing: 0.0,
        help: "each vehicle stood in a row, and each driven round the course",
    },
    Station {
        name: "ai",
        at: [52.0, -86.0],
        facing: core::f32::consts::FRAC_PI_2,
        help: "a squad of each biped that is not the player's, stood to look at",
    },
    Station {
        name: "effects",
        at: [74.0, -124.0],
        facing: 0.0,
        help: "starts each effect that makes particles along a row: the most it keeps alive",
    },
    Station {
        name: "sounds",
        at: [88.0, -136.0],
        facing: 0.0,
        help: "a row of every sound scenery, each an emitter",
    },
    Station {
        name: "shaders",
        at: [42.0, -92.0],
        facing: core::f32::consts::FRAC_PI_2,
        help: "a row of an object of each shader class its model uses, to light and look at",
    },
    Station {
        name: "animation",
        at: [42.0, -68.0],
        facing: core::f32::consts::FRAC_PI_2,
        help: "plays each animation of a biped's graph in turn, named",
    },
    Station {
        name: "hud",
        at: [46.0, -136.0],
        facing: 0.0,
        help: "gives each weapon that has a HUD in turn, to see its HUD",
    },
    Station {
        name: "physics",
        at: [70.0, -152.0],
        facing: 0.0,
        help: "drops a pile of crates onto the ground",
    },
];

/// The vehicles' course: a loop on the floor before the red base, from
/// the vehicles station to each corner in turn and back.
pub const COURSE: [[f32; 2]; 4] = [
    [84.0, -166.0],
    [110.0, -166.0],
    [110.0, -142.0],
    [80.0, -142.0],
];

/// The station named `name`.
pub fn station(name: &str) -> Option<&'static Station> {
    STATIONS.iter().find(|station| station.name == name)
}

/// The height of the ground of `level`'s first BSP at `at`, under
/// [`CEILING`], or none if there is none there.
pub fn ground(level: &Level, at: [f32; 2]) -> Option<f32> {
    level
        .structure_bsps
        .first()?
        .collision
        .ground_at(at[0], at[1], CEILING)
}

/// devgulch: `blood_gulch`'s ground, sky and scenario (renamed [`NAME`]),
/// with the definitions of `others` gathered into it, each tag once (Blood
/// Gulch's where they share one).
pub fn level(blood_gulch: &Level, others: &[Level]) -> Level {
    let mut all = Vec::with_capacity(others.len() + 1);
    all.push(blood_gulch.clone());
    all.extend(others.iter().cloned());
    let mut level = devmap::gathered(&all).unwrap_or_else(|| blood_gulch.clone());
    // (gathered takes the first level with a player's biped as its base;
    // devgulch's ground, sky and scenario are Blood Gulch's whichever it is)
    level.scenario = blood_gulch.scenario.clone();
    level.structure_bsps = blood_gulch.structure_bsps.clone();
    level.skies = blood_gulch.skies.clone();
    level.scenario.path = NAME.into();
    level
}

#[cfg(test)]
mod tests {
    use super::*;

    fn named(path: &str) -> Level {
        let mut level = devmap::level(None);
        level.scenario.path = path.into();
        level
    }

    #[test]
    fn the_stations_have_distinct_names_and_are_found_by_them() {
        for (at, one) in STATIONS.iter().enumerate() {
            assert_eq!(station(one.name), Some(one));
            assert!(STATIONS[..at].iter().all(|other| other.name != one.name));
            assert!(!one.help.is_empty());
        }
        assert_eq!(station("nowhere"), None);
    }

    #[test]
    fn the_stations_stand_apart_inside_blood_gulchs_bounds() {
        // (Blood Gulch's world bounds: x 5.9 to 131.9, y -190.2 to -45.1)
        for (at, one) in STATIONS.iter().enumerate() {
            let [x, y] = one.at;
            assert!(
                (12.0..126.0).contains(&x) && (-184.0..-50.0).contains(&y),
                "{}",
                one.name
            );
            for other in &STATIONS[..at] {
                let [dx, dy] = [x - other.at[0], y - other.at[1]];
                assert!(
                    dx * dx + dy * dy >= 100.0,
                    "{} and {}",
                    one.name,
                    other.name
                );
            }
        }
    }

    #[test]
    fn the_course_starts_at_the_vehicles_station_inside_the_bounds() {
        assert_eq!(Some(COURSE[0]), station("vehicles").map(|one| one.at));
        for [x, y] in COURSE {
            assert!((12.0..126.0).contains(&x) && (-184.0..-50.0).contains(&y));
        }
    }

    #[test]
    fn devgulch_is_blood_gulchs_ground_and_scenario_with_the_others_definitions() {
        let mut blood_gulch = named(r"levels\test\bloodgulch\bloodgulch");
        blood_gulch.scenario.player_starts.truncate(1);
        let mut other = named("other");
        other.structure_bsps.clear();
        let mut shader = devmap::grid_shader();
        shader.tag.path = r"shaders\other".into();
        other.shaders.push(shader);
        let made = level(&blood_gulch, &[other]);
        assert_eq!(made.scenario.path, NAME);
        assert_eq!(
            made.scenario.player_starts,
            blood_gulch.scenario.player_starts
        );
        assert_eq!(made.structure_bsps.len(), blood_gulch.structure_bsps.len());
        assert!(made
            .shaders
            .iter()
            .any(|shader| shader.tag.path == r"shaders\other"));
    }

    #[test]
    fn a_station_stands_on_the_ground_under_it() {
        let flat = devmap::level(None);
        assert_eq!(
            ground(&flat, [0.0, 0.0]).map(|z| z.abs() < 1e-4),
            Some(true)
        );
        assert_eq!(ground(&flat, [500.0, 0.0]), None);
        let mut empty = flat;
        empty.structure_bsps.clear();
        assert_eq!(ground(&empty, [0.0, 0.0]), None);
    }
}
