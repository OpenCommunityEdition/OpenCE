# PathObstacles: exact pill query and authentic private helper (2026-09-06)

## Result

Starting at published `e148167df0908acb3f3423a1d873a3eb1cfd9cd0`, reconstruct
the previously unwritten `obstacles_test_pill` and its genuine private
`circle_intersect_ray` helper. Both match on their first natural compile:

| Function | Meaningful bytes | Padded bytes | Relocations |
| --- | ---: | ---: | ---: |
| `obstacles_test_pill` | 229 | 240 | 5 |
| `circle_intersect_ray` | 132 | 144 | 3 |
| Gain | **361** | **384** | **8** |

Canonical advances to **919,549 / 2,198,102 meaningful exact code bytes**,
**6,225 / 11,060 credited functions**, and **6,271 / 8,245 strict functions**.
Matched data remains **2,061,024 / 4,176,062** and Matching objects remain
**391/833**. PathObstacles is **8 exact / 1 residual / 7 unwritten**, not
complete or whole-object admitted. The earlier circle query stays fuzzy.

## January hex and source provenance

January's public query at 0x50B20 initializes an eight-byte result, scans
signed-short disc indices, uses the real header accessor/assertions, filters
optional bit zero when requested, and replaces the nearest result only for a
strictly shorter intersection. Equal-distance hits retain the first disc.
The signed-short ignore argument and byte-sized boolean are visible directly
in the instruction widths. Discs retain the corrected January 24-byte stride.

The result belongs in `source/ai/path.h`: `real distance` at zero, followed by
`short disc_index` at four and `short obstacle_index` at six. Size and offset
assertions accompany the genuine public prototype. HCEX DIA independently
corroborates this eight-byte layout. The private prototype stays in the C file.

Later semantic evidence, relative to the outer `i-w` workspace:

- `research-cache/halocea-full-blobs-20260830/src/blam/bitmaps/obstacles_test_pill.c`;
- `research-cache/halocea-full-blobs-20260830/src/blam/physics/circle_intersect_ray.c`;
- `research-cache/halocea-full-blobs-20260830/src/headers/obstacles_test_pill_result.h`;
- snapshot `570c83fd9c365dad6f2a3e7041705d5b84c7847c`;
- `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe` and
  `research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`.

DIA identifies the helper as static, not specified inline, with the authentic
argument order `(center, radius, point, direction, distance)`. Its source is
`d:\projects\code\hcex\sources\ai\path_obstacles.c`, lines 32 and 39-64.
The January public caller independently identifies the optimized private ABI:
EDX center, ECX point, EAX direction, ESI output-distance pointer, one stack
radius, AL result. Ordinary C reproduces that ABI without parameter reordering,
an exported helper, a calling-convention annotation, assembly or an ABI shim.

The helper forms center-minus-point, projects onto direction, rejects a
nonpositive or unordered projection, returns zero for a containing circle,
and otherwise computes the forward quadratic intersection. January's ordered
comparisons are authoritative over a later decompiler's NaN-unsafe inversion.
No direction normalization, radius clamp or invented geometry fix is added.

## Genuine name and private storage

The symbol at January file offset 329088 changes from `_code_00050580` to
`_circle_intersect_ray`, with `static: true` in `config/symbols.json`.
This is authenticated name/storage metadata, not a byte-comparison exception.
The source helper was static from its first compile. A separate final `named`
capture records the metadata correction; the earlier before/owner/first/final
captures remain immutable. Only the selected PathObstacles target and symbols
configuration change in this last step. All 572 base objects remain identical.
The selected target has the same 11,767-byte length and differs only at byte
8611: symbol storage 2 becomes 3. All 28 section headers, payloads and
relocations are unchanged. No executable byte was edited.

The public function's normalized SHA-256 is
`8524208b95d4c9be122cf038216dadd2a6386839552ee2fe8dcf228719af355c`;
the private helper's is
`833877701c647f823dcb6876411ba17f81d71dde649f60515850561559a509cf`.
All eight relocation offsets, types, destinations and zero addends agree.

## Header, whole-object and regression checks

The real result header has 51 current consumers. The owner-only trial preserves
all 3,302 inherited runtime sections: 3,245 unchanged and 57 differing only in
proven compiler-local label spelling. All 6,269 inherited strict functions
survive both the owner-only trial and the completed packet. Units stays 189/189;
PathObstacleAvoidance keeps all nine exact functions.

The natural implementation adds only the public/private pair, an ordinary
zero literal, and header fallback copies of `dot_product2d` and `square_root`.
There is no new writable/COMMON owner, no new `point_from_line3d` COMDAT,
no source alias, no forced emission or helper suppression. Existing PathObstacles
aliases are unchanged; this packet does not silently certify them.

Direct current-base VC7 link diagnostics with ActionCharge fail in both input
orders on **three** duplicate helpers: `square_root`, `dot_product2d`, and
the already-present `magnitude_squared2d`. ActionCharge's NODUPLICATES owners
conflict with ordinary SELECT_ANY header fallbacks. Two helper copies are new
with this packet; the magnitude-squared copy predates it. Exact function bytes
do not resolve these whole-object ownership conflicts. These copies get no
additional January function credit and PathObstacles remains incomplete.
The complete current-base census finds seven exact `dot_product2d` providers,
60 exact `square_root` providers and 15 exact `magnitude_squared2d` providers.
Each family has one ActionCharge NODUPLICATES definition and otherwise
SELECT_ANY definitions. Thus the newly emitted copies do not create a new
conflict family; the authentic global ownership problem already exists.
Do not suppress or rename helpers to conceal this; pursue authentic shared
ownership separately and preserve inherited strict matches.

Root's final artifact replay pins all 1,405 current base/split objects. The
51 first/final/named base consumers remain byte-identical. Gate-to-ordinary
changes only the compiler debug filename; all runtime sections and the complete
symbol inventory agree. No C or header variant follows the first natural body.

## Bounded actual-machine-code replay

`tools/audit/path_obstacles_test_pill_runtime.py` executes each real January,
first-gate and final ordinary public/helper pair together, following the
object's actual call relocation. The private helper is neither stubbed nor
called through a fabricated standalone ABI. Only `display_assert` and the
nonreturning `system_exit` are modeled.

The verifier checks **43 scenarios x two x87 modes x three roles = 258
executions**, including 1,752 actual helper calls. It covers nonpositive and
oversized counts, signed-short ignore and byte boolean widths, optional bit
zero, nearest-hit and tie behavior, all 128 discs, last-record access, result
initialization on miss/assert, tangent boundaries, signed zero, subnormals,
quiet NaNs, infinities and negative radii. It checks read-only inputs, whole
output-page guards, caller argument storage, actual helper registers/stack,
public return state, preserved registers, direction flag and x87 state.

Three guarded in-memory actual-byte negative controls must fail: inverted
nearest selection, reversed helper distance subtraction, and the wrong
optional flag bit. No disk object or production code is patched. The helper
mutation's initial test-harness offset was corrected to its actual ModRM byte
at +0x72 during review; this is not a production-source retry.

FPCWs 0x027F and 0x037F are disclosed masked conventional environments, not
proof of the game's process default. The finite oracle does not cover all
floats, signaling NaNs, arbitrary pointer aliasing or unmasked exception timing.
Actual target/first/final outcomes agree for the tested environments.

Full Ninja and **650 tests plus 26 subtests** pass. Parks remain **297 active /
zero stale / zero invalid**. Admission has no new candidates, contradictions or
revocations and retains five existing rejections. The two changed production
files have zero fake-match scanner leads. Active Opus implementations and donor
worktrees are untouched. Original binaries, PDBs and scratch artifacts remain
local; only source, metadata, reusable verification tools and this ledger ship.

## Frozen evidence

- Capture manifests: `scratch/path-obstacles-pill-{before,owner,first,final,named}-20260906.json`.
- Named final manifest: `75b2a1cbd2ab318d79675362e754cab1ec2f176d0dd412992c862c4f12a7311d`.
- First gate object: `9d51c96acd5fce2affd58fc1a83c66af11641d325b1629d9f14391bc7e592743`.
- First/final base object: `4b6ac1461e309b1297d22ad384d30a983ee2136dea73fed6fc1be9e31ac98a41`.
- Named selected target: `f438f10d52492b5dd4ae35691a50cb10240d30ad01f61a81be57542b0e224a29`.
- C/header SHA-256: `9bd08252b0f69445eda8fd6756aa5b9d5b3da58aab65b8b60faeb4f571051fee` /
  `c27e7239287d950562dacc7403b80c079e721c82d3771c10357f75d55a7e4913`.
- Symbols configuration: `a0dc47c1b4e853b72c60c2fea3fcfbd464e2bf8eba4e1e012606b6f23c643b29`.
- January zero-literal owner: `scratch/path-obstacles-pill-january-zero-owner-20260906.obj`,
  SHA-256 `76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea`.
- `scratch/path_obstacles_pill_final_artifacts_20260906.py`, SHA-256
  `400eabb7f47aacc170e6333aa0aa2d720d5eda03903706bd9310602cf64941c7`.
- `scratch/path-obstacles-pill-named-stable-20260906.json`: two gains, zero losses
  against `scratch/path-obstacles-circle-final-stable-20260906.json`.
- Root final runtime JSON and independent final runtime JSON agree:
  `1e9dd63edd66b5780df77f7062cbb2d0236967968b3e510928e42891d3e07130`.
- Runtime tool SHA-256: `bdb70a257864f50209d52269b5cce66a1c0d18091c4e0b3923cda7a18863831f`;
  nine tests: `92d1df6bdcdda747f7861ad9a81e1a9ad85fc9bfa44b20c2f484039874dcd131`.
  Both include the stronger null-geometry optional-disc skip check.
