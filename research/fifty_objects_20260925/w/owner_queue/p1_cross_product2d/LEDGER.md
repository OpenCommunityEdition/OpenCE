# Owner-queue packet: actor_combat P1 cross_product2d (wave 5, base cdc8ebd3)

Slug: scratch/w/owner_queue/p1_cross_product2d/. Nothing lands. Evidence source:
research/fifty_objects_20260925/w/cross_product2d/ (wave-2 slug, base 931ed8dc) + results/wave2/cross_product2d_*.md.

## Steps
- S0 (read): WORKER_BRIEF.md sections 0-10; held-items table row "actor_combat P1"; wave-2 LEDGER.md; the
  wave-2 patch actor_combat_cross_product2d.patch (drops #define/#undef cross_product2d_inline, drops the
  NODUP hand copy, restores the /Od call with a (real_vector2d const *) view cast of input.facing_vector).
  git status clean at cdc8ebd3.
- S1 (rebase): source/ai/actor_combat.c is unchanged 931ed8dc..cdc8ebd3 (git diff --stat empty for it).
  `git apply --check` of the wave-2 patch at cdc8ebd3: clean. Copied as patches/01_actor_combat_cross_product2d.patch.
  Applied to a CRLF copy of the tree file (src/actor_combat_p1.c): byte-identical to wave-2 p1.c; the strip
  variant (cast removed, src/actor_combat_p1_strip.c) byte-identical to wave-2 p1_strip.c. No rebase change needed.
- S2 (actor_combat gate at cdc8ebd3, gate.py --source ... --all):
  base (tree source)  32 EXACT, residual _actor_aim_grenade [size 576!=544, sha], _actor_combat_plan_grenade_trajectory [sha]
  P1                  identical rows (32 EXACT + the same 2 residuals)
  P1 strip (no cast)  identical rows
  Keyed raw-COFF diff (slug tool rawcoff.py --keyed: sections keyed by owner symbol, bytes+relocs+flags+COMDAT
  selection, $L label names normalised, .debug excluded):
    base vs P1        : 1 difference = sym:_cross_product2d COMDAT selection NODUPLICATES -> ANY (88/88 sections)
    P1 vs P1 strip    : 0 differences  => the (real_vector2d const *) view cast is byte-inert (strip test)
    gate base vs build/base/source/ai/actor_combat.obj: 0 differences (production obj is current)
  wave-2 objeq.py base vs P1: 81 sections compared, 0 changed, identical symbol sets.
  Note: January's split lists EVERY code COMDAT as NODUPLICATES (csplit artifact; e.g. _add_vectors3d NODUP in the
  split vs ANY in our build), so NODUP->ANY moves _cross_product2d into the same state as every other header inline.
- S3 (warnings, /Zs /W3 real cflags via wave-2 warn.py): base 17, P1 17 (identical multiset), P1-strip 18 (+1 C4133
  real_vector3d* -> const real_vector2d*). The cast is type hygiene only.
  Park fingerprint _actor_combat_plan_grenade_trajectory 288/5/ff9a7477... unchanged in base, P1, strip (matches
  parked.json measurements.base). _actor_aim_grenade 576/22/e9bba5b0 unchanged (the held residual; not moved by P1).
  object_audit actor_combat: base and P1 identical output, FAIL(2) = the two pre-existing residual functions only;
  60 January symbols, 0 differ. actor_combat is NOT completed by P1 (aim_grenade + plan_grenade_trajectory + the
  4 pre-existing candidate-only _actor_combat_*_inline externals with no January provider).
- S4 (five consumers, TREE source at cdc8ebd3, no source change needed; gate --out objects keyed-identical to
  build/base, so build/base is current for all five):
  path_obstacle_avoidance gate 24/24 EXACT; object_audit PASS (52 Jan symbols, 0 differ, 18 surplus); pdb 0; surplus 11 code COMDATs 0 not identical
  path_smoothing          gate  6/6  EXACT; object_audit PASS (15, 0, 15 surplus); pdb 0; surplus 10/0
  real_math               gate 84/84 EXACT; object_audit PASS (134, 0, 60 surplus); pdb 0; surplus 35/0
  collision_features      gate 20/20 EXACT; object_audit PASS (30, 0, 38 surplus); pdb 0; surplus 29/0 (incl. _projection_from_vector3d IDENTICAL vs January decals)
  path_structure_bsp      gate  6/6  EXACT; object_audit PASS (13, 0, 15 surplus); pdb 0; surplus 10/0 (HCEX widths/heights tables already landed in b9a8d587; split current)
  provider_link (production actor_combat): each FAIL(1) = _cross_product2d only (LNK2005 vs NODUP hand copy).
  provider_link (provlink2 --subst=source/ai/actor_combat:obj/actor_combat_p1.obj): all five PASS, every row.
  report.json (objdiff 3.3.1): all five code 100% / fn 100% / data 100% (9570, 348, 976, 896, 200 data bytes).
  tools/audit_object_admission.py (production report): path_obstacle_avoidance, path_smoothing, path_structure_bsp,
  real_math = "audit-coff-ownership-before-admission" candidates; collision_features = "object-admission-rejected"
  (_projection_from_vector3d entry, which is STALE: surplus identical to January's decals copy, link PASS).
- S5 (board census of _cross_product2d definers, census_cross_product2d.py): 11 definers; all 10 non-actor_combat
  copies are SELECT_ANY and section_infos_equal to January's actor_combat copy; every one FAILS the pair link with
  production actor_combat and PASSES with P1: actor_looking, path_obstacle_avoidance, path_smoothing,
  path_structure_bsp, decals, geometry, real_math, collision_bsp, collision_features, bipeds.
  Remaining link failures after P1 (provider_link full surplus): actor_looking PASS, decals PASS, collision_bsp PASS,
  geometry FAIL(1) _plane2d_from_points, bipeds FAIL(1) _object_get_type. None of these five is function-complete:
  actor_looking 15/16 (_actor_look_update), decals 31/33, geometry 27/30, collision_bsp 27/30, bipeds 42/51.
- S6 (admission bookkeeping): patches/02 flips the five units to Matching (textual, same regex as admit.py);
  patches/03 retires the STALE collision_features `_projection_from_vector3d` rejection (json.dumps(indent=2)
  round-trips the blob byte-identically, so the patch equals what admit.py --drop-rejection would write).
  patches/03b = ALTERNATIVE to 03 that also retires the decals `_cross_product2d` entry (its premise "January's
  selected Actor Combat copy is no-duplicates" is false after P1; decals' copy is identical and pair-links; decals
  stays NonMatching on 2 residual functions either way). 03 and 03b touch adjacent JSON entries: apply ONE.
  git apply --check at cdc8ebd3: 01, 02, 03 each clean; 01+02+03 together clean; 01+02+03b together clean.
  Sandbox apply (GNU patch on cdc8ebd3 blobs, scratch/.../sandbox): Matching statuses 497 -> 502, rejections 8 -> 7,
  actor_combat == P1. Emulated admission audit (report copy with complete=True for the five, rejections with 03):
  summary candidates 7 / contradicted 0 / rejected 1 (models) / revoked 0 - none of the five contradicted.
  tools/fake_match_scan.py: actor_combat base 0 leads, P1 0 leads. No test under tools/test_*.py references these
  units or the rejection file. No symbols.json change needed (path_structure_bsp tables landed at b9a8d587; the
  split is current: object_audit sees _quantized_pathfinding_surface_widths 64/64 ok).
- S7 (alndiff _actor_aim_grenade, P1 obj): 177 vs 182 insns, first diff `sub esp,0x24/0x28`; January binds
  `lea ecx,[edi+0x174]` (&facing_vector) once at 0xe4 and reads both 2D products through it (0xea/0xf0 dot,
  0x10a/0x114 cross) = pointer-parameter inline signature; ours addresses [edi+0x174] directly (hand-expanded dot
  remains). Consistent with the aim_grenade lane's finding that only the /Od helper-call body (T2/G1) reaches
  January's register choice; P1 alone does not move aim_grenade (sha e9bba5b0 unchanged).
- S8 (deliverables): patches/01, 02, 03, 03b + APPLY_ORDER.txt; MEASURED.md; RULING.md (question, clause table,
  evidence for/against, ledger disclosure as it would land). No symbols.json copy (not needed).
  VERDICT: apply_ready = true. Every patch applies at cdc8ebd3; every measured claim holds. Owner YES ->
  5 objects COMPLETE (path_obstacle_avoidance, path_smoothing, real_math, collision_features, path_structure_bsp);
  actor_looking, decals, collision_bsp lose their only link failure but stay function-incomplete; geometry and
  bipeds lose _cross_product2d but keep _plane2d_from_points / _object_get_type conflicts. Integrator still owes the
  full batch gate (ninja, stable_verdicts, parks, admission, fake scan, pytest).
