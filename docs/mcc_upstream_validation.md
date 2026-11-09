# MCC upstream contribution and validation

## Documentation companion on OpenCE testing

`opence-mcc-documentation` starts from OpenCE `testing` at
`26aa36a93b75ea9330192be7c54275409d434f86`. It adds documentation only.
The companion implementation is
[`opence-mcc-support` at 1131d624b](https://github.com/chestahh/GulchCE/tree/1131d624b5c4e1f6e4d12174c4929452acf65285),
based on that same testing revision. Its common network agreement is OpenCE
**42** for Xbox, Custom Edition and MCC sessions. This documentation does not
change the protocol constant, generated wire-format record, source, tests or CI.

The documentation is extracted from
[`GET-YOUR-MCC-HERE` at 1db929c5](https://github.com/chestahh/GulchCE/tree/1db929c5e34101a45f644436e031016b5584c7bf).
The earlier extraction and refactor records below remain historical evidence.
Their full test suite, private runtime helpers and all recorded CI jobs are
not included in the smaller testing-based contribution. Missing implementation
and test links point to exact source commits; see the
[reading guide](reading/README.md#scope-and-provenance).

## Testing-based implementation checks

The [platform run for 1131d624b](https://github.com/chestahh/GulchCE/actions/runs/42603450409)
passed Linux, Windows, Android and web. Release publishing and Pages deployment
were skipped on that non-main branch. That revision adds a focused regression
subset without changing the implementation from its parent `a1b09a449`.

- Linux's existing suite and extended harness: **2,149 passed, 11 skipped**,
  with seven passing subtests.
- Selected native MCC/checkpoint checks in Linux CI: **531 passed, 3 skipped**.
- Local Windows selection: **700 passed, 9 skipped** across the initial run
  and an isolated rerun of the junction case outside the restricted sandbox.
- Selected browser unit checks: **18 passed** locally.
- Real Chromium and Firefox storage checks passed locally and in web CI.

Skips are not passes. These checks cover the submitted regression subset,
including malformed cache/tag inputs, sharing admission and publication,
metadata, console lifetime, boat initialization, checkpoint recovery and
browser storage. They do not repeat every historical Workshop gameplay run
or prove complete campaign, every-map or physical Android compatibility.
The broader bounded runtime observations and unresolved findings remain in
the historical records below; they are not relabelled as tests of 1131d624b.

## Full extraction record (historical)

The full GET-YOUR-MCC-HERE extraction was reconstructed from the completed refactor snapshot listed below,
on OpenCE main `fc9deb0b10eaae1d62caedb0710968b4e77e6e16`.
The exact source snapshot is `3d26678bf837335fb66a225c9c2fe1ad8626409c`.
The earlier [refactor ledger](mcc_refactor_validation.md) retains historical
format evidence and bounded runs on that source branch. Those runs do not
certify this extraction.

### Scope and ownership

MCC loaders, tags, script adapters, package discovery, rendering conversions,
menus, campaign manifests, sharing and browser storage remain independently
owned. Xbox `maps`, CE `custom_maps` and MCC `mcc_maps` remain separate.
The contribution omits Gulch's Modern/stylised menus, keyboard glyphs, optional
shoulder camera, lighting and unrelated fork changes. Existing upstream
controller glyph support is retained; an optional texture-release callback
allows MCC-owned glyph textures to be released before their bitmap headers.

OpenCE network agreement increases from 41 to 42. There is one protocol for
all map families, not a second MCC compatibility envelope. Join requests carry
the version before authentication state can change. Shared settings include
MCC identities (zero for legacy maps), with exact-size reassembly. Checkpoint
epochs, object recovery and cinematic poses are agreed shared state.
MCC transfers retain host authority, joined-peer admission, content identities,
bounds and both download opt-ins. See [NETCODE](../port/linux/NETCODE.md).

Shared fixes carried from the refactor correct existing contracts: initialize
all boat controls, preserve console syntax until its runtime consumers finish,
recover co-op state across checkpoints, synchronize scripted player poses,
retain the caller's GL texture binding when allocating a render target, and
join owned networking workers during shutdown. They are not map-name patches.
Minimal shared MCC hooks are necessary where the engine owns dispatch, tag
loading, rendering, widget lifetime or network message delivery; the actual
compatibility policies stay in MCC modules.

The stock selector reuses existing difficulty indicators for MCC completion.
The journal cache is keyed by profile and full mission identity, expires after
one second, and retains no widget/catalog pointer. Folder and MP entries do
not show campaign awards. Generated Back handlers unwind MCC selection state.

### Local evidence

Windows portable release, PGO/LTO disabled, clang 23, original Workshop inputs:

- Xbox b30 and Ruby's Rebalanced b30: two native peers, natural opening,
  61-second paired gameplay windows (122 position comparisons each), no judged
  gameplay divergences/problems, then engine-confirmed checkpoint and revert.
  Both processes stopped; original map hashes were unchanged. This does not
  establish complete mission or campaign compatibility.
- The earlier run labelled `upstream_ruby_b30_natural` actually staged Ruby a10;
  its old helper ignored the mission argument. Its drift-only result is not
  used as paired b30 evidence. The helper now records the real level and rejects
  zero paired position samples.
- Broad MCC suite: 2,502 passed, 13 skipped, one transient sharing fixture
  failure before the tested admission-revision mutation. Added failure-state
  diagnostics; all 69 sharing cases and 100 isolated repetitions subsequently
  passed. The original failure is retained and is not claimed fixed without
  a reproduced cause. The final broad run below passes.
- Stock menu, journal, texture ownership and preview checks pass. The glyph
  ownership test demonstrated a cache-hit release-order defect before the fix
  and passes after retaining the MCC callback on reused glyphs.
- Windows and web builds passed during integration. Seventy browser JS tests
  passed. Completed browser checks and platform CI are recorded below.
- The full harness initially encountered Windows CRT deprecation errors under
  `-Werror`; the Windows harness now defines `_CRT_SECURE_NO_WARNINGS`, matching
  the game build, without changing the functions being tested.

### Xbox a10 baseline assertion

The client assertion predates this refactor. It reproduces on the untouched
pre-Stage-1 build (`db1697c9b0c46289596f22213a5bef8ee327be24`, executable SHA-256
`f1ec0dca876aa3cf746a4095ac9115cc026902f4679d1c3941deb20928c85d56`) and pristine
OpenCE `fc9deb0` (executable SHA-256
`7f3106e7b69cf637e8cffaf99783685acfe1646c289ac5aeccdf928336d71605`).
Both used the same Xbox a10 SHA-256
`f7461e5b6eaf0314c4f03a1c6e43752b06ae3e4782b70b240a8c15e5f634b3ce`.

A pristine diagnostic build observed `tick=3362 bsp_index=8 cluster=2 count=2`
at `players_compute_combined_pvs`'s activating-cluster branch. Cluster 2 is
outside that BSP's two-cluster range. This is the activating-cluster field,
not the player's ordinary cached cluster. `objects_port_set_activating_cluster`
checks the range when setting it, but does not retain BSP identity. A stale
activation across a BSP transition is a plausible explanation; the exact
last setter/transition ordering has not been established. Upstream commit
`8e6b9a211e27e7439186c64cea77e1041b30f05a` introduced this path; it has not been
proved to be the introducing commit of the assertion.

A later attempted diagnostic linked pristine sources with modified objects;
its failed replication makes it invalid baseline evidence and it is excluded.
No production workaround was added. Under the agreed gate (fix if introduced
by this work), this is an upstream follow-up, not a passing a10 co-op result.

### First CI corrections

The initial CI build exposed omitted `host_mcc_memory.c/.h` and the Android
importer fixture. These are restored byte-for-byte from the refactored source.
The pause test now locates its actual MCC hook instead of a removed local
`extern` declaration. Windows fixture compilation uses the same empty CRT
macro as the fixtures, and CI installs SciPy for the existing button generator.

Two CE reader expectations were stale relative to upstream commit `29426d53c`
(`Fix custom edition protected map loading`): unused internal tag names may be
invalid and unreferenced tags may have address zero. The CE production reader
is byte-for-byte unchanged from the base. Tests now assert those accepted cases
and still reject invalid resource tag names and nonzero out-of-range addresses.
The focused rerun passed 147 tests with five environment/asset skips.

Real Chromium and Firefox imports, cancellation, persistence, transaction
recovery, delayed replies and map-window exhaustion checks pass locally.
Custom Edition Death Island also passed a two-native-peer multiplayer control:
122 paired positions over 61 seconds, no judged gameplay differences, original
files unchanged and both owned processes stopped.

### Final review and validation

The final code revision is `2b83ae7f92bebc0290d1f42458563e6f73faf601`.
The combined review covered MCC ownership and cleanup, stock menu generation,
shared settings admission, glyph texture lifetime, legacy code preservation,
regression fixtures and current reader documentation. It restored the upstream
cinematic alignment implementation and its original regression cases, removed
unused fork-specific fixture stubs and a duplicate declaration, and corrected
C89 declaration ordering. No further production simplification was justified.

The final local MCC selection passes **2,636 tests, 14 skipped**. Those skips
are environment/asset-dependent checks; they are not claimed as passes.
Seventy browser JavaScript unit tests pass. Targeted legacy texture, checkpoint,
teardown, inventory, cinematic and network-format fixtures pass after adapting
their extraction/stubs to the production interfaces. Faulty implementations
remain rejected by the existing negative controls.

Real Chromium and Firefox checks exercise imports, cancellation, persistence,
transaction recovery, delayed storage replies, simultaneous map windows and
memory exhaustion. Native/web WebRTC transport checks also pass in web CI.
The Windows and web portable release builds pass locally with PGO/LTO off.

#### Native host and Chromium client

Mercury Rising II was loaded from Workshop item `3021510694`. A fresh browser
accepted the observed stock-menu offer through normal input. The host sent
786,191,316 verified bytes in ten snapshot files: the selected mission and
required resource maps, metadata, supported thumbnails and receipt. The browser
rendered the package preview, loaded the map and simulated both players over a
connected bidirectional WebRTC channel. Snapshot hashes survived browser
shutdown/reopening, original Workshop hashes remained unchanged and the native
process exited normally.

That first run's full introductory comparison **failed**: two consecutive
position samples exceeded the unchanged one-unit limit during the initial
scripted vehicle sequence (maximum over the run 4.109 units). Its raw logs and
failed verdict are retained. It does not certify the cinematic or establish a
new production defect; the timing/replication cause remains unresolved.

A separate installed-package control on the final code used the same audited
browser snapshot with **both host sharing and client downloads disabled**.
It performed no new MCC installation. The complete comparison passes over
63 paired seconds/112 positions (maximum 0.633 units, aim difference 9 degrees),
and the two-live-player phase passes 49 paired samples/98 positions (maximum
0.028 units, aim difference 8 degrees). All 873 measured drift samples agree.
The two-live-player phase begins at the first common live state, tick 600,
and retains every subsequent observation. The cinematic ended naturally;
no skip input was sent. The private runner's generic phase-label text mentions
a skip, but the absent input receipt and zero log offsets establish that none
occurred. Neither tolerance nor sample timestamps were changed. This passing
control does not explain away the earlier failure.

These are bounded smoke checks, not complete campaign playthroughs or proof
of every cinematic. Xbox a10 remains the pre-existing assertion documented
above. No map-specific workaround or speculative simulation change was added.

#### CI and delivery

The [MCC validation run](https://github.com/chestahh/GulchCE/actions/runs/42545755452)
passes all three jobs: Linux readers/validators and sanitizers, GPU HUD rendering,
and Windows adapters. The [platform run](https://github.com/chestahh/GulchCE/actions/runs/42545755450)
passes Linux, Windows, Android and web, including the full Linux regression
job. Release publishing and Pages deployment are intentionally skipped on this
non-main branch.

Documentation-only finalization does not change the tested source or trigger
the repository's path-filtered build workflows. The Windows playtest package
is built after the final commit, carries that commit and per-file hashes, and
contains the executable, runtime DLL, notices and normal server examples.
It contains no retail maps, Workshop packages, user saves or fork-only menus.

Only verified fixtures and generated caches were removed after testing.
Inactive native saves/captures were archived losslessly (152 files, 621,124,460
original bytes, 38,910,232-byte archive), with every hash verified before loose
copies were removed and original paths recorded. Browser saves and metadata
remain preserved; downloaded maps and generated caches were removed only after
their completed audits. Original inputs, diagnostic logs, the final builds and
the pristine a10 baseline evidence remain available locally.
