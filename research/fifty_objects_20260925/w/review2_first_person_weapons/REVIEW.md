# Adversarial review: source/interface/first_person_weapons (wave 2, verdict FUNCTIONS_EXACT_OBJECT_BLOCKED)

Reviewer slug scratch/w/review2_first_person_weapons/. Base 931ed8dc.

## Setup
- git apply --check: production.patch (=prodA.patch) and owner_gated_exact.patch (=prodB.patch) both clean at 931ed8dc.
- Applied each patch (git apply, outside the repo, on HEAD blobs): patched .c/.h/symbols.json/parked.json are byte-identical
  to worker final_landable.c / final_exact.c / hdr/interface/first_person_weapons.h / config/symbols.json / prodA|prodB parked.json.
- A/ and B/ hold cand.c (verbatim patched .c) + interface/first_person_weapons.h (patched header, found first via quote-include
  search of the .c's own directory) so the real patched header is compiled, not the production one.

## Gate (independent re-run)
- NOTE: gate.py copies --source to scratch/_gate_<pid>.c, so a header placed beside cand.c is NOT used. First run (production
  header still declaring extern message) was redone with --edits redirecting the include to the patched header
  (A_edits.json/B_edits.json; redirect proven live with an #error lab header).
- A (patched header): 33 exact / 1 residual (_first_person_weapon_update 1520/60 sha ba39884c...), point_from_line3d guard passed.
  Every other non-debug section section_infos_equal to a fresh production compile (prod/prod.obj). Park base measurement
  (1520/60/ba39884c90f2...) reproduced exactly via tools.coff_compare.section_info.
- B (patched header): 34/0/0 EXACT, guard passed; vs production only update differs + surplus _square_root/_magnitude_squared3d/_magnitude3d.
- Header/prod-header compiles are section-identical for both A and B (only internal $L label numbering moves).
- object_audit: A FAIL(2) = update + message storage 2/3; B FAIL(1) = message storage 2(split pre-regen)/3. Surplus beyond
  production's pre-existing literal/.drectve list: B only the 3 math COMDATs.
- provider_link.py B: SELECTED-PROVIDER LINK PASS (all 12 surplus incl. 3 COMDATs). surplus_cand.py B: 3/3 IDENTICAL to
  action_charge/action_alert selected copies.
- Storage: cachebeta_publics lacks _first_person_weapon_message (0xCD670); pdb_storage: 1 disagreement (message). Exact-name scan
  of every build/split and build/base object: only first_person_weapons.obj names _first_person_weapon_message. git grep: only
  callers are message_from_unit/weapon inside the TU (lines 734/756) -> static is safe to link.

## Split regeneration (csplit into scratch only; build/split untouched)
- build/tools/csplit.exe -i cachebeta.exe (input data) -p <scratch cfg> -o <scratch out>, base vs patched symbols.json.
- Base regen: all 833 objects byte-identical to build/split (deterministic). Patched regen: ONLY
  source/interface/first_person_weapons.obj differs, ONLY `_first_person_weapon_message` storage 2 -> 3 (section 29, value 0).
- object_audit against the regenerated split (object_audit_alt.py with SPLIT_OBJ): B = OBJECT AUDIT PASS (45/45 symbols, all
  January sections, surplus = 3 math COMDATs + production's pre-existing literals/.drectve). A = only update differs.
- Storage double-attested: cachebeta publics absent AND HCEX.pdb `static void first_person_weapon_message(short, short)`.

## Evidence claims re-verified
- (1) January update @0x13c: `cmp ax,1; je 0x150` (0x150 = after the next_state call); production `je 0x148` (to the call).
  So key frame does NOT call next_state; production `||` is semantically wrong. /Od 0x61fb2a..0x61fb4f: result homed at
  [ebp-0x2c]; `cmp ecx,1; jne 0x61fb39; jmp 0x61fb4f; movsx edx,[ebp-0x2c]; cmp edx,2; jne end; call next_state`
  = empty-then trampoline of if/else-if (a switch would home into a separate temp and branch to case labels).
- (2) /Od 0x61fb22 and 0x61fc59 both `call 0x404ebc -> 0x61c0b0`; 0x61c0b0 = 3-arg body `push [ebp+10],[ebp+c],[ebp+8];
  push 0; call animation_update_internal-thunk` = the TU wrapper. Exact-name scan: no split object other than fpw names it.
- (4) /Od RTC descriptor 0x6204c0: [ebp-0x24] 4 sound_definition_index, [ebp-0x48] 8 forward, [ebp-0x70] 8 turning;
  turning.i/.j at -0x70/-0x6c. HCEX.pdb: `cl Relative [0x50] Local long sound_definition_index` (only stack local).
- (8) n1 (B + both guards restored) re-measured: residual [size 1552, sha]; alndiff: exactly 4 inserted insns
  (test eax,eax / je at O 0x24f/0x257 and 0x2a2/0x2ab) + tail padding; all replace rows are reloc-representation/shift.
- January 0x231..0x255: `test ecx,ecx; jne; xor eax,eax; jmp 0x24f; ... mov word [esi+0x1c],0; cmp dword [eax+0x10],3`
  = the unguarded NULL dereference. Correctly owner-gated (brief s7).

## Strip tests / necessity (lab, on B with the patched header; lab/*.json + *.txt)
| probe | change | result |
|---|---|---|
| S1_plain_eq2 | `if (r==_animation_will_restart_on_next_frame)` only (plainest spelling) | residual [sha] |
| S2_ne1_and_eq2 | `if (r!=key_frame && r==will_restart)` | EXACT (but /Od shows the jne+jmp trampoline, i.e. empty then, not this) |
| S3_switch | `switch` with `case key_frame: break;` | residual [sha] |
| N7_state_internal | state call back to animation_update_internal | residual [sha] |
| N5_moving_internal | moving call back to animation_update_internal | EXACT (inert; kept for /Od 0x61fc59 consistency) |
| N57_both_internal | both back | residual [sha] |
| N2_scalars | production turning_i/turning_j block | residual; frame `sub esp,0x10` vs January 0x14 |
- Empty then-arm: load-bearing AND independently attested by /Od (trampoline) + January `cmp ax,1`; semantics == January.
  Same disposition as the Lane B actions review precedent (empty-then-else attested by /Od `jne C; jmp exit`). Not in any
  brief s7 owner-gated class. `turning`: plain local aggregate of a project type with first-party name+size, not a view
  cast/copy. Wrapper call: first-party attested at both sites; natural (non-forced) inlining.

## Header blast radius (independent sweep, sweep/sweep.py + sweep.txt)
- Includers of interface/first_person_weapons.h: exactly 10 consumer .c files (no header includes it) = worker's list.
- Each compiled with the production header and with patch A's header: gate rows identical, every non-debug section
  section_infos_equal, symbol tables identical (ignoring $L labels) for all 10. Zero blast.

## Other checks
- objdiff 3.3.1 (build/tools) on the regenerated split: A update 95.21506 (park value), prod 93.48387 (current park),
  B update 100.0; data 536/536 all three. message/next_state 87.93/76.38 are credited via semantic_report accepted_ledger
  (semantic-coff) today; bytes unchanged by the patch, so no new semantic_matches entries needed.
- tools.parked_functions.validate_parked_functions on a mini project (A candidate + regenerated split + A park entry):
  active 1 / stale 0 / invalid 0.
- /W3 /Zs (w3r.py, patched header confirmed by /showIncludes): zero warnings in the candidate file for A and B; the
  remaining warnings are pre-existing real_math.h/cseries.h lines (B adds real_math.h:810 because magnitude3d is no longer external).
- fake_match_scan: production 0 leads; A/B 1 lead (empty-then-else) - authenticated above.
- nodup_census: _magnitude3d/_magnitude_squared3d/_square_root have 63/93/83 SELECT_ANY definers, 0 NODUP.
- My A/B objects are section-identical to the worker's final_landable_t.obj / final_exact_t.obj.

## Minor notes (non-blocking)
- gate.py copies --source to scratch/_gate_<pid>.c, so a patched header must be redirected via --edits (worker's *_t.c
  variants did this; plain final_landable.c gated alone compiles against the production header).
- Park evidence cites untracked scratch paths (precedent exists in parked.json); the integrator's docs ledger should carry it.
- Pre-existing, out of patch scope: `animation_update_render_only` body passes literal `0` instead of
  `animation_update_kind_render_only` (house rule 16); candidate for a separate byte-inert cleanup.

## Verdict
APPROVE the verdict FUNCTIONS_EXACT_OBJECT_BLOCKED. Patch A (production.patch) is landable now at zero credit: semantic
fix + attested structure + double-attested storage fix, zero regressions in the TU, its 10 header consumers and the split.
Patch B (owner_gated_exact.patch) is technically verified (34/34, audit PASS after regen, surplus identical, provider link
PASS) but MUST NOT be landed without an owner ruling admitting the authentic unguarded dereference.
