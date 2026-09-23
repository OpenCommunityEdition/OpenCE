# Halo-only object-closure batch (2026-09-24)

This batch starts at canonical `89230ef2` on `jonas/exact-pilots` and excludes all thirteen translation units reserved for the active Claude data/ownership lane. It admits nine Halo-owned objects; it does not count library objects, fuzzy bodies, or candidate-only inline helpers as January code.

| Object | Strict target functions | Admission evidence |
| --- | ---: | --- |
| `source/game/cheats` | 14/14 | Two private names authenticated by the HCEA SHIP PDB; the descriptive string-array name replaces an address placeholder. BSS extent/layout and selected helper links agree. |
| `source/items/items` | 18/18 | The update profile owner is byte/relocation-identical. ActorCombat's hand-written `add_vectors3d` and Items' two validator copies were replaced by their genuine shared-header forms, preserving every exact sibling and ordinary links. |
| `source/interface/hud` | 22/22 | Four-byte `_default_string` owner has the January relocation destination; five genuine inline helper copies match selected providers and link in both orders. |
| `source/effects/particles` | 20/20 | January's eight-byte collision-scale bounds are external, not private; the 1,536-byte update profile has identical payload and three relocations. Duplicate random helpers in DebugMemory, Contrails and BoredCamera were retired in favor of shared-header forms; provider links pass. |
| `source/rasterizer/rasterizer_frame_statistics` | 10/10 | The private statistics block and four initialized file-scope counters now reproduce January's BSS storage class, size and offsets. Existing owner tests pass. |
| `source/sound/sound_dsound_xbox` | 39/39 | Thirteen private names are recovered from the January atlas and represented as private in source and symbol metadata. Target code/data owners and SDK/provider link checks pass. |
| `source/tool/error_geometry` | 17/17 | Items' genuine validator provider removes the prior duplicate-owner veto. All 57 target non-code owners and selected inline providers match; ordinary pair links pass. |
| `source/effects/particle_systems` | 25/25 | The shared `local_random_direction3d` inline replaces a hand-written Effects copy while preserving its exact caller. Target code/data and provider-pair links pass. |
| `source/effects/contrails` | 19/19 | Shared random-range and cone helpers replace two hand-written copies. Five private symbols are marked private in January's split metadata; all target non-code owners and selected providers match, with successful two-order links. |

## Board result

| Measure | Before | After | Delta |
| --- | ---: | ---: | ---: |
| Conservatively complete objects | 459/833 | 468/833 | +9 |
| Halo complete objects | 341/468 | 350/468 | +9 |
| Strict exact functions | 7,585/8,245 | 7,585/8,245 | 0, no regression |
| Halo meaningful exact code | 1,549,953/1,770,166 | 1,549,953/1,770,166 | 0 |
| Credited data | 2,564,059 | 2,568,800 | +4,741 |

The newly credited data is Cheats 3,201, Items 1,528, HUD 4, and Particles' eight-byte bounds. Other pinned data sections were already credited; their pins document strict identity rather than claim duplicate new bytes.

Verification: full `ninja` succeeds; the rename-stable 8,245-function sweep gains and loses zero functions; all 103 parks remain valid; the admission audit has no new contradiction (thirteen inherited contradictions remain in Claude's reserved scope); `python -m pytest -q tools` passes 1,154 tests, five skips and 26 subtests; modified C files have zero fake-match scan leads; `git diff --check` passes. Pair-link probes were run in both orders for the admitted units' surplus shared-header/SDK providers. No header, compiler-option, `point_from_line3d` guard, or Claude-reserved file was changed.

## Held, not counted

An additional tenth object was sought but not falsely admitted. `bitmap_drawing` has an unidentified, unreferenced 16-byte January data prefix; manufacturing an unused global from its bytes would violate the source standard. `path_obstacle_avoidance` is 24/24 strict but its historical BSS relabel lacks authenticated private identities. `render_debug`, `model_animations`, `actor_firing_position`, `game_engine`, and several other otherwise code-exact units emit helper copies incompatible with January's selected provider. `rasterizer_xbox_dynavobgeom` has unresolved shared-type ownership. Previously parked one-function residuals retain their zero-credit holds. The ninth-object batch should stay local under the standing requirement of at least 10,000 newly exact code bytes before pushing.
