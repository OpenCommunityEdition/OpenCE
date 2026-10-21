//! The engine's import: reads the stock game's caches (the Xbox's and Halo
//! Custom Edition's) and turns them into the engine's own format.
//!
//! A cache is untrusted input. Every read here is checked against what was
//! read, and a refusal names the tag and the field it is about.

pub mod cache;
pub mod error;
pub mod reader;
pub mod tags;

#[cfg(test)]
mod fake;

pub use cache::{Cache, CacheHeader, CacheKind};
pub use error::ImportError;
pub use tags::{Block, Element, GroupTag, TagInstance};
