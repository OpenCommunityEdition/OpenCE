//! The sounds a tick makes: the sound parts its effects fired, the sounds
//! its animations reached, and its footsteps, each heard by the material
//! underfoot. They are queued in the tick and played after it, so that the
//! tick never takes the audio's lock.

use opence_content::{Level, MaterialEffects, TagRef};
use opence_sim::animation::AnimationEvent;
use opence_sim::effects::FiredPart;
use opence_sim::Vec3;

/// How far from the camera, in world units, a footstep is looked for: the
/// ground under a foot further off is not cast for.
pub(crate) const FOOTSTEP_REACH: f32 = 10.0;

/// A sound to play at a point.
#[derive(Clone, Debug, PartialEq)]
pub(crate) struct Heard {
    /// The sound.
    pub tag: TagRef,
    /// Where it is.
    pub position: Vec3,
}

/// An effect a footstep makes on the ground (a splash, a puff of dust).
#[derive(Clone, Debug, PartialEq)]
pub(crate) struct Stepped {
    /// The effect.
    pub effect: TagRef,
    /// The ground where the foot came down.
    pub point: Vec3,
    /// The ground's normal there.
    pub normal: Vec3,
}

/// Gathers the sounds of a tick into `heard` and its footsteps' effects
/// into `stepped`: the `snd!` parts in `fired`, the sounds `events` reached,
/// and the steps they took within [`FOOTSTEP_REACH`] of `camera`, on the
/// point, normal and material `ground` finds under them.
pub(crate) fn gather(
    level: &Level,
    fired: &[FiredPart],
    events: &[AnimationEvent],
    camera: Vec3,
    mut ground: impl FnMut(Vec3) -> Option<(Vec3, Vec3, i16)>,
    heard: &mut Vec<Heard>,
    stepped: &mut Vec<Stepped>,
) {
    heard.extend(
        fired
            .iter()
            .filter(|part| part.tag.group.0 == *b"snd!")
            .map(|part| Heard {
                tag: part.tag.clone(),
                position: part.position,
            }),
    );
    for event in events {
        if let Some(sound) = &event.sound {
            heard.push(Heard {
                tag: sound.clone(),
                position: event.position,
            });
        }
        if event.foot.is_none()
            || event.position.distance_squared(camera) > FOOTSTEP_REACH * FOOTSTEP_REACH
        {
            continue;
        }
        let Some(effects) = level
            .object(&event.tag)
            .and_then(|definition| definition.material_effects.as_ref())
            .and_then(|tag| level.material_effects(tag))
        else {
            continue;
        };
        let Some((point, normal, material)) = ground(event.position) else {
            continue;
        };
        let Some(step) = effects.of(MaterialEffects::RUN, material) else {
            continue;
        };
        if let Some(sound) = &step.sound {
            heard.push(Heard {
                tag: sound.clone(),
                position: point,
            });
        }
        if let Some(effect) = &step.effect {
            stepped.push(Stepped {
                effect: effect.clone(),
                point,
                normal,
            });
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Group, MaterialEffect, ObjectDefinition, ObjectKind};
    use opence_sim::animation::Foot;
    use opence_sim::effects::Surface;
    use opence_sim::objects::ObjectHandle;
    use opence_sim::Quat;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    /// A level whose biped `marine` steps with `steps`: on material 2 a
    /// run is heard as `gravel` and kicks up `dust`.
    fn level() -> Level {
        let mut level = crate::render_level::tests::level();
        let mut marine = crate::debug_scene::tests::test_object(ObjectKind::Biped, "marine");
        marine.material_effects = Some(tag(b"foot", "steps"));
        let mut walker = crate::debug_scene::tests::test_object(ObjectKind::Biped, "walker");
        walker.material_effects = Some(tag(b"foot", "missing"));
        level.objects = vec![marine, walker];
        let mut run = vec![MaterialEffect::default(); 3];
        run[2] = MaterialEffect {
            effect: Some(tag(b"effe", "dust")),
            sound: Some(tag(b"snd!", "gravel")),
        };
        level.material_effects = vec![MaterialEffects {
            tag: tag(b"foot", "steps"),
            events: vec![Vec::new(), run],
        }];
        level
    }

    fn part(group: &[u8; 4], path: &str, position: Vec3) -> FiredPart {
        FiredPart {
            effect: 1,
            event: 0,
            part: 0,
            tag: tag(group, path),
            position,
            rotation: Quat::IDENTITY,
            velocity: Vec3::ZERO,
            on: None,
            surface: None::<Surface>,
        }
    }

    fn event(
        object: &str,
        position: Vec3,
        sound: Option<&str>,
        foot: Option<Foot>,
    ) -> AnimationEvent {
        AnimationEvent {
            object: ObjectHandle { index: 1, salt: 1 },
            tag: tag(b"bipd", object),
            position,
            sound: sound.map(|path| tag(b"snd!", path)),
            foot,
        }
    }

    #[test]
    fn effects_sound_parts_and_animation_sounds_are_heard() {
        let level = level();
        let (mut heard, mut stepped) = (Vec::new(), Vec::new());
        gather(
            &level,
            &[
                part(b"snd!", "bang", Vec3::X),
                part(b"ligh", "flash", Vec3::Y),
            ],
            &[event("marine", Vec3::Z, Some("grunt"), None)],
            Vec3::ZERO,
            |_| None,
            &mut heard,
            &mut stepped,
        );
        assert_eq!(
            heard,
            [
                Heard {
                    tag: tag(b"snd!", "bang"),
                    position: Vec3::X,
                },
                Heard {
                    tag: tag(b"snd!", "grunt"),
                    position: Vec3::Z,
                },
            ]
        );
        assert!(stepped.is_empty());
    }

    #[test]
    fn a_step_is_heard_by_the_material_underfoot() {
        let level = level();
        let (mut heard, mut stepped) = (Vec::new(), Vec::new());
        let mut cast = Vec::new();
        let at = Vec3::new(3.0, 0.0, 1.0);
        let floor = Vec3::new(3.0, 0.0, 0.0);
        gather(
            &level,
            &[],
            &[
                event("marine", at, None, Some(Foot::Left)),
                // (too far to cast for, with no material effects, with
                // missing ones, and a foot on a material with none)
                event("marine", Vec3::new(20.0, 0.0, 0.0), None, Some(Foot::Right)),
                event("grunt", at, None, Some(Foot::Right)),
                event("walker", at, None, Some(Foot::Right)),
            ],
            Vec3::ZERO,
            |point| {
                cast.push(point);
                Some((floor, Vec3::Z, 2))
            },
            &mut heard,
            &mut stepped,
        );
        assert_eq!(cast, [at]);
        assert_eq!(
            heard,
            [Heard {
                tag: tag(b"snd!", "gravel"),
                position: floor,
            }]
        );
        assert_eq!(
            stepped,
            [Stepped {
                effect: tag(b"effe", "dust"),
                point: floor,
                normal: Vec3::Z,
            }]
        );

        heard.clear();
        stepped.clear();
        gather(
            &level,
            &[],
            &[event("marine", at, None, Some(Foot::Left))],
            Vec3::ZERO,
            |_| Some((floor, Vec3::Z, 1)),
            &mut heard,
            &mut stepped,
        );
        gather(
            &level,
            &[],
            &[event("marine", at, None, Some(Foot::Left))],
            Vec3::ZERO,
            |_| None,
            &mut heard,
            &mut stepped,
        );
        assert!(heard.is_empty() && stepped.is_empty());
    }

    #[test]
    fn a_definition_without_material_effects_is_not_cast_for() {
        let mut level = level();
        level.objects[0] = ObjectDefinition {
            material_effects: None,
            ..level.objects[0].clone()
        };
        let (mut heard, mut stepped) = (Vec::new(), Vec::new());
        let mut casts = 0;
        gather(
            &level,
            &[],
            &[event("marine", Vec3::ZERO, None, Some(Foot::Left))],
            Vec3::ZERO,
            |_| {
                casts += 1;
                Some((Vec3::ZERO, Vec3::Z, 2))
            },
            &mut heard,
            &mut stepped,
        );
        assert_eq!(casts, 0);
        assert!(heard.is_empty());
    }
}
