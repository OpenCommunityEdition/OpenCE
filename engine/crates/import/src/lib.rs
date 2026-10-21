//! The engine's import: reads the stock game's caches (the Xbox's and Halo
//! Custom Edition's) and turns them into the engine's own format.
//!
//! A cache is untrusted input. Every read here is checked against what was
//! read, and a refusal names the tag and the field it is about.

pub mod bitmap;
pub mod cache;
mod effect;
pub mod error;
pub mod globals;
mod hud;
pub mod level;
pub mod level_cache;
pub mod object;
pub mod reader;
pub mod shader;
pub mod tags;
mod texture;
mod vector_palette;
pub mod vehicle;
pub mod weapon;

#[cfg(test)]
mod fake;
#[cfg(test)]
mod fuzz_smoke;

pub use cache::{Cache, CacheHeader, CacheKind};
pub use error::ImportError;
pub use globals::import_globals;
pub use level::import_level;
pub use level_cache::{load_level, LoadError, LoadedFrom, LoadedLevel, IMPORTER_VERSION};
pub use object::{import_animation_graph, import_collision_model, import_model, import_object};
pub use tags::{Block, Element, GroupTag, Region, TagInstance};
pub use vehicle::{import_physics, import_vehicle};
