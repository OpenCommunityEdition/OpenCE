//! The sounds the game plays: their decoders ([`decode`]), the game's
//! sound classes ([`classes`]) and the mixer the app's Core Audio output
//! pulls from.

pub mod classes;
pub mod decode;

pub use classes::{class, class_index, ClassGains, SoundClass, SOUND_CLASSES, SOUND_CLASS_COUNT};
pub use decode::{decode, DecodeError, Decoded};
