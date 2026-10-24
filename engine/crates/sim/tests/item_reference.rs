//! An item's motion against the game's own, tick by tick: falls, throws,
//! slopes and pushes over one plane (tools/engine_item_reference.py writes
//! the table from source/items/items.c).

use glam::Vec3;
use opence_content::Item;
use opence_physics::{BodyId, Struck};
use opence_sim::item_physics::{ItemHit, ItemMotion, ItemWorld};
use opence_sim::random::RandomSeed;

const REFERENCE: &str = include_str!("item_reference.txt");

type Failure = Box<dyn std::error::Error>;

/// The plane through the origin, the level's.
struct Plane(Vec3);

impl Plane {
    fn distance(&self, point: Vec3) -> f32 {
        self.0.dot(point)
    }
}

impl ItemWorld for Plane {
    fn line(&self, from: Vec3, to: Vec3) -> Option<ItemHit> {
        let (d0, d1) = (self.distance(from), self.distance(to));
        if !(d0 >= 0.0 && d1 < 0.0) {
            return None;
        }
        let t = d0 / (d0 - d1);
        Some(ItemHit {
            point: from + (to - from) * t,
            normal: self.0,
            struck: Struck::Level,
        })
    }

    fn inside(&self, point: Vec3) -> bool {
        self.distance(point) < 0.0
    }

    fn body_point(&self, _body: BodyId, _offset: Vec3) -> Option<Vec3> {
        None
    }

    fn body_offset(&self, _body: BodyId, _point: Vec3) -> Option<Vec3> {
        None
    }
}

/// The game's item after a line, as its fields.
struct Expected {
    position: Vec3,
    velocity: Vec3,
    forward: Vec3,
    up: Vec3,
    angular_velocity: Vec3,
    at_rest: bool,
    detonation_ticks: u32,
}

fn numbers(fields: &[&str]) -> Result<Vec<f32>, Failure> {
    fields
        .iter()
        .map(|field| Ok(field.parse::<f32>()?))
        .collect()
}

fn vector(numbers: &[f32], at: usize) -> Result<Vec3, Failure> {
    let slice = numbers.get(at..at + 3).ok_or("a short vector")?;
    Ok(Vec3::new(slice[0], slice[1], slice[2]))
}

/// A line's words before the bar, and the item after it.
fn split(line: &str) -> Result<(Vec<&str>, Expected), Failure> {
    let (head, tail) = line.split_once(" | ").ok_or("no item on the line")?;
    let values = numbers(&tail.split_whitespace().collect::<Vec<_>>())?;
    if values.len() != 17 {
        return Err(format!("an item of {} numbers", values.len()).into());
    }
    let expected = Expected {
        position: vector(&values, 0)?,
        velocity: vector(&values, 3)?,
        forward: vector(&values, 6)?,
        up: vector(&values, 9)?,
        angular_velocity: vector(&values, 12)?,
        at_rest: values[15] != 0.0,
        detonation_ticks: values[16] as u32,
    };
    Ok((head.split_whitespace().collect(), expected))
}

/// Whether two vectors agree, within the drift of float order.
fn near(a: Vec3, b: Vec3) -> bool {
    (a - b).abs().max_element() <= 2e-4
}

fn check(motion: &ItemMotion, expected: &Expected, at: &str) -> Result<(), Failure> {
    let fields = [
        ("position", motion.position, expected.position),
        ("velocity", motion.velocity, expected.velocity),
        ("forward", motion.forward, expected.forward),
        ("up", motion.up, expected.up),
        (
            "angular velocity",
            motion.angular_velocity,
            expected.angular_velocity,
        ),
    ];
    for (name, ours, theirs) in fields {
        if !near(ours, theirs) {
            return Err(format!("{at}: the {name} is {ours}, the game's {theirs}").into());
        }
    }
    if motion.at_rest != expected.at_rest || motion.detonation_ticks != expected.detonation_ticks {
        return Err(format!(
            "{at}: at rest {} with {} ticks, the game's {} with {}",
            motion.at_rest, motion.detonation_ticks, expected.at_rest, expected.detonation_ticks
        )
        .into());
    }
    Ok(())
}

#[test]
fn every_tick_of_every_case_is_the_games() -> Result<(), Failure> {
    let mut case: Option<(String, Item, Plane, ItemMotion, RandomSeed)> = None;
    let (mut cases, mut ticks, mut impacts) = (0, 0, 0);
    for (number, line) in REFERENCE.lines().enumerate() {
        if line.starts_with('#') {
            continue;
        }
        let (words, expected) = split(line)?;
        let at = format!("line {}", number + 1);
        match words.first().copied() {
            Some("case") => {
                let [_, name, flags, x, y, z] = words[..] else {
                    return Err(format!("{at}: a short case").into());
                };
                let item = Item {
                    flags: flags.parse()?,
                    ..Item::default()
                };
                let plane = Plane(Vec3::new(x.parse()?, y.parse()?, z.parse()?));
                let mut motion =
                    ItemMotion::placed(expected.position, expected.forward, expected.up);
                motion.velocity = expected.velocity;
                motion.detonation_ticks = expected.detonation_ticks;
                motion.spin(expected.angular_velocity);
                case = Some((
                    name.to_owned(),
                    item,
                    plane,
                    motion,
                    RandomSeed(0xDEAD_BEEF),
                ));
                cases += 1;
            }
            Some("tick") => {
                let Some((name, item, plane, motion, seed)) = case.as_mut() else {
                    return Err(format!("{at}: a tick before a case").into());
                };
                let step = motion.update(item, plane, seed);
                let hit = words.get(1) == Some(&"impact");
                if step.impact.is_some() != hit {
                    return Err(
                        format!("{at} ({name}): an impact {}", step.impact.is_some()).into(),
                    );
                }
                if let Some(impact) = step.impact {
                    let scale: f32 = words.get(5).ok_or("no impact scale")?.parse()?;
                    let sound = numbers(words.get(7..10).ok_or("no sound")?)?;
                    if (impact.scale - scale).abs() > 2e-4
                        || !near(impact.sound_position, vector(&sound, 0)?)
                    {
                        return Err(format!("{at} ({name}): the impact differs").into());
                    }
                    impacts += 1;
                }
                if step.detonated != words.contains(&"detonated") {
                    return Err(format!("{at} ({name}): detonated {}", step.detonated).into());
                }
                check(motion, &expected, &format!("{at} ({name})"))?;
                ticks += 1;
            }
            Some("push") => {
                let Some((name, _, _, motion, seed)) = case.as_mut() else {
                    return Err(format!("{at}: a push before a case").into());
                };
                let push = vector(&numbers(&words[1..])?, 0)?;
                motion.accelerate(push, seed);
                check(motion, &expected, &format!("{at} ({name})"))?;
            }
            _ => return Err(format!("{at}: an unknown line").into()),
        }
    }
    assert_eq!(cases, 9);
    assert!(
        ticks > 800 && impacts > 20,
        "{ticks} ticks, {impacts} impacts"
    );
    Ok(())
}
