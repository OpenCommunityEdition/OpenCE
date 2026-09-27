# Commit split record, 2026-09-26 (local, unpushed history)

Commit 453bdbb5 ("Remove an unreferenced string local from ai_debug_render_actor", owner-approved E13) had carried
677 previously staged research-record files (research/compiler_application_20260925/workers/A1-A4, C1, D1, E1, F1,
G1, H1, K1, K2; LEDGER.md; OWNER_PACKET.md; tools/sig.py) in addition to its one-line source change. That violated
"one logical packet per commit". The lane is unpushed, so the commit was split with git plumbing (commit-tree), keeping
authors, dates and messages:

| old | new | content |
|---|---|---|
| 453bdbb5 | 700ca0d1 | research records only (tree = fe283cc5 + research/ files) |
| 453bdbb5 | 83e1921e | E13: source/ai/ai_debug.c only (tree = old 453bdbb5 tree) |
| a7e14ea4 | 219c2c72 | periodic x-table builder (tree identical) |
| 4f57f304 | eb7270b3 | periodic_functions.h style (tree identical) |
| bb57160f | c5366682 | periodic_functions admission (tree identical) |

Proofs: `git diff --name-only fe283cc5 700ca0d1` lists only research/ paths; `git diff --name-only 700ca0d1 83e1921e`
lists only source/ai/ai_debug.c; `c5366682^{tree} == bb57160f^{tree}`. The pre-split tip is kept as the local branch
`backup/pre-split-453bdbb5` (bb57160f). Documents written before the split (e.g. K1's `packets/VERIFY_4f57f304.txt`,
`VERIFY_a7e14ea4.txt`) name the old hashes; their trees equal the new ones.
