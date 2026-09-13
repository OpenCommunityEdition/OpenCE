# `render_cameras.obj`: natural frustum reconstruction (2026-09-13)

## Result

This packet replaces the unwritten January
`_render_camera_build_frustum` body with a complete, typed, ordinary-C
reconstruction and restores its real private warning-helper boundary. It is a
high-quality fuzzy result, not an exact match, and receives zero exact-byte
credit.

The narrow gate moved the 3,376-byte target body from unwritten to residual
without changing any of the translation unit's 12 inherited exact functions:

| function | January | candidate | result |
| --- | ---: | ---: | --- |
| `_render_camera_build_frustum` | 3,376 bytes / 113 relocations | 3,408 bytes / 112 relocations | residual, 88.98% |
| `_render_camera_check_warning_condition` | 160 bytes / 13 relocations | 160 bytes / 13 relocations | byte/shape equal; stale split relocation identities; no credit yet |

The build-split target currently carries the helper's old synthetic
`_code_00174fc0` label. The equivalent PC corpus names the same 160-byte
behavior `render_camera_check_warning_condition` in both
`tools/verify/function_bounds.json` and the reconstructed source. That corpus
also confirms the 64-entry warning array, initialization flag, threshold,
diagnostic, and all 22 callers. `config/symbols.json` now records the semantic
private name. This is cross-build semantic evidence, not a claim that a
January debug symbol survived.

The January comparison sequence uses `value >= 0.05f`, rather than the PC
lift's `value > 0.05f`; the retained source follows January. With that natural
correction, target and candidate helpers contain the same 49 instructions.
The current strict gate differs only because its pre-existing split target
still describes the array and initialized flag as offsets into
`_bss_004b8b28`. Semantic entries for both private globals were added beside
the helper rename, so the orchestrator can regenerate the split object and
verify the expected closure. Until that happens, the helper receives zero
credit.

## Reconstruction evidence

January disassembly and relocations remain the authority for control flow and
object shape. The retained body reconstructs:

- the optional normalized frustum rectangle and viewport-derived projection
  scales;
- the view-to-world basis and inverse world-to-view matrix through project
  math helpers;
- the six planes in left, right, bottom, top, near, far order;
- four far-plane corners, the camera-position apex, the midpoint, and the
  five-vertex world AABB;
- standard and mirrored near-plane projection construction, including the
  January projection-disabled matrix and screen-scale clears; and
- 16 absolute corner/plane checks followed by six signed midpoint/plane
  checks, using named condition and plane enums.

The typed field map and high-level geometry were independently checked against
the HCEA reconstruction in
`halocea-review/src/blam/render/render_camera_build_frustum.c`. The PC semantic
corpus in `stian-current/src/halo/render/render_cameras.c` corroborates the
warning-helper behavior and call ordering. January-only assertions, line
numbers, projection-disabled clearing, x86 helper schedule, and relocation
topology came from the target object rather than either later-build donor.

## Compiler schedule

The default object `/O2` setting auto-inlines the ordinary 160-byte private
warning helper into all 22 sites. That produces a 5,424-byte candidate with
288 relocations, which is incompatible with January's single helper section
and 22 calls.

The translation-unit-local `/Ob1` option permits only explicitly inline
helpers. It therefore keeps the project's intentional math-inline schedule
while preserving the ordinary warning helper as one real function. With
`/Ob1`, the candidate is 3,408 bytes and 112 relocations, all 12 previously
exact functions remain exact, and `_point_from_line3d` is not emitted. The
option is recorded only for `source/render/render_cameras.c` in
`config/config.json`; no source annotation or broader compiler change was
introduced.

The remaining main-body differences are broad VC7 local/x87 scheduling, plus
one fewer zero-literal relocation and the existing project `_pi` expression
folding to `0x40490e37` where January references `0x40490e38`. This packet
does not alter a shared constant or tune a literal merely to improve the
score.

## House-rule audit

The reconstruction uses `real`, `real_vector3d`, `real_point3d`,
`real_plane3d`, `real_matrix4x3`, the owning render-camera structures, named
plane/condition enums, and existing math/assert/error helpers. It contains no
raw float-array lift, hardcoded structure offset or address, foreign local
prototype, volatile/register/noinline forcing, pragma, compiler barrier,
undefined behavior, fake dependency, or hand-expanded math helper. The public
prototype remains in `render_cameras.h`; the private helper and private state
remain translation-unit local; every added function has an explicit return.

## Verification

The bounded final command was:

```text
python tools/campaign/gate.py source/render/render_cameras --all --cflag /Ob1 --out scratch/render_cameras_frustum_ob1.obj --forbid-emitted-symbol _point_from_line3d
```

It reports 12 exact, eight residual, and one target still shown as unwritten
only because the checked-in split object predates the semantic helper rename.
An alias-only diagnostic gate confirms that helper has identical normalized
bytes, size, and relocation count and differs only in those stale BSS
relocation identities.
JSON parsing, the fake-match scan, and `git diff --check` are also required for
the final packet handoff. Per orchestration policy, this bounded worker did
not run configure, a full Ninja build, commit, or push.

Exact meaningful-byte delta: **0**. Exact padded-byte delta: **0**. The
3,376-byte main body is retained strictly as documented fuzzy progress.
