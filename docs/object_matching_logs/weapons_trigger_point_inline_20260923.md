# Weapons trigger projectile exact match (2026-09-23)

Base: `188d6c1b` on `jonas/exact-pilots`. This packet changes only
`source/items/weapons.c`; it does not claim whole-object completion.

`_trigger_create_projectiles` now uses the genuine shared-header
`point_from_line3d` for the three first-person offsets, retaining the existing
vector construction and placement order. The `use_aiming_vector` initialization
and `inside_bsp` assignment are moved to their actual use sites. No helper is
hand-expanded, no UB or dead local is added, and no existing exact function
changes its verdict.

| Measure | Base | Candidate |
| --- | ---: | ---: |
| Halo meaningful exact code | 1,549,953 | 1,551,775 |
| `_trigger_create_projectiles` | residual, 1,808 padded | exact, 1,824 padded |
| Whole-board strict functions | 7,585 / 8,245 | 7,586 / 8,245 |
| Whole-object status | `weapons.obj` residual | residual (`_weapon_place`) |

The added `_point_from_line3d` is a genuine non-static shared-header inline.
Its 48-byte normalized COFF hash is
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`,
identical to the selected `action_charge.obj` provider. This is the admitted
all-inlined COMDAT class documented in
`claude_lane_b_HANDOFF_20260920.md` section 3, not a new code-byte credit for
the emitted helper. VC7 pair links in both object orders produced **no new**
`_point_from_line3d` duplicate-definition error. Both baseline and candidate
still expose the inherited `_real_random` duplicate against ActionCharge in
one order; this packet does not claim to repair or newly introduce it.

Verification in the isolated worktree: full Ninja build; strict stable diff
`gained 1 / 1,824 padded`, `regressions 0`; parks 103 active, 0 stale/invalid;
admission contradictions 0; fake-match scan of `weapons.c` 0 leads; pytest
1,154 passed, 5 skipped, 26 subtests. Credited data is unchanged. The
1,822-byte meaningful gain is new versus the immediate canonical base, not
the historical scratch lane where this candidate was first measured.
