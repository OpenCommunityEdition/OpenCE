# connected_geometry.obj intermediate-triangle recovery (2026-08-30)

## Result

`_connected_geometry_add_intermediate_triangle` is retained as a strict
ordinary-C recovery.  The independently sourced typed wrapper matched on its
first and only code-producing compile.

| owner | meaningful / padded bytes | relocations | normalized SHA-256 | verdict |
| --- | ---: | ---: | --- | --- |
| `_connected_geometry_add_intermediate_triangle` | 114 / 128 | 5 | `cc2602c2c2ac3bcf96b17b3301d9f50337c39cc318fcd07682a07c25cf172a67` | **strict exact** |

The object advances from 3/10 to 4/10 strict-exact owners and from 336/2176
to 464/2176 exact padded `.text` bytes.  The previously retained lifecycle and
recursive-group owners remain strict exact.

## Governance and freshness

This recovery followed `docs/matching_methodology.md`,
`tools/campaign/README.md`, the two earlier connected-geometry ledgers, and the
applicable Claude guidance in the canonical Halo, GLM/tooling, and Stian
reference trees.  It contains no inline assembly, naked body, `volatile`,
`register`, pragma, compiler barrier, forced inline, union/pointer punning,
post-build patch, or comparator exception.

The January authority is
`build/split/source/tool/connected_geometry.obj`, whole-object SHA-256
`bd9bcffee0f23780214492f62593ad478315de2246564c4e8d562cfc9b424429`.
An exhaustive local census found no prior implementation or compile trial of
this owner across Git refs or the 1,126 worktree copies inspected.  The public
`punpckhdq` tree contains only the stub; no object-specific Claude candidate was
available to integrate.

## Independent provenance

The readable PC donor is Stian's `FUN_00103b80` in
`src/halo/main/main.c`:

- provenance commit:
  `fe98145b9b11d4635c8dd5c8187e9fabfc41c2db`;
- commit-time file blob:
  `dc84379fd9b77ce7475fad6a52f92e9a11dc2465`;
- current file blob:
  `70c484584b9ead2f94d3736ae077bbb006a8a423`;
- donor measurements: 100% VC71, 48/48 instructions, and 100/100 semantic
  equivalence checks.

The Stian PC cluster and the Xbox object preserve identical consecutive
function extents around this owner.  This donor established the source topology
before the candidate was compiled; strict admission still comes from the
hardened January COFF comparison.

## Recovered typed topology

The source adds only the verified minimal views needed by this wrapper:

- a 0x34-byte intermediate triangle with three vertex indices at `+0x08`;
- a 0x50-byte intermediate vertex with its typed point at `+0x08`;
- a minimal intermediate-geometry prefix whose triangle and vertex dynamic
  arrays begin at `+0x134` and `+0x140`.

Named unused fields and compile-time layout assertions express these recovered
facts.  The minimal prefix is not asserted or represented as the full original
intermediate-geometry structure.  Its pointer remains non-const because the
existing typed `dynamic_array_get_element` API accepts a non-const array;
there is no const-removing cast.

The function performs one triangle lookup, then forwards the three resolved
typed points to `connected_geometry_add_triangle`.  The three vertex lookups
remain directly nested in the outer argument list in natural `[0], [1], [2]`
source order.  VC7's right-to-left argument evaluation therefore emits the
target lookup order `[2]`, `[1]`, `[0]` without a scheduling construct.

All target relocations match in address, kind, and destination:

- `_dynamic_array_get_element` at `+0x16`, `+0x31`, `+0x44`, and `+0x57`;
- `_connected_geometry_add_triangle` at `+0x67`.

## Validation

- direct hardened comparison over all four retained owners:
  `all_equal: true`;
- target and candidate are both 128 padded bytes with five relocations for the
  recovered owner;
- targeted Ninja dry run and build: exactly one `CL` action;
- full Ninja goals (`halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`): passed;
- semantic report: 4,985 functions evaluated, 4,865 semantic-exact, 0 unit
  errors, `ordinary_only=1`, `ordinary_structural=1`, and
  `ordinary_rejected=0`;
- repository tests: 212 passed;
- admission audit: 0 candidates, 0 contradicted, 0 revoked;
- parked-function audit: 13 active, 0 stale, 0 invalid;
- `git diff --check`: clean;
- candidate whole-object SHA-256:
  `33b778687b3ad97c6f9c8086128aaeb9c33e7f2cd62d6ecb6212d01f90321178`;
- source Git blob before commit:
  `b91257b301dfefa36db5304cc7b6fc1b0c02559b`;
- `build/report.json` SHA-256:
  `66e5a777c6713694a36b5932816083191715f21ad3b687aa6cc973919473da50`;
- `build/semantic_report.json` SHA-256:
  `3145fc158c03b08d7bd0748db7cc588828e6da0f05034f5290d36dbc15e124fa`.

Tracked scope is exactly the typed source recovery and this ledger.  Build
outputs, dependency links, caches, and research artifacts are not tracked.
