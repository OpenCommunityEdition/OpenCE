//! The sounds a tick makes: the sound parts its effects fired, the sounds
//! its animations reached, its footsteps, each heard by the material
//! underfoot, and the items lying about striking things. They are queued
//! in the tick and played after it, so that the tick never takes the
//! audio's lock.

use opence_content::{Level, MaterialEffects, TagRef};
use opence_sim::animation::AnimationEvent;
use opence_sim::effects::FiredPart;
use opence_sim::item_physics::{Impact, ItemImpact};
use opence_sim::seats::VehicleSlip;
use opence_sim::Vec3;

use crate::particles::ParticleEvents;

/// How far from the camera, in world units, a footstep is looked for: the
/// ground under a foot further off is not cast for.
pub(crate) const FOOTSTEP_REACH: f32 = 10.0;

/// A sound to play at a point.
#[derive(Clone, Debug, PartialEq)]
pub(crate) struct Heard {
    /// The sound.
    pub tag: TagRef,
    /// Where it is (`None` unspatialized, to the player: a pickup's).
    pub position: Option<Vec3>,
    /// Its scale, 0 to 1, between its sound's zero and one modifiers (a
    /// slip's: how hard it slips).
    pub scale: f32,
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
                position: Some(part.position),
                scale: 0.0,
            }),
    );
    for event in events {
        if let Some(sound) = &event.sound {
            heard.push(Heard {
                tag: sound.clone(),
                position: Some(event.position),
                scale: 0.0,
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
                position: Some(point),
                scale: 0.0,
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

/// Gathers what the items lying about make when they strike something
/// (`item_physics_update`): each one's collision sound where it bounced to,
/// and its material effects' impact event on the material `material` finds
/// for the impact, the sound there and the effect into `stepped`.
pub(crate) fn gather_items(
    level: &Level,
    impacts: &[ItemImpact],
    mut material: impl FnMut(&Impact) -> i16,
    heard: &mut Vec<Heard>,
    stepped: &mut Vec<Stepped>,
) {
    for struck in impacts {
        let impact = &struck.impact;
        if let Some(made) = struck
            .material_effects
            .as_ref()
            .and_then(|tag| level.material_effects(tag))
            .and_then(|effects| effects.of(MaterialEffects::IMPACT, material(impact)))
        {
            if let Some(sound) = &made.sound {
                heard.push(Heard {
                    tag: sound.clone(),
                    position: Some(impact.point),
                    scale: 0.0,
                });
            }
            if let Some(effect) = &made.effect {
                stepped.push(Stepped {
                    effect: effect.clone(),
                    point: impact.point,
                    normal: impact.normal,
                });
            }
        }
        if let Some(sound) = &struck.collision_sound {
            heard.push(Heard {
                tag: sound.clone(),
                position: Some(impact.sound_position),
                scale: 0.0,
            });
        }
    }
}

/// Gathers what the particles made in a tick: their collision and death
/// sounds into `heard` and effects into `stepped`, and their material
/// effects' impact event on the material `material` finds at each impact
/// (by its point and normal).
pub(crate) fn gather_particles(
    level: &Level,
    events: &ParticleEvents,
    mut material: impl FnMut(Vec3, Vec3) -> i16,
    heard: &mut Vec<Heard>,
    stepped: &mut Vec<Stepped>,
) {
    for made in &events.made {
        if made.tag.group.0 == *b"snd!" {
            heard.push(Heard {
                tag: made.tag.clone(),
                position: Some(made.point),
                scale: 0.0,
            });
        } else {
            stepped.push(Stepped {
                effect: made.tag.clone(),
                point: made.point,
                normal: made.normal,
            });
        }
    }
    for impact in &events.impacts {
        let Some(made) = level
            .material_effects(&impact.material_effects)
            .and_then(|effects| {
                effects.of(
                    MaterialEffects::IMPACT,
                    material(impact.point, impact.normal),
                )
            })
        else {
            continue;
        };
        if let Some(sound) = &made.sound {
            heard.push(Heard {
                tag: sound.clone(),
                position: Some(impact.point),
                scale: 0.0,
            });
        }
        if let Some(effect) = &made.effect {
            stepped.push(Stepped {
                effect: effect.clone(),
                point: impact.point,
                normal: impact.normal,
            });
        }
    }
}

/// Gathers the sounds of the vehicles' slips (`material_effect_new`): each
/// slip's material effects' slip event, metallic or not, on the material it
/// slipped on, heard a centimetre off the ground and scaled by how hard it
/// slipped. (Their effects start with the simulation's.)
pub(crate) fn gather_slips(level: &Level, slips: &[VehicleSlip], heard: &mut Vec<Heard>) {
    for slipped in slips {
        let slip = &slipped.slip;
        let event = if slip.metallic {
            MaterialEffects::METALLIC_SLIP
        } else {
            MaterialEffects::VEHICLE_SLIP
        };
        let Some(sound) = level
            .material_effects(&slipped.material_effects)
            .and_then(|effects| effects.of(event, slip.material))
            .and_then(|made| made.sound.as_ref())
        else {
            continue;
        };
        heard.push(Heard {
            tag: sound.clone(),
            position: Some(slip.position + slip.normal * 0.01),
            scale: slip.scale,
        });
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
                    position: Some(Vec3::X),
                    scale: 0.0,
                },
                Heard {
                    tag: tag(b"snd!", "grunt"),
                    position: Some(Vec3::Z),
                    scale: 0.0,
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
                position: Some(floor),
                scale: 0.0,
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

    #[test]
    fn an_item_striking_the_ground_is_heard_and_seen_by_its_material() {
        let mut level = level();
        let mut impact = vec![MaterialEffect::default(); 3];
        impact[2] = MaterialEffect {
            effect: Some(tag(b"effe", "sparks")),
            sound: Some(tag(b"snd!", "clank")),
        };
        let mut events = vec![Vec::new(); MaterialEffects::IMPACT + 1];
        events[MaterialEffects::IMPACT] = impact;
        level.material_effects.push(MaterialEffects {
            tag: tag(b"foot", "weapon"),
            events,
        });
        let struck = |effects: Option<&str>, sound: Option<&str>| ItemImpact {
            impact: Impact {
                point: Vec3::new(1.0, 2.0, 0.0),
                normal: Vec3::Z,
                struck: opence_sim::item_physics::Struck::Level,
                scale: 0.5,
                sound_position: Vec3::new(1.0, 2.0, 0.05),
            },
            material_effects: effects.map(|path| tag(b"foot", path)),
            collision_sound: sound.map(|path| tag(b"snd!", path)),
        };
        let (mut heard, mut stepped) = (Vec::new(), Vec::new());
        gather_items(
            &level,
            &[struck(Some("weapon"), Some("thud"))],
            |_| 2,
            &mut heard,
            &mut stepped,
        );
        assert_eq!(
            heard,
            [
                Heard {
                    tag: tag(b"snd!", "clank"),
                    position: Some(Vec3::new(1.0, 2.0, 0.0)),
                    scale: 0.0,
                },
                Heard {
                    tag: tag(b"snd!", "thud"),
                    position: Some(Vec3::new(1.0, 2.0, 0.05)),
                    scale: 0.0,
                },
            ]
        );
        assert_eq!(
            stepped,
            [Stepped {
                effect: tag(b"effe", "sparks"),
                point: Vec3::new(1.0, 2.0, 0.0),
                normal: Vec3::Z,
            }]
        );

        // (a material it makes nothing on, none, or no material effects:
        // only its collision sound)
        heard.clear();
        stepped.clear();
        gather_items(
            &level,
            &[
                struck(Some("weapon"), None),
                struck(Some("missing"), None),
                struck(None, Some("thud")),
            ],
            |_| -1,
            &mut heard,
            &mut stepped,
        );
        assert_eq!(heard.len(), 1);
        assert!(stepped.is_empty());
    }

    #[test]
    fn a_particles_sounds_are_heard_its_effects_started_and_its_impacts_seen() {
        use crate::particles::{Made, ParticleImpact};
        let mut level = level();
        let mut impact = vec![MaterialEffect::default(); 3];
        impact[2] = MaterialEffect {
            effect: Some(tag(b"effe", "chips")),
            sound: Some(tag(b"snd!", "tick")),
        };
        let mut events = vec![Vec::new(); MaterialEffects::IMPACT + 1];
        events[MaterialEffects::IMPACT] = impact;
        level.material_effects.push(MaterialEffects {
            tag: tag(b"foot", "shell"),
            events,
        });
        let at = Vec3::new(1.0, 0.0, 0.0);
        let made = |group: &[u8; 4], path: &str| Made {
            tag: tag(group, path),
            point: at,
            normal: Vec3::Z,
            scale: 1.0,
        };
        let impact = |effects: &str| ParticleImpact {
            material_effects: tag(b"foot", effects),
            point: at,
            normal: Vec3::Z,
            scale: 1.0,
        };
        let particles = ParticleEvents {
            made: vec![made(b"snd!", "ping"), made(b"effe", "puff")],
            impacts: vec![impact("shell"), impact("missing")],
        };
        let (mut heard, mut stepped) = (Vec::new(), Vec::new());
        let mut asked = Vec::new();
        gather_particles(
            &level,
            &particles,
            |point, normal| {
                asked.push((point, normal));
                2
            },
            &mut heard,
            &mut stepped,
        );
        assert_eq!(asked, [(at, Vec3::Z)]);
        let heard: Vec<&str> = heard.iter().map(|one| one.tag.path.as_str()).collect();
        assert_eq!(heard, ["ping", "tick"]);
        let stepped: Vec<&str> = stepped.iter().map(|one| one.effect.path.as_str()).collect();
        assert_eq!(stepped, ["puff", "chips"]);
    }

    #[test]
    fn a_slip_is_heard_by_its_material_and_scaled_by_how_hard_it_slips() {
        let mut level = level();
        let mut slip = vec![MaterialEffect::default(); 3];
        slip[2].sound = Some(tag(b"snd!", "skid"));
        let mut events = vec![Vec::new(); MaterialEffects::METALLIC_SLIP + 1];
        events[MaterialEffects::VEHICLE_SLIP] = slip;
        level.material_effects.push(MaterialEffects {
            tag: tag(b"foot", "wheels"),
            events,
        });
        let slipped = |material, metallic| VehicleSlip {
            vehicle: ObjectHandle { index: 2, salt: 1 },
            material_effects: tag(b"foot", "wheels"),
            slip: opence_sim::vehicle::Slip {
                position: Vec3::new(1.0, 2.0, 0.0),
                normal: Vec3::new(0.0, 0.0, 2.0),
                material,
                metallic,
                scale: 0.5,
            },
        };
        let mut heard = Vec::new();
        gather_slips(
            &level,
            &[slipped(2, false), slipped(1, false), slipped(2, true)],
            &mut heard,
        );
        assert_eq!(
            heard,
            [Heard {
                tag: tag(b"snd!", "skid"),
                position: Some(Vec3::new(1.0, 2.0, 0.02)),
                scale: 0.5,
            }]
        );
    }
}
