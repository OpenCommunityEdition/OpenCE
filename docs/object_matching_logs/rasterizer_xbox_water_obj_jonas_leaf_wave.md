# `rasterizer_xbox_water.obj` Jonas visibility-leaf wave

This Jonas-owned ledger records one bounded candidate wave in
`source/rasterizer/xbox/rasterizer_xbox_water.obj`. The object advances from
0/14 to 2/14 strict functions and remains `NonMatching`. No pre-existing or
Claude-authored Markdown file is edited.

## Scope and provenance

- Authoritative integration base:
  `6efbc916e2f34b9ad35e4d4d7dacf5aec68f9748`.
- Baseline source blob: `af54cdab5ecb0d0c3a6be9c1f4ba50e577e9200b`.
- January split-object SHA-256:
  `0d6480e2b845e331b0919dcd5e7d49ba2e11987d54634102c7435353bceaca37`.
- HCEA applied-source oracle commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`.
- HCEA supplies typed behavior: the frame setter records visibility and derives
  the update flag from zero, the window setter stores an unsigned byte, and the
  getter returns that byte. January PDB2.00 independently labels the three
  cdecl functions in `.text` at `0x00168AF0`, `0x00168B10`, and `0x00168B20`.
  January COFF disassembly further proves one low-byte stack argument for each
  setter, an AL byte return for the getter, and the two-byte global layout.
- Only `source/rasterizer/xbox/rasterizer_xbox_water.c` and this new ledger are
  changed. There is no shared-header, configuration, frozen-object, or
  existing-Markdown edit.

## Bounded experiment

The single natural candidate compile covered all three visibility leaves. The
window setter and getter were strict immediately. The frame setter had the
target's 32-byte padded extent and the same two global destinations, but its
natural typed C emitted a different store/register schedule: candidate
relocations were at offsets 9 and 17, versus target offsets 13 and 18. Its
definition was removed without a source-shape or code-generation tuning pass.

## Strict result

The final object measures 2/14 strict functions and 19 meaningful code bytes,
a gain of two functions, 19 meaningful bytes, 32 padded bytes, and two
relocations.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_water_set_visibility_for_window` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_rasterizer_water_get_visibility_for_window` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |

For both retained functions, target and candidate have identical normalized
bytes, padded extent, relocation address/type/destination, and one unique
external function owner with matching COFF type and storage class. The target's
286 non-code bytes remain uncredited.

## Typed state and source policy

The two booleans are represented as a named two-byte aggregate, with
`needs_update` at `+0` and `visible_for_window` at `+1`; compile-time checks
hold the size and both offsets. The candidate emits the aggregate as one
external COMMON owner, `_bss_004662e8`, of size two. It emits no `.data`,
`.rdata`, or file-backed BSS payload and makes no data-matching claim.

The retained implementation is readable, defined C with one parameter per
line, explicit `void` for the getter's empty parameter list, and an explicit
terminal return in the setter. It contains no assembly, volatile scheduling
device, forced inline, optimizer pragma/barrier, raw address reconstruction,
pointer/integer cast, union pun, signed-overflow match, synthetic anchor, or
byte-forcing device.

## Reproducibility and gates

- Untouched baseline object SHA-256:
  `d9ba62ee6e908dbc934455cef32c08fa532f27ff604dc19896b5523bfe42bbf2`.
- Final forced-replay object SHA-256 at this checkpoint:
  `71ccbed6ccd722e93a03517285ab00fe1b1df5b29fac0d434b31b2b590bd9135`.
  VC7 stamps the raw COFF header on each invocation, so strict acceptance uses
  the stable normalized function hashes and relocation identities above.
- Final source SHA-256:
  `e9cc2c8ad57c84ee5fb41caf122e7f8b0b195d2dc7f87b2bc3f5565ef5e149e2`.
- Complete Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,968 functions evaluated, 3,828 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,889 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,876/11,060 exact functions,
  471,583/2,198,102 exact code bytes, and 1,834,808/4,176,062 matched data
  bytes.
- Admission reports zero candidates and zero revocations, with only the
  inherited `source/shell/shell_xbox` completion-label contradiction.
- Park validation reports three active, zero stale, and zero invalid entries.
- The complete tooling suite passes 179/179 tests.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no
GitHub push is performed.
