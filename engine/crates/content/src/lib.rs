//! The engine's content: the typed data a map is made of, as the importer
//! writes it and the engine reads it. Nothing here points into a cache:
//! every index is into one of these types' own lists, and every list is
//! one the importer checked.

pub mod bitmap;
pub mod level;
pub mod pack;
pub mod shader;

pub use bitmap::{
    Bitmap, BitmapImage, Pixels, ResourceRange, Sequence, Sprite, Texture, TextureFormat,
};
pub use level::{
    CollisionBsp, CollisionMaterial, EnvironmentVertex, Level, Lightmap, LightmapVertex, Material,
    PlayerStart, Scenario, StructureBsp,
};
pub use shader::{ChicagoMap, EnvironmentShader, Shader, ShaderClass, TransparentChicagoShader};

use rkyv::{Archive, Deserialize, Serialize};
use std::fmt;

/// A tag group's four-character code (`sbsp`), first character first.
#[derive(Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord, Archive, Serialize, Deserialize)]
pub struct Group(pub [u8; 4]);

impl fmt::Display for Group {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0
            .iter()
            .try_for_each(|&byte| write!(f, "{}", char::from(byte).escape_default()))
    }
}

impl fmt::Debug for Group {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "'{self}'")
    }
}

/// A tag, named by its group and path (`bitm`,
/// `levels\test\bloodgulch\bitmaps\grass`).
#[derive(Clone, Debug, PartialEq, Eq, Hash, PartialOrd, Ord, Archive, Serialize, Deserialize)]
pub struct TagRef {
    /// Its group.
    pub group: Group,
    /// Its path.
    pub path: String,
}
