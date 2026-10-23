//! The animation graph's names and the way an animation is found in it
//! (source/models/model_animation_definitions.c, source/units/units.c):
//! each list of a seat, a weapon class, a weapon type, a weapon, a
//! vehicle and a device, the seat and weapon a unit animates with, and
//! the checks that keep a broken or mismatched animation from being
//! played.
//!
//! Halo's graphs inherit nothing from one another (the inheritance block
//! came with Halo 2): a graph that lacks an animation falls back only
//! within itself, as these functions do.

use opence_content::{Animation, AnimationGraph, AnimationKind, UnitSeat, WeaponClass, WeaponType};

/// An animation of a list: its name and what it must do to the nodes
/// (none for one any kind may fill, a seat's user animations).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ListEntry {
    /// Its name.
    pub name: &'static str,
    /// The kind it must be.
    pub kind: Option<AnimationKind>,
}

const fn entry(name: &'static str, kind: Option<AnimationKind>) -> ListEntry {
    ListEntry { name, kind }
}

const B: Option<AnimationKind> = Some(AnimationKind::Base);
const O: Option<AnimationKind> = Some(AnimationKind::Overlay);
const R: Option<AnimationKind> = Some(AnimationKind::Replacement);
const ANY: Option<AnimationKind> = None;

/// A weapon type's animations.
pub const WEAPON_TYPE: [ListEntry; 10] = [
    entry("reload-1", R),
    entry("reload-2", R),
    entry("chamber-1", O),
    entry("chamber-2", O),
    entry("fire-1", O),
    entry("fire-2", O),
    entry("charged-1", O),
    entry("charged-2", O),
    entry("melee", O),
    entry("overheat", O),
];

/// A weapon class's animations.
pub const WEAPON_CLASS: [ListEntry; 55] = [
    entry("idle", B),
    entry("gesture", B),
    entry("turn-left", B),
    entry("turn-right", B),
    entry("dive-front", B),
    entry("dive-back", B),
    entry("dive-left", B),
    entry("dive-right", B),
    entry("move-front", B),
    entry("move-back", B),
    entry("move-left", B),
    entry("move-right", B),
    entry("slide-front", B),
    entry("slide-back", B),
    entry("slide-left", B),
    entry("slide-right", B),
    entry("airborne", B),
    entry("land-soft", B),
    entry("land-hard", B),
    entry("unused", O),
    entry("throw-grenade", B),
    entry("disarm", R),
    entry("drop", R),
    entry("ready", R),
    entry("put-away", R),
    entry("aim-still", O),
    entry("aim-move", O),
    entry("surprise-front", B),
    entry("surprise-back", B),
    entry("berserk", B),
    entry("evade-left", B),
    entry("evade-right", B),
    entry("signal-move", B),
    entry("signal-attack", B),
    entry("warn", B),
    entry("stunned-front", B),
    entry("stunned-back", B),
    entry("stunned-left", B),
    entry("stunned-right", B),
    entry("melee", B),
    entry("celebrate", B),
    entry("panic", B),
    entry("melee-airborne", B),
    entry("flaming", B),
    entry("resurrect-front", B),
    entry("resurrect-back", B),
    entry("melee-continuous", B),
    entry("feeding", B),
    entry("leap-start", B),
    entry("leap-airborne", B),
    entry("leap-melee", B),
    entry("zapping", B),
    entry("unused", B),
    entry("unused", B),
    entry("unused", B),
];

/// A unit seat's animations.
pub const UNIT_SEAT: [ListEntry; 30] = [
    entry("airborne-dead", B),
    entry("landing-dead", B),
    entry("acc-front-back", O),
    entry("acc-left-right", O),
    entry("acc-up-down", O),
    entry("push", O),
    entry("twist", O),
    entry("enter", B),
    entry("exit", B),
    entry("look", O),
    entry("talk", O),
    entry("emotions", ANY),
    entry("unused", ANY),
    entry("user0", ANY),
    entry("user1", ANY),
    entry("user2", ANY),
    entry("user3", ANY),
    entry("user4", ANY),
    entry("user5", ANY),
    entry("user6", ANY),
    entry("user7", ANY),
    entry("user8", ANY),
    entry("user9", ANY),
    entry("flying-front", B),
    entry("flying-back", B),
    entry("flying-left", B),
    entry("flying-right", B),
    entry("opening", B),
    entry("closing", B),
    entry("hovering", B),
];

/// A first-person weapon's animations.
pub const FIRST_PERSON_WEAPON: [ListEntry; 28] = [
    entry("idle", B),
    entry("posing", B),
    entry("fire-1", B),
    entry("moving", O),
    entry("overlays", O),
    entry("light-off", B),
    entry("light-on", B),
    entry("reload-empty", B),
    entry("reload-full", B),
    entry("overheated", B),
    entry("ready", B),
    entry("put-away", B),
    entry("overcharged", B),
    entry("melee", B),
    entry("fire-2", B),
    entry("overcharged-jitter", O),
    entry("throw-grenade", B),
    entry("ammunition", O),
    entry("misfire-1", B),
    entry("misfire-2", B),
    entry("throw-overheated", B),
    entry("overheating", B),
    entry("overheating-again", B),
    entry("enter", B),
    entry("exit-empty", B),
    entry("exit-full", B),
    entry("o-h-exit", B),
    entry("o-h-s-enter", B),
];

/// A weapon's own (third-person) animations.
pub const WEAPON: [ListEntry; 11] = [
    entry("idle", B),
    entry("ready", B),
    entry("put-away", B),
    entry("reload-1", B),
    entry("reload-2", B),
    entry("chamber-1", B),
    entry("chamber-2", B),
    entry("charged-1", B),
    entry("charged-2", B),
    entry("fire-1", B),
    entry("fire-2", B),
];

/// A vehicle's animations.
pub const VEHICLE: [ListEntry; 8] = [
    entry("steering", O),
    entry("roll", O),
    entry("throttle", O),
    entry("velocity", O),
    entry("braking", O),
    entry("ground-speed", O),
    entry("occupied", O),
    entry("unoccupied", O),
];

/// A device's animations.
pub const DEVICE: [ListEntry; 2] = [entry("position", O), entry("power", O)];

/// The kinds of damage a unit flinches or dies from.
pub const DAMAGE_TYPES: [&str; 4] = ["s-ping", "h-ping", "s-kill", "h-kill"];
/// The directions it is hit from.
pub const DAMAGE_DIRECTIONS: [&str; 4] = ["front", "left", "right", "back"];
/// The parts it is hit in.
pub const DAMAGE_PARTS: [&str; 11] = [
    "gut", "chest", "head", "l-arm", "l-hand", "l-leg", "l-foot", "r-arm", "r-hand", "r-leg",
    "r-foot",
];

/// Where `name` is in `list` (the first of its name).
pub fn list_index(list: &[ListEntry], name: &str) -> Option<usize> {
    list.iter().position(|entry| entry.name == name)
}

/// The name a unit's damage animation goes by (`s-ping front head`).
pub fn damage_name(kind: usize, direction: usize, part: usize) -> Option<String> {
    Some(format!(
        "{} {} {}",
        DAMAGE_TYPES.get(kind)?,
        DAMAGE_DIRECTIONS.get(direction)?,
        DAMAGE_PARTS.get(part)?
    ))
}

/// The seat, weapon class and weapon type a unit animates with
/// (`unit.animation.seat_index` and the others).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SeatAndWeapon {
    /// The seat.
    pub seat: usize,
    /// The weapon class in it.
    pub class: usize,
    /// The weapon type in that.
    pub weapon_type: usize,
    /// Whether the seat sways as the unit accelerates (it has an
    /// acceleration overlay).
    pub showing_acceleration: bool,
}

/// Whether a weapon type answers to `weapon_label`: any, with none
/// asked for; the type of no label for `unarmed`; else the type of that
/// label, its case aside.
fn type_matches(weapon_type: &WeaponType, weapon_label: Option<&str>) -> bool {
    match weapon_label {
        None => true,
        Some(label) => {
            (label == "unarmed" && weapon_type.label.is_empty())
                || label.eq_ignore_ascii_case(&weapon_type.label)
        }
    }
}

/// The seat and weapon a unit labelled so animates with
/// (`unit_set_or_test_seat_and_weapon_label`): the last seat of
/// `seat_label` (any, with none) with a class holding a type that answers
/// to `weapon_label`, the last such class of it, the first such type of
/// that; none when no seat has one (the unit then keeps what it had).
pub fn seat_and_weapon(
    graph: &AnimationGraph,
    seat_label: Option<&str>,
    weapon_label: Option<&str>,
) -> Option<SeatAndWeapon> {
    /// The seat's acceleration overlays (`acc-front-back` to `acc-up-down`).
    const ACCELERATION: [usize; 3] = [2, 3, 4];
    let mut found = None;
    for (seat_index, seat) in graph.unit_seats.iter().enumerate() {
        if seat_label.is_some_and(|label| !label.eq_ignore_ascii_case(&seat.label)) {
            continue;
        }
        for (class_index, class) in seat.weapon_classes.iter().enumerate() {
            if let Some(type_index) = class
                .weapon_types
                .iter()
                .position(|weapon_type| type_matches(weapon_type, weapon_label))
            {
                found = Some(SeatAndWeapon {
                    seat: seat_index,
                    class: class_index,
                    weapon_type: type_index,
                    showing_acceleration: ACCELERATION
                        .iter()
                        .any(|&index| seat.animations.get(index).copied().flatten().is_some()),
                });
            }
        }
    }
    found
}

impl SeatAndWeapon {
    /// Its seat in `graph`.
    pub fn seat<'a>(&self, graph: &'a AnimationGraph) -> Option<&'a UnitSeat> {
        graph.unit_seats.get(self.seat)
    }

    /// Its weapon class in `graph`.
    pub fn class<'a>(&self, graph: &'a AnimationGraph) -> Option<&'a WeaponClass> {
        self.seat(graph)?.weapon_classes.get(self.class)
    }

    /// Its weapon type in `graph`.
    pub fn weapon_type<'a>(&self, graph: &'a AnimationGraph) -> Option<&'a WeaponType> {
        self.class(graph)?.weapon_types.get(self.weapon_type)
    }

    /// Its seat's animation `index` ([`UNIT_SEAT`]).
    pub fn seat_animation(&self, graph: &AnimationGraph, index: usize) -> Option<u16> {
        listed(graph, &self.seat(graph)?.animations, index)
    }

    /// Its weapon class's animation `index` ([`WEAPON_CLASS`]).
    pub fn class_animation(&self, graph: &AnimationGraph, index: usize) -> Option<u16> {
        listed(graph, &self.class(graph)?.animations, index)
    }

    /// Its weapon type's animation `index` ([`WEAPON_TYPE`]); a reload the
    /// type lacks is the standing seat's of the same class and type, when
    /// `borrow_reload` (`game.enhanced_animations`, for the cyborg: a
    /// reload is a replacement, which moves only the arms and the weapon).
    pub fn type_animation(
        &self,
        graph: &AnimationGraph,
        index: usize,
        borrow_reload: bool,
    ) -> Option<u16> {
        let weapon_type = self.weapon_type(graph)?;
        listed(graph, &weapon_type.animations, index).or_else(|| {
            if !borrow_reload || index > 1 {
                return None;
            }
            let class = self.class(graph)?;
            let stand = graph
                .unit_seats
                .iter()
                .find(|seat| seat.label.eq_ignore_ascii_case("stand"))?;
            let stand_class = stand
                .weapon_classes
                .iter()
                .find(|other| other.label.eq_ignore_ascii_case(&class.label))?;
            let stand_type = stand_class
                .weapon_types
                .iter()
                .find(|other| other.label.eq_ignore_ascii_case(&weapon_type.label))?;
            listed(graph, &stand_type.animations, index)
        })
    }
}

/// The animation at `index` of a list of `graph`'s: none past the
/// list's end, for none there, or for one past the graph's animations.
pub fn listed(graph: &AnimationGraph, list: &[Option<u16>], index: usize) -> Option<u16> {
    list.get(index)
        .copied()
        .flatten()
        .filter(|&animation| usize::from(animation) < graph.animations.len())
}

/// A unit's damage animation (`build_damage_animation_index`).
pub fn damage_animation(
    graph: &AnimationGraph,
    kind: usize,
    direction: usize,
    part: usize,
) -> Option<u16> {
    listed(
        graph,
        &graph.unit_damage,
        AnimationGraph::damage_index(kind, direction, part)?,
    )
}

/// Whether `animation` may pose a model of `model_nodes` nodes as
/// `kind`: it has frames, it is of that kind (when one is asked), and a
/// base animation moves each of the model's nodes while any other moves
/// no more than it has.
pub fn fits(animation: &Animation, kind: Option<AnimationKind>, model_nodes: usize) -> bool {
    let nodes = usize::from(animation.node_count);
    animation.frame_count >= 1
        && kind.is_none_or(|kind| kind == animation.kind)
        && match animation.kind {
            AnimationKind::Base => nodes == model_nodes,
            AnimationKind::Overlay | AnimationKind::Replacement => nodes <= model_nodes,
        }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{AnimationGraph, UnitSeat, WeaponClass, WeaponType};

    fn animation(kind: AnimationKind, frames: u16, nodes: u16) -> Animation {
        let mut animation = crate::animation::tests::animation_named("a", nodes);
        animation.kind = kind;
        animation.frame_count = frames;
        animation
    }

    fn weapon_type(label: &str, animations: Vec<Option<u16>>) -> WeaponType {
        WeaponType {
            label: label.into(),
            animations,
        }
    }

    fn class(label: &str, types: Vec<WeaponType>) -> WeaponClass {
        WeaponClass {
            label: label.into(),
            animations: vec![Some(0)],
            weapon_types: types,
            ..WeaponClass::default()
        }
    }

    fn seat(label: &str, classes: Vec<WeaponClass>) -> UnitSeat {
        UnitSeat {
            label: label.into(),
            weapon_classes: classes,
            ..UnitSeat::default()
        }
    }

    /// A cyborg's graph, cut down: a standing and a passenger's seat.
    fn graph() -> AnimationGraph {
        let mut graph = crate::animation::tests::graph_of(vec![
            animation(AnimationKind::Base, 10, 2),
            animation(AnimationKind::Replacement, 10, 2),
            animation(AnimationKind::Replacement, 10, 2),
        ]);
        let mut stand = seat(
            "stand",
            vec![
                class(
                    "rifle",
                    vec![
                        weapon_type("ar", vec![Some(1), Some(2)]),
                        weapon_type("ne", vec![]),
                    ],
                ),
                class(
                    "pistol",
                    vec![weapon_type("hp", vec![]), weapon_type("NE", vec![])],
                ),
                class("empty", vec![]),
                class("unarmed", vec![weapon_type("", vec![])]),
            ],
        );
        stand.animations = vec![None, None, None, Some(0)];
        let passenger = seat(
            "W-passenger",
            vec![class("rifle", vec![weapon_type("ar", vec![])])],
        );
        graph.unit_seats = vec![stand, passenger];
        graph
    }

    #[test]
    fn the_lists_are_the_games() {
        assert_eq!(list_index(&WEAPON_CLASS, "idle"), Some(0));
        assert_eq!(list_index(&WEAPON_CLASS, "throw-grenade"), Some(20));
        assert_eq!(list_index(&WEAPON_CLASS, "put-away"), Some(24));
        assert_eq!(list_index(&WEAPON_CLASS, "zapping"), Some(51));
        assert_eq!(list_index(&UNIT_SEAT, "enter"), Some(7));
        assert_eq!(list_index(&UNIT_SEAT, "hovering"), Some(29));
        assert_eq!(UNIT_SEAT[13].kind, None);
        assert_eq!(list_index(&FIRST_PERSON_WEAPON, "o-h-s-enter"), Some(27));
        assert_eq!(list_index(&WEAPON_TYPE, "overheat"), Some(9));
        assert_eq!(list_index(&WEAPON, "fire-2"), Some(10));
        assert_eq!(list_index(&VEHICLE, "unoccupied"), Some(7));
        assert_eq!(list_index(&DEVICE, "power"), Some(1));
        assert_eq!(list_index(&DEVICE, "nothing"), None);
        assert_eq!(damage_name(1, 3, 10).as_deref(), Some("h-ping back r-foot"));
        assert_eq!(damage_name(4, 0, 0), None);
        assert_eq!(
            AnimationGraph::damage_index(1, 3, 10),
            Some((4 + 3) * 11 + 10)
        );
    }

    #[test]
    fn a_unit_finds_its_seat_and_weapon_as_the_game_did() -> Result<(), &'static str> {
        let graph = graph();
        let found = |seat, weapon| {
            seat_and_weapon(&graph, seat, weapon).map(|f| (f.seat, f.class, f.weapon_type))
        };
        assert_eq!(found(Some("stand"), Some("ar")), Some((0, 0, 0)));
        // its case aside, and the last class holding it wins
        assert_eq!(found(Some("STAND"), Some("ne")), Some((0, 1, 1)));
        // unarmed is the type of no label; a class with no types never is
        assert_eq!(found(Some("stand"), Some("unarmed")), Some((0, 3, 0)));
        assert_eq!(found(Some("stand"), Some("rl")), None);
        assert_eq!(found(Some("crouch"), Some("ar")), None);
        // with no seat asked, the last seat that has it
        assert_eq!(found(None, Some("ar")), Some((1, 0, 0)));
        // with no weapon asked, the first type of the last class
        assert_eq!(found(Some("stand"), None), Some((0, 3, 0)));
        let stand = seat_and_weapon(&graph, Some("stand"), Some("ar")).ok_or("none")?;
        assert!(stand.showing_acceleration);
        let passenger = seat_and_weapon(&graph, None, Some("ar")).ok_or("none")?;
        assert!(!passenger.showing_acceleration);
        Ok(())
    }

    #[test]
    fn an_animation_missing_or_out_of_range_is_none() -> Result<(), &'static str> {
        let mut graph = graph();
        let stand = seat_and_weapon(&graph, Some("stand"), Some("ar")).ok_or("none")?;
        assert_eq!(stand.class_animation(&graph, 0), Some(0));
        assert_eq!(stand.class_animation(&graph, 54), None);
        assert_eq!(stand.seat_animation(&graph, 3), Some(0));
        assert_eq!(stand.seat_animation(&graph, 0), None);
        assert_eq!(stand.type_animation(&graph, 1, false), Some(2));
        graph.unit_seats[0].weapon_classes[0].weapon_types[0].animations[1] = Some(99);
        assert_eq!(stand.type_animation(&graph, 1, false), None);
        let gone = SeatAndWeapon { seat: 7, ..stand };
        assert_eq!(gone.class_animation(&graph, 0), None);
        assert_eq!(gone.type_animation(&graph, 0, true), None);
        graph.unit_damage = vec![None; AnimationGraph::DAMAGE_ANIMATIONS];
        graph.unit_damage[AnimationGraph::damage_index(0, 0, 2).ok_or("none")?] = Some(0);
        assert_eq!(damage_animation(&graph, 0, 0, 2), Some(0));
        assert_eq!(damage_animation(&graph, 0, 0, 3), None);
        assert_eq!(damage_animation(&graph, 9, 0, 3), None);
        Ok(())
    }

    #[test]
    fn a_seat_with_no_reload_borrows_the_standing_ones_when_enhanced() -> Result<(), &'static str> {
        let graph = graph();
        let passenger = seat_and_weapon(&graph, Some("w-passenger"), Some("ar")).ok_or("none")?;
        assert_eq!(passenger.type_animation(&graph, 0, false), None);
        assert_eq!(passenger.type_animation(&graph, 0, true), Some(1));
        assert_eq!(passenger.type_animation(&graph, 1, true), Some(2));
        // only the reloads are borrowed
        assert_eq!(passenger.type_animation(&graph, 4, true), None);
        let mut lone = graph.clone();
        lone.unit_seats.remove(0);
        let passenger = seat_and_weapon(&lone, None, Some("ar")).ok_or("none")?;
        assert_eq!(passenger.type_animation(&lone, 0, true), None);
        Ok(())
    }

    #[test]
    fn an_animation_that_does_not_fit_the_model_is_not_played() {
        let base = animation(AnimationKind::Base, 10, 19);
        assert!(fits(&base, Some(AnimationKind::Base), 19));
        assert!(fits(&base, None, 19));
        assert!(!fits(&base, Some(AnimationKind::Base), 20));
        assert!(!fits(&base, Some(AnimationKind::Overlay), 19));
        assert!(!fits(&animation(AnimationKind::Base, 0, 19), None, 19));
        let overlay = animation(AnimationKind::Overlay, 4, 12);
        assert!(fits(&overlay, Some(AnimationKind::Overlay), 19));
        assert!(!fits(&overlay, None, 11));
        assert!(fits(&animation(AnimationKind::Replacement, 1, 19), R, 19));
    }
}
