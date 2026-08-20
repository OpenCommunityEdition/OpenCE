# rasterizer_transparent_geometry.obj Codex checkpoint

## Scope and provenance

This is an honest partial checkpoint. `source/rasterizer/rasterizer_transparent_geometry`
remains `NonMatching`; neither `config/config.json` nor the parked-function ledger is
changed.

The reconstruction was recovered from the object-specific source history in
`claude/near-complete-objects-20260816`, specifically commits `2d0b1956`,
`f2b75a73`, `d57def09`, and `6d5fe559`. Only the C source was transplanted onto
canonical `a5f816f2`. Claude-authored Markdown and the branch's unrelated history
were not copied, edited, or deleted.

The imported source was then tightened before admission:

- the two emitted-but-nonexact definitions, `rasterizer_transparent_geometry_initialize`
  and `rasterizer_transparent_geometry_next_group`, were removed;
- `struct vertex_buffer` is read through its typed `type` member from
  `rasterizer_geometry.h`, not by reinterpreting the pointer as `short *`;
- the combined 0x4A-byte runtime state remains one packed object so its offsets and
  common-symbol size stay fixed, while source aliases preserve January's attested
  identifier spelling in assertion strings.

## Strict result

The forced XDK 3911 build emits 13 code sections. Every emitted section is strict
semantic-COFF exact: padded size, normalized bytes, relocation count, relocation
types, symbolic destinations, and addends all agree.

| Function | Padded bytes | Relocations |
| --- | ---: | ---: |
| `_rasterizer_transparent_geometry_begin` | 48 | 5 |
| `_rasterizer_transparent_geometry_end` | 16 | 0 |
| `_rasterizer_transparent_geometry_get_primary_vertex_type` | 112 | 7 |
| `_rasterizer_transparent_geometry_get_group_presorted_index` | 192 | 12 |
| `_rasterizer_transparent_geometry_get_group_from_presorted_index` | 80 | 6 |
| `_rasterizer_transparent_geometry_get_groups2` | 32 | 2 |
| `_rasterizer_transparent_geometry_get_groups` | 16 | 1 |
| `_rasterizer_transparent_geometry_new_group` | 48 | 3 |
| `_rasterizer_transparent_geometry_new_group2` | 48 | 3 |
| `_rasterizer_transparent_geometry_stop` | 16 | 1 |
| `_rasterizer_transparent_geometry_dispose` | 128 | 15 |
| `_rasterizer_transparent_geometry_set_group_pending_status` | 96 | 4 |
| `_rasterizer_transparent_geometry_get_group_pending_status` | 64 | 2 |

Total strict code is 13/19 functions, 896/2,304 padded bytes, 818/2,174
meaningful bytes, and 61 strict relocations. The `a5f816f2` baseline had only
`begin` and `end`, so this checkpoint adds 11 strict functions and 770 meaningful
code bytes without losing either baseline-exact function.

The candidate retains the exact 74-byte common-BSS size and layout for
`_bss_004b8ad8`. Its five emitted production string COMDATs are target-identical;
the strings owned only by the six unimplemented target functions remain absent.
The ordinary report therefore stays partial and does not claim whole-data or
whole-object completion.

## Remaining target functions

Six target functions remain deliberately absent:

- `_rasterizer_transparent_geometry_initialize` — 160 padded bytes / 16 relocs;
- `_rasterizer_transparent_geometry_next_group` — 160 / 12;
- `_code_00173f70` — 48 bytes;
- `_code_00173fa0` — 384 bytes;
- `_code_00174120` — 176 bytes;
- `_rasterizer_transparent_geometry_draw` — 480 bytes.

The first two had readable implementations on the Claude branch but were not
strict. `initialize` is an exact-size/relocation block-placement tie.
`next_group` is an exact-size/relocation epilogue scheduling tie; the already
measured result-variable, flat early-return, nested early-return, and pre-cast
increment forms did not close it.

The final four are one interlocked cluster. The comparator uses still-unnamed
fields at offsets 0x08, 0x70, and 0x9D; the sort driver references it and emits
the private assert helper; `draw` calls the driver. They should be reopened only
when source/PDB/oracle evidence supplies those field identities. Guessing names
would contaminate four functions at once.

## Gates

- forced clean XDK object rebuild: pass;
- hardened whole-emitted-function comparison: 13 exact, 0 emitted residuals;
- full Halo and libcmt build graphs: pass;
- semantic report: 470 units, 0 unit errors;
- object-admission audit: 0 candidates, 0 revocations; only the pre-existing
  unrelated `shell_xbox` contradiction remains;
- config remains `NonMatching`;
- house/Berth scan: no asm, volatile, forceinline, optimizer pragma, barrier,
  byte forcing, or raw pointer reinterpretation introduced;
- the five user-frozen large objects and all Claude-authored Markdown are outside
  this checkpoint's diff.
