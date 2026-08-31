# `bitmap_utilities.obj` vector-map quartet recovery — 2026-08-31

## Disposition

The four January vector-map functions are now ordinary, source-credible C and
are strict exact individually.  This advances `bitmap_utilities.obj` from
11/48 to **15/48 strict-exact target functions**, adding 1,161 meaningful /
1,184 padded bytes and 85 relocations without regressing any previously exact
function in the tree.

The object remains `NonMatching`.  Expanding the authenticated inline math
helpers also makes VC7 emit six unreferenced, select-any code COMDATs that are
not owned by January's `bitmap_utilities.obj`.  Their bytes are exact against
the canonical January owners, but direct XDK links fail against those owners'
no-duplicates sections.  This is the same fail-closed ownership boundary as
the documented `fast_ftol` player-rumble case.  No `Matching` label, semantic
allowlist entry, or candidate-only code credit is added.

## Scope and provenance

- Translation unit: `source/bitmaps/bitmap_utilities.c`
- January target: `build/split/source/bitmaps/bitmap_utilities.obj`
- Evidence base: `f708e2e1837504210ec5daf8d618032b2d734fc5`
- Selective source donor: Claude campaign commit
  `e02971dfdba111d5481c5b7d7b6fc38a9e90d155`; no commit was cherry-picked.
- Compiler: XDK 3911 `CL 13.00.9254.1`, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`
- Flags: repository `/O2 /Oy- /DDEBUG /Dxbox` flags, unchanged.
- Target object after semantic private-symbol aliases: SHA-256
  `2ef2a5d8a4caed15511b01b3bbab84b69a482d585897627e1a1fbb54b30d58c1`.
- Rebuilt candidate object: SHA-256
  `aee73cca486a35b4b2e0eda42a08d82df37aa86b4a93f5c88cc8ab792bd91f78`.

January's exact symbol atlas names the private helpers
`bitmap_2d_vector_map`, `bitmap_3d_vector_map`, and `bitmap_cm_vector_map`, and
the public dispatcher `bitmap_vector_map` in
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`.
The same names recur in multiple independent PC/CEA maps.  HCEA's independent
PPC reconstructions corroborate the actual algorithm: normalize each packed
RGB vector in a temporary mip-0 buffer, apply it per slice for volume maps and
per face for cube maps, preserve alpha, and dispatch on the bitmap type.
January assertion strings independently prove the three switch constants and
the source path/line topology.

## Strict function evidence

`tools/campaign/gate.py` reports exact padded bytes, normalized bytes, and
relocation address/type/destination/addend identity for all four functions:

| Function | Meaningful/padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_bitmap_2d_vector_map` | 600/608 | 39 | `1037cf9f014f0e092777532913acd64ae5811a2cf2b4cd475f4cfc35cc854f4a` |
| `_bitmap_3d_vector_map` | 218/224 | 17 | `8be59f0a0685d70dbe5c04cd47903014d7c27b38586f82bff5a670ff6dc8dda4` |
| `_bitmap_cm_vector_map` | 207/208 | 17 | `ae0435d005d4561caeb1049cd715f08df22a23ac2fcac7b5f7aa0f5a76387b5a` |
| `_bitmap_vector_map` | 136/144 | 12 | `69a58a7edb452ec6b041818bc128fb49e45f758d76173bde7b78066a92cd137e` |

The private target aliases in `config/symbols.json` are source-static, while
the public dispatcher prototype lives in `source/bitmaps/bitmaps.h`.  The
three private prototypes stay in their owning translation unit.  The retained
code uses `real`, `real_vector3d`, `pixel32`, the established bitmap enum
constants, `NUMBER_OF_FACES_PER_CUBE`, the cseries allocation/copy macros, and
the shared authenticated `fast_ftol`; it adds no address labels, new assembly,
forced inlining, volatile access, type-punning dependency, or inert control
flow.

## Data and ownership audit

The wave adds no `.data`, `.bss`, COMMON symbol, or other writable runtime
owner.  Every emitted read-only literal either belongs to the January
`bitmap_utilities.obj` data set or is byte/relocation exact against its linked
canonical owner.  In particular, the two pooled diagnostic strings are exact
against `bitmap_extract.obj`; the pooled math constants are exact against the
January owners in `action_charge.obj`, `action_alert.obj`, and the existing
bitmap/libtiff objects.  These select-any read-only duplicates are ordinary
pooling and receive no extra partial-data credit.

The six candidate-only code COMDATs are more restrictive:

| Candidate-only symbol | Bytes | Relocs | January canonical owner | Normalized SHA-256 |
| --- | ---: | ---: | --- | --- |
| `_fast_ftol` | 32 | 0 | `actor_combat.obj` | `8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248` |
| `_square_root` | 16 | 0 | `action_charge.obj` | `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7` |
| `_scale_vector3d` | 48 | 0 | `action_charge.obj` | `6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b` |
| `_magnitude_squared3d` | 48 | 0 | `action_alert.obj` | `0d04579096eeab3a1c45f87774e1eb20ce4856dbac219f624a855e8c9ae99725` |
| `_magnitude3d` | 48 | 0 | `action_charge.obj` | `114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c` |
| `_normalize3d` | 96 | 3 | `action_charge.obj` | `67fbd33f204d0c93cce135fcfd37d4ce5b443642edc6ad99230846b91c893e5b` |

All six candidate sections have flags `0x60501020` and
`IMAGE_COMDAT_SELECT_ANY`.  No relocation anywhere in the rebuilt
`bitmap_utilities.obj` refers to an out-of-line copy; the vector-map body has
the helper logic inlined.  The January canonical code owners are represented
as `IMAGE_COMDAT_SELECT_NODUPLICATES` in the split objects.

An XDK 3911 diagnostic link used `/dll /noentry /nodefaultlib
/force:unresolved` with the rebuilt bitmap object and the split
`actor_combat.obj`, `action_charge.obj`, and `action_alert.obj`.  It fails with
six `LNK2005` diagnostics, one for every symbol above, followed by `LNK1169`.
Thus these are not admissible discardable-helper ownership differences under
the current playbook.

## Experiment matrix

| ID | Source/control | Result | Decision |
| --- | --- | --- | --- |
| E00 | Untouched baseline | 11/48 exact; quartet unwritten | Baseline |
| E01 | Select only the four coherent Claude bodies, use atlas names and the shared cseries `fast_ftol` | All four exact; inherited 11/11 remain exact | Retained as partial source recovery |
| E02 | Put all cross-TU bitmap dependency declarations in the widely included `bitmaps.h` | Quartet stays exact, but `_unit_preprocess_node_orientations` regresses from exact | Reverted; only the new public dispatcher prototype remains in the shared header |
| E03 | Keep private vector helpers TU-local and rebuild every `bitmaps.h` consumer | Quartet exact; `_unit_preprocess_node_orientations` returns to exact; full verdict sweep has zero regressions | Retained |
| E04 | Link candidate against all six canonical helper owners | Six `LNK2005` errors plus `LNK1169` | Ownership admission rejected |

The shared-header result is another measured C2 definition-position effect.
It is not permission to ignore headers generally: the public API added by this
wave is in `bitmaps.h`, while the pre-existing incomplete internal bitmap
dependency declarations remain local until a header-layout recovery can move
them without losing January code.

## Validation and do-not-repeat boundary

- Targeted gate: 4 exact, 0 residual, 0 unwritten.
- Whole `bitmap_utilities.obj`: 15 exact, 0 residual, 33 unwritten.
- Whole-tree verdict diff: the quartet plus unrelated concurrent gains, zero
  exact-to-residual regressions.
- Header-sensitive sentinel:
  `_unit_preprocess_node_orientations` remains exact at 1,920 padded bytes.
- `halobetacache_build`: all affected `bitmaps.h` consumers compile.
- Scoped fake-match scan: 0 review leads.
- `git diff --check`: clean for this wave's files.

Do not retry address names, a TU-local duplicate of `fast_ftol`, extra inline
assembly, force-inline/pragma controls, dead calls, volatile anchors, or manual
bitwise lifetime tricks.  Byte-exact code is already known.  Reopen the
ownership boundary only with an evidence-backed shared-helper declaration or
source topology that preserves all four target functions while emitting none
of the six candidate-only code owners, or with independently authenticated
January object-level COMDAT metadata that changes the linker conclusion.
