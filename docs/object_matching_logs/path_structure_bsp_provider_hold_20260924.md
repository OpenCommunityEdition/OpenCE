# `path_structure_bsp` provider hold (2026-09-24)

Baseline: canonical `4d1ebf179023f46efb77929fb599e514601152fb`.
This was measured in a disposable worktree; no production source or config
changes were admitted.

- Fresh `gate.py --all` gives `path_structure_bsp` 6/6 exact and
  `actor_combat` 32 exact, 2 residual. The path-structure code and data are
  already byte-matched; this is a link-ownership problem, not a missing body.
- A direct VC7 link of those two freshly compiled objects reports
  `path_structure_bsp.obj : error LNK2005: _cross_product2d already defined in actor_combat.obj`.
- `actor_combat.c` defines `cross_product2d` out of line at line 523 while
  suppressing the shared-header name across its includes. Its only genuine
  source-level consumer is the still-residual `_actor_aim_grenade`, which
  currently expands the determinant by hand. The prior /Od evidence attests a
  call to the real helper there, but the owner explicitly held changing a
  fuzzy caller to emit new inline helpers and pointer-view casts.
- Diagnostic control: changing only the hand-written definition to `__inline`
  (leaving the held caller untouched) makes the compiler omit the unused
  `_cross_product2d` section. `actor_combat` then drops from 32 to 31 exact;
  the selected 32-byte helper owner is lost. The source was restored and
  recompiled before this ledger was committed.

No link-safe, house-rule-clean closure was found under the current hold.
Externalizing the path-structure calls would discard their authentic inline
schedule; forcing a duplicate linker definition, a synthetic reference, or a
manual helper copy would not reconstruct source. Keep the object unadmitted
and give zero new code/data/object credit. Reopen only when the owner permits
an evidenced `_actor_aim_grenade` repair that preserves its existing exact
siblings and the provider link passes in both orders, or when independent
first-party source evidence supplies another genuine provider arrangement.
