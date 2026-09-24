# source/objects/widgets/lightning

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\storage.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\od_shape.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\owner_gated_best.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\prod\\lightning_c5.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\prod\\lightning.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\od_submit.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\lightning\\LEDGER.md"
]

## production_changes
No strict credit. Optional, zero-credit and byte-inert changes, reviewed separately from any function gain:
(1) STORAGE CORRECTION (recommended). Both sources agree: `_lightning_globals` is missing from the cachebeta publics, and HCEX.pdb reports `lightning_globals` as DataKind 5 = File Static with an unnamed struct type. Apply scratch/w/lightning/storage.patch (`git apply --check` clean at 931ed8dc). It changes lightning.c to `static struct lightning_globals lightning_globals = {0};` and removes the `extern` (and the empty globals section) from lightning.h. Then take config/symbols.json line 23106 from scratch/w/lightning/config/symbols.json. That line was edited in place to `{ "file_offset": 4550212, "flags": 0, "name": "_lightning_globals", "static": true },` (CRLF kept; nothing re-serialised). Follow with a csplit-only regen. Verification: lightning.obj rebuilt with the edited header has all 34 sections identical to production, with storage going from 2 to 3. widgets.obj, the only other file that includes lightning.h, rebuilt in place has all 20 sections identical.
(2) /Od SHAPE EDITS (optional). Apply scratch/w/lightning/od_shape.patch (clean), or production.patch, which combines (1) and (2) and is clean on its own. It contains these edits, each attested in the /Od build (fn 0x79d1c0):
- no `jitter` local: `scale_vector3d(&up, real_seed_random_range(...)*jitter_offset, &displacement)`
- `set_real_point3d(&centroid, (x1+x0)*.5f, ...)`
- `jitter_scale *=`
- `(next-md)*fraction + md`
- the /Od declaration orders
- the RTC names `adjusted_color` and `displacement`
The `_lightning_submit` normalized sha stays 9885e979 (identical to production, so the park hash does not drift) and the COMDAT/surplus set is unchanged. /W3 shows only the inherited C4146/C4244. There are no config/parked.json changes and no parks to retire.

## evidence
All files are in scratch/w/lightning/ (LEDGER.md has every probe).
Baseline gate: 8 exact / 1 residual. alndiff shows 756/756 instructions and two real packets:
- P1 @0x38e: the global_z_axis3d fallback copy. January uses ECX as the pointer; ours uses EAX (the 5-byte A1 form). That 1-byte shift also causes the nop / mov edi,edi pad difference and the downstream edx/ecx swaps.
- P2 @0x876: January emits `fstp st(0); mov ecx,[ebp-8]; fstp [esi+0xc]`; ours emits the fstp before the reload.
The /Od build (odbuild fn 0x79d1c0, RTC names marker/points/bounds/centroid/adjusted_color/displacement) had not been used by any earlier ledger. It shows:
(a) The vertex pair is written with point_from_line3d(point,&up,width,vertices) and point_from_line3d(...,-width,...). The helper body at 0x42e0d0 equals real_math.h. The current source hand-expands it, which breaks rule 6.
(b) The segment-up fallback is ONE aggregate integer copy from global_z_axis3d, with the pointer loaded once (0x79df14). The HCEA PPC decompile also shows `side = *(const real_vector3d *)global_z_axis3d`. HCEX types: global_z_axis3d is `const union real_point3d *` and up is `union real_vector3d`.
Probes:
- c1 (point_from_line3d): body byte-identical to production. It adds a `_point_from_line3d` COMDAT that is byte-identical to January's selected copy (action_charge, 48 B, sha 9b763841), and provider_link PASSES.
- c2 (c1 + aggregate cast copy): P1 closes; only P2 is left.
- Four /Od shape edits, five /Od declaration orders, two /Od dead locals (lab only), u_offset+u and !vertex_index: all byte-inert.
- 36 + 66 statement-order permutations of {P,TX,C,TY} across both vertices: best is 2 hunks, never 0.
- Forwarded colour (vertices[-1].color) and vertex_index-referencing TX2/P2: refuted.
- Name-count oracle (0..40 dummy decls before the function): all 41 identical, so P2 is not a name-count tie.
Emission census (tail.py): vertex-2 TX2 (the fstp of the long-lived CSE value u+u_offset) is emitted right after the POP unless a vertex-2 integer store is emitted while that value is on the x87 stack. Every variant that separates them visibly moves C2 or TY2, which January does not show. A board scan (scan_blk.py) finds 224 January blocks that load the branch value before a trailing x87 store, so January's order is the usual one and ours is the exception.
Owner-gated best (owner_gated_best.c = production.patch + cast + point_from_line3d): 2576/80, one adjacent dependency-free transposition left, sha 4a3cf430.
Audit of the candidate: 18 January symbols with 0 differences apart from the storage finding. .rdata 268 and .bss 4 are 100% in build/report.json. Surplus COMDATs pass provider_link.

## blockers
1. The function is not exact. P2 is a dependency-free order between the vertex_index reload and the TX2 x87 store, decided by C2 dag.c emission. No statement order, declaration order, name count, helper spelling or /Od-attested shape reaches it (more than 100 measured shapes).
2. P1 needs an owner ruling. The only form that matches is the aggregate view copy `up = *(real_vector3d const *)global_z_axis3d;`. It is attested by /Od and by the HCEA PPC decompile, but it is load-bearing: stripping the cast is a compile error, and the six component-store orders fail. Under brief section 7 that makes it owner-gated.
3. The rule-6 exception for the `_point_from_line3d` COMDAT (identical copy, provider link PASS) also requires a strictly exact caller, so the /Od-attested point_from_line3d spelling cannot land until 1 and 2 are resolved.
4. The storage of `_lightning_globals` is wrong in the split and production (external; January has it file-static). This is fixed by the storage packet above and must land before any whole-object admission.

## reopen_criteria
Reopen only when BOTH hold:
(a) the owner admits the /Od-attested aggregate view copy for the axis fallback;
(b) there is either a decoded C2 emission rule (for example a live C2.dll trace of the TX2 fstp emission decision, comparing owner_gated_best against the ord2 variant p_P-TX-C-TY_P-C-TX-TY, where only C2's IL position differs) or a first-party January-era source fragment. It must show a natural spelling that emits the vertex_index reload before the TX2 fstp without moving the C2 or TY2 store.
Do not repeat: the TX/C/TY and P/TX/C/TY permutation matrices, /Od declaration orders, /Od dead locals, name-count 0..40, forwarded colour/texture, vertex_index-referencing TX2, &points[vertex_index] P arguments, u_offset+u, !vertex_index, swapped bounds arms, or anything in the earlier lightning ledgers.

## task notes
No strict credit this wave; the object stays at 8/9 exact.\n\nNew this wave:\n- /Od evidence (earlier ledgers never used the /Od build): point_from_line3d at both vertex sites (the current source hand-expands it, against rule 6), and a single aggregate copy for the axis fallback.\n- A storage correction confirmed by two sources: lightning_globals is file-static in January (missing from the cachebeta publics; HCEX DataKind FileStatic). It is byte-inert; the patch and the in-place symbols.json edit are provided.\n- A characterized emission-order rule explaining the last transposition.\n\nThe owner-gated best candidate leaves exactly one adjacent reload/x87-store swap. It needs the owner to rule on the axis aggregate view copy, which w2 already proposed and which now has /Od attestation.\n\nAll work stayed under scratch/w/lightning/. No tracked files were edited. No ninja, configure or git-mutating commands were run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\lightning\LEDGER.md
