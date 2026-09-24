# source/game/players::_player_examine_nearby_device (176)

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/owner_gated_wrapper_macro.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/wm.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/wm/"
]

## production_changes
Nothing is landable without an owner ruling. The owner-gated option is a separate shared-header patch, C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/owner_gated_wrapper_macro.patch (git apply --check is clean). It makes four edits:
1. real_math.h: after the fast_vector_intersects_sphere prototype, adds a one-line macro that parenthesises the radius argument.
2. real_math.c: the definition is spelled `boolean (fast_vector_intersects_sphere)(` so the macro does not expand on it.
3. ai.c: removes the LINE_OF_FIRE_PILL_SPHERE_RADIUS macro and its comment. The call site becomes `pills[pill_index].width`.
4. collisions.c:1580: removes the bare parenthesis.
The TUs that call the function, which the integrator must sweep: ai/ai, effects/decals, game/players, math/real_math, physics/collisions, physics/collision_bsp, units/bipeds. The macro declares no C name, so other real_math.h includers should be unaffected; this was not swept.

## evidence
January stages the radius through the x87: `fld [edi+0x5c]; add esp,0x1c; fstp [esp]`. The /Od build (0x5cce00, found through device_frontfacing at 0x55fbe0) reads the field directly, with locals player, unit, device and the RTC local camera, and has no helper. As an SSE build it stages every float argument through xmm, so it cannot tell us anything here.

NEW census: laws_w3 C3 'refuted' a wrapper macro by pointing to the integer-copy callers (collision_bsp `data->radius` and the real_math parameters). Earlier lanes measured that parenthesising exactly those sites is inert (xfer/cbsp_paren.c, xfer/real_math_paren.c). So the refutation does not hold.

Lab: I defined the same macro locally in each calling TU and stripped each file's own decoration, then compared every section against production with secdiff.
- ai, collisions, decals, collision_bsp and real_math (84/84): all sections identical.
- players: `_player_examine_nearby_device` becomes EXACT; every other section is identical.
- bipeds: only `_biped_update_moving` changes. Its radius is now x87-staged as in January, but the row is still residual.
- Control: stripping the decorations without the macro loses one exact row each in ai (44->43) and collisions (19->18).

So one callee-level macro explains all 11 January call sites. Today three separate per-site decorations emulate it.

## blockers
Owner ruling RB2 refused a parenthesis that decorates without meaning. The strip test fails by construction: the macro only adds a parenthesis, and no first-party name attests it. The 2026-09-20 admission of ai.c LINE_OF_FIRE_PILL_SPHERE_RADIUS rested on a meaning distinction (a pill width read as a sphere radius) that this site does not have.

## reopen_criteria
The owner admits the callee wrapper macro (or the TU-local DEVICE_BOUNDING_SPHERE_RADIUS form Lane B held), or a first-party macro or accessor that parenthesises the radius is found. Then apply the patch and run a full sweep over the 7 calling TUs.

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
