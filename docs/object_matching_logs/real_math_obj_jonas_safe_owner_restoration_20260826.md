# `real_math.obj` safe owner restoration wave

## Scope

- Production translation unit: `source/math/real_math.c`
- Public declarations: `source/math/real_math.h`
- January object: `build/split/source/math/real_math.obj`
- Candidate object: `build/base/source/math/real_math.obj`
- Compiler: XDK 3911 VC7 `CL.Exe` 13.00.9254.1 through the generated
  repository edge
- Starting HEAD: `940b894d`

The acceptance authority is padded section bytes plus ordered relocation
address/type/destination/addend identity.  Fuzzy scores and whole-object hashes
are diagnostic only.

## Stale-object discovery

The inherited candidate object reported 79 exact functions and five nonexact
functions, but the checked-in source did not define any of those five owners:

- `_pill_test_vector3d`
- `_point_in_triangle3d`
- `_sphere_intersects_triangle3d`
- `_pill_intersects_triangle3d`
- `_angular_accelerate_to_position`

Deleting only `build/base/source/math/real_math.obj` and rebuilding it from the
authoritative source made all five symbols disappear.  The apparent residual
frontier was therefore a stale build artifact, not a reproducible source
frontier.  The forced baseline was **79 exact, 0 emitted nonexact, 5 missing**,
with 17,952 exact padded code bytes.

This also explains why an ordinary incremental full build had previously
preserved the old result: the checked-out source timestamp was older than the
already-built object, and the generated Ninja edge has no content hash.

## Provenance

The repository's authenticated closeout state at `3b8b707e` supplied the
target-focused semantic bodies and an independently forced 81/84 record.  It
could not be restored literally because it used prohibited constructs:

- pointer/type punning in `_point_in_triangle3d` and `_pill_test_vector3d`;
- `_ReadWriteBarrier` in `_pill_intersects_triangle3d` and
  `_angular_accelerate_to_position`;
- a forced-inline third-edge helper in `_sphere_intersects_triangle3d`.

Independent public corroboration:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
  - `src/blam/physics/point_in_triangle3d.c`, SHA-256
    `56f0b98fa8a71c275f52533e469e1caa2f5f96b6b75957d5e1f5076aa6e35dd4`
  - `src/blam/physics/pill_test_vector3d.c`, SHA-256
    `173e9e9883dcc9ce397a3ef9b87bc44e615249ccda9b280dd4de47839f6b2516`
- Stian Eklund corpus commit
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`
  - `src/halo/math/real_math.c`

HCEA independently confirms direct scalar projection and normalization without
the January-branch pointer casts.  Stian independently confirms the triangle
and angular semantic topology.  Raw addresses and donor forcing constructs
were not transplanted.

## Retained legal reconstruction

All five owners are now readable C89 definitions with public declarations.
The retained code adds no assembly, `register`, `volatile`, barrier, pragma,
intrinsic, attribute, raw address, pointer/integer reconstruction, pointer or
union pun, undefined behavior, synthetic anchor, byte forcing, object patch,
comparator exception, or compiler substitution.

The final strict frontier is **79 exact, 5 emitted nonexact, 0 missing**.  All
79 previously exact functions remain exact and still own 17,952 padded code
bytes.  Target-owned data remains unchanged and exact.

| Owner | Target/base padded size | Relocations | Exact relocation address/identity pairs | Normalized differing positions | First difference | Target SHA-256 | Base SHA-256 |
|---|---:|---:|---:|---:|---:|---|---|
| `_pill_test_vector3d` | `448/464` | `9/9` | `8/9` | 95 | `+0x9F` | `2d1aad933f29...` | `066fbb978c3d...` |
| `_point_in_triangle3d` | `432/432` | `9/9` | `9/9` | 20 | `+0x30` | `f0c19ce34603...` | `b13147e046fc...` |
| `_sphere_intersects_triangle3d` | `608/624` | `6/6` | `4/6` | 179 | `+0xE1` | `aeba141c144e...` | `f59114304434...` |
| `_pill_intersects_triangle3d` | `832/848` | `12/12` | `8/12` | 385 | `+0x05` | `ca0e4598c4ac...` | `50a9ae7951cd...` |
| `_angular_accelerate_to_position` | `576/576` | `13/13` | `13/13` | 12 | `+0x127` | `4cd16f09784b...` | `62d0c46e2eea...` |

### Point/triangle controls

1. HCEA-style direct projection removed the unsafe casts but produced
   `448/432` and `8/9` relocations because January calls `project_point3d`
   three times.
2. Typed `real_point3d` intermediates plus explicit vector arithmetic restored
   the three real projection calls and reached `432/432`, `9/9`, initially
   with every relocation nine bytes late.
3. Mirroring the authentic inline cross-product's `k`, `j`, `i` evaluation
   order reduced the uniform displacement from nine bytes to three.
4. Target/candidate disassembly proved the remaining three bytes were
   `push ebx; xor bl, bl`, caused by eager `result = FALSE`.  Restoring the
   historical branch-local false assignments removed them and placed all nine
   relocations exactly.
5. The remaining 20 differences are ModRM stack-slot displacements only.
   Narrowing the scalar scopes was byte-neutral, establishing the current
   ordinary-C stack-allocation tie.  Declaration-order sweeping was stopped.

### Pill/vector controls

1. HCEA's manual two-dimensional normalization was safe, but produced
   `480/448`, `10/9` and omitted January's `_fast_normalize2d` relocation.
2. A typed `real_vector2d` local restored the real helper call and reached
   `448/448`, `9/9`.
3. The authenticated January control flow (`goto done`, `top_z`, and delayed
   result paths) plus reuse of the existing typed 2D offset made the first
   eight relocation addresses exact.  The helper call is one byte late and
   the function pads 16 bytes larger because safe code must copy the normalized
   two-component result into the three-component output.  Passing the 3D
   output to a 2D helper by cast is prohibited and was not restored.

### Sphere and pill/triangle controls

The initial safe generic reconstructions retain ordinary vector helpers.
Sphere has the first four relocation addresses exact and localizes the mismatch
to the third edge.  A direct third-edge scalar control reached `608/608`, but
globally disturbed all six relocation addresses; it was rejected and reverted.
The older forced-inline helper is prohibited.

Pill/triangle retains a plain scalar `boolean outside` and no barriers.  The
historical `outside[4]`/zero-byte-barrier shape is not a readable semantic data
model and was not restored merely to force VC7's schedule.

### Angular control

The historical body without `_ReadWriteBarrier` reproduces the complete
`576/576`, `13/13` structure.  Twelve scheduling bytes remain.  The old
nine-byte state depended on the prohibited zero-byte barrier, so the safe
twelve-byte residual is the accepted frontier.

## Do not repeat

- Do not trust `build/base/source/math/real_math.obj` without deleting and
  rebuilding it first.
- Do not restore the 81/84 historical file wholesale; it violates current
  house rules.
- Do not cast point/vector union pointers merely because their layouts match.
- Do not cast a `real_vector3d *` to `real_vector2d *` for normalization.
- Do not restore `_ReadWriteBarrier`, `volatile`, or forced-inline helpers.
- Do not repeat the rejected direct third-edge sphere control; exact padded
  size came at the cost of every relocation address.
- Do not declaration-order sweep `_point_in_triangle3d`; the remaining 20
  bytes are a proven stack-slot allocation permutation after authentic scope
  controls.

## Validation

- VC7 `/Zs` parse gates passed before each retained code-producing wave.
- A forced candidate-object replay emits all 84 owners.
- Whole-TU strict census: 79 exact, 5 nonexact, 0 missing; no exact sibling
  regression.
- `halobetacache_build`, `libcmt_build`, `semantic_progress`, and `progress`
  pass.
- Semantic report: 4,338 functions evaluated, 4,191 semantic exact, 4,252
  accepted, zero unit errors.
- All 179 repository tooling tests pass.
- Object admission reports 0 candidates, 0 contradictions, and 0 revocations.
- Parked-function audit reports 3 active, 0 stale, and 0 invalid entries;
  none belongs to this source-absent frontier.
- `git diff --check` passes, and the added-line policy scan finds no prohibited
  barrier, `volatile`, `register`, forced-inline, attribute, pragma, or typed
  point/vector pointer cast.
- Campaign progress remains 377/833 complete objects overall and 275/468 Halo
  objects; this wave repairs source reproducibility rather than claiming a new
  exact owner.

The object remains `NonMatching`.  The next strongest legal lever is the
one-byte-late `_fast_normalize2d` call/copy-out boundary in
`_pill_test_vector3d`, but it should be reopened only with a genuinely typed
API/topology control that does not regress the exact `fast_normalize2d` owner
or other callers.
