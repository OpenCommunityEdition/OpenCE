use std::collections::BTreeMap;

use super::*;

const GUEST: u32 = 0x1000_0000;
const GUEST_SIZE: u64 = 0x10_0000;

/// Guest memory mapped at GUEST.
struct Mapped;

impl GuestRanges for Mapped {
    fn is_mapped(&self, address: u32, length: u64) -> bool {
        u64::from(address) >= u64::from(GUEST)
            && u64::from(address) + length <= u64::from(GUEST) + GUEST_SIZE
    }
}

fn f(value: f32) -> u32 {
    value.to_bits()
}

const BGRA8: u32 = PixelFormat::Bgra8.word();
const RGBA8: u32 = PixelFormat::Rgba8.word();
const BC1: u32 = PixelFormat::Bc1.word();
const DEPTH: u32 = PixelFormat::Depth32f.word();
const PLANE: u32 = TextureKind::Plane.word();
const CUBE: u32 = TextureKind::Cube.word();
const FLOAT3: u32 = VertexFormat::Float3.word();
const FLOAT4: u32 = VertexFormat::Float4.word();
const STEP_VERTEX: u32 = Step::Vertex.word();
const STEP_INSTANCE: u32 = Step::Instance.word();
const TRIANGLE: u32 = Primitive::Triangle.word();
const UINT16: u32 = IndexType::Uint16.word();
const VERTEX: u32 = Stage::Vertex.word();
const FRAGMENT: u32 = Stage::Fragment.word();
const CLEAR: u32 = Load::Clear.word();
const DONT_CARE: u32 = Load::DontCare.word();
const RT: u32 = USAGE_RENDER_TARGET;
const SAMPLED: u32 = USAGE_SAMPLED;

/// A batch being written, as the guest writes one.
struct Writer {
    words: Vec<u32>,
    count: u32,
}

impl Writer {
    fn new() -> Writer {
        Writer {
            words: vec![MAGIC, VERSION, 0, 0],
            count: 0,
        }
    }

    fn command(mut self, op: u32, fields: &[u32]) -> Writer {
        self.words.push(op | ((fields.len() as u32 + 1) << 16));
        self.words.extend_from_slice(fields);
        self.count += 1;
        self
    }

    /// Words that are not counted as a command.
    fn raw(mut self, words: &[u32]) -> Writer {
        self.words.extend_from_slice(words);
        self
    }

    fn buffer(self, id: u32, length: u32) -> Writer {
        self.command(op::CREATE_BUFFER, &[id, length])
    }

    fn texture(
        self,
        id: u32,
        format: u32,
        width: u32,
        height: u32,
        levels: u32,
        usage: u32,
    ) -> Writer {
        self.command(
            op::CREATE_TEXTURE,
            &[id, PLANE, format, width, height, 1, levels, usage, 1],
        )
    }

    fn library(self, id: u32) -> Writer {
        self.command(op::CREATE_LIBRARY, &[id, GUEST, 64])
    }

    /// A pipeline on library 1 drawing `color` (and `depth`), with the
    /// attributes and layouts given.
    fn pipeline_with(
        self,
        id: u32,
        color: u32,
        depth: u32,
        samples: u32,
        attributes: &[[u32; 4]],
        layouts: &[[u32; 4]],
    ) -> Writer {
        let one = BlendFactor::One.word();
        let zero = BlendFactor::Zero.word();
        let add = BlendOperation::Add.word();
        let mut fields = vec![
            id,
            1,
            1,
            color,
            depth,
            samples,
            0,
            one,
            zero,
            add,
            one,
            zero,
            add,
            0xf,
            attributes.len() as u32,
            layouts.len() as u32,
        ];
        for attribute in attributes {
            fields.extend_from_slice(attribute);
        }
        for layout in layouts {
            fields.extend_from_slice(layout);
        }
        self.command(op::CREATE_PIPELINE, &fields)
    }

    /// Positions (float3) from slot 0, 12 bytes apart.
    fn pipeline(self, id: u32, color: u32, depth: u32) -> Writer {
        self.pipeline_with(
            id,
            color,
            depth,
            1,
            &[[0, FLOAT3, 0, 0]],
            &[[0, 12, STEP_VERTEX, 1]],
        )
    }

    fn pass(self, color: u32, depth: u32) -> Writer {
        self.command(
            op::BEGIN_PASS,
            &[
                color,
                depth,
                CLEAR,
                if depth == 0 { DONT_CARE } else { CLEAR },
                DONT_CARE,
                f(0.0),
                f(0.0),
                f(0.0),
                f(1.0),
                f(1.0),
                0,
                0,
                0,
            ],
        )
    }

    fn window_pass(self) -> Writer {
        self.pass(0, 0)
    }

    fn end(self) -> Writer {
        self.command(op::END_PASS, &[])
    }

    fn bytes(&self) -> Vec<u8> {
        let mut words = self.words.clone();
        words[2] = (words.len() * 4) as u32;
        words[3] = self.count;
        words.iter().flat_map(|word| word.to_le_bytes()).collect()
    }
}

fn validator() -> RenderValidator {
    let mut validator = RenderValidator::new();
    validator.set_drawable_size(640, 480);
    validator
}

fn decode(validator: &mut RenderValidator, writer: Writer) -> DecodedBatch {
    validator.decode(&writer.bytes(), &Mapped).unwrap()
}

fn errors(batch: &DecodedBatch) -> Vec<RenderError> {
    batch
        .rejected
        .iter()
        .map(|rejection| rejection.error.clone())
        .collect()
}

/// Decodes `writer` in a fresh validator and returns what it refused.
fn refused(writer: Writer) -> Vec<RenderError> {
    errors(&decode(&mut validator(), writer))
}

fn value(field: &str, value: u32) -> RenderError {
    RenderError::Value {
        field: field.into(),
        value,
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

fn float(field: &str) -> RenderError {
    RenderError::Float {
        field: field.into(),
    }
}

/// A window pass with pipeline 1 set and buffer 1 (`length` bytes) in
/// slot 0.
fn drawing(length: u32) -> Writer {
    Writer::new()
        .buffer(1, length)
        .library(1)
        .pipeline(1, BGRA8, 0)
        .window_pass()
        .command(op::SET_PIPELINE, &[1])
        .command(op::SET_VERTEX_BUFFER, &[0, 1, 0])
}

// ---------- the header

fn header_values() -> BTreeMap<String, u32> {
    let mut values = BTreeMap::new();
    for line in include_str!("../../../include/opence_render.h").lines() {
        let Some(define) = line.strip_prefix("#define OPENCE_RENDER_") else {
            continue;
        };
        let mut parts = define.split_whitespace();
        let name = parts.next().unwrap();
        // (the include guard and the macros)
        let Some(text) = parts.next().filter(|_| !name.contains('(')) else {
            continue;
        };
        let text = text.trim_end_matches('u');
        let number = match text.strip_prefix("0x") {
            Some(hex) => u32::from_str_radix(hex, 16),
            None => text.parse(),
        };
        values.insert(
            name.to_string(),
            number.unwrap_or_else(|_| panic!("{line}")),
        );
    }
    values
}

fn rust_values() -> BTreeMap<String, u32> {
    let mut values = BTreeMap::new();
    let mut insert = |name: String, value: u32| {
        assert!(values.insert(name.clone(), value).is_none(), "{name} twice");
    };
    for &(name, value) in HEADER_CONSTANTS {
        insert(name.into(), value);
    }
    for &(code, name, words) in op::SIZES {
        insert(format!("OP_{name}"), code);
        insert(format!("{name}_WORDS"), words);
    }
    let tables: &[&[(&str, u32)]] = &[
        ObjectKind::HEADER,
        TextureKind::HEADER,
        PixelFormat::HEADER,
        VertexFormat::HEADER,
        Step::HEADER,
        BlendFactor::HEADER,
        BlendOperation::HEADER,
        Compare::HEADER,
        StencilOperation::HEADER,
        Filter::HEADER,
        Address::HEADER,
        Border::HEADER,
        Load::HEADER,
        Cull::HEADER,
        Winding::HEADER,
        Fill::HEADER,
        Stage::HEADER,
        Primitive::HEADER,
        IndexType::HEADER,
        Visibility::HEADER,
    ];
    for table in tables {
        for &(name, value) in *table {
            insert(name.into(), value);
        }
    }
    values
}

#[test]
fn the_header_is_the_decoders() {
    assert_eq!(header_values(), rust_values());
}

#[test]
fn every_value_reads_back() {
    for &(_, value) in PixelFormat::HEADER {
        assert_eq!(PixelFormat::from_word(value).unwrap().word(), value);
    }
    assert_eq!(PixelFormat::from_word(0), None);
    assert_eq!(PixelFormat::from_word(14), None);
}

// ---------- batches

#[test]
fn a_batch_needs_its_header() {
    let mut validator = validator();
    let good = Writer::new().bytes();
    assert!(validator
        .decode(&good, &Mapped)
        .unwrap()
        .commands
        .is_empty());
    assert_eq!(
        validator.decode(&good[..12], &Mapped),
        Err(BatchError::Size { size: 12 })
    );
    let mut ragged = good.clone();
    ragged.push(0);
    assert_eq!(
        validator.decode(&ragged, &Mapped),
        Err(BatchError::Size { size: 17 })
    );
    let huge = vec![0; MAX_BATCH_LENGTH as usize + 4];
    assert_eq!(
        validator.decode(&huge, &Mapped),
        Err(BatchError::Size {
            size: u64::from(MAX_BATCH_LENGTH) + 4
        })
    );
    let mut magic = good.clone();
    magic[0] = 0;
    assert!(matches!(
        validator.decode(&magic, &Mapped),
        Err(BatchError::Magic { .. })
    ));
    let mut version = good.clone();
    version[4] = 2;
    assert_eq!(
        validator.decode(&version, &Mapped),
        Err(BatchError::Version { version: 2 })
    );
    let mut length = good;
    length[8] = 20;
    assert_eq!(
        validator.decode(&length, &Mapped),
        Err(BatchError::Length { said: 20, size: 16 })
    );
}

#[test]
fn a_frame_decodes() {
    let mut validator = validator();
    let stencil = [
        StencilOperation::Keep.word(),
        StencilOperation::Keep.word(),
        StencilOperation::Keep.word(),
        Compare::Always.word(),
        0xff,
        0xff,
    ];
    let mut depth_stencil = vec![1, Compare::Less.word(), 1, 0];
    depth_stencil.extend_from_slice(&stencil);
    depth_stencil.extend_from_slice(&stencil);
    let linear = Filter::Linear.word();
    let repeat = Address::Repeat.word();
    let writer = Writer::new()
        .buffer(1, 36)
        .command(op::UPLOAD_BUFFER, &[1, 0, GUEST, 36])
        .library(1)
        .pipeline(1, BGRA8, DEPTH)
        .command(op::CREATE_DEPTH_STENCIL, &depth_stencil)
        .command(
            op::CREATE_SAMPLER,
            &[
                1,
                linear,
                linear,
                Filter::None.word(),
                repeat,
                repeat,
                repeat,
                1,
                f(0.0),
                f(1000.0),
                Border::OpaqueBlack.word(),
            ],
        )
        .texture(1, RGBA8, 4, 4, 1, SAMPLED)
        .command(
            op::UPLOAD_TEXTURE,
            &[1, 0, 0, 0, 0, 0, 4, 4, 1, GUEST, 16, 64, 64],
        )
        .texture(2, DEPTH, 640, 480, 1, RT)
        .pass(0, 2)
        .command(op::SET_PIPELINE, &[1])
        .command(op::SET_DEPTH_STENCIL, &[1, 0])
        .command(
            op::SET_RASTER,
            &[
                Cull::Back.word(),
                Winding::Clockwise.word(),
                Fill::Solid.word(),
                f(0.0),
                f(0.0),
                f(0.0),
                0,
            ],
        )
        .command(
            op::SET_VIEWPORT,
            &[f(0.0), f(0.0), f(640.0), f(480.0), f(0.0), f(1.0)],
        )
        .command(op::SET_VERTEX_BUFFER, &[0, 1, 0])
        .command(op::SET_TEXTURE, &[FRAGMENT, 0, 1])
        .command(op::SET_SAMPLER, &[FRAGMENT, 0, 1])
        .command(op::SET_BYTES, &[VERTEX, 1, 6, 0x0403_0201, 0x0000_0605])
        .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
        .end()
        .command(op::PRESENT, &[]);
    let batch = decode(&mut validator, writer);
    assert_eq!(errors(&batch), []);
    assert_eq!(batch.commands.len(), 21);
    assert!(matches!(
        batch.commands[9],
        Command::BeginPass {
            color: 0,
            depth: 2,
            width: 640,
            height: 480,
            ..
        }
    ));
    assert_eq!(
        batch.commands[17],
        Command::SetBytes {
            stage: Stage::Vertex,
            slot: 1,
            bytes: vec![1, 2, 3, 4, 5, 6],
        }
    );
    assert_eq!(
        batch.commands[18],
        Command::Draw {
            primitive: Primitive::Triangle,
            first: 0,
            vertices: 3,
            instances: 1,
        }
    );
    assert_eq!(batch.commands[20], Command::Present);
    assert!(!validator.in_pass());
}

#[test]
fn a_pass_spans_batches() {
    let mut validator = validator();
    let first = decode(&mut validator, drawing(36));
    assert_eq!(errors(&first), []);
    assert!(validator.in_pass());
    let second = decode(
        &mut validator,
        Writer::new().command(op::DRAW, &[TRIANGLE, 0, 3, 1]).end(),
    );
    assert_eq!(errors(&second), []);
    assert_eq!(second.commands.len(), 2);
}

#[test]
fn a_truncated_command_ends_the_batch() {
    let mut validator = validator();
    let writer = Writer::new()
        .buffer(1, 16)
        // ten words said, two left
        .raw(&[op::CREATE_BUFFER | (10 << 16), 2])
        .buffer(3, 16);
    let batch = decode(&mut validator, writer);
    assert_eq!(
        batch.commands,
        [Command::CreateBuffer { id: 1, length: 16 }]
    );
    assert_eq!(
        errors(&batch),
        [RenderError::Truncated { words: 10, left: 5 }]
    );
    assert_eq!(batch.rejected[0].offset, 28);
    assert_eq!(batch.rejected[0].op, op::CREATE_BUFFER);
}

#[test]
fn a_command_of_no_words_ends_the_batch() {
    let batch = decode(
        &mut validator(),
        Writer::new().raw(&[op::PRESENT]).command(op::PRESENT, &[]),
    );
    assert!(batch.commands.is_empty());
    assert_eq!(
        errors(&batch),
        [RenderError::Truncated { words: 0, left: 2 }]
    );
}

#[test]
fn a_bad_command_is_skipped() {
    let batch = decode(
        &mut validator(),
        Writer::new()
            .command(99, &[1, 2])
            .command(op::PRESENT, &[1])
            .command(op::CREATE_BUFFER, &[1])
            .command(op::PRESENT, &[]),
    );
    assert_eq!(batch.commands, [Command::Present]);
    assert_eq!(
        errors(&batch),
        [
            RenderError::UnknownOp,
            RenderError::Size {
                words: 2,
                expected: 1
            },
            RenderError::Size {
                words: 2,
                expected: 3
            },
        ]
    );
}

#[test]
fn the_count_is_checked() {
    let mut writer = Writer::new().command(op::PRESENT, &[]);
    writer.count = 3;
    assert_eq!(refused(writer), [RenderError::Count { said: 3, found: 1 }]);
}

// ---------- objects

#[test]
fn ids_are_checked() {
    let buffer = ObjectKind::Buffer;
    assert_eq!(
        refused(
            Writer::new()
                .buffer(0, 16)
                .buffer(MAX_BUFFERS + 1, 16)
                .buffer(MAX_BUFFERS, 16)
                .buffer(MAX_BUFFERS, 16)
                .command(op::UPLOAD_BUFFER, &[2, 0, GUEST, 4])
                .command(op::DESTROY, &[ObjectKind::Texture.word(), 1])
                .command(op::DESTROY, &[7, 1])
        ),
        [
            RenderError::Id {
                kind: buffer,
                id: 0
            },
            RenderError::Id {
                kind: buffer,
                id: MAX_BUFFERS + 1
            },
            RenderError::Exists {
                kind: buffer,
                id: MAX_BUFFERS
            },
            RenderError::Missing {
                kind: buffer,
                id: 2
            },
            RenderError::Missing {
                kind: ObjectKind::Texture,
                id: 1
            },
            value("object kind", 7),
        ]
    );
}

#[test]
fn a_destroyed_id_is_made_again() {
    assert_eq!(
        refused(
            Writer::new()
                .buffer(1, 16)
                .command(op::DESTROY, &[ObjectKind::Buffer.word(), 1])
                .buffer(1, 32)
                .command(op::UPLOAD_BUFFER, &[1, 16, GUEST, 16])
        ),
        []
    );
}

#[test]
fn buffer_uploads_stay_inside() {
    assert_eq!(
        refused(
            Writer::new()
                .buffer(1, 0)
                .buffer(2, MAX_BUFFER_LENGTH + 1)
                .buffer(1, 64)
                .command(op::UPLOAD_BUFFER, &[1, 32, GUEST, 33])
                .command(op::UPLOAD_BUFFER, &[1, u32::MAX, GUEST, 2])
                .command(op::UPLOAD_BUFFER, &[1, 0, GUEST, 0])
                .command(op::UPLOAD_BUFFER, &[1, 0, GUEST - 4, 8])
                .command(op::UPLOAD_BUFFER, &[1, 0, u32::MAX - 3, 8])
        ),
        [
            value("length", 0),
            value("length", MAX_BUFFER_LENGTH + 1),
            bounds("upload"),
            bounds("upload"),
            value("length", 0),
            RenderError::Guest {
                address: GUEST - 4,
                length: 8
            },
            RenderError::Guest {
                address: u32::MAX - 3,
                length: 8
            },
        ]
    );
}

#[test]
fn buffer_writes_are_checked_as_uploads_are() {
    let batch = decode(
        &mut validator(),
        Writer::new()
            .buffer(1, 64)
            .command(op::WRITE_BUFFER, &[1, 16, GUEST, 16])
            .command(op::WRITE_BUFFER, &[1, 32, GUEST, 33])
            .command(op::WRITE_BUFFER, &[2, 0, GUEST, 4])
            .command(op::WRITE_BUFFER, &[1, 0, u32::MAX - 3, 8]),
    );
    assert_eq!(
        batch.commands.last(),
        Some(&Command::WriteBuffer {
            id: 1,
            offset: 16,
            address: GUEST,
            length: 16
        })
    );
    assert_eq!(
        errors(&batch),
        [
            bounds("upload"),
            RenderError::Missing {
                kind: ObjectKind::Buffer,
                id: 2
            },
            RenderError::Guest {
                address: u32::MAX - 3,
                length: 8
            },
        ]
    );
}

#[test]
fn textures_are_checked_when_made() {
    let texture = |fields: [u32; 8]| {
        let mut all = vec![1];
        all.extend_from_slice(&fields);
        Writer::new().command(op::CREATE_TEXTURE, &all)
    };
    let cases = [
        ([PLANE, RGBA8, 0, 4, 1, 1, SAMPLED, 1], value("width", 0)),
        (
            [PLANE, RGBA8, 4, MAX_TEXTURE_SIZE + 1, 1, 1, SAMPLED, 1],
            value("height", MAX_TEXTURE_SIZE + 1),
        ),
        ([PLANE, RGBA8, 4, 4, 2, 1, SAMPLED, 1], value("depth", 2)),
        // 4 x 4 has three levels
        ([PLANE, RGBA8, 4, 4, 1, 4, SAMPLED, 1], value("levels", 4)),
        ([PLANE, RGBA8, 4, 4, 1, 0, SAMPLED, 1], value("levels", 0)),
        ([CUBE, RGBA8, 4, 8, 1, 1, SAMPLED, 1], mismatch("cube")),
        ([PLANE, RGBA8, 4, 4, 1, 1, 0, 1], value("usage", 0)),
        ([PLANE, RGBA8, 4, 4, 1, 1, 4, 1], value("usage", 4)),
        ([PLANE, RGBA8, 4, 4, 1, 1, RT, 3], value("samples", 3)),
        ([PLANE, RGBA8, 4, 4, 1, 1, SAMPLED, 4], value("samples", 4)),
        ([PLANE, BC1, 4, 4, 1, 1, RT, 1], value("usage", RT)),
        (
            [PLANE, DEPTH, 4, 4, 1, 1, SAMPLED, 1],
            value("usage", SAMPLED),
        ),
        ([CUBE, DEPTH, 4, 4, 1, 1, RT, 1], value("format", DEPTH)),
        ([PLANE, 0, 4, 4, 1, 1, SAMPLED, 1], value("format", 0)),
        ([9, RGBA8, 4, 4, 1, 1, SAMPLED, 1], value("kind", 9)),
    ];
    for (fields, error) in cases {
        assert_eq!(refused(texture(fields)), [error], "{fields:?}");
    }
    let good = [
        [PLANE, RGBA8, 4, 4, 1, 3, SAMPLED, 1],
        [PLANE, RGBA8, 640, 480, 1, 1, RT, 4],
        [CUBE, BC1, 8, 8, 1, 4, SAMPLED, 1],
        [TextureKind::Volume.word(), RGBA8, 4, 4, 16, 5, SAMPLED, 1],
        [PLANE, DEPTH, 4, 4, 1, 1, RT | SAMPLED, 1],
    ];
    for fields in good {
        assert_eq!(refused(texture(fields)), [], "{fields:?}");
    }
}

#[test]
fn texture_uploads_stay_inside() {
    let upload = |fields: [u32; 12]| {
        let mut all = vec![1];
        all.extend_from_slice(&fields);
        Writer::new()
            .texture(1, RGBA8, 8, 8, 2, SAMPLED)
            .command(op::UPLOAD_TEXTURE, &all)
    };
    let cases = [
        // level 1 is 4 x 4
        ([1, 0, 1, 0, 0, 4, 4, 1, GUEST, 16, 0, 64], bounds("region")),
        ([2, 0, 0, 0, 0, 1, 1, 1, GUEST, 4, 0, 4], value("level", 2)),
        ([0, 1, 0, 0, 0, 1, 1, 1, GUEST, 4, 0, 4], value("slice", 1)),
        ([0, 0, 0, 0, 0, 0, 1, 1, GUEST, 4, 0, 4], value("width", 0)),
        (
            [0, 0, 0, 0, 0, 8, 8, 1, GUEST, 16, 0, 256],
            value("bytes per row", 16),
        ),
        (
            [0, 0, 0, 0, 0, 8, 8, 1, GUEST, 32, 0, 255],
            value("length", 255),
        ),
        (
            [
                0,
                0,
                0,
                0,
                0,
                8,
                8,
                1,
                GUEST + GUEST_SIZE as u32 - 128,
                32,
                0,
                256,
            ],
            RenderError::Guest {
                address: GUEST + GUEST_SIZE as u32 - 128,
                length: 256,
            },
        ),
        ([0, 0, 0, 0, 1, 1, 1, 1, GUEST, 4, 0, 4], value("z", 1)),
    ];
    for (fields, error) in cases {
        assert_eq!(refused(upload(fields)), [error], "{fields:?}");
    }
    // a last row needs only its own bytes
    assert_eq!(
        refused(upload([0, 0, 0, 0, 0, 8, 2, 1, GUEST, 64, 0, 96])),
        []
    );
    assert_eq!(
        refused(upload([1, 0, 0, 0, 0, 4, 4, 1, GUEST, 16, 0, 64])),
        []
    );
}

#[test]
fn compressed_uploads_are_whole_blocks() {
    let upload = |fields: [u32; 12]| {
        let mut all = vec![1];
        all.extend_from_slice(&fields);
        Writer::new()
            .texture(1, BC1, 8, 8, 4, SAMPLED)
            .command(op::UPLOAD_TEXTURE, &all)
    };
    assert_eq!(
        refused(upload([0, 0, 2, 0, 0, 4, 4, 1, GUEST, 8, 0, 8])),
        [value("x", 2)]
    );
    assert_eq!(
        refused(upload([0, 0, 0, 0, 0, 3, 4, 1, GUEST, 8, 0, 8])),
        [value("width", 3)]
    );
    // two rows of two blocks
    assert_eq!(
        refused(upload([0, 0, 0, 0, 0, 8, 8, 1, GUEST, 16, 0, 31])),
        [value("length", 31)]
    );
    assert_eq!(
        refused(upload([0, 0, 0, 0, 0, 8, 8, 1, GUEST, 16, 0, 32])),
        []
    );
    // level 2 is 2 x 2, less than a block, and level 3 is 1 x 1
    assert_eq!(
        refused(upload([2, 0, 0, 0, 0, 2, 2, 1, GUEST, 8, 0, 8])),
        []
    );
    assert_eq!(
        refused(upload([3, 0, 0, 0, 0, 1, 1, 1, GUEST, 8, 0, 8])),
        []
    );
}

#[test]
fn pipelines_are_checked_when_made() {
    let pipeline = |attributes: &[[u32; 4]], layouts: &[[u32; 4]]| {
        Writer::new()
            .library(1)
            .pipeline_with(1, BGRA8, 0, 1, attributes, layouts)
    };
    let layout = [0, 16, STEP_VERTEX, 1];
    type Case<'a> = (&'a [[u32; 4]], &'a [[u32; 4]], RenderError);
    let cases: [Case; 9] = [
        (
            &[[16, FLOAT4, 0, 0]],
            &[layout],
            value("attribute index", 16),
        ),
        (
            &[[0, FLOAT4, 0, 0], [0, FLOAT4, 0, 0]],
            &[layout],
            value("attribute index", 0),
        ),
        (&[[0, FLOAT4, 0, 1]], &[layout], value("attribute slot", 1)),
        (
            &[[0, FLOAT4, 2, 0]],
            &[layout],
            value("attribute offset", 2),
        ),
        (&[[0, FLOAT4, 4, 0]], &[layout], bounds("attribute")),
        (&[[0, 99, 0, 0]], &[layout], value("vertex format", 99)),
        (&[], &[[0, 6, STEP_VERTEX, 1]], value("stride", 6)),
        (&[], &[[0, 16, STEP_VERTEX, 2]], value("step rate", 2)),
        (&[], &[layout, layout], value("layout slot", 0)),
    ];
    for (attributes, layouts, error) in cases {
        assert_eq!(
            refused(pipeline(attributes, layouts)),
            [error],
            "{attributes:?} {layouts:?}"
        );
    }
    assert_eq!(
        refused(pipeline(
            &[[0, FLOAT4, 0, 0], [1, FLOAT4, 0, 1]],
            &[layout, [1, 16, STEP_INSTANCE, 2]],
        )),
        []
    );
    // the counts must be the words that follow
    let mut short = pipeline(&[[0, FLOAT4, 0, 0]], &[layout]);
    let last = short.words.len() - 1;
    short.words.truncate(last);
    let header = short.words.len() - 24;
    short.words[header] = op::CREATE_PIPELINE | (24 << 16);
    assert_eq!(
        refused(short),
        [RenderError::Size {
            words: 24,
            expected: 25
        }]
    );
    // a library must exist, and the colour format be a target's
    assert_eq!(
        refused(Writer::new().pipeline(1, BGRA8, 0)),
        [RenderError::Missing {
            kind: ObjectKind::Library,
            id: 1
        }]
    );
    assert_eq!(
        refused(Writer::new().library(1).pipeline(1, BC1, 0)),
        [value("colour format", BC1)]
    );
    assert_eq!(
        refused(Writer::new().library(1).pipeline(1, BGRA8, RGBA8)),
        [value("depth format", RGBA8)]
    );
}

#[test]
fn samplers_are_checked_when_made() {
    let sampler = |fields: [u32; 10]| {
        let mut all = vec![1];
        all.extend_from_slice(&fields);
        Writer::new().command(op::CREATE_SAMPLER, &all)
    };
    let linear = Filter::Linear.word();
    let repeat = Address::Repeat.word();
    let border = Border::OpaqueBlack.word();
    assert_eq!(
        refused(sampler([
            0,
            linear,
            0,
            repeat,
            repeat,
            repeat,
            1,
            f(0.0),
            f(1.0),
            border
        ])),
        [value("min filter", 0)]
    );
    assert_eq!(
        refused(sampler([
            linear,
            linear,
            0,
            repeat,
            repeat,
            repeat,
            17,
            f(0.0),
            f(1.0),
            border
        ])),
        [value("anisotropy", 17)]
    );
    assert_eq!(
        refused(sampler([
            linear,
            linear,
            0,
            repeat,
            repeat,
            repeat,
            1,
            f(f32::NAN),
            f(1.0),
            border
        ])),
        [float("lod")]
    );
    assert_eq!(
        refused(sampler([
            linear,
            linear,
            0,
            repeat,
            repeat,
            repeat,
            1,
            f(2.0),
            f(1.0),
            border
        ])),
        [float("lod")]
    );
}

// ---------- passes

#[test]
fn draw_state_is_only_inside_a_pass() {
    assert_eq!(
        refused(
            Writer::new()
                .command(op::SET_PIPELINE, &[1])
                .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
                .end()
                .window_pass()
                .window_pass()
                .command(op::PRESENT, &[])
                .command(op::GENERATE_MIPMAPS, &[1])
        ),
        [
            RenderError::OutsidePass,
            RenderError::OutsidePass,
            RenderError::OutsidePass,
            RenderError::InsidePass,
            RenderError::InsidePass,
            RenderError::InsidePass,
        ]
    );
}

#[test]
fn the_window_needs_a_size() {
    let mut validator = RenderValidator::new();
    assert_eq!(
        errors(&decode(&mut validator, Writer::new().window_pass())),
        [RenderError::NoWindow]
    );
    assert!(!validator.in_pass());
}

#[test]
fn a_pass_begins_with_nothing_bound() {
    assert_eq!(
        refused(
            drawing(36)
                .end()
                .window_pass()
                .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
                .command(op::SET_PIPELINE, &[1])
                .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
        ),
        [RenderError::NoPipeline, RenderError::Unbound { slot: 0 }]
    );
}

#[test]
fn pass_targets_must_agree() {
    let base = || {
        Writer::new()
            .texture(1, RGBA8, 64, 64, 1, RT)
            .texture(2, DEPTH, 32, 32, 1, RT)
            .texture(3, RGBA8, 64, 64, 1, SAMPLED)
            .texture(4, BGRA8, 64, 64, 1, RT)
            .command(op::CREATE_TEXTURE, &[5, PLANE, RGBA8, 64, 64, 1, 1, RT, 4])
    };
    assert_eq!(refused(base().pass(1, 2)), [mismatch("depth")]);
    assert_eq!(refused(base().pass(3, 0)), [value("colour", 3)]);
    assert_eq!(refused(base().pass(2, 0)), [value("colour", 2)]);
    assert_eq!(refused(base().pass(1, 1)), [value("depth", 1)]);
    let resolve = |resolve: u32| {
        base().command(
            op::BEGIN_PASS,
            &[
                5,
                0,
                CLEAR,
                DONT_CARE,
                DONT_CARE,
                f(0.0),
                f(0.0),
                f(0.0),
                f(0.0),
                f(1.0),
                0,
                resolve,
                0,
            ],
        )
    };
    assert_eq!(refused(resolve(4)), [mismatch("resolve")]);
    assert_eq!(refused(resolve(1)), []);
    // a clear depth past 1
    assert_eq!(
        refused(base().command(
            op::BEGIN_PASS,
            &[
                1,
                0,
                CLEAR,
                DONT_CARE,
                DONT_CARE,
                0,
                0,
                0,
                0,
                f(2.0),
                0,
                0,
                0
            ],
        )),
        [float("clear depth")]
    );
}

#[test]
fn a_pipeline_must_suit_the_pass() {
    assert_eq!(
        refused(
            Writer::new()
                .library(1)
                .pipeline(1, RGBA8, 0)
                .pipeline(2, BGRA8, DEPTH)
                .window_pass()
                .command(op::SET_PIPELINE, &[1])
                .command(op::SET_PIPELINE, &[2])
        ),
        [mismatch("colour format"), mismatch("depth format")]
    );
}

#[test]
fn pass_targets_outlive_the_pass() {
    assert_eq!(
        refused(
            Writer::new()
                .texture(1, RGBA8, 64, 64, 1, RT)
                .pass(1, 0)
                .command(op::DESTROY, &[ObjectKind::Texture.word(), 1])
                .command(op::SET_TEXTURE, &[FRAGMENT, 0, 1])
                .end()
                .command(op::DESTROY, &[ObjectKind::Texture.word(), 1])
        ),
        [RenderError::InsidePass, value("texture", 1)]
    );
}

#[test]
fn the_scissor_is_clipped() {
    let batch = decode(
        &mut validator(),
        Writer::new()
            .window_pass()
            .command(op::SET_SCISSOR, &[600, 400, 100, u32::MAX]),
    );
    assert_eq!(
        batch.commands[1],
        Command::SetScissor {
            x: 600,
            y: 400,
            width: 40,
            height: 80
        }
    );
}

#[test]
fn state_values_are_checked() {
    assert_eq!(
        refused(
            Writer::new()
                .window_pass()
                .command(
                    op::SET_VIEWPORT,
                    &[f(0.0), f(f32::INFINITY), f(1.0), f(1.0), f(0.0), f(1.0)],
                )
                .command(
                    op::SET_VIEWPORT,
                    &[f(0.0), f(0.0), f(1.0), f(1.0), f(0.0), f(1.5)],
                )
                .command(op::SET_DEPTH_STENCIL, &[0, 256])
                .command(op::SET_BYTES, &[VERTEX, 0, MAX_BYTES + 4])
                .command(op::SET_BYTES, &[VERTEX, 0, 8, 1])
                .command(op::SET_BYTES, &[FRAGMENT, BUFFER_SLOTS, 4, 1])
                .command(op::SET_TEXTURE, &[3, 0, 0])
                .command(op::SET_SAMPLER, &[FRAGMENT, SAMPLER_SLOTS, 0])
        ),
        [
            float("viewport"),
            float("viewport"),
            value("stencil reference", 256),
            value("length", MAX_BYTES + 4),
            RenderError::Size {
                words: 5,
                expected: 6
            },
            value("slot", BUFFER_SLOTS),
            value("stage", 3),
            value("slot", SAMPLER_SLOTS),
        ]
    );
}

// ---------- draws

#[test]
fn draws_stay_inside_their_vertices() {
    // three float3 positions are 36 bytes
    assert_eq!(
        refused(
            drawing(35)
                .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
                .command(op::DRAW, &[TRIANGLE, u32::MAX, 3, 1])
                .command(op::DRAW, &[TRIANGLE, 0, 2, 1])
        ),
        [bounds("vertices"), bounds("vertices")]
    );
    // nothing to draw is dropped
    let batch = decode(
        &mut validator(),
        drawing(36)
            .command(op::DRAW, &[TRIANGLE, 0, 0, 1])
            .command(op::DRAW, &[TRIANGLE, 0, 3, 0]),
    );
    assert_eq!(errors(&batch), []);
    assert!(!batch
        .commands
        .iter()
        .any(|command| matches!(command, Command::Draw { .. })));
}

#[test]
fn a_vertex_buffer_offset_counts() {
    assert_eq!(
        refused(
            drawing(48)
                .command(op::SET_VERTEX_BUFFER, &[0, 1, 12])
                .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
                .command(op::SET_VERTEX_BUFFER, &[0, 1, 16])
                .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
                .command(op::SET_VERTEX_BUFFER, &[0, 1, 2])
                .command(op::SET_VERTEX_BUFFER, &[0, 1, 52])
                .command(op::SET_VERTEX_BUFFER, &[BUFFER_SLOTS, 1, 0])
        ),
        [
            bounds("vertices"),
            value("offset", 2),
            bounds("offset"),
            value("slot", BUFFER_SLOTS),
        ]
    );
}

#[test]
fn instances_step_through_their_buffer() {
    let writer = Writer::new()
        .buffer(1, 36)
        // two instances' float4 colours
        .buffer(2, 32)
        .library(1)
        .pipeline_with(
            1,
            BGRA8,
            0,
            1,
            &[[0, FLOAT3, 0, 0], [1, FLOAT4, 0, 1]],
            &[[0, 12, STEP_VERTEX, 1], [1, 16, STEP_INSTANCE, 2]],
        )
        .window_pass()
        .command(op::SET_PIPELINE, &[1])
        .command(op::SET_VERTEX_BUFFER, &[0, 1, 0])
        .command(op::DRAW, &[TRIANGLE, 0, 3, 4])
        .command(op::SET_VERTEX_BUFFER, &[1, 2, 0])
        .command(op::DRAW, &[TRIANGLE, 0, 3, 4])
        .command(op::DRAW, &[TRIANGLE, 0, 3, 5]);
    assert_eq!(
        refused(writer),
        [RenderError::Unbound { slot: 1 }, bounds("instances")]
    );
}

#[test]
fn indexed_draws_stay_inside_their_indices() {
    let writer = drawing(36)
        .buffer(2, 12)
        .command(op::DRAW_INDEXED, &[TRIANGLE, 6, UINT16, 2, 0, 0, 1])
        .command(op::DRAW_INDEXED, &[TRIANGLE, 6, UINT16, 2, 2, 0, 1])
        .command(op::DRAW_INDEXED, &[TRIANGLE, 3, UINT16, 2, 1, 0, 1])
        .command(
            op::DRAW_INDEXED,
            &[TRIANGLE, 3, IndexType::Uint32.word(), 2, 0, 0, 1],
        )
        .command(op::DRAW_INDEXED, &[TRIANGLE, 3, UINT16, 3, 0, 0, 1])
        .command(op::DRAW_INDEXED, &[TRIANGLE, u32::MAX, UINT16, 2, 0, 0, 1]);
    assert_eq!(
        refused(writer),
        [
            bounds("indices"),
            value("index offset", 1),
            RenderError::Missing {
                kind: ObjectKind::Buffer,
                id: 3
            },
            bounds("indices"),
        ]
    );
}

#[test]
fn destroying_a_buffer_unbinds_it() {
    assert_eq!(
        refused(
            drawing(36)
                .command(op::DESTROY, &[ObjectKind::Buffer.word(), 1])
                .buffer(1, 36)
                .command(op::DRAW, &[TRIANGLE, 0, 3, 1])
        ),
        [RenderError::Unbound { slot: 0 }]
    );
}

#[test]
fn visibility_needs_a_buffer() {
    let pass = |visibility: u32| {
        Writer::new().buffer(1, 16).command(
            op::BEGIN_PASS,
            &[
                0,
                0,
                CLEAR,
                DONT_CARE,
                DONT_CARE,
                0,
                0,
                0,
                0,
                f(1.0),
                0,
                0,
                visibility,
            ],
        )
    };
    let boolean = Visibility::Boolean.word();
    assert_eq!(
        refused(
            pass(0)
                .command(op::SET_VISIBILITY, &[boolean, 0])
                .command(op::SET_VISIBILITY, &[Visibility::Disabled.word(), 0])
        ),
        [value("visibility", boolean)]
    );
    assert_eq!(
        refused(
            pass(1)
                .command(op::SET_VISIBILITY, &[boolean, 8])
                .command(op::SET_VISIBILITY, &[boolean, 4])
                .command(op::SET_VISIBILITY, &[boolean, 16])
        ),
        [value("visibility offset", 4), bounds("visibility offset")]
    );
}

// ---------- outside passes

#[test]
fn copies_stay_inside() {
    let base = || {
        Writer::new()
            .texture(1, RGBA8, 16, 16, 1, SAMPLED)
            .texture(2, RGBA8, 8, 8, 1, SAMPLED)
            .texture(3, BGRA8, 8, 8, 1, SAMPLED)
    };
    let copy = |fields: [u32; 12]| base().command(op::COPY_TEXTURE, &fields);
    assert_eq!(refused(copy([1, 0, 0, 8, 8, 2, 0, 0, 0, 0, 8, 8])), []);
    assert_eq!(
        refused(copy([1, 0, 0, 9, 8, 2, 0, 0, 0, 0, 8, 8])),
        [bounds("source")]
    );
    assert_eq!(
        refused(copy([1, 0, 0, 0, 0, 2, 0, 0, 1, 0, 8, 8])),
        [bounds("destination")]
    );
    assert_eq!(
        refused(copy([1, 0, 0, 0, 0, 3, 0, 0, 0, 0, 8, 8])),
        [mismatch("format")]
    );
    assert_eq!(
        refused(copy([1, 0, 0, 0, 0, 1, 0, 0, 8, 8, 8, 8])),
        [value("destination", 1)]
    );
}

#[test]
fn reads_stay_inside() {
    let base = || {
        Writer::new()
            .texture(1, RGBA8, 16, 16, 1, RT)
            .texture(2, DEPTH, 16, 16, 1, RT)
    };
    let read = |fields: [u32; 10]| base().command(op::READ_TEXTURE, &fields);
    assert_eq!(refused(read([1, 0, 0, 0, 0, 16, 16, GUEST, 64, 1024])), []);
    assert_eq!(
        refused(read([1, 0, 0, 0, 0, 16, 16, GUEST, 64, 1023])),
        [value("length", 1023)]
    );
    assert_eq!(
        refused(read([1, 0, 0, 0, 0, 16, 16, GUEST, 60, 1024])),
        [value("bytes per row", 60)]
    );
    assert_eq!(
        refused(read([1, 0, 0, 0, 0, 16, 16, GUEST - 1024, 64, 1024])),
        [RenderError::Guest {
            address: GUEST - 1024,
            length: 1024
        }]
    );
    assert_eq!(
        refused(read([2, 0, 0, 0, 0, 16, 16, GUEST, 64, 1024])),
        [value("format", DEPTH)]
    );
}

#[test]
fn mipmaps_need_levels() {
    assert_eq!(
        refused(
            Writer::new()
                .texture(1, RGBA8, 16, 16, 1, SAMPLED)
                .texture(2, RGBA8, 16, 16, 5, SAMPLED)
                .texture(3, BC1, 16, 16, 5, SAMPLED)
                .command(op::GENERATE_MIPMAPS, &[1])
                .command(op::GENERATE_MIPMAPS, &[2])
                .command(op::GENERATE_MIPMAPS, &[3])
        ),
        [value("levels", 1), value("format", BC1)]
    );
}

#[test]
fn vertex_formats_have_their_sizes() {
    let sizes = [
        (VertexFormat::Short, 2),
        (VertexFormat::Short3, 6),
        (VertexFormat::ShortNormalized, 2),
        (VertexFormat::Short3Normalized, 6),
        (VertexFormat::UcharNormalized, 1),
        (VertexFormat::Uchar2Normalized, 2),
        (VertexFormat::Uchar3Normalized, 3),
        (VertexFormat::Short4, 8),
        (VertexFormat::Float3, 12),
    ];
    for (format, size) in sizes {
        assert_eq!(format.size(), size, "{format:?}");
    }
}
