/* The macOS app's render commands (docs/macos_host.md, "The renderer"):
the guest's Direct3D 8 writes Metal-level commands in batches to a ring in
its memory, and the host checks each one (port/macos/runtime/src/render.rs)
before Metal sees it. The guest and the host both include this header; the
runtime's tests check that its values are the decoder's.

Everything is little-endian 32-bit words. A batch is a header and its
commands:

	word 0  OPENCE_RENDER_MAGIC
	word 1  OPENCE_RENDER_VERSION
	word 2  the batch's size in bytes, its header included
	word 3  the number of commands

A command's first word is its op and its size in words, itself included
(OPENCE_RENDER_COMMAND). Its fields follow, a word each, in the order
listed below; "f" marks a float, "i" a signed word, and the rest are
unsigned. A command the host refuses is skipped and logged, and the next
is read; a size that runs past the batch ends it.

Objects are named by the guest: an id from 1 to the kind's limit, made
once and destroyed before it is made again. Id 0 is none, and a pass on
colour texture 0 draws to the window.

As in Metal, the state a draw uses (SET_*) is set inside a pass, and a
pass begins with none of it set; copies, reads, mipmaps and presents are
outside passes. Objects may be made, filled and destroyed anywhere, except
a pass's own targets while it lasts. */

#ifndef OPENCE_RENDER_H
#define OPENCE_RENDER_H

#define OPENCE_RENDER_MAGIC 0x4252434fu /* "OCRB" */
#define OPENCE_RENDER_VERSION 1
#define OPENCE_RENDER_BATCH_HEADER_WORDS 4

#define OPENCE_RENDER_COMMAND(op, words) ((unsigned int)(op) | ((unsigned int)(words) << 16))
#define OPENCE_RENDER_COMMAND_OP(word) ((word) & 0xffffu)
#define OPENCE_RENDER_COMMAND_WORDS(word) ((word) >> 16)

/* ---------- objects */

/* id, length (bytes): a buffer, zeroed */
#define OPENCE_RENDER_OP_CREATE_BUFFER 1
#define OPENCE_RENDER_CREATE_BUFFER_WORDS 3
/* id, offset, guest address, length: guest memory copied into a buffer */
#define OPENCE_RENDER_OP_UPLOAD_BUFFER 2
#define OPENCE_RENDER_UPLOAD_BUFFER_WORDS 5
/* id, offset, guest address, length: as UPLOAD_BUFFER, but copied by the
host when it reads the command, not in the GPU's order. The guest promises
that no command before it in the frame reads the range; the host waits for
the GPU to finish the earlier frames that did. */
#define OPENCE_RENDER_OP_WRITE_BUFFER 29
#define OPENCE_RENDER_WRITE_BUFFER_WORDS 5
/* id, kind, format, width, height, depth, levels, usage, samples */
#define OPENCE_RENDER_OP_CREATE_TEXTURE 3
#define OPENCE_RENDER_CREATE_TEXTURE_WORDS 10
/* id, level, slice, x, y, z, width, height, depth, guest address,
bytes per row, bytes per image, length: guest memory copied into a region
of a texture (rows of blocks for the compressed formats) */
#define OPENCE_RENDER_OP_UPLOAD_TEXTURE 4
#define OPENCE_RENDER_UPLOAD_TEXTURE_WORDS 14
/* object kind, id */
#define OPENCE_RENDER_OP_DESTROY 5
#define OPENCE_RENDER_DESTROY_WORDS 3
/* id, guest address, length: Metal Shading Language source, with the
functions vertex_main and fragment_main */
#define OPENCE_RENDER_OP_CREATE_LIBRARY 6
#define OPENCE_RENDER_CREATE_LIBRARY_WORDS 4
/* id, vertex library, fragment library (0: none), colour format,
depth format (0: none), samples, blending (0 or 1), source colour factor,
destination colour factor, colour operation, source alpha factor,
destination alpha factor, alpha operation, write mask (OPENCE_RENDER_WRITE_*),
attributes, layouts; then each attribute (index, format, offset, buffer
slot) and each layout (buffer slot, stride, step, step rate) */
#define OPENCE_RENDER_OP_CREATE_PIPELINE 7
#define OPENCE_RENDER_CREATE_PIPELINE_WORDS 17
#define OPENCE_RENDER_PIPELINE_ATTRIBUTE_WORDS 4
#define OPENCE_RENDER_PIPELINE_LAYOUT_WORDS 4
/* id, depth compare, depth write (0 or 1), stencil (0 or 1); then the front
faces' and the back faces' stencil: fail, depth fail, pass, compare,
read mask, write mask */
#define OPENCE_RENDER_OP_CREATE_DEPTH_STENCIL 8
#define OPENCE_RENDER_CREATE_DEPTH_STENCIL_WORDS 17
/* id, min filter, mag filter, mip filter, address u, v, w,
max anisotropy (1 to 16), f min lod, f max lod, border colour */
#define OPENCE_RENDER_OP_CREATE_SAMPLER 9
#define OPENCE_RENDER_CREATE_SAMPLER_WORDS 12

/* ---------- passes and their state */

/* colour texture (0: the window), depth texture (0: none), colour load,
depth load, stencil load, f clear red, f green, f blue, f alpha,
f clear depth, clear stencil, resolve texture (0: none),
visibility buffer (0: none) */
#define OPENCE_RENDER_OP_BEGIN_PASS 10
#define OPENCE_RENDER_BEGIN_PASS_WORDS 14
#define OPENCE_RENDER_OP_END_PASS 11
#define OPENCE_RENDER_END_PASS_WORDS 1
/* pipeline */
#define OPENCE_RENDER_OP_SET_PIPELINE 12
#define OPENCE_RENDER_SET_PIPELINE_WORDS 2
/* depth-stencil state (0: none), stencil reference */
#define OPENCE_RENDER_OP_SET_DEPTH_STENCIL 13
#define OPENCE_RENDER_SET_DEPTH_STENCIL_WORDS 3
/* cull, front winding, fill, f depth bias, f slope scale, f clamp,
depth clip (0: clip, 1: clamp) */
#define OPENCE_RENDER_OP_SET_RASTER 14
#define OPENCE_RENDER_SET_RASTER_WORDS 8
/* f x, f y, f width, f height, f near, f far */
#define OPENCE_RENDER_OP_SET_VIEWPORT 15
#define OPENCE_RENDER_SET_VIEWPORT_WORDS 7
/* x, y, width, height: clipped to the pass's target */
#define OPENCE_RENDER_OP_SET_SCISSOR 16
#define OPENCE_RENDER_SET_SCISSOR_WORDS 5
/* f red, f green, f blue, f alpha */
#define OPENCE_RENDER_OP_SET_BLEND_COLOR 17
#define OPENCE_RENDER_SET_BLEND_COLOR_WORDS 5
/* slot, buffer (0: none), offset */
#define OPENCE_RENDER_OP_SET_VERTEX_BUFFER 18
#define OPENCE_RENDER_SET_VERTEX_BUFFER_WORDS 4
/* stage, slot, length (bytes); then the bytes, padded to a word: constants */
#define OPENCE_RENDER_OP_SET_BYTES 19
#define OPENCE_RENDER_SET_BYTES_WORDS 4
/* stage, slot, texture (0: none) */
#define OPENCE_RENDER_OP_SET_TEXTURE 20
#define OPENCE_RENDER_SET_TEXTURE_WORDS 4
/* stage, slot, sampler (0: none) */
#define OPENCE_RENDER_OP_SET_SAMPLER 21
#define OPENCE_RENDER_SET_SAMPLER_WORDS 4
/* primitive, first vertex, vertices, instances */
#define OPENCE_RENDER_OP_DRAW 22
#define OPENCE_RENDER_DRAW_WORDS 5
/* primitive, indices, index type, index buffer, index offset (bytes),
i base vertex, instances */
#define OPENCE_RENDER_OP_DRAW_INDEXED 23
#define OPENCE_RENDER_DRAW_INDEXED_WORDS 8
/* visibility (OPENCE_RENDER_VISIBILITY_*), offset in the pass's
visibility buffer (a multiple of 8) */
#define OPENCE_RENDER_OP_SET_VISIBILITY 24
#define OPENCE_RENDER_SET_VISIBILITY_WORDS 3

/* ---------- outside passes */

/* source texture, level, slice, x, y, destination texture, level, slice,
x, y, width, height. Here and in READ_TEXTURE a volume's slice is its z. */
#define OPENCE_RENDER_OP_COPY_TEXTURE 25
#define OPENCE_RENDER_COPY_TEXTURE_WORDS 13
/* texture, level, slice, x, y, width, height, guest address, bytes per row,
length: a region written to guest memory once the GPU has drawn it */
#define OPENCE_RENDER_OP_READ_TEXTURE 26
#define OPENCE_RENDER_READ_TEXTURE_WORDS 11
/* texture: its levels made from its first */
#define OPENCE_RENDER_OP_GENERATE_MIPMAPS 27
#define OPENCE_RENDER_GENERATE_MIPMAPS_WORDS 2
/* the window's frame is shown */
#define OPENCE_RENDER_OP_PRESENT 28
#define OPENCE_RENDER_PRESENT_WORDS 1

/* ---------- limits */

#define OPENCE_RENDER_MAX_BATCH_LENGTH 0x1000000 /* 16 MB, its header included */
#define OPENCE_RENDER_MAX_BUFFERS 16384
#define OPENCE_RENDER_MAX_TEXTURES 8192
#define OPENCE_RENDER_MAX_LIBRARIES 4096
#define OPENCE_RENDER_MAX_PIPELINES 8192
#define OPENCE_RENDER_MAX_DEPTH_STENCILS 1024
#define OPENCE_RENDER_MAX_SAMPLERS 1024
#define OPENCE_RENDER_MAX_BUFFER_LENGTH 0x10000000 /* 256 MB */
#define OPENCE_RENDER_MAX_LIBRARY_LENGTH 0x100000 /* 1 MB of source */
#define OPENCE_RENDER_MAX_TEXTURE_SIZE 16384
#define OPENCE_RENDER_MAX_TEXTURE_DEPTH 2048
#define OPENCE_RENDER_MAX_TEXTURE_LEVELS 15
#define OPENCE_RENDER_MAX_BYTES 4096 /* SET_BYTES */
#define OPENCE_RENDER_BUFFER_SLOTS 30
#define OPENCE_RENDER_TEXTURE_SLOTS 16
#define OPENCE_RENDER_SAMPLER_SLOTS 16
#define OPENCE_RENDER_MAX_ATTRIBUTES 16

/* ---------- values */

/* object kinds (DESTROY) */
#define OPENCE_RENDER_OBJECT_BUFFER 1
#define OPENCE_RENDER_OBJECT_TEXTURE 2
#define OPENCE_RENDER_OBJECT_LIBRARY 3
#define OPENCE_RENDER_OBJECT_PIPELINE 4
#define OPENCE_RENDER_OBJECT_DEPTH_STENCIL 5
#define OPENCE_RENDER_OBJECT_SAMPLER 6

/* texture kinds: a cube has six slices */
#define OPENCE_RENDER_TEXTURE_2D 1
#define OPENCE_RENDER_TEXTURE_CUBE 2
#define OPENCE_RENDER_TEXTURE_3D 3

/* texture usage, bits */
#define OPENCE_RENDER_USAGE_SAMPLED 1
#define OPENCE_RENDER_USAGE_RENDER_TARGET 2

/* pixel formats; the window's is BGRA8 */
#define OPENCE_RENDER_FORMAT_RGBA8 1
#define OPENCE_RENDER_FORMAT_BGRA8 2
#define OPENCE_RENDER_FORMAT_R8 3
#define OPENCE_RENDER_FORMAT_RG8 4
#define OPENCE_RENDER_FORMAT_B5G6R5 5
#define OPENCE_RENDER_FORMAT_BGR5A1 6
#define OPENCE_RENDER_FORMAT_ABGR4 7
#define OPENCE_RENDER_FORMAT_BC1 8
#define OPENCE_RENDER_FORMAT_BC2 9
#define OPENCE_RENDER_FORMAT_BC3 10
#define OPENCE_RENDER_FORMAT_RGBA16F 11
#define OPENCE_RENDER_FORMAT_DEPTH32F 12
#define OPENCE_RENDER_FORMAT_DEPTH32F_STENCIL8 13

/* vertex attribute formats */
#define OPENCE_RENDER_VERTEX_FLOAT 1
#define OPENCE_RENDER_VERTEX_FLOAT2 2
#define OPENCE_RENDER_VERTEX_FLOAT3 3
#define OPENCE_RENDER_VERTEX_FLOAT4 4
#define OPENCE_RENDER_VERTEX_UCHAR4_NORMALIZED 5
#define OPENCE_RENDER_VERTEX_UCHAR4_NORMALIZED_BGRA 6
#define OPENCE_RENDER_VERTEX_SHORT2 7
#define OPENCE_RENDER_VERTEX_SHORT4 8
#define OPENCE_RENDER_VERTEX_SHORT2_NORMALIZED 9
#define OPENCE_RENDER_VERTEX_SHORT4_NORMALIZED 10
#define OPENCE_RENDER_VERTEX_UCHAR4 11
#define OPENCE_RENDER_VERTEX_UINT 12
#define OPENCE_RENDER_VERTEX_SHORT 13
#define OPENCE_RENDER_VERTEX_SHORT3 14
#define OPENCE_RENDER_VERTEX_SHORT_NORMALIZED 15
#define OPENCE_RENDER_VERTEX_SHORT3_NORMALIZED 16
#define OPENCE_RENDER_VERTEX_UCHAR_NORMALIZED 17
#define OPENCE_RENDER_VERTEX_UCHAR2_NORMALIZED 18
#define OPENCE_RENDER_VERTEX_UCHAR3_NORMALIZED 19

/* vertex steps */
#define OPENCE_RENDER_STEP_VERTEX 1
#define OPENCE_RENDER_STEP_INSTANCE 2
#define OPENCE_RENDER_STEP_CONSTANT 3

/* blend factors */
#define OPENCE_RENDER_BLEND_ZERO 1
#define OPENCE_RENDER_BLEND_ONE 2
#define OPENCE_RENDER_BLEND_SOURCE_COLOR 3
#define OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_COLOR 4
#define OPENCE_RENDER_BLEND_SOURCE_ALPHA 5
#define OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_ALPHA 6
#define OPENCE_RENDER_BLEND_DESTINATION_COLOR 7
#define OPENCE_RENDER_BLEND_ONE_MINUS_DESTINATION_COLOR 8
#define OPENCE_RENDER_BLEND_DESTINATION_ALPHA 9
#define OPENCE_RENDER_BLEND_ONE_MINUS_DESTINATION_ALPHA 10
#define OPENCE_RENDER_BLEND_SOURCE_ALPHA_SATURATED 11
#define OPENCE_RENDER_BLEND_BLEND_COLOR 12
#define OPENCE_RENDER_BLEND_ONE_MINUS_BLEND_COLOR 13
#define OPENCE_RENDER_BLEND_BLEND_ALPHA 14
#define OPENCE_RENDER_BLEND_ONE_MINUS_BLEND_ALPHA 15

/* blend operations */
#define OPENCE_RENDER_BLEND_ADD 1
#define OPENCE_RENDER_BLEND_SUBTRACT 2
#define OPENCE_RENDER_BLEND_REVERSE_SUBTRACT 3
#define OPENCE_RENDER_BLEND_MIN 4
#define OPENCE_RENDER_BLEND_MAX 5

/* colour write mask, bits */
#define OPENCE_RENDER_WRITE_RED 1
#define OPENCE_RENDER_WRITE_GREEN 2
#define OPENCE_RENDER_WRITE_BLUE 4
#define OPENCE_RENDER_WRITE_ALPHA 8

/* comparisons */
#define OPENCE_RENDER_COMPARE_NEVER 1
#define OPENCE_RENDER_COMPARE_LESS 2
#define OPENCE_RENDER_COMPARE_EQUAL 3
#define OPENCE_RENDER_COMPARE_LESS_EQUAL 4
#define OPENCE_RENDER_COMPARE_GREATER 5
#define OPENCE_RENDER_COMPARE_NOT_EQUAL 6
#define OPENCE_RENDER_COMPARE_GREATER_EQUAL 7
#define OPENCE_RENDER_COMPARE_ALWAYS 8

/* stencil operations */
#define OPENCE_RENDER_STENCIL_KEEP 1
#define OPENCE_RENDER_STENCIL_ZERO 2
#define OPENCE_RENDER_STENCIL_REPLACE 3
#define OPENCE_RENDER_STENCIL_INCREMENT_CLAMP 4
#define OPENCE_RENDER_STENCIL_DECREMENT_CLAMP 5
#define OPENCE_RENDER_STENCIL_INVERT 6
#define OPENCE_RENDER_STENCIL_INCREMENT_WRAP 7
#define OPENCE_RENDER_STENCIL_DECREMENT_WRAP 8

/* sampler filters (mip: none, nearest or linear) */
#define OPENCE_RENDER_FILTER_NONE 0
#define OPENCE_RENDER_FILTER_NEAREST 1
#define OPENCE_RENDER_FILTER_LINEAR 2

/* sampler addressing */
#define OPENCE_RENDER_ADDRESS_REPEAT 1
#define OPENCE_RENDER_ADDRESS_MIRROR 2
#define OPENCE_RENDER_ADDRESS_CLAMP_TO_EDGE 3
#define OPENCE_RENDER_ADDRESS_CLAMP_TO_BORDER 4

/* sampler border colours */
#define OPENCE_RENDER_BORDER_TRANSPARENT_BLACK 1
#define OPENCE_RENDER_BORDER_OPAQUE_BLACK 2
#define OPENCE_RENDER_BORDER_OPAQUE_WHITE 3

/* attachment loads */
#define OPENCE_RENDER_LOAD_DONT_CARE 1
#define OPENCE_RENDER_LOAD_LOAD 2
#define OPENCE_RENDER_LOAD_CLEAR 3

/* culling, winding, fill */
#define OPENCE_RENDER_CULL_NONE 1
#define OPENCE_RENDER_CULL_FRONT 2
#define OPENCE_RENDER_CULL_BACK 3
#define OPENCE_RENDER_WINDING_CLOCKWISE 1
#define OPENCE_RENDER_WINDING_COUNTER_CLOCKWISE 2
#define OPENCE_RENDER_FILL_SOLID 1
#define OPENCE_RENDER_FILL_LINES 2

/* shader stages */
#define OPENCE_RENDER_STAGE_VERTEX 1
#define OPENCE_RENDER_STAGE_FRAGMENT 2

/* primitives: fans and line loops are made lists by the guest */
#define OPENCE_RENDER_PRIMITIVE_POINT 1
#define OPENCE_RENDER_PRIMITIVE_LINE 2
#define OPENCE_RENDER_PRIMITIVE_LINE_STRIP 3
#define OPENCE_RENDER_PRIMITIVE_TRIANGLE 4
#define OPENCE_RENDER_PRIMITIVE_TRIANGLE_STRIP 5

/* index types */
#define OPENCE_RENDER_INDEX_UINT16 1
#define OPENCE_RENDER_INDEX_UINT32 2

/* visibility results: none, any samples passed, or how many */
#define OPENCE_RENDER_VISIBILITY_DISABLED 1
#define OPENCE_RENDER_VISIBILITY_BOOLEAN 2
#define OPENCE_RENDER_VISIBILITY_COUNTING 3

#endif
