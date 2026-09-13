# Eighth 10K lane-reconciliation batch (2026-09-13)

## Canonical result

The frozen published base is
`eb61c0f7469a683ffaa70bf8aae18d828834e7aa`, with 6,918 accepted functions
and 1,123,718 meaningful accepted exact code bytes.  The reconciled build has
**6,960 accepted functions and 1,134,830 meaningful bytes**: **+42 functions
and +11,112 bytes**.

The independent fail-closed target-section census records **41 new strict
functions, +9,890 meaningful bytes, +10,192 padded bytes, and zero
regressions**.  One additional function,
`player_profile_3wide_list_update`, is admitted through the semantic
relocation-shape gate described below.  No fuzzy function, candidate-only
helper, or rejected relocation-owner result contributes to these totals.

| accepted packet | functions | meaningful | padded |
|---|---:|---:|---:|
| AI vehicle seat selection | 1 | 292 | 304 |
| AI firing-position evaluation | 5 | 898 | 928 |
| AI look-vector validation | 1 | 77 | 80 |
| AI script migration | 1 | 269 | 272 |
| encounter post-combat possibility | 1 | 132 | 144 |
| bitmap rebuild | 1 | 115 | 128 |
| transport endpoint-set lifecycle | 1 | 41 | 48 |
| Winsock connection/bind lifecycle | 3 | 461 | 480 |
| stack-walk initialization | 1 | 25 | 32 |
| particle-system renderer | 1 | 155 | 160 |
| weapon-HUD private packet | 4 | 420 | 464 |
| Main framerate renderer | 1 | 666 | 672 |
| LRU/V cache block allocation | 1 | 1,302 | 1,312 |
| network-server write wrapper | 1 | 24 | 32 |
| collision sphere-feature collector | 1 | 533 | 544 |
| physics collision/update pair | 2 | 972 | 992 |
| Xbox environment-fog packet | 5 | 1,109 | 1,152 |
| Xbox visibility-test wrappers | 2 | 19 | 32 |
| render-camera packet | 3 | 863 | 880 |
| game-sound PAS computation | 1 | 236 | 240 |
| structure cluster traversal | 2 | 585 | 592 |
| biped collision/damage pair | 2 | 696 | 704 |
| 3-wide player-profile UI | 1 | 1,222 | 1,232 |
| **total** | **42** | **11,112** | **11,424** |

## Exact-accounting reconciliation

`source/interface/ui_widget_game_data_input_functions::_player_profile_3wide_list_update`
adds 1,222 meaningful / 1,232 padded bytes after compiling naturally to
January's normalized code hash with all 71 relocation offsets and types.  The
strict symbolic comparator remains fail-closed at two genuine COFF naming
boundaries: `cached_player_profile + 0x9C` is the same address that January's
split labels as the immediately following
`local_player_index_for_draw_string_and_hack_in_icons`, and one identical wide
literal is represented as defined `.rdata` versus an undefined symbol.  No
source alias or fake global was added.  The dedicated UI ledger records the
full evidence and supersedes its earlier fuzzy classification.

Conversely,
`source/ai/path_obstacle_avoidance::_render_debug_obstacle_path` receives
**zero credit** despite objdiff's raw 100-percent report.  Its 224 normalized
code bytes and relocation positions/types agree, but January's split assigns
the BSS references to `_current_traverse_index` and an interior addend while
the coherent source correctly owns `debug_path` and `debug_obstacles`.
Renaming those globals to split artifacts or spelling raw BSS offsets would
violate the naming and source-authenticity rules.  A new fail-closed semantic
credit-veto manifest pins both complete ordered-relocation fingerprints and
automatically debits this result.  Missing or stale veto evidence now fails
the global audit.

## Source and ownership review

- Every admitted body uses its semantic or authenticated private name; no new
  `code_XXXXXXXX`, `bss_XXXXXXXX`, or `data_XXXXXXXX` owner was introduced.
- New shared runtime layouts are separated into the narrow owner headers for
  mass points, powered mass points, friction, vehicles, and rasterizer model
  packets.  Changed header consumers were rebuilt globally.
- Typed tag and object retrieval uses the subsystem access macros.  The two
  added direct `tag_get` casts occur only inside the typed HUD-definition
  accessor macros themselves.
- Particle, rasterizer-water, camera, AI, physics, collision, and transport
  prototypes were moved to their owner headers or retained TU-private as
  required; consumer-local declarations were removed.
- `/QIfist` remains narrowly limited to the already evidenced translation
  units.  No new optimizer pragma, volatile/register steering, forced
  inline/noinline, fake dependency, raw-byte emission, or nonsensical branch
  was admitted.
- The changed-source scan reports only seven inherited and documented
  low-level constructs: the naked CSeries string helper, the stack-walk ABI
  pair, the Decals math helper, and HUD's return-address helper.

The final emitted-helper census introduces no `_point_from_line3d` COMDAT.
The current candidate set is smaller than the published base because the
Collision and Vehicle surplus copies disappeared.  `units.obj` remains 189
exact / zero residual / zero unwritten; its pre-existing candidate-only helper
debt is unchanged and receives no object-completion credit.

## Retained best fuzzy source

House-rule-compliant complete or improved fuzzy bodies from the Claude/Fable/
Opus lanes remain in canonical source and in the 465-entry park manifest with
zero exact credit.  The batch ledgers record their current sizes, relocation
identities, similarity, provenance, and reopen conditions.  Notably, the
Biped turning body was deliberately changed from stronger pointer-punning to
safe typed 2D locals even though that lowers similarity; source correctness
takes precedence over the score.

The final donor census finds no high-confidence unaudited Claude payload.  The
remaining donor-only union consists of 71 functions / 29,958 meaningful bytes:
38 functions / 17,047 bytes already correspond to canonical parks, and the
remaining 33 / 12,911 are documented rejects, ownership blocks, or stale
aliases.  These are research inventory, not unintegrated approved progress.

## Publication gate

- Full configure/Ninja build, semantic report, and progress report: pass.
- Accepted ledger: 6,960 functions / 1,134,830 meaningful exact bytes.
- Stable target-section sweep: 41 gains / 9,890 meaningful / 10,192 padded /
  zero regressions.
- Semantic relocation-shape admission: one function / 1,222 meaningful bytes.
- Manual semantic-credit veto: one function / 224 bytes explicitly rejected.
- Units focused universal gate: 189 exact / zero residual / zero unwritten.
- Park audit: 465 active / zero stale / zero invalid.
- Object-admission audit: zero candidates, contradictions, or revocations;
  six known whole-object rejections remain visible.
- Tool tests: 1,147 passed, 5 skipped, plus 26 subtests passed.
- `git diff --check`: pass apart from checkout line-ending notices.
- Stable snapshot: `scratch/batch8-final-veto.json`, SHA-256
  `bae54b8559e2ea369371fa59cdfe7f73ca2d7c7e2114088cc2b0cc0be8ed8764`).

The canonical treemap is refreshed at `http://127.0.0.1:8765/`.  The next
10,000-byte cadence threshold is **1,144,830 meaningful accepted exact bytes**.
