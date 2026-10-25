//! What fights: weapons, the projectiles they fire, equipment (grenades and
//! powerups), damage effects and the globals' grenades. Speeds are in world
//! units a second, angles in radians, times in seconds and rates a second.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// An item's part of its definition (weapons and equipment), after the
/// object's.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Item {
    /// Its flags ([`Item::ALWAYS_MAINTAINS_Z_UP`] and the rest).
    pub flags: u32,
    /// Its message on the HUD, when it is near.
    pub hud_message_index: i16,
    /// Where it sorts in the inventory.
    pub sort_order: i16,
    /// What it makes on each material it strikes (a `foot` tag, its
    /// [`crate::MaterialEffects::IMPACT`] event).
    pub material_effects: Option<TagRef>,
    /// The sound it makes when it strikes something.
    pub collision_sound: Option<TagRef>,
    /// The shortest and longest it waits before it detonates, once set off.
    pub detonation_delay: [f32; 2],
    /// The effect while it waits to detonate.
    pub detonating_effect: Option<TagRef>,
    /// The effect where it detonates.
    pub detonation_effect: Option<TagRef>,
}

impl Item {
    /// It stays upright, lying about.
    pub const ALWAYS_MAINTAINS_Z_UP: u32 = 1 << 0;
    /// Explosions destroy it.
    pub const DESTROYED_BY_EXPLOSIONS: u32 = 1 << 1;
    /// Gravity does not move it.
    pub const UNAFFECTED_BY_GRAVITY: u32 = 1 << 2;
}

/// A weapon's own part of its definition.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Weapon {
    /// Its item part.
    pub item: Item,
    /// Its flags ([`Weapon::MUST_BE_READIED`] and the rest).
    pub flags: u32,
    /// The weapon type its holder's animation graph plays it by (`pistol`,
    /// `rifle`; empty for the graph's first).
    pub label: String,
    /// What its secondary trigger does: 0 normal, 1 slaved to the primary,
    /// 2 inhibits the primary, 3 loads alternate ammunition, 4 loads
    /// multiple primary ammunition.
    pub secondary_trigger_mode: i16,
    /// The most alternate shots it loads, in the fourth mode.
    pub maximum_alternate_shots_loaded: i16,
    /// How long it takes to ready.
    pub ready_time: f32,
    /// The effect or sound of its readying.
    pub ready_effect: Option<TagRef>,
    /// The heat below which an overheated weapon fires again.
    pub heat_recovery_threshold: f32,
    /// The heat at which it overheats.
    pub heat_overheated_threshold: f32,
    /// The heat at which it detonates.
    pub heat_detonation_threshold: f32,
    /// The heat it loses a second.
    pub heat_loss_per_second: f32,
    /// The damage its melee does.
    pub melee_damage: Option<TagRef>,
    /// The damage its melee does to the one it is aimed at.
    pub melee_response: Option<TagRef>,
    /// How many zoom levels it has.
    pub zoom_levels: i16,
    /// Its least and most zoom.
    pub zoom_magnification: [f32; 2],
    /// How it helps its holder aim.
    pub aim_assist: AimAssist,
    /// How its holder's movement slows: 0 always, 1 when zoomed, 2 when
    /// zoomed or reloading.
    pub movement_penalty_mode: i16,
    /// Its holder's forward speed, as a fraction lost.
    pub forward_movement_penalty: f32,
    /// Its holder's sideways speed, as a fraction lost.
    pub sideways_movement_penalty: f32,
    /// How much its age adds to its rate of fire's wait.
    pub age_rate_of_fire_penalty: f32,
    /// Its first-person model.
    pub first_person_model: Option<TagRef>,
    /// Its first-person animations.
    pub first_person_animations: Option<TagRef>,
    /// Its HUD.
    pub hud_interface: Option<TagRef>,
    /// The sound of its ammunition being picked up.
    pub pickup_sound: Option<TagRef>,
    /// What it is: 0 undefined, 1 shotgun, 2 needler, 3 plasma pistol,
    /// 4 plasma rifle.
    pub weapon_type: i16,
    /// Its magazines.
    pub magazines: Vec<Magazine>,
    /// Its triggers, the primary first.
    pub triggers: Vec<Trigger>,
}

impl Weapon {
    /// Its heat is shown upright on the HUD.
    pub const VERTICAL_HEAT_DISPLAY: u32 = 1 << 0;
    /// Its triggers can't be pulled together.
    pub const MUTUALLY_EXCLUSIVE_TRIGGERS: u32 = 1 << 1;
    /// It must be readied before it fires.
    pub const MUST_BE_READIED: u32 = 1 << 3;
    /// It is not counted towards the weapons a unit carries.
    pub const DOESNT_COUNT_TOWARD_MAXIMUM: u32 = 1 << 4;
    /// Its aim assist works only zoomed.
    pub const AIM_ASSISTS_ONLY_WHEN_ZOOMED: u32 = 1 << 5;
    /// Its holder can't throw grenades.
    pub const PREVENTS_GRENADE_THROWING: u32 = 1 << 6;
    /// It is picked up only by the action key (the flag and the ball).
    pub const MUST_BE_PICKED_UP: u32 = 1 << 7;
    /// Its holder can't melee.
    pub const PREVENTS_MELEE_ATTACK: u32 = 1 << 9;
    /// It detonates when it is dropped.
    pub const DETONATES_WHEN_DROPPED: u32 = 1 << 10;
    /// It can't fire once it is as old as it gets.
    pub const CANNOT_FIRE_AT_MAXIMUM_AGE: u32 = 1 << 11;
    /// Its secondary trigger is pulled by the grenade button.
    pub const SECONDARY_TRIGGER_OVERRIDES_GRENADES: u32 = 1 << 12;
    /// Firing it does not lessen active camouflage.
    pub const DOES_NOT_DEPOWER_ACTIVE_CAMO: u32 = 1 << 13;
}

/// How a weapon helps its holder aim.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct AimAssist {
    /// The angle within which a target turns the reticle red.
    pub autoaim_angle: f32,
    /// The farthest a target does.
    pub autoaim_distance: f32,
    /// The angle within which the aim is drawn to a target.
    pub magnetism_angle: f32,
    /// The farthest a target draws it.
    pub magnetism_distance: f32,
    /// The angle shots bend towards a target.
    pub deviation_angle: f32,
}

/// A weapon's magazine.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Magazine {
    /// Its flags ([`Magazine::WASTES_ROUNDS_WHEN_RELOADED`] and the rest).
    pub flags: u32,
    /// Rounds it gains a second by itself (the plasma weapons' battery).
    pub rounds_recharged_per_second: i16,
    /// Rounds a new weapon has, in all.
    pub rounds_total_initial: i16,
    /// The most rounds it holds, in all.
    pub rounds_total_maximum: i16,
    /// The most rounds loaded.
    pub rounds_loaded_maximum: i16,
    /// How long a reload takes.
    pub reload_time: f32,
    /// Rounds a reload loads.
    pub rounds_reloaded: i16,
    /// How long chambering a round takes.
    pub chamber_time: f32,
    /// The effect or sound of a reload (each round of a one-at-a-time
    /// reload).
    pub reloading_effect: Option<TagRef>,
    /// The effect or sound of chambering a round.
    pub chambering_effect: Option<TagRef>,
    /// The ammunition picked up for it.
    pub ammunition: Vec<Ammunition>,
}

impl Magazine {
    /// A reload throws away the rounds still loaded.
    pub const WASTES_ROUNDS_WHEN_RELOADED: u32 = 1 << 0;
    /// Every shot must be chambered.
    pub const MUST_BE_CHAMBERED_EVERY_SHOT: u32 = 1 << 1;
}

/// Ammunition a magazine is filled from.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Ammunition {
    /// Rounds it gives.
    pub rounds: i16,
    /// The equipment it is.
    pub equipment: Option<TagRef>,
}

/// A weapon's trigger: what it fires, how fast and how wide.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Trigger {
    /// Its flags ([`Trigger::TRACKS_PROJECTILE`] and the rest).
    pub flags: u32,
    /// Shots a second when it is first pulled.
    pub initial_rate_of_fire: f32,
    /// Shots a second once it has been held.
    pub final_rate_of_fire: f32,
    /// How long it takes to reach the final rate.
    pub rate_of_fire_acceleration_time: f32,
    /// How long it takes to fall back to the initial rate.
    pub rate_of_fire_deceleration_time: f32,
    /// The rate at which its effects blur.
    pub blurred_rate_of_fire: f32,
    /// Its magazine, by its index in [`Weapon::magazines`]; none fires
    /// without ammunition.
    pub magazine: Option<u16>,
    /// Rounds a shot uses.
    pub rounds_per_shot: i16,
    /// The fewest rounds loaded that it fires with.
    pub minimum_rounds_loaded: i16,
    /// The rounds between tracers.
    pub rounds_between_tracers: i16,
    /// The spread when it is first pulled.
    pub initial_error: f32,
    /// The spread once it has been held.
    pub final_error: f32,
    /// How long the spread takes to reach the final.
    pub error_acceleration_time: f32,
    /// How long it takes to fall back.
    pub error_deceleration_time: f32,
    /// How long it charges.
    pub charging_time: f32,
    /// How long it holds a charge.
    pub charged_time: f32,
    /// What an overcharge does: 0 none, 1 explode, 2 discharge.
    pub overcharged_action: i16,
    /// The illumination a charge gives.
    pub charged_illumination: f32,
    /// How long it spews.
    pub spew_time: f32,
    /// How its projectiles are spread: 0 point, 1 horizontal fan.
    pub projectile_distribution_function: i16,
    /// Projectiles a shot fires.
    pub projectiles_per_shot: i16,
    /// The fan's angle.
    pub projectile_distribution_angle: f32,
    /// The cone within which a projectile's error is less.
    pub projectile_error_inner_cone_angle: f32,
    /// The least and most a projectile is turned from the aim.
    pub projectile_error_angle: [f32; 2],
    /// Where projectiles leave from, from the first-person view.
    pub first_person_offset: [f32; 3],
    /// What it fires.
    pub projectile: Option<TagRef>,
    /// The heat a round adds.
    pub heat_generated_per_round: f32,
    /// The age a round adds.
    pub age_generated_per_round: f32,
    /// How long it overloads.
    pub overloading_time: f32,
    /// The effect a shot makes (the first firing effect's), whose debris
    /// (spent casings) is thrown out with each.
    pub firing_effect: Option<TagRef>,
}

impl Trigger {
    /// Its holder tracks the projectile it fires.
    pub const TRACKS_PROJECTILE: u32 = 1 << 0;
    /// Its firing effects are chosen at random.
    pub const RANDOM_FIRING_EFFECTS: u32 = 1 << 1;
    /// It fires with fewer rounds loaded than a shot uses.
    pub const CAN_FIRE_WITH_PARTIAL_AMMUNITION: u32 = 1 << 2;
    /// It fires once a pull.
    pub const LATCHED: u32 = 1 << 3;
    /// A pull turns it on, the next off.
    pub const TOGGLES: u32 = 1 << 4;
    /// Its projectiles leave from the weapon, not the view.
    pub const USES_WEAPON_ORIGIN: u32 = 1 << 5;
    /// It stays pulled when the weapon is dropped.
    pub const STICKS_WHEN_DROPPED: u32 = 1 << 6;
    /// A discharge spews.
    pub const DISCHARGING_SPEWS: u32 = 1 << 8;
    /// Its rate of fire follows how far it is pulled.
    pub const ANALOG_RATE_OF_FIRE: u32 = 1 << 9;
    /// Its error applies only unzoomed.
    pub const USE_ERROR_WHEN_UNZOOMED: u32 = 1 << 10;
    /// Its projectiles go where the weapon points, not where its holder
    /// aims.
    pub const PROJECTILES_CANNOT_BE_AIMED: u32 = 1 << 11;
    /// A shot's projectiles share their error.
    pub const PROJECTILES_HAVE_IDENTICAL_ERROR: u32 = 1 << 12;
}

/// A projectile's own part of its definition.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Projectile {
    /// Its flags ([`Projectile::ORIENTED_ALONG_VELOCITY`] and the rest).
    pub flags: u32,
    /// When its detonation timer starts: 0 immediately, 1 after its first
    /// bounce, 2 when at rest.
    pub detonation_timer_starts: i16,
    /// The projectile that a cluster of these sets off.
    pub super_detonation: Option<TagRef>,
    /// Its radius, for collisions.
    pub collision_radius: f32,
    /// How long it takes to arm.
    pub arming_time: f32,
    /// The radius within which the AI flee it.
    pub danger_radius: f32,
    /// Its effect in flight.
    pub effect: Option<TagRef>,
    /// The shortest and longest its timer runs.
    pub timer: [f32; 2],
    /// It detonates slower than this.
    pub minimum_velocity: f32,
    /// It detonates past this distance.
    pub maximum_range: f32,
    /// Its gravity in the air, as a scale.
    pub air_gravity_scale: f32,
    /// The range over which its damage falls off in air.
    pub air_damage_range: [f32; 2],
    /// Its gravity in water, as a scale.
    pub water_gravity_scale: f32,
    /// The range over which its damage falls off in water.
    pub water_damage_range: [f32; 2],
    /// Its speed when fired.
    pub initial_velocity: f32,
    /// Its speed at the end of its range.
    pub final_velocity: f32,
    /// How fast it turns towards its target.
    pub guided_angular_velocity: f32,
    /// The effect as it starts to detonate.
    pub detonation_started: Option<TagRef>,
    /// The damage it does when it detonates stuck to something.
    pub attached_detonation_damage: Option<TagRef>,
    /// The damage it does where it hits.
    pub impact_damage: Option<TagRef>,
    /// What it does on each kind of material.
    pub material_responses: Vec<MaterialResponse>,
}

impl Projectile {
    /// It turns to face where it goes.
    pub const ORIENTED_ALONG_VELOCITY: u32 = 1 << 0;
    /// The AI aim it as it falls.
    pub const AIM_BALLISTIC: u32 = 1 << 1;
    /// Stuck to something, it waits its longest to detonate.
    pub const DETONATION_MAX_TIME_IF_ATTACHED: u32 = 1 << 2;
    /// Several together detonate as its super detonation.
    pub const SUPER_COMBINING_EXPLOSION: u32 = 1 << 3;
    /// Its holder's velocity is added to its own.
    pub const COMBINE_INITIAL_VELOCITY_WITH_PARENT: u32 = 1 << 4;
    /// Stuck to something, it waits a random time to detonate.
    pub const RANDOM_DETONATION_TIME_WHEN_ATTACHED: u32 = 1 << 5;
}

/// What a projectile does on a kind of material.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct MaterialResponse {
    /// Its flags.
    pub flags: u16,
    /// What it does: 0 disappear, 1 detonate, 2 reflect, 3 overpenetrate,
    /// 4 attach.
    pub default_response: i16,
    /// The effect then.
    pub default_effect: Option<TagRef>,
    /// What it may do instead, within the bounds below.
    pub potential_response: i16,
    /// Its flags ([`MaterialResponse::ONLY_AGAINST_UNITS`]).
    pub potential_flags: u16,
    /// The chance it does not.
    pub potential_skip_fraction: f32,
    /// The angles of impact between which it may.
    pub potential_angle: [f32; 2],
    /// The speeds of impact between which it may.
    pub potential_velocity: [f32; 2],
    /// The effect then.
    pub potential_effect: Option<TagRef>,
    /// What its effects are scaled by: 0 damage, 1 angle.
    pub scale_effects_by: i16,
    /// How much a reflection's direction varies.
    pub angular_noise: f32,
    /// How much a reflection's speed varies.
    pub velocity_noise: f32,
    /// The effect where it detonates.
    pub detonation_effect: Option<TagRef>,
    /// The friction when it first touches.
    pub initial_friction: f32,
    /// The farthest it goes through.
    pub maximum_distance: f32,
    /// The friction along the surface.
    pub parallel_friction: f32,
    /// The friction across it.
    pub perpendicular_friction: f32,
}

impl MaterialResponse {
    /// Disappear.
    pub const DISAPPEAR: i16 = 0;
    /// Detonate.
    pub const DETONATE: i16 = 1;
    /// Reflect (bounce).
    pub const REFLECT: i16 = 2;
    /// Overpenetrate.
    pub const OVERPENETRATE: i16 = 3;
    /// Attach (stick).
    pub const ATTACH: i16 = 4;
    /// The potential response is only against units.
    pub const ONLY_AGAINST_UNITS: u16 = 1 << 0;
}

/// Equipment's own part of its definition (grenades, powerups,
/// ammunition).
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Equipment {
    /// Its item part.
    pub item: Item,
    /// The powerup it gives: 0 none, 1 double speed, 2 overshield, 3 active
    /// camouflage, 4 full spectrum vision, 5 health, 6 grenade.
    pub powerup_type: i16,
    /// The grenade it gives: 0 frag, 1 plasma.
    pub grenade_type: i16,
    /// How long its powerup lasts.
    pub powerup_time: f32,
    /// The sound when it is picked up.
    pub pickup_sound: Option<TagRef>,
}

/// A damage effect (`jpt!`): what a hit, an explosion or a fall does.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct DamageEffect {
    /// Its tag.
    pub tag: TagRef,
    /// The radius within which it does its damage in full, and the one
    /// past which it does none (none for a hit's).
    pub radius: [f32; 2],
    /// How much it does at the outer radius, as a scale.
    pub cutoff_scale: f32,
    /// Its flags ([`DamageEffect::DONT_SCALE_DAMAGE_BY_DISTANCE`]).
    pub flags: u32,
    /// Its side effect: 0 none, 1 harmless, 2 lethal to the unsuspecting,
    /// 3 emp.
    pub side_effect: i16,
    /// What it is counted as (falling, a grenade's).
    pub category: i16,
    /// Its damage's flags ([`DamageEffect::DOES_NOT_HURT_OWNER`] and the
    /// rest after it).
    pub damage_flags: u32,
    /// The radius within which an area's damage is not lessened.
    pub area_of_effect_core_radius: f32,
    /// The least it does.
    pub damage_minimum: f32,
    /// The range it does, at random.
    pub damage: [f32; 2],
    /// How much of it passes through a vehicle to its riders.
    pub vehicle_passthrough_penalty: f32,
    /// How much it lessens active camouflage.
    pub active_camouflage_damage: f32,
    /// How much it stuns.
    pub stun: f32,
    /// The most it stuns.
    pub maximum_stun: f32,
    /// How long it stuns.
    pub stun_time: f32,
    /// The push it gives, in world units a second.
    pub instantaneous_acceleration: f32,
    /// Its damage on each of the materials, as a scale (`MATERIAL_TYPES`
    /// of them, the materials' order).
    pub material_modifiers: Vec<f32>,
}

impl DamageEffect {
    /// The materials the modifiers are of.
    pub const MATERIAL_TYPES: usize = 40;
    /// Its damage does not fall off across its radius.
    pub const DONT_SCALE_DAMAGE_BY_DISTANCE: u32 = 1 << 0;
    /// It does not hurt the one who caused it.
    pub const DOES_NOT_HURT_OWNER: u32 = 1 << 0;
    /// It hits the head for more.
    pub const CAN_CAUSE_HEADSHOTS: u32 = 1 << 1;
    /// It pings units that resist it.
    pub const PINGS_RESISTANT_UNITS: u32 = 1 << 2;
    /// It does not hurt the owner's team.
    pub const DOES_NOT_HURT_FRIENDS: u32 = 1 << 3;
    /// It does not ping units.
    pub const DOES_NOT_PING_UNITS: u32 = 1 << 4;
    /// It sets off explosives.
    pub const DETONATES_EXPLOSIVES: u32 = 1 << 5;
    /// It hurts only shields.
    pub const ONLY_HURTS_SHIELDS: u32 = 1 << 6;
    /// Those it kills die in flames.
    pub const CAUSES_FLAMING_DEATH: u32 = 1 << 7;
    /// It skips shields.
    pub const SKIPS_SHIELDS: u32 = 1 << 9;
    /// It hits the head for more in multiplayer.
    pub const CAN_CAUSE_MULTIPLAYER_HEADSHOTS: u32 = 1 << 11;
}

/// One of the globals' grenades.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct Grenade {
    /// The most a player carries.
    pub maximum_count: i16,
    /// How many a player spawns with in multiplayer.
    pub multiplayer_spawn_default: i16,
    /// The effect when it is thrown.
    pub throwing_effect: Option<TagRef>,
    /// Its HUD.
    pub hud_interface: Option<TagRef>,
    /// The equipment it is, lying about.
    pub item: Option<TagRef>,
    /// The projectile it is, thrown.
    pub projectile: Option<TagRef>,
}

/// An effect (`effe`): its events, each making its parts in turn, and,
/// gathered from them, the damage, lights and decals it makes.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Effect {
    /// Its tag.
    pub tag: TagRef,
    /// The damage effects (`jpt!`) its events' parts create, in order.
    pub damage: Vec<TagRef>,
    /// The particles its events emit that are heavy enough to be bodies
    /// (spent casings and the like), in order.
    pub debris: Vec<EffectDebris>,
    /// The lights (`ligh`) its events' parts create, in order.
    pub lights: Vec<TagRef>,
    /// The decals (`deca`) its events' parts lay where it is made, in
    /// order.
    pub decals: Vec<TagRef>,
    /// Its flags.
    pub flags: u32,
    /// The event a looping effect (one attached to an object) goes back
    /// to after its last (none: it does not loop).
    pub loop_start_event: i16,
    /// The markers its parts are made at, by their `location`.
    pub locations: Vec<String>,
    /// Its events, one after another.
    pub events: Vec<EffectEvent>,
}

impl Effect {
    /// It is deleted when what it is attached to stops making it.
    pub const DELETED_WHEN_INACTIVE: u32 = 1;

    /// An effect named `tag` that makes nothing.
    pub fn new(tag: TagRef) -> Self {
        Self {
            tag,
            damage: Vec::new(),
            debris: Vec::new(),
            lights: Vec::new(),
            decals: Vec::new(),
            flags: 0,
            loop_start_event: -1,
            locations: Vec::new(),
            events: Vec::new(),
        }
    }
}

/// One of an effect's events: after its delay its parts are made, and the
/// next event begins once it has lasted its duration.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct EffectEvent {
    /// The chance, 0 to 1, that it is skipped.
    pub skip_fraction: f32,
    /// The least and most seconds before it begins.
    pub delay: [f32; 2],
    /// The least and most seconds it lasts.
    pub duration: [f32; 2],
    /// What it makes when it begins.
    pub parts: Vec<EffectPart>,
    /// The particles it emits over its duration.
    pub particles: Vec<crate::EffectParticles>,
}

/// Something an event makes: an object, a light, a decal, damage, a sound
/// or another effect.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct EffectPart {
    /// Where it may be made: 0 anywhere, 1 in air, 2 in water, 3 in space.
    pub create_in: i16,
    /// The game's mode it is made in: 0 either, 1 violent (blood shown),
    /// 2 not violent.
    pub create_in_mode: i16,
    /// The effect's location it is made at (none: the effect's origin).
    pub location: i16,
    /// Its flags.
    pub flags: u16,
    /// What is made.
    pub tag: TagRef,
    /// The least and most speed it leaves at.
    pub velocity: [f32; 2],
    /// The cone about the location's forward it leaves within.
    pub velocity_cone_angle: f32,
    /// The least and most speed it spins at, radians a second.
    pub angular_velocity: [f32; 2],
    /// The least and most its radius is scaled by.
    pub radius_modifier: [f32; 2],
}

impl EffectPart {
    /// It faces down, wherever its location faces (a decal laid on the
    /// ground).
    pub const FACE_DOWN: u16 = 1;
    /// Made in air.
    pub const IN_AIR: i16 = 1;
    /// Made in water.
    pub const IN_WATER: i16 = 2;
    /// Made in space.
    pub const IN_SPACE: i16 = 3;
    /// Made only when blood is shown.
    pub const VIOLENT: i16 = 1;
    /// Made only when blood is not shown.
    pub const NOT_VIOLENT: i16 = 2;
}

/// An effect's particles heavy enough to be bodies: their point physics
/// (`pphy`) is dense, so each tumbles and comes to rest as a body would.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct EffectDebris {
    /// The fewest and most of them an event emits.
    pub count: [i16; 2],
    /// Where they leave from, in the effect's frame.
    pub offset: [f32; 3],
    /// The way they leave, in the effect's frame: the yaw and pitch.
    pub direction: [f32; 2],
    /// The least and most speed they leave at.
    pub velocity: [f32; 2],
    /// The cone about their direction they leave within.
    pub velocity_cone_angle: f32,
    /// Each one's radius, in world units.
    pub radius: f32,
    /// Their density, in grams a millilitre.
    pub density: f32,
    /// Their friction on what they touch, 0 to 1.
    pub friction: f32,
    /// Their bounce, 0 to 1.
    pub elasticity: f32,
}
