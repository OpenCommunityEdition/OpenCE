# connected_geometry.obj coplanar-group driver recovery (2026-08-30)

## Result

`_connected_geometry_group_coplanar` is retained as a strict ordinary-C
recovery.  The provenance-faithful typed body matched on its first and only
code-producing compile.

| owner | meaningful / padded bytes | relocations | normalized SHA-256 | verdict |
| --- | ---: | ---: | --- | --- |
| `_connected_geometry_group_coplanar` | 289 / 304 | 10 | `4135a273f037ad0dad9f2a5b7de863ed03749b945fe4de3894104387695e431a` | **strict exact** |

The object advances from 4/10 to 5/10 strict-exact owners and from 464/2176
to 768/2176 exact padded `.text` bytes.  All four previously retained owners
remain strict exact.

## Governance and freshness

This recovery followed `docs/matching_methodology.md`,
`tools/campaign/README.md`, the three earlier connected-geometry ledgers, and
the applicable Claude guidance in the canonical Halo, GLM/tooling, and Stian
reference trees.  The retained body is typed C.  It contains no inline
assembly, naked body, `volatile`, `register`, pragma, compiler barrier, forced
inline, raw structure-offset access, union/pointer punning, post-build patch,
or comparator exception.

The January authority is
`build/split/source/tool/connected_geometry.obj`, whole-object SHA-256
`bd9bcffee0f23780214492f62593ad478315de2246564c4e8d562cfc9b424429`.
The same lane's exhaustive census found no prior implementation or compile
trial of this owner across local Git refs and worktree copies.  The public
`punpckhdq` tree retains the stub, and no object-specific Claude candidate was
available to integrate.

## Independent provenance

The readable PC donor is Stian's `FUN_00103c00` in
`src/halo/main/main.c`:

- original port commit:
  `0caabefd1cd5b1e6fec63f0dea8ad5f2ade906d8`;
- original port file blob:
  `12441f870c63a0d9037f98b7e131df6a1763064b`;
- later independent-tree integration commit:
  `1de6f05fa19ae757bdca6895f3252f4b9f11e610`;
- integrated/current file blob:
  `70c484584b9ead2f94d3736ae077bbb006a8a423`;
- score reference: `c2bebfed23f1869b`, with raw mnemonic 100.0%, operand
  94.6%, and recorded VC71 score 100.0%.

The donor score is independent provenance rather than exact-Xbox proof.
Admission here comes solely from the hardened January COFF comparison.

## Recovered typed topology

The function uses a typed 0x10-byte plane, a cached typed pointer to
`geometry->edges`, and two ordinary long locals for the group count and
triangle index.  The emitted local frame is the target 0x18 bytes.

For every triangle it resolves three oriented point indices through typed edge
and point dynamic arrays, constructs a plane, and seeds
`connected_geometry_group_recursive` with the typed coplanarity predicate when
the triangle is unassigned.  The source plane arguments remain points
`[0], [2], [1]`.  Keeping all three point resolutions directly nested makes
VC7 evaluate them right-to-left as `[1], [2], [0]`, matching the January call
schedule.

The orientation index is expressed as the ordinary typed condition
`(designator & LONG_MIN) != 0`.  The identical expression in
`structure_lens_flares.c` independently compiles to the target's
`AND/NEG/SBB/NEG` boolean materialization, so no cast, shift, helper, or
byte-forcing device is involved.

All ten relocations match in address, kind, and destination:

- `_dynamic_array_get_element` at `+0x2D`, `+0x46`, `+0x63`, `+0x7B`,
  `+0x98`, `+0xAF`, and `+0xCB`;
- `_plane3d_from_points` at `+0xD8`;
- the DIR32 predicate reference `_code_000f3220` at `+0xF4`;
- `_connected_geometry_group_recursive` at `+0xFA`.

The plane helper is declared locally with the recovered typed signature rather
than including `real_math.h`; that header's inline plane helper would change
the linkage boundary under examination.

## Validation

- direct hardened comparison over all five retained owners:
  `all_equal: true`;
- targeted Ninja dry run and build: exactly one `CL` action;
- full Ninja goals (`halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`): passed;
- semantic report: 4,986 functions evaluated, 4,866 semantic-exact, 0 unit
  errors, `ordinary_only=1`, `ordinary_structural=1`, and
  `ordinary_rejected=0`;
- repository tests: 212 passed;
- admission audit: 0 candidates, 0 contradicted, 0 revoked;
- parked-function audit: 13 active, 0 stale, 0 invalid;
- `git diff --check`: clean;
- candidate whole-object SHA-256:
  `d25378654794d3f5f8f1072c15a537bbb15d756ba24d8238939bdad32035adc7`;
- source Git blob before commit:
  `ba6d8b118af29c26282f193a6756104bdb9ddf7a`;
- `build/report.json` SHA-256:
  `3130aa28c63be1a1314014e8c36dc83adcc03b42252232f44d9ec973a70d7adc`;
- `build/semantic_report.json` SHA-256:
  `0c54a0f000e68184235f6adafcd11a5a4700ef80783f54b99ef19833fb540bf2`.

Tracked scope is exactly the typed source recovery and this ledger.  Build
outputs, dependency links, caches, and research artifacts are not tracked.
