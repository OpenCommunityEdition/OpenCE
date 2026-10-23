//! The animations the AI's behaviour plays on a unit: its alerts, dives,
//! evades, berserk, signals, celebrations and panic
//! (`unit_animation_impulse_get_index`, `unit_can_play_animation_impulse`,
//! `unit_start_animation_impulse` in units.c; the dive choice of
//! `actor_action_try_to_dive` and the surprise of
//! `actor_action_handle_surprise` in actions.c).

use glam::Vec2;
use opence_content::{AnimationGraph, WeaponClass};

use crate::unit_states::UnitState;

/// An animation the AI asks a unit to play (`_unit_animation_impulse`).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[allow(missing_docs)]
pub enum AnimationImpulse {
    Berserk,
    SignalMove,
    SignalAttack,
    SignalWarn,
    SurpriseFront,
    SurpriseBack,
    EvadeLeft,
    EvadeRight,
    DivingFront,
    DivingBack,
    DivingLeft,
    DivingRight,
    Celebrate,
    Panic,
}

/// A seat whose rider may celebrate and panic (`_unit_seat_unknown8_bit`).
pub const SEAT_ALLOWS_IMPULSES: u32 = 1 << 8;

impl AnimationImpulse {
    /// Every impulse, in the game's order.
    pub const ALL: [Self; 14] = [
        Self::Berserk,
        Self::SignalMove,
        Self::SignalAttack,
        Self::SignalWarn,
        Self::SurpriseFront,
        Self::SurpriseBack,
        Self::EvadeLeft,
        Self::EvadeRight,
        Self::DivingFront,
        Self::DivingBack,
        Self::DivingLeft,
        Self::DivingRight,
        Self::Celebrate,
        Self::Panic,
    ];

    /// Its animation among the weapon class's
    /// (`_unit_weapon_class_animation_*`).
    pub fn class_animation(self) -> usize {
        match self {
            Self::DivingFront => 4,
            Self::DivingBack => 5,
            Self::DivingLeft => 6,
            Self::DivingRight => 7,
            Self::SurpriseFront => 27,
            Self::SurpriseBack => 28,
            Self::Berserk => 29,
            Self::EvadeLeft => 30,
            Self::EvadeRight => 31,
            Self::SignalMove => 32,
            Self::SignalAttack => 33,
            Self::SignalWarn => 34,
            Self::Celebrate => 40,
            Self::Panic => 41,
        }
    }

    /// The frames the unit blends into it over: a surprise and a dive
    /// are sudden.
    pub fn interpolation_frames(self) -> u16 {
        match self {
            Self::SurpriseFront
            | Self::SurpriseBack
            | Self::DivingFront
            | Self::DivingBack
            | Self::DivingLeft
            | Self::DivingRight => 3,
            _ => 6,
        }
    }

    /// Whether a unit may only play it riding (a celebration, a panic).
    fn riding(self) -> bool {
        matches!(self, Self::Celebrate | Self::Panic)
    }
}

/// Whether a unit in `state`, riding a seat of `seat_flags` (none on
/// foot), may start `impulse`: not while it is hurt, dying, getting in or
/// out, already in an impulse, striking or throwing; a rider only
/// celebrates and panics, and only in a seat that lets it; one on foot
/// does everything else.
pub fn can_play(state: UnitState, seat_flags: Option<u32>, impulse: AnimationImpulse) -> bool {
    use UnitState as S;
    let busy = matches!(
        state,
        S::HardPing
            | S::DyingAirborne
            | S::Dying
            | S::EnteringSeat
            | S::ExitingSeat
            | S::AiImpulse
            | S::MeleeAttack
            | S::MeleeAirborne
            | S::MeleeContinuous
            | S::ThrowGrenade
            | S::ResurrectFront
            | S::ResurrectBack
            | S::LeapStart
            | S::LeapMelee
    );
    if busy {
        return false;
    }
    match seat_flags {
        Some(flags) => impulse.riding() && flags & SEAT_ALLOWS_IMPULSES != 0,
        None => !impulse.riding(),
    }
}

/// The weapon class a unit playing `animation` is in: the class whose
/// animations list it, else its standing seat's unarmed (or first) class
/// (`unit_test_animation_impulse` reads the unit's own seat and weapon).
pub fn class_of(graph: &AnimationGraph, animation: u16) -> Option<&WeaponClass> {
    let mut classes = graph
        .unit_seats
        .iter()
        .flat_map(|seat| seat.weapon_classes.iter());
    if let Some(class) = classes.find(|class| class.animations.contains(&Some(animation))) {
        return Some(class);
    }
    let seat = graph
        .unit_seats
        .iter()
        .find(|seat| seat.label == "stand")
        .or(graph.unit_seats.first())?;
    seat.weapon_classes
        .iter()
        .find(|class| class.label == "unarmed")
        .or(seat.weapon_classes.first())
}

/// The animation `class` plays for `impulse`, if its graph has it
/// (`unit_test_animation_impulse`).
pub fn impulse_animation(
    graph: &AnimationGraph,
    class: &WeaponClass,
    impulse: AnimationImpulse,
) -> Option<u16> {
    class
        .animations
        .get(impulse.class_animation())
        .copied()
        .flatten()
        .filter(|&animation| usize::from(animation) < graph.animations.len())
}

/// A way to escape (`_actor_evade_*`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Escape {
    /// To the left of `alignment`.
    Left,
    /// To the right of it.
    Right,
    /// Along it.
    Forward,
    /// Along it too (the game moves back along the same vector).
    Back,
}

/// The dives and evades an actor picks among, each with the way it moves
/// (left, right, front, back) and its bias over the others
/// (`global_dive_animation_table`): a dive is preferred to an evade.
const DIVES: [(AnimationImpulse, usize, f32); 6] = [
    (AnimationImpulse::DivingLeft, 0, 1.5),
    (AnimationImpulse::EvadeLeft, 0, 0.0),
    (AnimationImpulse::DivingRight, 1, 1.5),
    (AnimationImpulse::EvadeRight, 1, 0.0),
    (AnimationImpulse::DivingFront, 2, 1.5),
    (AnimationImpulse::DivingBack, 3, 1.5),
];

/// The dive or evade an actor facing `facing` plays to escape `escape`
/// of `alignment`, and the way it faces as it goes; none if its graph has
/// none that moves within a right angle and a half of the way
/// (`actor_action_try_to_dive`). `has` says whether the unit has an
/// impulse's animation.
pub fn choose_dive(
    facing: Vec2,
    escape: Escape,
    alignment: Vec2,
    has: impl Fn(AnimationImpulse) -> bool,
) -> Option<(AnimationImpulse, Vec2)> {
    let evade = match escape {
        Escape::Left => Vec2::new(-alignment.y, alignment.x),
        Escape::Right => Vec2::new(alignment.y, -alignment.x),
        Escape::Forward | Escape::Back => alignment,
    };
    let left = Vec2::new(-facing.y, facing.x);
    let front = evade.dot(facing);
    let side = evade.dot(left);
    let desire = [side, -side, front, -front];
    let mut best: Option<(AnimationImpulse, usize)> = None;
    let mut best_score = -0.5;
    for (impulse, direction, bias) in DIVES {
        let score = desire[direction] + bias;
        if best_score < score && has(impulse) {
            best = Some((impulse, direction));
            best_score = score;
        }
    }
    let (impulse, direction) = best?;
    let facing = match direction {
        0 => Vec2::new(evade.y, -evade.x),
        1 => Vec2::new(-evade.y, evade.x),
        _ => evade,
    };
    Some((impulse, facing))
}

/// The surprise an actor facing `desired_facing` plays at `surprise` (the
/// way to what surprised it, if it knows), and the way it then faces
/// (`actor_action_handle_surprise`): a start backward from what is
/// behind it, else forward.
pub fn choose_surprise(surprise: Option<Vec2>, desired_facing: Vec2) -> (AnimationImpulse, Vec2) {
    match surprise {
        Some(vector) if vector.dot(desired_facing) < 0.0 => {
            (AnimationImpulse::SurpriseBack, -vector)
        }
        Some(vector) => (AnimationImpulse::SurpriseFront, vector),
        None => (AnimationImpulse::SurpriseFront, desired_facing),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Animation, AnimationKind, ScreenBounds, UnitSeat};

    fn class(label: &str, animations: Vec<Option<u16>>) -> WeaponClass {
        WeaponClass {
            label: label.into(),
            grip_marker: String::new(),
            hand_marker: String::new(),
            aiming_bounds: ScreenBounds::default(),
            animations,
            ik_points: Vec::new(),
            weapon_types: Vec::new(),
        }
    }

    fn graph(classes: Vec<WeaponClass>, animations: usize) -> AnimationGraph {
        let animation = |name: String| Animation {
            name,
            kind: AnimationKind::Base,
            ..crate::animation::tests::animation_named("a", 1)
        };
        AnimationGraph {
            unit_seats: vec![UnitSeat {
                label: "stand".into(),
                looking_bounds: ScreenBounds::default(),
                animations: Vec::new(),
                ik_points: Vec::new(),
                weapon_classes: classes,
            }],
            animations: (0..animations)
                .map(|i| animation(format!("a{i}")))
                .collect(),
            ..crate::animation::tests::graph_of(Vec::new())
        }
    }

    #[test]
    fn each_impulse_has_the_games_slot_and_blend() {
        let slots: Vec<usize> = AnimationImpulse::ALL
            .iter()
            .map(|impulse| impulse.class_animation())
            .collect();
        assert_eq!(slots, [29, 32, 33, 34, 27, 28, 30, 31, 4, 5, 6, 7, 40, 41]);
        let frames: Vec<u16> = AnimationImpulse::ALL
            .iter()
            .map(|impulse| impulse.interpolation_frames())
            .collect();
        assert_eq!(frames, [6, 6, 6, 6, 3, 3, 6, 6, 3, 3, 3, 3, 6, 6]);
    }

    #[test]
    fn a_busy_unit_plays_no_impulse() {
        for state in [
            UnitState::Dying,
            UnitState::AiImpulse,
            UnitState::ThrowGrenade,
        ] {
            for impulse in AnimationImpulse::ALL {
                assert!(!can_play(state, None, impulse));
            }
        }
    }

    #[test]
    fn a_rider_only_celebrates_and_panics_where_its_seat_lets_it() {
        for impulse in AnimationImpulse::ALL {
            let riding = matches!(
                impulse,
                AnimationImpulse::Celebrate | AnimationImpulse::Panic
            );
            assert_eq!(can_play(UnitState::Idle, None, impulse), !riding);
            assert_eq!(
                can_play(UnitState::Idle, Some(SEAT_ALLOWS_IMPULSES), impulse),
                riding
            );
            assert!(!can_play(UnitState::Idle, Some(0), impulse));
        }
    }

    #[test]
    fn the_impulse_is_the_playing_classs() {
        let mut rifle = vec![None; 42];
        rifle[0] = Some(1);
        rifle[29] = Some(2);
        let unarmed = class("unarmed", vec![Some(0)]);
        let graph = graph(vec![unarmed, class("rifle", rifle)], 3);
        let playing = class_of(&graph, 1).map(|class| class.label.as_str());
        assert_eq!(playing, Some("rifle"));
        let standing = class_of(&graph, 7).map(|class| class.label.as_str());
        assert_eq!(standing, Some("unarmed"));
        let rifle = class_of(&graph, 1).ok_or("no class");
        assert_eq!(
            rifle.map(|c| impulse_animation(&graph, c, AnimationImpulse::Berserk)),
            Ok(Some(2))
        );
        assert_eq!(
            rifle.map(|c| impulse_animation(&graph, c, AnimationImpulse::Panic)),
            Ok(None)
        );
    }

    #[test]
    fn an_impulse_past_the_graphs_animations_is_none() {
        let mut animations = vec![None; 42];
        animations[29] = Some(9);
        let graph = graph(vec![class("unarmed", animations)], 3);
        let unarmed = &graph.unit_seats[0].weapon_classes[0];
        assert_eq!(
            impulse_animation(&graph, unarmed, AnimationImpulse::Berserk),
            None
        );
        let empty = AnimationGraph {
            unit_seats: Vec::new(),
            ..graph.clone()
        };
        assert!(class_of(&empty, 0).is_none());
    }

    #[test]
    fn a_dive_goes_the_way_out() -> Result<(), &'static str> {
        let all = |_| true;
        let facing = Vec2::X;
        // (escaping left of a threat ahead: a dive to the left)
        let (impulse, along) = choose_dive(facing, Escape::Left, Vec2::X, all).ok_or("no dive")?;
        assert_eq!(impulse, AnimationImpulse::DivingLeft);
        assert!((along - Vec2::X).length() < 1e-6);
        let right = choose_dive(facing, Escape::Right, Vec2::X, all).map(|d| d.0);
        assert_eq!(right, Some(AnimationImpulse::DivingRight));
        let front = choose_dive(facing, Escape::Forward, Vec2::X, all);
        assert_eq!(front, Some((AnimationImpulse::DivingFront, Vec2::X)));
        let back = choose_dive(facing, Escape::Back, -Vec2::X, all).map(|d| d.0);
        assert_eq!(back, Some(AnimationImpulse::DivingBack));
        // (with no dives, the evade of the same side)
        let evades = |i| {
            matches!(
                i,
                AnimationImpulse::EvadeLeft | AnimationImpulse::EvadeRight
            )
        };
        let (evade, along) =
            choose_dive(facing, Escape::Right, Vec2::X, evades).ok_or("no evade")?;
        assert_eq!(evade, AnimationImpulse::EvadeRight);
        assert!((along - Vec2::X).length() < 1e-6);
        // (nothing within reach of the way out)
        let left_only = |i| i == AnimationImpulse::EvadeLeft;
        assert_eq!(choose_dive(facing, Escape::Right, Vec2::X, left_only), None);
        assert_eq!(choose_dive(facing, Escape::Left, Vec2::X, |_| false), None);
        Ok(())
    }

    #[test]
    fn a_surprise_from_behind_starts_backward() {
        let facing = Vec2::X;
        assert_eq!(
            choose_surprise(Some(-Vec2::X), facing),
            (AnimationImpulse::SurpriseBack, Vec2::X)
        );
        assert_eq!(
            choose_surprise(Some(Vec2::Y), facing),
            (AnimationImpulse::SurpriseFront, Vec2::Y)
        );
        assert_eq!(
            choose_surprise(None, facing),
            (AnimationImpulse::SurpriseFront, facing)
        );
    }
}
