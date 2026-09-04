# `rasterizer_swizzle.obj` dirty Opus ownership re-audit (2026-09-04)

## Result

The interrupted Opus packet contains no code or data frontier beyond current
canonical commit `f62f603b7dc76e79ad2da4fe9de71141af5f3870`. Its two late source-shape
fixes were already integrated, and every one of its 13 candidate function
sections is identical to canonical after compilation through the current
gate. The unit therefore remains 12/13 strict exact: 3,328/4,128 padded code
bytes, 3,252/4,043 meaningful code bytes, and 476/476 reported data bytes.

The re-audit did find and correct a hidden ownership error in canonical's
12-byte private BSS packet. January uses the packet as z mask at +0, y mask at
+4, and x mask at +8. The previously chosen `global_swizzle_*_mask` source
identifiers made VC7 allocate x at +0, z at +4, and y at +8. Static-symbol
renaming let the per-function comparator treat those references as equal, so
the bad physical ownership did not lower the reported exact count. The
corroborated subsystem names `rasterizer_swizzle_z_mask`,
`rasterizer_swizzle_y_mask`, and `rasterizer_swizzle_x_mask` naturally emit
January's z/y/x layout and preserve all 12 exact functions.

This is consequently a 12-byte BSS ownership repair, not a newly claimed
code match. Rename-stable campaign gain is zero strict functions, zero padded
code bytes, zero meaningful code bytes, and zero reported data bytes. There
are zero whole-tree regressions.

## Scope, provenance, and authority

The clean baseline is canonical commit
`f62f603b7dc76e79ad2da4fe9de71141af5f3870`. The read-only donor is
`C:\halo-worktrees\opus-small-families-30k-20260902` at commit
`eeedd72e5ba0fc1761519f68cad241fe606320e7`, with one dirty source file:
`source/rasterizer/rasterizer_swizzle.c`.

Relevant identities are:

- canonical baseline source blob: `72359046f67630a0a0f47c7a2bbdfb16a6f499c9`;
- canonical baseline source SHA-256:
  `B71A6FD34896B1C4C983BD49CF310D98CE6EF07EB476FD7A2B1D01E6425A09C9`;
- donor committed-tip source blob:
  `7c1c34136bd09b07a68337d145d4663e063fe96c`;
- donor dirty source blob: `8b507106a9df3063317ea7e874c4e09ae5609685`;
- donor dirty source SHA-256:
  `3DAAF425C9D0058B3F002C18C9FC4A3BC05ADCA1A3B784C272CCA42AD5A70E54`;
- accepted source blob after the ownership correction:
  `20b920352bbe15744c4911b7a597f0a03a00bd5c`;
- accepted source SHA-256:
  `D84377FE468F6F536EE8F5A3F1BE8CECB963F5AD7CCB6E208C047D4C461A2956`.

January remains authoritative. Its `cachebeta.exe` has SHA-256
`4CC87B45F721270392A96F1674ED2B5CD4A7BB4355FAEAB4531D1CF1884D9520`.
The candidate compiler is XDK 3911 VC7; `xbox/bin/vc7/CL.Exe` has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`.
The independent HCEA source used only for semantic corroboration was read at
commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`.

Before editing, the complete matching methodology, exact-match acceleration
playbook, campaign throughput lessons, object-ledger README, campaign-tool
README, both prior `rasterizer_swizzle.obj` ledgers, and the Opus unfinished
session backlog were read. No applicable `CLAUDE.md` or `AGENTS.md` exists in
the canonical, donor, or ancestor path. The complete current source, owner
headers, donor source, and relevant donor probes were also audited.

## Donor disposition

The dirty donor's only changes relative to its own committed tip are:

- a block-scoped `padding` local in
  `rasterizer_xbox_bitmap_get_pixel_data_size`;
- shared post-branch stores in `bitmap_swizzle_vector2d`.

Both forms already exist in canonical. Rebuilding the dirty donor through the
current one-TU gate returns the same 12 exact / 1 residual / 0 unwritten
frontier. A normalized per-function COFF comparison found all 13 donor and
canonical candidate function sections identical, including the residual.

The donor's remaining differences were rejected:

- old short-form include paths instead of the current owned include paths;
- omission of `bitmaps_mipmap.h` and `rasterizer_swizzle.h`;
- caller-local public prototypes whose declarations already live with their
  owners;
- a duplicated local `NUMBER_OF_FACES_PER_CUBE_MAP` constant instead of the
  project-owned `NUMBER_OF_FACES_PER_CUBE` constant.

Those forms add no bytes and regress declaration ownership or project naming.
No donor header, prototype, constant, or source body was imported.

## BSS ownership evidence

The unsymbolized January split object owns one 12-byte BSS block formerly
named `_bss_004b82b0`. Relocation addends and the axis-specific uses in
`compute_swizzle_masks` and the 2D/3D copy loops establish its layout without
relying on an invented symbol:

- z mask: base +0;
- y mask: base +4;
- x mask: base +8.

Before this correction, canonical's three separate
`global_swizzle_*_mask` declarations compiled to x at +0, z at +4, and y at
+8. Renaming January's three addresses in `config/symbols.json` to the same
semantic identifiers made relocation identity comparisons succeed, but it
did not change those base-object offsets. That was a comparator-visible name
match over a physically wrong owner mapping.

Several bounded, recorded checks ruled out a structural workaround:

- all six declaration orders of three separate `global_swizzle_*_mask`
  scalars compiled to the same wrong x/z/y layout;
- a natural z/y/x aggregate struct restored the physical order but changed
  VC7 alias analysis, leaving only 5 exact and 8 residual functions;
- short or differently prefixed semantic spellings produced other physical
  orders and supplied no independent naming evidence.

HCEA independently names the x owner `rasterizer_swizzle_x_mask`. Applying
that subsystem convention consistently to the y and z owners gives the
ordinary private names retained here. With those names VC7 emits z at +0, y
at +4, and x at +8, exactly matching January, while the unit remains 12 exact
and 1 residual. `config/symbols.json` now assigns the same semantic names to
the corresponding January addresses. There are no `bss_<address>` aliases,
raw offsets, aggregate puns, volatile/register tricks, dummy dependencies,
barriers, pragmas, or assembly in this correction.

After regeneration, target and candidate both expose three four-byte static
BSS owners at z +0, y +4, and x +8. The 12-byte BSS sections compare exactly.
The 140-byte read-only owner packet containing `swizzle_table` and
`face_mapping_inverse_table` also remains exact. The progress report was
already crediting the entire unit's 476 data bytes before this correction,
so this more accurate ownership does not manufacture a scoreboard gain.

## Retained fuzzy boundary

`rasterizer_xbox_bitmap_swizzle` remains the sole residual:

- January: 800 padded bytes, 47 ordered relocations, normalized SHA-256
  `4e705de69eb0ee60da8a23b66cf9de1258831b8d1ac4493078af7e0479b0168f`;
- candidate: 800 padded bytes, 47 ordered relocations, normalized SHA-256
  `1155ecd5529a0f2c7c1942020eaf6da423f0ff5b783510feed709e61151e66bd`;
- objdiff similarity: 97.91958%.

Both sides contain 295 instructions. The eight differing instruction slots
are confined to the cube-map prologue, where January homes the widened
`bytes_per_pixel` before division by six and VC7's current natural source
keeps it live in another register until after the division. The complete
semantic body and all relocation identities remain intact. This is the same
documented `instruction-scheduling` fixed point, so its existing park remains
valid and was not reopened for source steering.

## Validation

Validation was run in the isolated
`agent/rasterizer-swizzle-dirty-reconcile-20260904` worktree only:

- full `ninja all_source progress semantic_progress`: pass;
- default full `ninja`: pass;
- focused campaign gate: 12 exact, 1 residual, 0 unwritten;
- rename-stable whole-tree snapshot: 8,245 functions, 6,095 strict exact;
- baseline-to-final stable diff: +0 exact / +0 padded bytes, zero
  regressions;
- progress: 878,556 meaningful code bytes, 6,048 credited functions,
  2,018,310 data bytes, and 391/833 credited objects;
- semantic audit: 473 units, 6,474 evaluated functions, 6,104 semantic
  exact, 169 hidden exact / 101,147 hidden bytes, 6,121 accepted, and zero
  unit errors;
- fake-match scan on `rasterizer_swizzle.c`: zero findings;
- parked-function validation: 223 active, zero stale, zero invalid;
- object-admission audit: zero candidates, contradictions, or revocations;
- tooling suite: 261 passed;
- protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes and 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- no `point_from_line3d` symbol is emitted by the touched object.

No header changed, no exact function reopened, no completion/admission label
was added, and nothing was pushed from this audit.
