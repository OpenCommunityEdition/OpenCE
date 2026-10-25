//! The weapons a player carries: the level's weapons and their
//! projectiles (the armory), the two a player holds, its grenades, and the
//! weapons lying in the world, which keep their rounds.

use bevy_ecs::prelude::*;
use opence_content::{Level, Projectile, TagRef, Weapon};

use crate::hash::WorldHasher;
use crate::weapon::WeaponState;

/// The most weapons a player carries.
pub const MAXIMUM_WEAPONS: usize = 2;

/// How near a weapon lying in the world is picked up, in world units.
pub const PICKUP_RADIUS: f32 = 0.6;

/// The stock multiplayer game's starting weapons, by their place in the
/// globals' weapon list: the assault rifle and the pistol (the "generic"
/// set).
pub const STARTING_WEAPONS: [usize; 2] = [0, 4];

/// A weapon of the level, with the projectile each trigger fires.
#[derive(Clone, Debug, PartialEq)]
pub struct ArmoryWeapon {
    /// Its tag.
    pub tag: TagRef,
    /// Its definition.
    pub weapon: Weapon,
    /// Each trigger's projectile, if the level has it.
    pub projectiles: Vec<Option<(TagRef, Projectile)>>,
}

/// The kinds of grenade: frag, then plasma.
pub const GRENADE_KINDS: usize = 2;

/// The powerup of equipment that is a grenade.
const POWERUP_GRENADE: i16 = 6;

/// A kind of grenade of the level: its globals' entry, and its projectile
/// if the level has it.
#[derive(Clone, Debug, PartialEq)]
pub struct ArmoryGrenade {
    /// The most a player carries.
    pub maximum: i16,
    /// How many a player starts with.
    pub starting: i16,
    /// Its projectile, if the level has it.
    pub projectile: Option<(TagRef, Projectile)>,
}

/// Every weapon of the level a player can carry, and its grenades.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Armory {
    weapons: Vec<ArmoryWeapon>,
    grenades: Vec<ArmoryGrenade>,
    /// The equipment of the level that is a grenade lying in the world,
    /// its kind and the sound of its being picked up.
    grenade_items: Vec<(TagRef, usize, Option<TagRef>)>,
}

impl Armory {
    /// `level`'s weapons, but those of more triggers than a weapon has.
    pub fn new(level: &Level) -> Armory {
        let weapons = level
            .objects
            .iter()
            .filter_map(|object| {
                let weapon = object.weapon.clone()?;
                WeaponState::new(&weapon).ok()?;
                let projectiles = weapon
                    .triggers
                    .iter()
                    .map(|trigger| {
                        let tag = trigger.projectile.as_ref()?;
                        let projectile = level.object(tag)?.projectile.clone()?;
                        Some((tag.clone(), projectile))
                    })
                    .collect();
                Some(ArmoryWeapon {
                    tag: object.tag.clone(),
                    weapon,
                    projectiles,
                })
            })
            .collect();
        let grenades = level
            .globals
            .grenades
            .iter()
            .take(GRENADE_KINDS)
            .map(|grenade| ArmoryGrenade {
                maximum: grenade.maximum_count.max(0),
                starting: grenade
                    .multiplayer_spawn_default
                    .clamp(0, grenade.maximum_count.max(0)),
                projectile: grenade.projectile.as_ref().and_then(|tag| {
                    let projectile = level.object(tag)?.projectile.clone()?;
                    Some((tag.clone(), projectile))
                }),
            })
            .collect();
        let grenade_items = level
            .objects
            .iter()
            .filter_map(|object| {
                let equipment = object.equipment.as_ref()?;
                let kind = usize::try_from(equipment.grenade_type).ok()?;
                (equipment.powerup_type == POWERUP_GRENADE && kind < GRENADE_KINDS)
                    .then(|| (object.tag.clone(), kind, equipment.pickup_sound.clone()))
            })
            .collect();
        Armory {
            weapons,
            grenades,
            grenade_items,
        }
    }

    /// The kind of grenade the equipment `tag` is, if it is one.
    pub fn grenade_item(&self, tag: &TagRef) -> Option<usize> {
        self.grenade_items
            .iter()
            .find(|(item, _, _)| item == tag)
            .map(|&(_, kind, _)| kind)
    }

    /// The sound of the grenade `tag` being picked up.
    pub fn grenade_pickup_sound(&self, tag: &TagRef) -> Option<&TagRef> {
        self.grenade_items
            .iter()
            .find(|(item, _, _)| item == tag)
            .and_then(|(_, _, sound)| sound.as_ref())
    }

    /// The grenade of `kind` (0 frag, 1 plasma).
    pub fn grenade(&self, kind: usize) -> Option<&ArmoryGrenade> {
        self.grenades.get(kind)
    }

    /// The weapon of `tag`'s place.
    pub fn find(&self, tag: &TagRef) -> Option<usize> {
        self.weapons.iter().position(|weapon| weapon.tag == *tag)
    }

    /// The weapon at `index`.
    pub fn get(&self, index: usize) -> Option<&ArmoryWeapon> {
        self.weapons.get(index)
    }

    /// The number of weapons.
    pub fn len(&self) -> usize {
        self.weapons.len()
    }

    /// Whether there are none.
    pub fn is_empty(&self) -> bool {
        self.weapons.is_empty()
    }
}

/// A weapon carried: its place in the armory and its state.
#[derive(Clone, Debug, PartialEq)]
pub struct Carried {
    /// Its place in the armory.
    pub armory: usize,
    /// Its triggers, magazines and heat.
    pub state: WeaponState,
}

/// The weapons a player carries, and the one in its hands.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Inventory {
    /// Up to [`MAXIMUM_WEAPONS`], in the order they were picked up.
    pub weapons: Vec<Carried>,
    /// The one in its hands.
    pub current: usize,
    /// Its grenades of each kind.
    pub grenades: [i16; GRENADE_KINDS],
    /// The kind it throws.
    pub grenade: usize,
}

impl Inventory {
    /// The stock starting weapons the armory has, the first in hand.
    pub fn starting(level: &Level, armory: &Armory) -> Inventory {
        let weapons = STARTING_WEAPONS
            .iter()
            .filter_map(|&place| {
                let index = armory.find(level.globals.weapons.get(place)?)?;
                let state = WeaponState::new(&armory.get(index)?.weapon).ok()?;
                Some(Carried {
                    armory: index,
                    state,
                })
            })
            .collect();
        let mut grenades = [0; GRENADE_KINDS];
        for (kind, count) in grenades.iter_mut().enumerate() {
            *count = armory.grenade(kind).map_or(0, |grenade| grenade.starting);
        }
        Inventory {
            weapons,
            current: 0,
            grenades,
            grenade: 0,
        }
    }

    /// Readies the next kind of grenade it has any of; false if it has no
    /// other.
    pub fn switch_grenade(&mut self) -> bool {
        let next = (1..GRENADE_KINDS)
            .map(|step| (self.grenade + step) % GRENADE_KINDS)
            .find(|&kind| self.grenades[kind] > 0);
        match next {
            Some(kind) => {
                self.grenade = kind;
                true
            }
            None => false,
        }
    }

    /// Gives it `count` grenades of `kind`, up to the armory's maximum.
    pub fn give_grenades(&mut self, armory: &Armory, kind: usize, count: i16) {
        let Some(maximum) = armory.grenade(kind).map(|grenade| grenade.maximum) else {
            return;
        };
        if let Some(carried) = self.grenades.get_mut(kind) {
            *carried = carried.saturating_add(count).clamp(0, maximum);
        }
    }

    /// Takes one grenade of `kind` picked up, if it carries fewer than the
    /// most; false if it has no room for it.
    pub fn take_grenade(&mut self, armory: &Armory, kind: usize) -> bool {
        let Some(maximum) = armory.grenade(kind).map(|grenade| grenade.maximum) else {
            return false;
        };
        match self.grenades.get_mut(kind) {
            Some(carried) if *carried < maximum => {
                *carried += 1;
                true
            }
            _ => false,
        }
    }

    /// The weapon in its hands.
    pub fn current(&self) -> Option<&Carried> {
        self.weapons.get(self.current)
    }

    /// The weapon in its hands, to change.
    pub fn current_mut(&mut self) -> Option<&mut Carried> {
        self.weapons.get_mut(self.current)
    }

    /// Whether it carries a weapon of the armory's `index`.
    pub fn carries(&self, index: usize) -> bool {
        self.weapons.iter().any(|carried| carried.armory == index)
    }

    /// The next weapon in its hands, readied; false with fewer than two.
    pub fn swap(&mut self, armory: &Armory) -> bool {
        if self.weapons.len() < 2 {
            return false;
        }
        self.current = (self.current + 1) % self.weapons.len();
        draw(self.weapons.get_mut(self.current), armory);
        true
    }

    /// Takes `weapon` into its hands: in a free place, or in place of the
    /// one in its hands, which it gives back to be dropped.
    pub fn take(&mut self, weapon: Carried, armory: &Armory) -> Option<Carried> {
        let dropped = if self.weapons.len() < MAXIMUM_WEAPONS {
            self.weapons.push(weapon);
            self.current = self.weapons.len() - 1;
            None
        } else {
            let current = self.current.min(self.weapons.len() - 1);
            self.current = current;
            Some(std::mem::replace(&mut self.weapons[current], weapon))
        };
        draw(self.weapons.get_mut(self.current), armory);
        dropped
    }

    pub(crate) fn hash(&self, hasher: &mut WorldHasher) {
        hasher.u64(self.weapons.len() as u64);
        hasher.u64(self.current as u64);
        for carried in &self.weapons {
            hasher.u64(carried.armory as u64);
            carried.state.hash(hasher);
        }
        // (none carried hashes as before grenades)
        if self.grenades != [0; GRENADE_KINDS] || self.grenade != 0 {
            for count in self.grenades {
                hasher.u64(count as u64);
            }
            hasher.u64(self.grenade as u64);
        }
    }
}

fn draw(carried: Option<&mut Carried>, armory: &Armory) {
    if let Some(carried) = carried {
        if let Some(definition) = armory.get(carried.armory) {
            carried.state.draw(&definition.weapon);
        }
    }
}

/// A weapon lying in the world, as it was dropped: its rounds (a weapon a
/// scenario places has none of this until it is touched, and is full).
#[derive(Component, Clone, Debug, PartialEq)]
pub struct WeaponItem(pub WeaponState);

/// Moves rounds from a weapon lying in the world into a carried one of the
/// same tag, as far as its carried maximum allows; the rounds taken.
pub fn take_rounds(carried: &mut WeaponState, item: &mut WeaponState, weapon: &Weapon) -> i32 {
    let mut taken = 0;
    for ((into, from), magazine) in carried
        .magazines
        .iter_mut()
        .zip(item.magazines.iter_mut())
        .zip(&weapon.magazines)
    {
        let room = (magazine.rounds_total_maximum - into.total).max(0);
        let from_total = from.total.min(room);
        from.total -= from_total;
        let from_loaded = from.loaded.min(room - from_total);
        from.loaded -= from_loaded;
        into.total += from_total + from_loaded;
        taken += i32::from(from_total) + i32::from(from_loaded);
    }
    taken
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Group, Magazine, Trigger};

    fn tag(path: &str) -> TagRef {
        TagRef {
            group: Group(*b"weap"),
            path: path.into(),
        }
    }

    fn rifle() -> Weapon {
        Weapon {
            magazines: vec![Magazine {
                rounds_total_initial: 120,
                rounds_total_maximum: 200,
                rounds_loaded_maximum: 60,
                ..Magazine::default()
            }],
            triggers: vec![Trigger {
                magazine: Some(0),
                rounds_per_shot: 1,
                ..Trigger::default()
            }],
            ..Weapon::default()
        }
    }

    fn armory() -> Armory {
        let weapon = |path: &str| ArmoryWeapon {
            tag: tag(path),
            weapon: rifle(),
            projectiles: vec![None],
        };
        let grenade = |maximum| ArmoryGrenade {
            maximum,
            starting: 2,
            projectile: None,
        };
        Armory {
            weapons: vec![weapon("a"), weapon("b"), weapon("c")],
            grenades: vec![grenade(4), grenade(2)],
            grenade_items: vec![(tag("frag"), 0, Some(tag("pickup")))],
        }
    }

    fn carried(index: usize) -> Carried {
        match WeaponState::new(&rifle()) {
            Ok(state) => Carried {
                armory: index,
                state,
            },
            Err(error) => panic!("{error}"),
        }
    }

    #[test]
    fn two_are_carried_and_a_third_takes_the_ones_place() {
        let armory = armory();
        let mut inventory = Inventory::default();
        assert!(!inventory.swap(&armory));
        assert_eq!(inventory.take(carried(0), &armory), None);
        assert_eq!(inventory.take(carried(1), &armory), None);
        assert_eq!(inventory.current, 1);
        assert!(inventory.swap(&armory));
        assert_eq!(inventory.current().map(|c| c.armory), Some(0));
        let dropped = inventory.take(carried(2), &armory);
        assert_eq!(dropped.map(|c| c.armory), Some(0));
        assert_eq!(inventory.current().map(|c| c.armory), Some(2));
        assert!(inventory.carries(1) && inventory.carries(2) && !inventory.carries(0));
        assert_eq!(armory.find(&tag("b")), Some(1));
        assert_eq!(armory.find(&tag("d")), None);
    }

    #[test]
    fn grenades_are_given_up_to_their_maximum_and_switched_between() {
        let armory = armory();
        let mut inventory = Inventory::default();
        assert!(!inventory.switch_grenade());
        inventory.give_grenades(&armory, 1, 5);
        assert_eq!(inventory.grenades, [0, 2]);
        assert!(inventory.switch_grenade());
        assert_eq!(inventory.grenade, 1);
        assert!(!inventory.switch_grenade());
        inventory.give_grenades(&armory, 0, 1);
        assert!(inventory.switch_grenade());
        assert_eq!(inventory.grenade, 0);
        inventory.give_grenades(&armory, 2, 1);
        assert_eq!(inventory.grenades, [1, 2]);
        // (one picked up at a time, while there is room)
        assert_eq!(armory.grenade_item(&tag("frag")), Some(0));
        assert_eq!(armory.grenade_item(&tag("a")), None);
        assert_eq!(
            armory.grenade_pickup_sound(&tag("frag")),
            Some(&tag("pickup"))
        );
        assert!(inventory.take_grenade(&armory, 0));
        assert!(!inventory.take_grenade(&armory, 1));
        assert!(!inventory.take_grenade(&armory, 2));
        assert_eq!(inventory.grenades, [2, 2]);
    }

    #[test]
    fn rounds_are_taken_up_to_the_maximum() {
        let weapon = rifle();
        let mut into = carried(0).state;
        let mut item = carried(0).state;
        // (60 loaded and 60 carried each; room for 140)
        assert_eq!(take_rounds(&mut into, &mut item, &weapon), 120);
        assert_eq!(into.magazines[0].total, 180);
        assert_eq!((item.magazines[0].loaded, item.magazines[0].total), (0, 0));
        let mut more = carried(0).state;
        assert_eq!(take_rounds(&mut into, &mut more, &weapon), 20);
        assert_eq!(into.magazines[0].total, 200);
        assert_eq!(
            (more.magazines[0].loaded, more.magazines[0].total),
            (60, 40)
        );
    }
}
