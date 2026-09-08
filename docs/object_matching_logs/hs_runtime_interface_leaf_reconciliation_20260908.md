# HS runtime and Interface leaf reconciliation (2026-09-08)

## Result

This packet reconciles ten small owners from the completed Claude lanes onto
published canonical commit `f8c709078`. Donor headline totals are not added:
every owner was rebuilt and measured in the current canonical tree, with the
pre-packet snapshot as the only accounting baseline.

| unit | owner | meaningful bytes | padded bytes | verdict |
| --- | --- | ---: | ---: | --- |
| `source/interface/interface` | `_interface_get_tag_index` | 117 | 128 | strict exact |
| `source/interface/ui_widget` | `_remap_sticks_for_local_player` | 301 | 304 | relocation-hardened exact |
| `source/interface/interface` | `_interface_initialize_for_new_map` | 86 | 96 | strict exact |
| `source/interface/interface` | `_interface_get_real_argb_color` | 118 | 128 | strict exact |
| `source/interface/interface` | `_interface_splitscreen_render` | 215 | 224 | strict exact |
| `source/interface/interface` | `_interface_draw_bitmap` | 382 | 384 | strict exact |
| `source/interface/interface` | `_interface_draw_bitmap_modulated` | 434 | 448 | strict exact |
| `source/hs/hs_runtime` | `_hs_thread_new` | 240 | 240 | strict exact |
| `source/hs/hs_runtime` | `_hs_evaluate_set` | 319 | 320 | strict exact |
| `source/hs/hs_runtime` | `_hs_runtime_initialize_for_new_map` | 711 | 720 | strict exact |
| **total** | **10 owners** | **2,923** | **2,992** | **zero regressions** |

Canonical meaningful exact code advances from **962,207 to 965,130 bytes**,
and credited functions advance from **6,417 to 6,427**. Exact data remains
**2,150,772 bytes**, and complete-object admission remains **390/833**. This is
a partial-owner packet; no data, BSS, or whole-object credit is inferred from
the code gains.

The rename-stable whole-tree sweep reports exactly the ten gains above and no
lost exact owners. The five-function Interface graph contributes 1,235
meaningful / 1,280 padded bytes, the HS packet contributes 1,270 / 1,280, and
the two initial leaves contribute 418 / 432.

## Provenance and independent reconciliation

- `_interface_get_tag_index` and the five-function Interface graph were
  recovered from Opus commit `00faf893b`. Only the authenticated code and its
  genuine declaration/type dependencies were retained; the donor's profile
  tables, debug renderers, p32 bitmap variant, and unrelated bulk edits were
  not imported.
- `_remap_sticks_for_local_player` was recovered from Opus commit
  `c195d321a`. January's natural single-exit switch writes the selected icon
  and reaches the shared return. Hardened COFF comparison proves all 304
  padded bytes and all 31 semantic relocations exact (normalized SHA-256
  `963b6756ac2f3b70faf18955bad37bac6ed40bbe366baa697feccc61e7050ce8`).
  The semantic manifest is used only because objdiff mispairs the switch's
  compiler-local labels; this is not fuzzy credit.
- The HS work descends from Opus commit `0973b18e6`, then was reconstructed
  against canonical's current headers. The accepted three owners have 91/91
  matching relocation rows. The dependency and consumer graph was audited
  rather than accepting the donor translation unit wholesale.

The January symbol atlas and independent HCEA reconstruction both identify the
old `_code_000cf2f0` owner as `_interface_splitscreen_render`. A scratch-only
target regeneration under that semantic name and an alias-based production
gate both report the same five Interface gains, zero residuals among the
written graph, and preserve the already-exact fullscreen caller.

## Interface ownership and house-rule decisions

The Interface packet uses the project's named `real`, color, bitmap,
rectangle, rasterizer, tag-block, and framebuffer-blend types. The color-table
tag, record definitions, and typed `color_table_definition_get` macro live in
`source/bitmaps/color_table_group.h`, instead of leaving a raw `tag_get` cast
or foreign duplicate in `interface.c`. HUD, terminal, main-framerate,
cinematic `draw_quad`, and public Interface declarations are placed in their
actual associated headers. The only source-local declaration is the
Interface-local splitscreen helper; its recovered January owner has external
linkage.

The accepted `_interface_get_tag_index` also supersedes the zero-count-path
rejection in
`docs/object_matching_logs/interface_obj_jonas_color_pair_20260821.md`.
January's target, the HCEA/cross-build evidence, and the loaded-map invariant
authenticate the optional-block accessor. The source expresses a typed macro
over the real scenario structure; it does not use a raw address, offset,
prefix-structure facade, or representation pun. The old ledger remains useful
as a record of the earlier evidence threshold, but its rejection is no longer
the current ruling.

`interface_draw_bitmap_modulated` deliberately has no newly published header
prototype in this packet. It currently has no cross-translation-unit caller,
so there is no consumer-local declaration to repair. Adding the otherwise
unused declaration to `interface.h` was isolated as the cause of a C2
definition-position change that reopened the previously exact 560-byte
`game_engine::_populate_statistic_buffer` owner through reordered independent
stack loads. Removing only that unused prototype restores the owner exactly.
A public declaration should be added when a genuine caller graph requires it,
with the full consumer blast radius rechecked then; no exactness credit was
traded away for cosmetic header placement.

Adding the genuine `main_framerate_render` declaration to `main.h` changes the
Shell compiler-local SEH scope-table label from `$T18271` to `$T18273`. The
12-byte `.rdata` payload, flags, both relocations, addends, and source-relative
handler identities remain identical. `config/semantic_data_matches.json` now
names `$T18273`; this is a compiler-local owner rename backed by identical
data/relocation evidence, not a data-content exception.

The bitmap loops retain natural source expressions that reproduce January's
x87 lifetime and store order. No forced inline, pragma, assembly, volatile or
register steering, fake branch, raw offset, or nonsensical construct was
accepted. The manual pixel packing is the natural implementation evidenced by
the target; replacing it with a function helper would change the call graph,
and no equivalent project macro exists.

## HS ownership and honest fuzzy retention

`hs_evaluate_set` is declared in its narrow owner
`hs_library_internal_runtime.h`; object-list reference helpers are declared in
`object_lists.h`. The signed `short` contract for `hs_global_evaluate` is
corroborated by the HCEA reconstruction, existing public HS APIs, January's
`movsx`, and the established sibling idiom in `scenario.c`: cast to `word` only
when testing flag bit 15, while leaving index masking signed. The inherited
`_hs_global_evaluate` owner remains exact.

The recovered `hs_debug_data` name is an external BSS owner defined by
`linker_common.obj`, not a `bss+address` placeholder. HS runtime retains only a
narrow external declaration and does not fabricate or move its storage. All
seven C consumers of `object_lists.h` were gated before and after; none loses
an exact owner, including `units.obj` at 189/189.

HS datum identifiers use the existing `DATUM_INDEX_NEW` macro rather than
open-coding the salt shift/OR. This byte-neutral cleanup preserves the inherited
initializer owner and keeps the new map initializer exact.

`_hs_global_reconcile_write` is retained as the best complete, readable
dependency for the new callers but receives **zero exact credit**. Its natural
39-case typed switch writes every supported external HS global and has the
same 47 semantic relocations as January. The target is 400 padded bytes and the
candidate is 416; normalized hashes differ, with an objdiff score of
68.44203%. The mismatch is entry/prologue and load scheduling, not missing
logic. It is recorded in `config/parked.json` with both hashes and a clear
reopen boundary. The donor's inequality double-staging workaround, BSS split,
foreign type duplicates, and code-generation facades were rejected.

## Verification

- Full `ninja` build and fail-closed semantic progress report: pass at
  965,130 meaningful exact code bytes / 6,427 credited functions.
- Rename-stable pre/post whole-tree sweep: 10 gains / 2,992 padded bytes / zero
  regressions.
- Focused final gates: Interface 12 exact / 0 residual / 6 unwritten; HS
  runtime 59 / 5 / 2; UIWidget 63 / 3 / 36. The Interface scratch target under
  the recovered semantic name gives the same 12/0/6 result.
- Relocation-aware comparisons: all five Interface graph owners and their
  inherited fullscreen caller have equal size, normalized SHA, relocation
  count, and zero differing relocation rows; the three HS exact owners have
  91/91 matching relocation rows; the UIWidget semantic owner has 31/31.
- Every affected `object_lists.h` consumer preserves its before/after exact
  set; the additional cinematics, HUD, terminal, and main header-owner gates
  preserve their established baselines.
- `gate.py --forbid-emitted-symbol point_from_line3d` passes for Interface, HS
  runtime, and UIWidget. No new `point_from_line3d` COMDAT is emitted; the
  inherited Units owner is byte-identical before and after.
- `fake_match_scan.py --fail-on-findings` reports zero review leads across the
  three changed C files and their associated headers.
- Park audit after recording the HS residual: 337 active entries, zero stale,
  zero invalid. Object-admission audit: zero candidates and zero
  contradictions.
- Correctly scoped tracked tools suite: **1,140 passed, 2 skipped, 26 subtests
  passed** in 14.47 seconds. An earlier repository-root invocation wandered
  into generated `scratch/` trees and produced collection errors; that command
  was mis-scoped and is not treated as a code/test failure.
- `git diff --check` and JSON parsing: pass.

## Treemap checkpoint

The previous treemap checkpoint was 962,207 meaningful exact code bytes. The
campaign policy refreshes it every 10,000 reconciled bytes, so the next refresh
threshold is **972,207**. This packet reaches 965,130, still 7,077 bytes short;
the local treemap is therefore intentionally not refreshed by this packet.
