//! The weapons a player carries: the level's weapons and their
//! projectiles (the armory), the two a player holds, and the weapons lying
//! in the world, which keep their rounds.

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

/// Every weapon of the level a player can carry.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Armory {
    weapons: Vec<ArmoryWeapon>,
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
        Armory { weapons }
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
        Inventory {
            weapons,
            current: 0,
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
        Armory {
            weapons: vec![weapon("a"), weapon("b"), weapon("c")],
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
