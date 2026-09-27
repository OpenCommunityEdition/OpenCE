# Claude type, static-local and disclosure reconciliation — 2026-09-27

Canonical baseline: `a2c73dd8997050e0ecd3ea4691a220ff41d1b258`.
This is an independent, zero-new-credit reconciliation of the completed
packets identified by the owner. The active donor worktree was not modified.
Only the frozen commits below were considered; no moving-tip merge was used.

## Included

- `03933e26`: the required game_engine storage prerequisite. Replace the
  invented aggregate with the separately evidenced debug global and teleporter
  static locals; rename exactly the two corresponding symbol rows. The later
  PDBs establish names/types/function scope, not every lexical placement. The
  playlist declaration placement remains explicitly inferred.
- `ba7d126f` (K): consolidate the HUD, item-collection and netgame types into
  their existing owner headers and remove the incompatible local views.
  Rebaseline only the approved ballistic-line-of-fire park, preserving its
  history. Its fresh 944-byte / 49-relocation hash is `ea5eb3a4...06692ae`,
  matching the donor measurement. It remains residual and earns no credit.
- `1d0e5c58`, `1bd50a79`: OQ-4 P-a and P-b1, after K. Restore the attested
  148-byte window_data view; give rasterizer_lights one complete type and
  owning-header extern. No COMMON definition is added. P-b2 through P-b4 are
  not included.
- `45070362`, `12fa21bb`, `944b6c27`, `a168f366`: the four remaining approved
  static-local packets in game_data_input, main, damage and render_sprite,
  together with their five symbol renames and one damage-index symbol row.
  Natural block placement and the demo-only damage-index name are disclosed.
  The game-data BSS repair places the server array at +0 and the build string
  at +0x28, as January does, instead of the inherited reversed ordering.
- `8b94691e`: 48 source-policy disclosure blocks in 37 files. These comments
  document existing behavior; they do not approve the underlying reachable UB,
  change byte credit, lift a hold or certify another object.

`f14b19c6` was not repeated: canonical `a567739b` already contains the
collisions static-local repair and its independently verified symbol row.
Canonical's provenance wording is retained.

Only exact patch hunks were reconciled. In particular, unrelated existing
global_window_parameters externs, the current hs ABI, king storage, and the
render_sprite string-table name were not replaced with donor whole files.

## G and game_engine admission do not pass on canonical

The complete `7663fc1a` G packet, without the unattested HUD macro, was applied
and rebuilt after storage + K. It changed the strict board from 7,641 to
7,640 exact rows: `_populate_statistic_buffer` (560 padded bytes) alone became
residual. No debit was authorized for this reconciliation.

G was therefore removed in full; the `57596ff7` Matching flip was never
applied. game_engine remains NonMatching, despite its already-exact functions
and data. This is the conditional source/admission ruling, not a rejection of
the independently justified K correction.

The subsequent OQ/static/comment packets touch none of G's compiler-source
dependency closure (103 current dependencies; 115 in the conservative G
overlay). They supply no changed-input reason to expect a different G result.
This dependency check is not a second compiled success/failure receipt: the
first failed G build/snapshot is preserved, but its full object/input archive
was not captured. No declaration-count or symbol-name tuning was attempted.
Reopen G only on new independently justified context/source evidence, then
require a fresh full sweep and 180/180 before admission.

The new upstream PR62 nav-point candidate also remains in scratch at zero
credit; its load-bearing same-type cast has no new source-policy approval.
Claude's still-running bitmap, COMMON, main-header, library and debit packets
are outside this batch.

## Independent verification

- Full configured build/progress/report generation passes, with objdiff
  **3.3.1 unchanged**. Storage + K, the additional type/static packets and the
  final combined tree each retain all inherited exact rows.
- Stable whole-board sweep: **8,252 rows / 7,641 exact; zero gains, zero
  losses**. No new Halo code or data credit is claimed.
- Parks **71 active / 0 stale / 0 invalid**. Admission audit **12 candidates,
  0 contradicted, 1 rejected, 0 revoked**. Fake scan retains the same **26**
  inherited leads. Whitespace check passes.
- Tests: **1,161 passed / 5 skipped / 26 subtests**. The initial baseline test
  invocation could not access pytest's default external temp directory; the
  baseline and final suite both passed using separate workspace temp roots,
  without changing tests, assertions or skips. Both receipts are retained.
- Focused game_engine audit: **263/263 January sections, 269/269 symbols**, PDB
  storage and BSS layout pass; all **192** BSS references agree. All **39**
  surplus definitions match January and current providers. Fourteen provider
  pairs pass **28** both-order duplicate-definition probes. These are bounded
  diagnostic links, not a complete executable link or boot test. The inherited
  ANY-versus-synthetic-NODUP selection differences and `.drectve` are unchanged.
- Nine OQ/static consumer TUs: **999 nondebug sections** checked. No added or
  removed sections, new COMMON, changed surplus helper, or selection/alignment
  change. The six static symbol/offset corrections agree with the fresh split.
  Damage's new scalar symbol replaces aggregate+0x48 references with scalar+0,
  preserving resolved addresses. GDI has the intended relocation/layout repair.
- Fresh `/W3` builds using a frozen pre-K source/header tree and final tree
  show identical warning multisets for those nine TUs. game_engine changes
  **21 -> 23** warnings: exactly the two disclosed C4244 long-to-short
  assignments for the attested fade_function and screen_flash_type locals.
  No unexplained warning is introduced; default production warnings do not rise.
- Comment-only rebuild proof: **622/622** saved/current objects have identical
  nondebug contents, flags, relocations and symbol/auxiliary records. The check
  covers 23,889 sections, 103,494 relocation records and 65,635 symbols,
  including 79 COMMON records. All 35 line-sensitive source uses preserve their
  logical file/line locations. Debug/timestamp differences receive no credit.

## Totals and publication boundary

- Halo meaningful code: **1,598,242 / 1,770,166 (90.29%)**.
- Halo accepted functions: **7,469 / 7,574**.
- Halo complete objects: **390 / 468**, unchanged by this batch.
- Halo credited data: **2,588,903**, unchanged.

The publication range also includes canonical's previously verified
`a567739b`, `a8b2512d` and `a2c73dd8` (static-local repair, path-obstacle
admission, and held object_lights evidence). Their credit is not counted twice.
Previously completed function work through `ef79aeda` was already present on
both remote exact-pilots branches at intake.

All focused reports, negative G receipts, snapshots, comparison scripts,
compiler/linker logs and input seals remain under
`scratch/astra_reconcile_packets_20260927/`. They are not publication payload.
Only verified source/configuration and this ledger are committed. Inherited
README edits, unrelated research, and private compiler/game/PDB assets remain
untouched. Publish the two authorized jonas/exact-pilots branches and merge
into halo-1 main while preserving its existing README and history; no force.
