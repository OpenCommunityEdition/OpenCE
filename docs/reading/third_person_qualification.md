# Online third-person qualification, 28 October 2026

This records the qualification of `online-third-person` through engine
revision `8294d69476996136cfb325557dc50420dd741c25`. Read the
[camera walkthrough](third_person.md) for behavior and ownership. This is
release-candidate evidence, not a declaration of complete campaign or WAN
compatibility. WAN means peers on separate external networks.

Third person remains optional and off by default. Player Choice remains the
default host policy. Xbox, Custom Edition and MCC discovery/loading paths
and defaults remain separate. No stencil-shadow feature, cosmetic head
tracking or new controller binding was added in this qualification.

## Findings and fixes

Two connected controllers exposed a startup assertion before the game clock
existed. `remap_game_active` in
[controller_remap_game.c](../../port/linux/game/controller_remap_game.c)
now guards active-player queries. The real browser startup test includes two
neutral standard gamepads; the production-function regression failed before
the fix. Existing button layouts remain authoritative.

Repeated co-op reconnects followed by an old checkpoint restore exposed a
session-lifetime bug, also reproduced with third person forced off. Saved
player datums could replace the live roster while machine ownership tables
still described the current session. `network_coop_restore_players` in
[network_coop.c](../../port/linux/game/network_coop.c) reconciles those
datums with the host's current membership before view and input caches
rebuild. The three-line registration in
[game_state.c](../../source/saved%20games/game_state.c) is necessary because
that file owns callback ordering. The reconciliation stays in the co-op
module; it does not adapt Xbox/CE loading to MCC.

A restored unit can also reuse a handle whose client copy died. In
[network_objects.c](../../port/linux/game/network_objects.c),
`network_objects_host_reverted` immediately refreshes loaded clients through
the existing reliable object-description path. `distributed_client_reuses_object`
recreates a dead co-op copy when the validated host description says alive.
Ordinary client requests retain their rate limit and retry backoff; only
the host's checkpoint operation initiates the immediate refresh. No client
can choose membership, revive itself or dictate a checkpoint.

Experimental protocols are now **32708 for Xbox/CE and 32709 for MCC**.
The admission envelope remains mandatory, including camera-off games.
OpenCE/main and earlier camera builds cannot join these sessions. No packet
IDs or layouts were repurposed. The generated protocol manifest was updated
and checked after CI caught its stale revision.

## Completed validation

All engine sessions used isolated data/save folders and at most two game
instances. Native test windows were hidden; browser runs used headless Edge.

| Check | Observed result |
| --- | --- |
| Windows portable release | Built successfully; final focused regression run: 56 passed, including the recorded-format check. |
| Windows/browser host-policy matrix | Six real Blood Gulch sessions: each platform hosted Player Choice, Forced On and Forced Off. Each peer logged at least 45 two-player samples. Conflicting local preferences obeyed the host policy. |
| Cross-platform simulation | All six pairs passed the unmodified `tools/net_compare.py` default tolerances, with 25–43 aligned seconds per pair after startup. Large transient drift occurred around scripted relocations/respawns; these results do not mean zero divergence. |
| Windows co-op stress | Two 140-second a30 runs, Forced On and Forced Off, each with three client reconnects and two explicit checkpoint restores, plus opening-cutscene restoration. Receive latency was 80 ms with 2% simulated packet loss. No stale-roster errors or engine exceptions remained. |
| Restore timing | Comparing host/client alive state after each explicit restore found at most one one-second mismatched sample. The first matching alive sample appeared within 35 simulation ticks of each scheduled restore. Reconnecting was not needed to recover. |
| Windows/browser co-op | Real a30 session over WebRTC, save/revert at approximately ticks 240/600, continued to tick 1350. Both players were alive and third-person policy remained enforced. Default log comparison passed. |
| Native round lifecycle | Native internet UDP tunnel with private local signalling, isolated from LAN discovery: three played rounds and their postgame/lobby transitions, followed by the fourth round beginning to load. |
| CE and MCC | Two-peer CE Death Island session passed. Two-peer MCC Mercury session used different host/client package-folder names and continued after a save/revert; both players remained alive. These are focused opening-session checks. |
| True split screen | One rendered browser engine, two local players, two synthetic standard gamepads. Player 2 alone scoped; player 1 alone orbited with the reticle hidden and returned on release. Both returned to third person. Gameplay aim stayed fixed during orbit/release; turning controller 2 subsequently changed only player 2's aim. Captures were inspected. |

The combat runners combine real bot firing with synthetic reported-hit
probes (`HALO_NETWORK_TEST_SHOOT`), forced deaths, pickups and vehicle events.
Their accepted-hit counts are not exclusively physical gunfire. An earlier
gunfire-only run is retained separately in the local evidence.

[Final platform CI](https://github.com/chestahh/GulchCE/actions/runs/40619370361)
passed Windows, Linux, Android and web. The Linux primary suite reported
1,793 passed and 27 skipped; additional suites also passed. Skipped retail
tests are not runtime evidence. Web transport checks passed Chromium and
Firefox normally and with 10% datagram loss, alongside browser startup and
page/socket tests.
[MCC validation](https://github.com/chestahh/GulchCE/actions/runs/40618494706)
passed on the same engine code before the manifest-only follow-up.

The local browser gameplay and final split-screen runs used the CI debug
artifact from `80d2f0bb`; the Windows build used the same engine code.
`8294d694` changes only the recorded network-format manifest. Artifact
SHA-256: `9d69398c18634180f7c9de53c3622a28da59e6037985388b1c0c4d344317d6b4`.

## Limits, review and retained evidence

An earlier native pair reached public signalling brokers and established
the native internet tunnel, but both endpoints were on this computer/LAN.
The Windows/browser matrix used a private local signalling broker and real
WebRTC. Neither establishes independent-WAN NAT traversal or long-distance
play quality. Physical-controller testing, long campaign playthroughs and
exhaustive weapon rendering on every map family remain playtest coverage.
Shared preferences and keyboard ownership are unchanged; viewport-state
isolation does not imply independent keyboard settings for each player.

The combined implementation, source hook, guards, declarations, state
ownership, tests and documentation were reviewed before final regression
checks. No unrelated refactoring or MCC boundary changes were needed.
Negative controls rejected missing roster reconciliation and dead-unit
reuse. The browser linker caught a missing `void` declaration during
development; it was corrected before the final passing builds.

Local evidence lives under the worktree's `build/qualification/`: final
policy cases are `final-native-*` and `final-browser-*`; checkpoint stress
is `windows-coop-immediate-{on,off}`; family checks are `final-families`;
rendered isolation is `split-browser-final`. These folders contain logs,
JSON reports, inspected captures and the local runners. Earlier failing
reproducers and fixture failures remain labelled separately; their initial
summary flags are not substitutes for the final recovery-timing checks.

Disposable map copies, browser profiles, caches, test binaries and temporary
broker dependencies were removed. Inactive test saves were archived
losslessly, with original paths and SHA-256 manifests verified before loose
files were removed. Original maps and user saves were preserved. The
Windows delivery and active build/toolchain remain available.
