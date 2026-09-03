# `bitmap_drawing.obj` reconstruction checkpoint

Date: 2026-09-03

## Result

The two previously unwritten public functions are now present as typed,
source-authentic C:

- `_bitmap_tile_and_bevel_rectangle` is strict exact: 1,056 padded bytes,
  12 relocations, and normalized SHA-256
  `8c6bafe8780c1502f173bee8fc6f3b81cddd8a560a27ec0ebf83ff4840ab294d`.
- `_bitmap_copy` is a complete semantic reconstruction and is fuzzy-parked at
  2,832 candidate bytes versus 2,784 target bytes, with the same 69-relocation
  count and 78.528175 percent ordinary objdiff similarity.

The focused gate is therefore **6 exact / 1 residual / 0 unwritten**. The
strict gain from this checkpoint is the 1,056-byte bevel/tile function; no
match credit is assigned to the copy residual.

## Sources and semantic evidence

The copy path was reconstructed from the January target disassembly, its
assertion strings and line numbers, the exact 18 x 18 x 4 translation table,
and the independently compiled Halo Editing Kit Sapien implementation at
`research/halo_hek_oracle/extracted/sapien.exe` (function range
`0x6C4680`-`0x6C51C9`, table at `0xA8CCC0`). All thirteen translation cases
were audited. Copy flag bit 0 selects blending and bit 1 selects modulation.
The apparently unused call to `bitmap_format_get_bits_per_pixel` is present in
the January target and is deliberately retained.

The bevel/tile path was decoded from January's control flow and data. Its
nine-part sequence map is `{0,1,1,1,1,2,2,2,2}` and its nine clipping masks
are `{64,53,54,57,58,36,33,40,34}`. The latter was given the semantic private
name `bitmap_bevel_translation_flags`; `config/symbols.json` records the
corresponding target data boundary so the relocation owner is represented
truthfully. The existing PDB-backed public name
`bitmap_bevel_sequence_indices` is retained for the sequence map.

The target has sixteen unidentified data bytes immediately before the public
`translation_table`. They have no proved PDB name or code reference. No fake
source global was invented for them.

## Interface and house-rule audit

- Public declarations live in `bitmaps/bitmap_drawing.h`.
- `bitmap_format_get_string` is declared in the narrow interface owned by
  `bitmaps.c`, `bitmaps/bitmaps_internal.h`, rather than in a foreign `.c`.
- Typed bitmap tag and tag-block access use the subsystem accessors/macros.
- Tile flag tests and writes use `TEST_FLAG`, `FLAG`, and `SET_FLAG`.
- Parameters are vertically formatted and both functions end in explicit
  `return;` statements.
- No inline function, pragma, volatile/register steering, assembly, raw
  address, representation pun, fake dependency, or address-derived private
  identifier is retained.

The bit masks inside `_bitmap_copy` are pixel-format packing operations, not
manual flag handling; replacing them with flag macros would obscure the
actual conversion formulas.

## Copy residual and rejected false lead

January and the candidate agree on the complete call topology, assertion
topology, translation table, switch case order, and relocation count. Three
conversion cases are instruction-shape identical after the pervasive
ESI/EDI role exchange; the remaining gap is concentrated in VC7 register and
spill choices in the blend cases. January uses a 0x48-byte frame while the
candidate uses 0x4C.

Two natural improvements were retained: explicit source-pixel temporaries in
the two plain copy loops prevent VC7 from merging the two cursor induction
variables, and declaring alpha values before the destination pixel improves
the four blend cases. A tempting declaration-order experiment reduced the
candidate from 2,832 to 2,816 bytes, but it introduced exactly the merged
`sub edi, esi` / `[edi+esi]` loop that January does not contain. It was
rejected rather than claiming misleading size progress.

Other measured variants included reversed cursor declaration order in every
case, swapped commutative blend addends, inline destination reads, const
source cursors, split cursor increments, indexed loops, reordered alpha-fill
terms, and an implicit `bitmap_format_get_string` declaration. None supplied
an authentic closure; the implicit declaration also introduces C4013 and was
rejected.

The fail-closed classifier returns `UNKNOWN`, class `unclassified`, confidence
`NONE`. Reopen only for authoritative January source/local records or a
natural same-compiler donor that explains the register/allocation schedule.

## Validation

- Full `ninja`: pass, including split, objdiff, semantic audit, and progress.
- Focused gate: 6 exact / 1 residual / 0 unwritten.
- Direct gates for every other `bitmaps_internal.h` includer preserve their
  prior exact/residual/unwritten sets (`bitmap_group`, `bitmap_extract`,
  `bitmap_utilities`, `main`, and `rasterizer_text`).
- Strict before/after snapshot: one function and 1,056 bytes gained, zero
  regressions; board 289/619 objects, 863,891/1,922,413 padded code bytes,
  and 5,670/8,245 functions.
- Tool tests: 261 passed; semantic audit: zero unit errors; fake-match scan:
  zero review leads; parked manifest: 167 active, zero stale or invalid.
- The final object emits no `point_from_line3d` symbol/COMDAT.
- Exact tile relocation identities and normalized bytes: pass.
- `_bitmap_copy`: target 2,784 / 69 /
  `d8154826cc75922608a282d952b0f3c6e517e4074599f218e01160c4f18033b6`;
  base 2,832 / 69 /
  `7a67b27bb76f74be0c497b87fc57389334cb3362913dbd9a12d4564af47921ef`.
