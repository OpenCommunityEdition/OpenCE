# MCC refactor milestones and validation

## Milestones and current position

Updated 8 November 2026. Milestones retain the existing stage numbers, so
"Stage 6" and "Milestone 6" refer to the same work. Stages 1–8 are complete;
Stage 9's combined review, testing and cleanup are complete. Ruby co-op
recovery failed on both original main and the Stage 9 candidate; those failures
remain recorded. Milestone 10 fixes the demonstrated shared replica-lifetime
defect under a separated network agreement. Its final Ruby, Mercury and Xbox
checkpoint checks, rejoin and native/browser transport controls pass within
the limits below. Milestone 10's implementation, validation, combined review
and verified cleanup are complete. This qualifies the demonstrated recovery
repair without asserting full campaign or every-map compatibility.
The shared boat correction, Mercury's shared clock-recovery correction and
separately authorized console-syntax correction are implemented under
Milestone 11. Its combined review, final native/Internet checks, both CI
revisions, verified cleanup and archived-evidence replay are complete. On
7 November the user explicitly removed the old-client crossplay constraint
for both milestones; the 8 November request also supersedes the console deferral.
The stage evidence below records the changes, actual validation and limits.
The subsequent delivery plan adds Milestone 13 for host sharing, Milestone 14
for Custom Edition and MCC browser playability, and Milestone 15 for the
upstream contribution. Milestone 13 is complete within the validation limits
recorded below. CI through `a972f450a`,
native downloads, metadata-mismatch preservation, managed removal and Mercury
follow-on gameplay have passed within the recorded limits. The dedicated
map-only admission correction passes its native rerun at `f8432e2a3`.
Further initial-download and on-demand transition runs exposed host loading
that starved pending clients. The general MCC loading barrier at `a972f450a`
passes both native reruns within the recorded limits, and that commit's CI
passes. Final Xbox/CE native controls, all three native/browser networking
controls, combined implementation review and verified cleanup are complete.
Milestones 14 and 15 remain planned.

| Milestone | Scope and outcome | Status |
| --- | --- | --- |
| 1 | Establish executable regression coverage, CI and Workshop runtime baselines. | Complete |
| 2 | Reproduce the shared boat defect and establish the crossplay constraint. | Investigation complete; corrected in Milestone 11 |
| 3 | Remove redundant MCC resource ordering and bitmap ownership lookup work. | Complete |
| 4 | Replace repeated per-texture stream scans with a local per-tag cursor. | Complete |
| 5 | Correct HUD ownership, remove bitmap-name dependence and trim duplicate normalization. | Complete |
| 6 | Consolidate duplicate checksum work with exact compatibility and linkage proved. | Complete |
| 7 | Consolidate script resolution, reject unused checkpoint object handles before restoration, and audit state ownership. | Complete; console defect corrected alongside Milestone 11 |
| 8 | Retain justified HUD exceptions and support MCC-wide AI deselection through its real native service. | Complete |
| 9 | Review the combined branch and extend runtime, lifecycle and performance validation. | Review/testing/cleanup complete; recovery failure addressed in Milestone 10 |
| 10 | Correct shared co-op checkpoint recovery with explicit lifetime and network agreement separation. | Complete; bounded recovery checks pass |
| 11 | Correct shared boat controls, small client clock delays exposed by Mercury, and the separately authorized console lifetime defect. | Complete; bounded runtime, platform and network checks pass |
| 13 | Review PR #95, complete host-only MCC package sharing, campaign progression and host opt-in menus. | Complete; final review, native/transport controls, CI and cleanup pass within recorded limits |
| 14 | Make Custom Edition and MCC maps playable in browsers, including import/download, persistence and native/browser multiplayer. | Planned; explicit requirement, not satisfied by a web build alone |
| 15 | Prepare the complete MCC-only upstream contribution and unify its network agreement with OpenCE. | Planned after Milestones 13 and 14; previously numbered 14 |

The completed Milestone 10 gameplay implementation is `67968518222f3ee59361670323cfcd48486fa97e`,
with passing MCC and four-platform debug workflows and 87 focused local passes.
The comment-only follow-up `7ed70168a` also passes both workflows.
Its [validation record](#stage-10-shared-checkpoint-recovery-fix) distinguishes
the authenticated local native build from the exact-revision CI Web build,
pre-final checks from final checks, and passing transport from unmeasured
cross-log state equivalence. Its then-current agreements were base 32714, MCC 66 and
camera 32716/32717; historical verification used OpenCE 41/MCC 65.
All 21 structural HUD profiles remain with measured evidence and removal
conditions. On 8 November the user explicitly authorized the shared console
syntax correction alongside Milestone 11, superseding its earlier deferral.
Milestone counts are not an estimate of percentage effort remaining
and do not establish equal audit depth for every MCC module.

All future changes remain evidence-backed, with general engine defects fixed
in the responsible implementation and MCC format defects fixed systemically
inside MCC-owned modules. Preserve Xbox/CE behavior, map-family ownership and
unrelated functionality. Milestones 10 and 11 may separate incompatible
network versions; the earlier crossplay-dependent deferrals below are historical
evidence. Each milestone includes design review, meaningful regression
coverage, the applicable platform/network checks, final review and cleanup.
Later integration checks supplement these checks; they do not postpone them.

### Milestone 13: host sharing and campaign delivery

Analyze the actual changes and discussion in
[PR #95](https://github.com/chestahh/GulchCE/pull/95), provide evidence-backed
recommendations and draft answers to the contributor's questions before
implementation. A draft response is not authorization to post it. Adapt the
reviewed contribution to this refactored branch, preserving contributor
attribution, rather than treating its older `testing` base or protocol numbers
as current. Complete the campaign manifests, follow-on missions and host menu
entry that the PR explicitly leaves out of scope.

MCC transfers have one source: the host. Host sharing is opt-in and off by
default; honor the joiner's download preference. The user has explicitly
approved extending the PR to eligible Workshop packages as well as hand-placed
packages. Use one transfer/validation implementation for both origins. Preserve
Workshop attribution and links, apply content eligibility and redistribution
restrictions, and retain subscription/manual installation as an alternative.
Workshop presence alone does not establish eligibility or validate received
bytes. Review the host-local classification list, manifest identity, staging,
installation and failure cleanup. Campaign delivery must declare and validate
mission dependencies and progression. The agreed policy is to obtain and verify
the campaign manifest upfront, then download and verify each mission and its
required resources before that mission starts. Reuse matching verified local
content rather than downloading it again. The planner now includes campaign/mission JSON upfront with only the current
mission binary. The Mercury integration runs below verify follow-on download,
admission and bounded gameplay progression; component parsing alone does not
establish them.

Offline/rehost limit: managed mission snapshots remain individually playable;
they do not form a complete campaign by combining folders. The campaign
contract in [`mcc_manifest.h`](../port/linux/src/mcc_manifest.h) contains ordered
names/titles, not future mission gameplay digests. Locally generated receipts
record the received current files and provenance, but cannot establish which
other snapshot is the intended next-mission revision. Consequently
`mcc_maps_next_mission` retains its missing-within-package refusal. Matching
JSON, Workshop IDs or a unique leaf-name candidate are insufficient evidence
to relax it. Connected clients follow each exact host-advertised identity;
offline/rehosted progression needs the complete Workshop/manual package.
Automatically linking downloaded snapshots would require an additional
authoritative per-mission identity or recorded transition contract and is not
implemented by M13.

Keep host-delivered content separate from Steam-managed and manually installed
packages. Identify the host's exact package/dependency snapshot by content
hashes, reuse a matching local copy, and never overwrite another installed
version to satisfy a join. Received content is a managed copy, not a Steam
subscription or a promise of automatic Workshop updates. Show attribution,
storage use and removal controls without treating saves as disposable data.

The agreed join flow detects missing or differing
content before gameplay and offers a size-labelled host download, the Workshop
page when applicable, or cancellation. Host-download acceptance starts the
validated transfer and resumes joining only after successful installation and
identity checks. Use the existing `network.map_download` client permission and
Download Maps menu switch for MCC as well as CE. Preserve its existing default
(on) and CE behavior; update setting/menu help to explain MCC host-only downloads.
For MCC, this permission allows the offer; accepting the prompt starts the
package download. If either host sharing or client downloads is off, omit the
host-download option, explain the applicable setting and direct Workshop users
to the Workshop page. A matching local
copy needs no download prompt. Workshop/manual installation must be rechecked:
the current Workshop revision may still differ from the host's snapshot. On
web, opening a Workshop page does not install files into OPFS; manual content
must be imported through the browser's package importer. Keep this distinction
explicit in the design and tests.

The user has confirmed the sharing-disabled case: when the host has MCC map
sharing off, a Workshop package mismatch offers only the Workshop page as a
content-acquisition action, plus cancellation/dismissal. Omit the host-download
option and explain that the host has disabled sharing. Do not label a differing
package as older or newer unless reliable version information establishes that
ordering. If no valid Workshop reference exists, explain that matching content
must be obtained manually rather than presenting a broken Workshop action.
These rules are agreed; final wording and layout remain implementation details.

The user has approved sharing required JSON metadata and optional PNG previews
as part of Milestone 13. Allow only recognized package metadata, including
`ModInfo.json`, `CampaignInfo.json` and referenced mission/map metadata, plus
explicitly referenced supported PNG previews. Do not transfer arbitrary JSON,
images or other Workshop-folder contents. Apply the same host-only sharing
permission and validated staging/install flow to these files.
This explicitly includes version-mismatch downloads: accepting the host's
version transfers its required JSON metadata and eligible optional PNG previews
alongside the applicable maps/resources. Do not retain stale metadata or artwork
from the client's differing package. Keep these files tied to the same host
snapshot and preserve the agreed mission-by-mission campaign delivery policy.

Local folders such as `mcc_maps/<package>/` use the same eligible-content,
host-only transfer and client-permission rules without requiring Steam or a
Workshop ID. Dedicated servers must be able to configure sharing and eligible
packages without menus, with actionable validation/refusal logs. Serve an
explicit package snapshot, not arbitrary folder contents; include supported
metadata and previews when present. Simple standalone map packages need not
invent campaign JSON or thumbnails; campaign progression requires valid campaign
metadata. Use content identity rather than folder names to match existing copies.
When downloads are unavailable, show a valid Workshop page if the local package
has a Workshop reference; otherwise explain which package/content is required
and that it must be obtained from the server operator and installed manually.
Do not invent Workshop links or automatically fetch arbitrary host-supplied URLs.
Retain the existing exclusions for loose root maps, stock content and `_shared`
resource directories as transfer packages; report required unavailable
dependencies instead of silently copying global resources.

Validate received metadata fields and package-relative paths; prevent traversal,
symlink/junction escapes and overwrites of existing installations. Bound file
counts, individual and total byte sizes, parsing depth and decoded image memory.
Retain the supported PNG subset and validate actual content, not just filenames.
The existing readers provide a starting point: JSON is limited to 256 KiB per
file, and PNG previews to 8 MiB and 2048 by 2048 pixels. Review aggregate budgets
and enforce receiving-client checks before accepting network-delivered files.
Treat descriptions as plain text; never execute metadata or automatically fetch
its URLs. Content hashes establish agreement with the host's manifest, not
safety. Keep optional preview failure separate from required-content admission:
reject invalid artwork and use the existing placeholder; invalid required
campaign metadata blocks installation with an actionable explanation.

Validation must include malformed JSON and PNG inputs, parser/decoder fuzzing,
path escapes, oversized and excessive files, truncated or hash-mismatched
transfers, optional-image fallback and required-metadata rejection. Verify the
resulting menus and campaign progression. Exercise all host-sharing/client-download
setting combinations, differing-version metadata/artwork, local-only dedicated
server packages, and packages with and without Workshop references. These are
agreed implementation and
validation requirements, not a completed security audit or test results.

Validate real transfers and resulting gameplay, host/client authority,
disabled settings, content changes, interrupted transfers, destination
collisions, hostile input, resource ownership and follow-on missions.
Preserve existing Xbox and CE behavior. Browser completion is an explicit
Milestone 14 gate, not inferred from this PR's CI or native transfer tests.

### PR #95 review and draft contributor response

Review source: [PR #95](https://github.com/chestahh/GulchCE/pull/95), its current
body/questions and issue discussion, inspected 8 November 2026. The PR targets
`testing`, is still a draft, and reports six commits at
`98130b623cc252f99ad063abf59b15040e438c9f`. Its author explicitly requests real
two-PC testing; the reported green CI and synthetic tests do not establish
that result. The issue discussion adds no technical question beyond the three
in the body. No response has been posted and no PR description has been edited.

Five contribution commits were cherry-picked with original attribution and
`-x` provenance into the dedicated refactor branch:

| Source commit | Refactor-branch commit | Contribution |
| --- | --- | --- |
| `9e74e1fe6be1ff8ad32dae779e373921e25e5637` | `46cba2bba` | Staging, explicit host list and transfer session |
| `a1a5b12dd337ab939b62e9ebab70d1beb4c4fd8e` | `2d4defa7f` | MCC wire/game integration |
| `1a12588c54e52d3e9d19086e19d122351aef6120` | `f041f08b0` | Documentation and CI |
| `4585f73da25c7f208bf98475eed52ef78b344779` | `5fd155415` | Longer staging/data-root paths |
| `686ba07a7ba9ccdd00e1ada7397026a98c87bddb` | `153c607fc` | Original Linux-only game-harness negative controls |

The sixth PR commit, `98130b623`, merges the older `testing` branch into the
contribution. It was not cherry-picked over the refactored baseline. The
adaptation is committed through `a972f450a`, including the later metadata,
managed-storage, identity, UI and loading-barrier corrections. The original
cherry-pick messages describe the contribution at import, not the resulting
M13 contract; later commits and the evidence below record its adaptation.

Review recommendations and their rationale:

- Preserve the contribution's MCC-owned policy, list, transport/session and
  staging separation. Its small shared hooks are needed because lobby holds,
  cache admission and dispatch belong to existing engine call sites; keep CE
  and Xbox implementations and message families intact.
- Retain the explicit exact-content/maker list. A folder name, Workshop ID or
  received manifest is not a custom-content classifier. List the gameplay
  digest of each approved mission and its used resources; required JSON has
  its own agreement digest. Do not imply that a hash grants rights or proves
  arbitrary data safe.
- Replace the older wire assignments, not the branch's newer join/camera
  envelopes. The integrated agreement is base 32726, MCC 69, camera
  32728/32729; MCC transfer messages are 103/104. The PR's proposed 65 and
  102/103 are historical and conflict with subsequent branch work.
- Add an explicit client waiting/acceptance state, bounded paged offers,
  backpressure handling and upload accounting for manifests as well as chunks.
  Reject hostile fields and messages from other peers; invalidate offers when
  session identities or host preparation change. Component regressions and
  selective fault controls exercise these guards.
- Replace destination-by-host-name and file-by-file publication with exclusive
  staging and atomic no-replace publication of a separate managed snapshot.
  Same-name existing packages remain usable. Stage tests cover pre-existing
  stages, path collisions, partial-file ownership and native junction refusal.
- Select resources from the validated loader result rather than copying nearby
  files. Actual resource-reader tests establish that self-contained maps ignore
  unused invalid siblings, required overrides are authoritative, and root/shared
  selection differs from package ownership. Transfer tests cover Workshop
  fallback paths; shared stock dependencies refuse an incomplete offer.
- Treat `ModInfo.json` as parsed metadata, not credit text. Use a bounded schema
  and reference allowlist for JSON/PNG; compare required-JSON identity, discard
  unsupported optional previews, and retain the gameplay probe before install.
  Workshop packages and local map-only folders share this implementation.

The user explicitly brought the PR's campaign manifests/follow-on missions,
Workshop delivery and host menu switch into M13. Campaign metadata is planned
upfront; mission binaries/resources are obtained as needed. Managed snapshot
attribution, storage use and safe removal are also required. Fetching from
other players remains excluded. Browser import/download/persistence is M14;
the MCC-only upstream protocol merge is M15. Neither is implicitly completed
by adapting this PR. Mac remains outside the requested platform scope.

**Draft response, not posted:**

> The host-local `mcc_share.txt` list is an appropriate explicit classification
> source for this work. Keep exact gameplay fingerprints and maker credit,
> refusing unknown content. Workshop origin alone will not qualify a package;
> eligible Workshop and manually installed packages will use the same checks.
>
> We should raise the agreement for the new wire contract now. On the refactored
> branch the current numbers are MCC 69, shared 32726 and camera 32728/32729,
> with transfer messages 103/104. Your proposed MCC 65 and 102/103 belong to the
> older base, so they cannot be copied directly into this branch.
>
> Include the host menu switch in Milestone 13. It stays off by default. Clients
> keep Download Maps and additionally accept a size-labelled offer before MCC
> file transfer. We are also handling the campaign/Workshop/metadata and managed
> snapshot work you left out of scope, while preserving your contribution's
> attribution and MCC-only boundaries.
>
> We have component evidence for consent, refusal, file integrity, cancellation,
> resource selection and preservation of existing packages. Native Ruby and
> Mercury download, admission and campaign follow-on checks now include real
> gameplay using two processes on one Windows PC. Those local loopback runs
> do not satisfy your two-physical-PC request. The final local native/browser
> networking controls and platform checks also pass, with their limits recorded.
> The adapted expectations differ in three places:
> a same-name different version installs separately; eligible listed Workshop
> packages can transfer; and disabled downloads offer manual/Workshop choices.
> The validation record identifies the tested revisions, map identities,
> settings, logs and remaining limits. Browser MCC playability is separate
> Milestone 14 work, not a result claimed by these Xbox transport controls.

### Milestone 13 implementation evidence

Status at this documentation pass: **complete within the recorded limits**.
The final gameplay implementation is `a972f450a1da46c2dcdc15b3c30accd473f86941`;
this closeout changes documentation only. CI through
`a972f450a` is green. Native Ruby download/late-join, metadata-mismatch
preservation, Mercury's download and follow-on gameplay, Xbox/CE controls and
managed snapshot confirmation/removal have passed within the limits below.
The later Mercury run includes the combined review's attribution/UI corrections.
The first dedicated map-only run fails admission after verified installation;
the completion-to-admission grace correction passes its native rerun at
`f8432e2a3`. Subsequent initial-download and on-demand transition runs expose
a separate host-loading timeout. The general MCC loading barrier at `a972f450a`
passes the initial-download and on-demand reruns recorded below. Final Xbox/CE
native controls, three native/browser Xbox networking cases and verified
cleanup also pass. Earlier stages' runtime passes do not validate the changed M13
transfer/identity implementation. All native pairs here are two isolated
processes on the same Windows PC, not tests across two physical PCs.

The current implementation includes host-only opt-ins, client consent and
manual/Workshop alternatives; paged bounded manifests; independent gameplay and
required-JSON digests; JSON/PNG reference validation; local map-only packages;
validated package-resource selection; and exclusive stage publication into a
separate `dl_<digest>` snapshot. The ownership record/removal API in
[`mcc_share_storage.h`](../port/linux/game/mcc_share_storage.h) is an integration
contract: listing/removing user-visible managed copies and preventing removal
of the active package are covered by native storage and menu fixtures. Native
managed removal and the corrected dynamic confirmation label are both observed
in the completed `ruby_storage_final` rerun below.
No network manifest may supply a local ownership receipt.

Recorded component runs:

The native Ruby download exposed a real allocator-boundary defect before
publication: the adapter used libc `calloc` with engine `debug_free`. The
symbolized crash reached `mcc_share_identity_check` through `client_check`;
the correction pairs engine `malloc`/`free` and explicitly zeros the entries.
The adapter fixture now distinguishes these allocators, and a before-fix
mutation reproduces the mismatch. Its 43 cases pass. This is an MCC adapter
fix, not a change to the shared allocator. The corrected Ruby download and
late-join rerun passed as recorded below. Native consent inspection also caught clipped text and an
incorrect lobby return; the existing MCC wrapping and engine go-back services
now handle the prompt, with observable restoration covered in the adapter.

The later integrated component selection passed 248 cases with five explicit
platform/privilege skips; its one failure was sandbox denial of junction
creation. Rerunning the complete staging and storage suites with junction
creation available passed 70 cases with one unavailable file-symlink skip.
These are overlapping selections, not additional totals. The Windows release
executable builds and corrected-commit CI passes. The final platform/runtime
results for the later corrections are recorded below rather than inferred
from this earlier selection.

| Evidence | Actual result and limit |
| --- | --- |
| `build/m13-stage-results.txt` | 22 Windows native staging tests passed, including an actual junction refusal, no-replace publication, pre-existing stage preservation, nested cleanup and malformed/colliding paths. This predates the new local receipt-companion integration. |
| `build/m13-game-results.txt` | 34 Windows game-adapter/planner cases and fault controls passed before the later resource-selection/revision changes. Real wire/session/stage code runs over synthetic files; engine/UI/identity and metadata parser are explicit stubs. |
| `build/m13-resource-results.txt` | 183 Windows tests passed across `test_mcc_share_game.py`, `test_mcc_resources.py`, `test_mcc_validation.py` and `test_mcc_precache.py`, including actual resource selection/cache invalidation and two planner fault controls. This run includes the four-argument installed-snapshot callback and host revision cancellation; it predates final storage/menu integration. |
| `build/m13-campaign-boundary-results.txt` | 69 Windows native catalog tests passed. Two new cases verify that identical campaign metadata and Workshop IDs do not splice separately downloaded mission folders, with either one or two candidates. The real catalog also retains individual mission selection and existing complete-package progression coverage; metadata decoding is stubbed. This establishes the offline catalog boundary, not receipt validation or online follow-on gameplay. |

These runs are overlapping development snapshots, **not additive counts** or
proof that the later combined implementation passes. The resource suite checks
real bounded map/resource readers; the game fixture's metadata decoder/schema
stubs do not validate actual PNG/JSON parsing. Their independent production
helper suites and final combined runs must be recorded separately. Native
fixtures also do not establish Android-device behavior or browser filesystem
support. The current browser publication helper fails closed; M14 must supply
and verify browser import/download, persistence and playable native/browser
sessions for both CE and MCC.

The resource planning correction retains successful validation's exact selected
paths and restamps map/package/root/shared candidates through
`mcc_validation_resources`. It introduces no new selection heuristics and no
Xbox/CE reader changes. Required global dependencies are deliberately refused
with an explicit host diagnostic instead of offering an incomplete package or
redistributing stock resources. The detailed reading route and input limits
are in [the sharing guide](reading/maps_and_resources.md#mcc-package-sharing-between-machines)
and [NETCODE.md](../port/linux/NETCODE.md#mcc-package-sharing).

Only verified task-owned fixture directories were removed after these runs:
0.51 MB for the first staging run, 41.9 MB for the game harness, and 148.1 MB
for the resource run (decimal sizes, rounded). Paths were resolved inside the
worktree and checked for reparse points before cleanup. Small result logs are
retained. These figures cover those component runs only, not the entire M13
workspace or any final cleanup claim. Originals, saves and other agents' output
were outside this cleanup.

Native runtime and platform evidence recorded on 8 November 2026:

- `build/m13-runtime/ruby_allocator_fixed/results.json` records two hidden
  Windows instances, Ruby's Rebalanced Workshop item `2915693904`, mission
  `a10`, co-op, host sharing/client downloads enabled and an initially missing
  client package with Workshop discovery disabled. The executable SHA-256 was
  `3491b03288c018e0487957c4213c6d00bee23d5ead6130dbe626629740ee3ff8`.
  The client accepted the offer, installed the managed snapshot, then joined
  the game in progress at tick 2768. Its final recorded state at tick 9398
  contained both living players. Both processes exited with code 0. This is
  same-machine loopback admission and bounded gameplay evidence, not a complete
  mission, campaign transition, two-physical-machine or internet gameplay test.
  Logs also retain late pregame keep-alive/weapon-deletion diagnostics and a
  lens-flare-capacity warning; an exit code alone does not resolve those.
- `ruby_allocator_fixed/installed-audit.json` under that runtime directory
  records a read-only SHA-256 comparison against the original Workshop source.
  All 15 transferred files match: one map, 12 JSON files (including all ten
  mission records) and two PNGs. The other two files are locally generated
  `download.receipt` and `workshop_id.txt`. The 17-file snapshot totals
  275,925,015 bytes and remains retained for rejoin/removal validation. Original
  Workshop files and all saves were left untouched. The run itself removed
  12 generated map caches totaling 1,535,115,264 bytes, with no cleanup errors.
- `ruby_storage_ui/results.json` records the initial menu inspection using
  that snapshot. The menu incorrectly disabled removal because the main menu
  itself has a running game clock. `cdbe0460b` corrects the gate to permit the
  menu scene while still refusing an active network session; the focused menu
  suite passed 20 tests. The subsequent `ruby_storage_removed/results.json`
  run uses an isolated copy of the 17-file snapshot. Frame 01440 shows the
  confirmation question after the first activation; frame 01920 shows
  "Downloaded snapshot removed" after the second. The managed folder is absent
  and the process exited with code 0. The original retained Ruby snapshot
  remains present. The confirmation question worked, but its button still
  displayed "REMOVE SELECTED": that separate dynamic-label defect is addressed
  in the later correction below. The initial failed inspection remains evidence.
- `ruby_storage_final/storage-audit.json` and its retained frames verify the
  corrected confirmation label and removal on executable SHA-256
  `1b5ca52e683dcb6b544d21bde3da06cc1307fa247230cdb3f67467e3a3f7e3b9`.
  Frame 01440 shows the snapshot still present after the first action and the
  button correctly labelled "CONFIRM REMOVAL". Frame 02400 shows removal after
  the second action, with no managed snapshots remaining and the removal
  button hidden. The process exits with code 0 and all 17 original seed files
  rehash unchanged. This is native menu confirmation/removal evidence, not
  consent-overlay, campaign or active-session removal coverage.
- CI at `17ce5d6e8` passed Windows, Android and web debug builds, Android package
  importer checks, web smoke/rooms in Chromium and Firefox, and MCC HUD render.
  Native WebRTC passed clean delivery at 320/320 in both browsers; the 10% loss
  exercises recorded 711/960 Chromium and 720/960 Firefox. Linux build tests
  had 2677 passes/27 skips and one failure from invalid UTF-8 in NETCODE.md.
  MCC Windows had 2019 passes/17 skips/eight missing-fixture-stub failures;
  MCC Linux had 2083 passes/21 skips/ten failures: those eight, metadata case
  ambiguity and an oversized pytest identifier. These were corrected before
  `cdbe0460b`. The first run is not reported as a green matrix. See
  `build/m13-platform-final/logs/ci-17ce5d6e8-summary.json`.
- Corrected `cdbe0460b3ece923e278ddeda7f3fcf8802f6141` passes
  [Build run 42486938113](https://github.com/chestahh/GulchCE/actions/runs/42486938113)
  and [MCC run 42486938110](https://github.com/chestahh/GulchCE/actions/runs/42486938110):
  Windows, Linux, Android and web debug builds and all three MCC jobs succeed.
  Linux's build selection reports 2,678 passed, 27 skipped and seven subtests;
  its MCC selection reports 2,100 passed/21 skipped. Windows MCC reports
  2,034 passed/17 skipped. Separate Linux preview sanitizer checks pass 38
  cases, and metadata/storage sanitizer checks pass 87 with two skips. Native
  WebRTC clean delivery is 320/320 with each browser; intentional ten-percent
  loss returns 737/960 in Chromium and 700/960 in Firefox. These are overlapping
  job selections and transport controls, not additive coverage or MCC browser
  download/gameplay proof. The retained receipt is
  `build/m13-platform-final/logs/ci-cdbe0460b-summary.json`.
- The Windows build at `cdbe0460b` has executable SHA-256
  `3ef3dca26cd370bf5a32e0c2910946de17cc3a7b1148815b17b2506441988a7b`, recorded
  in `build/m13-platform-final/logs/native-cdbe0460b.json`. This is a different
  binary from the Ruby run above; that older runtime result does not validate
  the complete current commit. The native cases below use this `cdbe0460b`
  binary; none is relabeled as execution of later corrections.
- `mercury_campaign/content-audit.json` and `results.json` record Workshop
  item `3021510694`, `mercury_falling`, both opt-ins enabled and an initially
  empty client. The first download installs at 13:56:07 UTC, resolves its
  gameplay identity and enters co-op with both living players. The owner-verified
  host console executes `(game_won)` after fresh paired gameplay. The next
  offer is visually present by 13:56:44; an explicitly observed Return input
  accepts it between the 13:57:40.010 prompt capture and first staged file
  creation at 13:57:42.567. No exact wall-clock input event was recorded.
  `mercury2` installs at 13:58:16 and resolves by SHA-256. Frame 47040 displays
  its downloaded title and correct thumbnail in the lobby.
  Each snapshot contains ten files including its local receipt. All 18
  receipted content files match their recorded hashes; the 16 transferred files
  also match the original Workshop files, while both generated sidecars contain
  the verified item ID/title. The client staging folder is empty at completion and both
  processes exit with code 0. First-file creation to receipt takes 249.515
  seconds for the first snapshot and 33.492 seconds for the second. The shared
  686,853,355 resource bytes have identical hashes in both snapshots and the
  original; second-resource writes finish in approximately 5.46 seconds.
  This supports local reuse, without claiming a measured network-byte saving.
  The second mission does **not** enter gameplay in this run: the host remains
  in its native campaign lobby/UI pause after installation. Logs retain
  fresh-profile/no-audio diagnostics, authored animation normalization,
  pregame-state messages and a client disconnect when the host exits; they are
  not zero-error logs. No sharing/identity failure, assertion or access violation
  was found. The later fixed follow-on run below supplies separate gameplay
  evidence; it does not change this earlier run's outcome.
- `ruby_version_mismatch/content-audit.json` verifies a metadata-only mismatch:
  the client has identical map bytes in a separate manual package, with
  deliberate whitespace appended only to `campaign/a10.json`. The host's
  version downloads into `dl_dddb5100a348fef34f7b`; all 15 original transferred
  files match their Workshop sources. The older manual folder remains intact,
  including its intentional metadata difference. Client samples span ticks
  2563–4813, with 76 samples; both processes exit with code 0. This demonstrates
  that gameplay identity alone cannot bypass required-JSON admission and that
  the existing package is preserved. It is not a semantic campaign-order-change
  or full-mission test.
- `legacy-control-audit.json` records Xbox Blood Gulch and Custom Edition
  Death Island Slayer controls with both MCC sharing switches enabled.
  Each peer records 39 state samples spanning ticks 30–1170, with no MCC
  transfer and no fatal diagnostic matches. These are bounded legacy-path
  controls, not full matches or independent state-divergence comparisons.
- `5682cbee3` passes
  [Build run 42489773343](https://github.com/chestahh/GulchCE/actions/runs/42489773343)
  and [MCC run 42489773377](https://github.com/chestahh/GulchCE/actions/runs/42489773377).
  Both workflow conclusions were verified on GitHub. A detailed job-count
  summary is not yet recorded; earlier revisions' counts are not reused.
  This validates that committed candidate, not the later admission correction.
- `mercury_follow_on_fixed/gameplay-audit.json` records the completed
  180-second `5682cbee3` Windows pair, executable SHA-256
  `54b697b50a6e78a95d91dbf00ba9906c6eddf6432014fac93cd8e865464a8ca0`.
  The client starts with both previously verified Mercury snapshots, so this
  checks matching-copy admission and progression, not another fresh download.
  Both peers load `mercury_falling` at 14:25:43 UTC. The host invokes
  `(game_won)` after 300 fresh ticks; the only console intervention is this
  deliberate completion. Both resolve `mercury2` by SHA-256 and load it at
  14:26:21 (client) / 14:26:22 (host), through the normal ready-lobby path.
  Each peer records exactly one tick reset: Falling spans ticks 30–330,
  then Mercury II spans ticks 30–3210. The second segment contains 107 samples,
  with both players living naturally from tick 570 through 3210 in 89 samples
  per peer. No cinematic skip or player-state injection is used. The short
  first segment remains in its authored intro and is not a two-living-player
  observation. Frame 14400 shows normal first-person gameplay and the other
  player marker without the sharing overlay. Both processes exit with code 0;
  all 20 retained snapshot files, including receipts, and ten distinct
  original Workshop files retain their audited SHA-256 values. Staging is
  empty. No sharing/identity failure, assertion or access violation was found;
  existing profile/audio/Bink and pregame diagnostics remain. This establishes
  bounded two-mission loading and second-mission co-op, not an independent
  state-divergence comparison or complete campaign playthrough.
- `dedicated_map_only/gameplay-audit.json` records the separate 240-second
  pair on that same executable: a `--server` host with no local player and
  only `mcc_maps/local_ruby/a10.map`, with no JSON, PNG or Workshop sidecar.
  The downloaded map is 275,753,385 bytes, SHA-256
  `71cf81a69ddda9f29b2128629b315a7a5d1aa49968db314817207da264e17c9b`,
  matching the host and original Ruby Workshop file exactly. The client
  installs only that map and a locally generated 393-byte receipt, with empty
  Workshop attribution, into `dl_120b363a0811b190f08c`; staging is empty.
  **Content verification passes, but admission/gameplay fails.** The machine
  joined at 14:28:37 UTC. At installation completion, 14:31:02, the server
  refuses it because no player can join; the client's identity match follows
  at 14:31:03, after removal. There are no client simulation samples and the
  server continues with zero players. The client exits with code 0; the host
  exits with code 1 after the bounded runner kills it at its deadline, not
  through normal shutdown. Source tracing identifies the completion race:
  successful transfer completion clears the downloading exemption before
  identity rebinding/player admission, exposing the original expired
  15-second playerless-machine deadline. A bounded MCC completion-to-admission
  grace correction is tested in the following rerun. This failed run is
  retained and is not reported as successful dedicated gameplay.
- `dedicated_map_only_fixed/gameplay-audit.json` verifies that correction at
  `f8432e2a3528e10d2b1faaa4097d7ac89793128c`, executable SHA-256
  `ac97e6f640fc2b4f4b8c9e6bb2b6fae9bd74231b2ca4c3be2a5d7c727a852889`.
  The same map-only fixture installs and resolves at 14:42:11 UTC, when the
  server adds its player, 128 seconds after machine admission. Both peers
  load by 14:42:37. The client records 81 monotonic samples at ticks 30–2430,
  with its player living throughout; the dedicated host records one player
  in its health log. The original, host and downloaded map hashes match the
  previous audit. Only the map and 393-byte local receipt are installed, with
  empty Workshop attribution and empty staging. Both processes exit with
  code 0: after the client finishes, the runner sends the server its ordinary
  stdin `quit`, and it stops at 14:44:01. Each process removes six generated
  caches totaling 767,557,632 bytes without cleanup errors. This passes bounded
  dedicated admission/gameplay, not a full mission or independent divergence
  comparison; it does not erase the earlier failing evidence.
- `mercury_on_demand_final/gameplay-audit.json` records a **failed** follow-on
  case on that `f8432e2a3` executable, with only the first Mercury snapshot
  seeded. Both peers run Falling at ticks 30–330. After the host's deliberate
  `(game_won)`, it selects Mercury II at 14:45:09 UTC, holds the pending client
  out of loading, then begins its own load at 14:45:10. The client times out
  in `network_connection_idle` at 14:45:25, before host loading completes at
  14:45:29. No second snapshot or client second-mission tick segment appears;
  the host's later ticks are solo, not co-op evidence. All ten seeded files,
  including the receipt, retain their audited hashes. No offer acceptance is
  recorded and no stage remains. Both normal exits do not make this a pass.
- `mercury_initial_loading/gameplay-audit.json` reproduces the same failure
  without campaign progression, on `f8432e2a3`, starting directly with missing
  `mercury2`. Frame 02880 shows the real 749.8 MiB consent prompt; frames 03840
  and 05280 show an accepted transfer at 3%. The host starts loading while
  that client is pending at 14:50:18 UTC and reaches synchronous audio
  conversion at 14:50:22. The client logs its genuine connection timeout at
  14:50:33. The private runner observes that log and stops its owned processes;
  both exit codes of 1 reflect this bounded failed-fixture cleanup, not normal
  shutdown or separate crash evidence. No installation or gameplay is claimed,
  and no incomplete map hash was needed to establish the failure. Staging and
  installed-package directories are empty after disconnect cleanup. The
  timeout comes from the real connection deadline; the runner neither creates
  that diagnostic nor injects a failure. These two reproductions justify an
  MCC-wide host loading barrier for pending consent, transfer and admission,
  rather than a special case for returning campaign players. The generalized
  correction is tested in the following reruns; Xbox/CE loading policy remains
  outside the MCC fix's scope.
- `mercury_on_demand_fixed/gameplay-audit.json` passes on
  `a972f450a1da46c2dcdc15b3c30accd473f86941`, executable SHA-256
  `03322a73077d667a32737d6d5ecd101715c82784a001865b07b85201749ca5e2`.
  Only the ten-file Falling snapshot is seeded. After first-mission ticks
  30–330 and the deliberate host `(game_won)`, the host waits while the client
  considers the second offer. Frame 08160 shows the consent prompt. Results
  record an owner-verified Return posting, but no exact wall-clock input time;
  the observed prompt and new map creation bound acceptance. The new map is
  created at 14:59:52.433684 UTC and its receipt at 15:00:24.732309, an interval
  of 32.298625 seconds. Identity matches at 15:00:27, when the host first
  signals second-mission loading; host/client loads finish at 15:00:46/47.
  Both peers have a fresh second segment of 80 samples at ticks 30–2400,
  including 62 samples with both players living at ticks 570–2400. Frame
  23520 shows normal first-person gameplay and the other player marker without
  the sharing overlay. All 20 snapshot files, including receipts, and ten
  distinct original Workshop files match their audited hashes. The first
  snapshot is unchanged, staging is empty and both processes exit with code 0.
  Shared resource writes cover 686,853,355 bytes in approximately 5.457 seconds;
  their hashes match both snapshots and the original. This supports local
  reuse, without claiming measured network-byte savings. The bounded result
  establishes download, consent, installation and follow-on co-op, not full
  campaign completion or an independent state-divergence comparison.
- `mercury_initial_fixed/gameplay-audit.json` passes on the same `a972f450a`
  executable, starting directly with missing `mercury2`, without campaign
  completion. The first-mission snapshot is seeded for resource reuse: this
  differs from the entirely empty client in the failing initial-load fixture.
  The new map is created at 15:04:02.482597 UTC and its receipt at
  15:04:34.749666, an interval of 32.267069 seconds. Identity resolution and
  the host's first load signal follow at 15:04:37; both peers load at 15:04:57.
  Each records 96 monotonic samples at ticks 30–2880, including 78 samples
  with both players living at ticks 570–2880. All 20 snapshot files/receipts
  and ten original Workshop files match their audited hashes, the seeded
  snapshot is unchanged, and staging is empty. Both processes exit with code
  0; each removes six generated caches totaling 767,557,632 bytes without
  cleanup errors. No timeout, sharing failure, assertion or access violation
  is found. This verifies the barrier during initial admission with resource
  reuse, not another full-resource download, complete mission or two-PC test.
- Final barrier revision `a972f450a` passes
  [Build run 42492469757](https://github.com/chestahh/GulchCE/actions/runs/42492469757)
  and [MCC run 42492469720](https://github.com/chestahh/GulchCE/actions/runs/42492469720),
  recorded in `build/m13-platform-final/logs/ci-a972f450a-summary.json`.
  Windows, Linux, Android and web jobs and all three MCC jobs succeed; the
  release job is skipped. Linux MCC reports 2,145 passed/21 skipped, Windows
  MCC 2,079 passed/17 skipped. Separate preview sanitizers pass 38 cases;
  metadata/storage sanitizers pass 87 with two skips. Linux's main build
  selection reports 2,678 passed/27 skipped and seven subtests. These are
  overlapping selections, not additive totals. Native/browser WebRTC clean
  delivery is 320/320 in Chromium and Firefox; intentional loss delivers
  717/960 and 719/960 respectively. The exact final web artifact is retained
  for the browser runtime checks below. Build and transport success do
  not establish MCC browser download/gameplay, which remains Milestone 14.
- `legacy-final-audit.json` and the individual `xbox_final/gameplay-audit.json`
  and `ce_final/gameplay-audit.json` record the final `a972f450a` native
  controls, using the same executable as the corrected Mercury runs. Both
  MCC opt-ins are enabled. Xbox Blood Gulch Slayer has 44 monotonic samples
  per peer at ticks 30–1320; CE Death Island Slayer has 39 per peer at ticks
  30–1170. All four processes exit with code 0. There are no MCC transfer or
  identity events, connection timeouts, assertions or access violations.
  Each client does log a reliable-endpoint failure, internal networking error
  and host departure when its host ends the timed run; those shutdown
  diagnostics are retained, not described as error-free logs. Each process
  removes six generated caches totaling 767,557,632 bytes without cleanup
  errors. These are bounded legacy-path controls on one PC, not complete
  matches or independent state-divergence comparisons.
- `build/m13-platform-final/logs/internet-a972f450a-summary.json` records all
  three final native/browser cases on the exact `a972f450a` native executable
  and CI web artifact: native host/browser client, browser host/native invite
  join, and browser host/native server-browser join with look input. Their
  run directories under `build/stage11-fix/internet/runs` are respectively
  `native-host-final-a972f450a1da-v3`,
  `browser-host-invite-final-a972f450a1da-v6` and
  `browser-host-browse-look-final-a972f450a1da-v6`. Each records a cached second
  round with 49 samples per peer and 47 paired seconds, WebRTC transport proof,
  no comparator problems or divergences within its recorded tolerances,
  native exit code 0, closed browser contexts and no cleanup errors. These
  are Xbox Blood Gulch networking controls, not MCC browser transfer/play.
  The browser client uses a null renderer; its evidence concerns state and
  transport. Both browser-host cases render through real headless SDL/WebGL
  and retain visual evidence of the observed command/round transition.
  Local broker/loopback testing and tolerant comparison do not establish
  two-physical-PC behavior, exact state equality or full-match compatibility.

The final combined review then found a sidecar-only lifecycle defect:
`mcc_identity_prepare_host` cached Workshop attribution with map/resource
stamps, so a changed `workshop_id.txt` could disagree with the advertised
origin without changing either content digest. The real identity/Workshop
reader regression fails before the correction (`origin-before.log`).
Preparation now refreshes advisory attribution separately, rejects stale
prepared settings/offers and advances the existing revision that cancels active
offers. Gameplay and required-JSON digests remain unchanged, and unchanged
preparation retains its hash/revision fast path. The focused identity/adapter
selection passes 60 checks (`origin-after.log`), with eight unrelated mutation
controls deselected. No new network layout or Xbox/CE implementation is involved.

Two UI follow-ups have 66 focused menu/adapter passes in
`build/m13-ui-final-results.txt`. The generated removal button now has one
dynamic text owner, preventing the renderer's static string binding from
overwriting "CONFIRM REMOVAL". The download progress overlay is suppressed
while consent is pending/visible, resumes after acceptance and clears on
disconnect; a faulty-guard control fails. These counts overlap the identity
and earlier component selections. The later combined local selection records
89 passes in `build/m13-final-identity-ui-results.txt`. The native confirmation
label is verified separately by `ruby_storage_final`, and the later Mercury
runs verify their visible consent/gameplay transitions. The final browser
networking cases and cleanup are recorded separately. Each CI/runtime result
above validates its own recorded candidate.
Browser playability remains Milestone 14 work.

Earlier completed-run cleanup archived 266 BMP captures losslessly into
`build/m13-runtime/completed-screenshots.zip` (106,967,746 bytes). The adjacent
JSON records each original path/size/SHA-256; all ZIP contents were reread and
verified before removing 326,875,164 loose bytes. Four selected menu/lobby/game
captures remain loose. The failed `ruby_native_read` run's verified owned
incomplete stage was removed (15 files, 275,922,109 bytes), retaining its logs
and cleanup inventory. No active Mercury output, saves, installed Ruby snapshot
or original/hardlinked map inputs were removed. These bounded cleanup figures
do not describe the entire worktree's disk use.

The final native/component cleanup receipt is
`build/m13-runtime/cleanup-final-summary.json`, linked to the per-file
`completed-cleanup-4.json`, `completed-cleanup-5.json` and
`component-cleanup-6.json` inventories. It removes 1,308 duplicate map paths
representing 111,549,211,014 logical bytes, reclaiming 4,192,861,352 unique file
bytes after accounting for hardlinks. It losslessly archives 1,404 save/capture
files from 1,114,825,697 to 144,907,310 bytes, verifying CRC and every member's
SHA-256 before removing loose copies. Four released component directories add
1,347 removed files and 134,716,129 bytes, with useful result logs retained
outside them. This pass's total net saving is 5,297,495,868 file bytes; these
figures are not additional to the linked inventories or estimates of allocated
filesystem clusters. Native runtime evidence retains approximately 410.6 MB,
including 333.2 MB of verified archives and 29 selected screenshots totaling
35.6 MB. Original Workshop/main-checkout maps, saved games, useful diagnostics,
the current build and toolchains are preserved. Browser-run cleanup is recorded
separately, rather than counted in these native totals.

The final browser receipt,
`build/m13-platform-final/logs/internet-a972f450a-summary.json`, records complete
cleanup of all three owned runs: 2,554,260,480 bytes of disposable copies and
caches removed, plus 56,932,352 bytes of restored map copies after rehashing
their originals. Closed browser profiles retain their storage and saves in
verified lossless archives: 2,697,247,651 loose bytes become 416,811,089 archive
bytes, with each run's `profile-archive.json` preserving original paths and
hashes. The three run directories retain 558,504,712 bytes of evidence, saves
and archives; the final web ZIP/site and current build remain available.
All owned native processes, browser contexts and helpers are stopped. These
browser totals are separate from the native/component cleanup above.

The final combined review covers the resulting PR adaptation, MCC modules,
shared integration hooks, declarations/build/settings/menu wiring, documentation
and regression fixtures across the milestone's commits. It includes independent
module review and the final documentation-to-code check. Evidence justified
corrections to native file/allocator boundaries, portable metadata identity,
advisory-origin invalidation, confirmation/consent UI ownership, bounded
completion admission and the general MCC loading barrier. The last two fixes
address reproduced lifecycle failures without replacing Xbox/CE loading policy;
their failing runs and successful reruns remain recorded. Required safety checks,
meaningful fault controls and map-family boundaries were retained. No further
unsubstantiated trimming was warranted. Final required checks run against
`a972f450a` after those corrections; this documentation closeout requires only
UTF-8, relative-link and evidence-consistency checks, which pass.

The [PR review and unposted draft response](#pr-95-review-and-draft-contributor-response)
remain available for the user. Completion means the agreed native M13 delivery
and its bounded checks are complete, not proof of every map, full campaigns,
two-physical-PC interoperability or Android-device play. MCC/CE browser
playability, persistence and native/browser gameplay remain Milestone 14;
the MCC-only upstream protocol contribution remains Milestone 15.

### Milestone 14: Custom Edition and MCC browser playability

The user requires both Custom Edition and MCC maps to be playable in web
browsers. This includes loading and playing locally, receiving shared content,
and native/browser multiplayer. Review the necessary general browser/platform
changes separately from MCC compatibility logic; preserve the established
Xbox, CE and MCC ownership and loading boundaries.

The starting web build explicitly disables Custom Edition because its tag
address window conflicts with the C heap. Its existing filesystem is WasmFS
backed by the site's Origin Private File System (OPFS), mounted at `/data`;
this is persistent browser storage, not the ordinary Downloads folder. See
[web startup](../port/web/src/web_main.c) and
[web limitations](../port/web/README.md#limits).
Establish an evidence-backed memory/loading design for each map family before
claiming that filesystem access makes those maps playable.

Provide appropriate browser import and discovery for user-supplied CE maps,
resources and MCC packages/campaign manifests. Implement and verify downloads
through the browser filesystem, with bounded staging, integrity checks,
installation, cancellation and quota/write-failure handling. MCC downloads
remain host-only. Existing native CE sharing includes peer seeding; preserve
its established policy when supporting the browser rather than assuming CE
is host-only. Keep `maps`, `custom_maps` and `mcc_maps` separate.

Completion requires actual CE and MCC browser gameplay and visual/audio
checks, not only compiling WebAssembly or saving downloaded bytes. Test
native-host/browser-client and browser-host/native-client roles, browser peers,
campaign follow-on missions, save/reload, browser reopening and retained
content, failure cleanup and map-family transitions. Include representative
Chromium and Firefox runs and native Xbox/CE/MCC regression controls. Record
tested maps, browser versions, resource limits and unsupported cases precisely.
Run the required platform and networking checks after the combined review.

Milestone 14 is **in progress**. The initial browser memory candidate keeps
the existing 2,176 MiB linear memory and Xbox window, ends the ordinary heap
at CE's linked tag address (`0x40440000`), and gives MCC one validated 64 MiB
window in its existing accepted address range. CE initialization follows the
page's arguments and OPFS/configuration setup. MCC reserve clears reused
storage; invalid releases cannot relinquish another allocation. Independent
review found no blocking ownership or initialization defect. Native allocation
branches and the established CE loading/sharing implementation remain intact.

On the working implementation after `b26f35fd0`, the isolated Windows-hosted
Emscripten 6.0.10 build completes, including strict Wasm linking. Portable
Python staging replaces shell-specific SDL/bootstrap copy commands; the SDL
source remains pinned to 3.4.16 with its existing checksum. Allocation exhaustion
returns failure (`ABORTING_MALLOC=0`) so loader cleanup can run. Existing native
configure arguments remain `--portable --release --pgo=off --lto=off`.
Ninja subsequently regenerated `build.ninja` for the changed build scripts;
the graph's bytes changed without changing those options.

The real production memory fixture passes in Node and in headless Chromium
151.0.7922.34 and Firefox 153.0. Each browser refuses a 16 MiB memory, then
preserves CE/Xbox boundary bytes while four worker threads exhaust the ordinary
heap with 32 allocated 32 MiB blocks. The fixture also checks invalid MCC
addresses/sizes, exclusive ownership, wrong releases, alternate bases and
zeroed reuse. All four browser cases exit zero without page errors or stderr;
the receipt is `build/m14-web/memory-browser-results.json`. Web CI now runs
both the Node fixture and the two-browser fixture. Build/CI preflight reports
93 passed and one explicit Windows POSIX-socket skip; this overlaps earlier
component checks and is not an additional unique-test count.

These are memory-layout checks, not CE/MCC gameplay or resource-headroom
evidence. Large converted resources, browser graphics/audio storage, imports,
download persistence and actual map-family transitions still require their
runtime gates. Owned browser contexts, loopback servers and fixture outputs
are closed/removed; the SDK, browser toolchains and current Web build remain
for the continuing milestone.

The first bounded CE browser gameplay control passes in Edge 157.0.4341.42:
`custom_maps\deathisland`, Slayer, one browser host with its real WebGL renderer
and audio enabled. The 100-second probe records 87 player-state lines and 99
frame samples advancing from 6 to 2,831, with no page errors or fatal result.
The retained third screenshot independently shows the rendered environment,
weapon and HUD. The final thirty audio samples report a running 48 kHz context
and nonzero signal (maximum peak 0.00300729, RMS 0.00099579); this establishes
audio activity, not an exhaustive listening/quality test. All seven original
Xbox/CE map and resource inputs retain their recorded hashes.

Evidence is `build/m14-runtime/ce_memory_probe_5/{result.json,audit.json,
browser.log,frame-3.png}`. Its closed profile remains available for a later
persistence check; this run itself does not establish reopen/save restoration,
host downloads, multiplayer agreement, MCC gameplay or measured heap headroom.
The earlier four probes retain their actual failed/incomplete outcomes: two
fixture setup failures, an observer's invalid non-exported `HEAP32` access,
and a menu-only run missing a stock carousel fixture. They are not CE gameplay
passes. The two started but inactive profiles are preserved as verified ZIPs
before their loose copies are removed; no original maps or player saves are
discarded. Milestone 14 remains in progress.

The initial Mercury 2 solo browser probe (`build/m14-runtime/mercury_solo_memory`)
establishes conversion/loading and simulation progress only: 30 player-state
lines through tick 901 in 180 seconds, with unchanged original Workshop inputs.
Its near-blank scene and repeated `mcc textures: upload failed, GL error 500`
are a visual failure, despite the probe's simulation-only success flag. The
MCC-owned uploader was issuing texture-swizzle parameters that
[WebGL 2 removes](https://registry.khronos.org/webgl/specs/latest/2.0/).
Its browser path now puts decoded BGRA bytes into RGBA order in the temporary
upload buffer and omits those parameters; compressed 3D or unaligned base
dimensions use the existing decoder under the
[WebGL S3TC restrictions](https://registry.khronos.org/webgl/extensions/WEBGL_compressed_texture_s3tc/).
The legacy uploader is unchanged. Actual-function boundary regressions failed
all nine initial Web cases before the correction, with 29 existing/native cases
passing; all 44 expanded upload/bridge checks then pass, including cube faces,
mips, volumes, direct compressed upload, decoded fallback, channel values and
immutable source pixels. Together with bitmap flags and HUD meter checks, the
focused run passes 89 tests. These tests use a recording GPU boundary. A separate
asset-free Edge 157.0.4341.42 WebGL probe confirms the extension's shifted-level
rule: a 12-pixel base with 6/3/1-pixel mips uploads, while 1/2/6-pixel level-zero
bases are refused (`build/m14-web/s3tc-browser.json`). Its disposable browser
context is closed.

The corrected Mercury 2 visual rerun reopens the same retained profile and
OPFS origin in Edge 157.0.4341.42 (`build/m14-runtime/mercury_visual_reopen`).
Over 180 seconds it records 106 player-state lines, frames reaching 4,340,
nonzero audio activity, no page errors and no texture-upload failures. The
primary's inspection of `frame-3.png` confirms the scene, vehicle, textures
and HUD render. The browser closes normally; this is one solo map/reopen
control, not campaign completion or multiplayer evidence. It reuses the
previous isolated OPFS profile at its original origin after explicitly
refreshing the test's code cache; this verifies retained imported content,
not the service-worker update UI or checkpoint restoration.

A separate pregame probe (`build/m14-runtime/mercury_memory_probe`) never
reached MCC loading: validation left a temporary stock lobby map selected,
and an expired countdown repeatedly took the deferred-start branch instead
of sending its ordinary heartbeat. The MCC-owned update now keeps an active,
unpaused, nearly expired countdown alive for the full existing `map_pending`
condition, including deferred selection/validation and downloads. It does not
start stopped timers, shorten longer timers, or alter legacy-only sessions.
Regressions exercise the real countdown/start/heartbeat branch for forty
seconds: four relevant cases failed at the existing fifteen-second silence
limit before the correction; the resulting UI/network suite passes 81 tests.
Cancellation, failure, changed revisions and inactive/session transitions are
covered. The combined pregame rerun reaches 43 player-state lines through tick
1,454, confirming that the heartbeat correction gets beyond validation into
the game. Its still-broken textures predate the upload correction and are not
counted as a visual pass.

The combined implementation subsequently passes 2,559 unique component checks
with 19 explicit skips, plus 70 Node browser-site checks. The initial broad run
reported four Windows junction-creation permission failures and one CI-wiring
failure; the targeted elevated filesystem rerun and corrected workflow resolve
all five. These totals merge per-case XML results, rather than adding overlapping
runs. Actual read-only Mercury header/geometry and available CE resource checks
are included. Missing JDK/local optional fixtures and platform-specific cases
remain explicit skips; CI must supply the relevant platform coverage.

The final browser transaction component matrix passes 35 Chromium/Firefox
WasmFS/OPFS cases. Independent review found and corrected two additional
boundaries: a transferred Workshop sidecar must not also be journaled as a
generated companion, and asynchronous page requests need stable argument
storage plus generation checks before decoding as well as completion. The
tests reproduce rehosting and proxy execution delayed beyond the C deadline.
These are component results, not network download gameplay results.

A Firefox 153.0 Ruby's Rebalanced `a10` probe records 96 player-state lines
and frames advancing from 5 to 3,871 in 180 seconds, with nonzero audio activity
and no page or texture-upload errors. The reviewed screenshot shows Captain
Keyes in the opening sequence. This establishes loading, rendered cinematic
output and simulation, not complete controllable gameplay or campaign progress.
Evidence is `build/m14-runtime/ruby_firefox_solo`. Separate hidden native
controls load Ruby `a10` and Mercury 2, reach their 150-tick markers and exit
zero (`native_mcc_controls/results.json`). These use read-only original
Workshop inputs and isolated saves.

The initial implementation and test portability follow-up are committed as
`397aceefe` and `55fc7df9c`. On the latter, the
[all-platform build](https://github.com/chestahh/GulchCE/actions/runs/42500292426)
passes Windows, Linux, Android and Web, and the
[MCC validation workflow](https://github.com/chestahh/GulchCE/actions/runs/42500292444)
passes all three jobs. The first Web CI attempt exposed a standalone fixture
using a Windows pthread stub on Linux; the follow-up uses the real POSIX
header there. This CI result predates the admission correction below.

The first complete large host-to-browser download exposes a separate MCC
admission defect. `build/m14-runtime/mcc_native_host_final` receives Mercury 2
and its package resources/metadata: ten files, 786,191,316 bytes. Every payload
hash matches its source, the committed snapshot survives complete browser
reopening, and all original inputs remain unchanged. Nevertheless, it **fails
gameplay admission**. The staged content probe blocks browser game updates
for about 26 seconds; the host's 20-second last-heard bound expires during it.
An additional installed-identity probe then blocks for about 25 seconds,
independently exceeding the 15-second post-download admission grace. The host
starts alone. The retained `admission-audit.json` correlates both logs and
does not count publication/persistence as a multiplayer pass.

The same synchronous identity probe runs on ordinary installed-package joins,
including when sharing is disabled. The correction therefore belongs to the
MCC compatibility engine's validation/admission lifecycle. Independent owned
checks now keep network updates running and publish SHA/CRC/resource-selection
facts only while the complete candidate snapshot remains unchanged. Actual
loading still fingerprints its opened content independently. Installed-file
checks use a bounded validation hold independent of download permission;
download verification retains the existing transfer lifetime. A successfully
published snapshot is bound locally before successful `DONE`.

The new reliable admission contract requires base agreement 32730, MCC 70 and
camera agreements 32732/32733. Independent review corrected a stale-publication
retry loop and a backpressure gap between validation and a download offer.
The first cold installed-package run with both sharing settings off then found
an outer integration omission: the server's pregame dispatcher dropped message
105 before it reached the tested adapter. A minimal joined-machine stream
route now handles that MCC message before the ordinary ingame/loaded gate.
A regression compiles the real stream and datagram dispatchers; removing this
hook reproduces the missing acknowledgement/hold. Unjoined, unmapped,
incapable and datagram senders remain refused, and ordinary gameplay data
retains its loaded-state gate. Client ingress already has the required route.

This cold-join run also repeated native exit `0xC0000005` after the test's
owner-checked close request. It remains under separate investigation and is
not a clean-exit result. Steam moved the original Workshop packages from `C:`
to `E:\SteamLibrary` during the run. A relocation audit verifies 39 recorded
inputs (1,238,151,561 bytes) against their original hashes, including 36 moved
Workshop files and three unchanged Xbox inputs. Historical path-check failure
is preserved; future private configurations use the verified new paths.
Runtime completion gates remain open.

After combined primary and independent review, the corrected Windows release
and isolated Web builds pass. The resulting admission/sharing/resource,
protocol, storage, UI and build regression run reports **607 passed, 3 skipped**
(`build/m14-ingress-combined.xml`). Skips are Linux sanitizer coverage, an
unavailable Windows file-symlink privilege and a POSIX-only socket harness.
All 204 relative links in the changed documents resolve. The preceding run's
81 UI fixture errors were a missing newly required real header; the fixture
now includes it. Two safely rolled-back storage fixture installations failed
transiently with no established cause. Full and long-path reruns plus 100
bounded repeat installations pass; added failure diagnostics retain native
error codes without retrying operations or weakening assertions. Subsequent
all-platform CI and real multiplayer results are separate completion gates.

The CE native-host/browser-client download control passes on the initial
browser candidate: four files totaling 183,811,669 bytes, including resources,
match their original hashes and survive complete browser reopening. The
download receipt remains present. A cached second round yields 47 paired
seconds, zero tick offset and no comparison problems or divergences, with
1,074 drift samples and none apart. Its native process exits zero. The earlier
late-join run remains failed overall because its diagnostic sample phases
were not aligned; thresholds were not relaxed. Evidence is
`build/m14-runtime/ce_native_host_download_cached/result.json`.

#### Admission fixture portability and bounded conversion progress

The subsequent `cdba` CI build compiles all four platforms, but its Linux
reader job reports nine failures (2,222 passed, 21 skipped). One is a standalone
fixture ABI error: Linux's 64-bit `long` enlarged the copied distributed header
and padded its test envelope, so the real adapter correctly refused the packet
size. The fixture now uses the production header's fixed 32-bit fields and
asserts its 12-byte header and 216-byte envelope. A private wide-field negative
control reproduces the exact missing-ACK assertion before this correction and
passes afterward (`build/m14-lp64-fixture-proof.json`). The other eight failures
come from checkpoint fixtures missing declarations/definitions for newly
registered MCC handlers; those independent handlers now have explicit fixture
stubs while the real checkpoint dispatcher and its assertions remain intact.

The focused identity, admission, real outer ingress, checkpoint and agreement
run then passes **52 tests** (`build/m14-load-contract-final.log`). Its ingress
fixture also checks message 106 on the real server dispatcher: joined reliable
pregame traffic reaches its handler; unjoined and datagram traffic do not, and
ordinary data retains the ingame/loaded gate. These are overlapping component
checks, not an additional unique total or a completed rerun of CI.

The conversion-progress contract uses base **32734**, MCC **71**, and camera
**32736/32737**, with message 106 independently bound to the admitted identity
and game round. Review checked the five-second send/receive advancement limit,
thirty-second host progress-idle bound, four-minute absolute bound, terminal
peer retirement and map/session cleanup. The client's established 75-second
pre-start transport-silence bound remains separate. Required shared additions
only route authenticated MCC messages before the ordinary loaded-player gate
and connect MCC-owned state to start/completion/timeout events. Headless
conversion has the same network lifecycle; no pulse runs frame, widget,
simulation or network-dispatch callbacks against incomplete tags. See
[the protocol contract](../port/linux/NETCODE.md#mcc-package-sharing).
Final platform and actual network/gameplay validation of this revision remain
pending; these source and component findings do not complete Milestone 14.

The actual installed-package rerun on `cdba8dc1a`
(`build/m14-runtime/mcc_installed_optouts_off_fixed`) admits the browser with
both sharing/download opt-ins disabled and no new download. Identity resolves
about 85 seconds after joining. Both machines then load Mercury 2; the native
host finishes first and removes the browser exactly sixty seconds later, while
the browser is still converting valid content. The browser subsequently finishes
conversion but cannot send its loaded acknowledgement to the disconnected host.
This is an admission pass and an overall multiplayer failure, not synchronized
gameplay evidence. The loading-progress extension above addresses this distinct
failure; its actual paired rerun is still required.

The same run's retained debugger trace establishes a separate shared shutdown
defect. A detached `p2p_thread` calls `platform_log` through lobby withdrawal
while the main thread is already in CRT exit; the fault occurs inside the
runtime's stream lock. The MCC window destructor on the main thread's stack
does not establish an MCC allocation defect. The focused platform correction
owns both P2P workers through existing thread handles, requests stop under the
service lock, waits outside that lock, and withdraws the listing and router
mapping after worker completion. UPnP cancellation prevents subsequent owned
router requests; a third-party HTTP operation already running still has to
return. This is not a guarantee of bounded shutdown against a stalled router.
Actual-source tests and independent ownership review cover this correction.
See `shutdown-debugger.log` and
`admission-loading-shutdown-audit.json` in that run's evidence directory.

The final focused shutdown suite passes **20 tests**, including actual OS
threads stopped during polling and UPnP completion, renewal-handle reaping,
ordinary work before exit, allocation/exit-hook failures and retry after a
failed start. Three isolated mutations removing either worker wait or the
post-select stop check fail their regressions. Router tests exercise the real
owned wrapper functions with cancellation during discovery, mapping scans,
address lookup, conflict retry and permanent-lease fallback. A mapping created
just as cancellation arrives remains owned and is removed. The actual static
P2P initializer is included in the fixture: native compilation caught its old
positional fields shifting when handles were added, so socket initialization
now names its fields. The normal Android generator emits the cancellation
import; the browser has a matching no-op. Standard Linux and Windows CI now
invoke `tools/test_p2p_shutdown.py`.

The resulting native build passes. Three sequential, isolated public-host
controls then load the selected map, simulate and use the same owner-checked
`WM_CLOSE` procedure as the failing run. Xbox Blood Gulch and CE Death Island
each record ticks 30–840; MCC Mercury 2 records ticks 30–211. All three withdraw
their listing and exit **0**, with original inputs and the executable hash
unchanged. These are single-host exit controls, not paired-network validation,
and UPnP is disabled during them. The private launcher initially counted an
absent standalone tick log; `native_shutdown_fixed/shutdown-audit.json` counts
the actual `native.log` records and explicitly corrects that field. Evidence is
under `build/m14-runtime/native_shutdown_fixed`; the tested executable SHA-256
is `a0625660e6880e0b839d13b94e83d92f5fc490b257543f93777a5ff0ed426b51`.
Final platform and paired native/browser checks remain required.

The next installed-package pair, `mcc_installed_profile_loading`, uses the
normal `mccprobe` browser profile with both download opt-ins off. It has no
missing-`lastprof.txt` errors. Timestamped logs measure 59 seconds for the
identity resource check and 89 seconds for conversion; the native host waits
69 seconds after becoming ready, beyond its old sixty-second loading timeout,
and both machines then enter the mission. The browser's own player becomes
live at tick 579 and remains live through tick 1479 (31 samples). Reviewed
`client-07.png` shows its first-person weapon, HUD and mission geometry. The
native host withdraws its listing and exits zero. The installed snapshot is
reused without a download, originals remain unchanged, and all 42 exported
save/profile files (41,980,396 bytes) retain their hashes after browser reopening.

This pair still **fails overall**: the unchanged strict judge finds zero paired
state samples because host logs occur at ticks 30/60/... and browser logs at
39/69/..., outside its two-tick pairing window. The source reporting cadence
stores the first actual report tick and subsequently adds thirty, retaining
that initial phase difference. No timestamps, states or judge tolerances are
adjusted. The separate drift counters (652 samples, five apart) do not replace
the missing state-field comparison. Loading also reports an incomplete
framebuffer and prior GL errors 506/502, retained as a separate graphics issue.
`installed-profile-audit.json` distinguishes these failures from admission,
loading, save persistence and clean-exit successes. The next paired run needs
real aligned observations and the graphics correction; this is not final
multiplayer compatibility evidence.

Cleanup removed the controls' eighteen generated caches (2,302,672,896 bytes)
after each process exited, preserving all saves. The two closed installed-join
failures lost only their twelve generated caches (1,535,115,264 bytes); their
verified screenshot archives save a further 75,781,164 bytes while preserving
every capture and one loose native frame per run. Logs, debugger evidence,
original maps, active browser storage and future fixture seeds remain.

Steam relocated the original MCC installation and Workshop packages from
`C:\Steam` to `E:\SteamLibrary` during these tests. A separate relocation audit
verifies all 39 recorded source files (1,238,151,561 bytes) against their original
hashes. Future fixtures use the new source paths; historical receipts remain
unchanged. This relocation was not test cleanup. The first installed-package
run's premature `sourcesUnchanged` flag is superseded by the explicit audit,
not accepted as proof after its old-path lookup failed.

After the combined source/integration review, the local loading/shutdown
regression set passes **663 unique tests**, with three explicit skips: Linux
sanitizers, Windows file-symlink privilege and the POSIX socket harness.
The initial sandbox run could not create three directory junction fixtures;
their separately authorized reruns pass without weakening assertions. Merged
per-case receipts are in `build/m14-loading-shutdown-summary.json`; overlapping
component batches are not added to this total. The wire-format check and 262
relative documentation links also pass.

Browser checkpoint testing now uses an actual `mccprobe` player profile selected
through Profiles and Campaign. In `mcc_lifecycle_5`, Mercury 2 saves at tick
1,081, reverts from tick 1,441 to 1,126 and resumes simulation. The observed
Save and Quit action reaches the main menu. That helper nevertheless fails
because it waits for a network precaching message absent from this menu path;
the failed receipt is retained. `mcc_lifecycle_6` explicitly continues from that
closed run: both transient and profile-owned 20 MiB save files retain identical
hashes and timestamps across browser closure/reopening. Normal Campaign Continue
restores ticks 1,126 through 1,246, rather than starting a fresh map. In the same
browser worker, CE Death Island then advances from ticks 31 to 182 and Xbox
Blood Gulch from 30 to 180, with matching load markers and inspected rendering.

The final Xbox-to-MCC return in that sequence is a **failure**: Mercury converts,
loads and restores tick 1,126, then reports a texture-upload GL error `506`,
preceded by a prior GL error `502`. It does not establish a successful final
gameplay/visual transition. The browser is closed, all original CE inputs are
unchanged, and saves, screenshots and the independent partial-evidence audit
remain under `build/m14-runtime/mcc_lifecycle_6`. The graphics cause and corrected
transition still require validation; checkpoint persistence alone does not
complete Milestone 14.

The MCC standalone loading controller inherited the preceding scene's color
and depth attachments while drawing into a backbuffer-sized viewport. The
existing loading effect then paired that inherited depth surface with its
back/front buffers; the first resume log records an incomplete framebuffer
immediately after MCC loading begins. The MCC controller now selects the
backbuffer without a depth attachment before each draw. Its temporary surface
reference is released, and the next ordinary scene installs its own matching
targets. The Xbox/CE loading effect and shared renderer remain unchanged.
An actual-controller regression fails on stale attachments before the change;
the resulting loading/network/texture checks pass 72 tests, including disabled
and headless paths, repeated draws and reference cleanup.

`mcc_lifecycle_7` reaches the restored MCC checkpoint, CE and cached Xbox
gameplay without the earlier framebuffer/upload diagnostics, but remains a
failed fixture: its observer required a cold Xbox precaching message that a
cache hit does not emit. The next observer accepts the exact current-map
diagnostic from checkpoint validation as alternate identity evidence. It still
requires fresh, increasing living-player samples after that identity, rejects
duplicate epochs, and retains the original failed receipt.

The corrected `mcc_lifecycle_8` passes on the graphics candidate WebAssembly
SHA-256 `0a697221789f95db573082acf1f87d9603b93638eff18eee848ff334a5634cf5`.
Both 20 MiB save exports are independently rehashed before and after complete
browser closure/reopening; their bytes and timestamps match. Normal Profiles
selection and Campaign Continue restore Mercury ticks 1,126–1,246 from the
prior saved state. The same browser worker then runs CE Death Island at ticks
30–181, cached Xbox Blood Gulch at 30–180, and returns to Mercury at 1,126–1,276
after accepting its saved checkpoint. Independent screenshot review confirms
the CE/Xbox weapon, HUD and environment and Mercury's Warthog, vehicle HUD and
textured scene. All four original CE inputs retain their hashes; the browser
closes normally. Evidence, including exact phase markers and save hashes, is
`build/m14-runtime/mcc_lifecycle_8/independent-audit.json`.

This run has no upload error `506` or incomplete-framebuffer diagnostic, but
the final return still reports **prior GL error `502`** before an MCC upload.
Its origin remains under investigation; the run is not described as entirely
GL-clean. This is one Edge solo checkpoint/lifecycle control, not Firefox,
multiplayer or complete-campaign evidence. It uses the graphics candidate
before the subsequent combined sampler/resource-optimization rebuild.

The combined current component suite passes **885 tests with eight skips**
(`build/m14-candidate-combined.log` and `.xml`). Skips cover Linux sanitizer
coverage, unavailable Windows file-symlink privilege, a POSIX socket harness
and five POSIX/Linux launcher/process-lifecycle checks. This overlaps earlier
batches and is not added to them. Independent review checks the final graphics
and Web-only reader changes, ownership, bounds and documentation; combined
platform builds and real paired gameplay remain pending.

#### Browser identity-check read cost

A private synthetic benchmark identifies OPFS read overhead in the incremental
MCC checker. Emscripten 6.0.10 implements each read-only OPFS Blob read through a
worker proxy and `Blob.slice(...).arrayBuffer()`. The stdio refill can issue two
backend reads. This is source evidence for an I/O cost, not a reason to omit
either CRC or SHA verification or extend admission deadlines.

The controlled experiment compiles the actual resource checker, SHA and CRC
implementations, changing only its read-buffer size and adding scoped counters.
Two generated 32 MiB resources and a 5,632-byte generated MCC map have an
independent Python CRC/SHA oracle. No retail assets are used. Each candidate
runs on a 256 KiB fixture-worker stack. The production validation worker requests
that size, but the platform thread wrapper enforces a 1 MiB minimum. The fixture
retains the production 2 ms and 4 MiB thresholds checked between reads; a final
read can cross the byte threshold by less than one chunk, as with the former
16 KiB reader. Three rotated OPFS trials per size and browser, MEMFS
controls, cancellation and injected read failures run serially in isolated
profiles. All other task builds, games and heavy I/O are idle during the
authoritative run, 2026-11-08 18:30:46–18:31:52 UTC.

| Browser | 16 KiB median | 32 KiB median | 64 KiB median |
| --- | ---: | ---: | ---: |
| Edge 157.0.4341.42 | 4.730 s | 2.314 s | 1.394 s |
| Firefox 153.0 | 3.714 s | 2.736 s | 2.254 s |

Actual backend calls fall from 8,194 to 4,098 to 2,050. Edge's measured read
time falls from 4.479 s to 1.156 s while SHA remains about 0.222 s and CRC about
0.016 s. Firefox's read time falls from 1.940 s to 0.604 s; SHA remains its
larger remaining cost. MEMFS controls show no meaningful improvement. All
24 complete checks match the independent identity; all six cancellations and
six read failures leave identity outputs unpublished. At 64 KiB, observed
cancellation takes 2.85 ms in Edge and 1.24 ms in Firefox. The time budget is
checked between reads, so an individual blocking read can exceed 2 ms.

Accordingly, only `HALO_WEB` uses a 64 KiB buffer in
[`mcc_resource_check.inl`](../port/linux/game/mcc_resource_check.inl); native
builds retain 16 KiB. Byte framing, checksum algorithms, slice limits,
cancellation ownership and all network deadlines remain unchanged. The real
checker regression runs both native and browser reader branches against
independent CRC/SHA values around 64 KiB and 128 KiB boundaries, odd tails,
cancel/restart and failed-read recovery.

Evidence is retained under `build/m14-check-bench`: `clock-fixed/results.json`,
`clock-fixed/summary.json`, source/artifact receipts and `report.txt`. Earlier
runs are explicitly superseded: the private fixture's direct floating-point
clock cast saturated on an epoch-sized worker timer, exercising only the byte
limit. The corrected fixture wraps through a 64-bit integer to match the
production 32-bit clock. Production clock code is unchanged. The final timing
run also occurs after a separate cleanup ended at 18:26:07 UTC, removing that
earlier contention concern. All six closed synthetic profiles are removed
(545,122,024 bytes); logs and fixtures remain reproducible from source.

These are first-open/reopened reads after fixture creation, not guaranteed
cold-cache measurements or full-game performance. Actual installed-package and
post-download verification still require runtime validation with the combined
browser build; no timing guarantee or completed multiplayer gate follows from
this optimisation.

### Milestone 15: MCC-only upstream contribution

After Milestones 13 and 14, create `GET-YOUR-MCC-HERE` against the then-current
OpenCE `main`. The completed `mcc-code-refactor` branch is the source of the
contribution, including its MCC documentation and regression tests. Preserve
provenance and exclude unrelated fork features, settings, assets and integration
such as optional shoulder-camera mode. Preserve authored MCC camera semantics
required by the maps. Include shared fixes and browser foundations needed for
MCC correctness; distinguish any CE-only browser work as a separate upstream
dependency or contribution rather than silently expanding the MCC-only scope.

Integrate MCC messages and admission into OpenCE's network agreement, removing
the separate MCC protocol/version layer while preserving map-family validation
and MCC-owned semantics. Audit message layouts, dispatch, identifiers,
simulation changes and old-peer refusal; changing a version constant alone is
insufficient. Use the then-current upstream network version plus one for the
combined agreement and account for any upstream movement before integration.
Revalidate the resulting branch across the required platforms and map families.
Keep reviewable commits and all meaningful tests/documentation; the complete
branch does not require presenting it as one oversized upstream PR.

### Milestone 6: checksum consolidation

Consolidated repeated CRC32 calculation and table construction in
[`mcc_checkpoint.c`](../port/linux/game/mcc_checkpoint.c),
[`mcc_script_threads.inl`](../port/linux/game/mcc_script_threads.inl),
[`mcc_resources.c`](../port/linux/game/mcc_resources.c) and
[`mcc_resource_check.inl`](../port/linux/game/mcc_resource_check.inl).
The existing `crc_checksum_buffer` in [`crc.c`](../source/memory/crc.c) was
evaluated; the already-linked port-prefixed zlib service was selected after
checking initialization, integer widths and standalone-tool linkage.

Old/new and independent-oracle checks establish unchanged seeds, complements,
empty and chunked results, resource ordering/framing, checkpoint bytes and
failure behavior. Required platforms and consumers pass. The saved per-check
storage and isolated checksum timing are measured, with their practical limits
recorded in [Stage 6 evidence](#stage-6-checksum-consolidation).

### Milestone 7: scripts and state lifecycle

Review script resolution/execution, campaign and checkpoint restoration,
script-thread and named-object ownership, and failed-load/unload cleanup.
The relevant routes include `mcc_scripts`, `mcc_script_runtime.inl`,
`mcc_script_threads.inl`, `mcc_checkpoint`, `mcc_campaign` and
`mcc_object_names`. These are investigation areas, not established new defects
or promised deletions. Keep host/client authority and saved-state contracts
explicit when evaluating a change.
Stage 6 also retained a baseline assertion when a checkpoint captured a
sleeping temporary console expression. Investigate that expression's syntax
lifetime separately from CRC validity; the recorded standalone-save controls
pass. Stage 7 reproduced this on Xbox, CE and MCC and isolated a shared
syntax-lifetime defect. Its correction was deferred pending a shared
save/ownership design, then explicitly reopened alongside Milestone 11;
see [Stage 7 evidence](#stage-7-scripts-and-state-lifecycle).

Complete when reviewed paths and ownership rules are recorded, demonstrated
defects/redundancy are addressed within scope, and affected save/revert,
restart, failure and unload behavior has been checked. Retain supported code
when no practical benefit from changing it is established. Any unresolved
semantic or compatibility question must have an explicit disposition and the
evidence needed to revisit it.

### Milestone 8: compatibility exceptions and known gaps

Review the 21 retained HUD structural profiles and other content selectors
encountered in the affected MCC paths. The immediate HUD question is whether
validated authored information or usage context can reliably distinguish
native and MCC safe margins. A replacement needs intended matches, near
misses and actual stock/custom HUD checks, including Ruby and Descent.
Record retained exceptions' rationale, activation/cleanup, regression evidence
and removal conditions; do not remove a profile merely to shorten the table.

Separately diagnose Quarantine's `life_lost_0` refusal at `ai_deselect`.
Establish the operation's intended semantics and implementation boundary
before enabling it. A systemic MCC fix must benefit any map using those
semantics; accepting an unproven no-op or a map-specific bypass is insufficient.
Complete this milestone with validated fixes where justified and explicit
retention/deferral decisions where evidence or supported scope is insufficient.
An investigation completed with a deferral does not mean the defect is fixed.

### Milestone 9: combined review and extended validation

Review the resulting implementation across all stages, including code, tests,
integration and reader documentation. Reconcile the MCC boundary inventory in
`CODINGSTANDARDS.md` with the work actually reviewed: identify changed,
reviewed-and-retained, and unaudited/deferred areas. Further code changes need
their own demonstrated benefit and must receive affected regression checks.

Extend representative Workshop checks with save/revert/restart, two-instance
co-op, repeated load/unload and failed-load recovery across map families.
Use Ruby's Rebalanced and other maps selected for the affected behavior, with
isolated saves and at most two hidden game instances. Compare loading cost
and memory retention under matched conditions; distinguish intended bounded
storage costs from leaks and do not infer speed gains from fewer lines.

Complete when the final combined state has passed the applicable platform,
network and runtime checks, evidence and remaining limits are recorded, and
temporary outputs are cleaned. Full campaigns, live split-screen, Android
devices and browser-map gameplay remain unverified until specifically tested;
build success alone does not close those limits. This milestone prepares the
branch for delivery review; PR descriptions and merging remain separate.

The combined review and planned runtime observations have completed. Platform
and representative runtime checks pass, but Ruby co-op recovery does not meet
the all-passing compatibility criterion above. It also fails on original main;
repairing the shared recovery path required a separate contract/version review
under the then-standing OpenCE-crossplay constraint. This exception is recorded,
not silently accepted or converted into a passing check. Final documentation
and cleanup are complete. Findings and coverage are recorded in
[Stage 9 evidence](#stage-9-combined-review-and-extended-validation).

### Milestone 10: shared co-op checkpoint recovery

Follow up the [Stage 9 recovery finding](#saves-restarts-and-co-op-findings):
after a team-wipe revert, Ruby B40's host has a living player unit while its
connected client lacks the corresponding player-unit binding. The failure
reproduces on original main and current in the OpenCE Crossplay profile.
The [Stage 10 investigation](#stage-10-shared-checkpoint-recovery-investigation)
now confirms that the client retains the same full handle as a dead object
while the host has restored its living version. This is shared-engine work arising from MCC validation;
it is separate from the explicitly deferred console-syntax lifetime defect.

1. **Establish the cause.** Trace checkpoint restoration, authoritative object
   descriptions, client object replacement and player attachment in
   `network_coop`, `network_objects` and `network_distributed`. Capture object
   identifiers, lifetime, inventory and player ownership across the failure.
   Add a minimal regression exercising the real implementation and retain
   Ruby as the runtime reproduction. Compare original and current behavior
   without weakening the strict judge or discarding failed observations.
2. **Establish the recovery agreement.** Evaluate the recovery extensions behind
   the GulchCE Third Person session profile as evidence, not a proven remedy.
   Correct the responsible shared engine contract where demonstrated; preserve
   Xbox/CE/MCC ownership and host authority. Validate object
   descriptions before replacing client state. Correct supported behavior
   must not require enabling a cosmetic feature. Review messages, simulation
   agreement and required version separation before implementation. The user's
   7 November policy change permits excluding old peers; enforce that boundary
   at discovery and admission. Do not add a map-specific bypass.
3. **Validate the resulting behavior.** Demonstrate failure before and recovery
   after the fix, including actual player reattachment and restored inventory,
   not only a successful load. Cover repeated team wipes and checkpoint
   reverts, ordinary co-op, join/rejoin, map/session transitions and failed-load
   cleanup, plus stale/malformed descriptions and unaffected live objects.
   Use Ruby's Rebalanced, Mercury and representative Xbox/CE paths,
   both host directions, and matching/mixed builds where their contracts
   permit. Check the existing optional profile as well as the default profile.
   Run affected regression suites, all required platform builds and native/web
   transport checks. Validate claims of OpenCE crossplay against an identified
   upstream executable on the map families/contracts it supports; a
   main/current GulchCE pair alone is insufficient.

Complete the repair only when its root cause and compatibility strategy are
established, the combined implementation is reviewed, and applicable checks
pass with evidence and cleanup recorded. An investigation ending in a
compatibility-dependent deferral leaves the repair milestone open and Ruby's
recovery sign-off withheld. Stage 9's historical failures remain recorded.
The policy change permits agreement separation; isolation, host authority,
meaningful validation and cleanup remain required.

### Milestone 11: shared boat control initialization

Use the [Stage 2 evidence](#stage-2-boat-proof-and-preserved-crossplay) and
[boat investigation](mcc_maps.md#shared-engine-investigation-7-november-2026)
as the starting proof. The three-point boat controller leaves two temporary
water-friction control values uninitialized; both physics paths can consume
them for accepted inputs. The bounded harness demonstrates non-finite forces
and incorrect finite friction with MCC inactive, and the existing MCC
initialization prevents both. This proves a shared producer defect, not that
stock Xbox maps contain affected boats or that full vehicle behavior is tested.

1. **Establish the repair agreement.** Recheck the responsible engine
   and simulation contract against the designated upstream revision. The
   unconditional correction changes accepted Xbox/CE simulation, including
   client prediction of a driven vehicle. Preserve host authority and document
   the required agreement, version separation and rollout before implementation.
   The user's 7 November policy change permits excluding incompatible old peers;
   it does not establish compatibility with an unmodified upstream build.
2. **Correct the producer systemically.** Under the reviewed, version-separated
   contract, initialize exactly the three temporary records in the existing
   three-point branch, preserving its arithmetic, active controls and other
   point-count paths. Retain required `port:` markers for shared source edits.
   Preserve the working MCC correction; retire its helper/hook only when the
   shared implementation demonstrably replaces it for every supported MCC
   path. Do not select the fix by map identity, route legacy maps through MCC
   code, or attach correct initialization to an optional camera/cosmetic mode.
3. **Validate behavior and agreement.** Preserve the existing regression
   coverage and demonstrate failure before the fix and success afterward.
   Check finite and non-finite stale controls, observable forces and resulting
   motion in both physics paths, zero/reverse propeller values, scratch bounds,
   unpowered controls, alternate point counts and unaffected controllers.
   Exercise the Mercury2/Doozy reproduction and relevant installed Workshop
   maps, plus Xbox/CE controls and supported-input fixtures where stock maps
   lack boats. Test actual driving, client prediction and seat replication in
   both host directions, with matching/mixed builds where their contracts
   permit, and verify map-family transitions. Run affected regressions, all
   required platform builds and native/web transport checks. Support crossplay
   claims with an identified upstream executable on supported map contracts;
   a main/current GulchCE comparison alone is insufficient.

Complete the repair only after the new agreement and combined change
are reviewed, applicable checks pass, and evidence and cleanup are recorded.
Convert the twelve strict legacy expected failures to ordinary passing
regressions only when the authorized correction fixes those paths; preserve
their behavioral coverage and historical failures. A compatibility-dependent
deferral leaves this repair milestone open. This remains separate work after
Milestone 10. Implementation, validation and verified cleanup are complete;
the Stage 11 record below includes Mercury's subsequently authorized shared
clock correction and the console lifetime repair, with their actual limits.

#### Former deferral: unconditional shared boat fix

Stage 2 retained the MCC correction and twelve strict legacy expected failures
under the user's earlier crossplay requirement. That constraint was explicitly
lifted on 7 November for Milestone 11. Preserve this evidence and the working
MCC path until the separately validated shared correction replaces it.

## Stage 1: validation baseline

Stage 1 of `mcc-code-refactor` establishes repeatable checks before changing
engine or MCC compatibility behaviour. It changes test tooling, CI and this
documentation only. Xbox, Custom Edition and MCC implementations, simulation,
network version and map-family boundaries are unchanged.

## Revision and build

The baseline is main commit
`db1697c9b0c46289596f22213a5bef8ee327be24`, checked on 7 November 2026.
The isolated worktree is `build/mcc-code-refactor` under the original checkout;
all paths below are relative to that worktree. The original working tree was
left untouched. Do not reuse another checkout's executable as this baseline.

The Windows build used clang 23.1.4, MSVC 14.44.35207 headers/libraries, Windows
SDK 10.0.26100.0, SDL 3.4.16 and these options:

```powershell
python configure.py --portable --release --pgo=off --lto=off
ninja windows
```

The build completed all 1,007 steps. Existing Expat import warnings remain in
the log. `build/windows/halo.exe` SHA-256 is
`f1ec0dca876aa3cf746a4095ac9115cc026902f4679d1c3941deb20928c85d56`;
the adjacent SDL DLL is
`47d508b4232cf9462096bdb6f613a330d9a94a7eb09c305fad073430c6531920`.
These identify a baseline, not a reproducible-binary guarantee.

## What the CI change makes executable

[mcc.yml](../.github/workflows/mcc.yml) now explicitly runs the previously
unlisted avoidance, cameras, Chicago shader, damage-tag, environment-shader,
HUD-layout, movement, particle and seat tests on Linux and Windows. The
Windows job also runs its memory reservation and startup-order checks.
[test_mcc_ci.py](../tools/test_mcc_ci.py) checks that MCC test modules have
an executable CI route, including modules discovered through the harness
directory. It distinguishes a pytest module from a standalone Python runner.

[test_mcc_hud_meter_render.py](../tools/test_mcc_hud_meter_render.py) must run
directly: passing it to pytest executes no GPU checks. Its separate Linux
job builds the project's checksum-pinned SDL version, then uses Xvfb and Mesa
software GL. The job needs clang, SDL3, NumPy, Pillow and SciPy. The shared
[HUD GL builder](../tools/test_hud_ammo_thresholds.py) now includes the
production shader emitters and missing renderer state needed by the current
translator, and preserves 32-bit colour words on an LP64 Linux host.

Two existing harness problems were reproduced and corrected without changing
production code. The pause-menu harness's unused counters failed clang 23's
warnings-as-errors build; they now assert upload/deletion balance, restoration
and diagnostics. The Windows startup fixture assumed the loader would honour
a preferred DLL address despite ASLR policy. Its fake DLL now explicitly
claims an allocation in the MCC window. The test still checks early reservation,
deferred diagnostics and preservation of a conflicting allocation.

## Run installed Workshop maps in place

Use [mcc_native_smoke.py](../tools/mcc_native_smoke.py) on Windows. Supply a
disposable data folder containing a privately sourced Xbox `maps/ui.map`;
`maps/loading.tga` supplies the loading artwork when available. Keep installed
Workshop maps and resources in their Steam folders. No MCC map copy, junction
or alternate Python discovery implementation is needed.

```powershell
python tools/mcc_native_smoke.py `
  --executable build/windows/halo.exe `
  --data-root build/stage1/data `
  --output build/stage1/workshop-final `
  --workshop --steam-root C:/Steam --campaigns --seconds 60 `
  2915693904/a10 2962107814/a10 3021510694/mercury2 `
  3675162225/halo 3003419983/spasm_playground `
  3021510694/mercury_falling 3675162225/ring_mp
```

The engine resolves these virtual `mcc_maps/<item>/<map>` names through its
normal Workshop discovery. `--steam-root` requires `--workshop`; omit it to
use the engine's normal Steam discovery. `--campaigns` enables campaign mission
discovery. Local `mcc_maps` invocation remains supported without these flags.
The tool accepts an optional `mcc_maps/` prefix and either slash convention.

Each case gets a fresh save/log folder and records its effective `HALO_*`
settings, executable hash, elapsed time, exit status and cache cleanup result.
Inherited `HALO_*` overrides are cleared. The copied executable gets a fresh
configuration; rendering uses a hidden 640x480 window, with audio and internet
discovery disabled. `HALO_NULL_RENDERER` must be absent: its presence enables
the null renderer even when its value is `0`. Elapsed time includes loading
and the bounded wait; it is not a performance benchmark.

`run_case` temporarily writes and restores the disposable data root's
`init.txt` through atomic replacement. If the file existed, its original bytes
are retained as `init.original.txt` in the case output for recovery if restoration
fails. A launcher error stops the remaining cases. Do not run two instances
against that data root or console port.
The runner refuses an occupied console port and reports console initialization
or socket-access failures. Before sending a script it checks Windows' TCP
connection table for the exact connection and launched process ID. A passing
result requires the expected loaded-map
log, an exact unique console output line after 150 simulation ticks, a normal
exit and no recognized fatal diagnostic. An echoed command is insufficient.

Only generated regular `saves/z/cacheNNN.map` files inside the fresh case are
removed; linked directories/files are refused. Saves, configuration and useful
logs remain, while copied test executables/DLLs are removed. The caller must
archive inactive saves if necessary and remove its own disposable asset copies
when finished. Never delete original Workshop packages or player saves.

## Reproduce the asset-free and optional fixture checks

Use a Python environment with pytest, PyYAML and the prerequisites named by
the workflow. Initialise the x86 MSVC compiler environment on Windows. The
complete workflow commands are authoritative; a useful local MCC selection is:

```powershell
$mccTests = Get-ChildItem tools/test_mcc_*.py |
  Where-Object Name -ne test_mcc_hud_meter_render.py |
  ForEach-Object FullName
python -m pytest -q -ra $mccTests tools/test_windows_mcc_startup.py `
  tools/test_steam_workshop.py tools/test_cache_file_formats.py
python -m pytest -q tools/harness/tests/test_mcc_boat.py `
  tools/harness/tests/test_mcc_damage.py tools/harness/tests/test_mcc_mods.py `
  tools/harness/tests/test_mcc_script_threads.py
python tools/test_mcc_hud_meter_render.py
python tools/test_hud_ammo_thresholds.py
```

Set `MCC_TEST_MAP` to a real Workshop level to opt into the cache/geometry
fixture checks. Set `HALO_CUSTOM_EDITION_MAPS` to a private CE directory for
the CE map/resource checks. These variables do not install or copy assets.
CI remains asset-free. Windows containment tests require permission to create
their temporary junctions or symlinks; keep those tests intact when a sandbox
prevents fixture creation.

## Evidence and limits

Private logs and results are retained in `build/stage1`; map assets and test
outputs are not committed. The initial broad run recorded 1,646 passes,
18 skips, 13 pause-harness compilation errors and one sandbox-denied junction
fixture. The unchanged preview containment test then passed with junction
creation permitted. The pause harness subsequently passed all 13 cases,
including its missing-unbind negative control. The added CI selection passed
149 checks. Optional Ruby cache/geometry and available CE map/resource checks
passed four checks; OpenSauce and the recorded CE Blood Gulch fixture were
absent. The four MCC modules under `tools/harness/tests` passed 45 checks.

After the combined review, the full local MCC selection (excluding the smoke
runner, checked separately), Windows startup checks, the existing frontend
CI selection, Workshop/CE format tests and those four harness modules passed
1,813 checks with 18 skips. The skips comprise JDK absence, Linux-only
filesystem/sanitizer/fuzzer cases, and optional asset fixtures; four of the
asset checks were exercised separately as described above.
The reviewed smoke-runner regression suite passed 47 checks with one skipped
file-symlink case because Windows did not grant that privilege. Real directory
junction checks and the deterministic file-reparse guard both passed.

The hidden GPU harness passed 36 MCC meter renders; the shared stock HUD
builder passed 15,360 ammunition renders. These compare output pixels for
their specified conditions, not full-map screenshots. The Windows machine
used NVIDIA OpenGL 4.5 on an RTX 5070 Ti. The new Linux Xvfb/Mesa job was
reviewed but had not run at local Stage 1 completion; its subsequent remote
result is recorded under Stage 2 below.

The first sandboxed Workshop attempt loaded and rendered Ruby and Cursed,
but loopback connections failed with Windows error 10013. Neither run counted
as simulation success. The interrupted next case, failed results and cleanup
record are retained separately from the final runs.

The final run used the command above with loopback access permitted. All seven
cases passed the loaded-map, delayed simulation output, diagnostic and normal
exit checks; each ran for the configured 60-second interval:

| Package | Virtual level | Result |
| --- | --- | --- |
| Ruby | `2915693904/a10` | Passed |
| Cursed | `2962107814/a10` | Passed |
| Mercury | `3021510694/mercury2` | Passed |
| Tiny Halo | `3675162225/halo` | Passed |
| Spasm | `3003419983/spasm_playground` | Passed |
| Mercury | `3021510694/mercury_falling` | Passed |
| Tiny Halo | `3675162225/ring_mp` | Passed |

The Mercury logs confirm package-owned bitmaps/sounds; both Tiny Halo cases
selected the installed shared singleplayer bitmap resource. Original Workshop
map hashes were unchanged after the runs. `baseline.json` records map, binary
and resource identities; `workshop-final/results.json` and the individual case
logs retain the checks and settings. The post-review smoke regression run
again passed 47 checks with the same one privilege-related skip.

Cleanup removed 7,675,576,320 bytes of generated map-cache files across the
successful and interrupted attempts, the disposable Xbox UI copy and temporary
test fixtures. No generated map caches or copied test executables remain in
the run folders. Fifty inactive save/cache-support files (221,548,296 bytes)
were archived losslessly to `inactive-smoke-saves.zip` (2,160,738 bytes); archive
members were hash-verified before loose copies were removed, and original
paths/hashes are recorded in `inactive-smoke-saves.json`. The baseline build
and SDL/build inputs (about 229 MB), test environment (150 MB) and diagnostic
evidence including that archive (about 3 MB) remain for the next stage.

Stage 1 establishes test execution and a bounded baseline. It does not prove
campaign completion, full visual/audio fidelity, save/revert, multiplayer
agreement, map-family transitions, performance improvements or Android/web
runtime compatibility. No simulation, protocol, engine or platform build
generator changed, so Android/web rebuilds and transport interoperability runs
were not required for these tooling changes. Later engine changes still need
their affected platform, networking and cross-family checks under
[AGENTS.md](../AGENTS.md) and [CODINGSTANDARDS.md](../CODINGSTANDARDS.md).

## Stage 2: boat proof and preserved crossplay

The [boat investigation](mcc_maps.md#shared-engine-investigation-7-november-2026)
establishes a shared producer-initialization defect. The user chose to preserve
the pinned OpenCE 41 crossplay contract and defer an unconditional engine fix.
This stage therefore changes regression coverage and documentation, with no
production or protocol change. The existing MCC correction remains active.

The initial expanded test ran against unchanged production code and reported
eight failures with eighteen passes. Non-MCC NaN controls reached the real
force consumer; finite stale values produced incorrect friction. The finished
harness also exercises the actual radius-based dispatch and `physics_update_old`,
checking its resulting linear velocity. The newer path checks computed forces
before its separately stubbed integrator. Both use an axis-aligned fixture with
controlled transforms and world queries, not a real map or a whole game.

The boat suite passed 52 checks and recorded twelve strict expected failures.
The latter are the identified legacy force defect, not successful compatibility
checks. Only its dedicated force-failure exception is eligible for an expected
failure; compilation, active-control and scratch-boundary errors remain failures.
Mutations verify that distinction. Removing the expected-failure handling for
the two legacy water/NaN cases reproduced failures in both physics paths.

```powershell
python -m pytest -q -rx tools/harness/tests/test_mcc_boat.py
# Deliberately reproduces the two deferred failures:
python -m pytest -q --runxfail tools/harness/tests/test_mcc_boat.py -k legacy_nan_water
```

Eight additional cases remove the MCC guard only from an extracted temporary
test copy. They show that unconditional initialization corrects the measured
forces with either poison pattern and both physics paths. This is evidence for
the deferred change, not shipped behavior. Production still uses its original
MCC guard. No performance improvement or new runtime compatibility is claimed.

The first remote Stage 1 run exposed a missing `libxtst-dev` dependency while
configuring SDL for the new Linux GPU job, before any render check ran. Commit
`4acb59d66` adds that dependency; the job retains SDL's enabled X11 functionality.
Workflow syntax and its 65 coverage checks passed locally. Remote results and
the post-review checks are recorded below. Private investigation logs are
retained under `build/stage2`.

Self-review and an independent read-only review of the combined tests,
documentation and CI dependency repair found no remaining corrections.
The final local boat, neighboring MCC harness, protocol, camera-protocol and
CI-coverage selection passed 167 checks with the twelve documented expected
failures. Documentation checks verified 55 local links, the investigation
anchor, named functions and Python syntax, and confirmed that no production
file changed. No new game run was needed for these test/documentation changes;
Stage 1's unchanged binary and runtime evidence remain applicable within
their original limits.

The original remote [Build run at `b8d2f0dc2`](https://github.com/chestahh/GulchCE/actions/runs/42373756516)
passed Windows, Linux, Android and web debug builds. Android included its APK
and importer checks; web included the browser/native transport harness.
The [MCC run](https://github.com/chestahh/GulchCE/actions/runs/42373756545)
passed its Linux/Windows pytest jobs but failed SDL configuration in the new
GPU job as described above.

The subsequent [Build run at `24fc89cdd`](https://github.com/chestahh/GulchCE/actions/runs/42374909776)
passed all four platform jobs with the expanded boat coverage. These are debug
builds; this workflow builds releases only on main. Android compiled the guest
and host, built its APK and passed the importer checks. Web passed its build
and browser/native transport checks with Chromium and Firefox, including the
configured packet-loss cases. These transport fixtures do not exercise a real
map or establish crossplay gameplay compatibility.

Linux's combined regression invocation passed 2,478 checks with 27 skips,
twelve expected failures and seven passing subtests. Across its pytest
invocations the job recorded 2,794 passes, 52 skips and the same twelve expected
failures; its ordinary and full-LTO ELF light-storage checks also passed.
The expected failures are the explicitly deferred boat defect described above.
Skips are retained as skips, including checks requiring unavailable retail
assets; they are not evidence of gameplay coverage.

The corresponding [MCC run](https://github.com/chestahh/GulchCE/actions/runs/42374909804)
passed all three jobs: Linux readers/validators, Windows adapters and Linux
GPU rendering. The corrected GPU job built the pinned SDL version with X11
enabled and passed all 36 meter render checks under Xvfb/Mesa. The Linux
reader/validator job passed 1,615 checks with sixteen skips; its separate
ASan/UBSan invocation passed 38 checks.

These results validate code and test revision
`24fc89cdd46761f010d81a41f16ad5ba6170487e`. The following documentation-only
commit records the completed runs; it does not change that implementation.
No production source or protocol changed in either stage. The twelve deferred
legacy boat failures remain unresolved, and the Stage 1 runtime limits still
apply. No Android-device gameplay, browser-map gameplay, full campaign,
save/revert or new map-family transition result is claimed.

Stage 2 launched no game instances and created no map copies or saves.
Temporary harness binaries and pytest fixtures/cache were removed after
checking their resolved paths and rejecting links. About 1.9 MB of diagnostic
logs and supporting evidence remain under `build/stage2`; no generated game
caches or test executables remain there. The Stage 1 build, environment and
verified save archive remain available as described above.

## Stage 3: resource ordering and bitmap ownership lookup

This stage removes two demonstrated sources of unnecessary work within
MCC-owned code. It changes no format interpretation, map-family dispatch,
simulation, network version, content-identity framing or resource lifetime.

`mcc_resource_open` no longer sorts its private directory by byte offset.
Every consumer finds entries by name and checks the whole directory for
duplicates; none depends on offset order. The comparator and sort call were
removed, while all bounds, duplicate-name and grouped-slice checks remain.
The expanded resource suite passed 117 checks before and after the change.
Compiled old/new readers also returned identical payloads, CRC32 and SHA256
for four fixed-input comparisons covering bitmap/sound files and both directory
orders. Different on-disk directory orders still have distinct raw identities;
only the unused in-memory reordering was removed.

`mcc_bitmap_valid_texture` now returns the owned, validated texture to both
`mcc_bitmaps_valid` and `mcc_bitmaps_pixels`. A successful pixels request no
longer searches the table twice. Validation order, first-match behavior,
failure-output clearing, descriptor identity, handles, ranges, channel flags
and storage checks remain intact. No lookup cache or new persistent state was
added. The ownership fixture checks both public entry points, copied/foreign
and mutated descriptors, missing storage, unchanged descriptor bytes, verifier
short-circuiting, duplicate entries, pressure beyond the legacy cache budget
and disposal. The media/flags/bridge selection passed 59 checks.

In isolated extracted old/new routines, the same 39 successful bindings over
13 descriptors required 546 pointer comparisons before and 273 after; returned
allocations, sizes and sampled pixel bytes matched. This measures removed
traversals, not frame rate or map-loading speed. The separate per-conversion
stream-offset scan and content-specific HUD exceptions were not changed.

Private evidence is retained under `build/stage3`. The original Stage 1 Windows
executable was preserved at `build/stage3/baseline/halo.exe` before rebuilding
`build/windows/halo.exe`; its recorded SHA-256 remains unchanged. Runtime
checks use installed Workshop maps directly and isolated Xbox/CE fixture
copies, saves and logs. Final review, build and runtime results follow below.

Self-review and an independent read-only review covered all Stage 3 code,
callers, lifecycle paths, tests and documentation; no further correction was
needed. After that review, the broad local MCC, Workshop, CE-format, protocol
and neighboring MCC harness selection passed 1,909 checks with nineteen skips
and the twelve previously documented boat expected failures. Skips covered
the JDK missing from this shell's PATH, Linux-specific checks, file-symlink
privilege and optional map fixtures. Four optional checks were then run with
Ruby's Workshop `a10` and the available CE Death Island/resources; all passed.
The Windows release build succeeded with the original configure options;
the existing Expat import warnings remained. Its executable SHA-256 is
`7608089e8b1c003ba7ff15c3120392d07342af473f93a50b3471e0f60e330c3d`.
The direct GPU runners passed 36 MCC meter cases and 15,360 stock HUD ammunition
cases on the local NVIDIA renderer.

The baseline and updated executables each completed this sequence in one
hidden 640x480 process, with fresh saves and actual rendering:

1. Workshop `3021510694/mercury_falling`, selecting package-owned resources.
2. Xbox `levels/test/bloodgulch/bloodgulch`.
3. Custom Edition `custom_maps/deathisland`.
4. A disposable, header-valid MCC fixture with an invalid tag index; the
   loader refused it and returned to the menu.
5. Workshop `2915693904/a10`, restoring MCC operation after the failed load.
   Ruby's log reports no external resource selection in this run.

Each valid map required a fresh simulation-tick epoch, an exact console marker
after 150 simulation ticks, and a subsequent rendered capture. Both processes
exited normally with no fatal diagnostic. Before/after captures were visually
inspected: the MCC intros and Xbox/CE gameplay rendered their expected scenes
and textures. They are not frame-synchronized image-equivalence tests; moving
cinematics and unique console markers differ. The malformed fixture tests early
load recovery before resource allocation, not every possible allocation failure.
These runs establish the named transitions and bounded rendering/simulation,
not campaign completion or new multiplayer, save/revert or device compatibility.

An additional 45-second hidden Tiny Halo run (`3675162225/halo`) passed the
loaded-map, delayed simulation marker, diagnostic and normal-exit checks. Its
log explicitly selects `mcc_maps/_shared/singleplayer/bitmaps.map`, covering
the installed shared-resource path separately from Mercury's package resources
and Ruby's recovery run.

Input verification rehashed all six original Xbox/CE fixture files and their
copies, the three exercised Workshop maps, Mercury's bitmap/sound resources
and the installed shared singleplayer bitmap resource; all hashes matched.
The three game runs removed 2,302,672,896 bytes of generated map caches.
Temporary test binaries, fixture directories and asset copies were removed.
The 107 inactive save/support/capture files (98,548,658 bytes) were archived
losslessly to `build/stage3/inactive-runtime-evidence.zip` (7,455,474 bytes).
Every member was hash-verified before removing its loose copy; the adjacent
JSON manifest records original paths, sizes and hashes. The baseline executable
and SDL DLL (about 27 MB) remain for later refactor comparisons, alongside the
current Windows build, test environment and diagnostic evidence.

The [MCC workflow at `829ef117b`](https://github.com/chestahh/GulchCE/actions/runs/42377561461)
passed all three jobs. Its Linux reader/validator selection passed 1,652 checks
with sixteen skips, followed by 38 passing ASan/UBSan checks; the Windows job
passed 1,853 checks with seventeen skips across its invocations. The direct
Xvfb/Mesa job passed all 36 MCC meter renders.

In the corresponding [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42377561452),
all four debug platform builds passed. Branch builds do not run the main-only
release job; the local Windows release build is recorded above. Android built
its guest/host and APK and passed touch, gyro and importer checks. Web passed
its JS/browser and native transport assertions for Chromium and Firefox, including configured packet
loss. These remain asset-free transport checks, not multiplayer gameplay
qualification. Linux's combined invocation passed 2,478 checks with 27 skips,
twelve expected failures and seven passing subtests. Across its pytest
invocations it recorded 2,794 passes, 52 skips and the same twelve expected
failures; ordinary and full-LTO ELF light-storage checks also passed. The
expected failures remain the deliberately deferred Stage 2 boat defect.

Both workflows validated implementation revision
`829ef117b945152dcffd61d9bba9994efcec5b66`. The following documentation-only
commit records their completed results and cleanup. Final documentation review
checked the claims against logs and archive metadata; local-link, named-symbol,
Python-syntax and whitespace checks passed. Roughly 36 MB remains under
`build/stage3`, including the 27 MB baseline build and 7.5 MB verified archive.
No disposable map copies, game caches or temporary test binaries remain there.

## Stage 4: per-tag bitmap stream preparation

This stage simplifies MCC-owned conversion bookkeeping. It does not change
pixel decoding, format interpretation, shader selection, simulation, network
version or Xbox/Custom Edition implementations. The old private converter
scanned every texture entry to recover the next stream offset for each texture.
Preparation now passes a local cursor to `mcc_texture_prepare`, resets it when
the full runtime handle changes, and advances it only after success.

The ordering contract is established by the actual constructors: validated
original handles have unique tag indices, `mcc_bitmaps_prepare` appends each
original bitmap block contiguously, and `mcc_bitmap_specialize` appends each
clone block under a fresh handle. Reusing a clone appends nothing. Empty blocks
add no descriptors. The items are not reordered before conversion. A resource
handle is not a runtime handle: clones can share the former while owning
separate streams. No persistent index, extra allocation or new shared-engine
hook is needed. Existing per-allocation, per-tag and aggregate bounds, failure
publication, loading progress and disposal remain unchanged.

The expanded extracted-source fixture exercises the actual preparation,
tag/block and clone constructors, with controlled consumer selection. It covers
multiple descriptors and original tags, both clone creation orders, clone
reuse, empty groups, exact stream boundaries, aggregate storage, and failures
in allocation, internal reads, external-resource reads and packing. Failed
descriptors remain unpublished; disposal releases earlier successful textures.
The same contracts were first checked against unchanged Stage 3 production
code. Private evidence and baseline executables are under `build/stage4`.

All 45 converter cases passed before and after. The construction cases also
produced identical full descriptor/pixel traces, read counts and storage ends.
Two deliberately broken private variants failed the expected offset assertion:
using resource handles for boundaries, and omitting cursor advancement. The
focused converter/media/channel/bridge selection passed 72 checks. Self-review
and an independent combined review found no remaining production cleanup or
structural correction justified within this stage.

Private Windows release probes were built from Stage 3 revision `53fe6673b`
and the updated bitmap module, replacing only that translation unit. Their
999 shared response-file inputs, configure options and probe framing matched. After
successful conversion, the probe hashes every initialized pixel-allocation byte
and 21 explicitly framed semantic metadata fields for every texture, excluding
addresses and structure padding. It also records ordered handle groups, total
storage and the maximum per-tag stream end. The comparator verifies complete
records, identities, counters and digests before comparing them. Instrumented
timings are not used to claim a loading-speed improvement.

All seven paired Workshop runs matched every recorded texture, handle group
and stable summary field:

| Installed Workshop map | Textures | Clones | Converted bytes | Old table visits | New cursor advances |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ruby's Rebalanced `2915693904/a10` | 1,275 | 10 | 107,505,536 | 1,625,625 | 1,275 |
| Ruby's Rebalanced `2915693904/b40` | 1,181 | 11 | 115,751,552 | 1,394,761 | 1,181 |
| Ruby's Rebalanced `2915693904/d40` | 1,444 | 11 | 150,436,608 | 2,085,136 | 1,444 |
| Cursed `2962107814/a10` | 1,531 | 29 | 265,864,960 | 2,343,961 | 1,531 |
| Spasm `3003419983/spasm_playground` | 908 | 12 | 200,100,480 | 824,464 | 908 |
| Mercury `3021510694/mercury2` | 1,577 | 47 | 606,337,280 | 2,486,929 | 1,577 |
| Tiny Halo `3675162225/halo` | 846 | 9 | 138,963,328 | 715,716 | 846 |

Together these compare 8,762 textures, 129 clones and 1,584,959,744 converted
bytes. The removed scans made 11,476,592 table visits; the cursor advanced
8,762 times. All fourteen hidden processes used the real NVIDIA renderer,
required a fresh simulation-tick epoch and exact console output after 150
ticks, captured a subsequent rendered frame, and exited normally. The complete
native smoke failure-pattern scan passed for all log/console sinks. Before/after
captures were visually inspected and showed the same expected scenes and
textures; these are not synchronized image-equivalence tests. Ruby's runs are
opening cinematics, while Spasm reaches its initial gameplay view. Installed
Workshop maps were read in place, without package copies.

After combined review, the broad local MCC, Workshop, CE-format, protocol and
neighboring harness selection passed 1,920 checks with nineteen skips and the
same twelve deferred boat expected failures. The skips retain their earlier
meaning: unavailable JDK/symlink privileges, Linux-only checks and optional
assets. Four optional real-map checks then passed with Ruby's `a10` and the
isolated CE fixtures. Direct GPU checks passed 36 MCC meter cases and 15,360
stock ammunition cases. The Windows release build succeeded with the unchanged
`--portable --release --pgo=off --lto=off` configuration and existing Expat import
warnings. Its executable SHA-256 is
`2f22adeba256b26843cfb8375c55c26d95535ea47491d172b5aab85874edf52b`.

The uninstrumented baseline and updated release executables each passed the
same one-process sequence: Mercury Falling with package resources, Xbox Blood
Gulch, CE Death Island, the intentionally invalid MCC tag-index fixture, then
Ruby's `a10`. Every valid map required a fresh tick epoch, delayed console
marker and rendered capture; both processes exited normally. This confirms
early failed-load recovery and the named family transitions. The malformed
fixture does not exercise late allocation failure; the extracted-source
failure cases cover conversion publication/disposal separately.

Both uninstrumented executables also ran Ruby's `b40` long enough to leave
the opening cinematic and render the initial gameplay area, including its
HUD, weapon and environment. Each completed the console marker after 1,800
simulation ticks, saved a later frame and exited normally. Before/after
captures were visually reviewed. All eighteen game runs passed the complete
native smoke diagnostic scan, allowing only the deliberately requested
`refactor_invalid` refusal in the two transition runs. No protocol version or
simulation code changed; network version 41 and the deferred boat behavior
remain intact. These are bounded runtime checks, not full-campaign, new
multiplayer gameplay, save/revert, Android-device or browser-map qualification.

All six original Xbox/CE fixture files and their copies, the eight exercised
Workshop maps, Mercury's bitmap/sound resources and the installed shared
singleplayer bitmap resource were rehashed and unchanged. Each game run cleaned
its generated map caches: 13,816,037,376 bytes across eighteen sequential runs.
The 254 inactive save/support/capture files (516,681,213 bytes) were archived
losslessly to `build/stage4/inactive-runtime-evidence.zip` (23,450,825 bytes).
Every member was hash-verified before removing loose copies; the adjacent JSON
manifest records their original paths. Private probe executables, objects and
debug symbols, disposable asset copies and pytest fixtures were removed after
checking containment and rejecting links. The Stage 3 baseline executable/SDL
pair remains for comparison, alongside the current release build and useful
logs, reproducible proof sources and archive.

The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42380303410)
passed all three jobs at implementation revision
`e670a18926acc4d69e574a6172f4d3e2ddd2bd62`. Linux readers/validators passed 1,663
checks with sixteen skips, followed by 38 passing ASan/UBSan preview checks.
Windows adapters passed 1,864 checks with seventeen skips across their
invocations. The Linux Xvfb/Mesa job passed all 36 MCC meter renders.

The corresponding [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42380303636)
passed all four debug platform builds; the main-only release job was skipped.
The local Windows release result is recorded above. Linux's combined invocation
passed 2,478 checks with 27 skips, twelve expected failures and seven passing
subtests. Across its pytest invocations, it recorded 2,794 passes, 52 skips and
the same expected failures and subtests; ordinary and full-LTO ELF light-storage
checks passed. Windows COFF light-storage and corruption-negative checks passed.
Android built its guest/host and APK and passed touch, gyro and MCC importer
checks. Web passed its JS/browser checks and native Chromium/Firefox transport
assertions, including configured packet loss. These remain asset-free transport
tests, not multiplayer gameplay qualification.

Final evidence review checked the combined Stage 4 claims against the retained
logs, manifests and archive. Local-link, named-symbol, Python-syntax and
whitespace checks passed. This documentation-only completion records CI for the
implementation revision above without changing its code or tests. About 60 MB
remains under `build/stage4`, primarily the 27 MB baseline build, 23.5 MB verified
archive and diagnostic logs. No disposable map copies, generated game caches
or temporary test/probe binaries remain there.

## Stage 5: HUD ownership and identity-independent normalization

Stage 5 starts at `a9aaa8b3ae70c7d0ac7490f0c5c181d45e663eae`. The preserved
Windows release baseline has the Stage 4 executable hash above. This stage
corrects an MCC compatibility-engine defect: supported interned HUD storage
was normalized in place for each consumer, so earlier writes could become
later consumers' input. Same-group root aliases could halve offsets, scales,
digit advances or margins twice. Shared item storage could also inherit the
wrong bitmap factor or frame marker. These are ownership errors, independent
of map names, Workshop IDs or checksums.

`mcc_hud_ownership.inl` plans from pristine data, identifies root aliases and
isolates mutable block consumers before normalization. Pointer-free overlay
and crosshair items receive snapshots even when no second HUD reference is
visible: their storage can be interned inside a different tag allocation.
This intentionally makes supported scalar interning independent of validator
traversal order. It does not preserve the old order-dependent rejection of
otherwise supported scalar storage. The actual-validator regression cases
retain rejection of conflicting pointer-bearing/root owners and partial
overlaps. For shared pointer-bearing prefixes, the largest original extent
remains reachable so a copy cannot hide a conflicting tail.

All snapshots finish before pointer publication. Temporary planning storage
is freed on success and failure; copies use the existing MCC runtime arena
and its normal teardown. The four-million-record budget is checked before
layout recognition; bitmap geometry walks retain their existing range checks.
No new shared-engine hook, persistent state, setting or network-version
change is involved. Xbox and Custom Edition loading and normalization remain
unchanged, as does the explicitly deferred shared boat fix.

The first whole-converter reproduction used production structures and source
with synthetic tag storage. Unchanged Stage 4 code failed 30 of 45 cases;
fifteen independent-storage controls passed. A separate fixture combines the
actual converter and actual MCC validator. Its baseline failed five cases:
scalar-first admission, scalar-last source corruption, repeated root conversion
and both shared-prefix orders. Eight controls passed. Replacing largest-owner
retention with shortest-owner retention in a private mutation made both
conflicting-tail tests fail, showing that these negative cases detect hidden
overlaps. No original map file was modified for these reproductions.

An inventory read ten installed Workshop maps in place: Ruby's Rebalanced
`2915693904/a10`, `b40` and `d40`; Mercury `3021510694/mercury2` and
`mercury_falling`; Cursed `2962107814/a10`; Tiny Halo `3675162225/halo`; Spasm
`3003419983/spasm_playground`; Descent `3042265379/firefight_descent_mcc`; and
Descent Quarantine `2963835242/firefight_descent_quarantine_mcc`. None contained
shared roots or placement blocks within the inspected HUD graph. The inventory
did not compare scalar addresses against every non-HUD allocation. These maps
are unchanged-behavior controls, not claimed reproductions of the
ownership bug. BioHalo was not found in the inspected `C:\Steam` library and
was not runtime-tested.

The inventory contains 487 component HUDs, including 92 unit HUDs: 71 recognized
and 21 unrecognized unit layouts. All eleven existing unit profiles were
represented. Removing bitmap names from recognition preserved all 487 original
classifications and introduced no recognized/unrecognized class collision in
this sample. Private production-C visitor checks passed 100 cases comparing
3,896 component records, including name-only changes, placement and bitmap
registration near misses, and unterminated names. Names remain bounds-checked.
The ten weapon/grenade profiles were already independent of bitmap names.

The 21 structural profiles remain a documented compatibility exception.
Supported anchors and scale flags do not establish the author's intended safe
frame; the unused canvas field supplies no demonstrated replacement rule.
Unknown layouts therefore retain the authored MCC frame. Replacing these
profiles requires evidence distinguishing stock and custom margins, including
near misses. This stage removes an irrelevant identity dependency without
claiming that every pre-existing exception is now eliminated or necessary.

Layout recognition now reads each distinct pristine root once. Conversion
writes frame markers alongside the corresponding placement changes, removing
the repeated geometry/name walk previously used to stamp recognized layouts.
The later `mcc_tags.c` placement pass was also redundant: successful HUD
conversion had already consumed every high-resolution placement flag it
tested. A private comparison of the actual old and trimmed metadata functions
restored the same arena before each call and matched every byte and return
value across 56 cases. The retained pass still clears unsupported meter
overlay metadata and checks the affected roots and meter blocks.
The ten-map inventory requires 798 scalar block copies containing 935 items,
104,732 bytes in total before arena alignment. This small added storage buys
correct consumer ownership; no loading-time or peak-memory improvement is
claimed. Private sources, inventories, reproduction logs and build manifests
are retained under `build/stage5`.

The first updated native probe exposed a fixture gap: `cseries.h` maps `free`
to `debug_free`, whose contract rejects NULL. The planner's initial nullable
free crashed before probe output. The batch stopped and its useful evidence
was retained. Nullable releases are now guarded. Both converter fixtures
enforce the engine's non-NULL free contract; the normalization fixture also
fills allocations with `0xCA` and enforces the allocator's individual size
limit. A strict private fixture fails the original planner and passes the
correction, including bitmaps-only input and every injected planning-allocation
failure. Final focused checks pass 579 cases, including 70 whole-normalization
cases and 132 validator cases. Independent re-review checked the correction,
remaining allocation bounds, ownership lifecycle, metadata trim, tests and
documentation before the final combined checks.

The final local MCC, Workshop, CE-format, protocol and neighboring harness
selection passed 2,004 tests, with nineteen skips and the same twelve deferred
boat expected failures. Skips cover unavailable JDK/symlink privileges,
Linux-only checks and optional assets. Four optional real-map checks then
passed using Ruby's `a10` and isolated CE fixtures. Direct GPU checks passed
36 MCC meter renders and 15,360 stock ammunition renders. Documentation checks
validated 136 local links, named routes and changed Python syntax; production
changes are confined to the four MCC HUD/metadata files.

The Windows release build passed with unchanged
`--portable --release --pgo=off --lto=off` options and the existing Expat import
warnings. Its executable SHA-256 is
`3e563cd2291cb72196350e68dac471f6f5b96799164177f4ecf9309dd647893a`.

Private Windows release probes compare the Stage 4 HUD module with the final
Stage 5 HUD module. Their 999 other link inputs match exactly, including the
old metadata object; the separate 56-case metadata proof and final
uninstrumented build cover that trim. Probe includes are copied privately and
hashed. The observer runs only after successful HUD conversion, reads without
allocating or mutating game state, and records selected normalized HUD fields
in a stable order rather than addresses or all tag data. Its comparator checks record framing, per-tag/aggregate
digests, inventory coverage, executable identity, runtime settings, capture
dimensions and all native smoke failure patterns across four log sinks.

All ten pairs match every recorded normalized field and stable summary:

| Workshop map | Records | Placements | Added arena bytes |
| --- | ---: | ---: | ---: |
| Ruby `2915693904/a10` | 847 | 180 | 6,832 |
| Ruby `2915693904/b40` | 909 | 228 | 7,504 |
| Ruby `2915693904/d40` | 1,019 | 221 | 7,616 |
| Cursed `2962107814/a10` | 1,273 | 325 | 16,128 |
| Quarantine `2963835242/firefight_descent_quarantine_mcc` | 983 | 169 | 7,520 |
| Spasm `3003419983/spasm_playground` | 1,188 | 508 | 22,768 |
| Mercury `3021510694/mercury2` | 1,425 | 324 | 12,992 |
| Mercury `3021510694/mercury_falling` | 1,003 | 204 | 7,152 |
| Descent `3042265379/firefight_descent_mcc` | 1,593 | 340 | 13,664 |
| Tiny Halo `3675162225/halo` | 730 | 149 | 6,128 |

Together these cover 10,970 records and 2,648 placements. The 108,304-byte
arena increase includes scalar-copy alignment. These are allocation sizes,
not measured peak-memory or loading-time improvements. All twenty hidden
processes advanced through a delayed 150-tick console expression, captured a
subsequent frame through the real NVIDIA renderer and exited normally.
Before/after captures were visually inspected; most short runs show opening
cinematics, Spasm shows gameplay, and Descent's short capture is still black.
They are not synchronized pixel-equivalence tests or complete HUD gameplay
qualification.

Nine map pairs pass the full diagnostic scan. Quarantine has the same
pre-existing refusal in both builds:
`mcc: script validation refused life_lost_0 (ai_deselect); 1 scripts won't run, 0 globals start at their defaults`.
Its normalized-output comparison passes, but its runtime is explicitly
script-limited, not a clean smoke pass. The comparator requires the exact
diagnostic and occurrence list to match and rejects additional failures.
This unrelated script-compatibility gap was not changed or hidden.

The final uninstrumented release and preserved baseline each passed the same
one-process sequence: Mercury Falling with its package resources, Xbox Blood
Gulch, CE Death Island, the deliberately invalid MCC tag-index fixture, then
Ruby's `a10`. Every valid phase required a fresh tick epoch, delayed console
output and a subsequent rendered capture. Both processes exited normally.
This verifies the named family transitions and early failed-load recovery;
synthetic allocation failures cover later conversion cleanup separately.

Both builds also ran Ruby's `b40` at 640-by-480 with High-Res HUD enabled and
1280-by-720 with it disabled, and Descent at 1280-by-720 with it enabled. Each
completed a console expression after 1,800 simulation ticks and captured later
gameplay, including the HUD, weapon and environment. Visual review found the
expected placement, scale and custom layout preserved. Timing-dependent
animation, camera and flashing meter differences are not pixel comparisons.
The eight paired uninstrumented runs and one exploratory baseline Descent run
passed an independent full diagnostic scan, executable/settings checks and
capture/simulation checks. Only the exact requested invalid-map refusal is
allowed in transition logs.

No original map files, network fields, simulation logic or map-family boundaries
changed. Network version 41 and the deferred boat behavior remain intact.
These results do not establish full-campaign, save/revert, multiplayer gameplay,
local split-screen, Android-device or browser-map compatibility. Existing
asset-free layout checks cover split-screen geometry; no new live split-screen
session was run.

The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42385944116)
passed all three jobs at implementation revision
`809f8b8cdb9ba3dd9e5727cc261e8f3043eba931`. Linux readers/validators passed
1,747 checks with sixteen skips, followed by 38 passing ASan/UBSan preview
checks. Windows adapters passed 1,948 checks with seventeen skips across their
invocations. The Xvfb/Mesa job passed all 36 MCC meter renders.

The corresponding [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42385944018)
passed all four debug platform builds. The main-only release job was skipped;
the local Windows release result is recorded above. Linux's main invocation
passed 2,478 checks with 27 skips, twelve expected failures and seven passing
subtests; its pytest invocations totalled 2,794 passes and 52 skips, with the
same expected failures and subtests. Ordinary and full-LTO ELF light-storage
checks passed. Windows COFF light-storage and corruption-negative checks
passed. Android built the guest/host and debug APK and passed touch, gyro and
MCC importer checks. Web passed its page/browser checks and native
Chromium/Firefox WebRTC assertions, including configured packet loss. These
transport checks do not constitute map-based multiplayer qualification.

All six original Xbox/CE fixture files and their copies, ten Workshop maps,
Mercury bitmap/sound resources and the installed shared singleplayer bitmap
resource were rehashed and unchanged. Thirty sequential process attempts
included the one diagnosed allocator crash and 29 normal exits; generated
map caches were removed after every attempt, totalling 23,026,728,960 bytes.
The 375 inactive save/support/capture files, including the failed attempt's
useful evidence, totalled 942,965,724 bytes. They were archived losslessly to
`build/stage5/inactive-runtime-evidence.zip` (66,518,323 bytes). Every archive
member was hash-verified before loose copies were removed; the adjacent
manifest records original paths and hashes.

Verified disposable fixture copies, pytest outputs and private probe binaries,
objects and debug symbols were removed without following links. About 108 MB
remains under `build/stage5`, primarily the 66.5 MB archive, 27 MB preserved
baseline build, inventories and diagnostic evidence. The final release build
remains usable under `build/windows`. No disposable map copies, generated game
caches or temporary test/probe binaries remain under this stage.

Final evidence review checked the combined implementation, preserved caller
boundaries, ownership/failure paths, tests, CI and documentation. It corrected
the allocator-contract gap, removed the demonstrated redundant work, and found
no further justified structural change within this stage. Local links, named
symbols, Python syntax and whitespace checks passed. This documentation-only
completion records results for the implementation revision above; it changes
no production code or tests.

## Stage 6: checksum consolidation

### Boundary and exact compatibility contract

The baseline is `18c01ec34`, with Windows release executable SHA-256
`3e563cd2291cb72196350e68dac471f6f5b96799164177f4ecf9309dd647893a`.
The initial checkpoint, resource, saved-game, identity, precache, protocol and
script-thread selection passed all 213 tests before implementation.

The four MCC consumers implement the same reflected CRC32 polynomial
(`0xedb88320`). Checkpoint and thread blocks begin with an all-one internal
seed and complement the result. Resource checks begin with the complemented
stored map checksum and append selected bitmap then sound resources, each
framed by its one-byte type and a `0xff` terminator. Only the published zero
is changed to one. SHA-256 separately hashes the map and selected resources
with length/type framing; it is not replaced by CRC.

Design review rejected a direct use of `source/memory/crc.c`: its
`unsigned long *` interface does not match portable `uint32_t` storage on
LP64 standalone readers, and its lazy mutable table would introduce an
initialization dependency. The existing port-prefixed zlib CRC is already
linked by Windows, Linux, Android and web. Their build configuration uses
its static tables; Android includes the implementation in the ILP32 guest
without adding host imports. Explicit relative header includes select this
port library rather than the separate original game zlib. No vendored code,
shared engine implementation or build graph needs modification.

The MCC callers use zlib's finalized CRC convention: seed zero for saved
blocks, the map checksum for resource chaining, and no extra complement.
Resource framing bytes remain explicit unsigned bytes. Bounds, selected
resources, file errors, loading pulses, incremental work budgets and SHA
framing retain their existing paths. There is no new mutable checksum state
to initialize or dispose. This is content-independent MCC code maintenance;
no map identities or compatibility exceptions select the change.

### Implementation review and local evidence

Review covered the four consumers, their callers, standalone linkage, test
oracles and reader documentation. The prefixed header must follow `cseries.h`
in the resource translation unit: the first release build caught a collision
between C-library limits macros and the engine's enum names. Correcting that
include order required no shared-source or build changes. The resulting
Windows release build passed with the existing Expat linker warnings.
Independent review found no further production or test blocker before the
final local regression selection.

The new checkpoint/thread characterization passed 96 checks on both old and
refactored implementations. A private same-process comparison using extracted
old/current functions compared 4,194,304 persisted bytes exactly across five
cases: ordinary/max inventory, ordinary/max extended names, and the expanded
thread image. The committed tests independently check payloads, footer fields,
little-endian checksums, reserved bytes and thread coverage against Python's
zlib, retaining existing corruption/refusal and migration cases.

Resource/catalog characterization passed 196 checks before production edits;
the final focused selection passed 238 checks. CRC oracles cover every resource
mask, zero/high-bit/all-one seeds, 16/32 KiB boundaries, final-zero remapping,
an intermediate zero before the second resource, cancellation and failed reads
without publishing partial output. SHA-256 and package-isolation coverage
remain in place. The complete original Mercury bitmap (429,799,299 bytes) and
sound (257,054,056 bytes) resources produced identical baseline/current/Python
checksums for bitmap-only, sound-only and combined masks. They were read in
their installed Workshop folder and rehashed unchanged. This private comparator
tests the actual reader/checksum with explicit resource paths; it does not
claim to exercise Workshop discovery or network admission.

The removed incremental table reduces the x86 check object from 2,680 to
1,656 bytes, measured with `sizeof`; the synchronous 1,024-byte local table and
both per-check table-construction loops are also removed. No new allocation
or table is introduced. An isolated Windows x86 `-O2` comparison passed 24
old/new/oracle vectors. Seven alternating-order trials over 128 MiB per sample
measured median CRC time of 0.3883 to 0.03890 seconds for 256-byte calls and
0.3867 to 0.01832 seconds for 1 MiB calls (about 10x and 21x). These are checksum
microbenchmarks, not whole-game loading, checkpoint or frame-rate measurements.

The final broad local selection passed **2,059 tests**, with nineteen skips and
the twelve unchanged strict boat expected failures. Production changes remain
entirely in MCC-owned files. Serialized formats, SHA content proof, network
version 41, OpenCE crossplay and Xbox/CE implementations are unchanged.
Private proof sources, commands, logs and hashes are under `build/stage6`.
The final focused checkpoint/thread selection includes two additional exact
thread-length boundary cases and passed 98 checks. Four optional real MCC/CE
asset checks also passed. The post-review release rebuild required no further
work; its executable SHA-256 is
`5f3f449c13fddf4198157bd94fed9cdc1fad02290ec34d966c3d420d0aa998c7`.

### Native runtime, retained findings and cleanup

The preserved baseline and final release each passed the same hidden,
singleplayer save/revert/core sequence on Ruby's Rebalanced `b40` and Mercury
Falling, loaded from their installed Workshop packages. Each run advanced
150 ticks, saved a changed MCC campaign flag, proved a subsequent mutation,
restored the checkpoint and confirmed the saved flag after another 150 ticks.
It repeated mutation/restoration through `core_save`/`core_load` and exited
normally. The paired executable hashes, settings, runner and map checksums
were checked independently. All eight retained 20 MiB images passed an
independent check of their protected MCC payload/footer/thread/name blocks.
These independently validated runtime images are not claimed byte-identical
across runs; the controlled five-case comparison above establishes exact
before/after serialized-byte equivalence.

The final release also passed Mercury Falling, Xbox Blood Gulch, CE Death
Island, a deliberately invalid MCC tag-index fixture, then Ruby `a10` in one
process. Each valid phase required a new simulation epoch, an exact delayed
output marker and a complete rendered capture. The invalid map was refused
and the later MCC load recovered. Independent review checked the complete
logs, expected refusal, marker ordering, settings, captures and normal exit.
The runtime verifier additionally passed 83 negative controls for missing or
echoed outputs, extra acceptance events and changed protected bytes.

Four earlier attempts are retained as failed/incomplete evidence. Two Ruby
attempts exposed runner command-length and repeated-log-observation problems;
an earlier transition collector could not encode one received character.
The other Ruby baseline attempt asserted on a stale script-node index after
revert. Its MCC checksums are valid. The runner had requested a save from a
temporary console expression that remained asleep during capture; another
baseline run reprinted that saved expression's marker after restoration.
Source inspection shows finished console threads are deleted and unreferenced
syntax is collected. This is a baseline console-expression lifetime finding,
not evidence of a CRC regression, and is retained for Stage 7 investigation.
The final paired recipe issues standalone save commands and waits outside
the game before checking completion. No production workaround was added.

Nine sequential game attempts were stopped; all five final accepted runs
exited normally. Their generated caches, including those from failed attempts,
were removed (6,908,018,688 bytes). All six original Xbox/CE fixtures, three
tested Workshop maps and Mercury's two resource files were rehashed unchanged.
The 142 inactive save/support/capture files (338,155,372 bytes) were archived
losslessly to `build/stage6/inactive-runtime-evidence.zip` (9,989,586 bytes).
Every archive member was hash-verified before removal of the loose copy;
the adjacent manifest records original paths. Original maps and saves were
not modified. Disposable synthetic pytest fixtures, duplicate map copies and
test/probe binaries were removed; useful logs, proof sources and the baseline
build remain. There are no generated game caches or active owned game processes.
About 39 MB remains under `build/stage6`, primarily the 27 MB baseline build,
10 MB verified archive and 1.4 MB of CI evidence. The final release remains
usable under `build/windows`.

These checks establish bounded native loading, save/revert/core behavior and
map-family transitions. They do not establish full campaign, multiplayer,
live split-screen, Android-device or browser-map compatibility. At this Stage 6
completion point, Milestone 7 implementation had not begun.

### Platform CI and completion

Both push workflows passed on implementation revision
`71dcfe6dd74c59fd8ab2f54d8b9fa112e04d1a36`, attempt 1, without CI repairs or
reruns. The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42390735879)
passed Linux readers/validators (1,782 checks, sixteen skips), 38 separate
ASan/UBSan preview checks, Windows adapters (1,983 passes and seventeen skips
across invocations), and 36 software-GL meter renders.

The [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42390735872)
passed all four debug platforms. Linux's main invocation passed 2,498 checks
with 27 skips, the twelve existing boat expected failures and seven passing
subtests; all its pytest invocations totalled 2,814 passes and 52 skips, with
the same expected failures/subtests. Ordinary and full-LTO ELF/COFF
light-storage checks and the Windows corruption negative control passed.
Android built its guest, host and debug APK and passed touch, gyro and MCC
importer checks. Web passed page/server tests, browser startup without retail
assets, and native Chromium/Firefox WebRTC checks with and without configured
packet loss. These are transport checks, not real-map multiplayer qualification.
Main-only release/profile jobs were skipped; the Windows release was built
and exercised locally. Counts from overlapping jobs are not unique test totals.

CI logs, metadata and local log hashes are retained in `build/stage6/ci`.
Artifact metadata names the exact revision; CI binaries were not downloaded
or independently rehashed. Final documentation review checked 145 local links,
the new section anchor, named functions and test syntax. Production remains
confined to four MCC-owned files, with no shared-source, vendor, build-graph,
protocol or map changes. The combined review found no further justified change
within this milestone. Final evidence records both successful checks and the
retained baseline finding; Milestone 6 is complete.

## Stage 7: scripts and state lifecycle

Stage 7 starts from `42176d5423cf6430b29ac39549a034a8feebb7bd` in the same
dedicated worktree. The preserved Windows release has SHA-256
`5f3f449c13fddf4198157bd94fed9cdc1fad02290ec34d966c3d420d0aa998c7`.
The initial checkpoint, campaign, names, progress, scripts, syntax and thread
selection passed 229 tests before production edits. Private commands, extracted
sources, logs and hashes are retained under `build/stage7`.

### Justified changes and boundaries

Two production files change, both MCC-owned. No map name, package ID, checksum
or asset fingerprint selects either behavior. No shared hook, native Xbox/CE
implementation, serialized format, network message or protocol version changes.
OpenCE version 41 and MCC protocol 64 remain unchanged: valid resolution and
checkpoint behavior are preserved; the correction rejects malformed saved data.

[`mcc_scripts.c`](../port/linux/game/mcc_scripts.c) had two copies of seven
MCC function mappings, the vibration alias and campaign fallback. The private
`mcc_script_resolve` now owns that mapping. Preparation still runs before MCC
activation and may use native fallback; public runtime lookup retains its
loaded-MCC/null guard and does not gain fallback. A recognized vibration alias
whose rumble target is absent still returns `NONE`, even if the native table
contains the original alias. Thirty-six source-based before/after traces match
exactly, including function IDs, rewritten call/predicate IDs, refusal and lookup
counts. The scripts/parameters/syntax selection passed 188 checks before and
after. This removes duplicate maintenance and drift risk; no game-speed or
frame-time improvement is claimed.

[`mcc_checkpoint.c`](../port/linux/game/mcc_checkpoint.c) now rejects a zero
object identifier in `mcc_checkpoint_units`. Native `datum_try_and_get` treats
an identifier of zero as an unused slot. A freed header can retain a bounded
datum pointer and unit type; matching a saved handle's zero salt therefore
does not establish a live unit. The old MCC preflight allowed that record
through. Native `game_state_image_accept` then copied the incoming image and
MCC names before the real grenade restore rejected it. This is an MCC
checkpoint-admission defect; fixing its preflight benefits every MCC checkpoint
using the format and requires no native change.

The new regression extracts the real native array layout, array validation,
datum deletion/lookup and grenade snapshot/restore. It deletes slot 0 while
slot 1 remains live, producing a legal high-water count of two and one active
entry, then recomputes both MCC CRCs. On the old code, generic validation and
MCC preflight succeeded, acceptance failed, and live bytes/names changed;
campaign restoration was not reached. The focused baseline had 59 passes and
that one intended failure. With the one-condition fix, the malformed image is
refused before copying anything, while a valid salted unit restores normally.
Stale-salt and out-of-range-pointer controls also pass. Checkpoint, grenades and
thread checks passed all 105 cases. Names/campaign and unrelated pool validation
remain adapters in this fixture; this is not a claim to validate every native
checkpoint allocation or every runtime state.

### Ownership reviewed and retained

The combined review examined the following routes before final checks. Sound
code was retained where removing it would lose an established contract or where
no practical benefit was demonstrated.

| Owner and route | Retained contract and evidence |
| --- | --- |
| `mcc_scripts_prepare`, `mcc_script_find`, `mcc_script_call_valid` | Preparation and guarded runtime resolution have different fallback rules. Unsupported names and malformed calls still refuse. Exact resolver traces and scripts/syntax tests cover the changed route. |
| `mcc_parameters_prepare`, `mcc_hs_release_arguments`, `mcc_hs_release_thread` | Prepared signatures/lexical owners are separate from live saved stack arguments. Publication counts clear on preparation failure/unload; return, cancellation, deletion and overflow release held object-list references. Existing recursion/sleep/overflow tests are retained. Full-capacity local-global reuse is not established by this audit. |
| `mcc_script_threads_prepare`, `mcc_hs_threads_activate`, `mcc_script_threads_dispose` | Reserved CPU storage owns extended frames; disposal restores the previous array and arena bytes before releasing MCC storage. Save-version bridges and address checks remain necessary. Thread lifecycle and checkpoint tests exercise these contracts. |
| `mcc_checkpoint_validate`, `mcc_checkpoint_restore` | Pre-copy validation and post-copy restoration are separate responsibilities. Preserve format/version, fixed saved-pointer addresses, CRC and bounds checks; the unused-unit correction closes one demonstrated preflight gap without a speculative transaction rewrite. |
| `mcc_campaign_validate`, `mcc_campaign_restore`, `mcc_campaign_dispose` | Validate the full supported snapshot before publishing campaign fields; restore owned engine globals and effects at lifecycle boundaries. Existing snapshot-version and malformed-state coverage remains. |
| MCC effects, game speed and progress | Keep host authority, reliable host-packet/size/stale-tick checks, map-owned restoration and progress journal's valid-prefix recovery with refusal to append after corruption. Inspection did not establish safe further trimming. |
| `mcc_object_names_*`, native name dispatch, `mcc_cache_tags_unload` | Extended names activate only for MCC scenarios beyond the native 512-name table. Deletion must scan aliases even for an object whose native name is `NONE`; checkpoint restoration intentionally restores mappings while restart/unload clears them. |

Named-object coverage grew from seven to sixteen passing cases. It checks
complete snapshots at 513, 640 and 32,767 names, output sentinels, refused
null/length restores preserving mappings, unnamed aliases, native restart
clearing, repeated unload, and partial cleanup with the loaded flag already
false. Four private negative controls caused nine expected failures while two
unaffected controls stayed green. The actual cache-unload function is extracted,
but other owners are inert there; the loader's failure funnel was source-reviewed.
These tests do not replace the native transition checks or prove network play.

### Shared console syntax finding: investigated, fix deferred

Three baseline runs reproduced an accepted core load followed by an expired
script-node assertion: Xbox Blood Gulch, CE Death Island and Ruby's Rebalanced
`b40`. Each saved inside a sleeping temporary console expression, let that
expression finish, waited externally through at least two collection
opportunities, then loaded the core. The assertions and abnormal exits are
retained as defect reproductions, never counted as passing gameplay checks.

Native `hs_runtime_initialize` stores threads in saved CPU state; MCC extended
threads use their reserved saved arena. Temporary console syntax lives outside
that saved state. `hs_thread_main` deletes a completed console thread, and
`hs_runtime_update` later runs `hs_node_gc`, which removes nonpermanent nodes.
Restoring the saved thread can therefore restore references to already-collected
syntax. A ten-case private proof uses the actual allocator, interpreter,
collector and MCC thread capture/restore. Native and MCC fail after collection,
with or without slot reuse, but resume the sleeping expression before collection.
Permanent scenario nodes and completed standalone commands are controls. Parser,
core admission and networking are outside that isolated proof; the native runs
establish the real admission/assertion sequence.

This is a shared engine ownership defect, not an MCC-only compatibility need.
Discarding all saved console threads would also remove working replay behavior;
retaining all temporary nodes would consume the bounded syntax pool. A correct
fix needs a defined shared lifetime/admission design for syntax and strings,
saved-image compatibility, and comparison with upstream OpenCE's checkpoint
contract. No network-version requirement has yet been established by this
investigation. Preserve the user's crossplay requirement while resolving those
questions; do not introduce an MCC-only bypass or claim the defect is fixed.
The scoped investigation is complete, and that shared correction is explicitly
deferred. The user confirmed this deferral before starting Milestone 8; it does
not block the remaining MCC refactor. The evidence needed to reopen it is a reviewed ownership design with
legacy/MCC replay, collection, slot reuse, save compatibility and relevant peer
agreement tests. It is separate from Milestone 8's compatibility exceptions.

### Final validation and completion

The combined production/test/reader-guide review found no further justified
change within this milestone. The final local regression selection passed
**2,108 tests**, with nineteen skips and the twelve unchanged strict boat
expected failures. Four additional real MCC/CE reader/resource checks passed.
The Windows release built with the existing portable/release/PGO-off/LTO-off
configuration; a post-review rebuild required no work. Existing Expat import
warnings remain. Executable SHA-256 is
`5786761be690e78678c97ce8c3dc241934d30d33a49db524efc3d2315e7956f3`.

The baseline and current releases each passed standalone save/revert/core-load
checks on Ruby B40 and Mercury Falling from their installed Workshop folders.
Each saved a changed campaign flag, proved the subsequent mutation, accepted
the saved image, confirmed the restored value and advanced another 150 ticks.
The current release also passed native `map_reset` on both MCC maps, Xbox
Blood Gulch and CE Death Island: a fresh low-tick epoch and 150 further ticks
distinguish a restart from merely continuing the old simulation. These restart
checks do not establish reset semantics for every campaign field.

One current-build process traversed Mercury, Xbox Blood Gulch, CE Death Island,
an invalid MCC tag-index fixture and Ruby A10. The invalid map was refused and
the later MCC load recovered; each valid phase produced fresh simulation and
a rendered capture. Inspection of the four captures confirmed coherent scenes
and the expected HUD where visible, not exhaustive visual equivalence.
All nine successful runtime checks exited normally. The three baseline console
assertion reproductions above are separate failing gameplay cases. Runs were
sequential, hidden and used isolated saves; no multiplayer gameplay is claimed.

Independent verification checked exact commands/results, map and executable
identity, paired settings, fresh epochs, normal exits, failure diagnostics and
all eight successful 20 MiB save/core images' protected MCC blocks. The three
reproducer cores retain their identities and hashes; the Ruby reproducer also
has valid MCC CRCs. All 188 host-only verifier controls passed. An initial
checker incorrectly required restart cues at exactly tick 1 and assumed CE's
launch path was its saved scenario name. Source inspection and the authenticated
CE tag index established the actual contracts; the corrected checks retain
strict epoch/order/identity requirements. Both checker versions and the initial
failed verification are preserved. No game rerun or production change was used
to hide those checker assumptions.

All twelve owned game processes stopped. Their 72 generated map-cache files
(9,210,691,584 bytes) were removed after each attempt. The 113 inactive
save/support/capture files (429,544,430 bytes) were archived losslessly to
`build/stage7/inactive-runtime-evidence.zip` (6,732,992 bytes). Every member and
loose file was hash-verified before removing the loose copy; the manifest records
original paths. Useful logs and reproducible proof sources remain available.
Final cleanup removed 431,648,191 bytes of disposable synthetic fixtures,
test binaries and the six copied map/resource inputs; earlier proof-binary
cleanup is recorded separately. All eleven original map/resource inputs were
rehashed unchanged. About 44 MB remains under `build/stage7`, principally the
27 MB baseline build, 6.7 MB verified archive, proof sources, manifests and CI
logs. No generated game caches or active owned game processes remain. The
current release stays usable under `build/windows`.

Both push workflows passed on implementation revision
`f8e621acf6505e1a414e4e52142c7efcbb7b61a9`, attempt 1, without repairs or reruns.
The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42394316405)
passed Linux readers/validators (1,795 checks, sixteen skips), 38 separate
ASan/UBSan preview checks, Windows adapters (2,032 passes and seventeen skips
across invocations), and 36 software-GL meter renders.

The [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42394316106)
passed all four debug platforms. Linux's main invocation passed 2,498 checks
with 27 skips, the twelve existing boat expected failures and seven passing
subtests; its nine pytest invocations totalled 2,814 passes and 52 skips, with
the same expected failures/subtests. Ordinary and full-LTO ELF/COFF storage
checks and the Windows deliberate-corruption control passed. Android built
the guest, host and debug APK and passed signing, touch/gyro and importer
checks. Web passed Node/page tests, asset-free browser startup and native
Chromium/Firefox WebRTC checks with and without configured packet loss.
These are transport checks, not real-map multiplayer qualification. The main-only
release job was skipped; Windows release was validated locally.
Overlapping job counts are not unique test totals.

Complete job logs, revision/attempt metadata, artifact metadata and local hashes
are retained in `build/stage7/ci`. Upload digests were matched to job logs;
CI binaries were not downloaded or independently rehashed. Final documentation
review checked 134 local links, the Stage 7 anchor, seventeen named functions,
test syntax and the two-file MCC production boundary. The final resulting
implementation, tests, integration and reader documentation were reviewed;
no further justified changes remain within this milestone. Full campaigns,
real-map co-op, live split-screen and physical Android/browser-map gameplay
remain unverified by these bounded checks. Milestone 7 is complete with the
shared console correction explicitly deferred. At that completion point,
Milestones 8–9 had not begun.

## Stage 8: compatibility exceptions and known gaps

### Retained HUD selectors

The 21 structural profiles in
[`mcc_hud_layout.inl`](../port/linux/game/mcc_hud_layout.inl) remain unchanged.
No simpler replacement was justified by the inspected authored fields or parent
usage. This is a retention decision with explicit limits, not proof that the
table is the only possible implementation or that every unknown layout intends
the wider MCC safe frame.

The fresh production-visitor audit covered **670 HUD components across seventeen
installed MCC maps**, with 495 recognized components. Inputs were seven stock
MCC maps and ten Workshop maps, including Ruby's Rebalanced A10/B40/D40,
Mercury, Descent and Quarantine. Every retained profile had an installed match:

| Profile label | Matching components |
| --- | ---: |
| assault rifle | 17 |
| master rounds | 23 |
| empty weapon/vehicle layouts | 210 |
| cyborg | 15 |
| cyborg_mp | 15 |
| warthog driver | 15 |
| passenger_2 | 15 |
| gunner | 15 |
| scorpion | 3 |
| passenger_4 | 11 |
| passenger | 9 |
| passenger_3 | 9 |
| pistol | 19 |
| master plasma | 15 |
| grenade counter panel | 36 |
| banshee | 8 |
| ghost | 4 |
| rocket launcher/needler | 26 |
| sniper rifle | 10 |
| shotgun | 13 |
| master fuel | 7 |

These labels describe evidence; names do not select the frame. The audit
extracts the current C visitor and checks every fingerprint against an
independent bounded numeric inventory. All 670 bitmap-name controls retained
their fingerprint. All 670 private root-anchor perturbations changed it, and
all 495 recognized components became unknown. Original map files were opened
read-only, and the private tag arena was compared byte-for-byte after all
private mutations were restored. The regeneration script, input identities, source hashes,
compiler invocation and per-component metadata are retained under
`build/stage8/hud`; run `audit_profiles.py` there using the recorded stage1
environment. These controls establish classification coverage and sensitivity,
not how hypothetical edited maps ought to render.

Seven combinations of tag group, root anchor, the set of scaling flags and the
set of scales occurred in both recognized and unknown components. Examples
include stock gunner geometry versus Ruby B40's shadow gunner, and stock master
plasma versus Ruby's master sword. That evidence defeats this particular
aggregate selector; it does not disprove every richer authored-field rule or
establish unknown layouts' intended margins. Parent inheritance also propagates
an already classified marker: `mcc_hud_weapon_frame_begin` cannot supply the
initial decision for a populated root without a parent.

`mcc_hud_plan_prepare` fingerprints validated pristine bytes once at load time,
before normalization. The ownership plan isolates mutable aliases; temporary
plan arrays are freed on success or failure. Runtime-owned markers and converted
data are released by failed-load/unload cleanup. Authored padding is cleared
before the converter writes its own marker. Drawing remains guarded by loaded
MCC tags. The sole weapon-frame caller brackets rendering with begin/end, without
an intervening return or goto; both boundaries clear transient state. No new
lifetime defect or cleanup edit was justified by this review.

A separate retained exception is `mcc_hud_message_frame` in
[`mcc_hud.c`](../port/linux/game/mcc_hud.c): top-left messages at normalized
offset `(0, 60)` use native safe margins. This recognizes stock placement
independently of replacement fonts or icon art; it is not an explicit authored
safe-frame flag. Integer coordinate conversion can map distinct raw positions
to that same normalized offset. A custom author choosing that geometry with
different intended margins remains indistinguishable. Its marker has the same
MCC runtime ownership and cleanup as other converted placements.

Existing public coverage includes 26 actual-visitor layout cases for geometry,
name independence, excluded fields, malformed references, bounds and read-only
hashing. Normalization tests cover a positive grenade profile, a two-pixel
authored near miss and aliased mutable blocks. Eleven stock-message cases cover
both coordinate units, nearby positions, other anchors and erasure of a seeded
marker. Together with the fresh all-profile audit, no material test gap was
found that justifies duplicating all authored profile layouts in tracked tests.
Runtime/rendering results are recorded separately below; classification
checks alone do not validate visual intent.

Remove or reduce either layout exception only after a demonstrated authored
field or usage rule expresses the intended margin and preserves recognized
layouts, nearby custom layouts, parent ownership and independently centered
components. Check renamed resources, bounds, aliases, failure cleanup and
MCC-to-Xbox/CE transitions, with actual rendered comparisons on Ruby's
Rebalanced, Descent and other affected maps. Rarity or line count alone is not
evidence for removing a profile.

The bounded adjacent-selector review distinguished these exceptions from
semantic contracts. `mcc_hud_multiplayer_text` selects
`ui\multiplayer_game_text`, string 100, because the existing score-hint consumer
in [`game_engine.c`](../source/game/game_engine.c) reads that exact resource and
index; resolving its MCC formatting argument is not a per-map bypass. Script
function/global names identify language operations, controller-entry ownership
identifies glyph roles, and structure offsets, bitmap flags and child anchors
describe validated formats. `mcc_hud_title_bounds` applies its non-wrapping
title policy to authored flags/alignment, without selecting Warthog Rally or a
160-unit box. No adjacent map-name/checksum dispatch or justified selector trim
was found in the inspected HUD/script paths. This bounded result is not a
repository-wide audit; the broader combined review remains Milestone 9.

### MCC-wide script admission

The installed Quarantine map (`2963835242`, SHA-256
`5b89b330291e7cd2a5c098b2c379aee3335e1349bc85aa1677638883b26ac8cf`)
contains 114 scripts and 19,331 syntax nodes. Its continuous `life_lost_0`
contains one valid, permanent, zero-argument `ai_deselect` call. Native map
policy deliberately excludes that operation, so the previous build disabled
the entire script. This was an MCC language-compatibility gap, not malformed
map data or evidence that Xbox/CE's policy should change.

The existing native `ai_scripting_deselect` service performs real selection
clearing when AI is initialized, including the native debug record/idle-look
cleanup. An eighteen-line addition to `mcc_scripts.c`, plus its private ID,
exposes that service through the existing MCC registry and validator. It adds
no persistent state, save fields, shared hook, map-name selector or no-op.
Existing loaded-MCC guards and unload ownership apply. Xbox/CE's allowlist,
adjacent `ai_select` refusal and client console authority remain unchanged.

MCC protocol advances from 64 to 65 because previously disabled authored
gameplay now executes. OpenCE stays at 41; packet layouts and the camera
envelope versions remain unchanged. The composed admission regression compiles
the complete actual server dispatcher and both envelope implementations:
MCC 64 is refused before the ordinary join handler with or without a camera
wrapper; MCC 65 reaches it with MCC capability. Camera-only requests do not
gain MCC access, and legacy control joins retain their path. Removing the
inner revision check makes the negative control fail. This routing harness
uses an observed join sink; separate existing actual-handler tests cover
decoding, tokens, bans, slots and session authorization.

Four focused resolver/admission cases failed before the adapter and pass
afterward. The final focused selection passed 121 checks, including real-source
native clearing and inactive-AI behavior, malformed calls, unload guards,
unchanged native/client policy and three deliberate-fault controls. The combined
review found no further necessary production change or justified HUD trimming.
It reviewed the final implementation, integration, tests and reader guides;
no shared `source/` implementation changed in this milestone.

### Runtime and local validation

The reviewed Windows release builds with the unchanged portable/release,
PGO-off/LTO-off configuration. Its SHA-256 is
`816055002eeb8f5434ec78d5dad817e5397d443e8ccb58e66d9d852e755cafbb`;
existing Expat import warnings remain. The local regression selection passed
2,123 tests, with nineteen skips and the twelve unchanged strict boat expected
failures. Four additional real MCC/CE reader/resource checks passed.

Matched baseline/current standalone runs on Workshop Ruby B40, Descent and
Quarantine each advanced at least 1,800 ticks and produced a fresh complete
capture. Baseline Quarantine deliberately records `passed=false` for its exact
script refusal, despite completing the observation; the current run has no such
refusal. The other five runs pass and all six exit normally. Visual inspection
found coherent scenes and no apparent HUD placement/clipping regression in
these views. Ruby used 640x480 and Descent 1280x720; this is bounded rendering
evidence, not pixel-identical animation or proof of all authored layout intent.

The baseline/current two-peer Quarantine comparison used separate saves and
authenticated host consoles. Both runs first established real cooperative mode,
two living players, fresh simulation, completed authored introduction, untouched
start device (`game_started=false`), positive lives and false end/cinematic
flags. The only assigned scenario value was `lives=0`, with immediate readback.
The baseline still had false end/cinematic flags after 1,000 additional ticks.
The current host reached `game_ended` at logged tick 1,770, the unique authored
`one_time_cinematic` marker at 2,640 and continued to 2,730. Source inspection
places `ai_deselect` after that unique marker and `ai_erase_all`, without an
intervening sleep.
Both peers continued and exited normally. This demonstrates the authored branch
after condition injection, not player death, life decrement, a full campaign
or a full synchronization verdict.

Two earlier baseline setup failures are retained. The first lacked stock
Carousel, which native network fast setup loads before choosing the requested
MCC map; adding one verified private input copy resolved that prerequisite.
The second stopped waiting for a living second player at 45 seconds. Quarantine's
authored co-op introduction lasts 1,430 ticks, and native cooperative spawning
holds extra players during a cinematic. A 75-second wait accommodates that
existing behavior. Neither failure required a production change or was relabeled
as a gameplay pass.

One current process traversed Mercury Falling, Xbox Blood Gulch, CE Death Island,
a bounded invalid MCC fixture and Ruby A10. All four valid phases produced a
fresh tick epoch, 150 further ticks and a complete capture. The invalid map was
refused, returned to the menu and did not prevent the subsequent MCC load.
Visual inspection confirmed coherent scenes and the native HUD where visible;
Mercury and Ruby were cinematic views. The process exited normally and its
unique invalid fixture was removed after containment/content checks.

The newly reached authored `garbage_man` calls the existing native
`rasterizer_decals_flush` operation, whose documented effect is to flush all
decals. The current branch emits two permanent-decal deletion diagnostics during
that operation, before `ai_deselect`. Source inspection traces them through the
native cache flush and decal deletion callbacks; removal is consistent with
the command's stated effect, while permanent bookkeeping/logging remains
inconsistent. These are the only new unique error/warning classes in the
baseline/current branch comparison, on the host; no new client class appears.
They are explicitly retained findings, not an assertion that the logs are clean.
A separate baseline control waited 1,800 ticks, called only authored
`garbage_man`, then advanced another 150 ticks. Neither diagnostic existed
before that call; both appeared once afterward, reproducing the same native
behavior without the new adapter. The control completed and exited normally,
while retaining `passed=false` for the baseline's original script refusal.
No shared decal implementation was changed. Repeated flush bookkeeping and
longer-term rendering effects remain outside this bounded attribution test.
Independent verification covers twelve sessions and sixteen stopped processes,
with no forced termination or nonzero exit. It checks executable/map identity,
settings, ordered commands and response frames, tick epochs, complete captures,
all log sinks, actual failure verdicts and diagnostic inventories. Fourteen
isolated negative controls reject altered outcomes, identities, missing responses
with echoes retained, hidden fatal messages, missing transitions, partial captures
and an incomplete flush control. `archive_ready` means evidence integrity and
inactive processes; it does not turn a known refusal or warning into a pass.

No old-checkpoint migration is established:
a checkpoint created when a continuous script had been disabled is not proven
to acquire its missing thread on restore. The supported admission path being
tested here is a fresh map load. Shared console-syntax and boat corrections
remain explicitly deferred; Milestone 9 has not begun.

### Platform validation

The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42399542277)
passed on implementation revision `99341deebf53cafdb82c9c901dfa31091a7b85a0`,
attempt 1: 1,796 Linux reader/validator passes with sixteen skips, 38 separate
ASan/UBSan preview checks, 2,037 Windows adapter passes with seventeen skips
across invocations, and 36 software-GL meter renders.

The initial Build workflow exposed one Linux compilation failure in the new
camera admission test: its helper had static linkage after a public external
declaration. Windows' MSVC-target compiler had accepted it. The one-word fixture
correction matches that header; no production code or runtime binary changed.
Ten targeted camera/MCC protocol checks passed after correction, and a
Linux-target front-end control reproduces the old error and accepts the fix.
The original failed job/logs remain in the evidence.

The [corrected Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42400612375)
passed all four debug platforms on `15d2ef13096751ba266d8845f18b4caae2f217e8`,
attempt 1. Linux's nine invocations totalled 2,824 passes and 52 skips, with
the twelve existing boat expected failures and seven passing subtests.
Windows storage/linkage checks, Android guest/host/debug-APK and importer/input
checks, and web Node/page/startup and Chromium/Firefox native WebRTC checks
passed, including configured packet-loss transport controls. These are not
full real-map internet/co-op or physical-device gameplay qualification. The
main-only release job was skipped; Windows release was validated locally.

The repair changes only the camera fixture; exact revision comparison proves
production, MCC workflow YAML and its test inputs unchanged. That file is
outside the MCC workflow's path filters and invocations, so its earlier green
result is reported on its actual revision rather than attributed to the repair.
Overlapping job counts are not unique test totals. Complete logs, revision and
attempt metadata, the original failure, revision proof and artifact metadata
are retained under `build/stage8/ci`. All six Build artifact upload digests
match their logs; binaries were not downloaded or independently rehashed.

### Final review, cleanup and completion

The final combined Stage 8 implementation, integration, tests and documentation
were reviewed before the required checks. The CI-discovered fixture correction
was reviewed and its affected local/platform checks rerun. The final document
check validates 170 local links, eight named symbols, Python syntax and the
MCC-owned production boundary. No additional code change or speculative
profile trimming is justified by this milestone's evidence.

All sixteen owned game processes exited normally. Their 96 generated map
caches, totalling 12,280,922,112 bytes, were removed after each attempt. The
462 inactive save/support/capture files (490,433,888 bytes) were archived
losslessly to `build/stage8/inactive-runtime-evidence.zip` (38,048,459 bytes).
Every archive member and loose file was hash-verified before loose removal;
the manifest records original paths. Cleanup removed another 439,092,050 bytes
of disposable fixtures, generated bytecode and seven private input copies.
All 26 original map/resource inputs, including the full seventeen-map HUD audit
corpus, were rehashed unchanged after cleanup. No generated map caches remain.

The cleanup gate checks session result and supporting-evidence hashes, resolved
path containment, linked ancestors and process identity. Windows reused one old
test PID for an unrelated application process; cleanup first refused it, then
verified its different executable and later creation timestamp without stopping
that process. Both failed preflight logs and the identity proof remain available.
Thirty-two private cleanup safety controls passed. About 79 MB remains under
`build/stage8`, chiefly the 38 MB verified archive, 27 MB baseline build, CI logs
and reproducible metadata/source evidence. The current Windows release remains
usable under `build/windows`.

Milestone 8 is complete. Its remaining limits are explicit: no full campaign,
full multiplayer synchronization, old-checkpoint migration, live split-screen,
physical Android or browser-map gameplay qualification; no correction of the
shared boat, console-syntax or native flush bookkeeping behavior.

## Stage 9: combined review and extended validation

This milestone reviews the resulting branch against original main
`db1697c9b0c46289596f22213a5bef8ee327be24`, including all earlier production
changes, tests, build integration and reader documentation. The shared boat
and console-syntax corrections remain explicitly deferred. No additional
game-code change is justified by this review; the demonstrated corrections
below affect validation tooling and a stale documentation statement.

### Combined review and boundary coverage

The resource directory sort had no ordering-dependent consumer; complete
name scans and duplicate refusal remain. Bitmap cursor correctness follows
from checked original handles and complete fresh-handle clone groups, with
reuse appending nothing. HUD planning snapshots shared owners before writes
or pointer publication, and temporary planning storage is freed on failure
and success. The remaining tag pass clears meter metadata after placement
normalization. Checksum callers preserve their distinct framing and seeds.
Script resolution retains preparation/runtime separation; checkpoint
validation rejects freed slots before copying live state. AI deselection
uses the existing native service through the MCC registry.

The final pass also inspected relevant activation, failure, unloading,
worker-join and GPU-before-pixel cleanup routes. These ownership boundaries,
required stubs, malformed-input checks and distinct regression cases remain
necessary. There is no production `source/` change across this refactor.
There is no new map-name, map-checksum or package-specific gameplay bypass.
The 21 structural HUD exceptions and separate message-frame rule retain the
evidence and removal conditions recorded in Stage 8. Existing optional HUD
artwork CRC selectors identify two known replacement sheets, after format
and size checks; they are not map selectors. Bridge tests use mocked CRCs,
so they do not independently verify those original artwork fingerprints.

This reconciles the navigation inventory in `CODINGSTANDARDS.md` with the
actual review. "Partial" identifies a bounded inspection, not a known defect
or a claim that the remaining implementation has been audited.

| Boundary | Changed and reviewed across the branch | Reviewed and retained; remaining limits |
| --- | --- | --- |
| Selection, loading, packages and resources | Resource ordering/checksums and HUD metadata conversion | Cache load/unload, validation worker teardown, relevant handle/root-alias checks and bitmap access reviewed. Catalog, loading UI, package/identity/SHA implementations and remaining parser/converter behavior are partial or unaudited. |
| Scripts, parameters and object names | Resolver, thread checksum, MCC AI deselection | Syntax, parameters, runtime/thread ownership, name tables and game speed reviewed. Full-capacity local-global reuse and every legacy interpreter route remain unestablished. |
| Checkpoints and campaign state | Checksum and unused-unit preflight | Campaign snapshot/lifecycle, script effects and progress journal reviewed. Individual campaign evaluators/rendering are partial; older disabled-script checkpoint migration remains unverified. |
| Explicit packages and progression | None | Full manifest, map-catalog and Android importer audit remains outside this review. Their automated checks are separate evidence. |
| Weapon-authored cameras | None | Existing camera/MCC admission-envelope composition reviewed as a network dependency; camera implementation not deeply audited. |
| Networking and names | MCC protocol version and combined manifest | Protocol, network adapter, names synchronization and relevant shared admission/cleanup routes reviewed. Workshop metadata and every native transport implementation not deeply audited. |
| Player, inventory, movement, damage, boats, seats and recordings | None | Boat initializer/native force consumers and checkpoint grenade lookup dependency reviewed. Remaining gameplay modules not fully audited. |
| Rendering, HUD and audio | Bitmap conversion/access, HUD ownership/layout and redundant normalization | Texture registry/bridge, relevant HUD frame/glyph cleanup and native decal-flush attribution reviewed. Geometry, shader, particles, sprites, audio/Vorbis and remaining HUD draw behavior not fully audited. |
| Menus and pause | None | Implementation audit remains outside this review; existing executable tests remain in the regression selection. |

No further production trim is supported by these findings. Removing required
family boundaries or retained exceptions merely to reduce code would lose
supported behavior. Private review notes are retained under
`build/stage9/reviews/`.

### Validation-tool corrections

Failure injection into the real `mcc_native_smoke.run_case` and `main`
reproduced seven adverse cases: kill failure, wait timeout, console-close
failure, stdout-only fatal diagnostics, and survivor/unknown/exception
completion from a case. Previously, caches or staged executables could be
deleted despite uncertain child exit; a close error bypassed shutdown and
init restoration; stdout-only failure could be reported as success.

Cleanup now requires observed exit or no launched child, uncertain completion
stops subsequent cases and preserves staged binaries, close errors retain
diagnostics while shutdown/restoration continues, and all four log sinks
contribute to the verdict. Results retain PID, output path and shutdown state.
Two normal-completion controls accompany the seven adverse cases. These tests
use fake processes and literal fixture bytes; they do not leave live games.
The before-fix log records the adverse behavior; the two normal controls
initially failed only because the new shutdown-result fields were absent.

The focused suite passed 56 tests, with one unavailable file-symlink privilege
skip. Two earlier sandbox-denied junction setup failures were retained and
the complete selection rerun with native Windows operations available.
`port/linux/NETCODE.md` also corrects one stale current MCC version from 64 to
65; historical version statements remain unchanged. OpenCE remains 41.

### Local and platform validation

Implementation revision `f25c4886a0ee1567b8b3808eee4bd8b4e1e3e6be`
passed the combined local selection: 2,193 tests, 24 skips and the twelve
unchanged strict boat expected failures. The selection includes all MCC test
modules, MCC harness cases, frontend tests, CE cache readers, Workshop/startup,
camera admission, network-comparison and HUD threshold checks. Four additional
installed Ruby B40/CE reader, geometry and resource checks passed. Windows
release was already up to date; the wire-format manifest matches its record.
Local skips include unavailable file-symlink privilege, opt-in assets,
Linux discovery and POSIX process-lifecycle checks; the four selected asset
checks were then run with explicit inputs. The document check covered 232
local links, eleven named symbols, changed Python syntax and the combined
MCC-only production boundary.

Both fresh workflows passed on that exact revision, attempt 1:

- [MCC map validation, run 42403748356](https://github.com/chestahh/GulchCE/actions/runs/42403748356):
  Linux reader/validator 1,802 passes and nineteen skips, plus 38 sanitizer
  passes; Windows adapters 2,046 passes and seventeen skips; 36 real
  software-GL meter-render checks.
- [Build, run 42403748394](https://github.com/chestahh/GulchCE/actions/runs/42403748394):
  Linux, Windows, Android and web debug builds passed. Linux recorded 2,824
  passes, 52 skips, twelve boat expected failures and seven subtests across
  its invocations. Windows storage, Android touch/gyro/importer, web startup,
  Node and Chromium/Firefox native-WebRTC checks passed, including configured
  packet loss. Branch release jobs were intentionally skipped; the local
  Windows runtime build is release.

These selections overlap and are not a unique-test total. Nine decoded job
logs, ten job records and six artifact metadata records are retained under
`build/stage9/ci`; all six artifact IDs, revision identities and upload digests
match their logs. No CI binaries were downloaded. Build and transport checks
do not establish physical Android play, browser-map gameplay or an official
upstream-executable crossplay result.

### Repeated loading, timing and memory

Four sequential, matched Windows sessions completed 64 valid loads and sixteen
deliberately invalid MCC loads with actual menu recovery. Each process repeated
Mercury Falling, Xbox Blood Gulch, CE Death Island, refusal/menu and Ruby A10
four times: one warm-up and three measured cycles. Original main and the
current build ran in both orders. All valid phases advanced at least 150 fresh
logged ticks; every process closed normally. Workshop originals stayed in
place. Renderer, 640x480 resolution, high-resolution HUD, 30 FPS cap, audio-off
settings and SDL DLL matched; captures were disabled during measurement.

The original-main executable remains SHA-256
`f1ec0dca876aa3cf746a4095ac9115cc026902f4679d1c3941deb20928c85d56`;
the unchanged current game executable is
`816055002eeb8f5434ec78d5dad817e5397d443e8ccb58e66d9d852e755cafbb`.
Milestone 9 changes validation tools/documentation, not that game implementation.
Private Win32 measurements use the owned process handle for private memory,
working set and handle counts. A read of the source-defined game-state header
confirms each actual map; it performs no process write, scan or save operation.

Six measured samples per build give the following median seconds and observed
minimum–maximum ranges. These include command queueing, log publication and
approximately 100 ms observer cadence; they are not engine-only loading timers
or statistical confidence intervals.

| Boundary | Original main | Current |
| --- | ---: | ---: |
| Mercury first logged tick | 13.567 (12.239–15.926) | 13.916 (11.657–14.467) |
| Xbox first logged tick | 1.613 (1.574–1.626) | 1.634 (1.583–1.680) |
| CE first logged tick | 2.035 (2.019–2.061) | 2.007 (1.984–2.072) |
| Ruby A10 first logged tick | 13.449 (11.796–14.949) | 14.606 (11.990–15.127) |
| Exact refusal/menu diagnostic | 0.444 (0.434–0.459) | 0.443 (0.430–0.451) |

Time order materially affects the result: original-main Mercury/Ruby medians
fell from approximately fifteen seconds in its first process to twelve in its
last; the second current process also became faster during its sequence.
Neither the first apparent improvement nor the pooled Ruby increase isolates
a build effect. The Xbox median difference is below observer cadence and its
ranges overlap. These observations demonstrate no clear attributable loading
regression, but do not statistically establish speed gains or performance
parity. File-cache state and scheduling were uncontrolled.

The first pair's measured menu-memory growth was 1.49 MiB on main and 10.11 MiB
on current, which prompted the reverse-order repeat. That pair grew 12.45 MiB
on main and 2.86 MiB on current. Current's observed menu maximum was 563.75 MiB
versus main's 564.72 MiB; Mercury maxima were 1,220.06 versus 1,223.80 MiB.
Some boundaries decreased, both builds released hundreds of MiB between MCC
and menu, and handle counts declined. There is no repeatable current-only
retention excess. This is not a universal plateau, attribution of every
retained byte, GPU-memory measurement or proof against long-duration leaks.

Logical converted-tag storage is separately bounded and identical on every
repeat: Mercury adds 7,152 occupied arena bytes and Ruby A10 adds 6,832, chiefly
the required HUD ownership copies. Bitmap counts and pixel bytes are unchanged.
Temporary planner arrays are freed; retained copies belong to the map arena's
unload/decommit path. Windows commits the same fixed 64 MiB arena in both
builds, so these occupied-byte deltas are not equivalent extra PrivateUsage
and do not explain the larger process-memory variation.

The first collector attempt remains failed: it incorrectly required a second
Xbox precache-start message, although a cached load does not emit that message.
The corrected observer verifies actual scenario identity/checksum from the
fixed native header and fresh tick epochs. Thirty-seven helper controls and
96 receipt-invalidation controls passed. Full trajectories, source identities,
raw logs and limitations are in `build/stage9/cycle-summary.md` and
`cycle-verification.json`; the initial failure was not relabelled successful.

### Saves, restarts and co-op findings

Current Ruby B40 and Mercury passed standalone save/revert, core save/load and
map-reset checks. Each restore proves a saved boolean's true–false–restored
round trip and at least 300 further fresh simulation ticks. Independent
decoding validates the retained 20 MiB checkpoint/core images and their MCC
footer/thread checksums. Xbox Blood Gulch and CE Death Island reset controls
also passed. These checks use completed standalone commands; they do not
snapshot a sleeping temporary console expression or close the separately
deferred console-syntax lifetime defect.

The first Mercury attempt correctly stopped when its checkpoint image changed
before the planned revert. Both images remain retained. The original helper
called this an auto-save, but the evidence establishes replacement, not its
writer. Removing the optional idle window between capture and revert allowed
the retry to verify the same captured image; all post-restore tick checks stayed.
The helper did not overwrite the live checkpoint with its retained copy, and
no authored script was disabled for that retry.

Extended current/current Mercury co-op passed admission, two living-player
checks and 1,800 further ticks. The unchanged strict network comparator passed
with sixty paired seconds and 1,799 on-foot drift samples, no threshold breaches
and normal exit of both peers. Nonzero drift remains recorded; this is not a
claim of bit-identical views or full campaign compatibility.

Ruby B40 exposed a separate **pre-existing shared co-op checkpoint-recovery
failure** in the OpenCE Crossplay profile. Both current/current and
original-main/original-main runs fail the unchanged strict comparator after
team-wipe reverts, with sufficient samples (1,031 and 1,271 respectively).
An additional current idle-input run also fails after reverts with 744 samples;
the finding is not an insufficient-sampling verdict or restricted to bot combat.
Both peers stay connected, but the client lacks a player-unit binding while
the host has a positive-vitality unit. The logger's `alive` field represents a
unit binding, not a direct client health measurement.

In the current bot run, mismatches span 21 and nine consecutive paired samples,
approximately twenty and eight seconds. Three samples is the failure trigger,
not their total duration. The baseline also crosses that threshold; subsequent
raw logs still disagree after their clock offset exceeds the comparator's
pairing tolerance. These runs prove the failure class predates the refactor;
they do not establish identical recovery timing or severity. A final one-shot
`(players)` conditional also failed in the current cases; it tests unit objects,
not session membership or continuous absence throughout its wait.

A final aim-only control completed 900 fresh ticks (1110–2010), using only
right-stick test input. Its strict verdict is also failed, with 840 drift
samples and no insufficient-sampling problem. The host reverted at tick 2012,
just after that window; fifteen paired samples then disagreed from 2040 to
2461, approximately fourteen seconds through shutdown. An earlier isolated
mismatch at 1920 did not reach the unchanged threshold. Thus the sustained
failure in this run occurred during the final confirmation wait, not within
the completed 900-tick window. The absent final unit-count receipt and failed
judge remain recorded; this is not an ordinary-co-op pass.

The responsible shared `network_coop`, `network_objects` and
`network_distributed` implementations are unchanged across this refactor.
Source inspection finds the live-unit recovery/replacement operations guarded
by `third_person_session_active`; both peers' recorded profile is
`opence_crossplay`. As documented in `NETCODE.md`, that profile retains the
existing checkpoint behavior and does not execute the separately versioned
recovery extensions. This supports a shared profile/recovery classification;
the exact stale-handle mechanism remains an inference because these logs do
not expose every object handle. It is distinct from console-syntax ownership.

No map bypass, profile change or guard broadening was made. Preserving OpenCE
crossplay and feature isolation precludes silently importing those extensions
into this cleanup. A separate shared-engine contract/version review is needed
before repairing or qualifying this recovery path. This is a retained failure,
not an MCC refactor fix or full co-op checkpoint compatibility sign-off.

### Legacy mixed-build checks and evidence audit

Original-main/current pairs passed on Xbox Blood Gulch and CE Death Island
in both host directions. Each completed 1,800 fresh post-admission ticks and
61 paired seconds with the unchanged strict comparator, normal process exits,
and no reported divergence or comparator problem.

| Map family | Host build | On-foot drift samples | Maximum position difference |
| --- | --- | ---: | ---: |
| Xbox | Original main | 1,822 | 0.079467 |
| Xbox | Current | 1,828 | 0.079051 |
| Custom Edition | Original main | 1,496 | 0.159609 |
| Custom Edition | Current | 1,518 | 0.079756 |

These establish bounded mixed-build behavior on the identified OpenCE 41
paths. They are not tests against an official upstream executable. Original
and current MCC peers were tested separately within their own protocol
versions; the earlier MCC version bump intentionally prevents mixed admission.

Independent verification binds every runtime session, raw logs, source
snapshots, settings, binary identities and supporting files. The final ledger
contains nineteen sessions: thirteen positive observations and six retained
failures (the first collector, the replaced Mercury checkpoint, and four Ruby
co-op runs). Its coverage-completed flag is true; its overall runtime-pass flag
is false. Archive readiness establishes integrity and process inactivity,
not compatibility. Additional support files are explicitly identified as
preservation-only where they received hashing rather than semantic decoding.
The state/pair verifier passed 37 negative/positive controls, the inventory
merger twenty, the frozen state/pair helper twenty and the cleanup gate 41.
These controls supplement the cycle controls; they are not gameplay samples.

### Final review, cleanup and delivery limits

The final combined implementation, integration, tests and reader documentation
were reviewed before the required regression checks. The resulting smoke-tool
correction received focused failure injection, the full local selection and
both fresh CI workflows. Subsequent changes record the completed runtime
observations and limits; they do not modify the tested implementation, workflow
or test inputs. Both workflows exclude documentation-only pushes, so their
successful results remain attributed to `f25c4886a`. The final document check
validates 232 local links, eleven named symbols, changed Python syntax and
the combined production boundary. No further game-code change or speculative
trim is justified by the review.

All 28 owned game processes exited normally. Their 168 generated map caches,
totalling 21,491,613,696 bytes, were removed after each run; none remain.
Cleanup removed 7,102 verified disposable fixtures and private input copies,
totalling another 440,218,858 bytes. No original maps or saves were removed.
The 1,097 inactive runtime save/support files and fifteen literal test-save
fixtures (745,741,325 bytes combined) were preserved losslessly in
`build/stage9/inactive-runtime-evidence.zip` (7,455,234 bytes). Its manifest
records original paths and SHA-256 values. Every archive member and loose
source was verified before removal of the loose copy.

The separate final delivery check reverified archive inventory, CRCs, sizes
and hashes, all 309 remaining loose runtime evidence files, exact session
inventory, process inactivity, missing disposable files and the cleanup
receipts. All fifteen original map/resource inputs and four executable/DLL
records were rehashed unchanged after cleanup. Roughly 29 MB remains under
`build/stage9`, including the archive, raw runtime/CI logs, small fixture-source
evidence and the original and final verification/cleanup receipts. Required
baseline and current builds remain at their existing locations.

Milestone 9's review and planned validation work are complete. The all-passing
runtime acceptance criterion remains unmet for Ruby co-op recovery; that
shared path is left unchanged under the standing crossplay and feature-isolation
constraints. Its original-main reproduction establishes a pre-existing failure
class, not parity of severity or proof against an indirect timing regression.
The branch is ready for delivery review with that explicit exception, not
unconditional compatibility sign-off. Full campaigns, live split-screen,
physical Android, browser-map gameplay, official upstream-binary crossplay and
long-duration leak/performance qualification remain unverified. The explicitly
deferred shared boat and console-syntax corrections remain unfixed. No pull
request description or merge is part of this milestone.

## Stage 10: shared checkpoint recovery investigation

### Confirmed defect and ownership

The game implementation remains unchanged at `175c2e945` while this
investigation adds tests and documentation. This is a shared replica-lifetime
defect: restoring an accepted checkpoint can revive a unit under its existing
full object handle. It does not depend on an MCC tag layout, filename, package
or map-specific script. The responsible implementation is the shared
`network_coop`, `network_objects`, `network_distributed` and `network_damage`
path. No MCC bypass, production refactor, changed default or protocol bump
has been introduced.

The host's object tracker records handles, so a restored living object can
remain indistinguishable from the previously advertised object. In default
Crossplay, `distributed_client_reuses_object` accepts the same handle and
definition. `distributed_client_apply_change` can make a living copy dead,
but does not rebuild a dead copy as living. `distributed_handle_unit_state`
then refuses that dead unit before player attachment. A new living sample
alone therefore cannot restore the player's binding or inventory.

The existing camera-profile extensions request full descriptions, reconstruct
the dead copy and refresh host snapshots after restore. Their history bundles
client replica handling with host roster/replication changes and explicitly
separates agreement versions. That history does not independently prove that
every possible client-only correction would require a new version.

### Real-function regression and counterexamples

[`test_coop_checkpoint_recovery.py`](../tools/test_coop_checkpoint_recovery.py)
extracts the actual create/validation/delete, player-state, player attachment,
inventory, datum and damage-handler functions. Its
[C fixture](../tools/harness/tests/coop_checkpoint_recovery.c) supplies small
object/player pools and explicit constructor, tag, graph-effect and damage-effect
boundaries. It is not a copied network implementation, a wire-layout test or a
full packet dispatcher. Behavioral constants and flags come from their owning
sources; intentionally smaller fixture capacities remain explicit.

The default-profile case verifies the exact known handle remains dead, no
replacement construction/deletion occurs, and player/control bindings remain
absent. The existing optional profile reconstructs and rebinds in this bounded
case. Ordinary controls check healthy objects, authoritative death, initially
bound corpses, different identifiers, malformed descriptions before deletion,
allocation failure, repeated restoration, bounded retry, carried weapons and
riders, and restored inventory/ammunition/grenades. Deferred inventory coverage
establishes missing-weapon replay, not timestamp freshness.

A second case captures a valid killing-blow payload while the first life is
bound, delivers it through the actual damage validators/handler, reconstructs
and rebinds the unit, then delivers the identical saved payload again. The
handler reaches the kill-effect boundary for the restored life. This is an
accepted-order handler reproduction; it does not claim an actual socket packet
was reordered in Ruby. Normal current-life kills must still execute, while
unknown handles and malformed numeric/tag/node data remain refused.

Both unresolved outcomes have narrowly typed, strict expected failures with
dedicated native statuses and exact markers. Compilation errors, unrelated
assertions and unexpected exits remain failures. Correct recovery returns
success and becomes strict XPASS, requiring review of the expected failure.
Eight deliberate mutations of generated function copies confirm the tests
detect lost validation, identity checks, attachment, weapon/rider preservation,
deferred inventory application and damage replay. Production files are never
mutated. The new suite is included beside the existing checkpoint tests in
the Build workflow.

### Ruby runtime observation

The unchanged Windows release executable has SHA-256
`816055002eeb8f5434ec78d5dad817e5397d443e8ccb58e66d9d852e755cafbb`.
Its matching PDB GUID/age and relevant types/global offsets were frozen in a
small receipt. Two hidden, isolated peers loaded Workshop Ruby B40 from package
`2915693904`, used aim-only inputs, completed 900 fresh admitted ticks, and
continued for at least twenty seconds. Both recorded `opence_crossplay`.

A bounded external observer used only the owned process handles. It read
full object identifiers, death flags, player bindings, inventory and supporting
replication state without modifying or suspending the game. Unchanged ticks,
player bindings, object headers and two matching semantic reads reject observed
races; raw reads and gaps are retained. The decoder received independent source/
PDB review and fourteen host-only controls. This is not atomic observation
across machines and does not directly trace packet handles or executed branches.

In `ruby-crossplay-observed-v2`, full handle `0xe53a02cb` was initially alive
and bound on both peers, then died. After logged host reverts at ticks 2095
and 2956, the host restored that same handle alive and bound. The client
retained it as a dead, known object with no player binding. A witnessed
client death-state true-to-false transition supports receipt of an alive
state; an initially false flag would not establish that. Host-only state
caches are not interpreted as received client packets.
The affected slot is the client's own local player, as recorded by its
player name and local-player index; the full player datum also matches.

At the first matched restoration observation, the host unit carried weapons
`0xe53d02ce` and `0xe53e02cf` with grenade counts `[4, 0]`; the client's corpse
had no weapons and `[0, 0]` grenades. Twenty-seven matched observations show
the same-handle mismatch across both restores. Their aggregate 31.33-second
span includes another host death and must not be described as one continuous
living-host mismatch.
The two sampled episodes contain 23 observations spanning 17.024 seconds
and four spanning 2.629 seconds, respectively.

The observer accepted 912 host and 904 client samples, retaining state changes
and one-second heartbeats. Nine host and seventeen client transient samples
were rejected; the maximum gap between accepted samples was below 0.226 seconds.
The unchanged strict comparator reports **FAIL**, with alive-state divergence
at ticks 2100 and 2971 and no sample-insufficiency problem. Evidence collection
completed; recovery acceptance did not. Both processes exited normally.
An earlier sandbox attempt failed at process inventory before launching any
game and remains recorded separately.

The separate `ruby-camera-profile-control` used the same binary, map and
input seeds with the existing `gulchce_third_person` session profile, camera
policy `forced_off` and third-person preference disabled. Both peers' explicit
environment settings are retained; the generated config file retains defaults
and is not itself proof of the active overrides. No packet-version capture
is claimed. After the authored revert at host tick 2104, the client first
had the same live handle bound with an empty inventory, then both exact
weapon handles and `[4, 0]` grenades approximately 0.110 seconds later.
The strict comparison **passed** with 1,155 drift samples and no reported
divergences or problems. It observed one revert, not full campaign recovery.

The control accepted 772 host and 767 client samples, with thirteen/eighteen
explicit race gaps and maximum accepted-sample gaps below 0.226 seconds.
Both processes exited normally. Complete frozen file inventories, hashes,
source receipts and raw decoded reads passed verification for both sessions.
Six replay/receipt controls supplement the fourteen observer controls.
The result supports the existing reconstruction mechanism but does not
resolve its old-message lifetime hazard or justify changing profile guards.

### Review and validation of the regression additions

The combined test implementation, CI wiring, runtime helpers and documentation
received primary and independent review. Review corrected two drifted fixture
constants by extracting owning definitions, added initially bound-corpse and
live-to-dead controls, and ensured a future correct duplicate rejection becomes
strict XPASS. Source/PDB review checked the observer's actual layout and rejected
interpretation of host caches as received client state. No production change
or speculative reduction is justified by these findings.

Windows validation covers the new recovery suite, existing checkpoint and
inventory tests, camera policy/wire/protocol/director checks, MCC protocol/
checkpoint/save/network checks, the comparison judge and co-op wipe controls.
The new suite contributes 35 passes and two strict expected failures.
The broader selection finished with 236 unique passes, five platform skips
and those two expected failures, after retrying 27 existing account-link tests.
Those initially failed while generating non-CP1252 source comments, then on
MSVC's standard-`sscanf` deprecation diagnostic. Python UTF-8 and a private
compiler wrapper defining `_CRT_SECURE_NO_WARNINGS` let all 27 execute and
pass; no production or unrelated test source was changed. All failed and
passing run logs are retained. This is an aggregate of the selected checks
and the targeted retry, not a claim that the first command passed.

The tests and workflow addition are committed as `f1a0d6e1b` and pushed to
`mcc-code-refactor`. Both fresh workflows passed on that exact revision:

- [MCC validation, run 42419694409](https://github.com/chestahh/GulchCE/actions/runs/42419694409):
  Windows adapters, reader/validator checks and hidden HUD-meter rendering.
- [Build, run 42419694396](https://github.com/chestahh/GulchCE/actions/runs/42419694396):
  Linux, Windows, Android and web debug builds and their required checks.
  The Linux log explicitly includes the new recovery test in the combined
  camera/checkpoint command: **74 passed, 2 expected failures**. Its broader
  suite reports 2,507 passes, 27 skips, twelve existing boat expected failures
  and seven passing subtests. The web job passed Chromium/Firefox smoke and
  room checks and the native WebRTC harness with both browsers. These are
  transport checks without retail-map internet gameplay. Release steps are
  skipped on this branch by the existing workflow conditions.

The final documentation check validates 68 local links, fourteen anchors,
twelve named source functions, changed Python syntax, the CI test selection
and unchanged production scope/version constants. Subsequent documentation
commits do not change the tested implementation or test inputs; the workflows
exclude documentation-only pushes. Milestone 10 still lacks an enabled repair
and its required post-fix gameplay matrix, irrespective of successful builds.

### Evidence preservation and cleanup

All four owned game processes exited normally. Their 24 generated map caches,
totalling 3,070,230,528 bytes, were removed after the runs. A manifest-bound
cleanup then removed nine verified input/binary copies and 86 generated test
binaries, totalling 277,757,525 bytes. The user's original maps, Workshop inputs,
required builds, existing worktrees and saves were not removed.

All 156 inactive runtime save/support files and 132 generated test-evidence
files were archived losslessly before removing their loose copies. Their
97,565,831 bytes occupy 1,141,882 bytes in
`build/stage10/inactive-saves-and-test-evidence.zip`, SHA-256
`e9223e41f761769ea011651e2b66a5b2eeb504594ffa9943b3fd7900d76dce57`.
The manifest records original paths and hashes. Exact ZIP inventory, CRCs,
sizes and SHA-256 values were verified before removal and again afterward.
Raw memory records, runtime logs, source/PDB receipts and failed observations
remain available; `data/gamestate.txt` was explicitly retained as a diagnostic.

Primary and independent cleanup review and 23 guard controls precede the
applied cleanup. The final verifier checks unchanged originals, retained loose
files, missing disposable targets, exact runtime inventories resolved through
loose files plus archive members, and all four stopped PIDs. All seventeen
original map/resource/executable records remain unchanged. Approximately 21 MB
remains under `build/stage10`, including the archive and useful diagnostic
evidence. The integrity result passes while the aggregate runtime gameplay
result remains false; archival success does not erase Ruby's default-profile
failure. The earlier Stage 9 records and archives remain untouched.

### Compatibility decision and remaining repair work

This section records the investigation's earlier compatibility decision.
The user subsequently lifted the old-client crossplay constraint for
Milestones 10 and 11; the fix below uses explicit agreement separation.

The authoritative upstream check on 7 November found the relevant shared
recovery modules unchanged between pinned OpenCE 41 commit `c507cb26b` and
then-current upstream `fc9deb0b1`. Official build 334 identifies the pinned
revision, but no upstream executable was run for this investigation; no new
claim of runtime crossplay qualification is made.

Simply removing profile guards is not justified. Recreating the same handle
does not distinguish its new life from earlier damage or inventory messages.
The source dispatcher deliberately lets ordinary reliable/unbatched damage
bypass its per-kind stale gate. Object changes have no corresponding lifetime
fence. An empty reliable damage message therefore cannot advance that gate;
the existing reverted event only stops script sounds. Ordered host delete/create
alone does not repair these old-client semantics, and broad refresh would also
detach healthy units and disturb seats and inventory.

A narrower client-only correction using fresh authoritative descriptions and
local lifetime tracking remains a possible research direction. It has not
been proved safe against stale creates, old kills, pending inventories or
same-tick events, and must retain the old host's resend limit. It cannot repair
an unchanged old client's receive implementation in the reverse host direction.
This is a specific unresolved contract, not a claim that all client corrections
are inherently incompatible.

At the end of this investigation, Milestone 10's repair remained **open** under
the crossplay condition. Proceeding required either a demonstrated compatible
lifetime/recovery contract or an explicit scope decision permitting updated
peers with agreement separation. The user subsequently chose the latter, as
recorded below. The original failures and meaningful regressions remain;
the investigation's tests and optional-profile observation alone did not
establish default recovery, all-map compatibility or completion of the
post-fix join/rejoin, transition, platform and transport matrix.

## Stage 10: shared checkpoint recovery fix

The user authorized incompatible old-peer separation on 7 November 2026 for
Milestones 10 and 11, with Milestone 10 implemented first. This supersedes the
prior crossplay blocker, without relaxing map-family ownership, authority,
validation or cleanup. Console-syntax work remains deferred and the shared
boat implementation remains separate.

### Design and evidence

The earlier real-map observation and extracted-function failures establish
two shared defects: a host restores a living object under a full handle that
the client still owns as a corpse, and a previous-life kill can act on a
reconstructed object with that same handle. Both paths use the shared co-op
replication contract; neither depends on an MCC layout, map identity or authored
workaround. The repair therefore belongs in `network_distributed`,
`network_objects` and `network_coop`, with existing MCC-owned cache cleanup
retained through `mcc_network_new_game`.

Each distributed gameplay header carries a 32-bit checkpoint epoch. The host
advances its nonzero epoch outside saved game memory on new maps and restores.
Clients adopt only an exact, reliable, nonbatched host boundary, before any
fresh object snapshot. Message time alone cannot distinguish a restore at the
same tick; independent per-kind clocks also cannot protect a new object life
against a later-arriving reliable kill. Epoch checks precede message clock and
gameplay handling. A late joiner's reliable ready request is the narrow
exception: it may solicit the rate-limited current boundary/snapshot, but
contributes no clock or prediction evidence.

The ordered snapshot must finish before the client receives or sends gameplay.
This prevents a current-epoch kill overtaking an earlier live description.
Only the first validated description of a slot may reconstruct its old corpse;
a later same-epoch resend must preserve a newly killed unit. Full snapshots
also carry reliable ordinary inventories, including empty ones: the existing
periodic sender deliberately stops sending empties after three seconds and
cannot clear retained future-world equipment after a slow snapshot. The
existing serializer and inventory lifecycle are reused; that periodic
optimisation remains. MCC extra grenades/campaign state remain independently
owned periodic updates. The ordered end marker does not prove every object
was constructed; failed creation retries remain required.

Transient input, prediction, inventory, death, animation and queued effect
history is reset. Connection identity, current roster and object ownership are
preserved. The host immediately sends fresh descriptions; a validated live
description reconstructs a dead replica through the normal delete/construct
lifecycle, while healthy replicas remain. Host-known carried weapons and
riders retain their established detach/reattach handling. Current-roster
restoration and dead-replica recovery no longer depend on camera permission.

Inspection found another old-world producer: client projectiles and delayed
effect parts can create a fresh hit report after adopting the new epoch. The
client discards its predicted projectiles through normal deletion. The small
`effects_discard_gameplay` service uses the same validated object/damage part
classes as `effect_generate_part` and normal `effect_delete`, including later
events; harmless visual-only effects remain. Its data and lifecycle are private
to `effects.c`, so this cannot be implemented safely by manipulating effect
storage from an MCC module. Host checkpoint projectiles/effects are retained.

Base agreement 32714, MCC 66 and camera 32716/32717 separate these semantics
from earlier clients. A version field appended to the ordinary join request
is checked before admission: advertisements alone did not protect direct
joins. The default UI label is now GulchCE Standard; the stored
`opence_crossplay` token is retained for configuration continuity. This does
not claim compatibility with OpenCE 41 or an untested future upstream release.

### Implementation and preliminary evidence

Historical failures and archives under `build/stage10` are unchanged. New
private evidence is isolated under `build/stage10-fix`. The original executable,
PDB and SDL library were copied and authenticated before rebuilding. Workshop
Ruby B40 and Mercury originals remain read-only; eight bounded Xbox/CE setup
files occupy 403,621,461 bytes in the isolated data folder.

The first implementation is committed and pushed as
`6666464c12c9fcc8f446cc21a324cee6f6096a61`. Its Windows portable release build
uses `--portable --release --pgo=off --lto=off`. The retained runtime candidate
has PE/PDB GUID `693AC064-21FC-7FD0-4C4C-44205044422E`, age 1; the exact
executable/PDB hashes, nine globals and ten decoded layouts are authenticated
in `build/stage10-fix/symbol-receipt-v3/layout.json`. It was built immediately
before the commit from the same gameplay sources. CI builds the committed
revision independently. Earlier candidate receipts remain historical evidence.

The first checkpoint, recovery, retried-inventory and epoch selection passed
**82 tests**. Seven epoch tests extract the production dispatcher through its
gameplay-handler switch, preserving framing, direction, batch recursion,
simulation clocks and stale-message handling. Final gameplay handlers are
observation stubs and some entry-size constants are mocked; this is not a whole
network stack. The separate lifecycle fixture executes actual object
replacement, player binding, inventory and damage-handler paths. It passes the
two former strict expected failures as ordinary regressions.

Controls include old/new messages at the same tick, reliable old kills and
inventories, malformed boundaries and markers, serial rollover, a current-life
death followed by a late live description, outgoing-send suppression and packer
discard, and identity/build retries. An actual snapshot serializer/replay test
delays a retained healthy replica's empty inventory beyond the old three-second
resend window; its future weapons/grenades are cleared before the ordered end
marker. A 123-unit case crosses the real inventory batch limit. Actual client
tick checks keep readiness/retry work active while preventing unsynchronized
local ammo history from rejecting that authoritative inventory. Mutation
controls demonstrate that the guards, empty inventory and later-event effect
cleanup matter. Host checkpoint effects and harmless visual effects are retained.

The version/admission and related wire selection passes **91 tests**; workflow
and menu/protocol selection passes **67**. These are separate selections, not
an assertion that their totals are unique. The real join codec/field table
establishes a 348-byte structure and 349-byte encoded packet, rejecting the old
codec, missing suffix bytes and wrong explicit agreement before host admission.
Legacy fixtures now extract the owning distributed header instead of retaining
an obsolete eight-byte copy. Earlier failed runs are retained: one related-test
run still used hard-coded MCC message numbers, and initial Windows fixture
compilation exposed encoding/CRT declaration issues. Targeted corrected runs
pass; those first commands are not relabelled successful.

The first full Linux job on `6666464c1` found one further stale producer:
`tools/system_link_bots.py` still emitted the old 346-byte join payload. The
engine codec expected 348 bytes. That job recorded 2,506 passes, 27 skips,
twelve existing boat expected failures, seven passing subtests and this one
failure. Follow-up `4fdaedd66017fd17385a0d471855375837776992` updates the utility
to read the canonical agreement and append its two bytes. Its existing test
now checks their value/order and preservation of the account fields as well as
all six message sizes. The failure was reproduced locally before correction;
the bot and wire-format checks then passed. This utility deliberately emits
only system-link updates, so it has no distributed-header/epoch producer to
convert. The follow-up changes no game, build or web-site inputs; runtime
artifacts remain explicitly bound to `6666464c1`, with that two-file source
equivalence recorded, rather than being relabelled as a later build.

Primary and independent combined review covered the production modules,
source additions, all callers of the changed header filler, test fixtures,
admission/version integration, generated labels and reader documentation.
Review identified and resolved the outgoing gameplay window, empty inventory
resend limit and delayed gameplay-effect producers before the final focused
checks. Existing object ownership, failed-create retry and MCC-owned sidecars
remain. No line-count or speed improvement is claimed for this correctness fix.
The explicit costs are four extra bytes per distributed header and an 8,192-byte
per-slot recovery-eligibility array at the configured object capacity. Full
ordinary inventory snapshots add bounded reliable traffic when a snapshot is
sent. Existing resend rate limits and the periodic empty-inventory cutoff
remain; there is no new per-frame scan or perpetual empty-inventory resend.
These are required correctness costs, not a claimed bandwidth optimisation.

The first implementation's default-profile Ruby B40 and Mercury runs both pass the unchanged strict
comparison, two controlled wipe sequences and raw-memory replay. Ruby supplies
2,194 drift samples, with maximum drift 0.006164 and no reported divergences;
Mercury supplies 1,884 samples with zero measured drift. Both peers recover
living player bindings and ordinary inventory after restores. Ruby's same
client handle `0xe53a02cb`, which remained a corpse in the pre-fix observation,
is alive and bound after each observed new checkpoint epoch. The separate
host-authoritative player restores/spawns also acquire matching handles on both
peers. Ruby observes four restores in total, including authored recovery; its
three-reverts-per-minute fallback remains in the logs. Each controlled sequence
requires a new revert and 300 fresh recovered ticks, but this does not prove
every restore was immediately caused by its preceding kill command.

The retained observer performs bounded double reads with explicit race gaps;
cross-peer pairing is asynchronous, not atomic or a packet trace. Both runs'
complete inventories, source/PDB receipts and raw reads verify, with no
qualifying same-epoch dead-client/live-host window. This does not establish
the absence of all transient states. A client death-cache reset at an epoch
boundary is not evidence that an alive packet was received.

### Completing the transfer boundary

Final review found a further producer path before sign-off. Client game ticks
continue while the ordered snapshot arrives. A retained living unit can create
a projectile or delayed gameplay effect after boundary cleanup; ordinary
remove-own-object cleanup excludes projectiles. Its later impact can produce
a current-epoch report after the marker. Host ownership, weapon/rate and
geometry checks do not universally reject that report, because they do not
identify the projectile's generation. This is a source-established lifetime
gap, not a claim that it caused one of the earlier real-map failures.

The new real-function regression first fails on the surviving partial-transfer
projectile. The correction retires client gameplay effects/projectiles again
at the first validated ordered marker, then clears damage queues and provisional
prediction/input histories. It preserves restored objects, inventories and
ownership. It also preserves snapshot receive timestamps: erasing them would
let an older same-epoch inventory datagram evade the stale-message check.
`distributed_reset_checkpoint_state` now takes an explicit receive-history
reset flag; new maps/epochs clear it, snapshot completion retains it. Duplicate
markers leave current gameplay producers and prediction work intact.

The final focused selection passes **87 tests** after combined review. The
expanded recovery fixture runs actual recursive projectile deletion, damage
reset/report sending and inventory/binding paths; creation and transport remain
explicit fake-world boundaries. Its three new mutations omit producer cleanup,
omit damage reset or repeat cleanup on duplicate markers, and each fails the
behavioral assertions. The dispatcher test uses the actual common reset:
partial prediction/input state is cleared, a received inventory at tick 100
still rejects tick 99 after completion, and duplicate markers preserve fresh
work. Existing real effect selector/deletion tests cover delayed gameplay
parts separately. This is not a whole-engine delayed-effect reproduction.

The rebuilt Windows candidate has PE/PDB GUID
`FB4E538B-B88F-E0C9-4C4C-44205044422E`, authenticated in
`build/stage10-fix/symbol-receipt-v4/layout.json`. The preceding executed
candidate was frozen and hashed in `prior-candidate-6666464` before rebuilding;
its earlier runtime and CI results remain pre-final evidence. The final native
executable is a local pre-commit build from the final gameplay sources, SHA256
`66c6565fbe908a1b489389a0e68af968d26d76985a3ecec31cee60569744dc04`;
the PDB hash is
`14a48f17ffc1437b05ed02b9c16db5fbff0ed1d9ea4b78863b79cad255f796bd`.
This is not described as a clean-checkout CI native binary.

### Final platform and runtime evidence

Both exact-revision workflows for
`67968518222f3ee59361670323cfcd48486fa97e` completed successfully:
[Build 42427426028](https://github.com/chestahh/GulchCE/actions/runs/42427426028)
and [MCC 42427426029](https://github.com/chestahh/GulchCE/actions/runs/42427426029).
`build/stage10-fix/ci/final-6796851.json` and retained decoded job logs record
Linux, Windows, Android and Web builds, named test selections and artifact
digests. Linux reports **2,507 passed, 27 skipped, twelve existing boat expected
failures and seven passing subtests**; its checkpoint/camera/join selection
passes 107 and its sanitizer selection 38. MCC readers report 1,811 passes and
19 skips, with 38 sanitizer passes and 36 meter GPU renders. The Windows
adapter selections report 221 passes; 42 passes/one skip; 1,754 passes/16 skips;
and 38 passes. These selections overlap and are not an additive distinct-test
total. Android touch/gyro and importer checks, Windows COFF/LTO and corruption
controls, and Web build checks also pass. Platform success is not device or
full-campaign gameplay qualification.

Final review corrected an obsolete actor-message comment promising upstream
crossplay in `network_distributed.h`. Commit
`7ed70168ac875ce28e115d5264e8036f55a22392` changes only that comment; the wire
record and gameplay remain the implementation validated at `67968518222f`.
The follow-up also passes
[Build 42430388710](https://github.com/chestahh/GulchCE/actions/runs/42430388710)
and [MCC 42430388612](https://github.com/chestahh/GulchCE/actions/runs/42430388612),
including the same named Linux/MCC selections. Its WebRTC echoes return 320/320
for each browser; the intentional-loss controls return 689/960 for Chromium
and 714/960 for Firefox. `build/stage10-fix/ci/final-7ed7016.json` retains exact
revision/job conclusions, eight decoded logs and six artifact digests/sizes
matched to upload logs. No follow-up artifacts were downloaded or executed;
runtime artifacts retain their original revision and build provenance.

The final native candidate repeats default-profile checkpoint recovery on
Ruby's Rebalanced B40, Mercury and Xbox B30. Each passes the unchanged strict
judge, two controlled wipe/recovery sequences, normal owned-process shutdown
and independent replay of authenticated process-memory reads. The final
`ruby-default-v2`, `mercury-default-v2` and `xbox-b30-v2` receipts record:

| Map | Paired state seconds / position pairs | Authoritative drift samples | Maximum drift | Observed checkpoint epochs |
| --- | --- | --- | --- | --- |
| Ruby B40 | 58 / 92 | 1,958 | 0 | 1–4 |
| Mercury | 68 / 126 | 1,952 | 0 | 1–3 |
| Xbox B30 | 70 / 126 | 2,019 | 0.000003 | 1–3 |

Recovered player bindings and ordinary inventories are observed; none of the
replays finds a qualifying same-epoch dead-client/live-host window. B30 logs
two alive and two death sample mismatches within the unchanged judge's limits;
passing does not mean every asynchronous sample is identical. Each controlled
sequence requires a new revert and 300 recovered ticks. Ruby also performs
authored recovery, so these observations do not assign every revert to its
preceding command. Bounded double reads and asynchronous pairing cannot prove
the absence of every transient state or reveal exact packet order.

Mercury `rejoin-mercury-v2` additionally passes checkpoint capture, the first
client leaving, a replacement joining, and restoration with the current roster.
Its independent verifier pins the expected peer/name sets for all five stages,
replays both guarded reads, checks full local player/unit reciprocity and
compares shared player slots and full unit handles across peers. Player datum
salts are local: before restore the same unit `0xe4da01ac` is bound to host
player `0xec720001` and client player `0xec710001`. Restoring epoch 1 to 2
retains the replacement `S10Rejoin`, with unit `0xe4d50001`. The captured and
final 20 MiB checkpoint images match and independently pass the MCC CRC checks.
The audit is retained in
`build/stage10-fix/reviews/rejoin-mercury-v2-verification-v2.json`.

The final Ruby camera-protocol control also passes one controlled wipe/recovery
sequence, with 70 paired state seconds, 116 position pairs and 1,552
authoritative-drift samples with zero measured drift. Maximum position
difference is 0.033541 and aim difference five; one alive, death and round sample
mismatch each remains within the unchanged judge's limits. Independent raw
replay observes epochs 1–3, no qualifying stale-body window, at least 300
recovered ticks after the controlled revert at clock 2,725, and normal exits.
Both peers use `gulchce_third_person` with `forced_off` camera policy and
`HALO_THIRD_PERSON=0`: this validates the protocol profile, not shoulder-camera
rendering.

Final Xbox BloodGulch and CE Death Island controls each pass two Slayer rounds.
Every round supplies 31 paired state seconds, 62 position pairs and 930
authoritative-drift samples, with zero measured drift and no judge problems or
divergences; maximum aim difference is two within the existing tolerance.
The standalone transition control also passes Mercury → Xbox BloodGulch → CE
Death Island → invalid MCC refusal/menu → Ruby B40, with 150 fresh ticks per
valid map; its exact private fixture is removed. Independent raw-evidence
audits of both round controls and the transition control pass. This transition
check does not establish network map switching or campaign completion.

Two failed final-candidate collector attempts remain failed records. B30 v1
used `game_is_cooperative` as a network co-op gate, although stock Xbox uses
that predicate for local split-screen. Rejoin v1 incorrectly required peer-local
player datum salts to match. Its failing post-join raw pair was not retained,
so that exact failure cannot be reconstructed from raw memory. Source review
establishes that the equality gate was invalid; the v2 reads demonstrate the
valid differing salts. The narrow collector corrections pass subsequent runs
without game-code or tolerance changes. Available earlier observations, raw
reads, helpers and failed receipts are retained.

Four old/new discovery-refusal controls also pass: Standard and camera profiles
with each side hosting. These eight processes stopped normally. Their evidence
is explicitly **pre-final**, bound to the `6666464c1` candidate; the subsequent
marker correction does not alter admission code. The independent
`admission-pre-final-verification.json` audits this attribution. These runtime
checks demonstrate discovery refusal, not forced raw old-wire injection; the
source-extracted codec/admission tests cover malformed and truncated requests.

### Real-map native/browser transport

The final Web artifact is the exact `67968518222f` CI build,
`halo-web-debug` artifact **13596301952**: 14,159,156 ZIP bytes, SHA256
`af1f3217caa982a8ea138969ec0f3fc23689ee1e244b2318e759163362f8cad6`.
Its authoritative metadata, upload digest and all 24 extracted site files are
verified. It is paired with the authenticated local native build above. The
private Windows adapter uses isolated saves and browser profiles, a local MQTT
broker, real Xbox BloodGulch and at most two game instances. Retail data remains
local. Independent replay verifies raw judge inputs, source/build receipts,
bidirectional WebRTC traffic, normal native exits and 41 exported browser
save/support files per final case.

| Final pairing | Two-player samples per peer | Drift samples / apart | Mean / maximum drift | Paired cross-log state seconds |
| --- | --- | --- | --- | --- |
| Native host, browser joins through discovery | 49 | 1,330 / 326 | 0.098554 / 109.070160 | 0 |
| Browser host, native joins by supported direct invite | 49 | 1,396 / 257 | 0.125030 / 97.919212 | 0 |
| Browser host, native discovery with look-only input | 49 | 1,330 / 21 | 0.083745 / 91.926247 | 0 |

All three original judge results pass. The measured authoritative drift is
nonzero; the apart counts remain below its unchanged limit. However, log sample
phases fall outside the unchanged two-tick pairing bound in every case, leaving
zero paired state seconds and zero position pairs. These runs establish bounded
real-map transport and authoritative-drift coverage, **not cross-log state-field
equivalence**. Logs were not shifted, tolerances were not relaxed and tests were
not repeated to obtain favourable phase alignment. The native/native recovery
checks above separately provide matched state observations.

The two failed preliminary Internet attempts are preserved: native-host v1
passed gameplay/transport but failed teardown on a disappearing window; its
narrow owned-window fix was independently reviewed before v2 passed. The first
browser-host discovery attempt exited normally before joining gameplay. Its
exact quit trigger is unproved. Final direct-invite and look-only discovery
controls pass, but they use the later candidate and do not isolate that earlier
failure's cause. Their outcomes and hashes remain separate in
`build/stage10-fix/internet/final-transport-summary.json` and the independent
`reviews/internet-*-verification-v2.json` receipts.

The `67968518222f` CI additionally passes native WebRTC echo with Chromium and
Firefox at 320/320 messages each; intentional ten-percent datagram-loss controls
return 714/960 and 698/960 respectively. Local-broker map gameplay and these echo
controls do not qualify WAN/NAT behaviour or browser checkpoint rollback.

### Completion, cleanup and remaining limits

Primary and independent combined review covers the resulting recovery code,
admission, integration, tests and reader documentation across the core fix and
both follow-up commits. No implementation blocker remained after the
first-marker producer correction. The obsolete interoperability comment was corrected in
`7ed70168a`. No further simplification had a demonstrated benefit: per-slot
eligibility, reliable empty inventories and the distinction between send-state
and receive-history resets each prevent a separately covered regression.
The review and raw-evidence audits are retained under
`build/stage10-fix/reviews`; no map identity selects the recovery behavior.

At Milestone 10 completion, Milestone 11 was next under the authorized
version-separation policy: its shared boat correction had not started and the
twelve strict expected failures remained. The separate console correction was
still deferred then. Stage 11 below records their subsequently authorized and
completed repairs.

Verified cleanup is recorded in `build/stage10-fix/cleanup-plan-final-v6.json`,
`cleanup-applied.json` and `cleanup-post-verification.log`. Across sixteen
native sessions and six Internet attempts, per-run cleanup removed **234
generated map-cache files, 29,934,747,648 logical bytes**. Final cleanup removed
another **2,787 verified disposable files, 973,111,030 bytes**, covering private
map/site copies, duplicate executables, dependencies and fixture binaries.
A separately reviewed cleanup removed 270 dummy control fixtures (57,789 bytes),
retaining their sources and logs. Original maps, Workshop resources, the active
build, source and other worktrees remain unchanged.

All **3,868 archived files, 6,585,303,596 uncompressed bytes**, are preserved in
`build/stage10-fix/inactive-saves-and-support.zip`: **798,147,821 bytes**, SHA256
`4f297443ccb8432f3ca57532bea49a959ff7f972d8bd91c5cd4ba2e35f334c7f`.
This includes inactive saves and checkpoint captures, complete isolated browser
profiles and exported saves, the two preceding executed native builds and
supporting fixture evidence. Every member was verified before loose-file
removal; the archive's exact bytes were checked again before removal and after
cleanup. Diagnostics and raw observations remain available separately.
`retained-size-census.json` records approximately **952 MB** of retained evidence,
mostly this archive. No loose map copies, executables, DLLs or PDBs remain in
`build/stage10-fix`; the required active build remains outside that folder.

The final integrity pass verifies the original inputs, active build and all
sixteen native evidence inventories. All seven raw recovery replays pass after
archival; earlier incomplete or failed gameplay attempts remain failed records.
The transport audits ran before cleanup, and their bound evidence is preserved
by the separate integrity checks; no transport rerun is claimed after removing
the private map copies. See `post-cleanup-replay-summary.json` for the replay
results and their exact archive identity.

Earlier cleanup refusals remain recorded. They exposed retained diagnostics
outside duplicate manifests, changing unrelated PID-reuse observations and
210 empty save/profile directories omitted by file-derived planning. The final
cleanup controls pass **60 tests and 25 subtests**. Fresh process-identity proofs,
exact file inventories, contained nonlink paths and late archive-integrity
checks remain mandatory; the refused cleanup attempts removed no loose files.

Full campaigns, live split-screen and Android-device gameplay remain outside
these bounded checks. Existing authored HUD, navigation-point and attachment
caches are not comprehensively rebuilt from checkpoint state; complete authored
campaign rollback remains unverified. The ordered marker proves ordering, not
successful creation of every replica. MCC extra grenade/campaign sidecars
remain separate periodic updates, so ordinary inventory coverage is not a
claim about every MCC sidecar. Historical failures and archives under
`build/stage10` remain unchanged.


## Stage 11: shared boat and console lifetime corrections

Implementation, validation and verified cleanup are complete. The 8 November
request includes both Milestone 11's shared boat fix and the separately investigated console
syntax lifetime defect. The earlier console deferral is historical. Work stays
in the existing `mcc-code-refactor` worktree; original maps and saves are untouched.
The later request explicitly includes diagnosing and correcting Mercury's
host-driven boat synchronization discrepancy within this milestone.

### Shared boat producer and agreement

`update_human_boat_physics` now initializes exactly its three temporary powered
point records before the existing assignments. The native accepted-input
contract and both force consumers establish a shared producer defect. The
former MCC helper, its declaration and sole call are removed; MCC receives the
same initialized values. Other point counts, authored tags, arithmetic and seat
rules are unchanged. No persistent state or save layout is added by this fix.

The unchanged baseline `46f8792f5e5c7642a0f729af097467f56a323ccb` fails twelve
unmasked boat cases. The expanded harness also runs the actual new-physics
linear integration and position publication, in addition to the existing old
path. It reports twelve legacy failures and twelve corresponding MCC passes
against the frozen baseline producer. With the shared correction, all 64 boat
checks pass. Guard-removal proof cases have become ordinary regressions and
negative controls that reintroduce the former MCC guard; their finite/NaN,
active-control, scratch-boundary and point-count coverage remains.

The combined local boat and neighboring vehicle/seat/damage/color selection
passes 116 tests. Updated admission, camera, codec and CI-selection checks pass
87 tests. These are controlled real-source harness checks, not full game or
upstream compatibility results. The initial boat correction used shared
agreement 32718, MCC 67 and camera 32720/32721 to exclude incompatible older
peers; its wire manifest changed only
these agreement values. The previous agreements are explicitly rejected by
both discovery/session tests and the direct-join handler tests.

Private evidence is under `build/stage11-fix/boat` and
`build/stage11-fix/admission-neighbors.log`. The preserved M10 native executable
has SHA-256 `66c6565fbe908a1b489389a0e68af968d26d76985a3ecec31cee60569744dc04`.
It is the earlier local portable release built from the final M10 gameplay
sources, not an exact-revision CI build. Its executable, PDB and SDL DLL were
copied and verified before rebuilding.

### Console baseline and affected map selection

The preserved executable reproduces the console lifetime defect on Xbox Blood
Gulch, Custom Edition Death Island and Workshop Ruby B40. Each saves inside a
sleeping temporary expression, lets it finish, waits externally through at
least two collection opportunities, compiles a replacement command and loads
the unchanged core. All three accept the saved image and then report an expired
script-node error, with abnormal exit. These are failing reproductions, not
passing validation. Raw logs, exact commands and cores are retained separately.
The corrected runtime probes require replay of the captured marker, rather
than accepting mere absence of an assertion. A separate baseline Xbox run
also demonstrates overwritten sleeping literal text without any restore.

A bounded read-only survey of 152 installed MCC version-13 maps found the
three-point friction case in Mercury2 (`3021510694`) and B30 (`3512468479`).
Useful control boats in other packages have rear powered-point flags `0x08`
(lift only), while these affected boats use `0x0a` (lift and friction) on all
three points. Ruby's inspected maps contain no boat-controller tags. This
survey selects runtime cases; it does not establish loading or gameplay success.
The inspector, input hashes and authored tag/placement records are retained in
`build/stage11-fix/validation`.

### Pre-clock boat observations and synchronization failure

The original affected Workshop B30 map runs unchanged on two peers for 1,200
fresh ticks. Read-only PDB-bound observations retain 351 host and 342 client
samples, 39 paired seconds and zero paired position difference. Both processes
exit normally. The boat remains stationary, with no throttle or driver samples;
this proves finite synchronized presence, not active boat physics or driving.

Mercury's existing named boat `d1` belongs to its credits BSP. A private fixture
instead binds that name to an existing gameplay boat through three 16-bit
scenario name/index fields. Changes to the 99,205,136-byte map are confined to
those six bytes; powered-point flags, physics, vehicle placement and scripts
are unchanged. The original Workshop map and resource files are hash-verified
and untouched.
The probe lets the authored intro finish and enters its ordinary BSP transition
volume, allowing the map's own cleanup and transition ordering. Earlier forced
BSP jumps caused baseline object-cluster assertions; those setup failures are
retained and are not passing controls. A later failed setup also exposed an
incorrect fixed host/player-slot assumption; the runner now derives each
peer's local slot and verifies the shared player identities before seating.

Baseline and corrected builds each exercise both client and host driving for
1,200 ticks per role, using ordinary pad input and read-only observations.
The candidate retains finite occupied boat state, forward/reverse controls,
movement and 339 fresh host-accepted client prediction samples. Client driving
passes the unchanged boat comparison: 40 paired seconds, maximum position
difference 0.673 world units (baseline: 39 seconds, 0.468). The replacement
producer is identical to the prior MCC producer after normalizing the removed
guard/helper call to its existing zero-initialization operation.

**Both runs fail the strict host-driven position comparison.** Baseline has
three consecutive sampled seconds above the unchanged one-world-unit limit,
with a maximum of 1.550; the candidate has two, with a maximum of 1.332. Each
failure interval has a client clock approximately five or six ticks behind
the contemporaneous host and observations about 0.219 seconds apart. Nearby
wall-time observations agree much more closely, but they do not replace the
required tick comparison. One candidate failure is an exact-tick pair, so this
cannot be dismissed as nearest-sample rounding. Both original strict verdicts
remain failed. No retry for favorable timing, shifted logs or relaxed tolerance
is used to declare a pass.

The shared control initialization is established independently by the failing
legacy harness and passing corrected cases. These pre-clock runtime runs establish
preserved finite MCC control production, occupied driving and client prediction
within their observed windows. They leave the pre-existing host-driven network
position discrepancy unresolved at that revision. The later clock correction
and its separate acceptance result follow below. The fixture, runner controls,
raw observations and independent audits are
under `boat`, `runtime` and `reviews` in the Stage 11 evidence root.

### Shared recovery from small clock delays

The preceding failed verdicts remain historical failures. Further diagnosis
pairs nearby read-only observations within twenty milliseconds, solely to
measure clock offset; it does not retime boat positions or change the gameplay
comparison. The original and boat-corrected builds start near the same clock,
then acquire seven-to-eight-tick offsets around Mercury's authored BSP
transition. Five-to-six-tick offsets persist during the later driving windows.
The retained observations do not identify every loading-time contribution to
the initial stall, but they establish that the offset is real and persistent.

The responsible shared recovery contract accepts the host's reliable full
game time and never rewinds the client. Its existing handler corrects ongoing
delays only when they exceed thirty ticks. A reproduction extracting that
actual handler and the actual `game_time_update` loop preserves each five-,
six-, seven- and eight-tick delay across three hundred frames, despite reliable
host updates every second. The first-update and greater-than-one-second
corrections still work. This is a shared clock recovery defect, independent of
map format or boat data, rather than a Mercury compatibility rule.

`game_time_synchronize_distributed` records a bounded, reliable host target
outside the saved world. On a running client, the existing frame loop may
perform up to its normal number of additional ticks, retaining the thirty-tick
total frame cap. It stops adding ticks at the last known host target and never
extrapolates it. Each added tick runs normal physics/scripts, the rendering
snapshot and the distributed network callback; fractional frame time remains
unchanged. First-update and large-stall hard synchronization remain separate.
The transient target is cleared on initialization, map disposal, game end and
explicit clock rebasing. Menus, local games, hosts, held clients, pauses and
zero game speed receive no extra ticks.

The actual-source frame harness passes 47 checks: 37 behavior/lifecycle cases
and ten selective faulty implementations that compile and are rejected by a
behavioral assertion. The combined clock, boat, vehicle, co-op recovery,
checkpoint-epoch, MCC network and pause selection passes 233 tests, including
those 47. Eleven admission/codec checks pass with shared agreement 32722,
MCC 68 and camera 32724/32725. Previous agreements remain refused, including
direct joins and composed MCC/camera envelopes; the wire manifest changes
only the agreement values. The Windows portable release build succeeds with
the existing Expat import warnings. The completed platform and loaded-world
checks for this timing change are recorded below.

The pre-clock native build is preserved byte-for-byte under `baseline-clock`
with its executable/PDB/SDL receipt. A fresh `mercury-clock` fixture has the
same map bytes as the earlier name-only fixture; the historical fixture and
cleanup receipt are not overwritten.

On `6096b0b4f969ad24c4f664cf90f276593ed244ce`, Mercury passes both driving
directions with the original two-tick pairing window, one-world-unit position
limit and two-consecutive-sample failure rule. Client driving has 39 paired
seconds and a maximum position difference of 0.278; host driving has 36 and
0.343. All 1,353 raw driving observations retain finite occupied state and
the correct player/driver/passenger identities. The host accepts 325 distinct
fresh client boat predictions. Both processes exit normally.

The authored BSP transition still produces a seven-tick clock delay. The
client recovers to within two ticks in 3.192 seconds and remains within that
bound for the remaining 668 nearby wall-time observations; both driving
windows have median clock offset zero. Raw frame observations include actual
two-tick catch-up frames. These clock observations explain the correction;
the unshifted simulation-tick comparison above is the driving acceptance gate.
A receipt proves that the runner, observer, input implementation and comparator
are byte-identical between the failed pre-clock and passing clock runs.

The broad on-foot comparison remains **INCONCLUSIVE** because the driving
scenario supplies only twelve qualifying on-foot drift ticks. It has no
persistent divergence across 84 paired seconds, but one isolated position
mismatch reaches 1.073. That broad result is not relabeled as passing; the
separate boat comparison supplies the occupied-driving evidence.
See `runtime/boat-6096b0b4f-mercury-clock`,
`reviews/boat-6096b0b4f-mercury-clock-clock-followup.json` and
`reviews/6096-mercury-unchanged-method-receipt.json`.

The unchanged original Workshop B30 also passes its 1,200-tick static boat
control on this build: 342 host and 340 client observations in the fresh
window, 39 paired seconds and zero position difference. Independent replay
checks both guarded reads for 1,402 total observations and all 107 frozen
evidence files. Movement, velocity, throttle and driver counts remain zero;
this supplies synchronized static presence, not additional driving coverage.
The original Workshop inputs are unchanged in both tests. Both private
Mercury fixtures have been removed through their separate verified cleanup.

### Shared console ownership

The fix snapshots the existing syntax allocator with the saved threads and
gives live commands bounded, separately owned source text. It does not discard
restored console threads or retain temporary syntax forever. The saved block
is 918,568 bytes on the supported ILP32 targets: 32,767 syntax records, allocator
metadata, 256 root handles and 256 source slots of 1,024 bytes. Native maps keep
their existing 19,001-node allocator limit. The block fits below MCC's separately
reserved thread and checkpoint storage without enlarging the CPU or save image.
The added allocation changes the existing allocation checksum, so old-layout
saves are refused instead of being interpreted using incomplete ownership data.

Before copying an image over live state, validation checks saved allocator
bounds, live frame references, source ownership, terminated string storage and
call structure. Permanent map syntax must match the loaded map. Inactive
damaged scripts already disabled by map admission do not become a new reason
to refuse a checkpoint. The existing map command permissions are retained.
The [reader walkthrough](reading/console_checkpoints.md) documents the concrete
flow, ownership, lifecycle and limits.

The pre-clock Windows console matrix passes **14 cases**, comprising **17 normal
process exits and 28 independently recounted marker observations**. Xbox Blood
Gulch, CE Death Island and Ruby B40 each pass sleeping-literal isolation, two
distinct checkpoint captures/reverts, exact core replay in a fresh process,
and clean old-layout refusal. Ruby and Mercury additionally pass saved temporary
MCC `abs_integer` calls. Positive image operations continue for at least 150
fresh simulation ticks. Distinct process IDs, exact image hashes, monotonically
advancing tick windows and output ranges are checked; command echoes do not
count as execution. These checks include the completed console fix and saved
source-owner guard. They do not establish full campaign or every-map behavior.

The matrix used `4f645bd110cbc01ffb7a793533091ebfba84442b`. The test-only follow-up
`ea2bef9bbd9fb9c8e18a2c9b718326c99dfce7de` has identical `source/` and `port/`
trees. The local rebuild reports no work, and both receipts authenticate the
same executable, SHA-256
`74209dea04310a669735f877e47ec95e02a02daca165711083fb7ffc23e135dc`, with PE/PDB
identity `05AB2D31-6861-97E2-4C4C-44205044422E`. Runtime provenance remains tied
to the revision actually executed; this source-equivalence proof does not
relabel it as a different CI build. See `console/final-console-audit.json`.

The focused selection passes **321 tests**, including 111 console cases with
seven selective fault controls, real core-header/load checks, MCC scripts,
checkpoint/deselect coverage and nine co-op wipe cases. The latter fixture was
missing the newly called capture service in the first Linux CI attempt; its
eight compilation failures remain recorded. The corrected fixture additionally
checks capture failure and later success on native and MCC paths. Failed
capture cannot publish a new image or roster, or reach MCC capture; it clears
the valid-checkpoint flag and uses the existing wipe respawn fallback. A later
successful save publishes the replacement and permits team reversion. The
final boat, vehicle, recovery, admission and wire integration selection passes
another **290 tests**; these selections overlap and are not summed as unique
coverage.

Old-layout cores are refused with map, player count and map checksum unchanged;
the allocation checksum is the only mismatch. The earlier inherited assertion
on that mismatch is removed, retaining the diagnostic and false return before
image publication. All three runtime refusal cases continue for 150 fresh
ticks, execute another command and exit normally. Existing saves are preserved,
not migrated; a save made with the new allocation layout is required.

The allocation ledger and ILP32 fixture both establish the 918,568-byte addition.
A conservative baseline ledger sum of 18,176,716 bytes gives an upper bound of
19,095,284 bytes afterward. Within the unchanged 20,709,376-byte CPU arena,
reserving 1,054,792 bytes for MCC threads and the maximum 197,492-byte other
MCC tail, with alignment, leaves a conservative 361,804-byte gap. The ledger
includes GPU allocations and therefore overestimates CPU use. The source-bound
calculation is `console/capacity-and-source-receipt.json`.

Text ownership remains bounded to 256 slots of 1,024 bytes. Live globals and
unaligned stack values are scanned conservatively; a coincidental pointer-like
integer can retain a slot. Exhaustion reports a diagnostic instead of replacing
live text. Reclamation occurs on command compilation, not each simulation tick.
Validation covers the new syntax/source ownership and reachable frames; it does
not comprehensively harden opaque inherited evaluator payloads or every field
of arbitrary native saves.

Earlier probe failures remain separate evidence. Newline-free console output
could concatenate a marker with the next command echo; positive execution is
now checked before sending that command. An initially invalid silent delimiter
was replaced with a valid conditional print whose unused marker must remain
absent. Neither preliminary result is substituted for the final matrix.

### Map lifecycle, platform builds and final integration

The following integration results precede the additional shared clock fix;
their exact tested revision remains recorded below.

The single-process lifecycle control passes Mercury → Xbox Blood Gulch →
CE Death Island → bounded invalid MCC input → menu → Ruby B40. It arms one
sleeping command before each active-map transition and checks new commands
afterward. Independent replay verifies that all three old markers remain
absent through destination tick windows 60–630, 60–1,050 and 60–1,470, beyond
their old absolute wake bounds of 540, 960 and 1,380 respectively. Failed-load
refusal, menu responsiveness and later Ruby execution are ordered in raw logs.
All 50 frozen evidence files are verified, the process exits normally and the
owned invalid fixture is removed. The runner has eleven offline controls;
three additional independent mutations reject missing armed output, a late
concatenated marker and insufficient deadline coverage.

Final two-peer checkpoint/recovery runs each capture a fresh checkpoint,
observe at least 900 ticks plus another twenty seconds, and perform two
controlled team wipes with at least 300 fresh recovery ticks after each.
Independent raw-memory replay verifies actual live player handles and their
player attachments, inventory observations, checkpoint epochs, receipt
identities and normal process exits. Reversion uses the game's current
checkpoint, which authored scripts may replace; these runs do not prove that
each wipe used the initially retained image. The unchanged comparator reports
no divergences or problems:

| Map and session profile | Paired state seconds | Authoritative drift samples |
| --- | ---: | ---: |
| Ruby B40, Standard | 59 | 2,214 |
| Xbox B30, Standard | 81 | 2,034 |
| Mercury2, Third Person agreement with camera forced off | 81 | 2,046 |

The Mercury profile check validates the separate session agreement and shared
recovery path; the disabled camera does not establish visual third-person
behavior. Raw evidence and independent replay receipts are retained under
`runtime/recovery-ea2bef9bb-*` and `validation/*-independent.json`.

Xbox Blood Gulch and CE Death Island each also complete Slayer → lobby →
Slayer with two hidden peers and normal exits. Separate raw log slices preserve
each round's clock. Both Xbox rounds have 31 paired seconds and 930 drift
samples. CE's first round has only 12 paired seconds and 533 drift samples;
its second has 31 and 930. The original judge passes all four rounds without
divergences/problems, but CE's first round misses the independent twenty-pair
coverage gate. Its independent audit therefore records that coverage limit,
not a full-coverage pass. The second round provides the stronger CE comparison;
no retry or shifted log substitutes for the first round. These are same-map
round transitions, not disconnected-client rejoin tests.

The pre-clock test-bearing revision `ea2bef9bbd9fb9c8e18a2c9b718326c99dfce7de`
passes [Build run 42439268449](https://github.com/chestahh/GulchCE/actions/runs/42439268449)
on Windows, Linux, Android and Web, and
[MCC run 42439268534](https://github.com/chestahh/GulchCE/actions/runs/42439268534)
for readers, HUD rendering and Windows adapters. Linux's broad invocation
reports 2,631 passed, 27 skipped and seven passing subtests; all subsequent
commands in that job also complete, including 107 checkpoint/network cases
and 38 controller/HUD cases. Skipped asset-dependent cases are not represented
as runtime passes. The earlier failed fixture-compilation run is retained.

Web CI passes native WebRTC echo with Chromium and Firefox at 320/320 messages
each. Intentional ten-percent datagram-loss controls return 676/960 and 723/960
respectively. These are transport controls, distinct from actual-map gameplay.
The complete logs, metadata and source identities are retained under
`build/stage11-fix/ci`, summarized in `summary-final-ea2bef9bb.json`.

### Combined clock revision: final native checks and platform CI

The complete gameplay implementation at
`6096b0b4f969ad24c4f664cf90f276593ed244ce` passes the combined implementation
review and 462 final focused tests, with no skips or failures. These include
64 boat, 111 console-lifetime and 47 clock cases, plus neighboring vehicles,
co-op wipes/recovery, native/MCC checkpoints, epochs, pause, saved-game routing
and protocol admission/codec coverage. These overlap earlier selections and
are not added to them as unique coverage. The receipt records the exact
command, per-module counts and 128 source/test hashes.

The separately invoked `tools/test_checkpoint_timing.py` CLI is not collected
by pytest and is not one of those 462 passes. Its existing fake world still
names `game_safe_to_save`, while the extracted caller now uses
`coop_scripts_safe_to_save`; the standalone invocation fails to compile.
The caller change predates this milestone. A private one-line fixture
correction passes the timing assertions at four frame rates, and four
selective faulty implementations are rejected by those assertions. The
one-line fixture correction and explicit Linux CI invocation are now applied;
the final Windows CLI run passes at 30, 60, 120 and 990 frames per second,
including its timeout, rewind and pause assertions. The completed Linux CI log
also executes the standalone command and passes all four frame rates. The
original failure is retained in
`reviews/final-checkpoint-timing-6096.log`.

The review confirms that all twelve previously reviewed boat/console source
files are unchanged by the clock work. Local and host clock behavior remains
unchanged in the extracted-source checks; only distributed clients catch up.
The earlier 17-process console matrix and map-family lifecycle run therefore
retain their original provenance rather than being represented as reruns.
The following new paired recovery tests exercise the combined implementation
with the same fresh checkpoint, two-wipe and minimum tick-window requirements:

| Map and session profile | Paired state seconds | Authoritative drift samples |
| --- | ---: | ---: |
| Ruby B40, Standard | 60 | 2,175 |
| Xbox B30, Standard | 66 | 1,998 |
| Mercury2, Third Person agreement with camera forced off | 71 | 2,111 |

Each passes independent raw replay, live player-unit/attachment and inventory
checks, checkpoint epochs and normal exits, with no comparison problems or
divergences. Maximum paired position differences are respectively zero,
0.003742 and 0.029275. The earlier limitation about authored replacement of
the current checkpoint still applies. The Mercury camera remains disabled;
this is agreement/recovery validation, not a shoulder-camera rendering test.

The new Xbox Blood Gulch and CE Death Island Slayer → lobby → Slayer controls
each supply **31 paired seconds and 930 drift samples in each round**, with
no problems or divergences. Xbox's maximum paired position difference is
0.002449 in each round; CE's is zero. Both have zero authoritative drift
differences. Both peers have one
actual clock reset, a fresh 60–960-tick window in each round and normal exits.
These complete the twenty-pair coverage requirement independently of the
earlier limited CE run; that historical twelve-pair result remains unchanged.
Evidence is in `runtime/*6096b0b4f*` and the corresponding independent audits
under `validation/`.

The portable Windows release executable has SHA-256
`da8fa9004787ace509dddd61480f739a66e27a4c8193c953254be49bd580c76f`
and PE/PDB identity `E5251594-AC49-CA85-4C4C-44205044422E`, age one.
The build retains `--portable --release --pgo=off --lto=off`.
[Build run 42443616551](https://github.com/chestahh/GulchCE/actions/runs/42443616551)
passes Windows, Linux, Android and Web debug builds, and
[MCC run 42443616589](https://github.com/chestahh/GulchCE/actions/runs/42443616589)
passes readers, Windows adapters and 36 actual GPU-render cases. Linux's broad
selection reports 2,678 passed, 27 skipped and seven subtests; subsequent job
commands also pass. Skipped asset cases and branch-skipped release jobs are
not runtime validation. Web's native WebRTC controls return 320/320 messages
with both Chromium and Firefox; the deliberate datagram-loss controls return
710/960 with each. These transport controls remain distinct from loaded-map
gameplay. `ci/6096b0b4f969-final-receipt.json` binds fourteen retained evidence
files to these exact jobs and revision.

The test-only follow-up `cafddf5d3cd77827c93feca1c9f967ad23daed35` passes
[Build run 42448770208](https://github.com/chestahh/GulchCE/actions/runs/42448770208)
and [MCC run 42448771192](https://github.com/chestahh/GulchCE/actions/runs/42448771192).
All four platform debug builds and the three MCC jobs pass, including the
standalone checkpoint timing command in the completed Linux log. Its broad
Linux selection again reports 2,678 passed, 27 skipped and seven subtests;
MCC retains 36 actual GPU-render passes. WebRTC controls return 320/320 with
both browsers; deliberate-loss controls return 714/960 in Chromium and 706/960
in Firefox. These counts overlap the prior revision and are not additional
unique tests. The `source` and `port` Git trees are byte-identical to 6096;
runtime receipts retain their actual 6096 build provenance.
`ci/cafddf5d3-final-receipt.json` binds eleven retained metadata/log files,
independently rehashed after collection.

The current separated agreements are shared 32722, MCC 68 and camera
32724/32725. Neither old upstream crossplay nor old-layout save compatibility
is claimed. Full campaigns, every installed map, Android hardware and public
relay/NAT traversal remain outside the observed coverage.

### Internet gameplay setup and retained failures

Actual-map Internet checks use two isolated processes at a time: a local
Windows release build and the exact-revision CI Web artifact, connected through
a local broker and real WebRTC data channels. The Web ZIP is artifact
`13602867233` from Build run `42443616551`, SHA-256
`a08c85a8a7f503107753afc42df1f42f279b36aea78c8c29be3dabc9b70353ae`.
All 24 expanded site files are verified against that archive. The browser
itself is headless; the browser-host case renders its normal SDL canvas so
ordinary console key events can be exercised.

Several private test setup failures are retained rather than reclassified:

- The earlier pre-clock native cue had a space that the inherited command
  wrapper interpreted incorrectly. The corrected input is `60(sv_end_game)`.
- The earlier browser's no-window SDL mode discarded keyboard input. A real
  rendered canvas is required for this input test.
- The first rendered browser attempt typed `9sv-end-game0`, as its screenshot
  shows. Actual SDL key conversion requires Shift with the physical digit and
  minus keys. Explicit paced key presses now produce `(sv_end_game)`, with
  missing/stuck-Shift controls and a required screenshot before submission.
- A native-host attempt entered its first game before the scheduled
  twelve-second test start. A separate PDB-bound
  observation replays 849 accepted double-read samples, retaining 116 races
  as gaps. The engine completed its postgame timer and continued frames for
  over 122 seconds in the scores state, but the debug test's `started` flag
  remained false. Its automatic next-round gate was therefore never armed.
  This diagnostic was ended normally and remains failed. The input code's
  bot A presses in menus explain the early start, but the individual press
  was not sampled. Host-only `look:0` input cannot press A, and the corrected
  run records its scheduled start. A positive scheduled-start event
  before the first complete gameplay sample is now mandatory. Client input,
  shooting and all comparison thresholds remain unchanged.
- The rendered browser-host v5 run executes the verified command, completes
  both rounds and yields 47 paired seconds without gameplay divergence,
  but still **fails** the overall gate on an unhandled pointer-lock rejection.
  An inert two-tab control reproduces the exact `WrongDocumentError` while
  the canvas remains connected and `document.hasFocus()` is true. Bringing
  that canvas tab forward makes the same request succeed. The final rendered
  browser-host runs explicitly activate their owned page before Play and
  console input, and have no such rejection. No pointer-lock override, error
  suppression or game/site/WASM change is used.

All three final cases independently pass on the unchanged 6096 gameplay
implementation. Each peer has one actual round reset and 49 fresh second-round
samples spanning 48 seconds. Both the recorded and independently rerun judges
report 47 paired seconds, zero tick offset and no problems/divergences:

| Host and joining route | Client input | Maximum paired position difference | Drift samples / apart |
| --- | --- | ---: | ---: |
| Native host, browser client (`native-host-…-v3`) | `bot:1` | 0.146769 | 1,262 / 341 |
| Browser host, native invite (`browser-host-invite-…-v6`) | `bot:1` | 0.073770 | 1,316 / 314 |
| Browser host, native public-game discovery (`browser-host-browse-look-…-v6`) | `look:1` | 0.001000 | 1,316 / 8 |

All hosts use `look:0`, so these Internet controls do not demonstrate free
host movement. The first two clients retain scripted movement; the discovery
case isolates look input. Both sides retain the scripted firing behavior.
The original drift share limit remains 0.6, with at least 300 drift samples
and twenty paired seconds required. These results do not claim bit-exact
positions. Raw corrections, transient mismatches and all tolerances remain
in the receipts.

For both browser hosts, the reviewed screenshot shows the exact console
command before Enter. Game over follows after thirty simulation ticks
(1,160 ms for invite; 372 ms for discovery), before the real lobby transition
and second game. The actual renderer, physical key sequence, normal page
activation and absence of PAGEERROR/fatal diagnostics are verified. The
discovery log proves browsing and selection from the server browser; that
client receives no invite argument. All cases establish bidirectional real
WebRTC traffic, unchanged original inputs, verified saved exports, normal
native exit and closed browser contexts. They remain local-broker tests,
not public relay or NAT-traversal tests.

Evidence is under `internet/runs/` and
`validation/internet-*-6096b0b4f969-*-audit.json`. Every earlier failed verdict
is preserved. The test-only checkpoint-timing follow-up also passes CI.

### Combined review and evidence preservation

The final implementation review covers the boat producer, console ownership
and saved-image validation, client clock lifecycle, protocol admission,
neighboring MCC integration, regression fixtures and reader documentation.
The redundant MCC boat helper/hook and unreachable console disposal branch
are removed because their replacements are demonstrated. No further production
abstraction, trimming or optimization is justified by this review. Map-family
limits, permission checks, cleanup, diagnostics and meaningful regression
coverage remain. The combined source review precedes the 462 final focused
checks and final runtime/CI validation; the later test-only correction has its
own focused review and actual Windows/Linux standalone checks.

Primary and independent documentation reviews check the resulting behavior,
named symbols, local links and numerical claims against the retained evidence.
The final CI follow-up changes no game-source tree, so the runtime results
are not relabeled as executions of a different binary.

Per-run cleanup removed 504 native generated map caches (64,474,841,088 bytes).
Separate verified cleanup of the nine closed Internet attempts removed another
54 caches (6,908,018,688 bytes), preserving every other run file at that stage.
Both Mercury fixtures are inactive and removed; original Workshop maps and
resources remain unchanged. These are generated caches, not player saves.

The final reviewed plan archives **5,304 files**, totaling 11,227,077,161
uncompressed bytes, into `inactive-evidence.zip` (1,217,001,463 bytes).
Original paths, sizes and SHA-256 values are recorded. Every archive member is
verified before its loose copy is removed. The archive preserves inactive
saves/core images, browser profiles and exported saves, both executed baseline
builds and necessary binary test inputs. Another **3,221 disposable files**
(18,622,942,131 logical bytes) are removed after ownership/input verification.
That logical total includes 344 map-copy hardlinks; it is not a disk-space claim.

Final inventory accounting, including the last 54 Internet caches, releases
about **18.18 GB of allocated space**, with about **1.99 GB retained** under
Stage 11. Earlier per-run native cache removals are outside that comparison.
The retained total includes the 1.22 GB archive, roughly 756 MB of loose raw
captures, logs, source copies, executed CI Web ZIPs and other evidence, plus
the cleanup manifests and final receipts. The active native build and installed
toolchains remain outside this cleanup. Empty evidence directories are retained.

The two cleanup operations each paused on a transient process-ID check. Fresh
process queries found those IDs absent, and the same plans resumed without
weakening guards or stopping unrelated processes. The final verifier rehashes
all 5,304 archive members, 2,418 retained evidence files and sixteen protected
original/current-build records, confirms every disposal target is absent and
finds no remaining generated map cache or active game process. The Ruby, Xbox
and Mercury recovery audits and Xbox/CE two-round audits replay successfully
against archived evidence; all three retained Internet log pairs again yield
47 paired seconds without problems or divergence. These are preservation
checks, not additional game launches. See `cleanup-plan.json`,
`cleanup-applied.json`, `validation/internet-cache-cleanup-applied.json` and
`validation/post-cleanup-verification.json` under `build/stage11-fix`.

Milestone 11 is complete within the stated runtime and platform limits.
The branch contains shared fixes without map-name selectors; Xbox, CE and MCC
ownership boundaries remain intact. Old network peers and old-layout saves
are deliberately incompatible. No complete-campaign, every-map, physical
Android-device or public-NAT compatibility claim follows from these checks.
