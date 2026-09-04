# `damage.obj` area-of-effect cluster (2026-09-04)

## Result

This isolated January-PC packet reconstructs the two-function area-of-effect
damage cluster in `source/objects/damage.obj`:

| Function | January size | Candidate size | Relocations | Result |
| --- | ---: | ---: | ---: | --- |
| `area_of_effect_cause_damage` | 116 meaningful / 128 padded | 128 padded | 4 / 4 | strict exact |
| `area_of_effect_cause_damage_to_object` | 1,316 meaningful / 1,328 padded | 1,312 padded | 51 / 52 | 95.47282%, credibly fuzzy-parked |

The focused unit advances from **22 exact / 0 residual / 12 unwritten** to
**23 exact / 1 residual / 10 unwritten**. The rename-stable 8,245-function
sweep records **+1 strict-exact function / +128 padded bytes / zero
regressions**. In meaningful-byte reporting this is +116 strict bytes. The
complete helper contributes a separately identified **1,328 padded / 1,316
meaningful fuzzy bytes** at 95.47282%; it is recorded in `config/parked.json`
but is not presented as strict match credit.

## Isolation and authority

Work was performed only in
`worktrees/damage-aoe-cluster-20260904`, branch
`agent/damage-aoe-cluster-20260904`, from canonical commit
`4453bedd7b6271763e7411521caa5f80d65cb9e7`. The unrelated saved-project
checkout, its bitmap changes, and its untracked files were not used or edited.

The pristine January split object is
`C:\halo-worktrees\fable-small-families-20260901\build\split\source\objects\damage.obj`,
SHA-256
`3FE5495D2A6CE3075C3FA6B04782CB7BBF97C3A3890F2E6F6F4F29EF8E5B08EC`.
Its bytes, COFF symbol storage classes, ABI, section boundaries, instructions,
and relocations are the sole match authority. After the semantic helper rename,
the lane-local regenerated target has SHA-256
`D7D5A055CAA62C22FAAAB1E8C16CC00EED863BE68E5E5DFF581BCB690CBF8224`;
the final rebuilt candidate object has SHA-256
`7A8E597BEE163A8DBB952389B2BA494ADEB54C21BC73831357B71B69BFF786BD`.

The required pre-work audit included all five prior damage ledgers:

- `damage_obj_codex_checkpoint.md`;
- `damage_obj_wave2_jonas_20260820.md`;
- `damage_obj_jonas_object_deplete_body_20260830.md`;
- `damage_obj_jonas_object_destroy_20260830.md`;
- `damage_obj_small_vitality_leaves_20260904.md`.

The paired `CLAUDE.md` and `AGENTS.md` instructions at historical commit
`76ed248678d92b401b8ff73913d756050129481f` were also read in full before
source work. Stian's January-PC lift and the independent HCEA reconstruction
were used only as semantic/name evidence. Neither donor was allowed to override
January code or layout evidence.

## Recovered source and naming

`_code_00128150` is renamed
`_area_of_effect_cause_damage_to_object`. The name is strongly corroborated by
HCEA's named helper and wrapper and by the same helper-to-wrapper placement and
three-argument call topology in January and Stian. January's COFF symbol table
marks both functions `External`; consequently the helper has external linkage
and a same-TU prototype, while only the public wrapper remains in `damage.h`.
No address-derived live name remains in this cluster.

The exact wrapper uses the January-specific shape:

- `long object_indices[64]`;
- the typed damage-effect definition and `cutoff_radius`;
- `objects_in_sphere`, followed by one helper call per result;
- `FALSE` for sibling traversal at the wrapper boundary;
- `breakable_surface_damage_area_of_effect` after the object loop;
- an intentionally unused, neutrally named second parameter.

It deliberately excludes HCEA's later 72-element result array and
double-radius cheat branch. It also excludes the later
`unlucky_object_index` interpretation, for which January has no use.

The reconstructed helper is complete ordinary typed C. It performs:

- the authenticated collision-user push/pop assertions;
- January's four-ray obstruction test for visible units when core radius is
  nonzero, and the direct-ray path otherwise;
- owner and friendly-fire filtering;
- January's infection-form pop/toughness/random behavior;
- area-of-effect flagging and typed distance falloff;
- `object_cause_damage` with the January argument topology;
- collision-model resistance-controlled child traversal;
- damaged-one-object bookkeeping; and
- optional next-sibling traversal.

All field access uses typed object, definition, unit, collision-model, and
damage-effect owners. The source uses project `real`, `boolean`, enums, flag
macros, vector helpers, and tag/object accessors. Functions have explicit
returns, `void` is used for no-argument declarations where applicable, and
each new parameter occupies its own line.

## Strict and fuzzy evidence

The public wrapper's target and candidate measurements are identical:

- size: 128 padded bytes;
- relocations: 4;
- normalized SHA-256:
  `b98879e0ee0e379da619f60c4a8ce09138f4e332f9010019a95b20be17003e77`.

The helper park is measurement-locked as follows:

| Measurement | January | Candidate |
| --- | --- | --- |
| padded size | 1,328 | 1,312 |
| relocation count | 51 | 52 |
| normalized SHA-256 | `d3f929d4ce73b3cd08de3cd85da26284c7ad3b7e3574a856192f9fa0b40221a1` | `46952a116c873e6f604b0e33c5b3e8b8e2a581b7c8afbd46364621b05d361abf` |
| objdiff | 100% authority | 95.47282% |

The retained natural source improved from the first complete helper's 84.01%
to 95.47282% by using independently meaningful vector temporaries, explicit
ultimate-parent lifetimes, and the January-compatible four-ray loop count.
Bounded ordinary-C probes then covered direct and assigned sibling parameters,
early-return recursion, clamp expression forms, declaration lifetimes, and
loop spellings. The natural recursive variants converged on the same result.
Explicit backedge/goto diagnostics disturbed register allocation and enlarged
the section, so they were rejected rather than retained as codegen steering.

The remaining measured boundary is concentrated in two compiler decisions:

1. January turns next-sibling self-recursion into parameter stores and a jump
   to the function top; the current natural VC7 compilation retains a call,
   accounting for the extra relocation and affecting prologue alignment.
2. January's x87 falloff clamp uses a different exchange/pop schedule from the
   natural project `PIN` expression.

The fail-closed blocker classifier returns `UNKNOWN`; the park is therefore
honestly `unclassified`, with no inferred compiler recipe. Closing the final
bytes would presently require unsupported source contortions or optimizer
steering, so the complete coherent implementation is retained at the measured
fuzzy boundary.

## Credibility and emission audit

No inline assembly, volatile/register steering, barrier, pragma, forced
inline/noinline, raw offset, address name, representation pun, dummy
dependency, undefined behavior, contradictory condition, or nonsensical
branch is present. No later-build gameplay behavior was imported. A symbol
scan of both the rebuilt and regenerated target objects finds the two semantic
area-damage functions and no `point_from_line3d` symbol or COMDAT, preserving
the required inline schedule.

## Verification

- Full `ninja halobetacache_build libcmt_build semantic_progress progress`:
  pass.
- Final focused `damage.obj` gate: **23 exact / 1 residual / 10 unwritten**.
- Rename-stable snapshots: 8,245 functions, 6,130 to 6,131 exact;
  **+1 / +128 padded bytes / 0 regressions**.
- Semantic report: 473 units scanned, 6,514 functions evaluated, 6,140
  semantic exact, 6,156 accepted exact, and zero unit errors.
- Overall strict progress: 6,082 to 6,083 functions and 885,642 to 885,758
  meaningful code bytes (**+1 / +116 meaningful bytes**).
- Protected Units gate: **189 exact / 0 residual / 0 unwritten**.
- `python -B -m tools.parked_functions`: **231 active / 0 stale / 0
  invalid**, including this helper's measurement-locked entry.
- `python -B tools/audit_object_admission.py`: zero candidates,
  contradictions, or revocations; the two inherited reviewed rejections are
  unchanged.
- Changed-file fake-match scan: two files, zero review leads.
- `python -B -m pytest tools -q -p no:cacheprovider`: **261 passed**.
- `git diff --check`: clean apart from informational line-ending warnings.

The final `damage.c` SHA-256 is
`472985BF6EFC19C3E50252CCCA5C34B39E8DA63AD9ECBC9DC97FF74999962543`.
Lane-local stable snapshots and diagnostic candidates remain under
`scratch/damage-aoe-cluster-20260904` and are intentionally uncommitted.
