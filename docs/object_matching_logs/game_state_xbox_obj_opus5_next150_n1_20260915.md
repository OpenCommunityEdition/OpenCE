# `game_state_xbox.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

## Baseline and final

Guarded gate at HEAD and after the wave: **16 exact / 1 residual / 0 unwritten**,
`_point_from_line3d` guard passes, park drift 0. The real file
`source/saved games/game_state_xbox.c` is **unchanged** (`git diff --stat` empty).
Nothing landed; no exact byte is claimed.

`branch_sweep`: seven historical blobs, best 10 exact / 7 residual. No donor.

## `_game_state_open_persistent_storage` — NOT LANDED (block-layout tie)

Target 400 padded / 395 meaningful / 26 relocations / sha `b8779377c24f612c`.
Ours 400 / 26 / sha `6457f7e47a8b11aa`. Parked `instruction-scheduling`.

### Census (run before any shape)

* **Frame:** identical. Both allocate with `mov eax,0x4100; call __chkstk` and use
  `[ebp-0x100]` (`path[256]`), `[ebp-0x4100]` (`zeroes[16*1024]`) and the incoming
  `directory` home `[ebp+8]` for `bytes_written`. No slot differs.
* **Relocations:** 26/26, identical targets, order and addends.
* **Instructions:** target 127, ours 131 — the four extra are trailing `nop` padding.
  The two instruction streams are otherwise element-for-element identical.

The entire residual is the order of two terminal blocks and one branch destination:

| | January | ours |
| --- | --- | --- |
| after `CloseHandle` | `or eax,-1` + epilogue @0x12d | same @0x129 |
| next | `strcpy` arm @0x135 | `strcpy` arm @0x131 |
| next | open-fail assert @0x147, `or eax,-1` @0x17c, epilogue @0x17f | `mov eax,esi` @0x143 + epilogue |
| last | `mov eax,esi` @0x184 + epilogue | open-fail assert @0x14a, `or eax,-1` @0x17f, epilogue |
| path-lookup failure edge | `je 0x12d` (merges with the resize return) | `je 0x17f` (merges with the final return) |

### January-side evidence

* Assert line literals `push 0x1eb` (491) and `push 0x1f2` (498) fix the resize
  diagnostic seven source lines before the open diagnostic, so the open-failure handler
  cannot be an early `if (file == INVALID_HANDLE_VALUE)` guard ahead of the resize block.
* `push esi` at 0x12 (before the outer test) proves the outer-failure return block pops
  esi, i.e. it is tail-merged with a return inside the esi-live region. Any spelling in
  which that return needs no callee-saved register lets VC7 shrink-wrap the push.

### Shapes measured (6, all scratch; `scratch/workers/n1_player_profile_game_state_xbox_etc/gsx_v1.c` … `gsx_v6.c`)

| # | shape | result |
| --- | --- | --- |
| v1 | outer `else {return INVALID;}`, open-fail `else {vassert; return INVALID;}`, `return file;` last | 139 insns; `push esi` sinks, failure return inlined at 0x29 |
| v2 | as v1 but keeping the trailing `return INVALID_HANDLE_VALUE;` | 139 insns, same damage; tail order unchanged |
| v3 | leading guard `if (!directory && !…path(path)) return INVALID;` | 139 insns, identical to v2 |
| v4 | open-fail `else { vassert(498); }` falling through to one `return file;` | **tail order becomes January's**, but the two returns merge; 384 padded / 126 insns |
| v5 | positive-polarity early returns for both size tests | 135 insns; `mov eax,esi` inlined at 0xe5 |
| v6 | both `return file;` as sibling `else` arms, one trailing `return INVALID;` | folds back to the baseline bytes exactly |

### New measured VC7 fact (offered for the law file)

For `if (c) { … } else { cold_diagnostic; }` followed by a value return, VC7 13.00.9254
lays the else block **before** the value-return block only when the else arm falls
through into that return (v4). As soon as the else arm carries its own
`return <constant>;` (v1/v2/v3), or both value returns are written as sibling `else`
arms (v6), the value-return block is hoisted ahead of the cold block. January requires
v4's order together with v2's two distinct return blocks; no admissible C spelling
produced both.

### Reopen criterion (refined)

Reopen only when a source form yields, at the same 127 instructions, the tail order
`[open-fail assert][or eax,-1 + epilogue][mov eax,esi + epilogue]` **and** a
path-lookup-failure edge targeting the resize-failure return copy. The six forms above,
plus the eight recorded in `game_state_xbox_obj_opus_remainder_reconciliation_20260908.md`,
are measured negative. Authoritative January source, or a same-compiler donor object
built from known source, is now the only admissible route.
