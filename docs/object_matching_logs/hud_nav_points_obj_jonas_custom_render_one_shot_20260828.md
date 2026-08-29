# `hud_nav_points.obj` `_custom_render_nav_point` one-shot rejection (2026-08-28)

## Outcome

**Rejected and fully pruned.** The frozen public-source adaptation of
`_custom_render_nav_point` compiled successfully on its only permitted
code-producing candidate compile, but it was not strict exact. No candidate
code, typed support, include, data owner, or declaration remains in the tracked
source. `_hud_render_nav_points` was not touched or retried.

The isolated lane was
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\hud-nav-custom-render-20260828`,
branch `jonas/hud-nav-custom-render-20260828`, from pushed commit
`f09572504dc86d4f19595a0c3f437a549c697983`.

## Frozen authority and scope

The only frozen owner was
`source/interface/hud_nav_points.obj::_custom_render_nav_point`:

| artifact | padded bytes | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | 1632 | 95 | `287a20cd338648964303d1f60b46e9cdf94753f43718b48f8c648a0dd9e60306` |
| one-shot candidate | 1584 | 96 | `3d425ef6ea26bbf2de725afc39f14de76c789824af72df07bd1c931fc50e22be` |

The regenerated January target object was 15,145 bytes with raw SHA-256
`05f6bcce14e467330c57ee1a7bcd0e9ef8862a6f6818bf17fc252a8e67d095fa`.
The frozen pre-edit candidate object copied from the pushed-base integration
build was 14,101 bytes with raw SHA-256
`dfc760a6be49bcac4765c09f632c1e77cc1a2f981947a0a0802c3241d7ce5a18`.

The authenticated public donor bodies were read in full:

- HaloCEA `src/custom_render_nav_point.c`, commit
  `c89106c4964f2df3a98ef7b4035d6750372797cd`, file SHA-256
  `6b07d226b941b6307c7f290a249d4550f6ad71f791bebf9f0a2458d1f6fc12f6`;
- HaloCEA typed reorganization
  `src/blam/interface/custom_render_nav_point.c`, commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file SHA-256
  `351f7f549aad5023a54ff621c4c64b13b3d258b8d96d3149f43cfd36a5aa26fc`.

Complete Stian and Pastudan histories had no competing implementation. Their
applicable `AGENTS.md` and `CLAUDE.md` files, the repository matching/campaign
and tool documentation, all prior HUD ledgers, target COFF body/relocations,
and registered worktree history were reviewed before emission. The active
Claude worktree was read-only and Units-only; no dirty HUD overlap existed.

## Candidate spelling

The public body was adapted only to the January evidence:

- January's four stack arguments were used; the later fifth
  `render_distance` argument and both of its behavioral gates were removed;
- the January `waypoint_type != NONE` assertion and ordinary 0x80-long stack
  sentinel topology were restored;
- bitmap, render-camera/frustum, color, number-HUD, and placement access used
  natural C89 typed fields and local declarations only;
- no shared header, configuration, symbol map, tool, or other function body was
  changed;
- no assembly, volatile/register spelling, pragma, intrinsic call, compiler
  barrier, undefined behavior, raw address/offset cast, punning, private ABI,
  or codegen-forcing device was introduced.

## One-shot gate

VC7 `/Zs` parse-only passed with no diagnostics. The production dry run showed
exactly one CL edge, for
`build/base/source/interface/hud_nav_points.obj`. Exactly one code-producing
candidate compile was then performed.

The resulting object was 18,666 bytes with raw SHA-256
`d17a68b8eb4d31a14cbf7f1ac39c7d34ac7ea6e4fdbae05902495446b01bd140`.
Immediate hardened `section_infos_equal` returned false with the tabled
1584/96/`3d425ef6...` candidate fingerprint. This was a structural one-shot
rejection, not an invitation to tune. The complete candidate object is retained
only as ignored audit evidence at
`build/audit/hud-nav-custom-render-one-shot/candidate-one-shot.obj`.

All 30 pre-existing exact public functions stayed strict exact on that
candidate compile: `lost_exact: []`, `newly_exact: []`. In particular,
`_code_000c61d0` remained exact. `_custom_render_nav_point` remained residual,
and the pre-existing residual `_hud_render_nav_points` was unchanged.

## Prune and closeout

Immediately after the failed strict comparison, the entire emitted body and
all natural support were removed without a semantic or codegen adjustment and
without a second candidate compile. The restored
`source/interface/hud_nav_points.c` has the pushed-base Git blob
`5963f46d0291a56586e8ab7cc72bac2e4e6cfbc3`; its content diff is empty.

A normal post-prune rebuild was performed. The rebuilt object has raw SHA-256
`377ae0eb3799a19dcc839059c22b8802020433a59f596c9c95ddf05d23ed162d`
(phase-local debug/path material differs from the copied baseline), while the
hardened census again reports the same 30 exact owners with `lost_exact: []`
and `newly_exact: []`. `git diff --check` was clean before the ledger was added.

The only retained tracked change from this lane is this rejection ledger. No
commit or push was performed from the isolated lane. The coordinator's
pre-existing untracked `scratch/` evidence state is excluded from integration,
staging, and commit.
