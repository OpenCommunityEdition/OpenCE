//! Why a cache was refused.

use crate::tags::GroupTag;

/// A cache that cannot be read, and where.
#[derive(Clone, Debug, PartialEq, Eq, thiserror::Error)]
pub enum ImportError {
    /// The cache's header, or the file around it.
    #[error("the cache's {field} {reason}")]
    Header {
        /// The field.
        field: &'static str,
        /// What is wrong with it.
        reason: String,
    },
    /// The tag index at the start of the tag data.
    #[error("the tag index's {field} {reason}")]
    Index {
        /// The field (and the tag it is in).
        field: String,
        /// What is wrong with it.
        reason: String,
    },
    /// A field of a tag.
    #[error("tag '{tag}' ({group}): {field} {reason}")]
    Tag {
        /// The tag's path.
        tag: String,
        /// The tag's group.
        group: GroupTag,
        /// The field, with the blocks it is in (`structure bsps[2].size`).
        field: String,
        /// What is wrong with it.
        reason: String,
    },
}
