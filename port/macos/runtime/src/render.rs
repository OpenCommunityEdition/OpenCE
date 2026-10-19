//! The renderer's commands (port/macos/include/opence_render.h): the batches
//! the guest's Direct3D 8 writes, decoded and checked before Metal sees them.
//!
//! A [`RenderValidator`] reads one batch at a time ([`RenderValidator::decode`])
//! and keeps what the batches before it made: each buffer's length, each
//! texture's size and format, each pipeline's vertex layout, the pass that is
//! open and what is bound in it. A command is passed on only if everything it
//! names exists and every region it touches lies inside its object and, for
//! guest addresses, inside the guest's mapped memory. Anything else is
//! refused ([`Rejection`]) and skipped, and the batch goes on, unless a
//! command's size runs past the batch's end.
//!
//! The batch is the guest's memory: the caller copies it out before decoding
//! (as the Swift app's [`crate::swift::RenderDecoder`] does), so that the guest
//! cannot change a command once it has been checked. What uploads read from
//! guest memory is only pixels and vertices; their ranges are checked here,
//! and guest memory is never unmapped while the guest lives.
//!
//! Indexed draws are checked against their index buffer, not the indices in
//! it: a wild index makes the GPU read past a vertex buffer, which Metal on
//! Apple silicon confines to the process's own GPU address space.

use std::collections::HashMap;

use crate::memory::{GuestMemory, GuestPtr, GuestRange};

#[cfg(test)]
mod tests;

pub const MAGIC: u32 = 0x4252_434f;
pub const VERSION: u32 = 1;
pub const BATCH_HEADER_WORDS: u32 = 4;

/// The ops, with the size in words of each (its minimum, for the two whose
/// commands carry a list).
pub mod op {
    pub const CREATE_BUFFER: u32 = 1;
    pub const UPLOAD_BUFFER: u32 = 2;
    pub const CREATE_TEXTURE: u32 = 3;
    pub const UPLOAD_TEXTURE: u32 = 4;
    pub const DESTROY: u32 = 5;
    pub const CREATE_LIBRARY: u32 = 6;
    pub const CREATE_PIPELINE: u32 = 7;
    pub const CREATE_DEPTH_STENCIL: u32 = 8;
    pub const CREATE_SAMPLER: u32 = 9;
    pub const BEGIN_PASS: u32 = 10;
    pub const END_PASS: u32 = 11;
    pub const SET_PIPELINE: u32 = 12;
    pub const SET_DEPTH_STENCIL: u32 = 13;
    pub const SET_RASTER: u32 = 14;
    pub const SET_VIEWPORT: u32 = 15;
    pub const SET_SCISSOR: u32 = 16;
    pub const SET_BLEND_COLOR: u32 = 17;
    pub const SET_VERTEX_BUFFER: u32 = 18;
    pub const SET_BYTES: u32 = 19;
    pub const SET_TEXTURE: u32 = 20;
    pub const SET_SAMPLER: u32 = 21;
    pub const DRAW: u32 = 22;
    pub const DRAW_INDEXED: u32 = 23;
    pub const SET_VISIBILITY: u32 = 24;
    pub const COPY_TEXTURE: u32 = 25;
    pub const READ_TEXTURE: u32 = 26;
    pub const GENERATE_MIPMAPS: u32 = 27;
    pub const PRESENT: u32 = 28;

    /// Each op's name in the header and its size in words.
    pub const SIZES: &[(u32, &str, u32)] = &[
        (CREATE_BUFFER, "CREATE_BUFFER", 3),
        (UPLOAD_BUFFER, "UPLOAD_BUFFER", 5),
        (CREATE_TEXTURE, "CREATE_TEXTURE", 10),
        (UPLOAD_TEXTURE, "UPLOAD_TEXTURE", 14),
        (DESTROY, "DESTROY", 3),
        (CREATE_LIBRARY, "CREATE_LIBRARY", 4),
        (CREATE_PIPELINE, "CREATE_PIPELINE", 17),
        (CREATE_DEPTH_STENCIL, "CREATE_DEPTH_STENCIL", 17),
        (CREATE_SAMPLER, "CREATE_SAMPLER", 12),
        (BEGIN_PASS, "BEGIN_PASS", 14),
        (END_PASS, "END_PASS", 1),
        (SET_PIPELINE, "SET_PIPELINE", 2),
        (SET_DEPTH_STENCIL, "SET_DEPTH_STENCIL", 3),
        (SET_RASTER, "SET_RASTER", 8),
        (SET_VIEWPORT, "SET_VIEWPORT", 7),
        (SET_SCISSOR, "SET_SCISSOR", 5),
        (SET_BLEND_COLOR, "SET_BLEND_COLOR", 5),
        (SET_VERTEX_BUFFER, "SET_VERTEX_BUFFER", 4),
        (SET_BYTES, "SET_BYTES", 4),
        (SET_TEXTURE, "SET_TEXTURE", 4),
        (SET_SAMPLER, "SET_SAMPLER", 4),
        (DRAW, "DRAW", 5),
        (DRAW_INDEXED, "DRAW_INDEXED", 8),
        (SET_VISIBILITY, "SET_VISIBILITY", 3),
        (COPY_TEXTURE, "COPY_TEXTURE", 13),
        (READ_TEXTURE, "READ_TEXTURE", 11),
        (GENERATE_MIPMAPS, "GENERATE_MIPMAPS", 2),
        (PRESENT, "PRESENT", 1),
    ];

    /// The size in words of a command of `op` (its fixed part).
    pub fn words(op: u32) -> Option<u32> {
        SIZES
            .iter()
            .find(|&&(code, _, _)| code == op)
            .map(|&(_, _, words)| words)
    }

    pub const PIPELINE_ATTRIBUTE_WORDS: u32 = 4;
    pub const PIPELINE_LAYOUT_WORDS: u32 = 4;
}

/// A batch's most bytes, its header included.
pub const MAX_BATCH_LENGTH: u32 = 0x100_0000;
pub const MAX_BUFFERS: u32 = 16384;
pub const MAX_TEXTURES: u32 = 8192;
pub const MAX_LIBRARIES: u32 = 4096;
pub const MAX_PIPELINES: u32 = 8192;
pub const MAX_DEPTH_STENCILS: u32 = 1024;
pub const MAX_SAMPLERS: u32 = 1024;
pub const MAX_BUFFER_LENGTH: u32 = 0x1000_0000;
pub const MAX_LIBRARY_LENGTH: u32 = 0x10_0000;
pub const MAX_TEXTURE_SIZE: u32 = 16384;
pub const MAX_TEXTURE_DEPTH: u32 = 2048;
pub const MAX_TEXTURE_LEVELS: u32 = 15;
pub const MAX_BYTES: u32 = 4096;
pub const BUFFER_SLOTS: u32 = 30;
pub const TEXTURE_SLOTS: u32 = 16;
pub const SAMPLER_SLOTS: u32 = 16;
pub const MAX_ATTRIBUTES: u32 = 16;

/// Texture usage bits.
pub const USAGE_SAMPLED: u32 = 1;
pub const USAGE_RENDER_TARGET: u32 = 2;

/// Colour write mask bits.
pub const WRITE_RED: u32 = 1;
pub const WRITE_GREEN: u32 = 2;
pub const WRITE_BLUE: u32 = 4;
pub const WRITE_ALPHA: u32 = 8;

/// The header's names of the constants above that are not ops or values,
/// for the test that checks them against it.
#[cfg(test)]
const HEADER_CONSTANTS: &[(&str, u32)] = &[
    ("MAGIC", MAGIC),
    ("VERSION", VERSION),
    ("BATCH_HEADER_WORDS", BATCH_HEADER_WORDS),
    ("PIPELINE_ATTRIBUTE_WORDS", op::PIPELINE_ATTRIBUTE_WORDS),
    ("PIPELINE_LAYOUT_WORDS", op::PIPELINE_LAYOUT_WORDS),
    ("MAX_BATCH_LENGTH", MAX_BATCH_LENGTH),
    ("MAX_BUFFERS", MAX_BUFFERS),
    ("MAX_TEXTURES", MAX_TEXTURES),
    ("MAX_LIBRARIES", MAX_LIBRARIES),
    ("MAX_PIPELINES", MAX_PIPELINES),
    ("MAX_DEPTH_STENCILS", MAX_DEPTH_STENCILS),
    ("MAX_SAMPLERS", MAX_SAMPLERS),
    ("MAX_BUFFER_LENGTH", MAX_BUFFER_LENGTH),
    ("MAX_LIBRARY_LENGTH", MAX_LIBRARY_LENGTH),
    ("MAX_TEXTURE_SIZE", MAX_TEXTURE_SIZE),
    ("MAX_TEXTURE_DEPTH", MAX_TEXTURE_DEPTH),
    ("MAX_TEXTURE_LEVELS", MAX_TEXTURE_LEVELS),
    ("MAX_BYTES", MAX_BYTES),
    ("BUFFER_SLOTS", BUFFER_SLOTS),
    ("TEXTURE_SLOTS", TEXTURE_SLOTS),
    ("SAMPLER_SLOTS", SAMPLER_SLOTS),
    ("MAX_ATTRIBUTES", MAX_ATTRIBUTES),
    ("USAGE_SAMPLED", USAGE_SAMPLED),
    ("USAGE_RENDER_TARGET", USAGE_RENDER_TARGET),
    ("WRITE_RED", WRITE_RED),
    ("WRITE_GREEN", WRITE_GREEN),
    ("WRITE_BLUE", WRITE_BLUE),
    ("WRITE_ALPHA", WRITE_ALPHA),
];

/// A field's values: an enum Swift sees, read from a word.
macro_rules! word_enum {
    ($(#[$meta:meta])* pub enum $name:ident {
        $($(#[$variant_meta:meta])* $variant:ident = $value:literal => $header:literal,)*
    }) => {
        $(#[$meta])*
        #[derive(Clone, Copy, PartialEq, Eq, Hash, Debug, uniffi::Enum)]
        pub enum $name {
            $($(#[$variant_meta])* $variant,)*
        }

        impl $name {
            pub fn from_word(word: u32) -> Option<$name> {
                match word {
                    $($value => Some($name::$variant),)*
                    _ => None,
                }
            }

            pub const fn word(self) -> u32 {
                match self {
                    $($name::$variant => $value,)*
                }
            }

            /// The header's name of each value.
            #[cfg(test)]
            const HEADER: &'static [(&'static str, u32)] = &[$(($header, $value),)*];
        }
    };
}

word_enum! {
    /// What DESTROY destroys.
    pub enum ObjectKind {
        Buffer = 1 => "OBJECT_BUFFER",
        Texture = 2 => "OBJECT_TEXTURE",
        Library = 3 => "OBJECT_LIBRARY",
        Pipeline = 4 => "OBJECT_PIPELINE",
        DepthStencil = 5 => "OBJECT_DEPTH_STENCIL",
        Sampler = 6 => "OBJECT_SAMPLER",
    }
}

word_enum! {
    pub enum TextureKind {
        Plane = 1 => "TEXTURE_2D",
        /// six slices
        Cube = 2 => "TEXTURE_CUBE",
        Volume = 3 => "TEXTURE_3D",
    }
}

word_enum! {
    pub enum PixelFormat {
        Rgba8 = 1 => "FORMAT_RGBA8",
        Bgra8 = 2 => "FORMAT_BGRA8",
        R8 = 3 => "FORMAT_R8",
        Rg8 = 4 => "FORMAT_RG8",
        B5g6r5 = 5 => "FORMAT_B5G6R5",
        Bgr5a1 = 6 => "FORMAT_BGR5A1",
        Abgr4 = 7 => "FORMAT_ABGR4",
        Bc1 = 8 => "FORMAT_BC1",
        Bc2 = 9 => "FORMAT_BC2",
        Bc3 = 10 => "FORMAT_BC3",
        Rgba16f = 11 => "FORMAT_RGBA16F",
        Depth32f = 12 => "FORMAT_DEPTH32F",
        Depth32fStencil8 = 13 => "FORMAT_DEPTH32F_STENCIL8",
    }
}

word_enum! {
    pub enum VertexFormat {
        Float = 1 => "VERTEX_FLOAT",
        Float2 = 2 => "VERTEX_FLOAT2",
        Float3 = 3 => "VERTEX_FLOAT3",
        Float4 = 4 => "VERTEX_FLOAT4",
        Uchar4Normalized = 5 => "VERTEX_UCHAR4_NORMALIZED",
        Uchar4NormalizedBgra = 6 => "VERTEX_UCHAR4_NORMALIZED_BGRA",
        Short2 = 7 => "VERTEX_SHORT2",
        Short4 = 8 => "VERTEX_SHORT4",
        Short2Normalized = 9 => "VERTEX_SHORT2_NORMALIZED",
        Short4Normalized = 10 => "VERTEX_SHORT4_NORMALIZED",
        Uchar4 = 11 => "VERTEX_UCHAR4",
        Uint = 12 => "VERTEX_UINT",
    }
}

word_enum! {
    pub enum Step {
        Vertex = 1 => "STEP_VERTEX",
        Instance = 2 => "STEP_INSTANCE",
        Constant = 3 => "STEP_CONSTANT",
    }
}

word_enum! {
    pub enum BlendFactor {
        Zero = 1 => "BLEND_ZERO",
        One = 2 => "BLEND_ONE",
        SourceColor = 3 => "BLEND_SOURCE_COLOR",
        OneMinusSourceColor = 4 => "BLEND_ONE_MINUS_SOURCE_COLOR",
        SourceAlpha = 5 => "BLEND_SOURCE_ALPHA",
        OneMinusSourceAlpha = 6 => "BLEND_ONE_MINUS_SOURCE_ALPHA",
        DestinationColor = 7 => "BLEND_DESTINATION_COLOR",
        OneMinusDestinationColor = 8 => "BLEND_ONE_MINUS_DESTINATION_COLOR",
        DestinationAlpha = 9 => "BLEND_DESTINATION_ALPHA",
        OneMinusDestinationAlpha = 10 => "BLEND_ONE_MINUS_DESTINATION_ALPHA",
        SourceAlphaSaturated = 11 => "BLEND_SOURCE_ALPHA_SATURATED",
        BlendColor = 12 => "BLEND_BLEND_COLOR",
        OneMinusBlendColor = 13 => "BLEND_ONE_MINUS_BLEND_COLOR",
        BlendAlpha = 14 => "BLEND_BLEND_ALPHA",
        OneMinusBlendAlpha = 15 => "BLEND_ONE_MINUS_BLEND_ALPHA",
    }
}

word_enum! {
    pub enum BlendOperation {
        Add = 1 => "BLEND_ADD",
        Subtract = 2 => "BLEND_SUBTRACT",
        ReverseSubtract = 3 => "BLEND_REVERSE_SUBTRACT",
        Min = 4 => "BLEND_MIN",
        Max = 5 => "BLEND_MAX",
    }
}

word_enum! {
    pub enum Compare {
        Never = 1 => "COMPARE_NEVER",
        Less = 2 => "COMPARE_LESS",
        Equal = 3 => "COMPARE_EQUAL",
        LessEqual = 4 => "COMPARE_LESS_EQUAL",
        Greater = 5 => "COMPARE_GREATER",
        NotEqual = 6 => "COMPARE_NOT_EQUAL",
        GreaterEqual = 7 => "COMPARE_GREATER_EQUAL",
        Always = 8 => "COMPARE_ALWAYS",
    }
}

word_enum! {
    pub enum StencilOperation {
        Keep = 1 => "STENCIL_KEEP",
        Zero = 2 => "STENCIL_ZERO",
        Replace = 3 => "STENCIL_REPLACE",
        IncrementClamp = 4 => "STENCIL_INCREMENT_CLAMP",
        DecrementClamp = 5 => "STENCIL_DECREMENT_CLAMP",
        Invert = 6 => "STENCIL_INVERT",
        IncrementWrap = 7 => "STENCIL_INCREMENT_WRAP",
        DecrementWrap = 8 => "STENCIL_DECREMENT_WRAP",
    }
}

word_enum! {
    /// A sampler's filter; None only for its mipmaps.
    pub enum Filter {
        None = 0 => "FILTER_NONE",
        Nearest = 1 => "FILTER_NEAREST",
        Linear = 2 => "FILTER_LINEAR",
    }
}

word_enum! {
    pub enum Address {
        Repeat = 1 => "ADDRESS_REPEAT",
        Mirror = 2 => "ADDRESS_MIRROR",
        ClampToEdge = 3 => "ADDRESS_CLAMP_TO_EDGE",
        ClampToBorder = 4 => "ADDRESS_CLAMP_TO_BORDER",
    }
}

word_enum! {
    pub enum Border {
        TransparentBlack = 1 => "BORDER_TRANSPARENT_BLACK",
        OpaqueBlack = 2 => "BORDER_OPAQUE_BLACK",
        OpaqueWhite = 3 => "BORDER_OPAQUE_WHITE",
    }
}

word_enum! {
    pub enum Load {
        DontCare = 1 => "LOAD_DONT_CARE",
        Load = 2 => "LOAD_LOAD",
        Clear = 3 => "LOAD_CLEAR",
    }
}

word_enum! {
    pub enum Cull {
        None = 1 => "CULL_NONE",
        Front = 2 => "CULL_FRONT",
        Back = 3 => "CULL_BACK",
    }
}

word_enum! {
    pub enum Winding {
        Clockwise = 1 => "WINDING_CLOCKWISE",
        CounterClockwise = 2 => "WINDING_COUNTER_CLOCKWISE",
    }
}

word_enum! {
    pub enum Fill {
        Solid = 1 => "FILL_SOLID",
        Lines = 2 => "FILL_LINES",
    }
}

word_enum! {
    pub enum Stage {
        Vertex = 1 => "STAGE_VERTEX",
        Fragment = 2 => "STAGE_FRAGMENT",
    }
}

word_enum! {
    pub enum Primitive {
        Point = 1 => "PRIMITIVE_POINT",
        Line = 2 => "PRIMITIVE_LINE",
        LineStrip = 3 => "PRIMITIVE_LINE_STRIP",
        Triangle = 4 => "PRIMITIVE_TRIANGLE",
        TriangleStrip = 5 => "PRIMITIVE_TRIANGLE_STRIP",
    }
}

word_enum! {
    pub enum IndexType {
        Uint16 = 1 => "INDEX_UINT16",
        Uint32 = 2 => "INDEX_UINT32",
    }
}

word_enum! {
    pub enum Visibility {
        Disabled = 1 => "VISIBILITY_DISABLED",
        Boolean = 2 => "VISIBILITY_BOOLEAN",
        Counting = 3 => "VISIBILITY_COUNTING",
    }
}

impl PixelFormat {
    /// The width and height in pixels of a block, and its size in bytes.
    pub fn block(self) -> (u32, u32) {
        match self {
            PixelFormat::R8 => (1, 1),
            PixelFormat::Rg8 | PixelFormat::B5g6r5 | PixelFormat::Bgr5a1 | PixelFormat::Abgr4 => {
                (1, 2)
            }
            PixelFormat::Rgba8 | PixelFormat::Bgra8 | PixelFormat::Depth32f => (1, 4),
            PixelFormat::Rgba16f | PixelFormat::Depth32fStencil8 => (1, 8),
            PixelFormat::Bc1 => (4, 8),
            PixelFormat::Bc2 | PixelFormat::Bc3 => (4, 16),
        }
    }

    pub fn is_depth(self) -> bool {
        matches!(self, PixelFormat::Depth32f | PixelFormat::Depth32fStencil8)
    }

    pub fn is_compressed(self) -> bool {
        matches!(self, PixelFormat::Bc1 | PixelFormat::Bc2 | PixelFormat::Bc3)
    }

    /// Whether it can be a pass's colour target.
    pub fn is_color_target(self) -> bool {
        !self.is_depth() && !self.is_compressed()
    }
}

impl VertexFormat {
    /// Its size in bytes.
    pub fn size(self) -> u32 {
        match self {
            VertexFormat::Float
            | VertexFormat::Uchar4Normalized
            | VertexFormat::Uchar4NormalizedBgra
            | VertexFormat::Short2
            | VertexFormat::Short2Normalized
            | VertexFormat::Uchar4
            | VertexFormat::Uint => 4,
            VertexFormat::Float2 | VertexFormat::Short4 | VertexFormat::Short4Normalized => 8,
            VertexFormat::Float3 => 12,
            VertexFormat::Float4 => 16,
        }
    }
}

impl IndexType {
    pub fn size(self) -> u32 {
        match self {
            IndexType::Uint16 => 2,
            IndexType::Uint32 => 4,
        }
    }
}

// ---------- the commands, as checked

/// A pipeline's blending.
#[derive(Clone, Copy, PartialEq, Eq, Debug, uniffi::Record)]
pub struct Blend {
    pub enabled: bool,
    pub source_color: BlendFactor,
    pub destination_color: BlendFactor,
    pub color_operation: BlendOperation,
    pub source_alpha: BlendFactor,
    pub destination_alpha: BlendFactor,
    pub alpha_operation: BlendOperation,
}

/// A vertex attribute: where shader input `index` comes from.
#[derive(Clone, Copy, PartialEq, Eq, Debug, uniffi::Record)]
pub struct VertexAttribute {
    pub index: u32,
    pub format: VertexFormat,
    pub offset: u32,
    pub slot: u32,
}

/// How a vertex buffer slot steps.
#[derive(Clone, Copy, PartialEq, Eq, Debug, uniffi::Record)]
pub struct VertexLayout {
    pub slot: u32,
    pub stride: u32,
    pub step: Step,
    pub step_rate: u32,
}

/// One side's stencil.
#[derive(Clone, Copy, PartialEq, Eq, Debug, uniffi::Record)]
pub struct StencilFace {
    pub fail: StencilOperation,
    pub depth_fail: StencilOperation,
    pub pass: StencilOperation,
    pub compare: Compare,
    pub read_mask: u32,
    pub write_mask: u32,
}

/// A command that passed its checks. Ids are the guest's; 0 is none (and,
/// as a pass's colour, the window).
#[derive(Clone, PartialEq, Debug, uniffi::Enum)]
pub enum Command {
    CreateBuffer {
        id: u32,
        length: u32,
    },
    UploadBuffer {
        id: u32,
        offset: u32,
        address: u32,
        length: u32,
    },
    CreateTexture {
        id: u32,
        kind: TextureKind,
        format: PixelFormat,
        width: u32,
        height: u32,
        depth: u32,
        levels: u32,
        sampled: bool,
        render_target: bool,
        samples: u32,
    },
    UploadTexture {
        id: u32,
        level: u32,
        slice: u32,
        x: u32,
        y: u32,
        z: u32,
        width: u32,
        height: u32,
        depth: u32,
        address: u32,
        bytes_per_row: u32,
        bytes_per_image: u32,
        length: u32,
    },
    Destroy {
        kind: ObjectKind,
        id: u32,
    },
    CreateLibrary {
        id: u32,
        address: u32,
        length: u32,
    },
    CreatePipeline {
        id: u32,
        vertex_library: u32,
        fragment_library: u32,
        color_format: PixelFormat,
        depth_format: Option<PixelFormat>,
        samples: u32,
        blend: Blend,
        write_mask: u32,
        attributes: Vec<VertexAttribute>,
        layouts: Vec<VertexLayout>,
    },
    CreateDepthStencil {
        id: u32,
        depth_compare: Compare,
        depth_write: bool,
        stencil: bool,
        front: StencilFace,
        back: StencilFace,
    },
    CreateSampler {
        id: u32,
        min_filter: Filter,
        mag_filter: Filter,
        mip_filter: Filter,
        address_u: Address,
        address_v: Address,
        address_w: Address,
        max_anisotropy: u32,
        min_lod: f32,
        max_lod: f32,
        border: Border,
    },
    /// `width` and `height` are the targets'.
    BeginPass {
        color: u32,
        depth: u32,
        color_load: Load,
        depth_load: Load,
        stencil_load: Load,
        clear_red: f32,
        clear_green: f32,
        clear_blue: f32,
        clear_alpha: f32,
        clear_depth: f32,
        clear_stencil: u32,
        resolve: u32,
        visibility: u32,
        width: u32,
        height: u32,
    },
    EndPass,
    SetPipeline {
        id: u32,
    },
    SetDepthStencil {
        id: u32,
        reference: u32,
    },
    SetRaster {
        cull: Cull,
        winding: Winding,
        fill: Fill,
        depth_bias: f32,
        slope_scale: f32,
        clamp: f32,
        clamp_depth: bool,
    },
    SetViewport {
        x: f32,
        y: f32,
        width: f32,
        height: f32,
        near: f32,
        far: f32,
    },
    /// Clipped to the pass's targets.
    SetScissor {
        x: u32,
        y: u32,
        width: u32,
        height: u32,
    },
    SetBlendColor {
        red: f32,
        green: f32,
        blue: f32,
        alpha: f32,
    },
    SetVertexBuffer {
        slot: u32,
        id: u32,
        offset: u32,
    },
    SetBytes {
        stage: Stage,
        slot: u32,
        bytes: Vec<u8>,
    },
    SetTexture {
        stage: Stage,
        slot: u32,
        id: u32,
    },
    SetSampler {
        stage: Stage,
        slot: u32,
        id: u32,
    },
    Draw {
        primitive: Primitive,
        first: u32,
        vertices: u32,
        instances: u32,
    },
    DrawIndexed {
        primitive: Primitive,
        indices: u32,
        index_type: IndexType,
        buffer: u32,
        offset: u32,
        base_vertex: i32,
        instances: u32,
    },
    SetVisibility {
        mode: Visibility,
        offset: u32,
    },
    CopyTexture {
        source: u32,
        source_level: u32,
        source_slice: u32,
        source_x: u32,
        source_y: u32,
        destination: u32,
        destination_level: u32,
        destination_slice: u32,
        destination_x: u32,
        destination_y: u32,
        width: u32,
        height: u32,
    },
    ReadTexture {
        id: u32,
        level: u32,
        slice: u32,
        x: u32,
        y: u32,
        width: u32,
        height: u32,
        address: u32,
        bytes_per_row: u32,
        length: u32,
    },
    GenerateMipmaps {
        id: u32,
    },
    Present,
}

/// Why a command was refused.
#[derive(Clone, PartialEq, Eq, Debug, thiserror::Error, uniffi::Enum)]
pub enum RenderError {
    #[error("a command of {words} words, with {left} left in the batch")]
    Truncated { words: u32, left: u32 },
    #[error("no such op")]
    UnknownOp,
    #[error("{words} words, not {expected}")]
    Size { words: u32, expected: u32 },
    #[error("{kind:?} id {id} is out of range")]
    Id { kind: ObjectKind, id: u32 },
    #[error("{kind:?} {id} exists already")]
    Exists { kind: ObjectKind, id: u32 },
    #[error("{kind:?} {id} does not exist")]
    Missing { kind: ObjectKind, id: u32 },
    #[error("{field} cannot be {value}")]
    Value { field: String, value: u32 },
    #[error("{field} is not a usable number")]
    Float { field: String },
    #[error("{length:#x} bytes at {address:#010x} are not the guest's mapped memory")]
    Guest { address: u32, length: u64 },
    #[error("{field} lies outside its object")]
    Bounds { field: String },
    #[error("{field} does not match")]
    Mismatch { field: String },
    #[error("only inside a pass")]
    OutsidePass,
    #[error("only outside a pass")]
    InsidePass,
    #[error("no pipeline is set")]
    NoPipeline,
    #[error("vertex buffer slot {slot} is empty")]
    Unbound { slot: u32 },
    #[error("the window has no size yet")]
    NoWindow,
    #[error("the batch said {said} commands, and has {found}")]
    Count { said: u32, found: u32 },
}

/// A command refused: its place in the batch (bytes), its op and why.
#[derive(Clone, PartialEq, Debug, uniffi::Record)]
pub struct Rejection {
    pub offset: u32,
    pub op: u32,
    pub error: RenderError,
    /// the error as text, for the log
    pub reason: String,
}

/// A batch's commands that passed, and those refused.
#[derive(Clone, PartialEq, Debug, Default, uniffi::Record)]
pub struct DecodedBatch {
    pub commands: Vec<Command>,
    pub rejected: Vec<Rejection>,
}

/// Why a whole batch was refused.
#[derive(Clone, PartialEq, Eq, Debug, thiserror::Error, uniffi::Error)]
pub enum BatchError {
    #[error("{size} bytes are not a batch (a header of 16 bytes and whole words, at most 16 MB)")]
    Size { size: u64 },
    #[error("the batch starts {magic:#010x}, not OCRB")]
    Magic { magic: u32 },
    #[error("the batch is version {version}; this host reads {VERSION}")]
    Version { version: u32 },
    #[error("the batch says it is {said} bytes, and is {size}")]
    Length { said: u32, size: u64 },
    #[error("the batch cannot be read from the guest: {reason}")]
    Guest { reason: String },
}

/// The guest memory a command may name.
pub trait GuestRanges {
    /// Whether all `length` bytes at `address` are mapped.
    fn is_mapped(&self, address: u32, length: u64) -> bool;
}

impl GuestRanges for GuestMemory {
    fn is_mapped(&self, address: u32, length: u64) -> bool {
        GuestRange::new(GuestPtr(address), length)
            .is_ok_and(|range| GuestMemory::is_mapped(self, range))
    }
}

/// The `size` bytes of the batch at guest `address`, copied out of
/// `memory` for [`RenderValidator::decode`].
pub fn copy_batch(memory: &GuestMemory, address: u32, size: u32) -> Result<Vec<u8>, BatchError> {
    if size > MAX_BATCH_LENGTH {
        return Err(BatchError::Size {
            size: u64::from(size),
        });
    }
    let mut bytes = vec![0; size as usize];
    memory
        .read(GuestPtr(address), &mut bytes)
        .map_err(|error| BatchError::Guest {
            reason: error.to_string(),
        })?;
    Ok(bytes)
}

// ---------- the validator

#[derive(Clone, Copy, Debug)]
struct Texture {
    kind: TextureKind,
    format: PixelFormat,
    width: u32,
    height: u32,
    depth: u32,
    levels: u32,
    usage: u32,
    samples: u32,
}

#[derive(Clone, Debug)]
struct Pipeline {
    color_format: PixelFormat,
    depth_format: Option<PixelFormat>,
    samples: u32,
    attributes: Vec<VertexAttribute>,
    layouts: Vec<VertexLayout>,
}

/// The open pass's targets.
#[derive(Clone, Copy, Debug)]
struct Pass {
    color: u32,
    depth: u32,
    resolve: u32,
    color_format: PixelFormat,
    depth_format: Option<PixelFormat>,
    samples: u32,
    width: u32,
    height: u32,
    /// the visibility buffer and its length
    visibility: Option<(u32, u32)>,
}

/// What the open pass has bound.
#[derive(Clone, Debug, Default)]
struct Bound {
    pipeline: Option<u32>,
    vertex_buffers: [Option<(u32, u32)>; BUFFER_SLOTS as usize],
}

/// Decodes and checks batches, keeping what they made (module docs).
#[derive(Debug, Default)]
pub struct RenderValidator {
    buffers: HashMap<u32, u32>,
    textures: HashMap<u32, Texture>,
    libraries: HashMap<u32, ()>,
    pipelines: HashMap<u32, Pipeline>,
    depth_stencils: HashMap<u32, ()>,
    samplers: HashMap<u32, ()>,
    pass: Option<Pass>,
    bound: Bound,
    drawable: (u32, u32),
}

impl RenderValidator {
    pub fn new() -> RenderValidator {
        RenderValidator::default()
    }

    /// The window's size in pixels, which passes on colour 0 draw to.
    pub fn set_drawable_size(&mut self, width: u32, height: u32) {
        self.drawable = (width, height);
    }

    /// Whether a pass is open (one may span batches).
    pub fn in_pass(&self) -> bool {
        self.pass.is_some()
    }

    /// Decodes and checks the batch `bytes`, which names guest memory in
    /// `memory`.
    pub fn decode(
        &mut self,
        bytes: &[u8],
        memory: &impl GuestRanges,
    ) -> Result<DecodedBatch, BatchError> {
        let size = bytes.len() as u64;
        if !bytes.len().is_multiple_of(4)
            || bytes.len() < (BATCH_HEADER_WORDS * 4) as usize
            || size > u64::from(MAX_BATCH_LENGTH)
        {
            return Err(BatchError::Size { size });
        }
        let words: Vec<u32> = bytes
            .as_chunks::<4>()
            .0
            .iter()
            .map(|&word| u32::from_le_bytes(word))
            .collect();
        if words[0] != MAGIC {
            return Err(BatchError::Magic { magic: words[0] });
        }
        if words[1] != VERSION {
            return Err(BatchError::Version { version: words[1] });
        }
        if u64::from(words[2]) != size {
            return Err(BatchError::Length {
                said: words[2],
                size,
            });
        }
        let said = words[3];
        let mut batch = DecodedBatch::default();
        let mut at = BATCH_HEADER_WORDS as usize;
        let mut found = 0;
        while at < words.len() {
            let word = words[at];
            let op = word & 0xffff;
            let length = (word >> 16) as usize;
            let offset = (at * 4) as u32;
            let left = words.len() - at;
            if length == 0 || length > left {
                // (nothing after it can be found)
                batch.rejected.push(rejection(
                    offset,
                    op,
                    RenderError::Truncated {
                        words: length as u32,
                        left: left as u32,
                    },
                ));
                return Ok(batch);
            }
            found += 1;
            match self.command(op, &words[at + 1..at + length], memory) {
                Ok(Some(command)) => batch.commands.push(command),
                Ok(None) => {}
                Err(error) => batch.rejected.push(rejection(offset, op, error)),
            }
            at += length;
        }
        if found != said {
            batch.rejected.push(rejection(
                bytes.len() as u32,
                0,
                RenderError::Count { said, found },
            ));
        }
        Ok(batch)
    }

    /// One command's fields (after its first word), checked; None for one
    /// with nothing to do.
    fn command(
        &mut self,
        code: u32,
        fields: &[u32],
        memory: &impl GuestRanges,
    ) -> Result<Option<Command>, RenderError> {
        let expected = op::words(code).ok_or(RenderError::UnknownOp)?;
        let words = fields.len() as u32 + 1;
        let listed = matches!(code, op::CREATE_PIPELINE | op::SET_BYTES);
        if words < expected || (!listed && words != expected) {
            return Err(RenderError::Size { words, expected });
        }
        let inside = matches!(
            code,
            op::END_PASS
                | op::SET_PIPELINE
                | op::SET_DEPTH_STENCIL
                | op::SET_RASTER
                | op::SET_VIEWPORT
                | op::SET_SCISSOR
                | op::SET_BLEND_COLOR
                | op::SET_VERTEX_BUFFER
                | op::SET_BYTES
                | op::SET_TEXTURE
                | op::SET_SAMPLER
                | op::DRAW
                | op::DRAW_INDEXED
                | op::SET_VISIBILITY
        );
        let outside = matches!(
            code,
            op::BEGIN_PASS
                | op::COPY_TEXTURE
                | op::READ_TEXTURE
                | op::GENERATE_MIPMAPS
                | op::PRESENT
        );
        if inside && self.pass.is_none() {
            return Err(RenderError::OutsidePass);
        }
        if outside && self.pass.is_some() {
            return Err(RenderError::InsidePass);
        }
        let f = fields;
        let command = match code {
            op::CREATE_BUFFER => self.create_buffer(f)?,
            op::UPLOAD_BUFFER => self.upload_buffer(f, memory)?,
            op::CREATE_TEXTURE => self.create_texture(f)?,
            op::UPLOAD_TEXTURE => self.upload_texture(f, memory)?,
            op::DESTROY => self.destroy(f)?,
            op::CREATE_LIBRARY => self.create_library(f, memory)?,
            op::CREATE_PIPELINE => self.create_pipeline(f)?,
            op::CREATE_DEPTH_STENCIL => self.create_depth_stencil(f)?,
            op::CREATE_SAMPLER => self.create_sampler(f)?,
            op::BEGIN_PASS => self.begin_pass(f)?,
            op::END_PASS => {
                self.pass = None;
                self.bound = Bound::default();
                Command::EndPass
            }
            op::SET_PIPELINE => self.set_pipeline(f)?,
            op::SET_DEPTH_STENCIL => {
                self.optional(ObjectKind::DepthStencil, f[0])?;
                if f[1] > 0xff {
                    return Err(value("stencil reference", f[1]));
                }
                Command::SetDepthStencil {
                    id: f[0],
                    reference: f[1],
                }
            }
            op::SET_RASTER => Command::SetRaster {
                cull: read("cull", f[0], Cull::from_word)?,
                winding: read("winding", f[1], Winding::from_word)?,
                fill: read("fill", f[2], Fill::from_word)?,
                depth_bias: finite("depth bias", f[3])?,
                slope_scale: finite("depth bias", f[4])?,
                clamp: finite("depth bias", f[5])?,
                clamp_depth: flag("depth clip", f[6])?,
            },
            op::SET_VIEWPORT => {
                let mut v = [0.0; 6];
                for (number, &word) in v.iter_mut().zip(f) {
                    *number = finite("viewport", word)?;
                }
                if !(0.0..=1.0).contains(&v[4]) || !(0.0..=1.0).contains(&v[5]) {
                    return Err(float("viewport"));
                }
                Command::SetViewport {
                    x: v[0],
                    y: v[1],
                    width: v[2],
                    height: v[3],
                    near: v[4],
                    far: v[5],
                }
            }
            op::SET_SCISSOR => {
                let pass = self.pass.expect("inside a pass");
                let x = f[0].min(pass.width);
                let y = f[1].min(pass.height);
                Command::SetScissor {
                    x,
                    y,
                    width: f[2].min(pass.width - x),
                    height: f[3].min(pass.height - y),
                }
            }
            op::SET_BLEND_COLOR => Command::SetBlendColor {
                red: finite("blend colour", f[0])?,
                green: finite("blend colour", f[1])?,
                blue: finite("blend colour", f[2])?,
                alpha: finite("blend colour", f[3])?,
            },
            op::SET_VERTEX_BUFFER => self.set_vertex_buffer(f)?,
            op::SET_BYTES => {
                let stage = read("stage", f[0], Stage::from_word)?;
                slot(f[1], BUFFER_SLOTS)?;
                let length = f[2];
                if length == 0 || length > MAX_BYTES {
                    return Err(value("length", length));
                }
                let expected = expected + length.div_ceil(4);
                if words != expected {
                    return Err(RenderError::Size { words, expected });
                }
                let mut bytes: Vec<u8> =
                    f[3..].iter().flat_map(|word| word.to_le_bytes()).collect();
                bytes.truncate(length as usize);
                Command::SetBytes {
                    stage,
                    slot: f[1],
                    bytes,
                }
            }
            op::SET_TEXTURE => {
                let stage = read("stage", f[0], Stage::from_word)?;
                slot(f[1], TEXTURE_SLOTS)?;
                if f[2] != 0 {
                    let texture = self.texture(f[2])?;
                    if texture.usage & USAGE_SAMPLED == 0 {
                        return Err(value("texture", f[2]));
                    }
                    let pass = self.pass.expect("inside a pass");
                    if [pass.color, pass.depth, pass.resolve].contains(&f[2]) {
                        return Err(mismatch("texture is the pass's target"));
                    }
                }
                Command::SetTexture {
                    stage,
                    slot: f[1],
                    id: f[2],
                }
            }
            op::SET_SAMPLER => {
                let stage = read("stage", f[0], Stage::from_word)?;
                slot(f[1], SAMPLER_SLOTS)?;
                self.optional(ObjectKind::Sampler, f[2])?;
                Command::SetSampler {
                    stage,
                    slot: f[1],
                    id: f[2],
                }
            }
            op::DRAW => match self.draw(f)? {
                Some(command) => command,
                None => return Ok(None),
            },
            op::DRAW_INDEXED => match self.draw_indexed(f)? {
                Some(command) => command,
                None => return Ok(None),
            },
            op::SET_VISIBILITY => {
                let mode = read("visibility", f[0], Visibility::from_word)?;
                if mode != Visibility::Disabled {
                    let length = self
                        .pass
                        .and_then(|pass| pass.visibility)
                        .map(|(_, length)| length)
                        .ok_or(value("visibility", f[0]))?;
                    if !f[1].is_multiple_of(8) {
                        return Err(value("visibility offset", f[1]));
                    }
                    within("visibility offset", f[1], 8, length)?;
                }
                Command::SetVisibility { mode, offset: f[1] }
            }
            op::COPY_TEXTURE => self.copy_texture(f)?,
            op::READ_TEXTURE => self.read_texture(f, memory)?,
            op::GENERATE_MIPMAPS => {
                let texture = self.texture(f[0])?;
                if texture.levels < 2 {
                    return Err(value("levels", texture.levels));
                }
                if texture.samples > 1
                    || texture.format.is_compressed()
                    || texture.format.is_depth()
                {
                    return Err(value("format", texture.format.word()));
                }
                Command::GenerateMipmaps { id: f[0] }
            }
            op::PRESENT => Command::Present,
            _ => unreachable!("every op in op::SIZES is handled"),
        };
        Ok(Some(command))
    }

    // ---------- objects

    fn has(&self, kind: ObjectKind, id: u32) -> bool {
        match kind {
            ObjectKind::Buffer => self.buffers.contains_key(&id),
            ObjectKind::Texture => self.textures.contains_key(&id),
            ObjectKind::Library => self.libraries.contains_key(&id),
            ObjectKind::Pipeline => self.pipelines.contains_key(&id),
            ObjectKind::DepthStencil => self.depth_stencils.contains_key(&id),
            ObjectKind::Sampler => self.samplers.contains_key(&id),
        }
    }

    /// `id` for an object to be made.
    fn fresh(&self, kind: ObjectKind, id: u32) -> Result<(), RenderError> {
        check_id(kind, id)?;
        if self.has(kind, id) {
            return Err(RenderError::Exists { kind, id });
        }
        Ok(())
    }

    /// `id` for an object that must exist.
    fn existing(&self, kind: ObjectKind, id: u32) -> Result<(), RenderError> {
        check_id(kind, id)?;
        if !self.has(kind, id) {
            return Err(RenderError::Missing { kind, id });
        }
        Ok(())
    }

    /// `id` for an object that must exist, or 0.
    fn optional(&self, kind: ObjectKind, id: u32) -> Result<(), RenderError> {
        if id == 0 {
            return Ok(());
        }
        self.existing(kind, id)
    }

    fn buffer(&self, id: u32) -> Result<u32, RenderError> {
        self.existing(ObjectKind::Buffer, id)?;
        Ok(self.buffers[&id])
    }

    fn texture(&self, id: u32) -> Result<Texture, RenderError> {
        self.existing(ObjectKind::Texture, id)?;
        Ok(self.textures[&id])
    }

    fn create_buffer(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        self.fresh(ObjectKind::Buffer, f[0])?;
        if f[1] == 0 || f[1] > MAX_BUFFER_LENGTH {
            return Err(value("length", f[1]));
        }
        self.buffers.insert(f[0], f[1]);
        Ok(Command::CreateBuffer {
            id: f[0],
            length: f[1],
        })
    }

    fn upload_buffer(&self, f: &[u32], memory: &impl GuestRanges) -> Result<Command, RenderError> {
        let length = self.buffer(f[0])?;
        if f[3] == 0 {
            return Err(value("length", 0));
        }
        within("upload", f[1], f[3], length)?;
        guest(memory, f[2], u64::from(f[3]))?;
        Ok(Command::UploadBuffer {
            id: f[0],
            offset: f[1],
            address: f[2],
            length: f[3],
        })
    }

    fn create_texture(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        self.fresh(ObjectKind::Texture, f[0])?;
        let kind = read("kind", f[1], TextureKind::from_word)?;
        let format = read("format", f[2], PixelFormat::from_word)?;
        let (width, height, depth, levels, usage, samples) = (f[3], f[4], f[5], f[6], f[7], f[8]);
        if !(1..=MAX_TEXTURE_SIZE).contains(&width) {
            return Err(value("width", width));
        }
        if !(1..=MAX_TEXTURE_SIZE).contains(&height) {
            return Err(value("height", height));
        }
        let most_depth = if kind == TextureKind::Volume {
            MAX_TEXTURE_DEPTH
        } else {
            1
        };
        if !(1..=most_depth).contains(&depth) {
            return Err(value("depth", depth));
        }
        if kind == TextureKind::Cube && width != height {
            return Err(mismatch("cube"));
        }
        if usage == 0 || usage & !(USAGE_SAMPLED | USAGE_RENDER_TARGET) != 0 {
            return Err(value("usage", usage));
        }
        let render_target = usage & USAGE_RENDER_TARGET != 0;
        let largest = width.max(height).max(depth);
        let most_levels = (32 - largest.leading_zeros()).min(MAX_TEXTURE_LEVELS);
        if !(1..=most_levels).contains(&levels) {
            return Err(value("levels", levels));
        }
        if ![1, 2, 4, 8].contains(&samples)
            || (samples > 1 && (kind != TextureKind::Plane || levels != 1 || !render_target))
        {
            return Err(value("samples", samples));
        }
        if format.is_compressed() {
            if render_target {
                return Err(value("usage", usage));
            }
            if kind == TextureKind::Volume {
                return Err(value("format", f[2]));
            }
        }
        if format.is_depth() {
            if kind != TextureKind::Plane {
                return Err(value("format", f[2]));
            }
            if !render_target {
                return Err(value("usage", usage));
            }
            if levels != 1 {
                return Err(value("levels", levels));
            }
        }
        self.textures.insert(
            f[0],
            Texture {
                kind,
                format,
                width,
                height,
                depth,
                levels,
                usage,
                samples,
            },
        );
        Ok(Command::CreateTexture {
            id: f[0],
            kind,
            format,
            width,
            height,
            depth,
            levels,
            sampled: usage & USAGE_SAMPLED != 0,
            render_target,
            samples,
        })
    }

    fn upload_texture(&self, f: &[u32], memory: &impl GuestRanges) -> Result<Command, RenderError> {
        let texture = self.texture(f[0])?;
        let [_, level, slice, x, y, z, width, height, depth, address, bytes_per_row, bytes_per_image, length] =
            *<&[u32; 13]>::try_from(f).expect("the size is checked");
        if texture.samples > 1 {
            return Err(value("samples", texture.samples));
        }
        if texture.format.is_depth() {
            return Err(value("format", texture.format.word()));
        }
        let size = level_size(&texture, level)?;
        let slices = if texture.kind == TextureKind::Cube {
            6
        } else {
            1
        };
        if slice >= slices {
            return Err(value("slice", slice));
        }
        if texture.kind != TextureKind::Volume && z != 0 {
            return Err(value("z", z));
        }
        if width == 0 {
            return Err(value("width", width));
        }
        if height == 0 {
            return Err(value("height", height));
        }
        if depth == 0 || (texture.kind != TextureKind::Volume && depth != 1) {
            return Err(value("depth", depth));
        }
        within("region", x, width, size.0)?;
        within("region", y, height, size.1)?;
        within("region", z, depth, size.2)?;
        let (rows, row_bytes) = blocks(texture.format, (x, y, width, height), size)?;
        if u64::from(bytes_per_row) < row_bytes {
            return Err(value("bytes per row", bytes_per_row));
        }
        let image = u64::from(bytes_per_row) * (rows - 1) + row_bytes;
        let needed = if depth > 1 {
            if u64::from(bytes_per_image) < u64::from(bytes_per_row) * rows {
                return Err(value("bytes per image", bytes_per_image));
            }
            u64::from(bytes_per_image) * u64::from(depth - 1) + image
        } else {
            image
        };
        if u64::from(length) < needed {
            return Err(value("length", length));
        }
        guest(memory, address, u64::from(length))?;
        Ok(Command::UploadTexture {
            id: f[0],
            level,
            slice,
            x,
            y,
            z,
            width,
            height,
            depth,
            address,
            bytes_per_row,
            bytes_per_image,
            length,
        })
    }

    fn destroy(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        let kind = read("object kind", f[0], ObjectKind::from_word)?;
        let id = f[1];
        self.existing(kind, id)?;
        if let Some(pass) = self.pass {
            let target = match kind {
                ObjectKind::Texture => [pass.color, pass.depth, pass.resolve].contains(&id),
                ObjectKind::Buffer => pass.visibility.is_some_and(|(buffer, _)| buffer == id),
                _ => false,
            };
            if target {
                return Err(RenderError::InsidePass);
            }
        }
        match kind {
            ObjectKind::Buffer => {
                self.buffers.remove(&id);
                for binding in &mut self.bound.vertex_buffers {
                    if binding.is_some_and(|(buffer, _)| buffer == id) {
                        *binding = None;
                    }
                }
            }
            ObjectKind::Texture => {
                self.textures.remove(&id);
            }
            ObjectKind::Library => {
                self.libraries.remove(&id);
            }
            ObjectKind::Pipeline => {
                self.pipelines.remove(&id);
                if self.bound.pipeline == Some(id) {
                    self.bound.pipeline = None;
                }
            }
            ObjectKind::DepthStencil => {
                self.depth_stencils.remove(&id);
            }
            ObjectKind::Sampler => {
                self.samplers.remove(&id);
            }
        }
        Ok(Command::Destroy { kind, id })
    }

    fn create_library(
        &mut self,
        f: &[u32],
        memory: &impl GuestRanges,
    ) -> Result<Command, RenderError> {
        self.fresh(ObjectKind::Library, f[0])?;
        if f[2] == 0 || f[2] > MAX_LIBRARY_LENGTH {
            return Err(value("length", f[2]));
        }
        guest(memory, f[1], u64::from(f[2]))?;
        self.libraries.insert(f[0], ());
        Ok(Command::CreateLibrary {
            id: f[0],
            address: f[1],
            length: f[2],
        })
    }

    fn create_pipeline(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        self.fresh(ObjectKind::Pipeline, f[0])?;
        self.existing(ObjectKind::Library, f[1])?;
        self.optional(ObjectKind::Library, f[2])?;
        let color_format = read("colour format", f[3], PixelFormat::from_word)
            .ok()
            .filter(|format| format.is_color_target())
            .ok_or(value("colour format", f[3]))?;
        let depth_format = match f[4] {
            0 => None,
            word => Some(
                PixelFormat::from_word(word)
                    .filter(|format| format.is_depth())
                    .ok_or(value("depth format", word))?,
            ),
        };
        let samples = f[5];
        if ![1, 2, 4, 8].contains(&samples) {
            return Err(value("samples", samples));
        }
        let blend = Blend {
            enabled: flag("blending", f[6])?,
            source_color: read("blend factor", f[7], BlendFactor::from_word)?,
            destination_color: read("blend factor", f[8], BlendFactor::from_word)?,
            color_operation: read("blend operation", f[9], BlendOperation::from_word)?,
            source_alpha: read("blend factor", f[10], BlendFactor::from_word)?,
            destination_alpha: read("blend factor", f[11], BlendFactor::from_word)?,
            alpha_operation: read("blend operation", f[12], BlendOperation::from_word)?,
        };
        let write_mask = f[13];
        if write_mask > 0xf {
            return Err(value("write mask", write_mask));
        }
        let (attribute_count, layout_count) = (f[14], f[15]);
        if attribute_count > MAX_ATTRIBUTES {
            return Err(value("attributes", attribute_count));
        }
        if layout_count > BUFFER_SLOTS {
            return Err(value("layouts", layout_count));
        }
        let expected = op::words(op::CREATE_PIPELINE).expect("an op")
            + attribute_count * op::PIPELINE_ATTRIBUTE_WORDS
            + layout_count * op::PIPELINE_LAYOUT_WORDS;
        let words = f.len() as u32 + 1;
        if words != expected {
            return Err(RenderError::Size { words, expected });
        }
        let lists = &f[16..];
        let (attribute_words, layout_words) =
            lists.split_at((attribute_count * op::PIPELINE_ATTRIBUTE_WORDS) as usize);
        let mut layouts: Vec<VertexLayout> = Vec::new();
        for words in layout_words
            .as_chunks::<{ op::PIPELINE_LAYOUT_WORDS as usize }>()
            .0
        {
            let (slot, stride, step_rate) = (words[0], words[1], words[3]);
            if slot >= BUFFER_SLOTS || layouts.iter().any(|layout| layout.slot == slot) {
                return Err(value("layout slot", slot));
            }
            let step = read("step", words[2], Step::from_word)?;
            if stride == 0 || !stride.is_multiple_of(4) {
                return Err(value("stride", stride));
            }
            let rate_fits = match step {
                Step::Vertex => step_rate == 1,
                Step::Instance => step_rate >= 1,
                Step::Constant => step_rate == 0,
            };
            if !rate_fits {
                return Err(value("step rate", step_rate));
            }
            layouts.push(VertexLayout {
                slot,
                stride,
                step,
                step_rate,
            });
        }
        let mut attributes: Vec<VertexAttribute> = Vec::new();
        for words in attribute_words
            .as_chunks::<{ op::PIPELINE_ATTRIBUTE_WORDS as usize }>()
            .0
        {
            let (index, offset, slot) = (words[0], words[2], words[3]);
            if index >= MAX_ATTRIBUTES
                || attributes.iter().any(|attribute| attribute.index == index)
            {
                return Err(value("attribute index", index));
            }
            let format = read("vertex format", words[1], VertexFormat::from_word)?;
            let layout = layouts
                .iter()
                .find(|layout| layout.slot == slot)
                .ok_or(value("attribute slot", slot))?;
            if !offset.is_multiple_of(4) {
                return Err(value("attribute offset", offset));
            }
            if u64::from(offset) + u64::from(format.size()) > u64::from(layout.stride) {
                return Err(bounds("attribute"));
            }
            attributes.push(VertexAttribute {
                index,
                format,
                offset,
                slot,
            });
        }
        self.pipelines.insert(
            f[0],
            Pipeline {
                color_format,
                depth_format,
                samples,
                attributes: attributes.clone(),
                layouts: layouts.clone(),
            },
        );
        Ok(Command::CreatePipeline {
            id: f[0],
            vertex_library: f[1],
            fragment_library: f[2],
            color_format,
            depth_format,
            samples,
            blend,
            write_mask,
            attributes,
            layouts,
        })
    }

    fn create_depth_stencil(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        self.fresh(ObjectKind::DepthStencil, f[0])?;
        let face = |words: &[u32]| -> Result<StencilFace, RenderError> {
            if words[4] > 0xff {
                return Err(value("stencil read mask", words[4]));
            }
            if words[5] > 0xff {
                return Err(value("stencil write mask", words[5]));
            }
            Ok(StencilFace {
                fail: read("stencil operation", words[0], StencilOperation::from_word)?,
                depth_fail: read("stencil operation", words[1], StencilOperation::from_word)?,
                pass: read("stencil operation", words[2], StencilOperation::from_word)?,
                compare: read("stencil compare", words[3], Compare::from_word)?,
                read_mask: words[4],
                write_mask: words[5],
            })
        };
        let command = Command::CreateDepthStencil {
            id: f[0],
            depth_compare: read("depth compare", f[1], Compare::from_word)?,
            depth_write: flag("depth write", f[2])?,
            stencil: flag("stencil", f[3])?,
            front: face(&f[4..10])?,
            back: face(&f[10..16])?,
        };
        self.depth_stencils.insert(f[0], ());
        Ok(command)
    }

    fn create_sampler(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        self.fresh(ObjectKind::Sampler, f[0])?;
        let filter = |field: &str, word: u32| {
            Filter::from_word(word)
                .filter(|&filter| filter != Filter::None)
                .ok_or(value(field, word))
        };
        let min_filter = filter("min filter", f[1])?;
        let mag_filter = filter("mag filter", f[2])?;
        let mip_filter = read("mip filter", f[3], Filter::from_word)?;
        let address_u = read("address", f[4], Address::from_word)?;
        let address_v = read("address", f[5], Address::from_word)?;
        let address_w = read("address", f[6], Address::from_word)?;
        if !(1..=16).contains(&f[7]) {
            return Err(value("anisotropy", f[7]));
        }
        let min_lod = finite("lod", f[8])?;
        let max_lod = finite("lod", f[9])?;
        if min_lod < 0.0 || min_lod > max_lod {
            return Err(float("lod"));
        }
        let border = read("border", f[10], Border::from_word)?;
        self.samplers.insert(f[0], ());
        Ok(Command::CreateSampler {
            id: f[0],
            min_filter,
            mag_filter,
            mip_filter,
            address_u,
            address_v,
            address_w,
            max_anisotropy: f[7],
            min_lod,
            max_lod,
            border,
        })
    }

    // ---------- passes

    fn begin_pass(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        let (color, depth, resolve, visibility) = (f[0], f[1], f[11], f[12]);
        let (color_format, samples, width, height) = if color == 0 {
            if self.drawable.0 == 0 || self.drawable.1 == 0 {
                return Err(RenderError::NoWindow);
            }
            (PixelFormat::Bgra8, 1, self.drawable.0, self.drawable.1)
        } else {
            let texture = self.texture(color)?;
            if texture.usage & USAGE_RENDER_TARGET == 0
                || texture.kind != TextureKind::Plane
                || !texture.format.is_color_target()
            {
                return Err(value("colour", color));
            }
            (
                texture.format,
                texture.samples,
                texture.width,
                texture.height,
            )
        };
        let depth_format = match depth {
            0 => None,
            id => {
                let texture = self.texture(id)?;
                if !texture.format.is_depth() {
                    return Err(value("depth", id));
                }
                if (texture.width, texture.height, texture.samples) != (width, height, samples) {
                    return Err(mismatch("depth"));
                }
                Some(texture.format)
            }
        };
        if resolve != 0 {
            let texture = self.texture(resolve)?;
            let suits = samples > 1
                && texture.samples == 1
                && texture.kind == TextureKind::Plane
                && texture.usage & USAGE_RENDER_TARGET != 0
                && texture.format == color_format
                && (texture.width, texture.height) == (width, height);
            if !suits {
                return Err(mismatch("resolve"));
            }
        }
        let visibility_length = match visibility {
            0 => None,
            id => Some((id, self.buffer(id)?)),
        };
        let color_load = read("colour load", f[2], Load::from_word)?;
        let depth_load = read("depth load", f[3], Load::from_word)?;
        let stencil_load = read("stencil load", f[4], Load::from_word)?;
        let clear = [
            finite("clear colour", f[5])?,
            finite("clear colour", f[6])?,
            finite("clear colour", f[7])?,
            finite("clear colour", f[8])?,
        ];
        let clear_depth = finite("clear depth", f[9])?;
        if !(0.0..=1.0).contains(&clear_depth) {
            return Err(float("clear depth"));
        }
        if f[10] > 0xff {
            return Err(value("clear stencil", f[10]));
        }
        self.pass = Some(Pass {
            color,
            depth,
            resolve,
            color_format,
            depth_format,
            samples,
            width,
            height,
            visibility: visibility_length,
        });
        self.bound = Bound::default();
        Ok(Command::BeginPass {
            color,
            depth,
            color_load,
            depth_load,
            stencil_load,
            clear_red: clear[0],
            clear_green: clear[1],
            clear_blue: clear[2],
            clear_alpha: clear[3],
            clear_depth,
            clear_stencil: f[10],
            resolve,
            visibility,
            width,
            height,
        })
    }

    fn set_pipeline(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        self.existing(ObjectKind::Pipeline, f[0])?;
        let pipeline = &self.pipelines[&f[0]];
        let pass = self.pass.expect("inside a pass");
        if pipeline.color_format != pass.color_format {
            return Err(mismatch("colour format"));
        }
        if pipeline.depth_format != pass.depth_format {
            return Err(mismatch("depth format"));
        }
        if pipeline.samples != pass.samples {
            return Err(mismatch("samples"));
        }
        self.bound.pipeline = Some(f[0]);
        Ok(Command::SetPipeline { id: f[0] })
    }

    fn set_vertex_buffer(&mut self, f: &[u32]) -> Result<Command, RenderError> {
        let (index, id, offset) = (f[0], f[1], f[2]);
        slot(index, BUFFER_SLOTS)?;
        let binding = if id == 0 {
            None
        } else {
            let length = self.buffer(id)?;
            if !offset.is_multiple_of(4) {
                return Err(value("offset", offset));
            }
            if offset > length {
                return Err(bounds("offset"));
            }
            Some((id, offset))
        };
        self.bound.vertex_buffers[index as usize] = binding;
        Ok(Command::SetVertexBuffer {
            slot: index,
            id,
            offset,
        })
    }

    // ---------- draws

    /// Checks that each of the pipeline's layouts of `steps` has a buffer
    /// with room for `elements(layout)` elements.
    fn check_vertices(
        &self,
        steps: &[Step],
        elements: impl Fn(&VertexLayout) -> u64,
    ) -> Result<(), RenderError> {
        let id = self.bound.pipeline.ok_or(RenderError::NoPipeline)?;
        let pipeline = &self.pipelines[&id];
        for layout in &pipeline.layouts {
            // the bytes of each element the attributes read
            let Some(end) = pipeline
                .attributes
                .iter()
                .filter(|attribute| attribute.slot == layout.slot)
                .map(|attribute| attribute.offset + attribute.format.size())
                .max()
            else {
                continue;
            };
            let (buffer, offset) = self.bound.vertex_buffers[layout.slot as usize]
                .ok_or(RenderError::Unbound { slot: layout.slot })?;
            if !steps.contains(&layout.step) {
                continue;
            }
            let last = elements(layout) - 1;
            let needed = u64::from(offset) + last * u64::from(layout.stride) + u64::from(end);
            if needed > u64::from(self.buffers[&buffer]) {
                return Err(bounds(match layout.step {
                    Step::Vertex => "vertices",
                    Step::Instance => "instances",
                    Step::Constant => "constants",
                }));
            }
        }
        Ok(())
    }

    fn draw(&self, f: &[u32]) -> Result<Option<Command>, RenderError> {
        let primitive = read("primitive", f[0], Primitive::from_word)?;
        let (first, vertices, instances) = (f[1], f[2], f[3]);
        self.bound.pipeline.ok_or(RenderError::NoPipeline)?;
        if vertices == 0 || instances == 0 {
            return Ok(None);
        }
        self.check_vertices(&[Step::Vertex, Step::Instance, Step::Constant], |layout| {
            elements(layout, u64::from(first) + u64::from(vertices), instances)
        })?;
        Ok(Some(Command::Draw {
            primitive,
            first,
            vertices,
            instances,
        }))
    }

    fn draw_indexed(&self, f: &[u32]) -> Result<Option<Command>, RenderError> {
        let primitive = read("primitive", f[0], Primitive::from_word)?;
        let index_type = read("index type", f[2], IndexType::from_word)?;
        let (indices, buffer, offset, instances) = (f[1], f[3], f[4], f[6]);
        let length = self.buffer(buffer)?;
        self.bound.pipeline.ok_or(RenderError::NoPipeline)?;
        if indices == 0 || instances == 0 {
            return Ok(None);
        }
        let size = index_type.size();
        if !offset.is_multiple_of(size) {
            return Err(value("index offset", offset));
        }
        within64(
            "indices",
            u64::from(offset),
            u64::from(indices) * u64::from(size),
            u64::from(length),
        )?;
        // (the vertices the indices name are not checked: module docs)
        self.check_vertices(&[Step::Instance, Step::Constant], |layout| {
            elements(layout, 0, instances)
        })?;
        Ok(Some(Command::DrawIndexed {
            primitive,
            indices,
            index_type,
            buffer,
            offset,
            base_vertex: f[5] as i32,
            instances,
        }))
    }

    // ---------- outside passes

    fn copy_texture(&self, f: &[u32]) -> Result<Command, RenderError> {
        let source = self.texture(f[0])?;
        let destination = self.texture(f[5])?;
        if f[0] == f[5] {
            return Err(value("destination", f[5]));
        }
        if source.format != destination.format {
            return Err(mismatch("format"));
        }
        if source.samples > 1 || destination.samples > 1 {
            return Err(mismatch("samples"));
        }
        let (width, height) = (f[10], f[11]);
        if width == 0 {
            return Err(value("width", width));
        }
        if height == 0 {
            return Err(value("height", height));
        }
        for (name, texture, words) in [
            ("source", &source, &f[1..5]),
            ("destination", &destination, &f[6..10]),
        ] {
            let (level, slice, x, y) = (words[0], words[1], words[2], words[3]);
            let size = level_size(texture, level)?;
            if slice >= slices(texture, size) {
                return Err(value("slice", slice));
            }
            within(name, x, width, size.0)?;
            within(name, y, height, size.1)?;
            blocks(texture.format, (x, y, width, height), size)?;
        }
        Ok(Command::CopyTexture {
            source: f[0],
            source_level: f[1],
            source_slice: f[2],
            source_x: f[3],
            source_y: f[4],
            destination: f[5],
            destination_level: f[6],
            destination_slice: f[7],
            destination_x: f[8],
            destination_y: f[9],
            width,
            height,
        })
    }

    fn read_texture(&self, f: &[u32], memory: &impl GuestRanges) -> Result<Command, RenderError> {
        let texture = self.texture(f[0])?;
        let (level, slice, x, y, width, height, address, bytes_per_row, length) =
            (f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9]);
        if texture.samples > 1 {
            return Err(value("samples", texture.samples));
        }
        if texture.format.is_compressed() || texture.format.is_depth() {
            return Err(value("format", texture.format.word()));
        }
        let size = level_size(&texture, level)?;
        if slice >= slices(&texture, size) {
            return Err(value("slice", slice));
        }
        if width == 0 {
            return Err(value("width", width));
        }
        if height == 0 {
            return Err(value("height", height));
        }
        within("region", x, width, size.0)?;
        within("region", y, height, size.1)?;
        let row_bytes = u64::from(width) * u64::from(texture.format.block().1);
        if u64::from(bytes_per_row) < row_bytes {
            return Err(value("bytes per row", bytes_per_row));
        }
        if u64::from(length) < u64::from(bytes_per_row) * u64::from(height - 1) + row_bytes {
            return Err(value("length", length));
        }
        guest(memory, address, u64::from(length))?;
        Ok(Command::ReadTexture {
            id: f[0],
            level,
            slice,
            x,
            y,
            width,
            height,
            address,
            bytes_per_row,
            length,
        })
    }
}

/// How many elements of `layout` a draw of `vertices` (from 0) and
/// `instances` reads; at least 1.
fn elements(layout: &VertexLayout, vertices: u64, instances: u32) -> u64 {
    match layout.step {
        Step::Vertex => vertices,
        Step::Instance => u64::from(instances.div_ceil(layout.step_rate)),
        Step::Constant => 1,
    }
}

fn check_id(kind: ObjectKind, id: u32) -> Result<(), RenderError> {
    let most = match kind {
        ObjectKind::Buffer => MAX_BUFFERS,
        ObjectKind::Texture => MAX_TEXTURES,
        ObjectKind::Library => MAX_LIBRARIES,
        ObjectKind::Pipeline => MAX_PIPELINES,
        ObjectKind::DepthStencil => MAX_DEPTH_STENCILS,
        ObjectKind::Sampler => MAX_SAMPLERS,
    };
    if id == 0 || id > most {
        return Err(RenderError::Id { kind, id });
    }
    Ok(())
}

/// A texture's width, height and depth at `level`.
fn level_size(texture: &Texture, level: u32) -> Result<(u32, u32, u32), RenderError> {
    if level >= texture.levels {
        return Err(value("level", level));
    }
    let depth = if texture.kind == TextureKind::Volume {
        (texture.depth >> level).max(1)
    } else {
        1
    };
    Ok((
        (texture.width >> level).max(1),
        (texture.height >> level).max(1),
        depth,
    ))
}

/// A texture's slices at a level of `size`: a cube's faces, or a volume's
/// depth (whose slice is its z).
fn slices(texture: &Texture, size: (u32, u32, u32)) -> u32 {
    match texture.kind {
        TextureKind::Plane => 1,
        TextureKind::Cube => 6,
        TextureKind::Volume => size.2,
    }
}

/// The rows of blocks of a `region` (x, y, width, height) of a level of
/// `size`, and the bytes in each, after checking that a compressed format's
/// region is whole blocks (or reaches the level's edge).
fn blocks(
    format: PixelFormat,
    (x, y, width, height): (u32, u32, u32, u32),
    size: (u32, u32, u32),
) -> Result<(u64, u64), RenderError> {
    let (block, block_bytes) = format.block();
    if !x.is_multiple_of(block) {
        return Err(value("x", x));
    }
    if !y.is_multiple_of(block) {
        return Err(value("y", y));
    }
    if !width.is_multiple_of(block) && x + width != size.0 {
        return Err(value("width", width));
    }
    if !height.is_multiple_of(block) && y + height != size.1 {
        return Err(value("height", height));
    }
    Ok((
        u64::from(height.div_ceil(block)),
        u64::from(width.div_ceil(block)) * u64::from(block_bytes),
    ))
}

fn read<T>(field: &str, word: u32, from_word: fn(u32) -> Option<T>) -> Result<T, RenderError> {
    from_word(word).ok_or_else(|| value(field, word))
}

fn flag(field: &str, word: u32) -> Result<bool, RenderError> {
    match word {
        0 => Ok(false),
        1 => Ok(true),
        _ => Err(value(field, word)),
    }
}

fn finite(field: &str, word: u32) -> Result<f32, RenderError> {
    let number = f32::from_bits(word);
    if !number.is_finite() {
        return Err(float(field));
    }
    Ok(number)
}

fn slot(slot: u32, slots: u32) -> Result<(), RenderError> {
    if slot >= slots {
        return Err(value("slot", slot));
    }
    Ok(())
}

/// Checks `length` bytes at `offset` lie inside `size`.
fn within(field: &str, offset: u32, length: u32, size: u32) -> Result<(), RenderError> {
    within64(field, u64::from(offset), u64::from(length), u64::from(size))
}

fn within64(field: &str, offset: u64, length: u64, size: u64) -> Result<(), RenderError> {
    if offset + length > size {
        return Err(bounds(field));
    }
    Ok(())
}

fn guest(memory: &impl GuestRanges, address: u32, length: u64) -> Result<(), RenderError> {
    if !memory.is_mapped(address, length) {
        return Err(RenderError::Guest { address, length });
    }
    Ok(())
}

fn value(field: &str, value: u32) -> RenderError {
    RenderError::Value {
        field: field.into(),
        value,
    }
}

fn float(field: &str) -> RenderError {
    RenderError::Float {
        field: field.into(),
    }
}

fn bounds(field: &str) -> RenderError {
    RenderError::Bounds {
        field: field.into(),
    }
}

fn mismatch(field: &str) -> RenderError {
    RenderError::Mismatch {
        field: field.into(),
    }
}

fn rejection(offset: u32, op: u32, error: RenderError) -> Rejection {
    Rejection {
        offset,
        op,
        reason: error.to_string(),
        error,
    }
}
