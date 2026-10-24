//! Every stock scenario's campaign data imports, and each index in it names
//! something in its list (skipped where there is no game data).

use std::error::Error;
use std::path::Path;

use opence_content::Campaign;
use opence_import::{import_campaign, Cache};

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

const MAPS: [&str; 24] = [
    "a10",
    "a30",
    "a50",
    "b30",
    "b40",
    "c10",
    "c20",
    "c40",
    "d20",
    "d40",
    "beavercreek",
    "bloodgulch",
    "boardingaction",
    "carousel",
    "chillout",
    "damnation",
    "hangemhigh",
    "longest",
    "prisoner",
    "putput",
    "ratrace",
    "sidewinder",
    "wizard",
    "ui",
];

fn campaign(map: &str) -> Result<Option<Campaign>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../assets/maps")
        .join(format!("{map}.map"));
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_campaign(&Cache::open(file)?)?))
}

/// Whether `index` is none (`-1`) or names one of `count`.
fn within(index: i16, count: usize) -> bool {
    index == -1 || usize::try_from(index).is_ok_and(|index| index < count)
}

/// Whether each index the campaign holds names something in its list.
/// (A command's points and a squad's platoon are not checked: the stock
/// tags leave stale ones where the command or squad does not use them, and
/// the game reads them only where it does.)
fn linked(campaign: &Campaign) -> bool {
    let actors = campaign.actor_palette.len();
    let lists = campaign.command_lists.len();
    let names = campaign.object_names.len();
    campaign.encounters.iter().all(|encounter| {
        encounter.squads.iter().all(|squad| {
            within(squad.actor, actors)
                && squad
                    .starting_locations
                    .iter()
                    .all(|l| within(l.actor, actors) && within(l.command_list, lists))
        })
    }) && campaign.command_lists.iter().all(|list| {
        list.commands
            .iter()
            .all(|command| within(command.object_name, names))
    }) && campaign.conversations.iter().all(|conversation| {
        let speakers = conversation.participants.len();
        conversation
            .participants
            .iter()
            .all(|p| within(p.object_name, names))
            && conversation
                .lines
                .iter()
                .all(|line| within(line.participant, speakers) && line.delay.is_finite())
    })
}

#[test]
fn every_scenarios_campaign_imports() -> Result {
    for map in MAPS {
        let Some(campaign) = campaign(map)? else {
            continue;
        };
        assert!(linked(&campaign), "{map}'s indices");
        eprintln!(
            "{map}: {} names, {} volumes, {} encounters, {} command lists, {} flags, {} camera points",
            campaign.object_names.len(),
            campaign.trigger_volumes.len(),
            campaign.encounters.len(),
            campaign.command_lists.len(),
            campaign.cutscene_flags.len(),
            campaign.camera_points.len()
        );
    }
    Ok(())
}

#[test]
fn the_pillar_of_autumn_has_its_encounters() -> Result {
    let Some(campaign) = campaign("a10")? else {
        return Ok(());
    };
    assert!(campaign.encounters.len() > 10);
    assert!(campaign.trigger_volumes.len() > 10);
    assert!(campaign
        .encounters
        .iter()
        .any(|e| e.squads.iter().any(|s| !s.starting_locations.is_empty())));
    // (its conversations speak: each line names a sound for some variant)
    assert!(!campaign.conversations.is_empty());
    assert!(campaign
        .conversations
        .iter()
        .flat_map(|c| &c.lines)
        .any(|line| line.dialogue.iter().any(Option::is_some)));
    // (each trigger volume holds the point at its middle, along its axes)
    for volume in campaign.trigger_volumes.iter().filter(|v| v.oriented) {
        let [f, u] = [volume.forward, volume.up];
        let left = [
            u[1] * f[2] - u[2] * f[1],
            u[2] * f[0] - u[0] * f[2],
            u[0] * f[1] - u[1] * f[0],
        ];
        let [a, b, c] = volume.extents.map(|e| e / 2.0);
        let middle: [f32; 3] =
            std::array::from_fn(|i| volume.position[i] + f[i] * a + left[i] * b + u[i] * c);
        assert!(volume.contains(middle), "{}", volume.name);
    }
    Ok(())
}

/// Whether each actor variant reads as one that fights: a unit, a range it
/// fires at and sees to, and its bounds in order.
#[test]
fn every_actor_variant_reads_as_one_that_fights() -> Result {
    for map in MAPS {
        let Some(campaign) = campaign(map)? else {
            continue;
        };
        assert_eq!(campaign.actor_variants.len(), campaign.actor_palette.len());
        for variant in campaign.actor_variants.iter().flatten() {
            let name = &variant.tag.path;
            assert!(variant.unit.is_some(), "{map}: {name}'s unit");
            assert!(
                (0.0..=1000.0).contains(&variant.firing_range),
                "{map}: {name}'s firing range {}",
                variant.firing_range
            );
            assert!(
                (0.0..=1000.0).contains(&variant.vision_distance),
                "{map}: {name}'s vision {}",
                variant.vision_distance
            );
            assert!(
                variant.combat_range[0] <= variant.combat_range[1],
                "{map}: {name}'s combat range {:?}",
                variant.combat_range
            );
            assert!(
                (0.0..=1.0).contains(&variant.shield_fraction_hide),
                "{map}: {name}'s hiding {}",
                variant.shield_fraction_hide
            );
            eprintln!(
                "{map}: {name}: type {}, fires to {:.1}, sees {:.1}, keeps {:?}",
                variant.kind, variant.firing_range, variant.vision_distance, variant.combat_range
            );
        }
    }
    Ok(())
}
