# weapons.obj — opus5 100K consolidated lane, wave C (2026-09-14)

## 8. Disposition (summary first)

- Landed in `source/items/weapons.c`: **`_weapon_trigger_fire` strict EXACT** (1984 padded / 1982 meaningful,
  62 relocations, normalized SHA-256 prefix `0a873f5653980679`, equal to the January target).
  Real-file gate `== exact 73 residual 4 unwritten 2` (baseline `== exact 72 residual 5 unwritten 2`),
  `_point_from_line3d` guard passed, no baseline-EXACT row lost, park `_weapon_can_be_fired` unchanged
  (144/5/`a0706edaa4f925be`), owner census clean, fake scan 0 leads. Diff: 3 lines (comparison polarity).
- Not landed (not strict exact): `_weapon_update` (best candidate 2800/97, 16 differing instructions),
  `_trigger_create_projectiles` (guard-blocked; not exact even unguarded), `_weapon_magazine_finish_reload` (tie).
- Untouched: `_weapon_can_be_fired` (parked; scratch-only diagnostics, no reopen proposal), `_weapon_place`,
  `_animation_update` (brief: owner header / ABI blocked).
- Object remains `NonMatching`.

## 1. Scope and provenance

- TU `source/items/weapons.c`; target `build/split/source/items/weapons.obj`.
- XDK VC7 CL 13.00.9254, unit flags `/O2 /Oy- /DDEBUG /Dxbox`, repository include paths.
- Evidence: January relocation/call graph and disassembly (alndiff), January map atlas `scratch/atlas/weapons.obj.txt`,
  HCEA reconstructions `halocea-review/src/blam/items/{weapon_trigger_fire,weapon_update,trigger_create_projectiles,
  weapon_magazine_finish_reload,weapon_can_be_fired}.c` (semantics/spelling hints only), wave-A report
  `scratch/workers/weapons.md` and ledger `weapons_obj_opus5_100k_20260914.md`.

## 2. Validated baseline (lane HEAD 7a3bbb9c8)

`== exact 72 residual 5 unwritten 2 (of 79)`, 11,024 of 18,240 padded code bytes exact.

| Function | Target size/relocs/sha16 | Base size/relocs/sha16 | First divergence |
| --- | --- | --- | --- |
| weapon_trigger_fire | 1984/62/0a873f5653980679 | 1984/62/f474dc017c670603 | T 0x345 effect-selection block placement (misfired/firing blocks moved, `effect_error=0` stores cross-jumped) |
| weapon_update | 2800/97/308bb149c4e2928e | 2816/97/e53cb51d7bb0b69e | T 0x1e frame slot rotation; magazine loop register homes; switch arms threaded differently |
| trigger_create_projectiles | 1824/41/95877e588ec3a4ec | 1808/41/4d21f1084e455f54 | frame 0x1c20 vs 0x1c18 (inline point_from_line3d temporaries) |
| weapon_magazine_finish_reload | 224/6/fe3b6ad624c77289 | 224/6/a1c99ff5e63fae15 | T 0x59 `movsx edi` vs `movsx edx` (rounds_total not kept for the reserve update) |
| weapon_can_be_fired (parked) | 144/5/ba8b380662480849 | 144/5/a0706edaa4f925be | T 0x44 engine-running return shares the TRUE epilogue |

## 3. Accepted controls

- `weapon_trigger_fire`: the heat error guard is `if (heat_overheated_threshold==0.0f) effect_error= 0.0f; else
  effect_error= heat/threshold;`. January bytes: `fcomp 0.0; test ah,0x44; jp` with the zero store on the
  fall-through, reusing `ecx` (=`_trigger_firing_effect`, 0) as the stored zero, and three separate
  `effect_error` stores (no cross-jumping). HCEA `weapon_trigger_fire.c:267` spells the same `== 0.0` guard.
  The `!=` spelling put the division on the fall-through, VC7 cross-jumped the three zero stores and dragged the
  misfired/firing blocks out of line. Semantics unchanged (natural divide-by-zero guard).

## 4. Experiment matrix (scratch/workersC/weapons/)

| Function | Shape | Result | Kept |
| --- | --- | --- | --- |
| weapon_trigger_fire | `==` polarity (t1.c) | 1984/62 EXACT | landed |
| trigger_create_projectiles | v0 proposal-B inline point_from_line3d (guard off) | 1824/41, 2 residual items: use_aiming_vector store position, else-block placement | no (guard) |
| | va1/va2 store `use_aiming_vector= TRUE` after adjust_origin (both decl orders) | 1824/41, whole frame re-sorted | no |
| | vb1/vb2 boolean expression `actor==NONE \|\| !blindly` / `!(actor!=NONE && blindly)` | frame kept, branchy 0/1 stores | no |
| | vc1 HCEA `inside_bsp= unit && player!=NONE; if (inside_bsp)` | 1808, frame 0x1c1c | no |
| | v1/v2 TU-local non-emitting POINT_FROM_LINE3D macro (collisions.c precedent, 2 temp orders) | guard passes, 1808/41, frame 0x1c18, lazy fld/fstp distance slot; worse than real file | no |
| weapon_update | D1 `triggers_down[trigger_index]` at every use (no `trigger_down` local) | switch region aligned; 131 diff insns (base 285) | candidate |
| | D1 + fraction-first (w1) / + no local fraction-first (r1) / + no local loaded-first (r3) | 142 / 2800 two divisions / 2816 | no |
| | D1 + long if-scope local (rA) | 2784 | no |
| | **D1 + rD `short rounds_recharged` at loop-body scope, used by the >0 test** | **2800/97, 16/17 diff insns, sha d55e98dca8fd09a1** | best candidate (not exact) |
| | rD + fraction-first (rE) / `-=` before `++` (rD2) / operand swaps (g12, g1) / field in test (h7) / const def (V1) / MIN clamp (V8) / function-scope or uninitialised local (V9, V10, V11) / decl order (V13) / `+= 1`, `++x`, long loop local (m2-m4) | inert or worse | no |
| weapon_magazine_finish_reload | F1 inline MIN in add; Ftot1 `+= old-new`; Ftot2 `-= new-old`; F5 decl order | identical to base | no |
| | F3short/F3long HCEA if-clamp local | worse | no |
| weapon_can_be_fired (park, scratch only) | Pcbf1 single-exit flag / Pcbf2 grouped exits / Pcbf9 negated tail / Pcbf10 nested | best Pcbf2 8/8 diff insns (FALSE epilogue cross-jumped to later position) | no |

Every candidate gate kept all 72 baseline exact rows; the park measurement never changed.

## 5. Do-not-repeat list

- trigger_create_projectiles: macro re-spellings of point_from_line3d cannot reproduce inline parameter temporaries;
  moving the `use_aiming_vector= TRUE` store re-sorts the frame; HCEA boolean-expression forms for use_aiming_vector
  and inside_bsp are worse.
- weapon_update recharge block on top of D1+rD: statement order, operand order, const, MIN, `+= 1`/prefix `++`,
  local scope (function/uninitialised) and declaration order are inert.
- weapon_magazine_finish_reload: MIN/ternary/if-clamp/compound-assignment/declaration-order spellings exhausted
  (with wave A's short/MIN/-=).
- weapon_can_be_fired: single-exit flag is worse; grouped-exit forms stop at epilogue cross-jump placement.
- Never: weapon_place long-return form, animation_update ABI guesses, emitting `_point_from_line3d` here.

## 6. Residual classification

- weapon_update: measured — the D1+rD candidate differs only in the recharge block where magazine_definition takes
  `edi` (ours) instead of `ecx` (January) and the rounds_loaded CSE takes `ecx` instead of `edi`; January then does not
  forward the stored rounds_loaded into `inc word [esi+8]`. Inferred: register-coloring priority tie. D1 (threading)
  and rD (single idiv from the test value) are source/topology findings backed by the January bytes.
- trigger_create_projectiles: ownership/guard (January's inline point_from_line3d COMDAT) plus a store-position /
  block-placement tie in the proposal-B form.
- weapon_magazine_finish_reload: scheduling (16-bit add, rounds_total CSE rematerialisation).
- weapon_can_be_fired: parked; block placement of a cross-jumped return epilogue.

## 7. Reopen criteria

- weapon_update: start from `scratch/workersC/weapons/wD1rD.c`; reopen with a source form that lets magazine_definition
  outrank the rounds_loaded CSE temp (e.g. new VC7 allocator-order evidence), keeping D1 and rD.
- trigger_create_projectiles: lane policy allowing the genuine inline point_from_line3d in weapons.obj AND evidence
  for the use_aiming_vector store position.
- weapon_magazine_finish_reload: independent evidence of local types/order.
- weapon_place: scenario weapon record owner header. animation_update: authentic caller/return type evidence.
