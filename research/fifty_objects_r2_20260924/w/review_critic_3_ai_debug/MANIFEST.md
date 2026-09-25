# MANIFEST - review of critic_3 ai_debug rename packet (01 + optional 02)

- Base/tip: worktree HEAD 5d3ca708 (task text named 7b9de585). No tracked edit; no ninja/configure/git mutation;
  `git status` shows only the pre-existing untracked research/ dir. Claim: source/ai/ai_debug (review_critic_3_ai_debug).
- Verdict: APPROVE WITH AMENDMENT. Use `01_ai_debug_amended.patch` (this dir) instead of critic_3's 01; 02 unchanged
  (`02_ai_debug_extra_static.patch`, copied here, optional, applies after 01 or 01-amended).
- Amendment: moves the orphaned stub comment `/* Render the path-node overlays selected by the caller. */` from above
  `ai_debug_render_surface` to above the real `ai_debug_render_path_nodes` definition. Byte-inert (gate rows and
  object identical except .debug$S).
- Credit: ZERO (names/storage only). Object does not complete (render_actor residual + jmptable ownership).

## Evidence per construct
- 21 renames: Sept-2001 map static names, unique in-order pairing, sizes equal (20/21); January atlas 4cc87b45 agrees at
  the exact address for 19/21; the other 2 attested by the Jan-2002 xbe atlas (render_spatial_effects, exact) and
  Aug-2001 atlas + Sept map + sole caller (get_this_actor). No PDB public at any address; no contradicting literal.
- "static": true (21 + 11): source already static; cachebeta.pdb publics absent at every RVA; owner house rule
  (symbols.json rename + "static": true for file statics).
- Dead stub removal: never called, 7 unreferenced locals (C4101), community skeleton origin (37e96a1c "TODO: finish").

## Measurements (files in this dir)
| check | prod | 01 / 01-amended | 01+02 |
|---|---|---|---|
| gate (gate_*.txt) | 59/1 | 59/1, rows identical (rename-mapped) | 59/1 identical |
| objcmp vs prod | - | identical except $L numbering (stub removal; lab/stubless.c proves) and .debug$S | same |
| objdiff 3.3.1 (objdiff/) | 16446/42523, 58/61, 5818/5874 | identical, per-fn identical | identical |
| object_audit (battery/) | FAIL(34) | FAIL(13) | FAIL(2) |
| pdb_storage | 33 | 12 | 1 |
| surplus_identity | 19 / 0 not identical | same | same |
| provider_link (both orders) | PASS | PASS | PASS |
| emulated split (split_*) | ctl == build/split 833/833 | only ai_debug.obj differs: renames + storage 2->3 | + 11 storage flips |
| CL /W3 | 30 warnings | 23 (7 stub C4101 gone, none new) | - |

## Not run
- Full ninja, pytest, board-wide stable sweep (integrator). csplit regen required after landing.

## Follow-ups noticed (not in this packet)
- `void ai_debug_drawstack_setup(` prototype is extern while its definition is static (extern->static redeclaration);
  make the prototype static (inert expected) when ai_debug is next touched.
- ai_debug.c carries consumer-local prototypes `actor_iterator_new` / `actor_iterator_next` (house rule 9).
- Pre-existing C4013 implicit declarations: console_printf (line ~4254), error (~4327).
- Stale docstring examples naming `_code_00039990` / `_code_0003a910`: tools/campaign/real_regions.py,
  tools/emission_shape_oracle.py, tools/context_diff_oracle.py, tools/c2_diag_map.py; header comment
  source/ai/ai_scenario_definitions.h:313 still says `code_00041220` (= ai_debug_render_encounter).
- `_ai_debug_render_actor_jmptable` split storage 2 vs not-public (last pdb_storage row).
