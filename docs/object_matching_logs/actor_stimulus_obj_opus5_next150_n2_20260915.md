# `actor_stimulus.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Baseline and final real-file gate (`--forbid-emitted-symbol _point_from_line3d`, guard passes):
**21 exact / 1 residual / 0 unwritten**, park drift 0, fake scan 0 leads.
`git diff --stat -- source/ai/actor_stimulus.c` is empty. No credit is claimed. 0 shapes spent.

## `_actor_stimulus_prop_acknowledged` (parked `register-allocation`): skipped as exhausted

Target 800 / 32 / sha `b66ac2399346e00a`; real file 800 / 32 / `0f62d6950730557f`.
w2 reached its five-shape budget (tA, tB, tE, tH, tG) and left an explicit "do not repeat tA, tB or tG".

Census re-confirmed at this baseline:

- January `sub esp,0x38`, ours `sub esp,0x34` — one extra 4-byte home. January's six homes are
  `definition` -4, the `'<'`/`'>'` character -8, close/far -0xc, shoot -0x10, `combat_status` -0x14
  (dword store at +0xBF of `xor eax,eax; mov ax,[ebx+0x6e]`), `actor` -0x18. Ours homes `surprise_level`
  instead and rematerialises combat_status.
- Relocations 32 / 32, identical. The remainder is one EBX<->EDI web (actor in EBX plus its home, versus EDI)
  that runs the whole function.

**New negative this wave** — the one untried lever the w2 reopen criterion invites. Making the late
`actor->state.combat_status < _actor_combat_status_definite` test read the LOCAL instead of the field would
keep `combat_status` live across `console_printf` and `actor_stimulus_surprise` and force a home. It is
**refuted by January's own bytes**: +0x2AA is `cmp word ptr [ebx+0x6e],3`, a fresh FIELD read, exactly as the
production source already spells it. The local's live range is genuinely only the surprise block in January
too, so the home is an allocator decision, not a recoverable source fact.

**Reopen criterion (unchanged from w2):** a natural source fact that makes the short `combat_status` local
non-forwardable, combined with tE (`DATUM_INDEX_TO_ABSOLUTE_INDEX(actor_index)`) and tH
(`!combat_status ? "noncom" : "combat"`). Note tE+tH measure 816/32, i.e. +16 padded, so they can only be
proposed together with a change that removes 16 bytes.

## Checks

parkcheck: 1 park, drift 0. `tools/fake_match_scan.py source/ai/actor_stimulus.c`: 0 review leads.
The production file is byte-identical to HEAD. No header, config, ninja, configure, pytest or git action was
taken.
