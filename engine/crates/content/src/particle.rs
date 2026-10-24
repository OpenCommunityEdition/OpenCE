//! What the renderer draws as sprites: particles (`part`) an effect's
//! events emit, contrails (`cont`) objects trail, the weather (`rain`) a
//! BSP's clusters have, and detail objects (`dobc`), grass and the like,
//! scattered over a BSP's ground.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// How a point moves (`pphy`): its weight, and how air and water drag it.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct PointPhysics {
    /// Its flags ([`PointPhysics::NO_GRAVITY`]).
    pub flags: u32,
    /// Its mass for a radius of 1 (worked out by the tools).
    pub mass_over_radius_cubed: f32,
    /// How much of gravity it feels in water (worked out by the tools).
    pub water_buoyancy: f32,
    /// How much of gravity it feels in air (worked out by the tools).
    pub air_buoyancy: f32,
    /// Its density, grams a millilitre.
    pub density: f32,
    /// How air drags it.
    pub air_friction: f32,
    /// How water drags it.
    pub water_friction: f32,
    /// How much of its speed along a surface it meets it loses, 0 to 1.
    pub contact_friction: f32,
    /// How much of its speed into a surface it meets it keeps, turned
    /// back.
    pub elasticity: f32,
}

impl PointPhysics {
    /// It is a flamethrower's flame.
    pub const FLAMETHROWER: u32 = 1 << 0;
    /// It meets the level's structure.
    pub const STRUCTURE_COLLISIONS: u32 = 1 << 1;
    /// It meets water's surface.
    pub const WATER_COLLISIONS: u32 = 1 << 2;
    /// The wind blows it.
    pub const SIMPLE_WIND: u32 = 1 << 3;
    /// The wind blows it, damped.
    pub const DAMPED_WIND: u32 = 1 << 4;
    /// It feels no gravity.
    pub const NO_GRAVITY: u32 = 1 << 5;
}

/// The look of something drawn as sprites: how its picture is blended.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct SpriteShader {
    /// Its flags (bit 0: sorted, bit 1: lit by the probes).
    pub flags: u16,
    /// How it is blended: 0 alpha blend, 1 multiply, 2 double multiply,
    /// 3 add, 4 subtract, 5 least, 6 greatest, 7 alpha multiply add.
    pub framebuffer_blend_function: i16,
    /// How it fades: 0 none, 1 when it faces away, 2 when it faces the
    /// camera.
    pub fade_mode: i16,
}

/// A particle (`part`): its picture and how its sprites, size and
/// opacity change over its life.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Particle {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags.
    pub flags: u32,
    /// Its picture: its sequences are its animations.
    pub bitmap: Option<TagRef>,
    /// How it moves, if it moves by more than its own velocity.
    pub physics: Option<PointPhysics>,
    /// The fewest and most seconds it lives.
    pub lifespan: [f32; 2],
    /// The seconds it fades in over at birth.
    pub fade_in: f32,
    /// The seconds it fades out over at death.
    pub fade_out: f32,
    /// Its radius at birth and at death, by the radius it was emitted at.
    pub radius: [f32; 2],
    /// The fewest and most frames a second its animation plays at.
    pub fps: [f32; 2],
    /// The first of its bitmap's sequences it plays.
    pub first_sequence: i16,
    /// How many sequences it may begin with, one at random.
    pub initial_sequences: i16,
    /// How many it may loop through while it lives.
    pub looping_sequences: i16,
    /// How many it may end with.
    pub final_sequences: i16,
    /// The way its sprite faces: 0 the screen, 1 parallel to its
    /// velocity, 2 across it, 3 along it.
    pub orientation: i16,
    /// How it is blended.
    pub shader: SpriteShader,
    /// What it makes on the materials it strikes (`foot`).
    pub collision_material_effects: Option<TagRef>,
    /// What it makes where it strikes the structure (`effe` or `snd!`).
    pub collision_effect: Option<TagRef>,
    /// What it makes where it dies (`effe` or `snd!`).
    pub death_effect: Option<TagRef>,
    /// The fewest pixels it is drawn at: smaller, it is drawn this big.
    pub minimum_pixels: f32,
    /// The seconds its frames slow by each time it settles on the ground.
    pub contact_deterioration: f32,
    /// The distances it thins out from and is gone by (falloff, cutoff).
    pub lod: [f32; 2],
    /// Its picture's flags (bit 0: unfiltered).
    pub primary_map_flags: u16,
    /// Its second picture, laid over its first.
    pub secondary_map: Option<SecondaryMap>,
}

/// A particle's second picture.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct SecondaryMap {
    /// The picture.
    pub bitmap: TagRef,
    /// What it turns with: 0 the world, 1 the screen, 2 the particle.
    pub anchor: i16,
    /// Its flags (bit 0: unfiltered).
    pub flags: u16,
    /// How big it is, by the particle's radius.
    pub radius: f32,
}

impl Particle {
    /// It may play its frames backwards.
    pub const CAN_ANIMATE_BACKWARDS: u32 = 1 << 0;
    /// Its frames stop when it comes to rest.
    pub const ANIMATION_STOPS_AT_REST: u32 = 1 << 1;
    /// It begins on a frame at random.
    pub const ANIMATION_STARTS_ON_RANDOM_FRAME: u32 = 1 << 2;
    /// It moves on a frame at most each frame drawn.
    pub const ANIMATE_ONCE_PER_FRAME: u32 = 1 << 3;
    /// It dies when it comes to rest.
    pub const DIES_AT_REST: u32 = 1 << 4;
    /// It dies when it strikes the structure.
    pub const DIES_ON_CONTACT_WITH_STRUCTURE: u32 = 1 << 5;
    /// It takes its colour from its picture's under it.
    pub const TINT_FROM_DIFFUSE_TEXTURE: u32 = 1 << 6;
    /// It dies in water.
    pub const DIES_ON_CONTACT_WITH_WATER: u32 = 1 << 7;
    /// It dies out of water.
    pub const DIES_ON_CONTACT_WITH_AIR: u32 = 1 << 8;
    /// It is not lit: it shines with its own colour.
    pub const SELF_ILLUMINATED: u32 = 1 << 9;
    /// It may be mirrored left to right.
    pub const RANDOM_HORIZONTAL_MIRRORING: u32 = 1 << 10;
    /// It may be mirrored top to bottom.
    pub const RANDOM_VERTICAL_MIRRORING: u32 = 1 << 11;

    /// Its radius `fraction` of the way through its life, for a radius of
    /// 1 at emission (`particle_get_radius`).
    pub fn radius_at(&self, fraction: f32) -> f32 {
        let [birth, death] = self.radius;
        (death - birth) * fraction.clamp(0.0, 1.0) + birth
    }

    /// A particle named `tag` with no picture, living one second.
    pub fn new(tag: TagRef) -> Self {
        Self {
            tag,
            flags: 0,
            bitmap: None,
            physics: None,
            lifespan: [1.0, 1.0],
            fade_in: 0.0,
            fade_out: 0.0,
            radius: [1.0, 1.0],
            fps: [0.0, 0.0],
            first_sequence: 0,
            initial_sequences: 1,
            looping_sequences: 0,
            final_sequences: 0,
            orientation: 0,
            shader: SpriteShader::default(),
            collision_material_effects: None,
            collision_effect: None,
            death_effect: None,
            minimum_pixels: 0.0,
            contact_deterioration: 0.0,
            lod: [0.0, 0.0],
            primary_map_flags: 0,
            secondary_map: None,
        }
    }
}

/// The particles an effect's event emits.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct EffectParticles {
    /// What is emitted.
    pub particle: TagRef,
    /// The effect's location they leave from.
    pub location: i16,
    /// Its flags ([`EffectParticles::RANDOM_ROTATION`]).
    pub flags: u32,
    /// How they are emitted over the event: 0 all at its start, 1 all at
    /// its end, 2 evenly, 3 more and more, 4 fewer and fewer, 5 most in
    /// the middle.
    pub distribution: i16,
    /// The fewest and most of them.
    pub count: [i16; 2],
    /// How far from the location they may begin, least and most.
    pub distribution_radius: [f32; 2],
    /// The way they leave, by yaw and pitch from the location's forward.
    pub direction: [f32; 2],
    /// Where they leave from, in the location's frame.
    pub offset: [f32; 3],
    /// The least and most speed they leave at.
    pub velocity: [f32; 2],
    /// The cone about their direction they leave within.
    pub velocity_cone_angle: f32,
    /// The least and most they spin at, radians a second.
    pub angular_velocity: [f32; 2],
    /// The least and most radius they are emitted at.
    pub radius: [f32; 2],
    /// The colours they are tinted between, alpha, red, green, blue.
    pub tint: [[f32; 4]; 2],
    /// Where they may be made: 0 anywhere, 1 in air, 2 in water, 3 in a
    /// vacuum.
    pub environment: i16,
    /// The effects they are made by: 0 any, 1 violent, 2 nonviolent.
    pub disposition: i16,
    /// Who sees them: 0 any camera, 1 the first person only, 2 the third
    /// person only.
    pub camera_mode: i16,
    /// The values the effect's scales A and B scale, each a mask of
    /// [`EffectParticles::SCALES_VELOCITY`] and the others.
    pub scales: [u32; 2],
}

impl EffectParticles {
    /// They stay with the location (move as it moves).
    pub const STAY_ATTACHED: u32 = 1 << 0;
    /// Each is turned at random.
    pub const RANDOM_ROTATION: u32 = 1 << 1;
    /// Their tint is multiplied by the effect's colour.
    pub const TINT_FROM_CHANGE_COLOR: u32 = 1 << 2;

    /// A scale scales the least speed.
    pub const SCALES_VELOCITY: u32 = 1 << 0;
    /// A scale scales the speed above the least.
    pub const SCALES_VELOCITY_DELTA: u32 = 1 << 1;
    /// A scale scales the cone.
    pub const SCALES_VELOCITY_CONE: u32 = 1 << 2;
    /// A scale scales the least spin.
    pub const SCALES_ANGULAR_VELOCITY: u32 = 1 << 3;
    /// A scale scales the spin above the least.
    pub const SCALES_ANGULAR_VELOCITY_DELTA: u32 = 1 << 4;
    /// A scale scales the fewest made.
    pub const SCALES_COUNT: u32 = 1 << 5;
    /// A scale scales how many above the fewest.
    pub const SCALES_COUNT_DELTA: u32 = 1 << 6;
    /// A scale scales the least distance from the location.
    pub const SCALES_DISTRIBUTION_RADIUS: u32 = 1 << 7;
    /// A scale scales the distance above the least.
    pub const SCALES_DISTRIBUTION_RADIUS_DELTA: u32 = 1 << 8;
    /// A scale scales the least radius.
    pub const SCALES_RADIUS: u32 = 1 << 9;
    /// A scale scales the radius above the least.
    pub const SCALES_RADIUS_DELTA: u32 = 1 << 10;
    /// A scale picks the tint between its bounds (otherwise it is random).
    pub const SCALES_TINT: u32 = 1 << 11;

    /// Made in air only.
    pub const IN_AIR: i16 = 1;
    /// Made in water only.
    pub const IN_WATER: i16 = 2;
    /// Made in a vacuum only.
    pub const IN_VACUUM: i16 = 3;
    /// Made by violent effects only.
    pub const VIOLENT: i16 = 1;
    /// Made by nonviolent effects only.
    pub const NONVIOLENT: i16 = 2;

    /// Emitted all at once when the event begins.
    pub const START: i16 = 0;
    /// Emitted all at once when it ends.
    pub const END: i16 = 1;
    /// Emitted evenly over it.
    pub const CONSTANT: i16 = 2;
    /// Emitted more and more over it.
    pub const BUILDUP: i16 = 3;
    /// Emitted fewer and fewer over it.
    pub const FALLOFF: i16 = 4;
    /// Emitted most in its middle.
    pub const BUILDUP_AND_FALLOFF: i16 = 5;
}

/// A contrail (`cont`): a trail of points an object leaves behind, drawn
/// as a strip through them.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Contrail {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags.
    pub flags: u16,
    /// Which of its values its attachment's function scales, by the
    /// `SCALE_` bits.
    pub scale_flags: u16,
    /// Points made a second.
    pub point_generation_rate: f32,
    /// The least and most speed its points leave at.
    pub velocity: [f32; 2],
    /// The cone about the marker's forward they leave within.
    pub velocity_cone_angle: f32,
    /// The part of the object's velocity they keep.
    pub inherited_velocity_fraction: f32,
    /// How the strip lies: 0 vertical, 1 horizontal, 2 media, 3 ground,
    /// 4 facing the viewer.
    pub render_type: i16,
    /// How many times its picture repeats across and along the strip.
    pub texture_repeats: [f32; 2],
    /// How fast its picture slides across and along, repeats a second.
    pub texture_animation: [f32; 2],
    /// The frames of its picture's sequence a second.
    pub frames_per_second: f32,
    /// Its picture.
    pub bitmap: Option<TagRef>,
    /// The first of its bitmap's sequences, and how many there are.
    pub sequences: [i16; 2],
    /// How it is blended.
    pub shader: SpriteShader,
    /// What its points become as they age, one after another.
    pub states: Vec<ContrailState>,
}

impl Contrail {
    /// Its first point is not faded out.
    pub const FIRST_POINT_UNFADED: u16 = 1 << 0;
    /// Its last point is not faded out.
    pub const LAST_POINT_UNFADED: u16 = 1 << 1;
    /// Its strip fades slowly as it turns edge on.
    pub const FADES_SLOWLY: u16 = 1 << 6;

    /// A strip standing up through its points.
    pub const VERTICAL: i16 = 0;
    /// A strip lying flat through its points.
    pub const HORIZONTAL: i16 = 1;
    /// A strip on the surface of water.
    pub const MEDIA: i16 = 2;
    /// A strip on the ground.
    pub const GROUND: i16 = 3;
    /// A strip turned to the viewer about its line.
    pub const VIEWER_FACING: i16 = 4;

    /// The bit of `scale_flags` scaling its point rate.
    pub const SCALE_RATE: u32 = 0;
    /// The bits scaling its points' least speed, and the rest of it.
    pub const SCALE_VELOCITY: u32 = 1;
    /// The bit scaling its cone.
    pub const SCALE_CONE_ANGLE: u32 = 3;
    /// The bit scaling the object's velocity its points keep.
    pub const SCALE_INHERITED_VELOCITY: u32 = 4;
    /// The bit scaling its frames a second.
    pub const SCALE_ANIMATION_RATE: u32 = 5;
    /// The bit scaling its picture's repeats across.
    pub const SCALE_REPEATS_U: u32 = 6;
    /// The bit scaling its picture's repeats along.
    pub const SCALE_REPEATS_V: u32 = 7;
    /// The bit scaling its picture's slide across.
    pub const SCALE_ANIMATION_U: u32 = 8;
    /// The bit scaling its picture's slide along.
    pub const SCALE_ANIMATION_V: u32 = 9;
}

/// One of a contrail's point states.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct ContrailState {
    /// The least and most seconds a point is in it.
    pub duration: [f32; 2],
    /// The least and most seconds it takes to become the next.
    pub transition: [f32; 2],
    /// The strip's width at a point in it.
    pub width: f32,
    /// The colours it is between, alpha, red, green, blue.
    pub color: [[f32; 4]; 2],
    /// How its points move.
    pub physics: Option<PointPhysics>,
    /// Which of its values a point's density scales, by the `SCALE_` bits.
    pub scale_flags: u32,
}

impl ContrailState {
    /// The bits scaling its least seconds, and the rest of them.
    pub const SCALE_DURATION: u32 = 0;
    /// The bits scaling its least seconds becoming the next, and the rest.
    pub const SCALE_TRANSITION: u32 = 2;
    /// The bit scaling its width.
    pub const SCALE_WIDTH: u32 = 4;
    /// The bit taking its colour between its two by the density.
    pub const SCALE_COLOR: u32 = 5;
}

/// A particle system (`pctl`): kinds of particle an effect or an object
/// makes, each a machine of states its emitter and its particles pass
/// through.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ParticleSystem {
    /// Its tag.
    pub tag: TagRef,
    /// How the system itself moves, when nothing carries it.
    pub point_physics: Option<PointPhysics>,
    /// How it moves: 0 by its point physics.
    pub update_physics: i16,
    /// Its physics' flags.
    pub physics_flags: u32,
    /// Its physics' constants.
    pub physics_constants: Vec<f32>,
    /// Its kinds of particle (the original uses four at most).
    pub types: Vec<ParticleSystemType>,
}

/// One of a particle system's kinds of particle.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ParticleSystemType {
    /// Its name.
    pub name: String,
    /// Its flags ([`ParticleSystemType::TYPE_STATES_LOOP`]).
    pub flags: u32,
    /// How many particles it begins with.
    pub initial_count: i16,
    /// How its sprites are drawn: 0 simple, 1 rotational.
    pub complex_sprite_render_mode: i16,
    /// How its sprites face: 0 the screen, 1 parallel to its velocity,
    /// 2 across it.
    pub sprite_render_mode: i16,
    /// Its particles' radius, by which the states' radius is scaled.
    pub radius: f32,
    /// How its particles are made: 0 at the marker, 1 an explosion, 2 a
    /// jet.
    pub creation_physics: i16,
    /// Its physics' flags.
    pub physics_flags: u32,
    /// Its physics' constants.
    pub physics_constants: Vec<f32>,
    /// The states its emitter passes through.
    pub type_states: Vec<ParticleTypeState>,
    /// The states each of its particles passes through.
    pub particle_states: Vec<ParticleState>,
}

impl ParticleSystemType {
    /// Its emitter's states go round again.
    pub const TYPE_STATES_LOOP: u32 = 1 << 0;
    /// ...forwards and backwards.
    pub const TYPE_STATES_LOOP_FORWARD_BACKWARD: u32 = 1 << 1;
    /// Its particles' states go round again.
    pub const PARTICLE_STATES_LOOP: u32 = 1 << 2;
    /// ...forwards and backwards.
    pub const PARTICLE_STATES_LOOP_FORWARD_BACKWARD: u32 = 1 << 3;
    /// Its particles die in water.
    pub const PARTICLES_DIE_IN_WATER: u32 = 1 << 4;
    /// Its particles die in air.
    pub const PARTICLES_DIE_IN_AIR: u32 = 1 << 5;
    /// Its particles die on the ground.
    pub const PARTICLES_DIE_ON_GROUND: u32 = 1 << 6;
    /// Its sprites turn at random as they are made.
    pub const ROTATIONAL_SPRITES_ANIMATE_SIDEWAYS: u32 = 1 << 7;
    /// It is turned off.
    pub const DISABLED: u32 = 1 << 8;
    /// Its colour is the system's times its own.
    pub const TINT_BY_EFFECT_COLOR: u32 = 1 << 9;
    /// Its first count is scaled by the system's scale.
    pub const INITIAL_COUNT_SCALES_WITH_EFFECT: u32 = 1 << 10;
    /// Its fewest particles are scaled by the system's scale.
    pub const MINIMUM_COUNT_SCALES_WITH_EFFECT: u32 = 1 << 11;
    /// Its rate is scaled by the system's scale.
    pub const CREATION_RATE_SCALES_WITH_EFFECT: u32 = 1 << 12;
    /// Its particles' size is scaled by the system's scale.
    pub const SCALE_SCALES_WITH_EFFECT: u32 = 1 << 13;
    /// Its particles' frames are scaled by the system's scale.
    pub const ANIMATION_RATE_SCALES_WITH_EFFECT: u32 = 1 << 14;
    /// Its particles' spin is scaled by the system's scale.
    pub const ROTATION_RATE_SCALES_WITH_EFFECT: u32 = 1 << 15;
    /// It is not drawn in the first person.
    pub const DONT_DRAW_IN_FIRST_PERSON: u32 = 1 << 16;
    /// It is not drawn in the third person.
    pub const DONT_DRAW_IN_THIRD_PERSON: u32 = 1 << 17;
}

/// What a particle's look is multiplied by, drawn between two bounds.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct RandomizedVariables {
    /// Its size.
    pub scale: f32,
    /// Its frames a second.
    pub animation_rate: f32,
    /// Its spin, turns a second.
    pub rotation_rate: f32,
    /// Its colour, alpha, red, green, blue.
    pub color: [f32; 4],
}

impl RandomizedVariables {
    /// `self` with `other` weighted by `t` and `self` by 1 - `t`.
    pub fn mix(&self, other: &Self, t: f32) -> Self {
        let lerp = |a: f32, b: f32| a + (b - a) * t;
        Self {
            scale: lerp(self.scale, other.scale),
            animation_rate: lerp(self.animation_rate, other.animation_rate),
            rotation_rate: lerp(self.rotation_rate, other.rotation_rate),
            color: [0, 1, 2, 3].map(|i| lerp(self.color[i], other.color[i])),
        }
    }
}

/// What an emitter's state sets.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct ParticleTypeVariables {
    /// What its particles' look is multiplied by.
    pub randomized: RandomizedVariables,
    /// What its particles' radius is multiplied by.
    pub radius: f32,
    /// The fewest particles it has: fewer, it runs its state out faster.
    pub minimum_count: f32,
    /// Particles made a second.
    pub creation_rate: f32,
}

impl ParticleTypeVariables {
    /// `self` with `other` weighted by `t` and `self` by 1 - `t`.
    pub fn mix(&self, other: &Self, t: f32) -> Self {
        let lerp = |a: f32, b: f32| a + (b - a) * t;
        Self {
            randomized: self.randomized.mix(&other.randomized, t),
            radius: lerp(self.radius, other.radius),
            minimum_count: lerp(self.minimum_count, other.minimum_count),
            creation_rate: lerp(self.creation_rate, other.creation_rate),
        }
    }
}

/// One of an emitter's states.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ParticleTypeState {
    /// Its name.
    pub name: String,
    /// The least and most seconds it lasts.
    pub duration: [f32; 2],
    /// The least and most seconds it takes to become the next.
    pub transition: [f32; 2],
    /// Its flags.
    pub flags: u32,
    /// What it sets.
    pub variables: ParticleTypeVariables,
    /// How it makes particles, if it overrides its type (`-1`: it does
    /// not).
    pub creation_physics: i16,
    /// How it moves them, if it overrides its type.
    pub update_physics: i16,
    /// Its physics' constants.
    pub physics_constants: Vec<f32>,
}

/// One of a particle's states.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ParticleState {
    /// Its name.
    pub name: String,
    /// The least and most seconds it lasts.
    pub duration: [f32; 2],
    /// The least and most seconds it takes to become the next.
    pub transition: [f32; 2],
    /// Its picture.
    pub bitmap: Option<TagRef>,
    /// The picture's sequence it is drawn from.
    pub sequence: i16,
    /// The least and most size.
    pub scale: [f32; 2],
    /// The least and most frames a second.
    pub animation_rate: [f32; 2],
    /// The least and most spin, turns a second.
    pub rotation_rate: [f32; 2],
    /// The colours it is between, alpha, red, green, blue.
    pub color: [[f32; 4]; 2],
    /// Its radius, by its type's.
    pub radius: f32,
    /// How it moves.
    pub point_physics: Option<PointPhysics>,
    /// How it is blended.
    pub shader: SpriteShader,
    /// Its picture's flags (bit 0: unfiltered).
    pub primary_map_flags: u16,
    /// Its second picture, laid over its first.
    pub secondary_map: Option<SecondaryMap>,
}

/// Weather (`rain`): the kinds of particle that fill a box about the
/// camera where a cluster has it.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeatherSystem {
    /// Its tag.
    pub tag: TagRef,
    /// Its kinds of particle.
    pub types: Vec<WeatherParticleType>,
}

/// A kind of weather particle: snow, rain, drifting dust.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeatherParticleType {
    /// Its flags ([`WeatherParticleType::RANDOM_ROTATION`]).
    pub flags: u32,
    /// The distances it fades in over and out over from the camera: in
    /// from the first to the second, out from the third to the fourth
    /// (the size of its box).
    pub distance_fade: [f32; 4],
    /// The least and most particles in a cubic world unit.
    pub density: [f32; 2],
    /// How they move.
    pub physics: Option<PointPhysics>,
    /// The least and most they accelerate by, and how fast its direction
    /// turns and its size changes.
    pub acceleration: [f32; 4],
    /// The least and most radius.
    pub radius: [f32; 2],
    /// The least and most frames a second.
    pub animation_rate: [f32; 2],
    /// The least and most they spin at, radians a second.
    pub rotation_rate: [f32; 2],
    /// The colours they are between, alpha, red, green, blue.
    pub color: [[f32; 4]; 2],
    /// Its picture.
    pub bitmap: Option<TagRef>,
    /// How it faces: 0 the camera, 1 the camera about its up, and the
    /// rest.
    pub render_mode: i16,
    /// How it is blended.
    pub shader: SpriteShader,
    /// The heights it fades over: in from the first to the second above
    /// the camera, out from the third to the fourth below it.
    pub height_fade: [f32; 4],
    /// Which way an oriented one points: 0 its velocity, 1 its
    /// acceleration.
    pub direction_source: i16,
    /// One over its picture's width in world units, as the tag has it.
    pub one_over_sprite_width: f32,
}

impl WeatherParticleType {
    /// Each is turned at random.
    pub const RANDOM_ROTATION: u32 = 1 << 2;
}

/// Wind (`wind`): how the air moves where a weather palette entry has it.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Wind {
    /// Its tag.
    pub tag: TagRef,
    /// The least and most it blows at.
    pub velocity: [f32; 2],
    /// How far its direction wanders, pitch and yaw.
    pub variation_area: [f32; 2],
    /// How much of it varies from place to place.
    pub local_variation_weight: f32,
    /// How fast that variation changes.
    pub local_variation_rate: f32,
    /// How much damped air keeps of it taken away.
    pub damping: f32,
}

/// A BSP's weather polyhedron: a convex volume weather is not drawn in.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeatherPolyhedron {
    /// The centre of the sphere about it.
    pub center: [f32; 3],
    /// That sphere's radius.
    pub radius: f32,
    /// Its planes, each a normal and a distance, facing out.
    pub planes: Vec<[f32; 4]>,
}

/// One of a BSP's weather palette's entries.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeatherPalette {
    /// Its weather.
    pub system: Option<TagRef>,
    /// Its wind.
    pub wind_tag: Option<TagRef>,
    /// Its wind's direction and speed.
    pub wind: [f32; 4],
}

/// A detail object collection (`dobc`): the sprites a BSP's detail
/// objects of one layer are drawn with.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct DetailObjectCollection {
    /// Its tag.
    pub tag: TagRef,
    /// How its sprites face: 0 the screen, about their up, 1 the screen,
    /// and the rest.
    pub kind: i16,
    /// Its picture, whose sequences its types are.
    pub bitmap: Option<TagRef>,
    /// Its types.
    pub types: Vec<DetailObjectType>,
}

/// One of a collection's kinds of detail object.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct DetailObjectType {
    /// The bitmap's sequence it is drawn from.
    pub sequence: u8,
    /// The first of the sequence's sprites, and how many it is drawn
    /// with.
    pub sprites: [u8; 2],
    /// How much its own colour shows over the sprite's.
    pub color_override: f32,
    /// The distances it fades out between, from the camera.
    pub fade: [f32; 2],
    /// World units a texel, least and most.
    pub size: [f32; 2],
}

/// A detail object a BSP places: a sprite standing on its ground.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct DetailObject {
    /// Where it stands.
    pub position: [f32; 3],
    /// Its colour, red, green and blue.
    pub color: [u8; 3],
    /// Its layer: the scenario's detail object collection it is drawn
    /// with.
    pub collection: u8,
    /// Its type in the collection (the high four bits, by the number of
    /// types) and its sprite in the type (the low four).
    pub data: u8,
}
