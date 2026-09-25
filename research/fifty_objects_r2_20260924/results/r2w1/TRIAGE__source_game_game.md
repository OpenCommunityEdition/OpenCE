# source/game/game

## decision
RETIRE

## est_probability
0.03

## route
None. The only blocker is _game_set_game_variant_from_name (96 B): January keeps `lea edx,[ebp-0x68]; test edx,edx`, a memset arm that pushes edx (known 0), and a 0xd0 two-aggregate frame. VC7 folds every admissible spelling to 48 B / 2 relocations.

## why_new
Nothing new. alndiff re-confirmed the folded address test and the missing _csmemset / second _game_variant_global relocations.

## prior_negatives_checked
Lane B game_REPORT (30 shapes and flags); opus5 n1 (6 in-tree shapes plus 11 lab forms); round-1 wave1 fn_game (8 new probe families, including a 400-compile name-count sweep, /Ob1 __inline, flags, deferral, declaration-plus-initialisation, and an old-style declaration); cross-build readouts (/Od cache_symbols, HEK sapien, Oct 2276P LTCG: the test survives in 5 builds); park 48.

## notes
Reopen only for a first-party game.c/game.h fragment, a C1 IL-level explanation, or a live C2 trace of the inline-fold gate (round-1 criteria). The rest of the object passes audit; pdb_storage shows 0 disagreements.