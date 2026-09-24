# source/game/players::_player_teleport_internal (1296)

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/o1_fn.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/o1.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/teleport_o1_not_landable.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/od_tele_internal.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/oracle/greedy2_root130.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/oracle/greedy3_c0edi.txt"
]

## production_changes
None to land now; the improved candidate exists in scratch only. The best body is scratch/w/players/o1_fn.txt; applied to the tree it is C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/teleport_o1_not_landable.patch (git apply --check is clean, sha ee2a07fa). It cannot land yet: the genuine point_from_line3d call emits a `_point_from_line3d` SELECT_ANY COMDAT from players.obj. That copy is byte-identical and the provider link passes, but rule 6 / ruling 20260921 #1 admits the COMDAT only once the caller is strictly exact. Keep the production body until then.

## evidence
1. /Od shape. The /Od function is at 0x5d2b70 and has an aligned frame; I read it with od_linear. Its RTC descriptor at 0x5d329c names best_adjustment_vector, adjustment_matrix, anchor_point, new_position, random_offset and forward, which matches HCEX.pdb. /Od gives a single `&&` condition, `for (i<N && !result)` loops, two tail `if (src!=NONE)` blocks, and a REAL point_from_line3d call at 0x42e0d0 (v*t+p).
2. KEY: the hand expansion of point_from_line3d, used by production and by t1, is byte-wrong. With the genuine helper (p1/o1), January's x87 orders appear exactly: magnitude_squared3d(&best) runs j*j first, and the 0x525 assert magnitude runs j,i,k. The inline's parameter temps are what shift this (LAW M8).
3. o1 measures 1280 bytes with 61==61 relocations and 428==428 instructions. Production is 1312/62.
4. Allocator oracle (Lane A res9 copied, same C2.Dll sha): o1 becomes STRICT EXACT when three allocator decisions are forced (greedy2_root130.txt, greedy3_c0edi.txt):
   - the inner random index takes edi instead of esi. This is a zero-cost chooser tie, or equivalently root-index priority >= 129 against 128.
   - the source_unit_index entry web is skipped.
   - the forward/up pointer temp goes to ecx.
   So every other IL fact already matches January.
5. Inert or refuted spellings, all byte-identical to o1 or changing the IL:
   - s1: static.
   - pa: loop conditions.
   - o2: the /Od dead unit_get local.
   - d1: do/while.
   - g1: goto loop.
   - h1: inline wrapper.
   - TU name-count oracle K=0..32.
   - o3: HCEX's static-local adjustment_weights. Refuted, because it yields the symbol `?adjustment_weights@?3??player_teleport_internal@@9@9`, while January's is `_adjustment_weights`, a file-scope static.
   - j1: moving where the inner index is initialised changes the IL.

## blockers
The residual is integer-allocator input only: priority, cost and benefit. It has no byte trace. No house-rule-compliant source construct found produces the three decisions, and the methodology stop rule applied after 11 spellings.

## reopen_criteria
A first-party IL fact that gives an uncoloured neighbour of the inner index a cost or preference on esi (or raises the root-index web above it), skips the source entry web, and puts the forward/up temp in ecx. Start from scratch/w/players/o1.c together with oracle/greedy2_root130.txt and oracle/greedy3_c0edi.txt. When it goes exact, land o1 with its _point_from_line3d COMDAT under ruling 20260921 #1.

## task notes
1. The laws_w3 C3 refutation of a wrapper macro for fast_vector_intersects_sphere is logically invalid: the integer-copy sites it cites are exactly the sites where a parenthesis was measured inert. Owner packet: owner_gated_wrapper_macro.patch.
2. Teleport: the /Od-attested point_from_line3d helper is January's source. Replacing the hand expansion flips both magnitude x87 orders to January's. Any future teleport landing must use the helper and will emit a byte-identical _point_from_line3d COMDAT; the provider link already passes.
3. The allocator oracle (C2.Dll forcing) proves o1 matches January except for three allocator inputs. This is diagnostic only and not landable.
4. The shadow-header sweep ran with /FI force-includes (cseries.h plus the shadow players.h). That perturbs the base, and 5 TUs failed to compile that way. Before any players.h prototype move, the integrator should rerun it as a real full ninja.
5. o3 finding: HCEX's static-local adjustment_weights is a later revision. January's symbol `_adjustment_weights` proves file scope, so the current source is right.
6. Background workers (greedy, sweep) have finished. Their outputs are under scratch/w/players/oracle/ and scratch/w/players/shadow/.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/LEDGER.md holds the full record: every probe (hypothesis, evidence, change, result), the /Od and HCEX readouts, the allocator traces and the audits. The allocator tooling copied from Lane A (with its paths pointed at this tree) is in scratch/w/players/oracle/: oracle, greedy, greedy2, greedy3, setpri_id, prio_probe, rounds_dump and blinddiff.

Results:
- Baseline gate is 68 exact / 2 residual.
- **Device:** exact only with a parenthesised radius. A new census shows one real_math.h callee-wrapper macro reproduces all 11 January call sites and lets ai.c and collisions.c drop their per-site decorations with zero section change. It is owner-gated (RB2).
- **Teleport:** the /Od-attested point_from_line3d call fixes January's x87 orders, which proves the hand expansion is byte-wrong. o1 is exact under three forced allocator decisions, but no source lever was found and the stop rule applied.
- **Object storage:** the zero-byte packet (production.patch plus 3 symbols.json static flags) is ready. The remaining storage for the two header-declared functions is blocked by declaration-count canaries.

No tracked file was edited.
