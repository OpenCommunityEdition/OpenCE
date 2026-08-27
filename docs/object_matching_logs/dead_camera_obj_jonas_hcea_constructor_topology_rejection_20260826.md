# `dead_camera.obj` HCEA constructor-topology rejection (2026-08-26)

## Result

The HCEA reconstruction supplied independent source evidence for the two
remaining `_dead_camera_new` residual regions. One minimal typed-C adaptation
was compiled once. It regressed the target-aligned 288-byte envelope to 272
bytes and was inverse-reverted wholesale.

`source/camera/dead_camera.obj` remains honestly `NonMatching` at 2/4 strict
functions. Both private helpers and both target-owned runtime data sections
remain strict exact. No source, configuration, semantic exception, parked
entry, object status, protected Units/AI path, compiler option, comparator
rule, or target artifact survives this wave.

## Provenance and prior boundary

The user-supplied `surreptitiousresearch/halocea` checkout at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains
`src/blam/camera/dead_camera_new.c`, raw SHA-256
`6bef46e49bd92a30fd8073dd0fe9d75599931917155bb05eee26f37c64cb1ea5`.
It assigns the negated random pitch directly to the camera before storing the
timer, then stores `current_player_index` before choosing the watched unit.

Those two differences align with the only constructor residuals identified in
`dead_camera_obj_codex_checkpoint.md`: the final random-pitch schedule and the
watched-unit/player-index store schedule. All reachable local source history
retains the current temporary-pitch and unit-before-current-player form, so the
HCEA form is new provenance rather than a repeated local experiment.

Only these two ordering/lifetime facts were adapted. The candidate retained
the established typed camera/player interfaces, assertion, one parameter per
line, and explicit terminal return. It imported no raw offsets, address casts,
missing house style, PowerPC-specific lowering, or donor declarations. It used
no assembly, `register`, `volatile`, barrier, pragma, intrinsic, attribute,
pointer-integer reconstruction, pointer/union pun, undefined behavior,
synthetic anchor, byte forcing, object patch, compiler change, or flag change.

## Frozen candidate and measurement

Starting integration commit:
`6736516578a6009687e11d3f92237144098b4108`. The restored source Git blob is
`d8dfb51c1b2c0b769e058d3c40c95ab3c919a92f`. The source-identical baseline
object was preserved at ignored path
`build/audit/dead_camera_before_hcea_constructor_topology_20260826.obj`, raw
SHA-256
`2671a9d5c074ab68c94394dd04ce043721eee4bfde497df226a189188375c868`.

The frozen candidate removed the named `pitch` local, assigned the negated
random result directly to `camera->facing.pitch` before the timer store, and
moved `camera->current_player_index = player_index` before the watched-unit
branch. A Ninja dry run exposed exactly one normal compile edge, and exactly
that edge compiled once:

```text
[1/1] CL build\base\source\camera\dead_camera.obj
dead_camera.c
```

The unchanged first-shot object is preserved at ignored path
`build/audit/dead_camera_hcea_constructor_topology_first_shot_20260826.obj`,
raw SHA-256
`8bd63446cf824de766fbdadde993720328c6fa616459c657ec5b98ce711f3bf0`.

| Measurement | January | Baseline | First shot |
| --- | ---: | ---: | ---: |
| Padded bytes | 288 | 288 | 272 |
| Relocations | 19 | 19 | 19 |
| Normalized differing positions | 0 | 32 | 68 |
| Total absolute relocation displacement | 0 | 1 | 11 |
| Normalized SHA-256 | `3888f6d2d64a...` | `6049682ee762...` | `486b54bdc99e...` |

The baseline already matches the first eleven relocation sites exactly and
places only the timer-table relocation one byte early. The HCEA candidate
moved that timer relocation five bytes late, while the final player-data and
`datum_get` relocations moved three bytes late. Normalized distance more than
doubled and the function lost its target-sized padded envelope.

The hardened comparator found `_dead_camera_new` to be the only changed code
owner. `_code_000740b0` remains strict exact at 112 bytes and six relocations;
`_code_00074120` remains strict exact at 176 bytes and six relocations;
`_dead_camera_update` remains byte-for-byte unchanged at 1,248 bytes and 46
relocations. The 12-byte timer table and 36-byte path literal also remain
strict exact.

## Closure

The source was reverted without a second candidate compile. Do not repeat the
combined direct-pitch/current-player-before-unit HCEA family or equivalent
spellings. The readable `_dead_camera_update` residual still cannot reproduce
January's uninitialized fallback home-slot read without forbidden undefined
behavior. Reopen either public function only with closer cache-beta source
provenance or a new, target-specific lifetime fact.

There is no function, byte, data, object, or admission credit and no campaign
total changes. The next production wave moves to another object.
