//! Sounds (`snd!`) and looping sounds (`lsnd`): what a map's objects,
//! effects, scripts and environments play. A sound's samples are kept as
//! the map encodes them (Xbox ADPCM, 16-bit PCM or Ogg Vorbis) and decoded
//! by the mixer when it first plays them: decoded, a campaign map's sounds
//! would be four times the size.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// How a sound's samples are encoded (`sound_compression`).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Archive, Serialize, Deserialize)]
pub enum SoundCompression {
    /// 16-bit little-endian PCM, the channels interleaved.
    Pcm,
    /// Xbox ADPCM: blocks of 36 bytes a channel, 64 samples each.
    XboxAdpcm,
    /// IMA ADPCM, which no shipped map uses.
    ImaAdpcm,
    /// Ogg Vorbis (Custom Edition's).
    Ogg,
}

impl SoundCompression {
    /// The compression of the tag's value, if it is one.
    pub const fn from_tag(value: i16) -> Option<Self> {
        match value {
            0 => Some(Self::Pcm),
            1 => Some(Self::XboxAdpcm),
            2 => Some(Self::ImaAdpcm),
            3 => Some(Self::Ogg),
            _ => None,
        }
    }
}

/// How a sound's skip fraction, gain and pitch change with its scale (an
/// object function, a weapon's heat, a vehicle's speed): their values at
/// scale 0 and at scale 1.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct SoundModifiers {
    /// The skip fraction's modifier.
    pub skip_fraction: f32,
    /// The gain's modifier.
    pub gain: f32,
    /// The pitch's modifier.
    pub pitch: f32,
}

/// One take of a sound: its samples, and the mouth's movements a speaking
/// unit makes to it.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct SoundPermutation {
    /// Its name.
    pub name: String,
    /// The chance it is skipped when chosen at random.
    pub skip_fraction: f32,
    /// Its gain.
    pub gain: f32,
    /// How its samples are encoded.
    pub compression: SoundCompression,
    /// The permutation played after it, for a sound of linked
    /// permutations; `None` for the last.
    pub next: Option<u16>,
    /// Its samples, encoded.
    pub samples: Vec<u8>,
    /// The mouth's aperture each tick (of 30 a second), 0 to 255.
    pub mouth: Vec<u8>,
}

/// The permutations of a sound made at one pitch, which play a pitch near
/// it.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct SoundPitchRange {
    /// Its name.
    pub name: String,
    /// The pitch its samples were made at.
    pub natural_pitch: f32,
    /// The pitches it plays, its bend bounds.
    pub bend_bounds: [f32; 2],
    /// How many of its permutations are chosen at random (the rest are
    /// only reached by links).
    pub actual_permutation_count: u16,
    /// The rate its samples play at, for a pitch of 1.
    pub playback_rate: f32,
    /// Its permutations.
    pub permutations: Vec<SoundPermutation>,
}

/// A sound, as its tag defines it. Distances are in world units, angles in
/// radians.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Sound {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags ([`Sound::LINKED_PERMUTATIONS`]).
    pub flags: u32,
    /// Its class (`sound_class`), which sets its priority, its counts and
    /// its distances when it gives none.
    pub class: u16,
    /// Its samples' rate, 22050 or 44100.
    pub sample_rate: u32,
    /// Its channels: 1 or 2.
    pub channels: u8,
    /// Within its minimum distance it plays at full gain; it is not heard
    /// past its maximum (0: its class's).
    pub distance: [f32; 2],
    /// The chance it is skipped.
    pub skip_fraction: f32,
    /// The random pitch it plays at.
    pub random_pitch_bounds: [f32; 2],
    /// Its cone: inside the inner angle it plays at full gain, outside the
    /// outer at the outer gain.
    pub cone_angles: [f32; 2],
    /// Its gain outside its outer cone.
    pub outer_cone_gain: f32,
    /// Its gain.
    pub gain_modifier: f32,
    /// How fast its pitch may bend, a second.
    pub maximum_bend_per_second: f32,
    /// Its modifiers at scale 0.
    pub zero: SoundModifiers,
    /// Its modifiers at scale 1.
    pub one: SoundModifiers,
    /// The sound played in its place when it plays too often.
    pub promotion: Option<TagRef>,
    /// How many plays at once promote it.
    pub promotion_count: u16,
    /// Its longest permutation's length, in milliseconds.
    pub longest_permutation_ms: u32,
    /// Its pitch ranges.
    pub pitch_ranges: Vec<SoundPitchRange>,
}

impl Sound {
    /// Its permutations were cut to whole compression blocks.
    pub const FIT_TO_COMPRESSION_BLOCK: u32 = 1 << 0;
    /// Its permutations play one after another, by their links.
    pub const LINKED_PERMUTATIONS: u32 = 1 << 1;
}

/// One track of a looping sound: an optional start, a loop, and a stop,
/// with alternates its object may switch to.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LoopingSoundTrack {
    /// Its flags ([`LoopingSoundTrack::FADE_IN_AT_START`] and the rest).
    pub flags: u32,
    /// Its gain.
    pub gain: f32,
    /// How long it fades in, in seconds.
    pub fade_in: f32,
    /// How long it fades out, in seconds.
    pub fade_out: f32,
    /// Played once as it starts.
    pub start: Option<TagRef>,
    /// Played over and over.
    pub looping: Option<TagRef>,
    /// Played once as it stops.
    pub stop: Option<TagRef>,
    /// The loop played while its alternate is on.
    pub alternate_loop: Option<TagRef>,
    /// The stop played while its alternate is on.
    pub alternate_stop: Option<TagRef>,
}

impl LoopingSoundTrack {
    /// It fades in when the looping sound starts.
    pub const FADE_IN_AT_START: u32 = 1 << 0;
    /// It fades out when the looping sound stops.
    pub const FADE_OUT_AT_STOP: u32 = 1 << 1;
    /// It fades in when its alternate turns on.
    pub const FADE_IN_ALTERNATE: u32 = 1 << 2;
}

/// A sound a looping sound plays now and again, from a random direction
/// and distance around its source.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LoopingSoundDetail {
    /// The sound.
    pub sound: Option<TagRef>,
    /// The seconds between plays.
    pub period_bounds: [f32; 2],
    /// Its gain.
    pub gain: f32,
    /// Its flags ([`LoopingSoundDetail::NOT_WITH_ALTERNATE`] and the rest).
    pub flags: u32,
    /// The yaw it comes from.
    pub theta_bounds: [f32; 2],
    /// The pitch it comes from.
    pub phi_bounds: [f32; 2],
    /// How far away it plays.
    pub distance_bounds: [f32; 2],
}

impl LoopingSoundDetail {
    /// Not played while the alternate is on.
    pub const NOT_WITH_ALTERNATE: u32 = 1 << 0;
    /// Not played while the alternate is off.
    pub const NOT_WITHOUT_ALTERNATE: u32 = 1 << 1;
}

/// A looping sound: an environment's, a machine's, a vehicle's engine's,
/// or the music's.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LoopingSound {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags ([`LoopingSound::STOPS_MUSIC`] and the rest).
    pub flags: u32,
    /// The detail sounds' period's scale at scale 0 and at scale 1.
    pub detail_period: [f32; 2],
    /// Its tracks (up to 4), played together.
    pub tracks: Vec<LoopingSoundTrack>,
    /// Its detail sounds (up to 32).
    pub details: Vec<LoopingSoundDetail>,
}

impl LoopingSound {
    /// It deafens: other sounds are quieter while it plays.
    pub const DEAFENING: u32 = 1 << 0;
    /// It is played as an impulse sound.
    pub const FAKE_IMPULSE: u32 = 1 << 1;
    /// It stops the music that is playing.
    pub const STOPS_MUSIC: u32 = 1 << 2;
}

/// A sound environment (`sound_environment`, the `snde` tag): the reverb of
/// the clusters it is given to.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct SoundEnvironment {
    /// Which of two environments the listener's clusters give is heard:
    /// the higher.
    pub priority: i16,
    /// Its values, in the tag's order: room intensity, room intensity at
    /// high frequencies, room rolloff, decay time, decay's high-frequency
    /// ratio, reflections' intensity, reflections' delay, reverb
    /// intensity, reverb delay, diffusion, density and the high-frequency
    /// reference.
    pub values: [f32; 12],
}

/// What a BSP's cluster is heard with: indices into its background sounds
/// and sound environments.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct ClusterSound {
    /// Its background sound, a looping sound played while the listener is
    /// in the cluster.
    pub background: Option<u16>,
    /// Its sound environment.
    pub environment: Option<u16>,
}

/// What a footstep, a slide or a landing makes on each material
/// (`material_effects`, the `foot` tag): by its event, the effect and the
/// sound of each of the game's materials.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct MaterialEffects {
    /// Its tag.
    pub tag: TagRef,
    /// Its events ([`MaterialEffects::WALK`] and the rest), each by
    /// material type.
    pub events: Vec<Vec<MaterialEffect>>,
}

/// What an event makes on one material.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct MaterialEffect {
    /// The effect it starts.
    pub effect: Option<TagRef>,
    /// The sound it plays.
    pub sound: Option<TagRef>,
}

impl MaterialEffects {
    /// A step, walking (crouched).
    pub const WALK: usize = 0;
    /// A step, running (standing).
    pub const RUN: usize = 1;
    /// A slide.
    pub const SLIDING: usize = 2;
    /// A shuffle of the feet, standing still.
    pub const SHUFFLE: usize = 3;
    /// A jump.
    pub const JUMP: usize = 4;
    /// A landing from a jump.
    pub const JUMP_LAND: usize = 5;
    /// An item striking something.
    pub const IMPACT: usize = 8;
    /// A vehicle's mass point slipping on the ground.
    pub const VEHICLE_SLIP: usize = 9;
    /// A vehicle's metallic mass point (a tread) slipping on the ground.
    pub const METALLIC_SLIP: usize = 10;

    /// What `event` makes on material `material` (a negative one: none).
    pub fn of(&self, event: usize, material: i16) -> Option<&MaterialEffect> {
        let material = usize::try_from(material).ok()?;
        self.events.get(event)?.get(material)
    }
}
