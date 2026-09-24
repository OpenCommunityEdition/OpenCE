# players (wave 3 worker, lane claude/fifty-objects-20260925, tree 954eebd2)

Targets: `_player_examine_nearby_device` (176), `_player_teleport_internal` (1296).

## Baseline
- gate --all (production): exact 68 residual 2 (device [size 160!=176, sha]; teleport_internal [size 1312!=1296, relocs 62!=61, sha]).
- t1 (opus5-150k w1 tele_t1_fn.txt spliced into current players.c): teleport [size 1280!=1296, sha], all else unchanged.
- pdb_storage (production): 5 disagreements vs cachebeta publics: _machine_to_player_table, _player_examine_nearby_unit
  (split 2 ours 3), _player_handle_powerup_equipment, _player_teleport_internal, _players_static_data (absent from publics
  => January file-static; ours external).
- /Od build: player_teleport = 0x5d2ab0; it calls player_teleport_internal at 0x5d2b70 DIRECTLY (not via ILT thunk)
  => static in the later build too (consistent with cachebeta: not public).

## Prior negatives (read, do not repeat)
- device: bare paren / macro paren (owner REFUSED RB2), const ptr, nested if, (real) cast, named local before/after camera
  call (frame 0x10), TU-local static accessor (frame 0x10), object_datum view, object_get_bounding_sphere helper (relocs 10),
  volatile.
- teleport: v2..v9, t1, tp2 if/else fwd/up, x3 result=FALSE first, x4 biped_get(player->unit_index), f1, dead-local D=1..8,
  loop arg source vs root index, HCEA LOS local, function-scope locals (0x84), separate trigger index, adj index scope,
  player_unit_index decl order.

## New evidence (this wave)
- /Od 0x5d2b70 (teleport_internal; aligned-frame prologue, od_linear). RTC descriptor at 0x5d329c names:
  best_adjustment_vector(-0x30), adjustment_matrix(-0x78), anchor_point(-0x94), new_position(-0xac),
  random_offset(-0xc0), forward(-0xe0). HCEX.pdb: player_teleport_internal is `static`, same 6 aggregates plus
  `adjustment_weights` as a STATIC LOCAL (const real_point3d[9]).
- /Od statement shape: player,player_unit_index,biped,result(byte) ; assert(src==NONE||count>1);
  if (src!=NONE && parent(src)!=src) { root_index=parent(src) [-0x18]; <dead named local> = unit_get(src) [-0x1c];
  root=object_get(root_index) [-0x20]; best=root->velocity; best.k=0; src=root_index;
  if (!(magnitude_squared3d(&best)>0)) { fwd.k<0.707 ? copy fwd : copy up  (two copies in /Od) } best.k=0;
  radius [-0x38]=biped_def->collision_radius; scale [-0x80]=radius*3+root->bounding_radius; assert x2; negate; normalize;
  matrix4x3_from_point_and_vectors(&adjustment_matrix,&root->center,&best,global_up3d); matrix.scale=scale;
  for (short i=0; i<NUMBEROF(w) && !result; i++) { transform_point(&m,&w[i],&anchor_point);
    result=biped_fix_position(player_unit_index, src, &anchor_point, NULL, 2.f, 0,0,1);
    if (!result) for (short j=0; j<8 && !result; j++) { random_offset=*global_zero_vector3d; random_direction3d(&random_offset);
      point_from_line3d(&anchor_point,&random_offset,radius,&new_position)  [REAL CALL 0x42e0d0: v*t+p];
      result=biped_fix_position(player_unit_index, src, &new_position, ...); } } }
  else result=biped_fix_position(player_unit_index, src, position, ...);
  player->cluster_index=NONE;
  if (result) { scenario=global_scenario_get(); assert(unit_index!=NONE); for (short k...) {... result=FALSE; break;} }
  if (result) { biped->velocity=*zero; if (src!=NONE) { forward=unit_get(src)->forward; source_biped=biped_try_and_get(src); ...facing }
                if (src!=NONE) { effect index ...; players_compute_combined_pvs; effect_new_from_object } }
  else { error(2,...); assert(local_player_index!=NONE); player_pseudo_kill(player_index, src); }
  return result;
- House-rule defect in production AND t1: the random position is a hand expansion of point_from_line3d (rule 6).

## Probes
| id | hypothesis / evidence | change (vs t1) | result |
|---|---|---|---|
| s1 | HCEX/cachebeta static storage might alter codegen | `static` on definition (header proto kept, lab) | byte-identical to t1 (fe25fd9e) |
| p1 | /Od real call point_from_line3d (rule 6); inlined helper params change webs | helper call in random loop | [size 1280!=1296, sha] ee2a07fa, rowdist 226 (=t1); emits surplus _point_from_line3d COMDAT |
| pa | /Od for-conditions `i<N && !result` (not `if(result) break`) | loop headers | byte-identical to p1 (ee2a07fa) |
| o1 | full /Od-faithful shape: decl+init order, /Od RTC/HCEX names, two tail `if (src!=NONE)` blocks, inline respawn index | whole body | byte-identical to p1 (ee2a07fa) => all these spellings are inert |
| o2 | /Od dead named local `source_unit = unit_get(src)` ([ebp-0x1c] in /Od) | named dead local | byte-identical to o1 (inert; not kept) |

Device /Od readout: /Od 0x5cce00 (found via device_frontfacing 0x55fbe0 caller): locals player[-8], unit[-0xc], device[-0x10],
camera RTC[-0x20]; radius read straight from the field (later SSE build always stages float args via xmm -> not diagnostic).
No helper/macro evidence. Device remains owner-blocked (RB2 refused paren; laws_w3 C3).

Allocator oracle (Lane A res9/alloc-oracle copied to scratch/w/players/oracle, WT repointed; same C2.Dll sha 65de5daa):
o1 teleport: 24 decisions, R0-R3 all consistent. Greedy counterfactual search launched (greedy_o1.txt).
| o3 | HCEX: adjustment_weights is a STATIC LOCAL of teleport_internal | moved into the search block | teleport bytes identical (ee2a07fa) BUT symbol becomes `?adjustment_weights@?3??player_teleport_internal@@9@9`; January's is `_adjustment_weights` => January had it at FILE scope (HCEX form is a later revision). REFUTED for January. |
| d1 | loop construct (outer loop as do/while + if(result) break) | diagnostic | byte-identical to o1: VC7 loop depth is CFG-based, construct inert |

Allocator analysis (oracle on o1; rounds_dump/init_allowed/setpri_id in oracle/):
- webs: j (inner idx) id32 pri 128 = 2x4+4x32-8; result id23 123 (ebx only); root idx id16 87 = 6x5+2x16+2x32-39;
  player_unit_index id19 70 = 4x12+2x16+2x32+2x5+4x6-108; root ptr id6 33.
- January's colouring needs root idx=esi, j=edi, pui=edi outside the inner loop, biped=esi (entry+tail), root ptr/outer idx/
  source/player memory. Ours: j=esi (first pop, class order), root idx=edi, pui uncoloured, root ptr esi.
- setpri root idx:=130 -> root esi, j edi (as January) but entry/tail pieces still differ (dblind 49 unchanged, d 286->262).
- setpri root 130 + pui 129 -> pui allowed {ebx} only (edi statically excluded), worse (dblind 71).
- Under R0 (priority = refs x n_b x 2^depth - live-through) root idx > j would need inner-loop weight < 11.5 or extra inner-loop
  references of root idx: no source spelling found that does that; construct/scope/declaration changes are all inert.

### KEY FINDING (p1/o1): the /Od-attested point_from_line3d call fixes BOTH x87 orders
- Production and t1 (hand expansion) compute magnitude_squared3d(&best) as i*i then j*j and the 0x525 assert magnitude as
  i,j,k. January: j*j first (with the k store interleaved after), and j,i,k. p1/o1 (genuine `point_from_line3d(&anchor_point,
  &random_offset, collision_radius, &new_position)` in the random loop, as /Od 0x42e0d0 real call) reproduce January's j,i and
  j,i,k orders exactly (aln_o1.txt 0xa3..0xb5 and 0x12d..0x141). The inline's parameter temps shift the IL-local numbering (LAW M8).
  => the hand expansion is a byte-proven source defect (as well as a rule-6 defect). Remaining teleport difference is ONLY
  integer register colouring (+ its frame consequence 0x78 vs 0x7c).
- Caveat: o1 emits a `_point_from_line3d` SELECT_ANY COMDAT (players.obj January defines none). Ruling 20260921 #1 admits it
  only once the caller is strictly exact => o1 is not landable while teleport stays residual.

### Device: wrapper-macro census (NEW evidence; the laws_w3 C3 'refutation' is invalid)
- laws_w3 C3 refuted a parenthesising wrapper macro because collision_bsp `data->radius` and real_math's pill tests
  integer-copy. But the 100K wave-C/D lanes measured that parenthesising exactly those sites is INERT (xfer/cbsp_paren.c,
  xfer/real_math_paren.c). So those sites are consistent with a wrapper macro, not a refutation.
- Lab (TU-local, after the last #include) `#define fast_vector_intersects_sphere(point, vector, center, radius)
  fast_vector_intersects_sphere(point, vector, center, (radius))`, with the per-site decorations STRIPPED:
  * ai.c (LINE_OF_FIRE_PILL_SPHERE_RADIUS use removed): all sections identical to production (secdiff).
  * collisions.c (bare paren at 1580 removed): all sections identical to production.
  * decals.c, collision_bsp.c: identical. real_math.c (definition spelled `boolean (fast_vector_intersects_sphere)(`):
    84/84 exact, all sections identical.
  * players.c: `_player_examine_nearby_device` becomes EXACT (176), every other section identical.
  * bipeds.c: only `_biped_update_moving` changes (radius now x87-staged as January; row stays residual: 1077 vs 1080 insns
    of 1064, hunks 225 vs 212).
  * strip-only controls: ai 44->43 and collisions 19->18 exact (the decorations are load-bearing there today).
- A macro declares no C name (preprocessor only) -> no declaration-count blast radius; only the 7 invoking TUs are affected.
- Still OWNER-GATED: it is a parenthesising macro with no first-party name attestation (RB2 class). Reported, not landable.

### ALLOCATOR PROOF (greedy2_root130.txt): o1 is January modulo THREE allocator decisions
Seeded greedy on o1.c (inside the real C2.Dll via dbg32c FORCE; diagnostic only):
  setpri web id16 (root index, natural pri 87) := 130 (> inner index j id32 = 128)
  + N5  id=37 -> skip (a web the natural run colours; January leaves it in memory)
  + C21 id=28 eax -> ecx
  => _player_teleport_internal STRICT EXACT (size 1296, d 0, dblind 0, 417/417 insns).
So every non-allocator IL fact of o1 (statement order, calls, helper, loops, tail, frame objects) equals January; the whole
residual is register-allocator INPUT (priority/benefit/cost), i.e. an IL fact with no direct byte trace.
| j1 | inner index initialised at the top of the outer body (diagnostic) | block-scope `short j = 0;` | IL changes (zero-pinned esi pushes), 414 insns, frame 0x7c, rowdist 148 - NOT January (bytes differ); refuted |
| g1 | inner loop as goto loop (diagnostic) | | priorities identical (CFG-based depth) |
| h1 | TU-local inline wrapper for biped_fix_position (diagnostic, no evidence) | 3 sites | priorities identical (param copies coalesced before pricing); not landable anyway |
| seed j86 | setpri inner index 86 (< root 87) | allocator | root idx still chooses edi (cost), dblind 25 after C2 -> ordering alone insufficient |

Priority model check: root idx = 6x5 + 2x16 + 2x32 - 39. Only a larger weight of its def block (n_b 5 -> 12, e.g. more
flagged-live webs across the velocity/magnitude block) or extra in-loop references reaches >= 129 without changing bytes.
No first-party fact found that supplies either. STOP (methodology rule): 11 shapes, all byte-identical or IL-changing.

## Storage / admission audit (object level)
- object_audit on production: symbol `_player_examine_nearby_unit` storage split 2 / ours 3 (cachebeta: not public => static).
- pdb_storage: 5 January-static symbols; ours external for 4 of them.
- LANDABLE zero-byte packet (production.patch + config/symbols.json copy):
  * players.c: `static` on `machine_to_player_table`, `players_static_data` (no header declarations; all sections identical).
  * players.c: drop the consumer-local `extern short player_spawn_count;` (verbatim duplicate of main/main.h:192, already
    included) - all sections identical.
  * players.c: `#include "cseries/errors.h"` (genuine owner of `error`, removes the C4013 implicit declaration at the
    teleport failure path) - all code sections identical (only internal $L label names renumber).
  * symbols.json: `"static": true` on _player_examine_nearby_unit (2598), _players_static_data (22521),
    _machine_to_player_table (23014). MUST land together with the players.c statics (else audit gains 2 diffs).
  => after it, pdb_storage leaves only _player_teleport_internal and _player_handle_powerup_equipment.
- BLOCKED storage: teleport_internal / handle_powerup_equipment need their players.h prototypes moved into players.c's
  private block. Consumer sweep (73 TUs, /FIcseries.h + /FI shadow players.h, control vs moved, shadow/sweep_results.txt):
  the move flips _rasterizer_frame_statistics_draw, _race_engine_player_update, _race_touch_flag,
  _ai_test_ballistic_line_of_fire, _player_profile_3wide_list_update (declaration-count canaries) and 5 TUs did not build
  under /FI. Keeping the extern header prototype over a static definition is byte-inert in players.c (st1) but is an
  owner-gated form. => owner ruling needed.
- surplus: 13 existing COMDATs identical to January's selected copies; provider link PASS (wm.obj and o1.obj incl.
  _point_from_line3d). Data 100% (build/report.json).
- production.patch FINAL (ps3.c): data statics + duplicate `extern short player_spawn_count;` removal only; all sections
  identical to production (secdiff: only .debug$S). The cseries/errors.h include was dropped again: byte-inert for code but it
  renumbers internal $L labels (declaration counter moves) - no benefit, avoidable risk.
- owner_gated_wrapper_macro.patch: real_math.h wrapper macro + real_math.c `(fast_vector_intersects_sphere)` definition +
  ai.c LINE_OF_FIRE_PILL_SPHERE_RADIUS removal + collisions.c bare-paren removal; git apply --check clean; lab-verified
  byte-identical for ai/collisions/real_math/decals/collision_bsp, device EXACT, bipeds moving changes (still residual).
- teleport_o1_not_landable.patch: o1 body (sha ee2a07fa). Not landable: emits _point_from_line3d COMDAT while residual.
| nc K=0..32 | TU name-count oracle (opaque struct tags right before the function; diagnostic only) | o1 + K names | all 33 byte-identical (ee2a07fa): NOT a TU-count tie |

## FINAL STATE (wave 3)
- device: BLOCKED (owner). Exact with a parenthesised radius; new census shows ONE callee-wrapper macro in real_math.h
  reproduces all 11 January call sites and lets ai.c/collisions.c drop their per-site decorations (zero section change);
  macro is count-neutral. Patch: owner_gated_wrapper_macro.patch. Strip test fails by construction (owner RB2 class).
- teleport: residual; best body o1 (/Od + HCEX shaped, genuine point_from_line3d) has January's x87 orders and is EXACT
  under three forced allocator decisions (root-index priority above the inner random index, source entry web skipped,
  forward/up pointer temp in ecx). No source lever found (11 shapes + TU-count oracle K=0..32 inert). Not landable:
  its _point_from_line3d COMDAT is allowed only once the caller is exact.
- object admission: production.patch + config/symbols.json (3 static flags) zero-byte; remaining storage for
  teleport_internal / handle_powerup_equipment blocked by declaration-count canaries (shadow sweep) or owner form.
Reopen: teleport - an IL fact that raises the root-index web above the inner-index web (or makes the inner index choose
edi) without changing bytes; start from o1.c + oracle/greedy2_root130.txt. device - owner ruling on the wrapper macro.
- Confirmation (oracle/greedy3_c0edi.txt): seeding ONLY "inner index j chooses edi at the very first pop" (cost esi 0 = edi 0,
  a pure class-order tie) + N5 (source entry web skipped) + C21 (fwd/up temp eax->ecx) is also STRICT EXACT. So decision 1 is
  a zero-cost chooser tie in our IL: January must carry a cost/preference for esi on an uncoloured neighbour of j (or exclude
  esi) at that pop. Decisions 2-3 are likewise cost/benefit inputs. All three are allocator-input facts with no byte trace.
