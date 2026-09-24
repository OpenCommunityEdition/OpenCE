# interface/progress_bar :: _draw_gravy_layer

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\progress_bar\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\progress_bar\\g1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\progress_bar\\optional_gen_mask_coord.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\progress_bar\\st1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\progress_bar\\labv.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\progress_bar\\lab\\xvt2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\progress_bar\\jcmp.py"
]

## production_changes
None required. Do not apply any change for credit.

OPTIONAL, zero credit (authenticity only): C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\progress_bar\optional_gen_mask_coord.patch (candidate file scratch/w/progress_bar/g1.c). It adds a gen_mask_coord prototype to progress_bar.c's own prototype block, replaces the four TEXCOORD1 SetVertexData2f calls in draw_gravy_layer with gen_mask_coord(0,0 / 640,0 / 640,480 / 0,480, progress), and drops the mask_position local. VC7 inlines the calls. The bytes of _draw_gravy_layer, _gen_mask_coord, _progress_bar_draw and _draw_gravy_base_layer are verified identical to production, and gate stays at 53 exact / 1 residual / 0 unwritten.

No park to retire. No park should be added unless the owner wants one; the function stays an honest residual.

For the integrator, storage evidence outside this function's scope: pdb_storage shows that draw_fullscreen_blur, set_blur_texture_coordinates, draw_gravy_screen_layer, progress_bar_screen_initialized and progress_bar_rendering_enabled were file-static in January (absent from cachebeta publics), but ours are external. Probe st1.c made blur, set_blur and rendering_enabled static: all code stays EXACT, but object_audit (which uses the split's storage=2) then flags them. Making draw_gravy_screen_layer static drops its emission (UNWRITTEN), because it has no in-TU user. Not proposed as a change: it needs an owner ruling on split-vs-PDB storage.

## evidence
Baseline gate: 53 exact, 1 residual (_draw_gravy_layer, 336 vs 352, sha), 0 unwritten. Frame 0x1c on both sides; relocation multiset 22/22.

There is no /Od readout: the later build replaced the progress screen (fn 0x666670, string 'the old loading progress screen has been replaced with glowy halo gravy'). HCEX.pdb has none of these functions.

New findings:
(1) The 16 bytes come from January holding each rect corner in a NON-rematerialisable variable. Each corner is loaded into scratch, copied to a carrier (e.g. mov ebx,ecx; push ebx) and stored to a spill home. x0 is reloaded from home [ebp-4] at vertex 4. Ours loads rect.* directly and re-reads rect.x0.
(2) Lab harness scratch/w/progress_bar/labv.py (lab/v0 reproduces production bytes exactly) shows the cause. VC7 applies cross-call CSE and remat of rect fields only because generate_gravy_rect (defined in the TU) has a no-capture summary. With generate extern, or passing rect to an unknown function (gesc/gext), CSE disappears. Adding corner variables assigned at first use (xvt2) then reproduces January's exact carrier+home signature.
(3) The summary is robust to every source-level perturbation tested: definition order (l2/l3/tsa/tsl), K&R and void* prototypes and prototype-scope tags (knr/m1/m2), casts (cast1/2), a real[4] array (arr1/2), a union (u1_full), __inline to_screen or generate (ti/gi), an empty inlined helper taking &rect (emp0/1), pointer-alias defs (e1), register (e3), dead stores (va/ds1), point-struct forms (f5/f6), direct D3DDevice_* calls (vh/vi/vj) and a nested block (ve).
(4) January's emission order is a clean callee-before-caller source order (to_screen, generate, create_noise, gen_cloud, gen_mask, draw_layer_int, ...). So generate was compiled first, and the missing summary has no found source cause. Even without the summary, the lab allocation stays reversed: 640-mask gets ebx and the corners get homes, while January homes 640-mask and carries the corners (xnv1-4).
(5) The orphan public gen_mask_coord(x,y,m) reproduces every TEXCOORD1 value (480*0.033333335f folds to 16.f). Its inlined use (g1) is byte-inert.

Real-TU probes: s1, g1, t1, t2, u1_full, st1. Every probe is in scratch/w/progress_bar/LEDGER.md.

## blockers
The remaining gap is an IL fact: corner values that VC7 cannot rematerialise from rect, with a specific register allocation. Every admissible spelling found collapses to production bytes. The only lab route that shows the pattern (no no-capture summary for generate_gravy_rect) has no source-level justification. It also still allocates differently (640-mask vs corners for ebx). Pragmas, fillers and forced escapes are rejected by house rules and were not tried as landings.

Object-level: the function residual blocks completion. The pdb_storage static-vs-external disagreements (5 symbols) and the D3D inline-copy storage rows (split 2 vs ours 3, where ours matches the PDB) are separate from this function.

## reopen_criteria
Reopen if any of these appears:
(a) A genuine source construct (not a pragma, filler or dummy escape) that removes generate_gravy_rect's no-capture summary at the draw_layer_int call while generate and to_screen stay exact.
(b) A measured VC7 lever that blocks remat or forward substitution of a local copied from an address-taken, non-escaping struct.
(c) A C2 allocator replay (lane-a scratch/res9/alloc-oracle) that identifies which IL gives the corner webs priority over the 640-mask web. With the summary removed, January then follows from t2-style corner variables.
(d) First-party source or map evidence of draw_layer_int's body, or of the in-TU user of the January-static draw_gravy_screen_layer (that user is also needed for storage completeness).

## task notes
Tools written for reuse: scratch/w/progress_bar/mk.py (swaps the function body into a copy of the unit and gates it), lab.py/labv.py (standalone lab compile with progress_bar's exact cflags; head/body/tail composition; the LABHEAD, GENTAIL and TSTAIL env switches), jcmp.py (aligned instruction diff against January with call targets normalised), vwscan.py (board scan for the carrier+home pattern; draw_gravy_layer is unique). No tracked files were edited and no ninja, configure or git commands were run. The six recorded prior negatives (S1, S3, gA, gC, gD, gH) were not repeated as landings; only S1 was re-measured once to confirm the record.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\progress_bar\LEDGER.md
