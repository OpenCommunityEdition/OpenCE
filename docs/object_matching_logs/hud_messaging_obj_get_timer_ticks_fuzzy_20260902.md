# `hud_messaging.obj` timer-ticks fuzzy reconstruction (2026-09-02)

## Result

`_scripted_hud_get_timer_ticks` now has a complete ordinary-C implementation
and is fuzzy-parked at **97.5%**. The focused gate advances from **29 exact / 1
residual / 6 unwritten** to **29 exact / 2 residual / 5 unwritten**. The target
and candidate both occupy 80 padded bytes and carry the same two relocation
identities.

The January normalized SHA-256 is
`c28cd50e0150845564921ef4ab5bb445c5a9cb58ac319a84461460fa53686468`;
the retained candidate is
`aaa93f528312a9d8cffdd2e9fed2f8b143dc031d73500550964b8eaeacd95350`.

## Source and behavior

The retained function uses `struct hud_timer_data_definition`, `short`, and
`word` rather than raw offsets or primitive stand-ins. It returns zero while
the timer is hidden, `NONE` for the sentinel duration, the stored tick count
while paused, and the elapsed-time-adjusted 16-bit result while running. This
behavior is independently corroborated by:

- the January target's complete instruction and relocation topology;
- HCEA's later Xbox reconstruction in
  `_research_cache/halocea/src/blam/sound/scripted_hud_get_timer_ticks.c`;
- the independent Stian January-PC lift recorded in the prior first-shot
  ledger; and
- the short-return HaloScript evaluator ABI.

The public declaration now lives in its owner header,
`source/interface/hud_messaging.h`; the duplicate consumer declaration was
removed from `source/hs/hs.c`.

## Fixed codegen boundary

The candidate is instruction-identical to January through the sentinel
comparison and again from the paused check through the function tail. The only
meaningful difference is the sentinel return. January emits the five-byte
`mov eax,-1`; VC7 recognizes that AX already contains `0xffff` and emits the
three-byte `or ax,ax`. That two-byte delta moves the otherwise identical
`game_time_get` relocation from `+0x32` to `+0x30`.

Bounded natural variants covered direct assignment, direct return, a named
game-time sample, and 16- and 32-bit named totals. They either reproduced this
fixed point or worsened the surrounding register allocation. The classifier
fails closed as `UNKNOWN`, so the park is honestly labeled `unclassified`.
No volatile/register forcing, barrier, pragma, assembly, raw offset,
representation pun, fake dependency, forced inline, or nonsensical branch is
retained.

## Validation

- focused HUD gate: `29 exact / 2 residual / 5 unwritten`;
- `_scripted_hud_get_timer_ticks`: 80/80 padded bytes, 2/2 relocation
  identities, 97.5% objdiff similarity;
- `hs.obj` consumer gate: `445 exact / 3 residual / 0 unwritten`;
- all 29 inherited exact HUD functions remain exact;
- parked-function validation, fake-match scans, focused tool tests, and the
  production HUD/HS build are required before integration.

