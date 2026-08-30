# `rasterizer_debug.obj` lifecycle fixed point (2026-08-30)

## Verdict

The January PC object remains **5/12 strict exact**.  The seven residual owners
are still unwritten in the restored production source and account for 3,008
padded target bytes:

| owner | target padded bytes |
| --- | ---: |
| `_code_0016d980` | 96 |
| `_rasterizer_debug_dispose` | 224 |
| `_rasterizer_debug_draw` | 1,056 |
| `_rasterizer_debug_initialize` | 144 |
| `_rasterizer_debug_line_shaded` | 592 |
| `_rasterizer_debug_new_primitive` | 112 |
| `_rasterizer_debug_triangle_shaded` | 784 |

One target-disassembly-backed lifecycle packet was compiled.  It emitted
`new_primitive`, `initialize`, and `dispose`, preserved all five inherited exact
owners, but produced no new strict exact owner.  The packet was therefore
rejected atomically and the source and base object were restored exactly.

## Frozen baseline

- isolated worktree:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\light-volumes-closure-20260830`
- branch: `agent/light-volumes-closure-20260830`
- baseline commit before this ledger:
  `2e5381bf81e4ecce11416d7110b67aabdc4c9755`
- source:
  `source/rasterizer/rasterizer_debug.c`
- January object:
  `build/split/source/rasterizer/rasterizer_debug.obj`
- restored base object:
  `build/base/source/rasterizer/rasterizer_debug.obj`
- restored source SHA-256:
  `6d17974eedef7a5e30866e853c7eceb9a27f68e8bbec45b5e1296e7e11fe27ed`
- restored base-object SHA-256:
  `d436093a806eec26f8624c15c34f60bfc0b21a055bf9b476e2dbc3cb3186fcab`
- January-object SHA-256:
  `46317f071d3e4ddc3669910fcf786d17cd48f76314d73ee81fe68d47621ffc1e`

The inherited exact owners remain:

- `_rasterizer_debug_begin` (32 padded bytes, 4 relocs);
- `_rasterizer_debug_end` (16, 0);
- `_rasterizer_debug_test` (16, 0);
- `_rasterizer_debug_line` (32, 1); and
- `_rasterizer_debug_triangle` (32, 1).

The retained BSS is still the source-owned 0x21-byte `_bss_004662f8` object.

## Evidence census

The full prior wrapper ledger was read first:

- `docs/object_matching_logs/rasterizer_debug_obj_jonas_wrapper_pair_20260826.md`.

The registered-worktree source census covered 1,580 copies of
`rasterizer_debug.c` and found seven raw source hashes.  None defines any of the
seven missing owners.  Dedicated worktrees and their relevant commits were also
read:

- `C:\tmp\rasterizer-debug-hooks`, `37c3d2a57`;
- `work\lanes\rasterizer-debug-immediate-hooks`, `6f997ebb4`;
- `work\lanes\rasterizer-debug-model-vertices`, `86536ee8e` and `077d7c8a8`;
- `work\rasterizer-debug-model-resume`, `565666d15` and `0af4be513`; and
- `work\rasterizer-debug-wrapper-pair-20260826`, `69d97e6ff`.

The first and last contain only the already integrated public wrappers.  The
other lanes affect `rasterizer.obj`, not `rasterizer_debug.obj`.

The object-artifact census covered 1,580 registered worktrees, 1,154 available
`build/base/source/rasterizer/rasterizer_debug.obj` files, and 1,131 distinct
raw hashes.  The maximum strict frontier was five owners.  No artifact contains
an exact residual owner or exceeds the restored production baseline.

The authenticated HCEA copies at
`research-cache/halocea-cseries-20260820/src/blam/rasterizer/` were read rather
than inferred from filenames.  Their `rasterizer_debug_initialize`,
`rasterizer_debug_dispose`, `rasterizer_debug_draw`, `line_shaded`, and
`triangle_shaded` files are later Xbox 360 retail no-op stubs (except initialize
returning true), not January PC donor bodies.  Stian, Pastudan, and punpckhdq
contain symbols/skeletons or a different PC debug-render facility, but no donor
for this TU.  These sources do not justify another spelling.

## Rejected lifecycle packet

The one packet used readable C89, a typed 60-byte primitive record, typed
prefixes for `rasterizer_debug_options+2` and
`rasterizer_frame_statistics+0x140`, the target count limit 8,192, the target
0x78000-byte allocation, exact path/error strings, and target source lines
96--98, 137--139, and 141--143.  It introduced no forbidden construct.

| owner | target / candidate padded bytes | target / candidate relocs | target normalized SHA-256 | candidate normalized SHA-256 | result |
| --- | ---: | ---: | --- | --- | --- |
| `_rasterizer_debug_new_primitive` | 112 / 112 | 9 / 9 | `ed488176cf12b0ebb8f87c2ce7721a5a6b317ad0d22b79cc8177d7bad3513ffa` | `2815f47ee2fc066fb6c46ab3cd262621aff11019ebdcf374e3472e441313004c` | byte/control-flow residual |
| `_rasterizer_debug_initialize` | 144 / 144 | 15 / 15 | `df4a1b2cc36f99d42650b96b504b023bd2aa63471f302ca1df6787a0416be12b` | `8b2ae01d509cd4b80922f5ca9de7ec01877c434ca2555dd9737016998358a230` | byte/control-flow residual |
| `_rasterizer_debug_dispose` | 224 / 224 | 26 / 26 | `472fde96fb7073395bddc79675562ee3ce140f717eaa58c50ed895ab841ab728` | same | relocation-identity only |

The rejected complete candidate object SHA-256 was
`a09e4744a8acad65799087a85fda1f95e2b6eaaedaa948a8c6eab9917c5c9193`.
It is evidence only and is not retained as production code.

### Exact remaining compiler boundaries

`dispose` already reproduced every target byte and every relocation address,
type, and non-string target.  Only the three assertion expression literals
differed:

- target: `debug_data.opaque_triangles`;
- target: `debug_data.opaque_lines`; and
- target: `debug_data.non_opaque_primitives`.

The rejected source stringified `bss_004662f8.*`.  A future authorized packet
should retain the real linked global and expose its source name with a normal
alias such as `#define debug_data bss_004662f8`, then spell the three assertions
through `debug_data`.  This is a literal relocation-identity correction, not a
reason to change the function's already exact control flow.

`initialize` matches target instructions and relocations through the third
allocation and the three pointer tests.  Its final branch is inverted relative
to January because the rejected spelling used a mutable `success` variable and
a common assignment.  January places the success block first:

1. if all three allocations are non-null, set `initialized = TRUE` and return
   `TRUE` immediately;
2. otherwise report `"### ERROR failed to allocate debug buffers"`, set
   `initialized = FALSE`, and return `FALSE`.

`new_primitive` has the correct semantics, size, literals, globals, and nine
relocation identities, but its instructions start four bytes earlier because
the rejected spelling returns `NONE` directly.  January saves `ESI`, initializes
it to `NONE`, and moves it into `EAX` only on the overflow path.  A future
authorized spelling should use one named result initialized to `NONE`, assign
the prior per-buffer count on the accepted path, and return that result once;
the target then keeps the successful prior count in `EAX` while using `ESI` for
the rejected-path constant.

These three observations form a precise Claude/Fable handoff.  They must not be
used for another local fixed-point iteration unless a new authorized packet or
new provenance changes the campaign premise.

## Restore and admission boundary

After rejection, `source/rasterizer/rasterizer_debug.c` was restored byte for
byte and the production Ninja edge rebuilt the base object.  Strict comparison
reconfirmed all five inherited owners.  No production source or header change
survives this ledger.

The final restored validation packet passes:

- focused Rasterizer Debug gate: 5 exact, zero emitted residual, seven
  unwritten;
- complete `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress` graph: pass;
- semantic report: 470 units, 4,957 functions evaluated, 4,841 semantic exact,
  4,851 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 linked objects, 4,812/11,060 functions,
  610,646/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- Halo progress: 282/468 linked objects, 4,645/7,574 functions, and
  597,732/1,770,166 code bytes;
- object admission: zero candidates, contradictions, and revocations;
- parked-function audit: 12 active, zero stale, and zero invalid;
- Python tooling suite: 212/212 passed;
- protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256:
  `13c9f7e8ac93b0a6de047e0f40bc27408dd076504228e7e1211f2cc4fe526f4f`;
- `build/semantic_report.json` SHA-256:
  `9f948fc7de4a7d8d11f52ebbfb7f17126cb03ea893662921475c6e9484f66db0`;
  and
- `git diff --check`: clean.
