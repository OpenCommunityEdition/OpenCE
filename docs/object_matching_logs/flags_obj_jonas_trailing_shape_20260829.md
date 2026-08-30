# `flags.obj` trailing-edge shape (Jonas, 2026-08-29)

## Result

`_flag_set_trailing_shape` is byte-exact and relocation-exact on the only
production compile. It has 225 meaningful / 240 padded bytes, six relocations,
and target/candidate normalized SHA-256
`bcfc14d9975bd784345bee9268d4acf3704630796c2355e0ec6574893929657b`.
All six relocations are `REL32` calls to `_flag_tesselate_region`, at section
offsets 93, 123, 153, 168, 198, and 213.

`source/objects/widgets/flags` advances from 9/16 to 10/16 strict functions,
from 732/5752 to 957/5752 meaningful matched code bytes, and from 800/5888 to
1040/5888 strict padded code bytes. The nine inherited exact owners remain
strict exact.

## Frozen census and collision audit

The pre-edit census covered all 1,500 then-registered worktrees, all 1,464
refs, all six ref-reachable `flags.c` blobs, and all four ref-reachable Flags
ledger blobs. It found no prior `_flag_set_trailing_shape` body, production
compile, exact/miss measurement, name/config entry, or ledger claim. No
worktree had a dirty `flags.c`, `flags.h`, or Flags-ledger overlap. Sixteen
unrelated worktrees had dirty global configuration/semantic files; this wave
does not change any of those files.

The six committed `flags.c` blobs were
`0777a0052528b3ea2bfe32d50d2c8b6d93d0fc5a`,
`145e032b15754379a662a5ddf80a6ea8eaf43a0d`,
`308b9d18fe782bd06f498b8f8f02d84f6ff1aeef`,
`8e20520ac282c217a6dd5825997541312ff0fb10`,
`c6852ea01ccac54b5c5f70e55bd45cc397be8224`, and
`e8d9a4998acfba350adb23576e01db8312870bd0`. The four Flags ledgers had blobs
`1d6da0cb915026d9e9e02504871fe7ab65bc5152`,
`4b6ea23079bade3e367f39cee7bea141b778375f`,
`6f5b0d7a0158b88016ad01a9a8bdda4cbfb62ca7`, and
`de2e3dc4e94418964168e8caa4fe07b45db12209`. A global Markdown/text/log and
unregistered-source search under the campaign, `C:\halo-worktrees`, and
`C:\tmp` found no additional Claude/agent attempt or scratch body.

All 1,109 worktrees containing a January `flags.obj` held the same immutable
object, SHA-256
`8bf8e0959be8366ab347ccbefbf24a037d9639ec1c57f779c1b9923d91c47c5d`.
The January executable authority has SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.

The immediately preceding `_flag_tesselate_region` experiment is explicitly
excluded and was fully reverted: January was 288 padded bytes / one relocation
(normalized SHA beginning `cb2c11dc`), while the one candidate was 304 / one
(SHA beginning `65423234`). This wave neither reconstructs nor credits it.

## Authority and typed reconstruction

January disassembly is the code and call-topology authority. It proves the
shape-zero early return, the `height - 1` split for shapes 3/4, the
`height >> 1` split for shapes 1/2, the nonnegative x clamp, and the direct
shape tests/calls in the retained order. HCEA was used only for semantic names,
field meanings, and independent shape/tesselation mapping evidence:

- clean reference commit `aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`,
  `src/flag_set_trailing_shape.c` blob
  `b1d072f46b604c5598a122e2cc7d82137581f6d2`;
- reorganized reference commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`,
  `src/blam/objects/widgets/flag_set_trailing_shape.c` blob
  `46c06f1898d5d7bcefbb02f9bd966df413072358`.

The retained C89 source adds named shape/tesselation enums, a typed helper
prototype, three natural `short` locals, the repository `MAX` clamp, and direct
typed calls. It contains no assembly, intrinsic, volatile/register lever,
pragma, attribute, undefined behavior, raw-address/byte-offset access, byte
forcing, code generation, or object patch.

## Exact comparison and regression boundary

`python -m tools.coff_compare` reports `all_equal: true` for the new owner and
the nine inherited owners: `_flags_initialize`,
`_flags_initialize_for_new_map`, `_flags_dispose_from_old_map`,
`_flags_dispose`, `_flag_delete`, `_flags_update`, `_flag_render`,
`_flag_datum_get_vertex`, and `_flag_datum_get_cell`.

The pre-edit whole-TU manifest is rooted at canonical
`9a7b6dd3cd75c3ae4219a537dc65eebe831e03d7` and has SHA-256
`398ad713f0ba354a3cb1bae3b52d7cd9a9d21d5b750a28f86c6301c9800ca531`.
Its comparison reports `_flag_set_trailing_shape` as `NEWLY_EXACT`, all nine
prior owners as `still_exact`, and `changed_nonexact: []`. It fails closed only
on the expected new function-symbol ownership and compiler CodeView
`.debug$S` record. No exception or adjudication hides those intentional
inventory changes; direct hardened comparison independently proves every
inherited executable section remains exact.

## Repository gates

- Full Ninja build/report/semantic/progress: pass. Progress is 382/833 linked
  files, 598700/2198102 meaningful code bytes, and 4721/11060 functions.
- Semantic report: 470 units, 4877 evaluated functions, 4750 semantic-exact,
  129 hidden-exact, 4760 accepted-exact, and zero unit errors.
- `_unit_preprocess_node_orientations`: remains strict exact at 1920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `halobetacache_build` and `libcmt_build`: pass (`ninja: no work to do`).
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Pytest: 205 passed.
- `git diff --check`: pass.

Only `source/objects/widgets/flags.c` and this additive ledger are tracked
changes. No header, config, symbol, semantic-match, parked, generated, or
protected-source file changes. Nothing is pushed.
