# sound_manager wave-3 worker ledger (claude fifty-objects lane, tree 954eebd2)

## 0. Baseline
- gate --all (production source): exact 63 residual 2 unwritten 0 (of 65).
  - residual 208 _sound_set_definition_end [sha]
  - residual 1360 _update_channel_for_looping_sound [sha]
- Not in frame_census / structural_split baseline (size+relocs equal on both rows).
- Prior ledgers read: n3a, n4, w3c, w1 (sound_manager_obj_*), astra research5 shared context + sound_type_rejected.
  Recorded blocker for both rows: January caller `xor eax,eax; mov ax,word [sound+0x8e]` (16-bit param
  fingerprint) for the 3rd argument of sound_definition_find_pitch_range_by_pitch; header+def are `long`.
  Owner ruled short-header/long-definition an ABI lie; coherent short header+definition regresses the
  owner `_sound_definition_find_pitch_range_by_pitch` by 1 byte (`mov si,word [ebp+0x10]` vs January
  `mov esi,dword [ebp+0x10]`) under the UNCHANGED body (astra research5).

## 1. Evidence re-read (before any probe)
- Current production looping row differs from January at THREE places, not two: 0x2ea (movzx vs
  `xor ecx,ecx; mov cx,[edi+0x8e]`), 0x37e (missing `xor eax,eax` before channel_get_state), and the
  branch target at January 0x3d2 `jne 0x406` (lands on the `test byte [edi+4],8` waiting test = the
  `else if (!waiting)` arm) vs ours 0x3ce `jne 0x482` (skips it). alndiff normalises branch targets, so
  it does not print the third hunk; sbs.py shows it.
- /Od (later build) fn 0x899e50 attests: outer `if (type != stop && (type != start || !fade_in))`, then a
  short block local `channel_state = channel_get_state(...)` stored [ebp-0x70], then
  `state != queued || waiting || (state == queued && pp->next == NONE && next_def != NONE)` (0x89a588..0x89a5cb),
  then the COMBINED `if (next_def != NONE && (!pp || pp->next == NONE)) ... else if (!waiting)` (0x89a5d1..0x89a5f3).
- Lane D (claude-lane-d-refresh-20260922 scratch/lane_d/w/sound__sound_manager) already measured these as
  A1 (combined condition) + A2 (channel_state local): looping 1 instruction pair from exact; the
  orchestrator reverted A1 as a 'behaviour change' - but January's own bytes (0x3d2 -> 0x406) have that
  behaviour, so the production nested form is the deviation from January.
- /Od callers of pitch-range fn (0x877810) push the 3rd arg with `movzx r, word [sound+0x8e]` (0x89725f,
  0x89a4c0) - same as the /Od push of the known-short tick_index param at 0x877f49; /Od callee reads the
  param only as `movsx ecx, word [ebp+0x10]` / `mov ax, word [ebp+0x10]`.

## 2. Probes
### P1 cand_a12.c = production + lane-D A1 (combined next-definition condition, January 0x3d2->0x406 + /Od
0x89a5d1) + A2 (/Od block local `short channel_state` with the /Od-attested redundant `state == queued &&`
term, /Od 0x89a588..0x89a5cb); `(word)` casts KEPT (production spelling).
Hypothesis: A2 restores January's `xor eax,eax` @0x37e, A1 restores the 0x3d2 branch target.
Result: gate 63/2 (rows identical), guard passed. Looping now differs from January ONLY at 0x2ea
(`movzx ecx,word [edi+0x8e]` vs `xor ecx,ecx; mov cx,word [edi+0x8e]`), 421 vs 420 insns, branch
targets all equivalent. set_definition_end unchanged (only 0x3c pair). => FUZZY improvement (3 diffs -> 1).
### P2 header lab (hdr/): lane-D candidate (A1+A2, (word) casts removed) + LOCAL COPY of sound_definitions.h
with `short pitch_range_index` (3rd param of sound_definition_find_pitch_range_by_pitch), injected via
gate --edits (absolute include path). Result: sound_manager 65/65 EXACT, guard passed;
sound_definitions (definition still `long`, sees short prototype -> C4028) 7/7 EXACT.
=> Reproduces lane D. This is the owner-REJECTED "header prototype contradicting its definition" form
(throughput checkpoint 40k 'Rejected' table). Not landable.
### P3 VC7 width lab (lab/width.c, mini.py with sound_definitions cflags) - NEW FINDING
Hypothesis: the load width of a short parameter is not fixed by the type alone; astra research5 tested
coherent `short` only with the retained EARLY-RETURN body. The /Od build (0x877810) and HCEA show a
SINGLE-EXIT body: `result = old_range_index` in the in-range arm, then `if (result == NONE) { loop ...
result = range_index; break; ... }`, `return result;`.
Result (callee load of the 3rd param):
- w_long (production body, long + casts): `mov esi, dword [ebp+0x10]` (control, = January)
- w_short (early-return body, short, no casts): `mov si, word [ebp+0x10]` (= astra's regression)
- w_short_od (/Od single-exit body, short `old_range_index`): `mov esi, dword [ebp+0x10]`, 256 B,
  alndiff vs January _sound_definition_find_pitch_range_by_pitch: "no normalized instruction differences".
- w_word / w_short_local (early-return fragments): `mov si, word`.
=> A coherent short prototype+definition IS reachable: the owner must use the /Od-attested single-exit shape.
### P4 coherent short owner (coh/): sound_definitions.c definition rewritten to the /Od 0x877810 single-exit
shape with HCEX param `short old_range_index` (HCEX DIA2Dump: params `sound`(struct sound_definition*),
`pitch`(float), `old_range_index`(short)); /Od statement order in the closest update (result then
closest); /Od if/else after the break (0x87791f jmp exit; 0x877921 jmp over else). Header prototype
`short old_range_index` (coh/sound_definitions.h, injected by --edits).
Result: sound_definitions 7/7 EXACT (owner _sound_definition_find_pitch_range_by_pitch EXACT 256).
### P5 sound_manager with coherent header (coh/sound_manager.c = lane-D candidate: A1 + A2 + plain
`sound->pitch_range_index` args, no `(word)` casts): gate 65/65 EXACT, guard passed.
objeq (non-debug sections + symbols) vs a production gate compile: only the two target .text sections differ.
sound_definitions coh obj vs production gate compile: IDENTICAL (owner bytes unchanged).
### P6 header blast-radius sweep (sweep/): all 16 other sound_definitions.h consumers compiled with production
source vs production source + coh header (via --edits): 16/16 objects IDENTICAL (non-debug sections +
symbols); row counts unchanged. Consumers: predicted_resources xbox_sound_cache devices effects particles
game_engine_multiplayer_sounds hs_library_external hud_sounds ui_widget projectiles weapons objects scenario
game_sound sound_dsound_xbox unit_dialogue.
### P7 storage: pdb_storage on production flags 45 disagreements: 44 sound_manager statics (ours 3, split 2,
not in cachebeta publics) + `_sound_manager_globals` (split 2, ours 2, NOT public, and no UNDEF reference in
any other January split object -> file-static). cand coh/sound_manager_st.c = coh/sound_manager.c +
`static struct sound_manager_globals sound_manager_globals = { 0 };` -> gate 65/65 EXACT, guard passed.
scratch/w/sound_manager/config/symbols.json: 45 in-place line edits (`, "static": true`) on the 44 function
entries in [1815168,1831696) + `_sound_manager_globals` (5057888). Scratch csplit
(build/tools/csplit.exe -i cachebeta.exe -p scratch/w/sound_manager/cfg -o scratch/w/sound_manager/split):
833 objects, ONLY source/sound/sound_manager.obj differs from build/split.
object_audit vs the emulated split (object_audit_t.py, AUDIT_TARGET): 113 January symbols, 0 differ, every
January-owned section ok -> OBJECT AUDIT: PASS. Surplus = 9 literals, 9 header-inline COMDATs, .drectve.
### P8 audits on coh/sound_manager_st.obj (coherent header injected)
- gate --all: 65/65 EXACT, _point_from_line3d guard passed.
- objeq vs production obj: only the 2 target .text sections + `_sound_manager_globals` storage 2->3.
- surplus_identity (production obj, same 9 COMDATs byte-identical in candidate): 9/9 IDENTICAL.
- provider_link (candidate obj): 18 surplus items all PASS; SELECTED-PROVIDER LINK: PASS.
- build/report.json data for sound_manager: .bss 376 / .data 1548 / .rdata 2028 all 100% (3952/3952).
- fake_match_scan on the 3 candidate files: 0 leads.
### P9 strip test of A2's redundant `channel_state == _sound_channel_queued &&` term (coh/strip_redundant.c):
looping -> residual 1360 [sha]; January's `xor eax,eax` @0x37e is lost. The term is load-bearing AND
verbatim in the /Od build (0x89a5a1 `movsx edx,word [ebp-0x70]; cmp edx,2; jne 0x89a866`). Disclosed for review.
/Od frame order new_sound_index [-0x6c] -> channel_state [-0x70] -> permutation_index [-0x74] places the
channel_state declaration between the crossfade block and the else-if block = the outer-if block (candidate).
### P10 law-(g) scan of sound_manager.c: two consumer-local FUNCTION prototypes (`long game_time_get(void);`
duplicate of game/game.h:176 which the TU already includes; `unsigned long system_milliseconds(void);` whose
genuine owner is cseries/cseries_windows.h, included by 24 of the 25 other callers). Fix: delete both,
add `#include "cseries/cseries_windows.h"` after cseries/profile.h (coh/sound_manager_hdr.c).
Result: 65/65 EXACT, guard passed; surplus gains 3 XDK tables (_D3DPRIMITIVETOVERTEXCOUNT,
_D3DSIMPLERENDERSTATEENCODE, _D3DTEXTUREDIRECTENCODE) - surplus_identity_c.py: 21/21 candidate-only
sections IDENTICAL to January's selected providers; provider_link PASS (both orders) for all 21.
(csplit keeps selectany tables only in the selected provider, so their absence from January's
sound_manager split is not evidence against the include; collision_debug admitted with the same D3D surplus.)
Consumer-local DATA externs (sound_data, looping_sound_data, sound_channels, loud_dialog_hack,
debug_looping_sound) left as-is: no definer in the tree (January linker_common COMMON), same accepted state as
admitted collision_debug (21 externs) / render_objects (3). Evidence noted: January references
_sound_data/_looping_sound_data/_sound_channels ONLY from sound_manager.obj (+linker_common).
### FINAL (final/): sound_manager.c 65/65, sound_definitions.c 7/7 (obj IDENTICAL to production),
object_audit vs emulated split PASS (0 of 113 symbols differ), storage vs cachebeta publics: 0
disagreements in both candidate objects; sound_definitions object_audit PASS.
### P11 objdiff 3.3.1 (build/tools/objdiff-cli.exe, scratch project od_proj/, target = emulated split):
sound_manager final 16085/16085 code, 3952/3952 data, 65/65 fns (control: production 14521/16085, 63/65 =
build/report.json); sound_definitions final 922/922, 212/212, 7/7.
Patches: production.patch (sound_manager.c + sound_definitions.c), header.patch (sound_definitions.h),
symbols.patch (= config/symbols.json copy, 45 line edits), parked.patch (= config/parked.json copy, 2 parks
removed; parked_functions validates: stale [] invalid []). All `git apply --check` clean; applying them to a
copy reproduces the final/ files byte-for-byte.
### P12 mechanism lab (lab/width2.c): h1 = single-exit in-range arm (`result = old_range_index;`) + early-return
loop -> `mov esi, dword [ebp+0x10]` (272 B, not exact); h2 = early-return in-range arm + single-exit loop ->
`mov si, word [ebp+0x10]` (272 B). => The dword parameter load comes from the in-range `result = old_range_index`
copy (param web coalesced with the dword-homed `result`, January `mov dword [ebp-4],0xffffffff`); the full
/Od shape (both single-exit pieces) is needed for the exact 256-byte owner.
NOTE for integrator: production.patch and header.patch MUST land together; the short definition with the old
`long` header (or vice versa) is exactly the owner-rejected mismatch (C4028).

## 3. Verdict: OBJECT_COMPLETE_CANDIDATE (sound_manager 65/65; sound_definitions unchanged bytes)
Row diff vs production gate: _sound_set_definition_end residual->EXACT (208), _update_channel_for_looping_sound
residual->EXACT (1360); 63 others unchanged. Integrator packet (apply together):
  production.patch  source/sound/sound_manager.c + source/sound/sound_definitions.c
  header.patch      source/sound/sound_definitions.h (shared header; 18 consumers, 16 swept IDENTICAL)
  symbols.patch     config/symbols.json (45 `"static": true` line edits; copy at config/symbols.json)
  parked.patch      config/parked.json (retire the 2 sound_manager parks; copy at config/parked.json)
  config_status.patch config/config.json sound_manager.c NonMatching -> Matching
Review flags (disclosed): (1) A1 restores January's behaviour (0x3d2 -> 0x406) that the production nested form
deviated from - the lane-D orchestrator revert read it as a behaviour change; (2) A2's redundant
`channel_state == _sound_channel_queued &&` is /Od-verbatim and load-bearing; (3) cseries_windows.h include adds
3 identical/link-clean XDK tables; (4) data externs (sound_data etc.) left as consumer-local (collision_debug
precedent). Supersedes the "ABI lie"/"coherent short regresses owner" blocker: the coherent short form works with
the /Od-attested single-exit owner body.
### P13 fallback measurement: production sound_manager.c (A1/A2 absent) + coherent header/definition:
64/65 - _sound_set_definition_end EXACT with or without the `(word)` casts; looping residual 1376 [size, sha].
So the header+definition packet alone closes set_definition_end (208); the looping row additionally needs A1+A2.
