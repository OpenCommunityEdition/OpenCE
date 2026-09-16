# weapons.obj — next-150K lane, final implementation wave, Tier B row B10 (2026-09-16)

Worker scope: `source/items/weapons.c`, TARGET `_weapon_can_be_fired` (Tier B10, PARKED). Nothing else in the unit
was in scope. **Real file not modified** (`git diff --stat -- source/items/weapons.c` is empty). Result: a strict-EXACT
**park-reopen proposal** built in scratch.

## Baseline and final real-file gate (HEAD 6d6529d98)

| gate | result |
| --- | --- |
| baseline (guard on) | `== exact 74 residual 5 unwritten 0 (of 79)`; guard passed; park drift 0 |
| final real file (guard on) | identical rows; guard passed; park drift 0; fake scan 0 leads |
| reopen candidate `scratch/workers/tB_weapons/reopen_candidate.c` | `== exact 75 residual 4 unwritten 0`; guard passed; only `_weapon_can_be_fired` changes (residual -> EXACT) |

## Census (written before any shape)

- tinfo: January 144 padded / 132 meaningful / 5 relocations / normalized sha16 `ba8b380662480849`
  (`_object_get_and_verify_type`, `_tag_get`, `__real@3f800000`, `_game_engine_running`, `_tag_block_get_element_with_size`).
- alndiff at HEAD: 60 (January) vs 59 (ours). January: age test falls through into an inline FALSE epilogue at +0x37;
  running/count/max/loaded failures branch FORWARD to the tail TRUE at +0x7e; the rounds_total test `je 0x37` branches
  BACKWARD to the FALSE epilogue; `mov al,1` epilogue last. Ours: inline TRUE epilogue at +0x46 bound backward by
  count/max/loaded, `setne al` tail.
- Frame: no `sub esp` and no `[ebp-N]` slot on either side (frameslot CLEAN); `[ebp+8]` only.
- Relocations by target: identical multiset (the literal is the systemic select-any class, symbolic-equal); only the
  tag-block call address differs (0x5c vs 0x62) because of our extra inline epilogue.
- Both research lenses hold at HEAD: instruction count 60/59, and F5 SETCC-DEFECT (January 0 / ours 1 setcc).

## Duplicate prevention finding

The research's implied change ("fold the final guard into the preceding five-term `&&` chain") had already been
measured: w3 c1/c3/c5 and 100K wave C Pcbf2 (see weapons_obj_opus5_150k_w3_20260914.md and
weapons_obj_opus5_100k_waveC_20260914.md). Re-measured here as v1: the setcc disappears (60/60 instructions), but the
unified FALSE exit is hosted after the chain (+0x78, age test `je 0x78` forward) instead of inline after the age test.
The F5 lens is therefore real but its named change is not sufficient; the rest of the residual is exit hosting.

## New evidence

Same-compiler strict-EXACT donors whose January bytes have this function's topology (a later chain's last test binding
BACKWARD into an earlier arm's inline constant exit, falling through into the other constant):

- `actions::_actor_action_can_stop_conversing` (source/ai/actions.c:1757-1799, read only; Codex-reserved file): flat
  `if (A1 && B1) result = TRUE; else if (A2 && B2) result = TRUE; else if (A3 && B3) result = TRUE; else result = FALSE;`
  gives arm 1's `mov al,1` epilogue inline at +0x62 and `jge 0x62` from arms 2 and 3.
- `actor_combat::_actor_combat_enable_special_fire_situation` (switch arms `result = A && B;`): case 3's last test
  `jge 0x40` binds backward into case 1's `mov al,1` epilogue.

(Located with the sibling B9 worker's read-only scan `scratch/workers/tB_rasterizer_transparent_geometry/backscan.txt`
and re-read from the bytes and sources by this worker.)

## Shapes (5 spent, all scratch; `scratch/workers/tB_weapons/v1..v5.c`)

| shape | evidence | result |
| --- | --- | --- |
| v1 F5 implied change: `if (age>=1) return FALSE; if (running && count>0) { magazine; if (max>0 && !loaded && !total) return FALSE; } return TRUE;` | research B10 | 144/5 sha `8d10523e3c79ff0b`; 60/60 insns; FALSE hosted last (= w3 c1) |
| v2 last term as a nested single-term guard | HEAD/e12 backward-guard bytes | sha == v1 (VC7 flattens it) |
| **v3 flat `if (age>=1) { result = FALSE; } else if (running && count>0 && TAG_BLOCK_GET_ELEMENT(...)->rounds_loaded_maximum>0 && !loaded && !total) { result = FALSE; } else { result = TRUE; } return result;`** | donors above; laws_w2 A3 single exit; laws_w3 A26 | **STRICT EXACT 144/5 `ba8b380662480849`** |
| v4 control: v3's flat chain with early returns | lab e13/c1 | sha == v1: the result-variable sibling arms are the lever, not the flat chain |
| v5 control: `boolean result = TRUE;` without the final else (house rule 17) | laws_w2 A10 | EBX constant-1 pin, sha `8489bd603b75d8a3`; declare+init is not practical here |

## Admission checks on the reopen candidate

- Whole-TU gate: 75 exact / 4 residual; row diff against the HEAD gate changes only `_weapon_can_be_fired`.
- Every other code section is byte-identical to the HEAD object (`section_infos_equal` over all 95 code owners).
- `_point_from_line3d` guard passed.
- `scratch/parkcheck.py`: the only drift is the target itself (144/5/`ba8b380662480849` = January), as a reopen expects;
  the unit has no other parks.
- Owner census vs build/base and build/split: 130/130 owners, no January owner missing, no new code/data/BSS/COMMON
  owner; only compiler `$L` jump labels renumber. The select-any `__real@3f800000` literal (the 20260831 rejection
  reason) is already owned by the HEAD body and base; no change.
- `tools/fake_match_scan.py`: 0 leads (HEAD 0).
- Authenticity: the weapon cannot fire when fully aged, or in a running game engine when magazine 0 of a magazine-using
  weapon has neither loaded nor reserve rounds; otherwise it can. Two distinct rejection reasons each assign FALSE in
  sibling arms (the A26-admitted identical-assignment form with distinct conditions), and one result is returned once
  (A3; the donor's style). Behaviour equals the HEAD body, including short-circuit order (the tag block element is read
  only when `count > 0`). House rules: typed accessors and `TAG_BLOCK_GET_ELEMENT`, `boolean`, one parameter per line,
  explicit `return result;`, no casts, no dead locals (the unused `magazine_definition` local is gone).

## Park-reopen proposal

- Park: `config/parked.json` entry `source/items/weapons` / `_weapon_can_be_fired` (class unclassified). Its criterion
  forbids further branch-spelling search "without new original-source or same-compiler evidence"; the w3 reopen
  criterion asks for "a measured VC7 rule for which duplicate epilogue survives cross-jumping (e.g. proven on another
  object), or January-side source evidence of the statement structure".
- Why it is met: the two exact same-compiler donors above prove, on other objects, which exit copy a chain's last test
  binds to when identical constant results are assigned in sibling else-if/switch arms. v1/v2/v4 (early-return
  spellings) and v5 (initializer) are measured controls showing the rule does not apply to them. The research's
  SETCC reading (F5 V14) independently refutes the park's "control-flow placement tie" premise.
- Candidate: `scratch/workers/tB_weapons/reopen_candidate.c` (the current real file with only this body replaced).
  Apply the body and remove the park entry in one commit. No other park needs a re-measure.

## Law note for the orchestrator (CJ3 refinement, measured on this row only)

When the same constant is returned from an early guard and from a later `&&` chain, early-return spellings host the
unified exit at the later chain (v1, v2, v4; lab e13/c1). If each arm assigns a result variable in a flat
`if / else if / else` and the function returns the variable once, the exit is hosted at the FIRST arm, and the later
chain's last test binds backward to it (v3; donors can_stop_conversing and enable_special_fire_situation).
Declare-and-initialize (`result = TRUE;` with no else) pins the constant in a callee-saved register instead (v5).
Possible consumers (unmeasured here): other F5 SETCC rows whose January bytes show a backward jcc into an earlier
inline constant exit, e.g. `_rasterizer_transparent_geometry_initialize` (B9), where w3c t5/t6 used early returns.

## Do-not-repeat

- Early-return spellings of the final chain (v1 = w3 c1 = Pcbf2, v2 nested guard, v4 flat chain), and the
  initializer form v5.

## Reopen criterion if the proposal is declined

An owner ruling on the A26 identical-assignment else-if form for this row (the body is otherwise measured closed).
