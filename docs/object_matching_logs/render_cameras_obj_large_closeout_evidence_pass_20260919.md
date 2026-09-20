# `render_cameras.obj` large-closeout evidence pass (2026-09-19)

## Result

The object is now **18/21 strict exact**. The admissible production change
closes `_render_frustum_cube_visible` for **+715 meaningful / +720 padded
bytes**. No header or configuration changed. A focused whole-TU gate passed
the emitted-owner guard for `_point_from_line3d` and preserved all 17
previously exact siblings.

| function | target padded bytes / relocations | current/best status |
| --- | ---: | --- |
| `_render_camera_build_frustum` | 3,376 / 113 | current 3,408 / 112; best evidence packet reaches 3,376 / 113 but retains 48 x87 scheduling regions |
| `_render_camera_mirror` | 864 / 28 | exact size/relocations; one final z-row x87 keep-versus-consume region differs |
| `_render_frustum_cube_visible` | 720 / 4 | **strict exact**; direct dot-product spelling plus four real reverse-test bit temporaries |
| `_render_frustum_sphere_visible` | 432 / 6 | exact size/relocations; the January 0/2/1 return-block specialization and 16-bit shared exit differ |

The best build-frustum packet remains
`C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/w3_render_cameras/bf5a.c`.
It includes the diagnostic `_pi` correction and is not production-ready as a
standalone C-file transplant.

## New primary-evidence pass

- The HCEA PDB marks all four functions `hasInlineAssembly:false` and
  `WasSpecifiedAsInline:false`, and identifies the original source as
  `d:/projects/code/hcex/sources/render/render_cameras.c` (MD5
  `0CD980A71A7B8019A5E19F5459E2A040`). Authentic locals include
  `z_axis`, `y_axis`, `x_axis`, `viewspace_plane`, `viewspace_normal`,
  `frustum_midpoint`, `frustum_vertex`, and `clip_plane` in build-frustum;
  `plane`, `adjusted_plane`, and `adjusted_normal` in mirror; and
  `bounding_vertices[8]` in cube-visible. The PDB has no injected source or
  source-server payload.
- HCEA line records for sphere-visible put the compound classification at
  line 829, the `2` assignment at 832, the `1` assignment at 837, and the
  shared return at 838. This authenticates a result-variable topology and
  rejects artificial `goto`/duplicated-return steering even though such forms
  can alter the January block order.
- The official October 12, 2001 Xbox prototype contains normalized functions
  identical to January at `_render_camera_build_frustum` `0x187250` (1,978
  matching windows), `_render_camera_mirror` `0x186ef0` (533),
  `_render_frustum_cube_visible` `0x1867f0` (649), and
  `_render_frustum_sphere_visible` `0x186ac0` (331). It contains no usable
  CodeView/source payload.
- A fresh Ghidra decompile of the January executable was produced from the
  read-only `january-exe` project at VAs `0x576310`, `0x576740`, and
  `0x576AA0` (`scratch/ghidra_render_cameras_january.c`). Sphere-visible
  confirms the six nested plane tests, five retained signed distances, the
  negated-radius classification, and the 0/1/2 result semantics. Mirror
  confirms the natural `point_from_line3d` topology, while build-frustum
  confirms the recovered axis/plane/midpoint/vertex local graph. This closes
  the remaining semantic-evidence gap; the outstanding differences are
  compiler ownership/layout choices rather than unknown algorithms.
- The leaked symbol-build IDA records, HCEA reverse source, Stian's current and
  historical reconstruction, the local punpckhdq snapshots, Marathon, and Git
  unreachable history were searched. The punpckhdq main snapshot has only an
  empty object shell for this unit; the PR snapshots contain the same shell.
  Stian implements build-frustum only and uses raw-address reconstruction, so
  it is not an authentic donor for the residual source shapes.
- A same-VC7 target census for the distinctive `mov ax,2` / `mov ax,1`
  return pair found only sphere-visible, cube-visible, leaf-map
  `_intersect_planes3d`, and exact decals `_projection_from_vector3d`.
  The exact source control uses separate specialized epilogues and therefore
  does not provide the missing shared-exit lowering rule.
- The HCEA reverse reconstruction independently recovers a real `bit`
  temporary for the AABB face tests. Replacing four dead/constant-only
  allocation aliases from an archived exact experiment with four directional
  `word` temporaries (`right_bit`, `bottom_bit`, `top_bit`, `near_bit`) gives
  each local a genuine comparison result and immediate use. Together with the
  direct `dot_product3d(...) - plane.d` spelling, this is strict exact without
  changing the reverse-test machine code.

## Lawful source probes

The HCEA sequential-return sphere form, positive direct returns, a
function-scope result, block-scope result, compound fail conditions, ternary
initializers, whole-expression short casts, nested assignments, and ordinary
if/else/negated condition families were compiled. They either preserve the
current residual or grow to 448 bytes with duplicated epilogues. The closest
authentic stream has the same 157 decoded instructions and exact plane-test
ordering, but VC7 places the shared zero return at the end and specializes
constant returns as 32-bit EAX writes; January places the zero block before
the success classifier and carries the 1/2 phi in AX. Block-local short
returns recover the AX writes but duplicate the epilogues, so they are useful
negative evidence rather than an admissible closure. Mirror
helper/operand/scoping families were rechecked against the new evidence. For cube-visible, plane
pointers, hoisted loop pointers/flags, bound-coordinate aliases, one reused
`bit`, and the HCEA first-loop `bit` spelling all remained residual or grew the
body. Four compile-time flag aliases also matched but were rejected because
they existed only to steer allocation. The landed form is the smallest exact
form in which all four additional locals carry real algorithm state.
Build-frustum's natural helper-routing packet remains the closest source, but
the residual regions are x87 rank choices and not missing semantic work.

## Verification of the landed cube closure

- whole-TU gate: **18 exact / 3 residual / 0 unwritten**;
- `_render_frustum_cube_visible`: **720 bytes / 4 relocations / strict exact**;
- emitted-owner guard: `_point_from_line3d` absent;
- fake-match scan: 0 review leads;
- owner census versus `build/base`: 98/98 non-section owners, no candidate-only
  or base-only owner;
- data/BSS/rdata census: 57/57 owners, no candidate-only or base-only owner;
- `git diff --check`: clean.

## Durable blockers and reopen criteria

- Build-frustum: land the genuine `_pi` header correction only with a full
  includer regression audit, then reopen when original source/IL or a measured
  same-compiler rank law explains the remaining x87 sites.
- Mirror: reopen with authentic evidence for a source distinction that keeps
  the joined offset through the final z row. Inline assembly is excluded by
  the PDB and by the house rules.
- Sphere-visible: reopen with original source/IL or a source-known VC7 sibling
  that specializes only the zero return while sharing a 16-bit 2/1 exit.

Do not use artificial gotos, dead locals, operand reversal solely for register
steering, inline assembly, or any form that emits `_point_from_line3d`.
