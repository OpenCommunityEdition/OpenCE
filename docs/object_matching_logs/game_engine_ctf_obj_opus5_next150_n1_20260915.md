# `game_engine_ctf.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

## Result

No source change. `source/game/game_engine_ctf.c` is byte-identical to the lane
baseline `ae10935da`; `git diff --stat` for the file is empty.

| gate | exact | residual | unwritten | point guard | park drift |
| --- | ---: | ---: | ---: | --- | --- |
| baseline (HEAD) | 38 | 1 | 0 | pass | — |
| final (real file) | 38 | 1 | 0 | pass | 0 of 1 |

`tools/fake_match_scan.py`: 0 review leads.

## `_ctf_engine_weapon_update` (parked, unclassified) — re-investigated, park kept

Target 576 padded / 566 meaningful / 40 relocations, sha16 `0c8e2649895a5831`;
candidate 576 / 40, sha16 `bff168bc4a51e834`. 188 instructions on both sides;
frames equal; relocation census by target identical (only five relocation
addresses move, all inside the divergent block). All 78 differing byte positions
lie in `+0x13A..+0x192`, the flag-return block.

### New decode of the block

```
January
  13a  movsx edi, word ptr [esi+0x68]        ; return team index read into a 32-bit register
  13e  lea ebx,[edi+1]
  141  and ebx,0x80000001 ; jns ; dec ebx ; or ebx,0xfffffffe ; inc ebx   ; signed % 2
  14e  test byte ptr [esi+0x1dc],0x40        ; the handled-flag test comes AFTER both values
  157  mov ecx,edi ; neg ecx ; sbb ; and ecx,0xfffffffd ; add ecx,0xc     ; 32-bit sound select
  192  mov edi, dword ptr [ebp+8]            ; weapon_index reloaded from its parameter home

ours
  13a  test byte ptr [esi+0x1dc],0x40 ; mov bx, word ptr [esi+0x68]       ; 16-bit short local
  147  mov cx,bx ; neg cx ; ...                                          ; 16-bit sound select
  15f  movsx ebx,bx                                                      ; late sign extension
  17c  inc ebx ; and ebx,0x80000001 ; ...                                ; % 2 inside the if
```

Two January facts contradict the current body (width detector, laws_w2 A18 /
laws_w3 A34): `movsx` into a 32-bit register plus `mov ecx,edi` prove the
return-block team local is a **`long`**, not the `short return_team_index` the
body declares; and both it and `(team + 1) % 2` are ordinary block-head
statements evaluated before the handled-flag test, so VC7 must hold two
callee-saved longs across the two calls — which is why January spends EDI on the
team and reloads `weapon_index` from `[ebp+8]` at `+0x192`.

### Shapes measured (2, scratch only)

1. `cw_w1.c` — reuse the function-scope `team_index` / `other_team_index` locals
   that already exist at the top of the function (the authentic C89 shape).
   Reproduces `movsx edi`, but VC7 then caches `weapon_index` in EBX from the
   prologue (`mov ebx,[ebp+8]` at `+0x7`) and moves the zero register to EDI:
   193 instructions and 20+ rows that are exact today break.
2. `cw_w2.c` — block-scope `long return_team_index` only. Byte-identical to
   `cw_w1.c`; this re-measures the wave-w2 `P1` result on the current body.

Every `long` spelling buys the block and loses the prologue, so HEAD's 78-byte
fixed point remains the best body. Prior evidence (Fable W3-W6/Vw2, the 91
closeout trial objects, the 2026-09-13 re-audit, and w2 P1/P2) is unchanged.

### Sharpened reopen criterion

Reopen only with a lever that lets the flag-return block hold two callee-saved
longs (team in EDI, `(team + 1) % 2` in EBX) while `weapon_index` keeps its
January home and is reloaded from `[ebp+8]` at `+0x192` instead of being hoisted
into EBX at `+0x7`. A `long` type change alone is insufficient and is now
measured twice.

## Checks

- Final real-file gate with the `_point_from_line3d` guard: 38 / 1 / 0, unchanged.
- `scratch/parkcheck.py source/game/game_engine_ctf`: parks 1, drift 0.
- `tools/fake_match_scan.py source/game/game_engine_ctf.c`: 0 leads.
