# `rasterizer_geometry.obj` Jonas scalar-codec wave

This Jonas-owned ledger records one bounded natural-C experiment for six
scalar codec functions in `source/rasterizer/rasterizer_geometry.obj`. Two
decoders were strict on the first compile and are retained. Four compressors
were not strict and were withdrawn without source-shape or code-generation
tuning. The object advances from 1/19 to 3/19 strict functions and remains
`NonMatching`; no data or completion claim is made.

## Scope and provenance

- Authoritative integration base:
  `e1e95eecc4dcb2d0d9946f4af868b1c277d470da`.
- Baseline `rasterizer_geometry.c` blob:
  `8f2d9fd25dc01f204bd0b91facea4422f1b03dbb`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `rasterizer_geometry.obj` SHA-256:
  `d6357486072be1c8fb98fed92bc36eb0c1d6c7e68386df5c9ce164f0627ebb49`.
- HCEA supplies independent `byte`/`short` to `float` signatures and scalar
  formulas. January COFF is the sole byte, literal, x87, extent, relocation,
  and ownership authority.
- Only `source/rasterizer/rasterizer_geometry.c` and this new ledger change.
  No header changes: the retained definitions have authentic signatures, and
  there is no surviving repository consumer that proves a shared prototype is
  presently needed.

The byte decoder zero-extends its input and multiplies by the single-precision
`1/255` literal, bit pattern `0x3B808081`. The short decoder sign-extends its
input, evaluates `value*2.0f + 1.0f`, and multiplies by the single-precision
`1/65535` literal, bit pattern `0x37800080`. January and the retained compile
use the same x87 instruction order and literal relocations.

## Bounded natural experiment

The first compile contained exactly the requested six candidates. The two
decoders were strict immediately. The remaining four were removed after that
measurement, with no alternative expression, declaration, cast, helper,
control-flow, or compiler-directive experiment:

- `_compress_real_to_int8`: candidate 80 padded bytes and eight relocations;
  target 96 and seven.
- `_compress_real_to_int8_clamp`: candidate 80 padded bytes and eight
  relocations; target 80 and five.
- `_compress_real_to_int16`: candidate 96 padded bytes and nine relocations;
  target 112 and eight.
- `_compress_real_to_int16_clamp`: candidate 80 padded bytes and seven
  relocations; target 96 and six.

All four natural compressor bodies selected the external `__ftol2` conversion
helper, while January uses an inline x87 `fistp` conversion. No rejected
definition or reference remains in the final candidate object.

## Strict code result

The untouched baseline measured 1/19 exact functions and one meaningful code
byte. The retained source measures 3/19 and 51 meaningful bytes: a gain of two
functions, 50 meaningful bytes, 64 padded bytes, and three relocations.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_uncompress_int8_to_real` | 21 | 32 | 1 | `c8d445f22bb94140d08cc78f9d312c2d55474343075bc7ddcf3caa8d620da8ca` |
| `_uncompress_int16_to_real` | 29 | 32 | 2 | `55efeed54cdddbcddd62789b26e398aa30207db3dd5e87fa475a2787fc7b9886` |

Both retained functions have identical normalized bytes, padded extents,
relocation addresses/types/destinations, and unique external function owners.
The pre-existing `_rasterizer_geometry_byte_swap_vertices` remains exact at
1/16 bytes with no relocations and normalized SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.

## Data and source policy

The final candidate emits three compiler-owned four-byte float-literal COMDATs
used by the retained functions. It emits no `.bss`, `.data`, COMMON owner, or
source-defined global. The literals receive no data credit here, and the
target object's 864 data bytes remain unclaimed.

The retained bodies are readable, typed, defined C with one parameter per
line and explicit terminal returns. There is no assembly, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, raw address
reconstruction, pointer/integer cast, union pun, signed-overflow match,
synthetic anchor, or byte-forcing construct.

## Reproducibility and gates

- Final tracked source blob:
  `78681b2c0a7be4fdb91dc198bf427f104b6f6b6c`.
- Final source SHA-256:
  `74f30874e678be1a8adc6e87c7c0fea524c2b921b615d805db54371ce5f15c04`.
- Complete Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,992 functions evaluated, 3,852 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,913 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,900/11,060 exact functions,
  472,190/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Admission reports zero candidates and zero revocations, with only the
  inherited `source/shell/shell_xbox` completion-label contradiction.
- Park validation reports three active, zero stale, and zero invalid entries.
- The complete tooling suite passes 179/179 tests.
- A clean committed regression snapshot/check and forced object replay
  preserve all three exact functions, their relocation identities, the absent
  rejected compressor symbols, and the no-global-storage candidate shape.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No configuration or pre-existing
Markdown file is changed, no tracked file is deleted, and no push is
performed.
