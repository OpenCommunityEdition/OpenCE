# critic_r2 MANIFEST (read-only completeness critic, round 2)
Base/tip: worktree HEAD 5d3ca708 (task named 7b9de585; tree source identical), clean; no tracked edits.
Lab-only probes (claimed + released with label critic_r2):
- source/bink/bink_playback: bink/cand_static_used.c (static empty stub + call in bink_playback_render) -> 23/24 EXACT, _code_001b5850 EXACT, provider_link PASS; cand_static_unused.c -> stub not emitted.
- source/hs/hs_compile: hs_compile/cand_extern_tables.c, cand_static_finish.c -> 61/64 unchanged, all sections section_infos_equal to baseline, only 4 storage rows change.
Tools written: battery.py, table.py, storage_check.py, matching_audit.py, lab/{xbe.py,pe.py,map_vs_atlas.py,map_candidates.py,map_align.py,xbuild.py,xbuild_run.py,xdiff.py,struct_now.py}.
Outputs: battery/*.txt, table.txt, storage_check.txt, matching_audit.txt, lab/map_align.txt, lab/xbuild.txt, lab/struct_now.txt.
Credited: nothing (critic). See LEDGER.md C1-C9.
