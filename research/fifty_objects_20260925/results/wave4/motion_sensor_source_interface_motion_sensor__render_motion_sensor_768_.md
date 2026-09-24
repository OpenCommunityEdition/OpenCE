# source/interface/motion_sensor :: _render_motion_sensor (768)

## verdict
FUZZY_IMPROVED

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\cand_prod.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\od_render_motion_sensor.txt"
]

## production_changes
The rest of production.patch (== cand_prod.c, CRLF). In render_motion_sensor: (1) one `real weight` local holds the history weight and is then reassigned `weight = (real)(pow(1.0f - weight, 3.5) * 7.0 + 1.0);` (with a one-line comment); the separate `radius` local is removed and both render_blip calls pass `weight`. (2) pow is called directly instead of the power() wrapper. (3) HCEX local names `corner` (was center) and `blip_pos` (was point, in both blocks). (4) /Od declaration order: `short history_index;` before `real_point2d corner;`. Park re-baseline: 784/40, normalized sha 69f4678045fb7eb843cd57c0a6715f102f2e7982cdeed68468dda6be4250ed44 (was de9701add36aae94...). The integrator must re-measure objdiff%. The other two parks are unchanged by this patch (1136/44 sha 40e4819de2760a22..., 816/37 sha 5bae910d9c27cad0...). Surplus shrinks: `_power` is no longer emitted. provider_link --baseline finds no new surplus. The _point_from_line3d guard passes.

## evidence
NEW /Od evidence (0x6426c0, od_render_motion_sensor.txt). [ebp-0x34] holds (10-i)/10.0f; fade [ebp-0x38] = w*w; the pow result is stored back into [ebp-0x34] and pushed as render_blip's radius argument. /Od never merges locals, so the source uses ONE variable for both. January's own bytes agree: the radius store `fstp [ebp-8]` reuses the weight slot, and the inner trip counter sits at [ebp-0x20]. The two-variable form produced the [ebp-8]/[ebp-0x20] slot swap that every prior wave carried; probe r1 removes it (alndiff blocks 34 -> 32). /Od calls a pow<float,double> template (0x62fcb0), so the source called pow(x, 3.5) directly; render_blip in the same TU already does. HCEX.pdb names the aggregates corner and blip_pos. Inert probes (all recorded): r2 /Od (active+10-i) operand order; r4/r5 `/ MOTION_SENSOR_HISTORY_COUNT` and `/10.0f` (a lab shows VC7 folds both to fmul 0.1f); r3 long history_index (contradicts /Od short, and worse); r6 declaration order; r7 direct pow; r8 single-exit nested if (byte-identical to r7, not proposed because it touches the label next to the stack-check block); d1-d3 statement orders (farther); the name-count oracle K=0..50 gives one identical sha. Diagnostics: e1 shows the `sub eax,[ebp-0x18]` fold and the idiv-first order appear exactly when no x87 op is in the block, so the residual is x87/integer interleaving. e2 removes the whole stack sentinel and the residual is unchanged, so it does NOT depend on check_stack_buffer.

## blockers
Remaining machine difference, all in the loop-head block. January emits `sub eax,dword ptr [ebp-0x18]` and finishes add/cdq/mov ecx,10/idiv plus the player reload before the x87 chain (fmul 0.1f; fst; fmul; fstp). Ours emits `mov edx,[ebp-0x18]; sub eax,edx` and interleaves the x87 chain into the integer statement. That changes code offsets by 2 bytes, which then adds two loop-alignment pads (0x11a, 0x19a) and a 5-byte `jmp`+lea pad in the stack-check loop; the pads are offset-driven only. It is a scheduler tie that survived every /Od-attested spelling. Methodology stop rule reached (8 shapes).

## reopen_criteria
A decoded C2 scheduler rule, or first-party evidence, for why January does not interleave the x87 weight chain into an idiv statement: for example a C2 trace of the loop-head block's list scheduler, or a corpus function with the same fild-IV + idiv + x87 block that we already match. Do not respend on statement order, index spelling, division form, loop-counter width or name/local-count oracles; all are measured inert.

## task notes
Landable now: production.patch. It is zero exact credit: a structural fix to _render_motion_sensor (the slot swap is gone; the park re-baselines to 784/40, sha 69f4678045fb...), one less surplus COMDAT (`_power`), and a .bss symbol-order fix, so object_audit now shows 0 of 44 January symbols wrong. The patch touches only motion_sensor.c: no header, no consumer sweep, and no symbols.json change.

Dependency on the stack sentinel: none of the three residuals depends on check_stack_buffer (lab diagnostics e2 and e3 remove the whole sentinel and the residuals stay). The only sentinel-area byte difference in render is an alignment pad driven by code offsets. The object still needs the hud_stack_check lane's check_stack_buffer packet before it can be admitted.

Held by owner: owner_gated_full.patch gives exact 17 / residual 2. After an owner ruling, the only open rows would be the render scheduler tie and the update reassociation tie.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\motion_sensor\LEDGER.md. It records every probe with hypothesis, evidence, change and result: r1-r9 for render, u7-u10 for update, the m4/m5 re-verification, lab e1-e3, d1-d3, L1-L9 and bss1, and all name/local-count oracles. /Od dumps are in od_*.txt. The tree was clean before and after (`git status` empty); no tracked file was touched.
