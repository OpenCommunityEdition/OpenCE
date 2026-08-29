# `main.obj`: public-nine salvage and policy-clean six-leaf closeout (2026-08-28)

## Outcome

This lane froze nine ordinary public bodies from the clean historical donor,
spent one code-producing compile on the complete frozen batch, and then pruned
fail-closed.  Six ordinary C bodies are retained:

- `_main_get_solo_level_name`
- `_main_movie_start`
- `_main_save_current_solo_map`
- `_main_load_ui_scenario`
- `_main_menu_load`
- `_main_present_frame`

All six pass the strict COFF section comparator after the final normal rebuild.
All 64 exact owners inherited by `main.obj` from the clean base remain exact.
The final object-local board is therefore 70/95 exact functions and
3360/13792 exact padded bytes: 64 inherited owners plus six new owners, with
no new residual body.

Three frozen candidates were not retained:

- `_main_pregame_render` was a first-shot residual and was pruned without
  tuning.
- `_main_get_solo_level_from_name` was byte-exact itself, but its required
  `long` return declaration made inherited `_code_000f0850` residual.  The body
  and long declaration were pruned and the canonical local `short` extern was
  restored.
- `_main_vertical_blank_interrupt_handler` was byte-exact itself, but the
  typed support needed to express its paired frame/vblank operations introduced
  new `volatile` fields and anonymous-union aliasing surface.  Policy review
  rejected that representation support, so the body and all rasterizer support
  were pruned without attempting another source form.

No commit or push was made from this worktree.

## Base, branch, and provenance

- worktree:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\main-public-nine-salvage-20260828`
- branch: `jonas/main-public-nine-salvage-20260828`
- clean base: `83883cc2e9dc7619b76068d6c86eb01b8ba735d6`
- donor worktree:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\main-strict-complete`
- donor branch: `agent/main-strict-complete`
- donor tip: `f50be1825f6291c8837c7dcaba078d983bf9e8a5`
- donor `source/main/main.c` blob: `4abe5197...`
- donor `docs/object_matching_logs/main_obj.md` blob: `eef44838...`

The donor worktree was clean.  Its unrelated crash, raw/private-ABI, pragma,
barrier, and UB material was used only as negative evidence and was not copied.
The target bytes and strict normalized relocation identity remained the
admission authority.

The sibling worktree
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\xbox-sound-cache-public-quintet-20260828`
also points at `83883cc2`, but it is not a clean base: it has an uncommitted
`source/cache/xbox_sound_cache.c` and an untracked ledger.  A repository-wide
set comparison against its generated objects therefore reports its five dirty
`xbox_sound_cache` leaves as apparent losses.  They are concurrent,
unintegrated work, not regressions from pushed `83883cc2`; this lane did not
import or modify that work.

## Required documentation read before emission

The following canonical in-repository documents were read in full:

- `docs/object_matching_logs/main_obj.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `docs/object_matching_logs/main_obj_jonas_crash_policy_closeout_20260826.md`
- `tools/campaign/README.md`
- `tools/audit/README.md`

The canonical agent documents are external to this exact worktree, not absent
from the workspace.  Both were read in full from the sibling canonical tree:

- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\halo\CLAUDE.md`
  - SHA-256:
    `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`
- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\halo\AGENTS.md`
  - SHA-256:
    `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`

Relevant `main.obj` history through `f50be182`, the clean donor ledger, the
historical `ef64130c` support diff, and available Claude session/memory evidence
were also reviewed.  No newer Claude-active `main.obj` edit overlapped this
lane.

## Frozen one-shot batch

The batch was frozen before the candidate compile:

| Function | Target padded size |
| --- | ---: |
| `_main_get_solo_level_name` | 32 |
| `_main_get_solo_level_from_name` | 368 |
| `_main_movie_start` | 176 |
| `_main_vertical_blank_interrupt_handler` | 192 |
| `_main_save_current_solo_map` | 112 |
| `_main_load_ui_scenario` | 224 |
| `_main_menu_load` | 144 |
| `_main_pregame_render` | 304 |
| `_main_present_frame` | 144 |

Explicit exclusions for the batch were `_main_crash`, `_code_000f1c20`,
`_main_rasterizer_throttle`, every pragma/barrier/UB/raw/private-ABI form, and
the donor's private static `_code_000f13a0` screenshot helper.  The retained
`_main_present_frame` uses the ordinary typed historical `tiff_export` path.

## Parse-only and one-shot evidence

The first parse-only pass identified declaration order and typed rasterizer
layout dependencies.  After bounded support import, the second `/Zs /W3`
parse passed.  Warnings were the translation unit's existing `cseries` and
`real_math` warnings, the expected long-to-short conversion exposed by the
frozen long solo-level declaration, and existing implicit declarations in the
unfinished `main_loop`; there were no parse errors.

Exactly one code-producing compile was spent on the complete frozen candidate
batch:

```text
ninja build\base\source\main\main.obj
[1/1] CL build\base\source\main\main.obj
```

The target split was regenerated directly with `csplit` after the symbol map
change, without recompiling the candidate.  Independent strict comparisons of
the nine frozen owners produced:

| Function | Result | Size | Relocs | Candidate normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_main_get_solo_level_name` | exact | 32 | 1 | `c8371e416579ba49eda6fe8d0688bec6051516f6982e60a8e519d72d0e2ddebf` |
| `_main_get_solo_level_from_name` | exact | 368 | 22 | `68e7372c529d46cf6292e2698cf022704ed3085cbabae5fd7bedc48598bf81a9` |
| `_main_movie_start` | exact | 176 | 16 | `903b8c7c3083cca08b14eedbfa3d9d2b4fb68f837541b3070cf499c9aab64b2f` |
| `_main_vertical_blank_interrupt_handler` | exact | 192 | 26 | `a521a165a21cccf94e131328067d26ea586ba9d1a3297a59ded61f7d72dd31c8` |
| `_main_save_current_solo_map` | exact | 112 | 9 | `3d264609554d2c9287927b3e86d83126dcd4a5838326ddc01663c74286422a3c` |
| `_main_load_ui_scenario` | exact | 224 | 22 | `9148cc4da1304ca62ebfa482c41ffa42bc7ede60b53d4b84b74ad77c5835f2e1` |
| `_main_menu_load` | exact | 144 | 18 | `4cc7244c61c24eadb6e51e56052b2f0a8b4239155283dd09a3bc23851d99959d` |
| `_main_pregame_render` | residual | 304 | 29 | `586504c8d0746d16cb85ec3d79fddc5405b33c56d5420c3f58d5a0896f948b28` |
| `_main_present_frame` | exact | 144 | 12 | `5ad92723dc9df3e7e18ec0f699037ce430d77af403e299b5b0cc2df9a6ef34bc` |

The target pregame hash was
`74a8d1ce4fefa85007818da3ece3b4570e628944bd89889e3c630e6a199fadca`.
The equal size and relocation count did not override the normalized byte-hash
failure.

The one-shot inherited-owner census was 63/64: the sole loss was
`_code_000f0850`.  This reproduced the donor ledger's proven one-for-one
declaration conflict with the long-return `_main_get_solo_level_from_name`.
That conflict was not tuned around.

## Fail-closed pruning sequence

1. `_main_pregame_render` and its candidate-only collision/render-camera
   support were removed immediately after the first strict gate.
2. `_main_get_solo_level_from_name` and its long prototype were removed; the
   canonical local short extern was restored.  A normal rebuild then measured
   64/64 inherited plus seven exact retained bodies.
3. Policy review rejected the new volatile/anonymous-union representation
   support required by the exact vblank leaf.  The vblank body, its header
   formatting change, the entire rasterizer header layout change, and the
   temporary compatible rasterizer initializer were removed.  No alternate
   source or representation was attempted.
4. A final `/Zs /W3` parse and normal `main.obj` rebuild were performed for the
   policy-clean six-body subset.

The final retained support is limited to:

- the `config/symbols.json` BSS ownership split at file offsets 4544336
  (`_window_storage`, static) and 4544512 (`_main_globals`, static);
- a typed `render_window` storage prefix and the physically corresponding
  local `main_globals` layout;
- the bounded typed `game_options` prefix used by `_main_load_ui_scenario`;
- ordinary typed declarations/includes for the retained public callees;
- target-owned initialized-data definitions and the ten-entry solo scenario
  path table; and
- public header declarations for the six retained bodies.

Final tracked source changes are confined to `config/symbols.json`,
`source/main/main.c`, and `source/main/main.h`, plus this ledger.  All temporary
collision, render-camera, render signature, and rasterizer source/header
changes compare identically to the base and are absent from the final diff.

## Final strict evidence

After the final normal rebuild and a fresh direct csplit pass:

| Retained function | Exact | Size | Relocs | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_main_get_solo_level_name` | yes | 32 | 1 | `c8371e416579ba49eda6fe8d0688bec6051516f6982e60a8e519d72d0e2ddebf` |
| `_main_movie_start` | yes | 176 | 16 | `903b8c7c3083cca08b14eedbfa3d9d2b4fb68f837541b3070cf499c9aab64b2f` |
| `_main_save_current_solo_map` | yes | 112 | 9 | `3d264609554d2c9287927b3e86d83126dcd4a5838326ddc01663c74286422a3c` |
| `_main_load_ui_scenario` | yes | 224 | 22 | `9148cc4da1304ca62ebfa482c41ffa42bc7ede60b53d4b84b74ad77c5835f2e1` |
| `_main_menu_load` | yes | 144 | 18 | `4cc7244c61c24eadb6e51e56052b2f0a8b4239155283dd09a3bc23851d99959d` |
| `_main_present_frame` | yes | 144 | 12 | `5ad92723dc9df3e7e18ec0f699037ce430d77af403e299b5b0cc2df9a6ef34bc` |

- inherited exact census: 64/64, zero lost
- retained exact census: 6/6
- final board: 70/95 functions, 3360/13792 padded bytes
- `_main_get_solo_level_from_name`: unwritten
- `_main_pregame_render`: unwritten
- `_main_vertical_blank_interrupt_handler`: unwritten
- no new residual body

Final artifact hashes:

- `source/main/main.c`:
  `d0446e485793a45266d113fd6e97ce8751e342e6e855ef8008561e9fd866a5b8`
- `source/main/main.h`:
  `9d4ba279b0b76c6291ea707fd0df282ac0b8bb704be2013323a76b068a676196`
- `config/symbols.json`:
  `a756bed2f38a8aafd6e52efdb4e0def87f2261891cb57645bc6ebc87cf02e911`
- final candidate `build/base/source/main/main.obj`:
  `041d7c0e36fd15495ea20851acd8ec2eac83100dcc052be0257c7e7ff30331fc`
- regenerated target `build/split/source/main/main.obj`:
  `abd673d4db1cc315662dde43eac3d4bdec2914ca2384077fa5e112fbcdda1255`

## Validation

Toolchain evidence:

- VC7 `CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`
- `csplit.exe` SHA-256:
  `708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
- `objdiff-cli.exe` SHA-256:
  `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`

Validation results:

- final `/Zs /W3` parse: pass; pre-existing warnings only
- final normal `main.obj` rebuild: pass, `[1/1] CL`
- full `ninja halobetacache_build libcmt_build`: pass
- direct 64-inherited plus six-retained strict census: pass
- `python tools/campaign/board.py source/main/main`: 70/95,
  3360/13792 bytes
- generated `build/report.json`: pass
- `python -m tools.audit_semantic_matches`: pass
  - 470 units scanned
  - 4700 functions evaluated
  - 4571 semantic-exact
  - 4581 accepted-exact
  - zero unit errors and zero ordinary rejections
- `python -m tools.audit_object_admission`: pass
  - zero candidates, contradictions, and revocations
- `python -m tools.parked_functions`: pass
  - 13 active, zero stale, zero invalid
- `python -m unittest discover -s tools -p 'test*.py'`: 205 tests pass
- `git diff --check`: pass
- new-line prohibited-form scan for pragmas, assembly, emitted opcodes,
  naked functions, volatile additions, barriers, and raw/private ABI forms:
  no hits

The generated report SHA-256 is
`8f4b58f9338bc57e23757431f4fef9801c13f5decdee4fbeec18948a4452bff`;
the generated semantic report SHA-256 is
`bf3d940125f62930edf791bb0034a9d6be2377a0cc823b3e0976d87036fb2254`.

## Reopen conditions

- Reopen `_main_get_solo_level_from_name` only with a legal typed declaration
  topology that independently preserves inherited `_code_000f0850`; do not
  accept the measured one-for-one trade.
- Reopen `_main_pregame_render` only with new source provenance or a newly
  justified ordinary-C topology; the first-shot equal-size/equal-relocation
  residual is already measured.
- Reopen `_main_vertical_blank_interrupt_handler` only if ordinary typed
  support can express the January operation without adding volatile,
  aliasing-union, pragma, barrier, raw, or private-ABI surface, or if campaign
  policy explicitly changes.
