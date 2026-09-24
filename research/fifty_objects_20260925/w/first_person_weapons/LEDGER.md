# first_person_weapons wave-2 ledger (lane claude/fifty-objects-20260925, base 931ed8dc)

Target: `_first_person_weapon_update` (January 1536 padded / 59 relocs / sha 912d9568...). Only non-exact row.

## Baseline
- base.c = production copy; gate --all: 33 exact / 1 residual (update: size 1520!=1536, relocs 60!=59, sha).
- Read: WORKER_BRIEF (incl. s7), house rules, lane log, all 7 fpw ledgers, park entry, prior worker notes
  /c/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/first_person_weapons.md (u1/u2/t1-t4 recorded; NOT repeated).
- Recorded negatives (do not repeat): definition reorder (inert), t1/t2 turning_i/j scope moves (frame stays 0x10),
  t4 = guarded base + aggregate (1520/60, frame 0x18).
- Recorded blockers: (a) REAL_MATH_EXTERNAL_MAGNITUDE3D forces a call (+1 reloc); (b) January has NO null test after
  `count ? TAG_BLOCK_GET_ELEMENT(...,0,...) : NULL` (moving + overcharged-jitter blocks) = authentic NULL-deref (owner-gated, brief s7).

## Probe R0 (t3r.c = prior w2 t3.c recompiled at 931ed8dc; unguarded + real_vector2d turning aggregate + no MAGNITUDE defines)
- gate: 33 exact / update residual [sha only]; 1536 padded, 59 relocs, frame 0x14 == January. Instruction count 469 vs 468.
- NEW FINDING (missed by all prior waves because alndiff normalises branch displacements): January @0x13c
  `cmp ax,1; je 0x150(SKIP); cmp ax,2; jne 0x150; mov ebx,[ebp+8]; call next_state` -- the ==1 branch SKIPS next_state.
  Ours `je 0x148` (to the call) = `r==1 || r==2`. The prior ledger claim "January accepts either result" is WRONG.
- /Od 0x61f9b0 (od_update.txt) @0x61fb2e: `movsx ecx,[r]; cmp ecx,1; jne else; jmp end; else: cmp 2; jne end; call next_state`
  = `if (result==_animation_key_frame) { <empty> } else if (result==_animation_will_restart_on_next_frame) { next_state(); }`
  (a switch in /Od would home the switch expression; this is if/else-if on the short). Double-attested with January bytes.

## Probe c1 (= t3r + `if (r==_animation_key_frame) { /* comment */ } else if (r==_animation_will_restart_on_next_frame) next_state();`)
- Hypothesis: January @0x140 `je 0x150` skips the call on key_frame; /Od shows empty then-arm + jmp. Expect 0x140 displacement to match.
- Result: 0x13c..0x150 now byte-identical to January (je 0x150). Still residual [sha]; remaining = register rotation ties:
  director arg eax/ecx, impulse_sound loads (T fwd=ecx,origin=edx,snd=eax,weapon=ecx; O edx,eax,ecx,edx), post-call `mov dx,[esi+0xc]`
  before add esp (T) vs store-eax-first + `mov ax` (O), unit_flying arg eax/ecx, animation_update_internal arg ecx/edx,
  moving elem temp edx/eax, jitter elem temp eax/ecx, fmod block (T edx=graph,ecx=idx; O edx=idx,eax=graph). Pattern: O = T rotated one
  step in eax->ecx->edx order at each site.
- /Od RTC descriptor 0x6204c0: sound_definition_index(4)@-0x24, forward(8)@-0x48 (later-build only), turning(8)@-0x70
  => `real_vector2d turning` aggregate attested by first-party name+size; triggered sound local's authentic name = sound_definition_index.
- /Od moving/jitter blocks: `animations= count ? TAG_BLOCK_GET_ELEMENT(...) : NULL; frame_index= 0; index= (1 && animations->animations.count>K) ? ... : NONE`
  -- the later first-party build ALSO dereferences without a NULL test (`mov eax,1; test eax,eax` = constant-true VALID_INDEX(K,count) half).

## Probe c2 (c1 + rename triggered_sound_index -> sound_definition_index, HCEX + /Od RTC name): byte-identical to c1 (text inert, as the law says). KEEP (authentic name).
## Probe c3 (c2 + /Od-attested dead `struct model *model= model_definition_get(...)` local, decl order per /Od -0x18): byte-identical to c1. INERT -> not kept (dead local, no effect).
- Residual fully localised: after 0x2fe every row differs only by the -1 address shift; the ENTIRE residual is register rotation in
  T 0x156..0x2d8 (director call .. fmod element fetch). O is T rotated +1 (eax->ecx->edx->eax) at EVERY site; the 1-byte size delta is
  O getting eax for `mov eax,[_global_origin3d]` (A1 short form). Impulse-sound post-call order (T hoists `mov dx,[esi+0xc]` above the
  store of eax) follows from the same register choice (O gives the state temp eax, so it cannot be hoisted).

## LAB L1: name-count oracle, K=0..24 dummy struct tags immediately before first_person_weapon_update (lab/pre_K.c)
- Hypothesis: the rotation is a TU arena-ordinal tie (tu-name-count-oracle). Result: update bytes IDENTICAL for all 25 K (sha1 ea775347bc);
  every other row unchanged. => NOT a name-count tie at this position; the residual is a real IL/web-structure difference. (lab only)

## Finding F2: /Od calls the TU-local wrapper, not animation_update_internal
- /Od 0x61fb22 and 0x61fc59: 3-argument calls to 0x61c0b0 (= this TU's first function, i.e. January `_animation_update_render_only`
  @0xCBC60, 32 bytes, defined at the top of the TU), not the 4-argument animation_update_internal. January bytes show the inlined
  expansion (push 0 kind + 3 args). Hypothesis H2: January's source calls animation_update_render_only(...) and VC7 inlines it; the
  inlined helper's parameter webs change caller colouring (wave-1 law b) -- a source-structure fact outside the allocator model.

## Probe c4 (c2 + both animation_update_internal(animation_update_kind_render_only, ...) calls -> animation_update_render_only(...))
- Evidence: /Od 0x61fb22/0x61fc59 call the TU wrapper 0x61c0b0 (3 args; body = push 0 + call animation_update_internal);
  January `_animation_update_render_only` (0xCBC60, external) has NO referencing object anywhere in build/split (only its own
  definition) => January's only user is first_person_weapon_update, where VC7 inlined it.
- RESULT: gate --all == exact 34 residual 0 unwritten 0. `_first_person_weapon_update` EXACT (1536/59).
  alndiff residue = only relocation-name representation rows (symbol vs defined-noncode .rdata literal), normalised equal by gate.
- c4 still contains the OWNER-GATED unguarded dereference (from prior t3) -> necessity/strip probes follow.

## Necessity n1 (c4 + the two production `first_person_weapon_animations &&` NULL guards restored) = the defined-behaviour shape
- gate: residual [size 1552!=1536, sha], relocs 59==59. alndiff (reloc/shift rows filtered): the ONLY differences are 4 inserted
  instructions -- `test eax,eax` + `je` after each of the two `count ? TAG_BLOCK_GET_ELEMENT(...) : NULL` merges. Everything else,
  including the whole former register-rotation region, is identical to January. => the sole remaining blocker of the object is the
  owner-gated unguarded dereference (January bytes + /Od later first-party build both dereference without a test).
## Negative n6 (c4 minus the remaining SIGNED_ANGULAR_DIFFERENCE / REAL_LOCAL_RANDOM / REAL_LOCAL_RANDOM_RANGE defines)
- residual [size 1552, relocs 60, sha] + surplus _signed_angular_difference/_real_local_random(_range) COMDATs. Keep those 3 defines
  (production state; January calls these out of line and the header's local-random body is not January's). Out of scope.
## Necessity n2 (c4 with the production turning_i/turning_j scalars instead of `real_vector2d turning`): residual [sha]; frame
  `sub esp,0x10` vs January 0x14 and every stack slot shifted -- the aggregate is the frame-structural fact (RTC `turning` size 8).
## Necessity n3 (c4 + REAL_MATH_EXTERNAL_SQUARE_ROOT/MAGNITUDE_SQUARED3D/MAGNITUDE3D restored): residual [1520, relocs 60] (magnitude3d call).
## Necessity n4 (c4 with the production `r==key_frame || r==will_restart` condition): residual [sha] (je displacement at 0x140).
## Necessity n7 (c4 with the STATE call back to animation_update_internal): residual [sha] -> the wrapper call is load-bearing there.
## Inert n5 (c4 with only the MOVING-block call back to animation_update_internal): EXACT. The moving-block wrapper call is byte-inert;
  keep it anyway because /Od 0x61fc59 attests the wrapper at that site too (consistency with the first-party source).

## Section-4 audit on c4 (owner-gated exact candidate)
- gate --all --forbid-emitted-symbol _point_from_line3d: 34 exact / 0 residual / 0 unwritten; guard passed.
- object_audit: PASS (all January sections + 45 symbols); surplus .text COMDATs _magnitude3d(48) _magnitude_squared3d(48)
  _square_root(16) + pooled .rdata literals.
- surplus_cand.py (candidate variant of surplus_identity): all three COMDATs section_infos_equal to January's selected copies
  (action_charge / action_alert / action_charge) -> PASS. provider_link.py c4.obj: SELECTED-PROVIDER LINK PASS (all surplus).
- fake_match_scan c4.c: 1 lead `empty-then-else` @ the key-frame arm -- authenticated by /Od 0x61fb2e (explicit empty then + jmp)
  and January @0x140 (je to the skip block); n4 proves it load-bearing. Production file: 0 leads.
## STORAGE finding (pdb_storage): `_first_person_weapon_message` (0xCD670) is ABSENT from cachebeta publics => file-static in January.
  No other January object references it (only _from_unit/_from_weapon in this TU); symbols.json line 3523 lacks "static": true.
- c5 = c4 + `static` definition + private prototype (after first_person_weapon_new_unit, January address order); header prototype
  removed (tested via shadow hdr/interface/first_person_weapons.h, c5t.c includes it by a scratch-relative path).
  gate c5t: 34/34 exact, guard passed, update bytes identical to c4. object_audit: only diff = storage 2(split, pre-regen)/3(ours).
- Header blast radius (hdr/consumer_sweep.py): all 10 consumer TUs (effects, particles, game, interface, weapons, object_lights,
  render, render_objects, render_particles, bipeds) compiled with and without the shadow header: every code/data section
  section_infos_equal; only .debug$S differs (scratch file name). Row summaries unchanged. => prototype removal is blast-free.

## objdiff 3.3.1 (frozen build/tools/objdiff-cli.exe, scratch project od_proj/) on candidates vs January split
- production base.obj: 28/34 matched, update 93.48387%.  c4/c5t: 29/34, update 100.0 (credited 1,533 B), data 536/536.
- message (544, 7 internal relocs) and next_state (368, 8 internal relocs) are strict-EXACT in production but objdiff 3.3.1 gives
  87.93 / 76.38 -- the same private switch-table/local-label false negative already pinned for state_from_weapon_message,
  animation_type_from_weapon_state and set_state in config/semantic_matches.json. Pre-existing under-credit; needs 2 pinned entries.
## Landable-now shape g1 (= c5 + production NULL guards + REAL_MATH_EXTERNAL_{SQUARE_ROOT,MAGNITUDE_SQUARED3D,MAGNITUDE3D} kept,
   so NO new COMDAT is emitted while the caller is non-exact, per rule 6)
- gate g1t: 33/1, update residual [1520!=1536, relocs 60!=59, sha]; guard passed; objdiff update 95.21506 (production 93.48387).
  Remaining vs January: magnitude3d out-of-line call (+1 reloc, frame 0x18 via CSE'd &unit->throttle slot) + the two NULL tests.
- n1 (guards, defines removed) = 99.139786 but emits _magnitude3d/_magnitude_squared3d/_square_root with a NON-exact caller -> not landable (rule 6).

## Third-build corroboration (HCEA 2011 PPC decompiled source, research/halocea/src/first_person_weapon_update.c)
- line 90: `if (animation_update_internal(...) == 2)` -> next_state ONLY on _animation_will_restart_on_next_frame (semantics of the fix).
- lines 138-145: `animation_set = count ? address : 0; frame_index = 0; if (animation_set->animations.count <= K) ...` -> the same
  unguarded dereference. January, HCEA 2011 and the later /Od build all read through the possibly-NULL pointer.

## /W3 (w3.py, gate flag extraction, sanity-checked to report C4013): base, final_landable, final_exact (real and shadow header): 0 warnings.

## FINAL CANDIDATES
- final_landable.c (production.patch = prodA.patch; git apply --check clean at 931ed8dc):
  static first_person_weapon_message (+ private prototype, header prototype removed, symbols.json "static": true),
  key-frame if/else-if, animation_update_render_only at both sites, `real_vector2d turning`, sound_definition_index;
  guards kept + MAGNITUDE defines kept (no new COMDAT). gate 33/1 (update 1520/60 [sha ba39884c...], objdiff 95.21506 vs 93.48387);
  every other section byte-identical to production. parked.json base measurement + evidence refreshed in the patch.
- final_exact.c (owner_gated_exact.patch = prodB.patch; git apply --check clean): final_landable minus both NULL guards (with BUG
  comments) minus REAL_MATH_EXTERNAL_{SQUARE_ROOT,MAGNITUDE_SQUARED3D,MAGNITUDE3D}; gate 34/0/0 EXACT, guard passed,
  object_audit PASS (pre-regen storage diff only for the static fix), surplus identity PASS, provider link PASS, objdiff 29/34 with
  message/next_state already credited by build/semantic_report.json accepted_ledger (semantic-coff). Retires the park.
  NOT landable without an owner ruling on reproducing the authentic unguarded dereference (brief s7).

## VERDICT: FUNCTIONS_EXACT_OBJECT_BLOCKED (owner ruling). The whole object is complete in owner_gated_exact.patch (34/34 strict,
data 536/536, 45 symbols incl. the PDB-proven static message, surplus identical and link-clean). The only blocker is the brief s7
owner-gated class "reproducing authentic January NULL dereferences" (two unguarded reads, triple-attested: January, HCEA 2011,
later /Od). production.patch lands now at zero credit: a semantic fix (key frames no longer advance the state), /Od-attested structure
and the storage correction.
