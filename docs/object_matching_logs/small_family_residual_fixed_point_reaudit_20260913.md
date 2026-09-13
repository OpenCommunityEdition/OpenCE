# Small-family residual fixed-point re-audit (2026-09-13)

## Result

This bounded pass re-audits the only residual functions in
`game_statistics.obj`, `recorded_animations.obj`, and `game_engine_ctf.obj`.
It admits **zero exact functions and zero exact bytes**.  All three retained
implementations are meaningful, typed C, but none has a new authentic source
form that closes its remaining compiler schedule.  Their existing fuzzy parks
remain active and receive no matching credit.

| Object | Focused gate | Residual | Target / current padded bytes | Relocations | Target normalized SHA-256 | Current normalized SHA-256 |
| --- | ---: | --- | ---: | ---: | --- | --- |
| `game_statistics` | 3 exact / 1 residual / 0 unwritten | `_game_statistics_record_kill` | 672 / 672 | 22 / 22 | `faed51b584f65f1a0f97f4416a0f599800dceeb2352a80dcf565d92d37724a69` | `c83a7c0613ccce274d91ca76f8f2315496ee2b9eefa91862c84160ff35ce5c0b` |
| `recorded_animations` | 15 exact / 1 residual / 0 unwritten | `_recorded_animation_play_internal` | 640 / 640 | 38 / 38 | `df113ef0763a2a945aaf24c999e7976d55b0f998719fff88ba52585d73eb1a7c` | `21d8285c6b390376e50a99d6b6de987fbbaf585cafe83d99973a40c5072c0be7` |
| `game_engine_ctf` | 38 exact / 1 residual / 0 unwritten | `_ctf_engine_weapon_update` | 576 / 576 | 40 / 40 | `0c8e2649895a583167f4b4c8d53d9d29c7e4c0edbad2488e717d4517ce66d20b` | `bff168bc4a51e834f7be79383114d53173377a1ba2c60404ff287930f5e5ecd5` |

Every focused compile used `tools/campaign/gate.py` with the
`_point_from_line3d` emitted-symbol guard.  Every pre-existing exact owner
remained exact, and the guard passed for all three translation units.

## Claude and donor census

A fresh read-only census covered all 1,840 registered worktrees.  Including
line-ending variants, the three source paths reduce to 20, 15, and 14 physical
byte families respectively.  The current Fable and Opus campaign trees add no
new `recorded_animations.c` or `game_engine_ctf.c` family.  Their
`game_statistics.c` is the already-audited direct-member family without the
newer narrow owner-header include.

The historical claim that Claude/Fable completed `game_statistics.obj` refers
to the exact-looking one-use local:

```c
short *kills = credited_player->statistics.kills;
kills[0]++;
```

That alias has no independent semantic purpose.  It exists only to obscure the
relationship between `kills[0]` and the adjacent `last_kill_time` member so
that C2 retains January's store/load order.  HCEA independently uses the direct
typed statistics member, and no original Bungie source or local-variable
record supports the alias.  The exact-looking donor therefore remains rejected
under the anti-fake-match rule.  The canonical direct-member source differs
only because C2 moves the independent `last_kill_time` load before the
`kills[0]` increment.

## Recorded-animation natural probe

The retained playback worker differs only in the scheduling immediately after
`tag_data_get_pointer`: January loads `animation_threads_debug`, stores the
thread stream pointer, masks the thread index, and then materializes the thread
stream address; the candidate performs the same independent operations in a
different order.

One source-plausible untried ordering was measured: calculate the typed debug
record before assigning `thread->event_stream`.  It preserved the 15 exact
siblings but made the playback residual substantially worse.  C2 moved the
debug lookup before the call and changed register allocation from offset
`+0x118` through later playback initialization.  The probe was reverted.  The
known function-exact alternative still requires false BSS linkage/order for
`animation_threads`, `debug_recording`, and `animation_threads_debug`; it is
not admissible even though those bytes are zero-filled.

## CTF fixed point

`_ctf_engine_weapon_update` has 566 meaningful bytes and 188 instructions on
both sides.  Only 78 relocation-normalized byte positions differ.  The aligned
stream is exact before `+0x13A`, resynchronizes at `+0x19A`, and is exact to the
end.  All relocation destinations agree; five relocation addresses move only
inside the idle-return message block.

January eagerly sign-extends the owner team into EDI and computes the opposing
team in EBX before testing the handled flag.  Natural C2 output tests the flag
first, retains the genuine 16-bit team value in BX, and promotes/calculates the
message arguments later.  This pass reviewed the 91 saved closeout trial
objects and the type, declaration, and direct-expression variants.  None beats
the current 78-byte fixed point; the next-best saved candidate differs at 203
positions, while long/reused-team variants differ at 324.  HCEA corroborates
the three gameplay phases but is a later-build reconstruction and does not
resolve the January schedule.

## Disposition

No production source, header, configuration, symbol map, or park entry is
changed by this pass.  In particular, no alias carrier, false storage linkage,
manual bit lowering, `volatile`, `register`, barrier, forced inline/noinline,
assembly, undefined behavior, or source-pressure construction is admitted.
Reopen these residuals only when first-party/local-variable evidence explains
the remaining schedule naturally.

Detailed predecessor evidence remains in:

- `docs/object_matching_logs/game_statistics_obj.md`;
- `docs/object_matching_logs/recorded_animations_obj_natural_recovery_20260830.md`;
- `docs/object_matching_logs/recorded_animations_obj_codex_small_closeout_20260903.md`; and
- `docs/object_matching_logs/game_engine_ctf_obj_codex_closeout_20260903.md`.
