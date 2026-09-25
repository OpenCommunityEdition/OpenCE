# source/objects/widgets/lightning

## decision
RETIRE

## est_probability
0.05

## route
None for _lightning_submit (P2: a dependency-free `fstp st(0); mov ecx,[ebp-8]; fstp [esi+0xc]` reload/x87-store order; P1: the axis fallback needs the owner-gated aggregate view cast). Keep the park. The zero-credit storage correction is still pending and should land independently: round-1 scratch/w/lightning/storage.patch makes `static struct lightning_globals lightning_globals = {0};` and removes the extern from lightning.h. With it goes symbols.json line 23106 `"static": true` for _lightning_globals, then a csplit regen. pdb_storage at f6d00a8c still shows `_lightning_globals split 2 ours 2 PDB-public False`, and HCEX reports it as a File Static. The /Od shape edits (od_shape.patch) can go in the same zero-credit step.

## why_new
Nothing new. P2 has no oracle anywhere in January outside lightning (w2 reload/store scan), and more than 100 statement, declaration, name-count and helper shapes were measured inert. Round-1 reopen needs BOTH an owner ruling on `up = *(real_vector3d const *)global_z_axis3d` AND a decoded C2 emission rule or a January source fragment. Neither exists. The Sept-2001 map names (lightnings_dispose, lightning_render, lightning_offset_marker_position) are already in symbols.json. tinfo confirms point_from_line3d is inlined (no out-of-line reference), so the Rule-6 COMDAT question stays tied to an exact caller.

## prior_negatives_checked
Round-1 wave2 result and ledger (c1/c2, the 36+66 permutation matrices, /Od declaration orders and dead locals, name-count 0..40, forwarded colour, vertex_index TX2, u_offset+u, !vertex_index). opus5_150k_w2 (HCEX locals, add-esp oracle scan, reload/store scan: no site) and w3 (offset_marker_position closed via set_real_vector3d, 'S1-S16 / component-order / vertex-2 store permutations' do-not-repeat). credible_fuzzy_completion_20260902 and fable_salvage_reconciliation_20260902.

## notes
The object stays 8/9. The storage packet is byte-inert: round 1 showed all 34 lightning.obj sections identical and all 20 widgets.obj sections identical. It is an admission prerequisite, not a credit gain.