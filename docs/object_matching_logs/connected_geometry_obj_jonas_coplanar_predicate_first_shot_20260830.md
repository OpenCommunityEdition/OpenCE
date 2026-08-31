# `connected_geometry.obj`: coplanar-predicate first-shot boundary (2026-08-30)

## Outcome

The sole provenance-backed ordinary-C candidate for `_code_000f3220` is a
strict miss and has been removed in full.  January and the candidate are both
380 meaningful / 384 padded bytes with the same 11 ordered relocations, but 16
normalized bytes differ.  Every difference is confined to the final
facing-dot x87 evaluation after `plane3d_from_points`.

There was no source permutation, retry, byte tuning, compiler-control change,
exception, or adjudication.  Production source is restored byte-for-byte to
its starting Git blob.  `connected_geometry.obj` therefore remains at five of
ten strict-exact owners and 768/2176 exact padded `.text` bytes.  This ledger is
the sole retained change from the experiment.

## Governance and provenance

The attempt followed `docs/matching_methodology.md`,
`tools/campaign/README.md`, the four prior connected-geometry ledgers, and the
applicable Claude guidance in the canonical Halo, GLM/tooling, and Stian
reference trees.  The body was typed C and contained no inline assembly,
`volatile`, `register`, pragma, compiler barrier, forced inline, raw
structure-offset access, pointer/integer reconstruction, union/pointer punning,
undefined behavior, post-build patch, or comparator exception.

The January authority is
`build/split/source/tool/connected_geometry.obj`, whole-object SHA-256
`bd9bcffee0f23780214492f62593ad478315de2246564c4e8d562cfc9b424429`.
The production file's starting and restored Git blob is
`ba6d8b118af29c26282f193a6756104bdb9ddf7a`.

The readable donor is Stian's `FUN_00103a00`:

- original port commit:
  `056c3d2419990bfc591e101d7684dd91c0ffba45`;
- original port file blob:
  `d590654fc8248213f8f4a092d04e6f73feef46bb`;
- current integrated file blob:
  `70c484584b9ead2f94d3736ae077bbb006a8a423`;
- score reference: `c41454ff94cb77eb`, with raw mnemonic 100.0%, operand
  94.6%, recorded VC71 score 100.0%, and 100/100 equivalence.

Repository-wide history and worktree census found no earlier local production
implementation.  The donor score establishes independent semantics and source
topology; only the hardened January COFF comparison can establish admission.

## Sole candidate and exact packet

The candidate resolved the triangle's three oriented point indices through
typed dynamic arrays in edge order `[0], [1], [2]`, tested all three signed
plane distances with ordinary `fabs(expression) < 0.01f`, constructed the
triangle plane from points `[0], [2], [1]`, and accepted only a positive facing
dot product.  Plain `fabs` is independently ordinary-C-safe in this compiler
lane: exact `_convex_hull2d_area` emits the target `FABS` without a pragma.

| Packet | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January target | 380 | 384 | 11 | `02acd868e062aee29df8e260c3fbd62221e3f2ced5e5f43573a1a288e6129d54` |
| Sole candidate | 380 | 384 | 11 | `552ea6cd7c8bb24f11262e96c680319ff1853eac0e97f8cc0216a1477bbe7533` |

All relocation addresses, kinds, and semantic destinations agree:

- `_dynamic_array_get_element` REL32 at `+0x21`, `+0x3D`, `+0x59`,
  `+0x76`, `+0x93`, and `+0xB0`;
- the widened `0.01f` qword constant DIR32 at `+0xD7`, `+0x104`, and
  `+0x12A`;
- `_plane3d_from_points` REL32 at `+0x13D`;
- the float-zero DIR32 at `+0x15F`.

Exactly 16 normalized bytes differ.  The first divergence is `+0x14A`
(target `F0`, candidate `F4`); the complete differing ranges are `+0x14A` and
`+0x14C-+0x15A`.  Everything before and after those ranges is identical.

The target final dot schedule is x product, z product, `faddp`, y product,
`faddp`.  VC7 compiling the donor-shaped expression emits y product, z
product, `faddp`, x product, `faddp`.  The comparison and branches then rejoin
byte-for-byte.  This is an instruction-scheduling/source-expression boundary,
not an ABI, size, relocation, constant, dependency, or semantic mismatch.

Reopening requires newly authenticated source topology or an independent
ordinary-C donor that naturally establishes the target x87 schedule.  Binary-
led arithmetic permutations are outside the campaign rules.

## Restoration and validation

The candidate was inverse-reverted, the restored TU was rebuilt, and the five
inherited owners remain strict exact under the direct hardened comparator.  The
restored raw base object SHA-256 is
`c984a8e7bbf75d3d042eaed2418f7dd5d1206ce6cc99018b4d997617763c23b2`.

Final gates after restoration:

- full Ninja goals (`halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`): passed;
- semantic report: 470 units and 4,986 functions evaluated, 4,866 semantic
  exact, zero unit errors, `ordinary_only=1`, `ordinary_structural=1`, and
  `ordinary_rejected=0`;
- repository tests: 212 passed;
- admission audit: 0 candidates, 0 contradicted, 0 revoked;
- parked-function audit: 13 active, 0 stale, 0 invalid;
- `git diff --check` and the connected-geometry prohibited-construct scan:
  clean;
- `build/report.json` SHA-256:
  `3130aa28c63be1a1314014e8c36dc83adcc03b42252232f44d9ec973a70d7adc`;
- `build/semantic_report.json` SHA-256:
  `0c54a0f000e68184235f6adafcd11a5a4700ef80783f54b99ef19833fb540bf2`.

No production source from the rejected candidate is retained.
