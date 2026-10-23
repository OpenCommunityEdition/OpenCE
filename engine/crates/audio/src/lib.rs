//! The sounds the game plays: their decoders ([`decode`]), the game's
//! sound classes ([`classes`]) and the mixer the app's Core Audio output
//! pulls from.

pub mod choice;
pub mod classes;
pub mod decode;
pub mod mixer;
pub mod player;

pub use choice::{
    limit_pitch, next_permutation, pitch_range, scale_random_value, scale_value, LocalRandom,
    Promoted, Promotion, RangeState,
};
pub use classes::{class, class_index, ClassGains, SoundClass, SOUND_CLASSES, SOUND_CLASS_COUNT};
pub use decode::{decode, DecodeError, Decoded};
pub use mixer::{
    distance_attenuation, placement_gains, travel_delay_ms, Listener, Mixer, Placement, VoiceId,
    VoiceProperties, OUTPUT_RATE,
};
pub use player::{
    SoundBank, SoundPlayer, Source, DIALOG_CLASSES, MUSIC_CLASS, UNSPATIALIZED_CLASS,
};
