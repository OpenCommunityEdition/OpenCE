//! Damage, shields and death (`damage.c`): what a damage effect does to an
//! object's shield and body by the material it strikes, the stun that
//! holds its shield back and the recharge after it, headshots, and the
//! death that follows when its body is spent.
//!
//! Vitality is kept as the game keeps it, as fractions of the collision
//! model's maximums: 1 is whole, 0 is spent.

use std::collections::BTreeMap;

use bevy_ecs::prelude::*;
use glam::Vec3;
use opence_content::{
    AnimationNode, Biped, CollisionModel, CollisionModelMaterial, CollisionRegion, DamageEffect,
    FallingDamage, Level, ObjectKind, TagRef,
};

use crate::animation::{
    model_orientations, node_matrices, AnimationPlayer, NodeMatrices, NodeMatrix, Orientation,
};
use crate::clock;
use crate::hash::WorldHasher;
use crate::objects::{Object, ObjectHandle, ObjectTable, Team};
use crate::projectile::Impact;
use crate::random::RandomSeed;
use crate::Transform;

/// A damage effect's side effect that kills those who do not see it
/// coming.
pub const LETHAL_TO_THE_UNSUSPECTING: i16 = 2;
/// A damage effect's side effect that empties shields at once.
pub const EMP: i16 = 3;

/// One of the game's transition functions (`transition_function_evaluate`)
/// at `v`, clamped to 0 to 1: linear, early, very early, late, very late
/// and cosine; another is linear.
pub fn transition(function: i16, v: f32) -> f32 {
    let v = v.clamp(0.0, 1.0);
    match function {
        1 => libm::sqrtf(v),
        2 => libm::sqrtf(libm::sqrtf(v)),
        3 => v * v,
        4 => v * v * v * v,
        5 => (libm::sinf(v * std::f32::consts::PI - std::f32::consts::FRAC_PI_2) + 1.0) / 2.0,
        _ => v,
    }
}

/// An object's shield and body, and the damage to its regions.
#[derive(Component, Clone, Debug, PartialEq)]
pub struct Vitality {
    /// Its body, as a fraction of the model's maximum (below 0 once dead).
    pub body: f32,
    /// Its shield, as a fraction of the model's maximum.
    pub shield: f32,
    /// The ticks before its shield starts to recharge.
    pub stun_ticks: u32,
    /// Its shield has been emptied, and has not yet begun to recharge.
    pub shield_depleted: bool,
    /// It is dead.
    pub dead: bool,
    /// The damage to each of the model's regions, in body points.
    pub region_damage: Vec<f32>,
    /// Which regions have been destroyed.
    pub regions_destroyed: Vec<bool>,
}

impl Vitality {
    /// Whole: each of the model's maximums that is above 0 full.
    pub fn new(model: &CollisionModel) -> Vitality {
        let full = |maximum: f32| if maximum > 0.0 { 1.0 } else { 0.0 };
        Vitality {
            body: full(model.maximum_body_vitality),
            shield: full(model.maximum_shield_vitality),
            stun_ticks: 0,
            shield_depleted: false,
            dead: false,
            region_damage: vec![0.0; model.regions.len()],
            regions_destroyed: vec![false; model.regions.len()],
        }
    }

    /// One tick of recovery: a shield that is not full counts its stun
    /// down, then recharges over the model's recharge time.
    pub fn recover(&mut self, model: &CollisionModel) {
        if self.dead || model.maximum_shield_vitality <= 0.0 || self.shield >= 1.0 {
            return;
        }
        if self.stun_ticks > 0 {
            self.stun_ticks -= 1;
        } else if model.recharge_time > 0.0 {
            let rate = 1.0 / (model.recharge_time * clock::TICKS_PER_SECOND as f32);
            self.shield = (self.shield + rate).min(1.0);
            self.shield_depleted = false;
        }
    }

    pub(crate) fn hash(&self, hasher: &mut WorldHasher) {
        hasher.f32(self.body);
        hasher.f32(self.shield);
        hasher.u64(u64::from(self.stun_ticks));
        hasher.u64(u64::from(self.shield_depleted) | u64::from(self.dead) << 1);
        for (damage, destroyed) in self.region_damage.iter().zip(&self.regions_destroyed) {
            hasher.f32(*damage);
            hasher.u64(u64::from(*destroyed));
        }
    }
}

/// How a damage effect reaches what it hits.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct DamageHit {
    /// How much of its damage arrives, 0 to 1 (1 for a direct hit; a
    /// fall's is from its speed).
    pub scale: f32,
    /// Where in its damage's range it falls, 0 to 1 (drawn from the seed).
    pub random: f32,
    /// The model's material struck, if it is known.
    pub material: Option<u16>,
    /// The model's region struck, if it is known.
    pub region: Option<u16>,
    /// It comes from a friend.
    pub friendly: bool,
    /// It comes from where the victim does not see it.
    pub unsuspecting: bool,
    /// The game is multiplayer.
    pub multiplayer: bool,
    /// A headshot kills outright: always, but for a player's biped in the
    /// campaign.
    pub headshot_kills: bool,
}

impl Default for DamageHit {
    fn default() -> Self {
        DamageHit {
            scale: 1.0,
            random: 0.0,
            material: None,
            region: None,
            friendly: false,
            unsuspecting: false,
            multiplayer: false,
            headshot_kills: true,
        }
    }
}

/// What a damage effect did.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Outcome {
    /// The shield it took, as a fraction.
    pub shield: f32,
    /// The body it took, as a fraction.
    pub body: f32,
    /// It emptied the shield.
    pub shield_depleted: bool,
    /// It struck the head.
    pub headshot: bool,
    /// It killed.
    pub killed: bool,
}

/// The material a hit takes its multipliers from: the one struck, else the
/// model's material for damage that strikes no material, else none, which
/// takes no damage.
fn hit_material(model: &CollisionModel, material: Option<u16>) -> Option<&CollisionModelMaterial> {
    material
        .filter(|&material| usize::from(material) < model.materials.len())
        .or(model.indirect_damage_material)
        .and_then(|material| model.materials.get(usize::from(material)))
}

/// `effect` done to an object of `model` at `vitality` (`object_cause_damage`).
pub fn cause_damage(
    vitality: &mut Vitality,
    model: &CollisionModel,
    effect: &DamageEffect,
    hit: &DamageHit,
) -> Outcome {
    let mut outcome = Outcome::default();
    if vitality.dead {
        return outcome;
    }
    let flags = effect.damage_flags;
    let [lower, upper] = effect.damage;
    let ranged = lower + (upper - lower) * hit.random.clamp(0.0, 1.0);
    let scale = hit.scale.clamp(0.0, 1.0);
    let mut total = (1.0 - scale) * effect.damage_minimum + ranged * scale;
    let material = hit_material(model, hit.material);
    let modifier = |kind: i16| {
        usize::try_from(kind)
            .ok()
            .and_then(|kind| effect.material_modifiers.get(kind))
            .copied()
            .unwrap_or(1.0)
    };
    if effect.side_effect == LETHAL_TO_THE_UNSUSPECTING && hit.unsuspecting {
        outcome.body = vitality.body.max(0.0);
        vitality.body = 0.0;
        vitality.dead = true;
        outcome.killed = true;
        return outcome;
    }

    // (the shield first, leaking a share of the damage through, and less
    // of it the lower it is below its failure threshold)
    if flags & DamageEffect::SKIPS_SHIELDS == 0 && model.maximum_shield_vitality > 0.0 {
        let mut shield_damage = 0.0;
        if vitality.shield > 0.0 {
            let leak = material.map_or(0.0, |material| material.shield_leak_percentage);
            shield_damage = (1.0 - leak) * total;
            let threshold = model.shield_failure_threshold;
            if threshold > 0.0 && vitality.shield <= threshold {
                let failing = model.failing_shield_leak_fraction;
                shield_damage *= (1.0 - failing)
                    * transition(model.shield_failure_function, vitality.shield / threshold)
                    + failing;
            }
            shield_damage = shield_damage.max(0.0);
            total -= shield_damage;
            let multiplier = material.map_or(0.0, |material| material.shield_damage_multiplier);
            let actual = multiplier * shield_damage * modifier(model.shield_material_type);
            let normalized = actual / model.maximum_shield_vitality;
            if normalized > vitality.shield || effect.side_effect == EMP {
                // (what the shield could not hold goes on to the body)
                let excess = actual - model.maximum_shield_vitality * vitality.shield;
                if excess > 0.0 {
                    total += excess;
                }
                outcome.shield = vitality.shield;
                outcome.shield_depleted = true;
                vitality.shield = 0.0;
                vitality.shield_depleted = true;
            } else {
                outcome.shield = normalized;
                vitality.shield -= normalized;
            }
        }
        if total + shield_damage > 0.0
            && (shield_damage >= model.minimum_stun_damage || vitality.shield <= 0.0)
        {
            vitality.stun_ticks = (model.stun_time * clock::TICKS_PER_SECOND as f32) as u32;
        }
    }

    // (then the body)
    if flags & DamageEffect::ONLY_HURTS_SHIELDS == 0 && model.maximum_body_vitality > 0.0 {
        if model.flags & CollisionModel::ONLY_HURT_BY_EXPLOSIVES != 0
            && flags & DamageEffect::DETONATES_EXPLOSIVES == 0
        {
            total = 0.0;
        }
        let multiplier = material.map_or(0.0, |material| material.body_damage_multiplier);
        let mut amount = multiplier * total;
        if hit.friendly {
            amount *= 1.0 - model.friendly_damage_resistance;
        }
        let kind = material.map_or(-1, |material| material.material_type);
        let mut actual = amount / model.maximum_body_vitality * modifier(kind);
        let head =
            material.is_some_and(|material| material.flags & CollisionModelMaterial::HEAD != 0);
        if actual > 0.0 && head {
            if flags & DamageEffect::CAN_CAUSE_HEADSHOTS != 0 && hit.headshot_kills {
                outcome.headshot = true;
                actual = actual.max(vitality.body);
                outcome.killed = true;
            } else if flags & DamageEffect::CAN_CAUSE_MULTIPLAYER_HEADSHOTS != 0 && hit.multiplayer
            {
                outcome.headshot = true;
                actual *= 2.0;
            }
        }
        if actual > 0.0 {
            vitality.body -= actual;
            outcome.body = actual;
            if let Some(region) = hit.region.map(usize::from) {
                damage_region(vitality, model, region, actual, &mut outcome);
            }
            if vitality.body <= 0.0 {
                outcome.killed = true;
            }
        }
    }
    if outcome.killed {
        vitality.body = vitality.body.min(0.0);
        vitality.dead = true;
    }
    outcome
}

/// `actual` of the body's damage to `region`, which is destroyed once it
/// has taken its threshold (and with it the object, if it forces that).
fn damage_region(
    vitality: &mut Vitality,
    model: &CollisionModel,
    region: usize,
    actual: f32,
    outcome: &mut Outcome,
) {
    let (Some(definition), Some(damage), Some(destroyed)) = (
        model.regions.get(region),
        vitality.region_damage.get_mut(region),
        vitality.regions_destroyed.get_mut(region),
    ) else {
        return;
    };
    *damage += actual * model.maximum_body_vitality;
    if definition.damage_threshold > 0.0 && *damage >= definition.damage_threshold && !*destroyed {
        *destroyed = true;
        if definition.flags & CollisionRegion::FORCES_OBJECT_TO_DIE != 0 {
            outcome.killed = true;
        }
    }
}

/// What a damaged kind of object is: its collision model, and where its
/// nodes are at the default pose, from its origin, each with the
/// collision model's node of its name.
#[derive(Clone, Debug)]
pub struct Damageable {
    /// Its collision model.
    pub model: CollisionModel,
    /// Its pill: the radius and the height standing, if it is a biped.
    pub pill: Option<(f32, f32)>,
    /// It takes no damage from falls.
    pub immune_to_falling: bool,
    /// Its model's nodes: the name, where it is at the default pose (from
    /// the object's origin), and its collision node.
    pub nodes: Vec<(String, Vec3, Option<usize>)>,
}

/// The level's damage: each damaged kind of object, by its tag, and every
/// damage effect.
#[derive(Resource, Clone, Debug, Default)]
pub struct DamageTable {
    /// The objects that take damage, by their tags.
    pub objects: BTreeMap<TagRef, Damageable>,
    /// The damage effects, by their tags.
    pub effects: BTreeMap<TagRef, DamageEffect>,
    /// The game is multiplayer.
    pub multiplayer: bool,
    /// The damage of falls.
    pub falling: FallingDamage,
}

/// The collision model's node for a model's node: by its name, else the
/// one whose name ends the model's (`bip01 head` takes `head`).
fn collision_node(model: &CollisionModel, name: &str) -> Option<usize> {
    model
        .nodes
        .iter()
        .position(|node| node.name == name)
        .or_else(|| {
            model
                .nodes
                .iter()
                .position(|node| !node.name.is_empty() && name.ends_with(node.name.as_str()))
        })
}

impl DamageTable {
    /// `level`'s bipeds and their damage effects; `multiplayer`, if the
    /// game is.
    pub fn new(level: &Level, multiplayer: bool) -> DamageTable {
        let objects = level
            .objects
            .iter()
            .filter(|object| object.kind == ObjectKind::Biped)
            .filter_map(|object| {
                let model = level
                    .collision_model(object.collision_model.as_ref()?)?
                    .clone();
                let mut nodes = Vec::new();
                if let Some(render) = object
                    .model
                    .as_ref()
                    .and_then(|tag| level.model_index(tag))
                    .and_then(|index| level.models.get(index))
                {
                    let links: Vec<_> = render
                        .nodes
                        .iter()
                        .map(|node| AnimationNode {
                            name: node.name.clone(),
                            parent: node.parent,
                            first_child: node.first_child,
                            next_sibling: node.next_sibling,
                        })
                        .collect();
                    let mut orientations = vec![Orientation::REST; links.len()];
                    model_orientations(render, &mut orientations);
                    let matrices = node_matrices(&links, &orientations, &NodeMatrix::IDENTITY);
                    nodes = links
                        .iter()
                        .zip(&matrices)
                        .map(|(link, matrix)| {
                            (
                                link.name.clone(),
                                Vec3::from_array(matrix.position),
                                collision_node(&model, &link.name),
                            )
                        })
                        .collect();
                }
                let pill = object
                    .biped
                    .map(|biped| (biped.collision_radius, biped.collision_height_standing));
                let immune_to_falling = object
                    .biped
                    .is_some_and(|biped| biped.flags & Biped::IMMUNE_TO_FALLING_DAMAGE != 0);
                Some((
                    object.tag.clone(),
                    Damageable {
                        model,
                        pill,
                        immune_to_falling,
                        nodes,
                    },
                ))
            })
            .collect();
        let effects = level
            .damage_effects
            .iter()
            .map(|effect| (effect.tag.clone(), effect.clone()))
            .collect();
        DamageTable {
            objects,
            effects,
            multiplayer,
            falling: level.globals.falling.clone(),
        }
    }
}

/// Where an object was struck: its node nearest the point, and the
/// collision model's material and region there.
pub fn struck_part(
    damageable: &Damageable,
    transform: &Transform,
    nodes: Option<&NodeMatrices>,
    names: Option<&[AnimationNode]>,
    point: Vec3,
) -> (Option<u16>, Option<u16>) {
    // (the animated pose, if it has one; else the default pose)
    let nearest = match (nodes, names) {
        (Some(nodes), Some(names)) if !nodes.current.is_empty() => nodes
            .current
            .iter()
            .zip(names)
            .map(|(matrix, node)| {
                let distance = Vec3::from_array(matrix.position).distance_squared(point);
                (distance, collision_node(&damageable.model, &node.name))
            })
            .min_by(|a, b| a.0.total_cmp(&b.0))
            .and_then(|(_, node)| node),
        _ => damageable
            .nodes
            .iter()
            .map(|(_, position, node)| {
                let world = transform.position + transform.rotation * (*position * transform.scale);
                (world.distance_squared(point), *node)
            })
            .min_by(|a, b| a.0.total_cmp(&b.0))
            .and_then(|(_, node)| node),
    };
    let Some(node) = nearest else {
        return (None, None);
    };
    let region = damageable
        .model
        .nodes
        .get(node)
        .and_then(|node| node.region);
    (damageable.model.node_material(node), region)
}

/// Where a ray from `from` along `path` first enters a pill: a cylinder
/// of `radius` capped by half-spheres, its axis from `bottom` to `top`;
/// the fraction of the path, and the normal there.
pub fn ray_pill(
    from: Vec3,
    path: Vec3,
    bottom: Vec3,
    top: Vec3,
    radius: f32,
) -> Option<(f32, Vec3)> {
    let length = path.length();
    if length <= 0.0 {
        return None;
    }
    let direction = path / length;
    let axis = top - bottom;
    let sphere = |centre: Vec3| -> Option<f32> {
        let offset = from - centre;
        let b = direction.dot(offset);
        let c = offset.length_squared() - radius * radius;
        let h = b * b - c;
        (h >= 0.0).then(|| -b - libm::sqrtf(h))
    };
    let axis_squared = axis.length_squared();
    let along_direction = axis.dot(direction);
    let offset = from - bottom;
    let along_offset = axis.dot(offset);
    let a = axis_squared - along_direction * along_direction;
    let mut distance = None;
    if a > 1e-6 {
        let b = axis_squared * direction.dot(offset) - along_offset * along_direction;
        let c = axis_squared * offset.length_squared()
            - along_offset * along_offset
            - radius * radius * axis_squared;
        let h = b * b - a * c;
        if h >= 0.0 {
            let t = (-b - libm::sqrtf(h)) / a;
            let y = along_offset + t * along_direction;
            if y > 0.0 && y < axis_squared {
                distance = Some(t);
            }
        }
    }
    let distance = distance.or_else(|| {
        [sphere(bottom), sphere(top)]
            .into_iter()
            .flatten()
            .filter(|&t| t >= 0.0)
            .min_by(f32::total_cmp)
    })?;
    if !(0.0..=length).contains(&distance) {
        return None;
    }
    let point = from + direction * distance;
    let along = if axis_squared > 0.0 {
        ((point - bottom).dot(axis) / axis_squared).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let normal = (point - (bottom + axis * along)).normalize_or_zero();
    Some((distance / length, normal))
}

/// What damage did to an object.
#[derive(Clone, Debug, PartialEq)]
pub struct DamageEvent {
    /// The object damaged.
    pub object: ObjectHandle,
    /// The object that caused it, if one did.
    pub owner: Option<ObjectHandle>,
    /// The damage effect.
    pub effect: TagRef,
    /// What it did.
    pub outcome: Outcome,
}

/// An object that projectiles and melee strike: its pill, from `bottom` to
/// `top`, of `radius`.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Target {
    /// Its object.
    pub handle: ObjectHandle,
    /// Its pill's lower centre.
    pub bottom: Vec3,
    /// Its pill's upper centre.
    pub top: Vec3,
    /// Its pill's radius.
    pub radius: f32,
}

impl Target {
    /// Where a ray from `from` along `path` enters it, if it does: the
    /// fraction of the path and the normal.
    pub fn cast(&self, from: Vec3, path: Vec3) -> Option<(f32, Vec3)> {
        ray_pill(from, path, self.bottom, self.top, self.radius)
    }
}

/// The objects of the table with pills, but `except`, in the order of their
/// handles.
pub fn targets(world: &mut World, except: ObjectHandle) -> Vec<Target> {
    let Some(table) = world.get_resource::<DamageTable>() else {
        return Vec::new();
    };
    let pills: BTreeMap<TagRef, (f32, f32)> = table
        .objects
        .iter()
        .filter_map(|(tag, damageable)| Some((tag.clone(), damageable.pill?)))
        .collect();
    let mut targets: Vec<Target> = world
        .query::<(&ObjectHandle, &Object, &Transform)>()
        .iter(world)
        .filter(|(handle, ..)| **handle != except)
        .filter_map(|(handle, object, transform)| {
            let &(radius, height) = pills.get(&object.tag)?;
            let radius = radius * transform.scale;
            let height = (height * transform.scale).max(2.0 * radius);
            Some(Target {
                handle: *handle,
                bottom: transform.position + Vec3::Z * radius,
                top: transform.position + Vec3::Z * (height - radius),
                radius,
            })
        })
        .collect();
    targets.sort_by_key(|target| target.handle);
    targets
}

/// The first of `targets` a ray from `from` along `path` enters: it, the
/// fraction of the path and the normal.
pub fn first_target(targets: &[Target], from: Vec3, path: Vec3) -> Option<(Target, f32, Vec3)> {
    targets
        .iter()
        .filter_map(|target| {
            let (fraction, normal) = target.cast(from, path)?;
            Some((*target, fraction, normal))
        })
        .min_by(|a, b| a.1.total_cmp(&b.1))
}

/// Does each impact's damage to the object it struck; what each did.
/// `player` is the player's biped, whom headshots do not kill outright in
/// the campaign.
pub fn apply_impacts(
    world: &mut World,
    impacts: &[Impact],
    player: Option<ObjectHandle>,
) -> Vec<DamageEvent> {
    let mut events = Vec::new();
    for impact in impacts {
        let (Some(object), Some(effect)) = (impact.object, impact.damage.as_ref()) else {
            continue;
        };
        let owner = Some(impact.owner).filter(|owner| *owner != object);
        let hit = |world: &World, entity: Entity, damageable: &Damageable, random: f32| {
            let transform = world.get::<Transform>(entity).copied().unwrap_or_default();
            let names = world
                .get::<AnimationPlayer>(entity)
                .map(|player| player.nodes().to_vec());
            let (material, region) = struck_part(
                damageable,
                &transform,
                world.get::<NodeMatrices>(entity),
                names.as_deref(),
                impact.point,
            );
            let team = |handle: ObjectHandle| {
                let entity = world.resource::<ObjectTable>().entity(handle)?;
                world.get::<Team>(entity).map(|team| team.0)
            };
            let friendly = owner
                .and_then(team)
                .is_some_and(|owner| team(object) == Some(owner));
            // (from behind: the way it came is the way the victim faces)
            let facing = transform.rotation * Vec3::X;
            DamageHit {
                scale: 1.0,
                random,
                material,
                region,
                friendly,
                unsuspecting: facing.dot(impact.direction) > 0.0,
                multiplayer: false,
                headshot_kills: true,
            }
        };
        if let Some(outcome) = damage_object(
            world,
            object,
            effect,
            |world, entity, damageable, random, multiplayer| DamageHit {
                multiplayer,
                headshot_kills: multiplayer || Some(object) != player,
                ..hit(world, entity, damageable, random)
            },
        ) {
            events.push(DamageEvent {
                object,
                owner,
                effect: effect.clone(),
                outcome,
            });
        }
    }
    events
}

/// A fall's damage to `object`, landing at `speed` into the ground: none
/// below the globals' least, the scaled falling damage up to their most,
/// and above the fastest a biped falls, the damage of the greatest falls.
pub fn apply_fall(world: &mut World, object: ObjectHandle, speed: f32) -> Option<DamageEvent> {
    let table = world.get_resource::<DamageTable>()?;
    let falling = &table.falling;
    let (effect, scale) = if speed > falling.maximum_falling_velocity {
        (falling.maximum_distance_damage.clone()?, 1.0)
    } else if speed > falling.minimum_damage_velocity {
        let range = falling.maximum_damage_velocity - falling.minimum_damage_velocity;
        let scale = if range > 0.0 {
            (speed - falling.minimum_damage_velocity) / range
        } else {
            1.0
        };
        (falling.falling_damage.clone()?, scale.clamp(0.0, 1.0))
    } else {
        return None;
    };
    let entity = world.resource::<ObjectTable>().entity(object)?;
    let tag = &world.get::<Object>(entity)?.tag;
    if table.objects.get(tag)?.immune_to_falling {
        return None;
    }
    let outcome = damage_object(
        world,
        object,
        &effect,
        |_, _, damageable, random, multiplayer| DamageHit {
            scale,
            random,
            material: damageable.model.indirect_damage_material,
            multiplayer,
            ..DamageHit::default()
        },
    )?;
    Some(DamageEvent {
        object,
        owner: None,
        effect,
        outcome,
    })
}

/// `effect` done to `object`, struck as `hit` says (given the world, its
/// entity, what it is, a random draw and whether the game is multiplayer);
/// none if it is not damaged.
fn damage_object(
    world: &mut World,
    object: ObjectHandle,
    effect: &TagRef,
    hit: impl FnOnce(&World, Entity, &Damageable, f32, bool) -> DamageHit,
) -> Option<Outcome> {
    let entity = world.resource::<ObjectTable>().entity(object)?;
    let random = world.get_resource_or_init::<RandomSeed>().next_real();
    let table = world.get_resource::<DamageTable>()?;
    let tag = &world.get::<Object>(entity)?.tag;
    let damageable = table.objects.get(tag)?;
    let definition = table.effects.get(effect)?;
    let hit = hit(world, entity, damageable, random, table.multiplayer);
    let mut vitality = world
        .get::<Vitality>(entity)
        .cloned()
        .unwrap_or_else(|| Vitality::new(&damageable.model));
    let outcome = cause_damage(&mut vitality, &damageable.model, definition, &hit);
    world.entity_mut(entity).insert(vitality);
    Some(outcome)
}

/// Gives each newly made object the table knows its vitality.
pub(crate) fn give_vitality(
    mut commands: Commands,
    table: Option<Res<DamageTable>>,
    objects: Query<(Entity, &Object), Without<Vitality>>,
) {
    let Some(table) = table else { return };
    for (entity, object) in &objects {
        if let Some(damageable) = table.objects.get(&object.tag) {
            commands
                .entity(entity)
                .insert(Vitality::new(&damageable.model));
        }
    }
}

/// A tick of each object's recovery.
pub(crate) fn recover_vitality(
    table: Option<Res<DamageTable>>,
    mut objects: Query<(&Object, &mut Vitality)>,
) {
    let Some(table) = table else { return };
    for (object, mut vitality) in &mut objects {
        if let Some(damageable) = table.objects.get(&object.tag) {
            vitality.recover(&damageable.model);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{CollisionNode, Group};

    fn tag(path: &str) -> TagRef {
        TagRef {
            group: Group(*b"jpt!"),
            path: path.into(),
        }
    }

    fn material(name: &str, flags: u32) -> CollisionModelMaterial {
        CollisionModelMaterial {
            name: name.into(),
            flags,
            material_type: 1,
            shield_leak_percentage: 0.0,
            shield_damage_multiplier: 1.0,
            body_damage_multiplier: 1.0,
        }
    }

    /// A cyborg's: 75 shield over 75 body, stunned for 2 seconds, then
    /// recharging over 4.
    fn cyborg() -> CollisionModel {
        CollisionModel {
            tag: tag("cyborg"),
            flags: 0,
            indirect_damage_material: Some(1),
            friendly_damage_resistance: 0.5,
            body_damaged_threshold: 0.0,
            body_destroyed_threshold: 0.0,
            shield_material_type: 2,
            shield_failure_function: 0,
            shield_failure_threshold: 0.0,
            failing_shield_leak_fraction: 0.0,
            minimum_stun_damage: 1.0,
            stun_time: 2.0,
            recharge_time: 4.0,
            shield_damaged_threshold: 0.0,
            maximum_body_vitality: 75.0,
            maximum_shield_vitality: 75.0,
            materials: vec![
                material("head", CollisionModelMaterial::HEAD),
                material("body", 0),
            ],
            regions: vec![CollisionRegion {
                name: "head".into(),
                flags: CollisionRegion::FORCES_OBJECT_TO_DIE,
                damage_threshold: 50.0,
                permutations: Vec::new(),
            }],
            pathfinding_box: Default::default(),
            pathfinding_spheres: Vec::new(),
            nodes: vec![CollisionNode {
                name: "head".into(),
                region: Some(0),
                parent: None,
                next_sibling: None,
                first_child: None,
                bsps: Vec::new(),
            }],
        }
    }

    fn bullet(damage: f32, flags: u32) -> DamageEffect {
        DamageEffect {
            tag: tag("bullet"),
            radius: [0.0; 2],
            cutoff_scale: 0.0,
            flags: 0,
            side_effect: 0,
            category: 0,
            damage_flags: flags,
            area_of_effect_core_radius: 0.0,
            damage_minimum: damage,
            damage: [damage, damage],
            vehicle_passthrough_penalty: 0.0,
            active_camouflage_damage: 0.0,
            stun: 0.0,
            maximum_stun: 0.0,
            stun_time: 0.0,
            instantaneous_acceleration: 0.0,
            material_modifiers: vec![1.0; DamageEffect::MATERIAL_TYPES],
        }
    }

    fn at(material: u16) -> DamageHit {
        DamageHit {
            material: Some(material),
            ..DamageHit::default()
        }
    }

    #[test]
    fn the_transitions_are_the_games() {
        assert_eq!(transition(0, 0.25), 0.25);
        assert_eq!(transition(1, 0.25), 0.5);
        assert!((transition(2, 0.0625) - 0.5).abs() < 1e-6);
        assert_eq!(transition(3, 0.5), 0.25);
        assert_eq!(transition(4, 0.5), 0.0625);
        assert!((transition(5, 0.5) - 0.5).abs() < 1e-6);
        assert_eq!(transition(9, 2.0), 1.0);
    }

    #[test]
    fn the_shield_takes_damage_before_the_body_and_its_excess_carries_on() {
        let model = cyborg();
        let mut vitality = Vitality::new(&model);
        let outcome = cause_damage(&mut vitality, &model, &bullet(15.0, 0), &at(1));
        assert!((vitality.shield - 0.8).abs() < 1e-6, "{vitality:?}");
        assert_eq!(vitality.body, 1.0);
        assert!((outcome.shield - 0.2).abs() < 1e-6);
        // (60 more: 60 of shield is left, so 15 goes on to the body)
        let outcome = cause_damage(&mut vitality, &model, &bullet(75.0, 0), &at(1));
        assert!(outcome.shield_depleted && vitality.shield_depleted);
        assert_eq!(vitality.shield, 0.0);
        assert!((vitality.body - 0.8).abs() < 1e-5, "{vitality:?}");
        assert!(!vitality.dead);
        // (then the body alone, to death)
        let outcome = cause_damage(&mut vitality, &model, &bullet(70.0, 0), &at(1));
        assert!(outcome.killed && vitality.dead);
        assert!(vitality.body <= 0.0);
        // (the dead take no more)
        let after = cause_damage(&mut vitality, &model, &bullet(70.0, 0), &at(1));
        assert_eq!(after, Outcome::default());
    }

    #[test]
    fn a_shield_leaks_its_materials_share() {
        let mut model = cyborg();
        model.materials[1].shield_leak_percentage = 0.25;
        let mut vitality = Vitality::new(&model);
        cause_damage(&mut vitality, &model, &bullet(20.0, 0), &at(1));
        assert!((vitality.shield - (1.0 - 15.0 / 75.0)).abs() < 1e-6);
        assert!((vitality.body - (1.0 - 5.0 / 75.0)).abs() < 1e-6);
    }

    #[test]
    fn headshots_kill_once_the_shield_is_down() {
        let model = cyborg();
        let sniper = bullet(10.0, DamageEffect::CAN_CAUSE_HEADSHOTS);
        let mut vitality = Vitality::new(&model);
        // (the shield takes it all)
        let outcome = cause_damage(&mut vitality, &model, &sniper, &at(0));
        assert!(!outcome.headshot && !vitality.dead);
        vitality.shield = 0.0;
        let outcome = cause_damage(&mut vitality, &model, &sniper, &at(0));
        assert!(outcome.headshot && outcome.killed && vitality.dead);
        // (a player's biped in the campaign is not killed outright)
        let mut player = Vitality {
            shield: 0.0,
            ..Vitality::new(&model)
        };
        let hit = DamageHit {
            headshot_kills: false,
            ..at(0)
        };
        let outcome = cause_damage(&mut player, &model, &sniper, &hit);
        assert!(!outcome.killed && player.body > 0.8);
        // (in multiplayer, a pistol's doubles)
        let pistol = bullet(10.0, DamageEffect::CAN_CAUSE_MULTIPLAYER_HEADSHOTS);
        let mut target = Vitality {
            shield: 0.0,
            ..Vitality::new(&model)
        };
        let hit = DamageHit {
            multiplayer: true,
            ..at(0)
        };
        let outcome = cause_damage(&mut target, &model, &pistol, &hit);
        assert!(outcome.headshot && (outcome.body - 20.0 / 75.0).abs() < 1e-6);
    }

    #[test]
    fn friends_hurt_less_and_a_region_can_kill() {
        let model = cyborg();
        let mut vitality = Vitality {
            shield: 0.0,
            ..Vitality::new(&model)
        };
        let hit = DamageHit {
            friendly: true,
            ..at(1)
        };
        cause_damage(&mut vitality, &model, &bullet(30.0, 0), &hit);
        assert!((vitality.body - (1.0 - 15.0 / 75.0)).abs() < 1e-6);
        let hit = DamageHit {
            region: Some(0),
            ..at(1)
        };
        let outcome = cause_damage(&mut vitality, &model, &bullet(50.0, 0), &hit);
        assert!(vitality.regions_destroyed[0] && outcome.killed && vitality.dead);
    }

    #[test]
    fn the_shield_is_stunned_then_recharges() {
        let model = cyborg();
        let mut vitality = Vitality::new(&model);
        cause_damage(&mut vitality, &model, &bullet(75.0, 0), &at(1));
        assert_eq!(vitality.shield, 0.0);
        assert_eq!(vitality.stun_ticks, 120);
        for _ in 0..120 {
            vitality.recover(&model);
        }
        assert_eq!(vitality.shield, 0.0);
        for _ in 0..120 {
            vitality.recover(&model);
        }
        assert!((vitality.shield - 0.5).abs() < 1e-4, "{vitality:?}");
        assert!(!vitality.shield_depleted);
        for _ in 0..200 {
            vitality.recover(&model);
        }
        assert_eq!(vitality.shield, 1.0);
    }

    #[test]
    fn the_unsuspecting_die_to_what_is_lethal_to_them() {
        let model = cyborg();
        let mut melee = bullet(1.0, 0);
        melee.side_effect = LETHAL_TO_THE_UNSUSPECTING;
        let mut vitality = Vitality::new(&model);
        cause_damage(&mut vitality, &model, &melee, &at(1));
        assert!(!vitality.dead);
        let hit = DamageHit {
            unsuspecting: true,
            ..at(1)
        };
        assert!(cause_damage(&mut vitality, &model, &melee, &hit).killed);
    }

    #[test]
    fn without_a_material_nothing_is_hurt() {
        let mut model = cyborg();
        model.indirect_damage_material = None;
        let mut vitality = Vitality::new(&model);
        cause_damage(
            &mut vitality,
            &model,
            &bullet(50.0, 0),
            &DamageHit::default(),
        );
        assert_eq!((vitality.shield, vitality.body), (1.0, 1.0));
    }

    #[test]
    fn rays_enter_pills_on_their_sides_and_caps() {
        let (bottom, top) = (Vec3::new(5.0, 0.0, 0.2), Vec3::new(5.0, 0.0, 1.6));
        let side = ray_pill(Vec3::new(0.0, 0.0, 1.0), Vec3::X * 10.0, bottom, top, 0.2);
        let Some((fraction, normal)) = side else {
            panic!("missed the side");
        };
        assert!((fraction - 0.48).abs() < 1e-5, "{fraction}");
        assert!((normal + Vec3::X).length() < 1e-5);
        let cap = ray_pill(Vec3::new(5.0, 0.0, 5.0), Vec3::Z * -10.0, bottom, top, 0.2);
        let Some((fraction, normal)) = cap else {
            panic!("missed the cap");
        };
        assert!((fraction - 0.32).abs() < 1e-5, "{fraction}");
        assert!((normal - Vec3::Z).length() < 1e-5);
        assert_eq!(
            ray_pill(Vec3::new(0.0, 1.0, 1.0), Vec3::X * 10.0, bottom, top, 0.2),
            None
        );
        // (short of it)
        assert_eq!(
            ray_pill(Vec3::new(0.0, 0.0, 1.0), Vec3::X * 4.0, bottom, top, 0.2),
            None
        );
    }

    #[test]
    fn falls_hurt_past_their_least_speed_and_kill_past_the_fastest() {
        let mut simulation = crate::Simulation::empty();
        let world = &mut simulation.world;
        let unit = TagRef {
            group: Group(*b"bipd"),
            path: "cyborg".into(),
        };
        let mut fall = bullet(75.0, 0);
        fall.damage_minimum = 0.0;
        let table = |immune_to_falling| DamageTable {
            objects: BTreeMap::from([(
                unit.clone(),
                Damageable {
                    model: cyborg(),
                    pill: Some((0.2, 0.7)),
                    immune_to_falling,
                    nodes: Vec::new(),
                },
            )]),
            effects: BTreeMap::from([
                (tag("fall"), fall.clone()),
                (tag("distance"), bullet(1000.0, 0)),
            ]),
            multiplayer: true,
            falling: FallingDamage {
                minimum_damage_velocity: 5.0,
                maximum_damage_velocity: 10.0,
                maximum_falling_velocity: 15.0,
                falling_damage: Some(tag("fall")),
                maximum_distance_damage: Some(tag("distance")),
            },
        };
        world.insert_resource(table(false));
        let spawned = crate::objects::spawn(
            world,
            crate::objects::ObjectSpawn {
                kind: ObjectKind::Biped,
                tag: unit.clone(),
                transform: Transform::default(),
                name: None,
                permutation: None,
                team: None,
            },
        );
        let Ok(object) = spawned else {
            panic!("no room");
        };
        assert_eq!(apply_fall(world, object, 4.0), None);
        // (halfway from the least to the most: half the damage)
        let Some(event) = apply_fall(world, object, 7.5) else {
            panic!("no damage");
        };
        assert!((event.outcome.shield - 0.5).abs() < 1e-6, "{event:?}");
        assert_eq!(event.effect, tag("fall"));
        let Some(event) = apply_fall(world, object, 20.0) else {
            panic!("no damage");
        };
        assert!(event.outcome.killed && event.effect == tag("distance"));
        world.insert_resource(table(true));
        assert_eq!(apply_fall(world, object, 20.0), None);
    }
}
