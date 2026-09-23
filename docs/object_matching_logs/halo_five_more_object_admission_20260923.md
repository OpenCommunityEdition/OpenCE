# Halo whole-object admission follow-up (2026-09-23)

This batch is restricted to Halo-owned translation units outside the concurrent
Claude structural lane. Whole-object status is separate from new exact-byte
credit: all target code here was already credited, so these admissions add
**zero code bytes**. An independently reverified folded-literal data section
can gain data credit without changing source bytes.

Final full build: **five newly admitted Halo objects**, 405/833 project-wide
and 301/468 halobetacache linked. The separate `aim_assist` closure adds
**832 strict meaningful code bytes** and `unit_dialogue` adds 44 credited
data bytes. Stable verdict sweep against the pre-batch snapshot: one function
gained, zero regressed. Pytest: 1,152 passed, five skipped, 26 subtests passed;
park validation: 124 active, zero stale, zero invalid.

## `source/saved games/game_state_xbox`

Promoted from `NonMatching` to `Matching`. A fresh PID-isolated gate with the
`_point_from_line3d` emitted-symbol guard found 17 exact functions, zero
residual, zero unwritten. All target-owned literals and the 20-byte
`_xbox_game_state_globals` BSS match January in bytes, relocations, flags,
storage, and offsets. The five candidate-only SELECT_ANY sections consist of
three stock SDK tables and two pooled literals. Each is byte-identical to its
January selected provider. Ordinary VC7 duplicate-owner diagnostic links with
all providers, in both input orders, reported no LNK2005. These diagnostic
links still have unrelated unresolved externals, so they are not presented as
successful whole-program links. No candidate-only section receives January
byte credit. Source review found no invented names, caller-local prototypes,
fake-match leads, or active park/rejection; its unusual persistent-storage
path operations are target-attested and previously documented.

## `source/rasterizer/xbox/rasterizer_xbox_decals`

Promoted from `NonMatching` to `Matching` after correcting nine PDB-backed
private BSS symbols in `config/symbols.json`; its C source was not changed.
January's split defaults those symbols to external storage, but the first-party
PDB omits them from publics and separate translation units own same-named
globals. The corrected private symbols match January's 27-byte `.bss` extent
at offsets 0, 4, 8, 12, 16, 20, 24, 25, and 26. All 29 functions and 819
target data bytes are strict exact. Five candidate-only SELECT_ANY sections
match their selected providers byte-for-byte; ten pairwise VC7 duplicate-owner
diagnostics (provider before/after decals) reported no LNK2005 or LNK1169.
Unrelated unresolved externals prevent treating these probes as full links.
Source review found no new house-rule exceptions, fake-scan leads, or guards.

## `source/units/unit_dialogue`

Promoted from `NonMatching` to `Matching` after a fresh 14/14 strict code gate,
target-owned data and source review. Its 44-byte speech-name table has the
same bytes and eleven relocation locations, types, and independently resolved
destinations as January. The COFF-resolved SHA-256 is
`85759b3811ff7dc47b03792ac85317be51431a3f9e01dcafce317ed736a391b0`
on both sides. Objdiff undercredits this table because nine of its folded
literal providers reside in other January objects, so a fail-closed semantic
data entry now re-verifies and credits **44 previously uncredited data bytes**.
All 24 candidate-only SELECT_ANY literals/constants are identical to January's
selected providers; 24 pairwise VC7 links in both orders had no duplicate
owner error. The pre-existing 4,420 meaningful code bytes were already
credited; this admission claims zero new code bytes.

## `source/rasterizer/rasterizer_transparent_geometry`

Promoted from `NonMatching` to `Matching` after a fresh 19/19 strict gate,
target-owned `.rdata`/`.bss` comparison, and whole-board zero-regression
sweep. Eight BSS symbols and the private sorter have corrected static
attribution in `config/symbols.json`: five names and types are supported by
the HCEX first-party PDB file statics, all eight are absent from January's
publics, and no other TU references them. The C source now includes the real
owner interfaces and declares `rasterizer_sort_external` in its own header.
The broad Xbox rasterizer header pulled in stock SDK tables whose January
providers are NODUP; ordinary VC7 pair links failed in both orders. A narrow
Xbox rasterizer state interface supplies only the three genuinely used
backend declarations, leaving the 19 function bodies exact and eliminating
those surplus definitions. Remaining candidate-only pooled literals are
byte-identical SELECT_ANY copies; both-order diagnostic links showed no
duplicate owner. No new code bytes were claimed.

## `source/cseries/stack_walk_windows`

Promoted from `NonMatching` to `Matching`. Its 16 functions were already
strict exact. Reordering two file statics and giving the function-local
`symbol_buffer` an explicit zero initializer reproduces January's 16,392-byte
`.bss`: `walk_up_current_frame` at 0, `old_ebp` at 4, and the buffer at 8.
All 36 target-owned data/BSS symbols match in name, storage, size, alignment,
offset, bytes, and relocations. This is an **inferred storage-order spelling**,
not a claim to possess January's original declarations. The XDK's documented
`NOD3D` and `NODSOUND` feature switches exclude APIs unused by this source;
both are needed because `NOD3D` alone makes the sound header fail to compile.
They remove three surplus SDK tables, without changing any of the 16 code
matches. The only two remaining candidate-only pooled literals are
byte-identical SELECT_ANY copies of January-selected literals in `ai_debug`
and `tiff_file`; VC7 pair links in both input orders found no duplicate owner.
No new exact code bytes were claimed.

## Exact source retained without whole-object admission

`source/game/aim_assist` gained the first-party `/Od`-attested
`_player_aim_projectile` body: 832 strict meaningful code bytes and 16/16
functions, with zero board regressions. Its `pin_normal_to_cone3d` prototype
is in the dedicated math owner header, not locally in the `.c`. The object
is **not** marked `Matching`: its candidate still emits shared-inline helper
COMDATs that conflict with January-selected NODUP providers in ordinary
VC7 links. The new function receives code credit, not whole-object credit.

## Held candidates

The target-owned code of these units is exact but whole-object admission is
held for independent source/ownership reasons:

- `key_agreement`: January's packet data has 4-byte section alignment while
  the candidate has 8-byte alignment. An explicit alignment annotation is
  not authenticated source and is held, even though it can force equality.
- `lruv_cache`: the `#define index datum_index` assertion-text facade and a
  public prototype local to its `.c` file need source correction.
- `action_guard`, `action_flee`, `actor_firing_position`, `devices`, and
  `particle_systems`: unresolved helper COMDAT ownership and/or source debt.

Further verified admissions, if any, are to be appended here only after
target-owned sections, source conventions, selected-provider behavior, full
build, stable function sweep, parks, and tests are checked.
