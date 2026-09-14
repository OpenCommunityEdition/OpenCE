# `unit_dialogue.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Result

The real source file is **unchanged** (all five non-exact rows are locked parks). Three parks have
strict-exact, house-rule-compliant scratch bodies and are proposed for re-opening:

| function | padded | relocs | January normalized sha16 | lever |
| --- | ---: | ---: | --- | --- |
| `_unit_dialogue_determine_variant` | 176 | 7 | `5331055b840b29dd` | `sequential_counter` is a file static |
| `_unit_notify_impulse_sound` | 240 | 10 | `ff46a3c5213b3d65` | genuine in/out copy + `MAX` clamp |
| `_unit_dialogue_update` | 768 | 15 | `0bc104489dda51af` | `unit->unit.speech.` spelling (no pointer local) |

Candidate: `scratch/workers/unit_dialogue/reopen_candidate.c` (current real file plus exactly these three
changes). Whole-TU gate: **12 exact / 2 residual / 0 unwritten** (baseline 9 / 5 / 0). Every baseline-exact row
stays exact, the `_point_from_line3d` guard passes, the two remaining parks do not drift, the fake-match scan has
zero leads. `_unit_describe_speech` and `_unit_make_damage_sound` stay parked (not landed).

Baseline real-file gate: 9 exact / 5 residual / 0 unwritten; guard passes. `branch_sweep`: no historical blob is
better. Ledgers read: the five earlier `unit_dialogue_obj_*` logs and the five `config/parked.json` entries.

## Park reopen 1 — `_unit_dialogue_determine_variant`

- Park criterion: "residual confined to VC7's final independent load/increment/store register schedule; bounded
  natural local and direct-assignment spellings did not close it".
- January: `mov ecx,[_sequential_counter]; mov eax,ecx; movsx esi,di; cdq; idiv esi; inc ecx;
  mov [_sequential_counter],ecx; mov ax,[ebp+edx*2-0x24]; mov edx,[ebp-4]; mov [edx+0x6e],ax`. The counter load is
  reused for the increment across the word store through the unit pointer, so C2 knew that store cannot alias the
  counter. The park body's tentative definition `long sequential_counter;` is emitted as COMMON (external) and is
  re-read with `inc dword ptr [...]` after the store.
- Linkage evidence: January PDB publics (`scratch/pdb200-current/pdb_symbols.json`) list this TU's public data
  (`_global_speech_priority_names`, `_global_speech_override_priorities`) and public functions, but not
  `_sequential_counter` — exactly like the TU's private statics `unit_lose_speech`, `unit_find_dialogue_variant`
  and `unit_dialogue_setup`. No other source file references the counter.
- Change: `static long sequential_counter;` (the body is untouched). Postfix-in-subscript
  (`variants[sequential_counter++ % variant_count]`) was measured byte-identical to the park body and rejected.
- The park premise (register schedule) is refuted: this was linkage-driven alias analysis.
- Needs config: `config/symbols.json` line 22735
  `{ "file_offset": 5032724, "flags": 0, "name": "_sequential_counter" },` →
  `{ "file_offset": 5032724, "flags": 0, "name": "_sequential_counter", "static": true },`
  (consumers: unit_dialogue.c only). The code gate is exact against the current split; the flag aligns the
  target's storage class with the proven private linkage (the candidate emits `.bss` sclass 3 instead of COMMON).

## Park reopen 2 — `_unit_notify_impulse_sound`

- Park criterion: "different in scheduling/register/branch shape; no codegen pressure admitted".
- The park body reused one `long play_type` as both the sound-index in/out reference passed to `unit_test_speech`
  and the returned play type (with `(short)` casts). That is representation reuse, not credible source.
- January: frame `sub esp,0x34` = speech item (0x30) + `vocalization_type`. The address-taken copy of the sound index
  and then the play type both live in the enregistered parameter's home `[ebp+0xc]`. The parameter itself stays in
  `ebx` for the item field and the line-406 assert
  (`unit->unit.speech.current.sound_definition_index == sound_definition_index`). The clamp is
  `cmp ax,2; mov [ebp+0xc],eax; jg; mov [ebp+0xc],2`: a `?:` phi with the store before the branch, which is the
  `cseries.h` `MAX` shape. HCEA `unit_notify_impulse_sound.c` independently uses a separate test copy and a short
  result.
- Shapes:
  1. Separate `long test_sound_definition_index` plus `short play_type` with an if-assign: frame 0x38, residual.
  2. The same plus `play_type = MAX(play_type, _unit_play_speech_immediate);`: **exact**.
  3. Probe: `MAX` on the old single-variable body stays residual, so both changes are required.
- It is exact independently of the other reopens (`v4b.c`).

## Park reopen 3 — `_unit_dialogue_update`

- Park criterion: "remaining differences are local branch and instruction scheduling".
- Assert-literal proof of spelling: January's line-757 assert relocates to
  `??_C@_0DH@LMOPNAFJ@unit?9?$DOunit?4speech?4current?4priori@` =
  `"unit->unit.speech.current.priority > _unit_speech_none"` (the literal `unit_speak` shares). The park body wrote
  `speech->current.priority ...` through a `struct unit_speech *speech` local. That emitted a candidate-only literal
  and let C2 forward the `current.priority = none` store into the following test. January re-reads
  `cmp word ptr [esi+0x338],0`.
- Change: drop the pointer local and spell `unit->unit.speech.<field>` at all 48 sites. This is mechanical, with no
  logic change, and it is the style of `unit_speak` / `unit_test_speech` in the same file. Result: **exact**.
  This is an accessor-binding family effect: binding the aggregate to a pointer local changes CSE and store
  forwarding.
- Original bug, already reproduced by the park body and now documented with the policy comment:
  - January decrements `damage_minor_timer` (+0x39C) twice and never decrements `damage_major_timer` (+0x39E).
  - Layout proof: `unit_make_damage_sound` tests +0x39E against zero before the major-pain arm and stores 60 there
    (target +0x19A / +0x2C0). +0x39C is the minor timer (test at +0x1CC, 30 stored).
  - No other source decrements `damage_major_timer`.
  - Not undefined behaviour. HCEA corroborates the double decrement.
  - The candidate adds a `/* BUG (preserved for exact matching): ... */` comment. It is comment-only and bytes are
    unchanged.

## Not landed

### `_unit_describe_speech` (272, residual)

Everything aligns except the abbreviated path-strip loop:
- January leaves it **unrotated**: preheader `mov eax,esi`, header `test eax,eax; je` at +0x60, back edge `jmp +0x60`.
- VC7 clones the header into the preheader (`test esi,esi`) and folds the bottom test into `inc` flags.

Eight spellings were measured (`scratch/workers/unit_dialogue/shapes_ds.py`):
- `while (scan)`, `while (scan && (scan = strchr(..)))`, a for-loop, `for (;;)` and `do {...} while (TRUE)`: all
  byte-identical to the park body.
- `while (sound_name)` with `sound_name = scan + 1`: worse.
- `if (scan && ++scan)` (the Bungie idiom at ai_debug.c:6085): threads past the header, still rotated.

`loopscan.py` finds no accepted-exact January function with the same unrotated loop, so there is no law to import.

Reopen when a measured VC7 rule for declined header duplication, or original text, appears.

### `_unit_make_damage_sound` (784, residual)

January facts recovered:
- Single exit: `dialogue_index == NONE` jumps to the common epilogue with `spoke` stored.
- Frame 0x44 comes from four coalesced dword slots: category/vocalization, unit/sound index, effect type, effect
  volume.
- `unit` lives in `edi`, with its range split around the died arm where `edi` holds `actor_index`.
- The death chain lays out the quiet arm first.

Five shapes were tried (whole-body splices; alndiff differing lines, park body = 197):
1. Single exit: 191.
2. Plus an if/else actor-index selection: 198.
3. Plus `else if (!severe) quiet; else if (died_instantly) instant; else violent`: 192. The chain layout and the
   took_body/instantaneous slot sharing now match January.
4. Nested `?:`: 198.
5. `severe = FALSE` with a conditional TRUE: 200.

The frame stays 0x48 in every shape because `unit` never wins `edi` over `damage_category`. Classification: a
callee-saved web-priority residual with cascading slot coalescing. The best research body is
`scratch/workers/unit_dialogue/mds_f.txt`.

Reopen with a measured live-range-splitting law, or a structural fact that removes damage_category's register web.

## Proposals for the orchestrator

1. **park-reopen** `_unit_dialogue_determine_variant`, `_unit_notify_impulse_sound`, `_unit_dialogue_update`.
   - Candidate: `scratch/workers/unit_dialogue/reopen_candidate.c`.
   - Remove the three `config/parked.json` entries in place.
2. **config** `config/symbols.json` `_sequential_counter` `"static": true` (exact text above). Required for the
   determine_variant reopen's owner census to match the proven private linkage.

No header edits are needed. No `.h` or config file was modified by this worker.
