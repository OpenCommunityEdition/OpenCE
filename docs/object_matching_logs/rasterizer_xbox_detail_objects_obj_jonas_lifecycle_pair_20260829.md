# `rasterizer_xbox_detail_objects.obj`: lifecycle pair (2026-08-29)

## Outcome

This bounded ordinary-C wave adds strict-exact implementations of
`_rasterizer_detail_objects_initialize` and
`_rasterizer_detail_objects_dispose`. Together with the inherited exact
`__rasterizer_detail_objects_end`, the object advances from 1/20 to 3/20
strict functions and from 16/4,240 to 240/4,240 padded code bytes. The two
new bodies contribute 204 meaningful bytes and 224 padded bytes.

The four-byte `_bss_0045e904` owner is also exact. It is emitted as an
external zero-initialized `.bss` definition, matching January's symbol value,
storage class, section size, flags, and zero-relocation payload. The object
remains incomplete and `NonMatching`; no completion label, exception, park,
or configuration change is made.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_detail_objects_initialize` | 102 | 112 | 11 | `ca526d22c4421d669111f7f6dcea12a9c054facbf249e900ab9298855bf3b2a2` |
| `_rasterizer_detail_objects_dispose` | 102 | 112 | 13 | `b1f5238b6c17e493aa677bf21cceaa32fac48f935eb406e9af5a47d4f908680f` |

## Evidence and source model

The clean starting commit is
`93e1dd3e0c7413113af3504137e814db21600054`. The January split object is
SHA-256
`d1a307f55b0170ff7e25427aee3a7ca9eb1b9498f821f40c826a4a4a9f023a46`.
The pre-wave source blob is
`a85a581d14715aed890d8dc2e9972579151a83a1`; the retained source blob is
`ba1129489dc0141349a96046be465c98245a44dd`.

January COFF fixes both public zero-argument signatures, the complete padded
extents, all ordered relocations, and the four-byte BSS owner. Its instruction
and relocation topology proves the following behavior:

- initialize asserts `global_d3d_device` at source line `0x62`, creates a
  `0x20000`-byte dynamic/write-only default-pool vertex buffer, returns true
  on a nonnegative result, and reports both the D3D failure and the silent
  subsystem error before returning false;
- dispose asserts `local_d3d_vertex_buffer` and `global_d3d_device` at lines
  `0x77` and `0x78`, releases the typed vertex buffer through the SDK wrapper,
  clears the pointer, and explicitly returns.

The complete Stian history donor independently preserves the same lifecycle
meaning, D3D constants, assert expressions and line metadata, error strings,
and release/clear topology. Its raw-address storage spelling was rejected;
the retained code uses typed `D3DDevice` and `D3DVertexBuffer` pointers, an
external device declaration, a real zero-initialized BSS owner, and an alias
only to preserve January's asserted identifier spelling. HCEA's later-platform
detail-object lifecycle stubs corroborate the public signatures but are not
used as byte authority.

## Bounded compiler-shape record

The disposer was strict on the first code-producing batch. Initialize had the
target size and relocation identities throughout, but its boolean lifetime
needed the source-authentic form:

- preinitializing success to true kept EBX live across the whole function and
  placed its save before the device assert;
- a positive early return rematerialized false after the error calls and
  omitted January's failure-only EBX save;
- assigning the comparison directly emitted `setge`;
- assigning true and false in explicit `if`/`else` arms produced January's
  success early return and failure-only EBX lifetime exactly.

The SDK `IDirect3DVertexBuffer8_Release` wrapper resolves naturally to
`_D3DResource_Release@4` and avoids a conflicting hand-written prototype.
Replacing numeric usage flags with `D3DUSAGE_DYNAMIC |
D3DUSAGE_WRITEONLY` preserved the exact immediate and improved source intent.

## Validation and policy

- Direct hardened comparison: all three emitted functions are strict exact;
  the two new functions match every padded byte and every relocation address,
  type, destination, and addend.
- Per-unit gate: 3 exact, 0 residual, 17 unwritten.
- Full Halo and libcmt build graphs: pass.
- Strict board: 274/619 objects, 629,207/1,922,669 padded code bytes, and
  4,594/8,246 strict functions after park and assembly exclusions.
- Campaign progress: 575,147/2,198,102 meaningful code bytes and
  4,586/11,060 functions overall; Halo contributes 562,233/1,770,166 bytes
  and 4,419/7,574 functions. Data is 1,848,118/4,176,062 bytes, including
  this object's exact four-byte BSS owner.
- Semantic audit: 470 units, 4,735 functions evaluated, 4,613 semantic exact,
  4,623 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parks: 13 active, zero stale, zero invalid.
- Tool suite: 205 tests pass.

The retained source is readable typed C with no-argument `void` formatting,
one parameter per line, and explicit terminal returns. It contains no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, barrier, raw
address or byte offset, pointer/integer reconstruction, type pun, undefined
behavior, byte forcing, compiler-option forcing, object patch, or comparator
exception. Units, Vehicles, protected AI sources, and `game_engine.c` are
untouched.
