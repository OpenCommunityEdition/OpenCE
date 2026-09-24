# source/physics/breakable_surfaces

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\cand_T1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\patches\\breakable_surfaces.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\patches\\config.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\patches\\symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\patches\\b\\source\\physics\\breakable_surfaces.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\patches\\b\\config\\config.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\patches_owner_gated\\real_math.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\patches_owner_gated\\effects.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\sweep.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit2\\edits.py"
]

## production_changes
An ADMISSIBLE PACKET (zero regressions) for the integrator to apply. Every patch passes `git apply --check` against 931ed8dc. Post-images are under scratch/w/breakable_surfaces_audit2/patches/b/.
(1) scratch/w/breakable_surfaces_audit2/patches/breakable_surfaces.c.patch. This is wave-1's T1 change. It deletes the static __inline breakable_surface_get_plane_from_designator, a hand copy of the bsp3d.h helper that breaks rule 6. The single call site now calls the genuine bsp3d.h bsp3d_get_plane_from_designator.
(2) scratch/w/breakable_surfaces_audit2/patches/config.json.patch. Line 520 becomes `{ "name": "source/physics/breakable_surfaces.c", "index": 145, "status": "NonMatching" },`, which removes the per-TU options override `/nologo /c /O2 /Oy- /Ow /QIfist`.
(3) config/symbols.json has a full edited copy at scratch/w/breakable_surfaces_audit2/config/symbols.json, with patches/symbols.json.patch alongside. It makes two in-place line edits: line 5362 `_breakable_surface_effect` gains `, "static": true`, and line 23114 `_globals` (file_offset 4550316) gains `, "static": true`. After that, regenerate build/split. A csplit run from the scratch config changes only breakable_surfaces.obj (833 objects compared).
OWNER-GATED, do not self-land: scratch/w/breakable_surfaces_audit2/patches_owner_gated/real_math.h.patch and effects.c.patch (post-images in patches_owner_gated/b/). real_math.h: real_local_random becomes `real result= real_seed_random(get_global_local_random_seed_address()); return result;`. effects.c: remove the `#define real_local_random real_local_random_inline`/`#undef` rename and the NODUP hand copy. In effect_generate_particles, restore the /Od-attested `real_local_random()` alpha call and the `real_local_random_range(0.0f, 2.f*_pi)` rotation call.

## evidence
Current production state: 12/12 EXACT, data 685/685. object_audit shows 26 January sections ok and 2 storage DIFFs (the symbols.json side is wrong: both symbols are absent from cachebeta PDB publics, so January had them file-static). Surplus has 9 problems: 7 DIFFERENT COMDATs and 2 static helpers.
Rule 22 proof for dropping the flag override (lab SWEEP_DROP, production source):
- Dropping /QIfist gives an object identical to production in every non-debug section and the symbol table, so /QIfist does nothing in this TU (the conversions go through fast_ftol).
- Dropping /Ow leaves all 12 owned functions and the data byte-identical. Only the helper COMDATs change, and six of them become IDENTICAL to January's selected copies: _cross_product3d, _plane3d_from_point_and_normal, _plane3d_negate, _project_point2d (Layer 2 fix confirmed), _project_point3d and _vector_from_points3d.
- /Ow appears in no other TU, and no exported function changes, so callers are unaffected.
Combined candidate C1 (T1 source + default flags + fixed split):
- lab_gate 12/12 EXACT (gate_C1.txt).
- audit2 OBJECT AUDIT PASS, 26/26 symbols.
- 41 defined symbols, 0 storage disagreements with cachebeta publics.
- fake_match_scan: 0 leads. No parks or rejections for the unit.
- New COMDAT _bsp3d_get_plane_from_designator is IDENTICAL to decals' copy and link PASS.
- provider_link fails on exactly one row, _real_local_random against effects.
- Shadow whole-board sweep (sweep.py compiles all 447 build\base\source units exactly as build.ninja does; the pristine control equals build/base 447/447): 446 identical, only breakable changes.
_real_local_random, double-attested (January bytes and the /Od build):
- January's selected effects copy ends `add esp,4; ret`. The header's direct-return body gives `pop ecx; ret`. The named-local form reproduces January exactly.
- /Od build: real_local_random at 0x573ae0 has a push-ecx slot with fstp/fld (a named local). /Od real_random at 0x42e1c0 has no slot, and January's real_random is the pop-ecx direct form. real_local_random_range is already a named local in the header and matches January.
- January's effects object defines the section but has no reference to it: every use was inlined, and effects (index 324) is the first user in link order.
- /Od effect_generate_particles (0x56f580) calls real_local_random at 0x56fdee and real_local_random_range at 0x56fd6d.
Owner-gated candidate C2 (C1 plus the real_local_random packet):
- breakable: audit PASS, provider_link PASS; the only surplus problem left is the static plane_distance.
- effects: still 38/3, surplus 0 problems, link PASS. weather_particle_systems' copy also becomes the January form.
- Cost: rasterizer_frame_statistics goes 10/10 to 9/10 (_rasterizer_frame_statistics_draw, 4176) and bitmap_drawing goes 7/7 to 6/7 (_bitmap_copy, 2784).

## blockers
B1 (owner ruling): the genuine real_local_random body adds one declared name to every TU that includes real_math.h. By declaration count alone, that knocks out _rasterizer_frame_statistics_draw and _bitmap_copy.
- Lab oracle: one dummy prototype reproduces the same flips.
- _rasterizer_frame_statistics_draw breaks at every tested offset from -2 to +5 except 0.
- Layer 2 was count-neutral for both victims: they include real_math.h but not random_math.h, so -2 `v` locals were offset by +2 lock/unlock prototypes.
- No evidence-backed -1 compensator exists. The duplicate-prototype census finds only CRT duplicates, stack_walk_disregard_symbol_names (-2, measured still broken) and bitmap_delete (-2). real_math.h's nonuniform_cubic_spline(_vector3d) bodies are decompiler junk (v9..v20 locals, -14 if repaired), so the header's total is known to be off and the victims sit on a coincidental count.
B2 (owner ruling): the static candidate-only _breakable_surface_plane_distance (32 B) is the owner-landed j+k association helper (95d674d0). January's breakable text has no gap for it. bsp3d was admitted with the same TU-local lever; the bipeds park declined it. A probe removing the header helper's outer parentheses (the bsp3d B24 control) is NEGATIVE: the effect stays residual and 7 other TUs change.

## reopen_criteria
B1: reopen for an evidence-backed January declaration difference that brings the name count before _rasterizer_frame_statistics_draw and _bitmap_copy back to net 0, or an owner ruling that accepts losing those count-coincidence exact functions for the /Od- and January-attested header body. Also reopen if the owner rules on repairing nonuniform_cubic_spline(_vector3d) together with a count re-baseline. B2: reopen for January source or an owner ruling that extends the bsp3d TU-local plane-distance precedent to breakable_surfaces.

## task notes
Tools written for this re-audit, all in the slug directory:
- sweep.py: shadow-tree compile of all 447 build\base\source units with the exact build.ninja flags, run from sh/ so __FILE__ text matches production. It compares non-debug sections and symbol tables; SWEEP_DROP="unit=Ow,QIfist" models a per-TU config override removal. The pristine control equals build/base 447/447.
- edits.py: named byte-level CRLF edit sets for the shadow tree.
- fnstat.py: per-function strict status of any object.
- provider_link_v.py: provider_link with a PROVIDER_ROOT override, so it can link against shadow-variant providers.
- split/: csplit output from the scratch config.

Git Bash heredocs collapse double backslashes, so write Python helpers with the file tool.

Cross-unit findings for the integrator:
- The owner-gated real_local_random packet would also clear weather_particle_systems' _real_local_random surplus.
- first_person_weapons.c forces out-of-line calls with `#define REAL_MATH_EXTERNAL_REAL_LOCAL_RANDOM`. January's first_person_weapons really does call _real_local_random out of line (2 relocations in _first_person_weapon_update).
- /Od also shows word-slot patterns in local_random (0x551170), local_random_range (0x566650) and random_range (0x43e850). These are probably named locals, but the bytes of int-returning copies cannot tell; each would add further declaration-count cost.

No tracked files were edited and no ninja, configure or git-mutating commands were run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\breakable_surfaces_audit2\LEDGER.md
