# HUD ARGB converter: bitmap-inline owner (2026-09-23)

Frozen base: `f2f880f9`. This packet adds one strict January function,
`_real_argb_color_to_pixel32`: 308 meaningful / 320 padded code bytes. The
whole-board strict sweep moves 7,588 to 7,589 of 8,245 functions, with zero
regressions. It does not claim the remaining HUD object is complete.

The implementation belongs in `bitmaps_inlines.h`, as attested by the later
first-party source-file/inline record and January's own assertion paths and
line literals (89 and 188). January's instructions perform two genuinely
different packings of the same four channels, compare them under the literal
`verify == result`, and return the first. The two bounded scopes model the
first pass's alpha cell and the second pass's accumulator sharing a stack
slot at disjoint lifetimes. Sparse x87 assembly in this color-conversion
helper expresses January's 32-bit FISTP and non-top x87 scaling; no literal
frame offsets, padding, forced inline, pragma, dead local, or hand expansion
of a gameplay caller is used. Every local read has a preceding write.

The public declaration moved from `hud_draw.h` to the bitmap-owned
`bitmap_color_conversion.h`. Its consumers include that owner header; the
unrelated local prototype in `rasterizer_xbox_text.c` was removed. The inline
header includes `particles.h` for the genuine `valid_real_argb_color`
declaration, which is defined in `particles.c`. This is a dependency, not a
new validator body or fabricated prototype.

An earlier authentic header-owner trial lost exact
`__rasterizer_screen_effect` through one independent load-schedule swap.
That consumer changed subsequently; the same class of owner restoration
now leaves it strict exact at 3,888 bytes. The coherent prototype migration
also preserves it. No code regression is waived.

Ownership: January selects one 320-byte ARGB body in `hud_draw.obj`. The
current compiler emits that exact 320-byte/12-relocation body there and two
additional SELECTANY copies in `rasterizer_xbox_dynavobgeom.obj` and
`rasterizer_xbox_screen_effect.obj`. All three have normalized SHA-256
`ae86b8fdd729000ffa35ff6657b0935762b82edd78d93263d6ae9bef4e7be0a3`
and strict section-info equality to January's selected body, including every
relocation. Both consumer bodies remain strict exact (2,960-byte dynamic
quad and 1,344-byte screen flash). The selected-provider pair-link check
passes in both orders for each surplus copy. This is the previously admitted
folded shared-header inline COMDAT class, not extra function credit.

Verification on the isolated worktree:

- Full `ninja -j 8 all_source progress semantic_progress`: pass.
- Stable verdict diff: one gained 320-byte section, zero lost.
- Parks: no stale or invalid entries; admission audit: zero contradicted or
  revoked.
- `python -m pytest -q tools`: 1,154 passed, 5 skipped, 26 subtests.
- `git diff --check`: clean.
- Fake-match scanner reports two new inline-assembly review leads for the
  evidenced ARGB helper blocks; no claim is made that a clean scan exists.

The isolated build uses the already installed `objdiff-cli` 3.3.1 because
network access blocks the pinned 3.6.0 download. Strict byte/relocation
verdicts above come from the independent COFF gate/sweep, not a scorer-only
percentage. Generated build-file/tool setup is untracked and not committed.
