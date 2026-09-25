# REVIEW source/effects/decals

approve: True

amended_patch: None

## checks
All checks were run in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic3_decals\ at 5d3ca708. No tracked file was edited. The unit was claimed as review_critic3_decals and released at the end.

(1) Name evidence, checked by my own tools:
- Sept-2001 cachebeta.map, static symbols section: `_decal_check` at 0001:00086520 in decals.obj, size 0x130. It is the first function of the object.
- Aug-2001 map: the same name, also the first decals static.
- Own alignment (order_align.txt): Sept has 33 decals functions and January has 33. 32 of them match by name, in the same order and at the same sizes. The only slot without a name match is Sept `_decal_check` 0x130 against January `_decal_verify_neighbors` 0x130.
- January atlas 4cc87b45: `_decal_check`, tier exact, at 0x486fe0. That is symbols.json offset 0x86fe0 + 0x400000.
- bodyid.py compared the January split function with the other builds, masking relocation bytes. Against the Sept xbe at 0x97520 there are 0 differing bytes. Against the Aug xbe there are 4, all assert line numbers. Against January's cachebeta.exe there are 0.
- Callers: the only caller in both builds is decals_reconnect_to_structure_bsp, with calls at the same offsets (+0x10c/+0x1ba in January, +0x10b/+0x1b9 opcode in Sept).
- January exe strings: no hit for decal_check, verify_neighbors or neighbors. The only related string is the assert "!layer_check || layer==decal->layer", which matches the function's layer_check parameter.
- The old name was invented in 4277f892 without any source cited. Earlier ledgers (opus5 100k decals 8.C and the names-only batch in the handoff) proposed this same rename, and no ruling or rejection was ever recorded.
- HCEX.pdb has none of the decals statics, including decal_update, so it gives no signal either way.

(2) `git apply --check`, on the worktree and with --cached: PASS.

(3) Candidate build: I applied the patch to copies inside my slug. csplit with the production config reproduces build/split exactly (833/833). With the candidate config, only decals.obj differs, and it is identical apart from the rename (sections, relocations and symbol table, storage 3 kept).

(4) gate.py: production gives 31 exact / 2 residual. The candidate (split emulated with with_split) gives 31 / 2, with the same rows apart from the rename (`EXACT 304 _decal_check`). The production and candidate objects differ only in the .file record.

(5) Audit tools, run through the redirect, which I confirmed with a probe:
- object_audit: FAIL(4), identical before and after. These are older failures (the clip/new_from_collision code and the .bss offsets of decal_globals/points2d_temp).
- pdb_storage: 0 disagreements on both sides.
- surplus_identity: 21 surplus copies, all identical to January's, on both sides.
- provider_link, both link orders: FAIL(1) on both sides. This is the known `_cross_product2d` clash with actor_combat (the reserved actor_combat repair). The renamed function is static, so it never enters this check.
- Data sections: identical.
- objdiff 3.3.1 (sha1 3130e428): 7365/15295 code, 31/33 functions, 33755/33755 data, 96.9359 on both sides.

(6) References: after the patch, git grep finds no code or config reference to the old name. The one remaining mention is prose in a docs ledger (decals_obj_opus5_100k_20260914.md:118), which recorded the proposal for this very rename. There are no parked.json, semantic_*.json, rejection, tool, test or header references, and no other TU or patch touches decals.c or the neighbouring symbols.json lines.

(7) The strip test and the /Od declaration-order check do not apply, because the change is identifiers only. On house rules, the authentic first-party name replaces an invented descriptive one (rule 7), and the layout is unchanged.

## issues
No blocking issues; approved as-is with no amendment.

The change earns zero credit. It is a byte-inert rename of a static that already matches exactly. The two remaining decals residuals (decal_clip_to_surface, decal_new_from_collision) are closed for this lane and are neither touched nor affected.

The lane rule that closes decals.c (per docs/object_matching_logs/claude_decals_physics_20260924.md) covers only the three residual functions. This rename is outside that scope, but the integrator can still choose to defer it.

Landing requirements: symbols.json must stay an in-place edit of line 1840 (never re-serialized), and csplit must be regenerated after applying.

The object stays incomplete, and every blocker predates this patch:
- object_audit: FAIL(4).
- provider_link: FAIL(1), `_cross_product2d` against the NODUP copy in actor_combat.
- admission rejections: projection_from_vector3d, vector_from_points2d and cross_product2d.
