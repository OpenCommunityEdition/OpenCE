# Xbox Profile genuine shared-owner reconciliation (2026-09-06)

## Scope and result

This packet starts from canonical/published commit
`d751da11dd730f7cbb380505abc9572695534fd9`. It replaces Profile's incompatible
local debug-options prefix with the complete shared declaration in
`source/rasterizer/rasterizer_debug_options.h`, recovered and layout-tested by
the preceding [Active Camouflage owner packet](rasterizer_xbox_active_camouflage_obj_shared_owner_cache_20260906.md).
The provider remains `source/rasterizer/rasterizer.c`; Profile now names its
genuine `stats` and `profile_log` fields. No header or target is modified.

The same bounded packet removes Rasterizer's redundant local declaration of
`_rasterizer_profile_enable`: that consumer already includes the proper
`rasterizer/xbox/rasterizer_xbox.h` owner. No function body, API signature,
compiler setting, SDK table, COMDAT metadata, or layout is otherwise changed.

This is **source-ownership progress, not new byte credit or whole-object
completion**. The Profile isolated gate retains all 16 target functions;
the Rasterizer consumer gate retains all 131. Both pass the explicit
`_point_from_line3d` emission guard. The complete Profile runtime COFF
inventory is unchanged by the new include/type access.

## Source-authenticity review

Root read the complete Profile implementation and independently reviewed the
new include/member access and existing provider declaration. A second reader's
complete post-prefix audit is preserved at
`scratch/rasterizer_xbox_profile_postprefix_source_admission_review_20260906.md`,
SHA-256 `790d810dc7321cdf3f48d09d3d922dc1ca7a501808acf6f912bc30ebb28cf463`.
Its source audit precedes the redundant Rasterizer prototype deletion; that
one deletion is separately covered by the full gate.

The four private function names are conventional semantic reconstructions,
not claimed original names. January's Profile PDB module contains compile
records but no private procedure/local records. Their linkage and private
data/BSS ownership agree with the target. The callback types are genuine
`static void(unsigned long)`, compatible with the stock XDK's cdecl
`D3DCALLBACK`; the performance-counter calls use `LARGE_INTEGER` and the
unmodified SDK declarations. No assembly or compiler-control trick is used.

Existing volatile fields represent CPU/D3D callback sharing, not artificial
scheduling controls. The unusual extra argument in Profile's `NONE` error
branch is an inherited, explicitly documented January defect supported by
the target push/relocation sequence. This packet does not modify those bodies
or treat accidental nonsensical source as evidence of correctness.

## Remaining whole-object boundaries

Profile still is **NonMatching**, despite 16/16 target functions and all
2,234 target data bytes being exact (2,511 meaningful / 2,640 padded code).

### Stock SDK data ownership

The candidate emits three stock SDK `SELECT_ANY` tables absent from this
January split: `_D3DTEXTUREDIRECTENCODE` (16 bytes),
`_D3DSIMPLERENDERSTATEENCODE` (328), and `_D3DPRIMITIVETOVERTEXCOUNT` (88).
Their combined 432 bytes receive no target credit. Source provenance is the
pinned XDK's `D3D8.h` declarations, not hand-replayed data.

The earlier `/FORCE:UNRESOLVED` and `/INCLUDE` diagnostics demonstrate only
the current-base duplicate-selection phase. They do not prove a resolved
canonical image or complete ordinary ownership/discard. A fresh no-force
diagnostic with current Profile plus the real current headers/progress-bar/
decals objects produces no duplicate conflict but fails with 134 unresolved
externals and no image. Replacing those providers with January's split
selected owners produces all three `LNK2005` conflicts in both orders plus
139 unresolveds. The split's selection-1 encoding is not proof of original
SDK linkage, so neither test is represented as original-link provenance.

The compare-only Ninja graph has no final-image link rule, and the local
`xbox/` tree has compiler binaries/headers but no dependency/import libraries.
Even the complete 833-object split union leaves 171 unresolved externals.
The bounded link evidence and exact commands are retained at
`scratch/rasterizer_xbox_profile_no_force_link_boundary_20260906.md`, SHA-256
`4cd7cc05bee07a9d29cda8013d8ffba8ffeedc88d351dff2bda855e55b0ade1f`.
No fake stubs, emission anchors, forced link, altered COMDAT selection, or
invented discard pass is an acceptable substitute.

### Shared declaration ownership

Profile's remaining local `global_window_parameters` extern uses the genuine
complete type from `rasterizer.h`, so its field ABI is sound. The header does
not yet own the extern, however; 22 rasterizer TUs have local externs and
some still use incompatible partial views. This requires a separate genuine
global-owner migration and full consumer gates, not a Profile-only facade.

The former ledger's assertion that no consumer-local Profile declarations
remained was incomplete. This packet removes Rasterizer's redundant
`_rasterizer_profile_enable` declaration. Local `rasterizer_profile_begin/end`
pairs remain in `rasterizer_transparent_geometry.c`,
`xbox/rasterizer_xbox_models.c`, and `xbox/rasterizer_xbox_detail_objects.c`.
Their current signatures agree, but proper owner-header migration is still
required. Neither this documentation nor a function-exact result waives it.

The explicit rejection remains in `config/object_admission_rejections.json`.
Its wording now distinguishes the resolved debug-type issue from these live
boundaries; no rejection is retired and no matching rule is relaxed.

## Frozen verification

The pre-edit capture is `scratch/profile-shared-owner-before-20260906.json`,
SHA-256 `ad7872203f0e23cef60ce991c6ab7e8dec4593ac7a7c7944e887801735a3be68`.
It freezes 2,301 source/config/toolchain/report/object files at the base SHA.
The stable baseline is
`scratch/profile-shared-owner-stable-before-20260906.json` (6,323/8,245).

The initial scratch Profile source gate object is
`scratch/profile-shared-owner-probe-20260906.obj`, SHA-256
`b36d9a5db36a28a231fc7fff5e4910dc792a9cdec9e1b84801bcd6f4e176d028`.
Its complete runtime inventory, undefined/COMMON inventory, and symbol
offset/type/storage counts match the frozen original. It has 76 sections,
77 defined symbols (eight at nonzero offsets), and no COMMON. CodeView and
compiler-local serial labels are not used as runtime identity.

Final production verification passes:

- Full Ninja build/report generation and production progress.
- Full stable sweep: 6,323/8,245 exact, **zero gains and zero regressions**.
- All **572 base objects are byte-identical to their frozen baseline after
  normalizing only the COFF timestamp**. Only Profile and Rasterizer have
  changed timestamps; the other 570 objects are raw-identical. The actual
  production proof is therefore stronger than the scratch inventory check:
  runtime code, data/BSS, all symbols/offsets/relocations/selection records,
  candidate-only owners, and compiler debug records are unchanged.
- All **833 target split objects remain raw-identical**. The frozen file
  census permits exactly the two source edits and the Profile rejection's
  explanatory text; every header, target/config selector and compiler input
  remains unchanged. All other rejection entries are unchanged.
- Tests: **955 passed, 2 skipped, 26 subtests passed**.
- Parks: **294 active, zero stale, zero invalid**.
- Admission: **zero candidates, contradictions or revocations**, with the
  same five fully-accounted inherited rejections.
- Production totals remain **928,770/2,198,102 meaningful exact code bytes**,
  **6,277/11,060 credited functions**, **2,061,024/4,176,062 data bytes**,
  and **391/833 Matching objects**. No Opus headline bytes are newly claimed.

The final capture is `scratch/profile-owner-final-20260906.json`, SHA-256
`68fcbe3d79996f0f5374320b07c39e2ca71d8fbb4dae21a1cad6607d15999170`.
The fail-closed complete replay is
`scratch/profile_owner_full_audit_20260906.py`; its immutable result is
`scratch/profile-owner-complete-audit-20260906.json`, SHA-256
`26f5e295f059675fb6cbd1e622d17fb7f8814da29f1440bc0cf0ec368b926678`.
Stable, park and admission outputs are respectively
`scratch/profile-owner-final-stable-20260906.json`,
`scratch/profile-owner-parks-20260906.json`, and
`scratch/profile-owner-admission-20260906.json`.
