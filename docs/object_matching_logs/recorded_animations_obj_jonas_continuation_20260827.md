# `recorded_animations.obj` continuation wave (2026-08-27)

## Result

This wave advances `recorded_animations.obj` from 6/16 to 12/16
strict-exact functions. The object now has no unwritten function owners:
12 are strict exact and the remaining four are typed, target-sized residuals
with the target relocation counts.

The strict-exact owners account for 1,648/2,800 padded target bytes.

| Strict function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_000836c0` | 96 | 4 | `dfe89944f5a5133e3b05901bba5076f4bcda484539e5373342c7a97452e745b7` |
| `_recorded_animation_controlling_unit` | 96 | 4 | `f21618f605218178bd0239d32d5361e014e232c079b62c3efde6d64fecdcfef0` |
| `_recorded_animation_play` | 32 | 1 | `ef585acc1d2013cd265d49eaa84ecdaf199a4240338ab525e2e5415f55138fa2` |
| `_recorded_animation_play_and_delete` | 32 | 1 | `35ec6175e1878120e0ea7fdb6a81827fd940b345897631bc2401ac44c32740ea` |
| `_recorded_animation_play_and_hover` | 32 | 1 | `c11e67670227db5072be50fc141406af4628cdaabce20cdbd8b1377aed52d2a3` |
| `_recorded_animations_clear_debug_storage` | 64 | 7 | `1211ce37dc4823572de1d952fab7bec9bfd63c39ac60cad7e77b3c10d3603e2e` |
| `_recorded_animations_dispose` | 48 | 4 | `b9b64bb030325c73231a51ab101f24b1d8ce3ec37789c76d2686751f0cc07ed0` |
| `_recorded_animations_dispose_from_old_map` | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_recorded_animations_initialize` | 128 | 14 | `62d7955bddb2ac2c835aad58824e6de31cd285e0c65afd49c7325dd4cf8ed262` |
| `_recorded_animations_initialize_for_new_map` | 80 | 9 | `974d8b0cddba4e676cb8649ac9b1efbfa2e238ef8e1fe20f223dc475581781e8` |
| `_recorded_animations_update` | 560 | 34 | `d0606972324bba256754b94bd47bef61e499de94bc830df2dcbb5cff364c5f05` |
| `_render_debug_recording` | 464 | 28 | `a888551deefc05cfa73b658369687feeb95c6c0fe75561a53f9122ef938ea885` |

The retained residual owners are:

| Residual function | Target/candidate padded bytes | Target/candidate relocations | Remaining difference |
| --- | ---: | ---: | --- |
| `_code_000839a0` | 640/640 | 38/38 | normalized instruction SHA |
| `_recorded_animation_get_time_left` | 144/144 | 8/8 | normalized instruction SHA |
| `_recorded_animation_kill` | 96/96 | 4/4 | normalized instruction SHA |
| `_recorded_animation_verify` | 272/272 | 14/14 | normalized instruction SHA |

## Recovered behavior

The update owner is now a complete typed implementation. It advances each
playback codec, validates the relative tick count and debug stream cursor,
applies unit control, detects corrupt early termination, restores unit state,
performs the optional destroy/hover completion actions, clears the debug slot,
and retires the datum. Its 192 decoded instructions match the target exactly.

The debug renderer is also complete and strict exact. It builds the original
tabbed debug report in the 0x2800-byte local buffer, resolves scenario object
and recording names through typed tag blocks, applies the 200/300 tab stops,
renders the string, and restores the tab-stop state. Its 150 decoded
instructions and all 28 relocation identities match the target.

This wave also closes initialization, new-map debug clearing, debug storage
clearing, and the controlling-unit query. `recorded_animation_verify`, the
time-left query, the kill operation, and the private playback worker are kept
as lawful semantic residuals rather than forced through compiler artifacts.

## Evidence and experiments

The January split COFF remained the byte, relocation, string, line-number,
field-width, and control-flow authority. Semantic recovery used the pinned
local donors under:

- `build/audit/refs/stian-halo/src/halo/cutscene/recorded_animations.c`;
- `build/audit/refs/halocea/src/blam/cutscene/recorded_animations_update.c`;
- the recorded-animation playback and lookup donors documented by the prior
  playback wave.

The update target's relocation order exposed the two omitted stream checks and
the dynamic `animation %s appears corrupt` assertion. Restoring those ordinary
source blocks first produced an equal 560-byte body; matching the natural
positive-valid-object branch, branch-local lifetimes, direct flag fields, and
the original stringized expression then made the owner strict exact.

The renderer donor produced an instruction-for-instruction match on its first
typed implementation apart from the two independent zero assignments. Reversing
their source order recovered the target schedule. Naming the two standalone
globals directly recovered the target relocation identities without raw
addresses or synthetic aliases.

For `recorded_animation_verify`, a typed 0x40-byte `unit_control_data` aggregate
recovered the target size and relocation count. Its remaining difference is
the compiler's choice of two interchangeable local stack slots. Natural loop,
break, nested-if, shared-result, and early-return forms were evaluated for the
two small query/kill residuals; the retained forms preserve target sizes and
relocation counts without steering constructs.

Claude's active work was treated read-only throughout. No Claude worktree,
index, object, build output, or session state was modified.

## Policy and validation

The retained source uses typed animation-thread, codec, debug-entry, scenario,
object, unit-control, tag-block, and data-iterator access. It contains no
assembly, volatile/register steering, pragmas, intrinsics, barriers, raw
addresses, byte-offset access, pointer reconstruction, punning, undefined
behavior, synthetic anchors, byte payloads, or object patching. Declarations
remain C89-compatible and every function returns explicitly.

The frozen whole-object scratch gate reports `12 exact / 4 residual / 0
unwritten`. One selected-object production Ninja build completed successfully,
and direct post-build comparison reproduced `12 / 4 / 0`. `git diff --check`
passed; Git emitted only its line-ending notice. The production compiler also
reported one pointer-level warning at the `game_state_data_new` assignment,
caused by this translation unit's missing visible allocator prototype; the
32-bit emitted owner nevertheless matches the target strictly.
