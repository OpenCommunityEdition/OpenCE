# Cinematics typed quad recovery (Jonas, 2026-08-25)

## Result and bounded scope

This wave starts from clean cumulative commit
`1082b414a8e9de412e81b6c0acf64dbce9cc5a12` and recovers only
`draw_quad`.  The retained implementation is ordinary typed C and is strict
exact on its first and only edited-source production build:

| January owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_draw_quad` | 331 / 336 | 9 | `98221d0a3037338ddd2696a341a859d48bc6eecd9e92643a6d4d8e2cbc7fb44a` |

The complete ordinary Cinematics census advances from 14/17 to 15/17 exact
functions and from 388 to 719 accepted meaningful code bytes.  All 14
inherited emitted function sections are byte-and-relocation unchanged, and
`_draw_quad` is the only added code owner.  The two remaining absent bodies,
`cinematic_set_title_delayed` and `cinematic_render`, are untouched.  All
existing runtime data sections are unchanged, so this wave claims no data
gain.

The shared Rasterizer thunk remains strict exact:

| Preserved owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_psuedo_dynamic_screen_quad_draw` | 16 | 1 | `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51` |

The completed `source/rasterizer/rasterizer` unit remains 131/131 exact with
3,294 accepted code bytes and 612 accepted data bytes.  No completion label,
configuration, semantic exception, parked-function record, storage owner,
Units source, or protected source changes.

## Immutable authority and source identity

- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/cutscene/cinematics.obj`: SHA-256
  `a5e6cd5d77d661d9b47f59d9b1c6bf6a83051a62a977706dcadacbc4e19074f5`.
- January split `source/rasterizer/rasterizer.obj`: SHA-256
  `c6869c570b967be7d4256fe789cea0b6b41016b20763a79160e49f8c119c4744`.
- Baseline source blobs for `cinematics.c`, `rasterizer.h`, and
  `rasterizer.c`: `260869035daa6e65ba467d3066ba7c0de548e0c6`,
  `1273896ad17902147078205e248461450de65d34`, and
  `8b67d32b5efdcfd78c7119d547eb9d919850958f`.
- Frozen retained source blobs before commit: `86e4ff0aaaede41aace50be6f5448064df14d905`,
  `50abf28f18e1637ed4263e8a6cb1adf402bf3f73`, and
  `bf3333f8d94d7d41e3a0ebd51f0d56806eb691f2`.
- Clean freshly built baseline Cinematics and Rasterizer objects: SHA-256
  `04d9b4cb7a0e7be8fdd2029c54dd34fa74c6bd5dcd011edf8f3e8f6e04c0ddc9`
  and `0d792ae5ad6c219300d98c08da87bcbd18baba80303c1b1626fa986d900b4d`.
- Immutable first-shot Cinematics and Rasterizer objects: SHA-256
  `f46ac016c7e34940a8d53b372831b4713a8d5296477719f1863dd33d73667325`
  and `3de8f9f3332eed6fec8a0b8f89a803f462d045434d224b7bdab46020eb55e7ab`.
- XDK 3911 VC7 `CL.Exe` 13.00.9254.1: 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

Raw candidate object identities contain normal fresh-build debug metadata.
Acceptance uses padded bytes, normalized hashes, and full relocation
address/type/destination/addend equality.

## Claude documentation and dirty-donor boundary

The complete Claude `cinematics_obj.md` experiment log was read before this
wave.  Its authenticated Git blob is
`dc893145b0a3ad55a1a0634b2802e42702120442`.  It records the exact topology,
336-byte result, nine relocations, and normalized hash reproduced here.  It
also documents why the two larger missing Cinematics bodies remain blocked by
the forbidden inline-x87 conversion helper; this wave does not revisit them.

The dirty `work/cheats-strict-complete` carrier was used only as exact source
evidence.  Its `cinematics.c` blob is
`11d39ed66e1d3d4ec3416b3c8b3469277c2a8298`; the associated dirty
`rasterizer.h` and `rasterizer.c` blobs are
`3699db667d90171298882fd18f4fe7b98cc821ee` and
`71940745a3cb92b8cb3f08e6baa16356ec411aec`.  Only the audited quad body,
three required includes, typed declarations/layouts, and two-pointer thunk
interface were transcribed.  The carrier's unrelated Cinematics bodies,
extra includes/constants, global initializer, timing edits, Rasterizer global
initializer/BSS edits, Unicode declaration, HUD/config changes, and all other
dirty state were excluded.

The retained code gets the game globals through `scenario_get_game_globals`,
the rasterizer block through `TAG_BLOCK_GET_ELEMENT`, and the bitmap group
through the existing `bitmap_group_get` typed tag accessor.  It uses no raw
tag cast, object cast, raw offset/address, pointer or union pun, undefined
behavior, inline assembly, `volatile` scheduling device, barrier, pragma,
intrinsic, attribute, `register`, byte patch, or synthetic anchor.  Every
parameter has its own line and the function has an explicit terminal return.

## Shared-interface consumer inventory

`rasterizer.h` has exactly 16 direct defining/consumer translation units on
this cumulative tip.  The complete inventory was derived from the current
source tree and gated, rather than copied from an older seven-unit audit:

1. `source/ai/ai_debug.c`
2. `source/cutscene/cinematics.c`
3. `source/interface/terminal.c`
4. `source/main/main.c`
5. `source/objects/widgets/light_volumes.c`
6. `source/rasterizer/common/rasterizer_common.c`
7. `source/rasterizer/rasterizer.c`
8. `source/rasterizer/rasterizer_debug.c`
9. `source/rasterizer/rasterizer_frame_statistics.c`
10. `source/rasterizer/rasterizer_lights.c`
11. `source/rasterizer/rasterizer_memory_pool.c`
12. `source/rasterizer/rasterizer_transparent_geometry.c`
13. `source/render/render.c`
14. `source/render/render_contrails.c`
15. `source/saved games/game_state.c`
16. `source/shell/shell.c`

The clean Rasterizer-only fail-closed manifest is
`build/audit/cinematics_draw_quad_baseline_rasterizer_20260825.json`,
1,297,466 bytes, SHA-256
`3b9beb749af77df5e6e9ceb13e3613fd03370bea4fe3c92f92668d6c34df8e60`.
It pins clean commit `1082b414` and all 131 accepted functions.  A second
15-unit header-consumer manifest is 15,108,659 bytes, SHA-256
`c4b60e772bc882d0cbf0b71b56650cc16606305455c0bc0946da907f21ef1aca`.
The regression tool cannot parse the spaced `game_state` Ninja edge, so all
16 baseline objects, including `game_state`, were also frozen independently
before the edit.

The donor spelling `_rasterizer_lock_none` would collide with the existing
translation-unit-local exact Contrails enum once placed in the shared header.
The shared enum therefore uses the natural cumulative-safe spelling
`_rasterizer_lock_unlocked = 0` while retaining the proven
`_rasterizer_lock_cinematics = 8`.  Names do not affect emitted code, and this
avoids touching the completed Contrails source.  The current lock field's
`+0x02` layout is protected by the same style of offset assertion already used
for surrounding Rasterizer fields.

All 16 TUs passed XDK `/Zs` with the production optimization and include
graph.  The sole warning was the inherited `ai_debug.c(293)` declaration
warning.  After the fixed one-shot build, a hardened baseline-to-candidate
comparison of every common emitted code owner and every `.rdata`, `.data`,
and `.bss` owner found zero changes or removals in all 16 objects.  The only
addition was `_draw_quad`.  Debug type/symbol records naturally changed in
header consumers and receive no runtime credit; the dedicated Rasterizer
manifest check itself passes with 131 `still_exact`, zero failures, zero
warnings, zero `newly_exact`, and zero changed nonexact functions.

## Sole candidate emission and strict adjudication

After the parse-only gates, the frozen three-file source packet was not
changed again.  The 16 proven object paths were resolved inside this isolated
worktree, deleted with literal-path semantics, verified absent, and rebuilt in
one fixed Ninja invocation:

```text
[1/16] ... [16/16] CL <complete rasterizer.h consumer inventory>
```

There was no source-shape retry, tuning compile, candidate pruning, option
change, or second edited-source emission.  Direct hardened comparison proves
all 336 padded quad bytes and all nine relocation records equal.  The
relocations resolve to `global_scenario_get`, `scenario_get_game_globals`, two
tag-block calls, `tag_get`, both `rasterizer_globals + 2` stores, `csmemset`,
and the Rasterizer thunk at the exact January addresses.  The Rasterizer
wrapper likewise retains its 16-byte/one-relocation fingerprint.

The authenticated csplit v0.0.2 and objdiff-cli v3.3.1 binaries have SHA-256
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
and `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.
The generated rule remains `/O2 /Oy- /DDEBUG /Dxbox` with no per-object flag.

## Full pre-commit gates

- Complete `halobetacache_build libcmt_build`: pass; 486 copied-build
  catch-up edges completed after the bounded object wave without rebuilding
  either adjudicated object.
- Direct authenticated objdiff report generation: pass.  Cinematics reports
  15/17 exact and 719/2,130 accepted meaningful code bytes; Rasterizer remains
  131/131, 3,294/3,294 code, and 612/612 data.
- Semantic audit: 470 units, 4,238 functions evaluated, 4,096 semantic exact,
  114 hidden exact / 64,705 hidden code bytes, 4,157 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 objects, 4,131/11,060 functions, 501,769
  accepted code bytes, and 1,835,088 accepted data bytes.  Halo contributes
  273/468 objects, 3,964/7,574 functions, 488,855 accepted code bytes, and
  1,829,896 accepted data bytes.
- Object admission: zero candidates and zero revocations; the only
  contradiction is the inherited unrelated `source/shell/shell_xbox`
  completion label.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- Source scope, policy scan, `git diff --check`, full consumer code/runtime
  A/B, direct function fingerprints, and Rasterizer manifest replay: pass.

The generated report target attempted to refresh already authenticated tool
download edges because copied Ninja timestamp records are worktree-specific;
sandboxed network was unavailable.  No source or candidate object changed.
The authenticated local binaries were restored and invoked directly to
generate the report, after which semantic/progress and all remaining gates
passed.  This operational detail grants no acceptance exception.

## Committed-state replay plan

No committed-state replay is claimed in the implementation commit.  After the
three source files and this initial ledger are committed by Jonas Volman and
the worktree is clean, clean accepted-state manifests will pin the exact
commit.  The Cinematics and Rasterizer generated objects will be resolved and
proven inside this isolated worktree, deleted with literal-path semantics,
rebuilt once together by their ordinary Ninja edges, and checked immediately.
Direct fingerprints, the 15/17 and 131/131 censuses, runtime-data invariants,
consumer A/B, and a final Ninja dry run will be repeated before one additive
ledger-only replay commit.  The same clean snapshot/delete/rebuild/check
sequence will then run at corrected HEAD.  No push, amend, rebase, history
rewrite, or worktree removal is performed.

## Clean implementation-state replay

Implementation-and-initial-ledger commit
`3e15e24ca05de36314f2d3fce20f44d672c746b1` was authored by Jonas Volman
`<theunknowentity@gmail.com>` and was clean before replay.  Re-reading that
commit proves the retained Cinematics, Rasterizer header, and Rasterizer
source blobs remain `86e4ff0aaaede41aace50be6f5448064df14d905`,
`50abf28f18e1637ed4263e8a6cb1adf402bf3f73`, and
`bf3333f8d94d7d41e3a0ebd51f0d56806eb691f2`.  The initial ledger blob is
`0850cbc14692e06e2564b3acb8005716930dd043`, and the implementation commit
contains exactly the three intended source paths plus this new ledger.

A fresh two-unit accepted-state snapshot was written with `--no-build` at
that exact clean commit.  The ignored manifest
`build/audit/cinematics_draw_quad_impl_replay_20260825.json` is 1,559,597
bytes with SHA-256
`a14b1121e99bb2157fcfa1768ab6098bc3062847a51e8254fd6cb63bd866c217`
and pins the full implementation commit.

The generated Cinematics and Rasterizer object paths resolved to
`build/base/source/cutscene/cinematics.obj` and
`build/base/source/rasterizer/rasterizer.obj`.  Both absolute normalized paths
were proven to begin with this isolated worktree root.  The snapshotted
objects retained the immutable first-shot SHA-256 identities
`f46ac016c7e34940a8d53b372831b4713a8d5296477719f1863dd33d73667325`
and `3de8f9f3332eed6fec8a0b8f89a803f462d045434d224b7bdab46020eb55e7ab`.
Only those two verified files were removed with `Remove-Item -LiteralPath`,
and immediate verification proved both absent.  Their ordinary targets then
ran exactly:

```text
[1/2] CL build\base\source\cutscene\cinematics.obj
[2/2] CL build\base\source\rasterizer\rasterizer.obj
```

The rebuilt objects are again 6,057 and 43,911 bytes.  Their expected raw
metadata-phase SHA-256 identities are
`508adbca09699179a3eeb351d609aee9c7d6e5717fb08ea969e01b2523257ca0`
and `b6fe8ca8fc6a8a53edbb77506b4848eaaa256f546f78d490e6645f943e2f091b`;
the complete runtime acceptance view is unchanged.

The immediate regression check returned `ok: true`, zero failures, zero
warnings, zero `newly_exact`, and zero changed nonexact functions.  It records
all 15 emitted Cinematics target owners, including `_draw_quad`, and all 131
Rasterizer report owners as `still_exact`.  Independent direct replay proofs
also passed:

- `_draw_quad` remains strict exact at 336 padded bytes, nine relocation
  records, and normalized SHA-256 `98221d0a3037338...`;
- the Rasterizer wrapper remains strict exact at 16 padded bytes, one
  relocation, and normalized SHA-256 `6fde8ac9ac079976...`;
- the complete 16-consumer baseline-to-replay comparison still has zero
  changed or removed code/runtime owners, with `_draw_quad` the only
  addition; and
- a final generated two-object Ninja dry run reports `ninja: no work to do`.

Only this new ledger is modified for the additive replay record.  After the
ledger-only commit, the same clean snapshot/delete/rebuild/check sequence will
be repeated at corrected HEAD so the handed-off branch itself is the final
replay authority.  No source change, candidate retry, tuning, header-scope
expansion, configuration edit, amend, push, history rewrite, or worktree
removal occurred after the implementation commit.
