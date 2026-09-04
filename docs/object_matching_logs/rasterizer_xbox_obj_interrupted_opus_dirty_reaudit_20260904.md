# `rasterizer_xbox.obj` interrupted Opus dirty-state re-audit (2026-09-04)

## Scope and authority

This lane audits only the uncommitted
`source/rasterizer/xbox/rasterizer_xbox.c` state in the frozen read-only donor
`C:\halo-worktrees\opus-small-families-30k-20260902`.  The donor is at
`eeedd72e5ba0fc1761519f68cad241fe606320e7`; its committed source blob is
`4b59f8531cc91999d77aacbff6904269583bbf08`, while the dirty physical source
hashes to `d61921b15d7bc11ea759240a1d06686f2c11350d`.  The audit branch starts from
canonical `f62f603b7dc76e79ad2da4fe9de71141af5f3870`, whose source blob is
`de2f6c866388cb1a9039f5f92b854279033cae41`.

January's split PC COFF, including section sizes, code bytes, symbol ownership,
and ordered relocations, is the admission oracle.  HCEA, Marathon, Stian's
source-history cache, and other local branches were consulted only for
semantic and naming corroboration.  No donor or canonical checkout was edited;
all compilations ran in an isolated worktree with PID-named scratch objects.
No applicable `CLAUDE.md` or `AGENTS.md` exists in the canonical repository,
donor repository, or their ancestor directories.  The campaign methodology,
throughput, safe-reconciliation, backlog, and prior Xbox-rasterizer ledgers were
read before adjudication.

## Already canonical

Commit `4b84e8fdc3947a8fbd4b7b1b125882a498cda16e` (`Reconcile safe Xbox
rasterizer core`) already performed the selective reconciliation of this
Opus work.  It admitted 47 strict-exact functions for 13,056 padded bytes and
10,533 meaningful code bytes with zero stable regressions.  It also established
semantic XDK-wrapper names, semantic data owners, narrow owner-header
prototypes, corrected return types, six evidence-backed parked bodies, and the
current 52 exact / 6 residual / 37 unwritten focused frontier.  This re-audit
therefore starts from that admitted state and does not duplicate any of it.

The canonical baseline at `f62f603b7` passes `ninja all_source progress
semantic_progress`.  It reports 878,556 / 2,198,102 meaningful code bytes,
6,048 / 11,060 credited functions, 2,018,310 data bytes, and 391 / 833 linked
objects.  Semantic progress reports 6,104 semantic-exact and 6,121 accepted
functions among 6,474 evaluated functions, with zero unit errors.  The focused
gate reports 52 exact / 6 residual / 37 unwritten.  The rename-stable baseline
snapshot contains 8,245 functions, of which 6,095 are strict exact.

## Dirty donor frontier test

The dirty donor changes this file by 1,681 insertions and 42 deletions relative
to donor HEAD, but it defines exactly the same 58 of 95 listed functions as
canonical.  It supplies no body for any of canonical's 37 unwritten sections.
Compiled directly in the canonical include/configuration context, the dirty
source falls to **42 exact / 16 residual / 37 unwritten**.  The ten lost exact
functions are:

- `__rasterizer_dispose`
- `__rasterizer_initialize`
- `_rasterizer_get_stencil_mode`
- `_rasterizer_get_target`
- `_rasterizer_preinitialize__fill_you_up_with_the_devils_cock`
- `_rasterizer_set_framebuffer_blend_function`
- `_rasterizer_set_target`
- `_rasterizer_set_target_as_texture`
- `_rasterizer_set_texture_non_blocking`
- `_rasterizer_set_vertex_shader`

Those losses are entirely relocation-owner regressions.  An in-memory
diagnostic-only alias of the donor's `bss_0045e028`, `data_0030ceec`, and
`rdata_0028bfac` owners to canonical's `rasterizer_xbox_d3d_globals`,
`rasterizer_state_cache`, and `framebuffer_blend_function_states` restores
exactly **52 exact / 6 residual / 37 unwritten**.  The aliases do not produce a
single exact function beyond canonical.  They were not admitted: address names
violate the naming rules, and canonical already has the relocation-correct
semantic owners.

This function-inventory and alias-controlled compile proves that the remaining
dirty donor state contains **zero new strict-exact padded bytes**, rather than a
forgotten tail of the original 13,056-byte packet.

## Disposition of every donor-only hunk

No source, header, or configuration hunk was admitted.  The complete set of
differences from canonical is adjudicated below:

1. **Symbol-banner regressions:** the donor rewrites the twenty standard XDK
   wrapper names to `code_001448f0` through `code_00146250`, rewrites
   `rasterizer_filthy_bitmap_default_initialize` to `code_00146650`, and
   rewrites the three semantic data owners to address-derived names.  These are
   comment-only for the banner but contradict known XDK exports and the house
   naming rules, so canonical's semantic names remain.
2. **Owner-header removal and caller-local redeclarations:** the donor removes
   the narrow bitmap, profile, decals, HUD, main, rasterizer subsystem,
   Xbox-draw-primitives, render-camera, and `rasterizer_xbox_internal.h`
   includes, then inserts roughly 160 lines of foreign prototypes into this
   implementation file.  These declarations belong to their owning or closest
   associated headers.  The prior safe lane blast-tested their deliberately
   narrow placement against VC7 definition-position sensitivity; duplicating
   them here is both stale and contrary to the prototype-ownership rule.
3. **Private view relocation and type renames:** the donor moves the same
   `rasterizer_xbox_rasterizer_globals` overlay about two thousand lines earlier,
   renames `rasterizer_frame_statistics_globals` to the less specific
   `rasterizer_frame_statistics_prefix`, and renames
   `rasterizer_model_skinning_parameters` to
   `rasterizer_model_skinning`.  None changes the function frontier.  Canonical
   retains its owner-consistent names and already validated definition schedule
   rather than risk allocator changes for a cosmetic move.
4. **Raw-address data owners:** the donor defines the D3D aggregate as
   `bss_0045e028`, the hardware-state cache as `data_0030ceec`, and the blend
   table as `rdata_0028bfac`, then hides two of them behind macros.  These names
   fail the explicit global-naming rule and cause the ten relocation-identity
   regressions listed above under the current symbol configuration.  Canonical's
   semantic names match the same bytes and relocations.
5. **`rasterizer_spin_begin` ABI regression:** the donor changes the parameter
   list from `short profile` to `void`.  The owning header and
   `rasterizer_xbox_widgets.c` caller pass `_rasterizer_profile_screen_effect`;
   Stian's recovered source independently corroborates a reason/profile
   parameter and the corresponding `0x1a` call.  A no-op body happens to emit
   the same bytes either way, but erasing a real parameter is a fake semantic
   match and is rejected.
6. **Raw D3D descriptor literals:** the donor replaces
   `RASTERIZER_TARGET_RENDER_PRIMARY_COMMON`,
   `RASTERIZER_TARGET_RENDER_PRIMARY_SIZE`, and
   `RASTERIZER_TARGET_RENDER_PRIMARY_FORMAT` with `0x00040001`, `0x271df27f`,
   and `0x00011229`.  The values are identical, so this gains nothing and loses
   project vocabulary.  Canonical keeps the named constants.
7. **Trailing whitespace:** the donor adds a blank line at EOF and consequently
   fails `git diff --check`.  It is rejected.

## Representation-cast observation

Both canonical and the dirty donor currently spell part of
`__rasterizer_window_set_fog` by casting the camera position to
`real_vector3d *` for `dot_product3d`.  This is pre-existing canonical debt,
not a donor-only hunk.  Two bounded scratch-only tests replaced it with direct
component products, once in each natural operand order.  Both retained the
1,136-byte section and normalized instruction sequence but changed the ordered
global-field relocation slots, so neither matched January.  Expanding this
dirty-donor audit into a speculative rewrite would violate its scope; the
source remains unchanged.  A future focused source-quality lane may revisit it
only with new evidence for January's original helper spelling.

## Preserved fuzzy boundary

The previously admitted six coherent bodies remain parked without change:
`_SetupSmartStates` (128 bytes), `__rasterizer_window_begin` (416),
`_rasterizer_set_frustum_z` (320), `_rasterizer_set_model_skinning` (320),
`_rasterizer_set_stencil_mode` (784), and `_rasterizer_set_texture` (496 target /
432 base).  The remaining 37 target functions remain unwritten.  No fake
dependencies, address names, representation punning, raw offsets, volatile or
register forcing, barriers, pragmas, dummy control flow, inline-schedule
changes, or speculative large bodies were added.

## Final result

The admissible result is deliberately a documentation-only audit:

- strict exact functions: **+0**
- padded code bytes: **+0**
- meaningful code bytes: **+0**
- data bytes: **+0**
- rename-stable regressions: **0**
- source/header/configuration changes: **none**

## Verification

The documentation commit was tested as an unambiguous repository state:

- `ninja all_source progress semantic_progress`: pass.  Totals remain
  878,556 / 2,198,102 meaningful code bytes, 6,048 / 11,060 credited
  functions, 2,018,310 / 4,176,062 data bytes, and 391 / 833 linked objects.
- `python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox
  --all`: 52 exact / 6 residual / 37 unwritten.
- Rename-stable snapshot diff over all 8,245 functions: **0 gains / 0 padded
  bytes / 0 regressions**; both snapshots contain 6,095 strict-exact
  functions.
- Translation-unit regression manifest: all 52 accepted rasterizer functions
  remain exact; zero failures, warnings, changed non-exact sections, or newly
  exact claims.
- Semantic audit: 473 units, 6,474 functions evaluated, 6,104 semantic exact,
  169 hidden exact / 101,147 hidden bytes, 6,121 accepted exact, and zero unit
  errors.
- Object-admission audit: zero candidates, contradictions, or revocations;
  only the two inherited `player_rumble` and `models` rejections remain.
- Park audit: 223 active / 0 stale / 0 invalid.
- Fake-match scan of `rasterizer_xbox.c`: zero review leads and zero skipped
  paths.
- Protected Units sentinel
  `_unit_preprocess_node_orientations`: exact at 1,920 padded bytes.
- COFF symbol census of the rebuilt `rasterizer_xbox.obj`: zero
  `point_from_line3d` symbols or COMDAT owners.
- `python -B -m pytest tools -q`: 261 passed.
- `git diff --check`, source-diff, and final worktree checks: clean.
