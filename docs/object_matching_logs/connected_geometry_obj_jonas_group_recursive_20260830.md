# connected_geometry.obj recursive-group recovery (2026-08-30)

## Result

`_connected_geometry_group_recursive` is retained as a strict ordinary-C
recovery.  It matched on its first code-producing compile and adds 208 padded
bytes to the exact set.

| owner | meaningful / padded bytes | relocations | normalized SHA-256 | verdict |
| --- | ---: | ---: | --- | --- |
| `_connected_geometry_group_recursive` | 197 / 208 | 4 | `4ca7c404a14987765c235196243b2a9fe8f75f38e0ca4a47aa9270679117bd08` | **strict exact** |

The object advances from 2/10 to 3/10 exact owners and from 128/2176 to
336/2176 exact padded `.text` bytes.  The two inherited lifecycle owners remain
strict exact:

- `_connected_geometry_new`: 48 padded bytes, 3 relocations,
  `d85f2504e2ffcdd236b7e29ae2beda66e0f3ad33a7b3cef3bc6292946d4200c1`.
- `_connected_geometry_delete`: 80 padded bytes, 5 relocations,
  `e74305d7f27a6a8ad73684af18843186b6e4f76971a75f30828609519dbb1c7f`.

## Governance and documentation

The reconstruction followed the repository object-matching policy, the
existing connected-geometry lifecycle ledger, and the applicable Claude
instructions for the canonical Halo tree, the GLM/tooling tree, and the Stian
reference tree (`../halo/CLAUDE.md`, `../halo-glm/CLAUDE.md`, and
`../stianeklund-halo-reference/CLAUDE.md`).  The retained code is typed C.  It
contains no inline assembly, naked body, `volatile`, `register`, pragma,
compiler barrier, forced inline, raw structure offset, union/pointer punning,
or post-build byte manipulation.

The January authority is
`build/split/source/tool/connected_geometry.obj`, whole-object SHA-256
`bd9bcffee0f23780214492f62593ad478315de2246564c4e8d562cfc9b424429`.

## Independent provenance

Stian's independently recovered PC cluster preserves the same consecutive
function boundaries as the Xbox target.  The corresponding donor is
`FUN_00103530` in `src/halo/main/main.c`.

- introduction commit:
  `af4ab5caba58fbfbfb5dfa61fe52ecae27d47a15`;
- introduction-time file blob:
  `67a103bb0bd2f773bdf10ea05e2164d94d63a7df`;
- callback-order correction commit:
  `056c3d2419990bfc591e101d7684dd91c0ffba45`;
- corrected file blob:
  `d590654fc8248213f8f4a092d04e6f73feef46bb`;
- current donor file blob:
  `70c484584b9ead2f94d3736ae077bbb006a8a423`.

The introduction was measured at 98.8% VC71 with 85/85 instructions and
125/125 semantic-equivalence checks, but that historical body used the wrong
callback argument order.  The later correction established the disassembly-
proven order `(predicate_data, geometry, triangle, group_index)`; it has no
separate recorded donor score.  Accordingly, the historical 98.8% is provenance
context only and is not attributed to the corrected body.  Exact admission here
comes solely from the hardened January target comparison.

No local prior implementation or compile trial of this owner was found.  The
public `punpckhdq` tree retains the stub and does not supply this recovery.

## Recovered source topology

The retained function:

1. fetches the requested 0x18-byte triangle from `geometry->triangles`;
2. returns unless its group is `NONE` and the optional typed predicate accepts
   `(predicate_data, geometry, triangle, group_index)`;
3. assigns the group and walks exactly three edge designators;
4. skips `NONE`, masks a designator with `LONG_MAX`, and fetches the typed
   0x1C-byte edge from `geometry->edges`;
5. walks the edge's typed long-index dynamic array and recursively groups each
   adjacent triangle.

The inner counter is intentionally a `short`; VC7 emits the target's
`inc`/sign-extension loop shape from that source-level type.  This is a recovered
type fact, not a byte-forcing construct.

All four relocations match in address, kind, and destination: REL32 calls at
`+0x14`, `+0x68`, and `+0x85` to `_dynamic_array_get_element`, followed by the
self-recursive REL32 relocation at `+0x9A`.

## Rejected one-shot in the same wave

`_plane3d_from_points` was also evaluated once from a fresh, independently
proven scalar donor and was rejected and fully reverted.

- target: 176 padded bytes, 2 relocations, normalized SHA-256
  `44d64972b09abae673273d594bfffe68ee1058ecbe300c3eb0f20fe51f97cddf`;
- candidate: 176 padded bytes, 2 relocations, normalized SHA-256
  `9c032b5b4cfd76ad32908a92c8b1036eaf53ae24773127580afc2aa66784f262`;
- target calls at `+0x6A`/`+0x70`; candidate calls at `+0x69`/`+0x6F`;
- first normalized divergence at `+0x03`; 157 normalized bytes differed.

The target reserved a 0x0C local frame while the ordinary scalar candidate
reused argument home slots.  The evidence did not justify source-shape tuning,
so this avenue is closed until genuinely new independent provenance appears.
No plane source remains in the tree.

## Validation

- direct `tools.coff_compare` over all three retained owners: `all_equal: true`;
- full Ninja graph (`halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`): 572 actions passed;
- semantic report: 4,984 functions evaluated, 4,864 semantic-exact, 0 unit
  errors, `ordinary_only=1`, `ordinary_structural=1`,
  `ordinary_rejected=0`;
- repository tests: 212 passed;
- admission audit: 0 candidates, 0 contradicted, 0 revoked;
- parked-function audit: 13 active, 0 stale, 0 invalid;
- `git diff --check`: clean;
- candidate whole-object SHA-256:
  `6ad5cc43e280266f7d2e7121dd9543738977299bad4c26686191b871792b4dcb`;
- source Git blob before commit:
  `88fa70af6e7ac52704e830f829db06723e6a53c0`;
- `build/report.json` SHA-256:
  `78b3732f26180af6d3fb954462938cbc60b4a9fc2cba469c6add3d3adb36a3ec`;
- `build/semantic_report.json` SHA-256:
  `d5bc9406261a0ad2f9716ac72ae04df488a214d07c43e68bccb5e862b3df0ea0`.

Tracked scope is exactly the typed source recovery and this ledger.  Build
outputs, dependency links, caches, and rejected candidates are not tracked.
