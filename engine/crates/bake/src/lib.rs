//! The level's light baked again (the lighting track's second phase).
//!
//! The stock lightmaps were baked by the original's radiosity, at a low
//! resolution and without a direction to light normal maps by. This crate
//! bakes them again from what the tags say of the light: the sky's lights
//! and ambient colours, and each shader's radiosity power and colour.
//!
//! - [`texel`] decodes a texture's smallest level for a material's mean
//!   colour.
//! - [`scene`] gathers a structure BSP's triangles, their materials and the
//!   sky's lights.
//! - [`bvh`] puts the triangles in a bounding-volume hierarchy, flattened
//!   as the GPU's path tracer reads it.
//! - [`layout`] holds scenes whose light is known, to check tracers by.
//! - [`trace`] is the path tracer: the light arriving at a point.
//! - [`chart`] lays the lightmap pages' texels out as points to bake.
//! - [`filter`] holds down the traced light's fireflies.
//! - [`probe`] places the irradiance and reflection probes.
//! - [`output`] makes the baked light from the tracer's results, and
//!   keeps it in a cache file.
//! - [`mood`] checks the baked light against the stock lightmaps'.
//! - [`seam`] finds the lightmap seams, stitches them and measures them.

pub mod bvh;
pub mod chart;
pub mod edge;
pub mod filter;
pub mod layout;
pub mod mood;
pub mod output;
pub mod probe;
pub mod scene;
pub mod seam;
pub mod texel;
pub mod trace;

pub use bvh::{Bvh, BvhNode, BvhTriangle, Hit, Ray};
pub use chart::{Atlas, BakePoint, Chart};
pub use mood::PageMood;
pub use output::{BakePlan, BakeSettings, BakedLighting, BakedPage, BakedSun};
pub use probe::ProbeGrid;
pub use scene::{BakeMaterial, DirectionalLight, Scene};
pub use trace::{Irradiance, Random, Tracer};
