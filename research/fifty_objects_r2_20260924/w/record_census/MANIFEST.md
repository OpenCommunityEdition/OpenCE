# MANIFEST - full-period record-count census (slug record_census), LAB ONLY

Base/tip: 42125a52 (clean; `git status` shows only the pre-existing untracked research/fifty_objects_r2_20260924/).
No claims taken, no tracked file edited, no candidate source produced, nothing proposed for landing. Dummy declarations
are an ORACLE only. real_math.h is RESERVED for the Codex packet and is not touched or proposed.

Outside-slug side effects: validate_gate.py ran the REAL tools/campaign/gate.py 7 times, which (as always) left
scratch/_gate_<pid>.c/.obj pairs in scratch/ (not deleted: outside the slug). All census compiles used census.py's
gate.py copy with temporaries in tmp/ (deleted; tmp/ is empty).

## Artifacts
- census.json - per unit (86), per function: status at K=0, exact-K set and sha forms per position (after_include,
  top; + before_fn, inside_fn for residuals), tier (S1/S2/T) for residuals; count-sensitive residuals with windows
  (k, width, per-K TU-joint check and broken canaries); count-coupled residuals; TU joint windows; fragility;
  object-completion-by-count; 64x64 grid summaries; zero-loss hosts for solo. Plus whole_board_fragility (453 units).
- results/*.jsonl - raw rows (status, size, normalized sha12, reloc count per function per compile).
- Scripts: inspoint.py, fnpos.py, census.py, validate_gate.py, windows.py, flipsets.py, build_census.py, tier.py,
  formdist.py, incsets.py, header_candidates.py, board_fragility.py.

## Compile counts (all 0 failures)
after_include 5,504; top 5,504; before_fn 8,064; inside_fn 8,064; grid 32,768; whole-board top 28,992;
validation 128 + 600-row gate.py equivalence check (0 mismatches).

## Result (see LEDGER.md Conclusions)
1/126 residuals closes by count (solo, +2/+3), 7/126 count-coupled never exact, 118/126 count-inert; 0 objects
completable by count alone; 22 count-sensitive exact canaries board-wide (7 break within |shift|<=3).
