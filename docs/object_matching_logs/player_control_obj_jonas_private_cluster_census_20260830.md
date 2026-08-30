# `player_control.obj` private-cluster fixed-point census (2026-08-30)

## Result

No production candidate was emitted. `source/game/player_control.c` remains at
41/49 strict-exact functions, with zero residual and eight unwritten target
functions. Its Git blob remains
`6b5eb6c89cc1efde8d335b01effc25be224bc901`; all 41 inherited exact owners and
the complete inherited translation-unit state are unchanged.

The remaining work is not eight independent leaf functions. January's object
proves same-translation-unit private calling conventions between the small
public wrappers and the large private bodies. Ordinary typed C cannot safely
provide one of those wrappers while leaving its private callee unwritten. With
no complete January-proven natural-C body for the cluster, admitting a partial
wrapper would violate whole-TU ownership even if that wrapper happened to
compare exact.

## Strict residual census

| target function | target padded bytes | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_000a5920` | 16 | 1 | `6e6690b65743dd4aed7c2480d704558a599ac4455958d5692dc9adff935cf92d` |
| `_code_000a5f40` | 512 | 13 | `468f32f7420ac3729550516a5d7794b91f2d906dff6809219ae92f876e932cf2` |
| `_code_000a6420` | 3456 | 130 | `4b4ad9ae985a33b5447df49d1037b7ec716d220c7a6a1544720c49a2b52c1b9a` |
| `_code_000a7300` | 1584 | 83 | `6e468bddb00f141c0c0018648fc4965e97ac383f6e688247adeab58281c6d4fb` |
| `_player_control_initialize_for_new_map` | 144 | 10 | `adb32edd759e2bbcf07d7c5b87f8a11099d2f03e8e93e465605bcbc3512994e7` |
| `_code_000a79c0` | 1568 | 95 | `490eb357b2d76f34ef5d2e72a92c21a3e625e230e3c4a0a586f9502f0d4cc845` |
| `_player_control_update` | 128 | 12 | `ca839661040e79993c2eefea0ce84594a26e1071e43e6ab956a224e7a6f94eb9` |
| `_player_control_permanent_impulse` | 64 | 5 | `85f3ed804f320738d098cbfef1b83c09ff52df74eb62555801272f1eac7c8925` |

The regenerated January target object has raw SHA-256
`7b282d76bdc4b571e5fab060391800565b94b134124742484a0f6473106c1bdc`.
The pushed-base candidate object has raw SHA-256
`07cdf5be599e6afafec5192d77b41f1e2a7660f5853ea291214124d4d4489821`.

## Private-ABI evidence

The 128-byte `_player_control_update` target enters and exits the profiling
owner, brackets the four-player loop with the collision begin/end calls, and
calls `_code_000a79c0` once per player. At that call site January pushes only
`delta_time`; the loop index lives in `EDI`. This is VC7's optimized private
relationship between two definitions in the same C translation unit, not a
public one-argument declaration that can be reconstructed independently.

Likewise, `_player_control_permanent_impulse` supplies its local-player index
in `EAX` and pushes only the two real impulse arguments before calling
`_code_000a7300`. `_code_000a5920` is a 16-byte clear wrapper whose destination
arrives in `EAX`. These are compiler-selected private interfaces. Expressing
them with `register`, inline assembly, a raw function-pointer cast, a naked
function, a pragma, or an ABI-forcing declaration would be outside House C and
would not establish ownership of the missing callee bodies.

January also owns a 1536-byte `.data` section and a small `.bss` section that
the present partial translation unit does not emit. The initial typed values
include the player-control enable flags and look-rate setting used by the
update family; the remaining data includes the profiler owner. Adding only
those globals would not be independently admissible: their observable owner
is the same missing update/private-helper cluster.

The earlier bounded probe of
`_player_control_initialize_for_new_map` remains closed. It reached the exact
144-byte/10-relocation envelope and the exact suffix, but ordinary-C spellings
could not reproduce January's initial `EDI` save schedule. The candidate and
its globals were pruned, as recorded in
`player_control_obj_jonas_aiming_unit_leaf_20260828.md`.

## Evidence harvest

All registered local worktrees, all reachable branch-tip and source-history
blobs, fixed audit/build artifact roots, and recursive stray object roots under
the campaign workspace, `C:\halo-worktrees`, and `C:\tmp` were enumerated.
The object scan covered 1,563 registered worktrees, 1,206
`player_control.obj` paths, and 1,177 unique raw object hashes. No candidate
contained a new strict-exact residual owner. The sole apparent 49/49 artifact,
`work/units-vehicle-scripting-wave-20260824/build/verified-split/source/game/player_control.obj`,
is byte-for-byte the January target above, not a compiled candidate.

The complete reachable source census contains 16 unique
`source/game/player_control.c` blobs across 26 commits. The campaign branch
sweep found 12 compiled branch-tip blobs; none improves on the retained 41/49
object and none carries a missing admissible data owner.

The following external evidence was read, but it does not justify a fresh
production spelling:

- HaloCEA supplies readable later-retail implementations for the update,
  permanent-impulse, initialize-for-map, new-unit, and one-player input family.
  Its update topology is a two-player, non-profiled build and differs from the
  four-player January target; its large input body also contains behavior and
  representation assumptions not established by this target.
- The Stian and Pastudan reconstructions corroborate the semantic function
  families, including the explicit `(player_index, delta_time)` source-level
  helper relationship, but their lifted bodies rely on raw offsets, casts,
  puns, and machine-code structure that are not admissible source donors.
- Complete local Claude/Fable lanes and their relevant documentation contained
  no later typed natural-C owner for this target cluster. Active foreign lanes
  were inspected read-only.

## HUD fallback

The ordered fallback, `source/interface/hud_nav_points.obj`, is also at its
documented fixed point: 30 exact, one 400-byte/17-relocation residual
`_hud_render_nav_points`, and one 1632-byte/95-relocation unwritten
`_custom_render_nav_point`.

The current render-loop spelling compiles to 416 bytes with the correct 17
relocations; the existing recovery ledger records the exhausted legal C89
control-flow spellings. `_custom_render_nav_point` already consumed its frozen
public-source one shot and was rejected at 1584 bytes/96 relocations. A sweep
of eight compiled branch-tip blobs found no improvement. It was therefore not
recompiled or tuned here.

## Reopen criteria

This unit should be reopened only for new source provenance or new target
evidence that supplies a complete private cluster in readable natural typed
C. In particular, another assembly-shaped lift of the target, a copied target
object, or a reordering-only retry of either frozen probe is not new evidence.
Any future attempt must own the full same-TU helper relationship and associated
data, preserve all 41 inherited exact functions, and pass the strict whole-TU
and semantic gates before admission.

## Verification

The isolated branch was configured and rebuilt directly from campaign base
`a4f1baed8d0d04526c1b6b23e942b5a2e0f9c4b5`; the canonical checkout was not
used for compilation and was not modified.

- `halobetacache_build` and `libcmt_build`: successful from clean generated
  objects.
- Strict progress: 384/833 objects, 4,808/11,060 functions, and
  610,370/2,198,102 code bytes overall; 282/468 objects, 4,641/7,574
  functions, and 597,456/1,770,166 code bytes for Halo.
- Semantic report: 470 units scanned, 4,954 functions evaluated, 4,837
  semantic exact, 4,847 accepted exact, and zero unit errors.
- `tools.campaign.gate source/game/player_control --all`: 41 exact, zero
  residual, eight unwritten.
- `tools.campaign.gate source/interface/hud_nav_points --all`: 30 exact, one
  residual, one unwritten.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object-admission audit: zero candidates, contradictions, and revocations.
- Parked-function audit: 12 active, zero stale, zero invalid.
- Python regression suite: 212 passed.
- `git diff --check`: clean.
