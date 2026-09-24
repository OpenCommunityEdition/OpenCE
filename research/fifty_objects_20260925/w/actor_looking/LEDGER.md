# actor_looking worker ledger (lane claude/fifty-objects-20260925)

## 0. Start (fresh; slug dir did not exist)
- Baseline gate (production build/base): 14 exact / 2 residual / 0 unwritten.
  residual 608 _actor_look_idle_find_prop [sha]; residual 4720 _actor_look_update [size 4704!=4720, sha].
- Read: WORKER_BRIEF, house rules, ledgers evidence_closeout_20260919, astra_90pct packet + research10,
  150k_w1 (find_prop: (int) probe + single-exit `found` flag both IDENTICAL -> do not repeat),
  lane A rejected hypotheses (find_prop: fcos-gap load-fold + jg@0xdf near-vs-short width anomaly;
  wave H: width decisions are non-local).
- find_prop sbs: only diffs = (a) +0x8d cmp ebx,[_ai_debug+0x38] (Jan) vs mov eax,[..];cmp ebx,eax (ours),
  (b) +0xdf jg near (6B, Jan) vs short (2B, ours); identical otherwise. Jan meaningful 0x25a, ours 0x257.
- /Od find_prop located at 0x45dbd0 (callee of 0x45e130/0x45e720 = idle major/minor): single-exit
  `found` byte flag [ebp-0x16], `valid` byte inited FALSE at loop-body top, debug compare
  `mov eax,[ebp+8]; cmp eax,[ai_debug]` (actor_index on left). Later revision adds time-since local.

## 1. Probes
- P0 baseline candidate (copy of production) -> find_prop sz608 REAL1/16 ins203; update sz4704 REAL119/197.
  Tools: scratch/w/actor_looking/score.py (gate + per-fn REAL region count), namesweep2.py (lab oracle).
- P1 LAB name-count oracle (tu-name-count-oracle law): K=0..40 opaque `struct lab_name_N;` inserted right
  before find_prop's definition -> BOTH residual sections byte-identical to baseline for every K (sha unchanged).
  Oracle NEGATIVE for this TU: the residuals are not arena-ordinal ties. (lab only, never lands)
- P1b LAB name-count oracle redone with insertion immediately before the find_prop DEFINITION (K=1..16) and before
  the update DEFINITION (K=1..24): all byte-identical (both shas unchanged). Oracle NEGATIVE, closed.
- L1/L2/V1/V2 LAB mechanism probes for the find_prop +0x8d fold (never land):
  L1 cos block replaced by constant stores -> `cmp ebx,[_ai_debug+0x38]` FOLDS.
  L2 straight-line cos (no if/else) -> FOLDS (scheduler interleaves 5 insns in fcos shadow).
  V1 arms swapped (`!= combat`) -> same split as baseline (REAL3). V2 arm B [1] uses sin (no tail merge) ->
  still split (`mov eax,[g]` in join, fstp, `cmp ebx,eax`). => split is caused by the x87 value carried
  live into the join block (sunk store of look_delta_deviations[1]), not by tail merge.
- S1 /Od-attested inverted state test `if (state<2 || state>3 || !visibility) {clear} else if (interest>0)`:
  byte-IDENTICAL to baseline (sha b145a6e8). S2 /Od-attested `boolean valid = FALSE;` at loop-body top:
  byte-IDENTICAL. Both inert (neither fold nor jg width moves).
- J6 LAB `[1] = cos(combat ? a1 : b1)` (join holds x87 value) -> still split. J8 LAB compare against another
  ai_debug int field -> still split (same normalized sha). The split is robust whenever an x87 value is live
  into the compare's block; January folds with the identical join bytes -> no source spelling of the check
  itself reaches it (consistent with recorded operand-order / (int) negatives).
- MINI LAB (scratch/w/actor_looking/mini/, run.py): m11 = minimal reproduction of the find_prop join
  (if/else cos arms + `if (actor_index == g.sel)` with actor_index in a callee-saved reg) -> SPLIT like ours.
  m13 straight-line -> FOLD. m16 check duplicated in both arms -> still SPLIT (IL-level tail merge).
- KEY EVIDENCE (/Od): every cos in the /Od actor_looking TU (find_prop 0x45dbd0 x4, update 0x45f5d0 x4 +
  stationary) is a CALL to the float->float wrapper 0x455220 (push ecx; movss [esp] -> 4-byte float arg,
  result fstp from st0), which chains to 0x4553b0 = `(float)cos((double)x)`. I.e. the first-party source spells
  `cosine(angle)` (real_math.h `__inline real cosine(real angle) { return cos(angle); }`), NOT `(real)cos(...)`.
- m26 = m11 with real_math.h-style `cosine()` inline -> `fcos; cmp esi,[g]; fstp; jne` = JANUARY's exact fold.
  Hypothesis H1: January's actor_looking uses cosine() (header inline) for the look_delta_deviations cosines.
- **C1 (production candidate c1.c): find_prop's four `(real)cos(` -> `cosine(` => _actor_look_idle_find_prop
  STRICT EXACT** (gate: exact 15 / residual 1). Both the +0x8d fold AND the +0xdf jg near width close together
  (lane A's "one root cause" hypothesis confirmed). update unchanged (sha 0ecf37ee).
- C2 update look-delta cos -> cosine(): update byte-INERT (sha 0ecf37ee). C3 + stationary `real cosine = (real)cos(..)`
  -> `real stationary_cosine = cosine(..)` (local renamed: a local named `cosine` would shadow the helper; /Od
  0x4611d3 attests a cosine() call there): update INERT. C4 + test_validity cosine(): stays EXACT.
  C5 + find_random_vector `(real)sin/cos(pitch|yaw)` -> sine()/cosine() (/Od 0x45d795.. attests 0x455220 cosine
  + 0x455320 sine wrappers, pushed right-to-left): stays EXACT.  C5 = 15 exact / 1 residual (update).
- C5 surplus: candidate newly emits `_cosine`, `_sine` COMDATs (systemic header-inline class; base already emits
  them in 31 objects). Both 16 B / 0 relocs, section_infos_equal with January's selected provider
  (build/split actor_combat.obj) = True; provider_link.py vs baseline: `_cosine` PASS, `_sine` PASS (both orders).
  _point_from_line3d guard passes.

## 2. update (after C5; update still 4704, REAL119)
- /Od update 0x45f5d0 read in full vs source. Differences found (later revision vs ours):
  (a) primary accepted block tests `looking_free` ([ebp-0x5f]) not `can_look` before copying looking vector;
  (b) `can_look` initialised TRUE at declaration; asleep arm `looking_enabled = can_look = FALSE` (chained);
  (c) override case chained `succeeded = change_facing = TRUE` and `succeeded = change_facing = face_exactly = TRUE`;
  (d) secondary bodies also store `facing_optional = FALSE`; override change_facing arm stores `facing_locked = FALSE`
      (both dead after the switch -> no January byte trace);  (e) write-only byte local [ebp-0x61] (later build).
  January's third secondary branch tests [ebp-4]=can_look (bytes 0x66f) -> keep `can_look` there (later build differs).
- U1 (a) `if (looking_free)` in primary block: update byte-INERT.  U2 (b) `can_look = TRUE` init + chained asleep
  assignment: INERT.  U4 (c) override chained assignments: INERT.  U5 (d) dead facing_optional/facing_locked stores:
  INERT. (all on top of C5; update sha 0ecf37ee unchanged). None kept.
- Stationary non-flying 2D path (Jan 0xfc4): January reads facing.i/.j via the CSE'd facing pointer ([eax],[eax+4])
  and aiming via [ebx],[ebx+4]; ours reads .i via pointer but .j as direct fields [esi+0x5a8]/[esi+0x5b4].
  /Od 0x46127d.. attests AGGREGATE copies (integer mov pairs) into RTC/HCEX-named locals desired_facing,
  desired_aiming, stationary_facing, and dot_product2d(&desired_facing,&stationary_facing),
  dot_product2d(&desired_aiming,&stationary_facing) argument order (ours: (&fixed2d,&facing2d), (&aiming2d,&fixed2d)).
  Second inline dot product x87 operand order differs from January at Jan 0x104f.
- Va LAB: /Od-attested aggregate view copies `real_vector2d facing2d = *(real_vector2d *)&actor->control.desired_facing_vector`
  (x3): stationary copy region now reads through the facing/aiming pointers like January (register names differ);
  update 4688 padded / 4686 meaningful (Jan 4714; C5 4690), REAL132 (register-renaming cascade).
- Vb LAB: Va + /Od dot_product2d argument order (&facing,&fixed),(&aiming,&fixed): BOTH inline dot products now
  x87-operand-identical to January (Jan 0x1037..0x105b); REAL131; still 4688. Vc (arg order alone): byte-INERT.
  Vb is structurally closer in the stationary block but NOT landable (view-cast rule requires a strictly exact caller).
- update residual root (unchanged): callee-saved colouring swap from 0x32f on (Jan: pp->edi, zero pin->ebx,
  aiming/facing pointer reload webs->ebx; ours: pp->ebx, zero->edi, pointer reloads->edi/edx; Jan homes
  secondary_aim_valid at [ebp-7] while ours keeps it in bl) plus frame-slot order (facing_locked -8 vs -7,
  aiming_cosine/primary_priority -0x20/-0x24 swapped). STOP RULE fired for update (9 evidence shapes, all inert or
  register-only).

## 3. Final
- PRODUCTION CANDIDATE: scratch/w/actor_looking/cand_final.c (CRLF preserved; == c5.c modulo line endings);
  patch scratch/w/actor_looking/production.patch (git apply --check clean). All 15 `(real)cos(`/`(real)sin(` in the TU
  -> real_math.h `cosine()`/`sine()` (/Od-attested at every site), stationary local `cosine` -> `stationary_cosine`.
  gate: 15 exact / 1 residual (_actor_look_update 4704 vs 4720) / 0 unwritten; _point_from_line3d guard PASS.
  NEW STRICT EXACT: _actor_look_idle_find_prop (608 padded / 602 meaningful / 20 relocs).
- Audit: object_audit FAIL(1) = only _actor_look_update (base was FAIL(2)); 80 January symbols, 0 differ;
  pdb_storage 0 disagreements; data 100% (report.json 1700/1700); new surplus `_cosine`,`_sine` (16 B each)
  section_infos_equal to January's selected provider actor_combat.obj and provider_link PASS both orders.
  /W3: only the pre-existing C4013 game_time_get (recorded byte-inert by 150k_w3).
- Minimal alternative (if the integrator prefers the smallest diff): C1 = find_prop's four cos only (same surplus).
- Parks to retire: none (no actor_looking park). Object remains blocked by _actor_look_update.
