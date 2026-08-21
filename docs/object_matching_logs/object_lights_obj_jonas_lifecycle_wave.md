# `object_lights.obj` Jonas lifecycle wave

## Result

This bounded source-only wave starts at authoritative integration commit
`9097dbb7dad5ddac695af090e722c4c34df12926` and recovers exactly three
public lifecycle leaves in `source/objects/object_lights.c`. The unit advances
from 0/43 to 3/43 strict functions for a gain of 53 meaningful code bytes. It
remains `NonMatching`.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_lights_dispose` | 12 | 16 | 2 | `3475063ddc9b8553ce16428d3190f88859ad052efe3a03875658043eb03bdb63` |
| `_lights_dispose_from_old_map` | 25 | 32 | 4 | `175c6346d5835e0c48319d1663ebe28be3a60ed6c67db99716658580040a5687` |
| `_lights_enable` | 16 | 16 | 1 | `b603d9aa305a2d5728fd85b226200faed109b09d8c08e367704ab72624fad28d` |
| **Wave delta** | **53** | **64** | **7** | |

The hardened COFF comparator proves padded bytes, relocation count/order/type,
semantic relocation destinations and addends, and normalized bytes for all
three functions.

## Provenance and typed reconstruction

- The January split `object_lights.obj` is the sole code and relocation
  authority. Its SHA-256 is
  `a398a56409b9cf4555ac93df1c208baf328253fc95d2ee6349b0f9bd175d66e9`.
- The HCEA semantic oracle is commit
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its independent lifecycle
  bodies call the same cluster-partition and data-array APIs, and its
  `lights_enable` reconstruction identifies the byte-sized `render_lights`
  field at offset zero.
- The repository's existing `object_lights.h` supplies all three public
  signatures. Its SHA-256 at the wave base is
  `f53306e74c03a5ebf3a6464d4aa4f7251cb9cbfa9e07c5c59e4da4480a5220bf`.
- The existing `data.h` and `cluster_partitions.h` declarations supply the
  typed `data_array` and `cluster_partition` APIs. A source-local
  `lights_game_globals` prefix names only the authenticated offset-zero
  Boolean and keeps the remaining three allocation bytes opaque as
  `reserved01`; a compile-time assertion proves its four-byte size.

The public header uses real point/color types without including their defining
header itself. The compiler front end therefore stopped before code generation
until `real_math.h` was included first in this translation unit. The unchanged
three-body candidate was then compiled and measured once; no body was tuned.

## Global ownership boundary

All three runtime globals are typed externals in the retained source. No
storage definition or data credit is claimed.

The January target records `_lights_game_globals` as value 0, section 65
(`.bss`), type 0, storage class 2. A tentative source definition emitted a
different COMMON representation: value 4, section 0, type 0, storage class 2.
It was replaced by the typed external required by this lane. The retained
candidate records value 0, section 0, type 0, storage class 2 and owns no
storage. A necessary ownership-only replay preserved all three exact function
hashes and relocations. In particular, `_lights_enable` retains its exact
symbolic relocation while the target resolves that symbol to its defined BSS
owner.

The target's 2,645 non-code bytes (`.bss` 4, `.data` 1,541, `.rdata` 1,100)
remain wholly uncredited.

## Scope and source policy

Only `source/objects/object_lights.c` and this new Jonas ledger are changed.
No shared header, configuration, prior or Claude-authored Markdown, or frozen
large-object source is touched. There are no file deletions.

The retained functions are ordinary readable typed C with explicit `void`
parameters, one parameter per line, and explicit terminal returns. They contain
no assembly, volatile scheduling, forced inline, optimizer pragma/barrier,
raw-address reconstruction, inactive-union read, type pun, signed-overflow
match, synthetic anchor, or byte/code-generation forcing.

## Regression and validation

The clean pre-wave translation-unit manifest reports exactly the three reviewed
`NEWLY_EXACT` functions, the expected compiler debug/symbol-inventory changes
from adding their definitions, and `changed_nonexact: []`. There were no
pre-existing exact functions in this unit to regress. No adjudication or
configuration exception is added; a clean committed-state snapshot/check and
forced post-commit replay close the structural gate.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct strict comparison: 3/3 retained functions pass.
- Semantic audit: 470 units, 3,972 functions evaluated, 3,832 semantic exact,
  3,893 accepted exact, and zero unit errors.
- Progress: 375/833 complete objects, 3,880/11,060 exact functions,
  471,649/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Object-admission audit: zero candidates and zero revocations, with only the
  inherited `source/shell/shell_xbox` contradiction.
- Parked-function validation: three active, zero stale, zero invalid.
- Tooling suite: 179/179 tests pass.

No GitHub push is performed.
