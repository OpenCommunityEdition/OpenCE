# Cinematics render residual boundary (Jonas, 2026-08-30)

## Result and scope

This evidence-only closeout starts from canonical commit
`11d27dcf5f5c4bce837df5a1571ec7ca6c23b415`.  The ordered primary,
`source/render/render_particles.obj::_render_particles`, was released without
editing or compiling because the current Render Particles ledgers already
record a complete fixed point: the best 1,168-byte candidate differs in only
45 stack-slot displacement bytes after the lifetime/source-shape matrix and
public-donor audit were exhausted.

The fallback was `source/cutscene/cinematics.obj::_cinematic_render`.  It is
also a measured fixed point under the current ordinary-C campaign policy.  No
production source, header, configuration, generated object, completion label,
semantic exception, or parked-function record is changed by this closeout.
No new candidate compile was taken: the authenticated earlier experiment
matrix already proves that the only missing code-generation primitive is the
prohibited original inline-x87 `fast_ftol` helper.  Recompiling the same
ordinary C would duplicate a closed experiment rather than test new evidence.

The current Cinematics census remains 15/17 strict-exact functions.  The two
absent owners remain `_cinematic_set_title_delayed` and `_cinematic_render`.
The previously accepted `_draw_quad` and all other inherited exact owners are
untouched.

## January authority and exact measurements

- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/cutscene/cinematics.obj`: SHA-256
  `a5e6cd5d77d661d9b47f59d9b1c6bf6a83051a62a977706dcadacbc4e19074f5`.
- January `_cinematic_render`: 1,280 padded bytes, 57 ordered relocations,
  normalized SHA-256
  `a89dcee38e6a239615cde9f2d1f3b2fc577ced717e620de08a6397f74a0e6288`.
- Best policy-legal default-flag `_cinematic_render`: 1,088 padded bytes,
  59 ordered relocations, normalized SHA-256
  `695f78e01dec9162f99d799ffdf4a6d9ecf409c05e0159683e7203b4e715fb0e`.
  The candidate is 192 padded bytes short and has two surplus relocations.
- January `_cinematic_set_title_delayed`: 144 padded bytes, seven ordered
  relocations, normalized SHA-256
  `2efe5dea0cc04b8a14adb1524c901f2709dc4dafb36e8a5c4f4518fef287a252`.
- Best policy-legal default-flag `_cinematic_set_title_delayed`: 128 padded
  bytes, eight ordered relocations, normalized SHA-256
  `9e0b669abf2cb0dd6af4096f5973f52412f081ccb5f87a342d6e78b50f33615c`.

The candidate measurements above come from the preserved Claude experiment
object with SHA-256
`84d5edd2bc04ac9639f74e9cd5bea79859659a443ed6307d29d450cf8977267e`.
The complete Claude experiment log has Git blob
`dc893145b0a3ad55a1a0634b2802e42702120442`; its natural typed source packet
has blob `11d39ed66e1d3d4ec3416b3c8b3469277c2a8298`.  Both are evidence only and
remain outside production.

## Donor, history, and compiler audit

The HCEA donor
`reference/halocea/src/cinematic_render.c` supplies the public control-flow
and typed field blueprint.  Its later-build differences were already
reconciled against January disassembly and PDB layouts: January passes authored
title bounds directly, uses the January 0x60-byte chapter-title layout, keeps
distinct letterbox scratch quantities, initializes fade before the bounds
fallback, derives shadow alpha directly from the title field, and still adds
zero while title timing is paused.  The older pastudan tree has only rough
raw-offset drafts and empty shells, so it contributes no admissible production
shape.  The punpckhdq tree has no residual body.  The full worktree census
found the same Claude experiment family and no independent exact ordinary-C
donor.

All recorded natural-source experiments converge on one provenance boundary:

- January materializes the viewport/bar coordinates and the shadow-alpha
  clamp with an inline `fistp dword ptr [...]` conversion and has no
  `__ftol2` relocations for those operations.
- Ordinary C casts under the production flags call `__ftol2`; the best render
  packet consequently contains the missing five `__ftol2`-shape differences
  and cannot reproduce January's 32-bit x87 homes.
- `/QIfist` was tested with CL 13.00.9210, CL 13.00.9254, and XDK 3911
  CL 13.00.9254.1.  Every available compiler emits `fistp qword ptr [...]`,
  not January's dword form.
- Named real, long, and short lifetimes, return and pointer-output forms,
  aggregate and bitfield destinations, and the available precision/backend
  switches did not produce the January primitive.
- January's standalone `_fast_ftol` in `actor_combat.obj` and HCEA's
  `fast_ftol` source identify the original primitive conclusively: a
  hand-written inline-assembly `fld`/dword-`fistp` helper.

Inline assembly, `volatile` code-generation forcing, intrinsics, barriers,
pragmas, raw addressing, type punning, undefined behavior, byte forcing, and
tool/config exceptions are prohibited.  Therefore the natural typed C89
implementation cannot be retained under current policy even though its
control flow and data layouts are reconstructed.

## Durable reopening rule

Do not retry either Cinematics residual through declaration order, local
lifetime, cast spelling, `/QIfist`, or another copy of the HCEA body.  Reopen
only if one of these genuinely new authorities exists:

1. authenticated January source or local-variable records that reveal a
   different ordinary-C primitive;
2. an approved, provenance-faithful implementation of Bungie's original
   `fast_ftol` helper; or
3. a newly discovered compiler intrinsic/backend that emits the exact 32-bit
   `fistp` sequence while satisfying the campaign policy and exceptional-value
   semantics.

Until then, `cinematics.obj` remains honestly partial at 15/17 functions; this
ledger claims no code, data, or completion gain.

## Restored-state gates

The untouched canonical production tree was configured and rebuilt with XDK
3911 CL 13.00.9254.1 under the ordinary `/O2 /Oy- /DDEBUG /Dxbox` edge.  Full
`halobetacache_build`, `libcmt_build`, report, semantic report, and strict
progress generation pass.  Direct hardened comparison finds all 15 emitted
Cinematics functions strict exact; `_draw_quad` remains 336 padded bytes,
nine ordered relocations, and normalized SHA-256
`98221d0a3037338ddd2696a341a859d48bc6eecd9e92643a6d4d8e2cbc7fb44a`.
The current production Cinematics object has raw SHA-256
`f88310b74b2d447c3200348a3b46601f3d21ec8e44347e7183683740b3830b9d`.

The complete restored-state gates report:

- semantic audit: 470 units, 4,954 functions evaluated, 4,837 semantic exact,
  4,847 accepted exact, 131 hidden exact, and zero unit errors;
- strict progress: 384/833 linked objects, 4,808/11,060 functions,
  610,370/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- Halo progress: 282/468 objects, 4,641/7,574 functions, and 597,456 accepted
  code bytes;
- object admission: zero candidates, contradictions, or revocations;
- parked validation: 12 active, zero stale, and zero invalid;
- tooling tests: 212/212 pass;
- `git diff --check`: pass.

`build/report.json` hashes to
`fd2d950866b443b6c2d66c93c767505d8e8b25e05ebd21382fee29840d1cc466`;
`build/semantic_report.json` hashes to
`3c51c7e32625c13fe2f70adde7cfdcc895f7e69390c7cd5e883243e512370936`.
Protected `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
