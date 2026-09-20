> Research only: zero admission credit; no retained source/header/configuration change.

# Packet 8: independent preliminary aim-assist review

**HOLD: the genuine cone-owner prerequisite failed its first historical
consumer falsifier. The setter remains uncompiled; zero credit.** The cone
declaration and typed setter operation have primary support, but this cannot
override an inherited exact loss. This reviewer performed
read-only primary extraction and source/history comparison: zero compiler
invocations, zero source/header/config mutations. Root owns any replay and its
complete consumer audit. IDA_AUTOMATION_UNAVAILABLE.

## Cone ABI and owner

Fresh worker DIA records identify `pin_normal_to_cone3d` in
`math/real_math.c` (HCEA source MD5 `92076005323D44C5516925E25ECBC209`), with
parameters `const real_vector3d *normal`, `const real_vector3d *direction`,
`float sine`, `float cosine`, `real_vector3d *result`.
Independent raw MSF7/CodeView extraction adds the return-type proof that the
ordinary DIA function listing omits: `real_math.obj`, stream 2728, S_GPROC32
record 0x1f80 references LF_PROCEDURE 0x6f577. It returns primitive type 0x20
(unsigned char), has five parameters and calling-convention field 0. The
argument list is 0x6f576; the two scalar entries are primitive float 0x40.
This agrees with `boolean` being `byte` and the existing definition at
`source/math/real_math.c:2320`. Do not interpret DIA's PPC register spellings
as January x86 register-allocation evidence.

January `player_aim_projectile` has an ordinary relocation to
`_pin_normal_to_cone3d` at +0x2e5. Its preceding setup pushes result, cosine,
sine, direction and normal, then cleans 20 bytes at +0x2e9. This independently
supports the five-argument call ABI; the caller ignores the return value.
`real_math.h` already has the real-math prototype section and the genuine
vector types. Adding the declaration there is coherent ownership. No evidence
recovers its January declaration position, and no invented focused header or
consumer-local prototype is justified by this review.

## Setter evidence and its limit

Independently re-read the supplied executable (SHA256
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`).
At 0x58a05f..0x58a09c it computes camera position plus the already scaled
displacement, pushes three float values and the camera-position address, and
calls thunk 0x4070f4. Raw E9 resolution reaches 0x4ac810. That callee stores
the three arguments to destination offsets 0, 4, 8 and returns the destination
pointer, ending at 0x4ac83d. The RTC descriptor 0x58a224 identifies the
destination at EBP-64 as the 12-byte `camera_position`; fresh HCEA locals type
that name as `real_point3d`.

Fresh HCEA `set_real_point3d` DIA explicitly names a point pointer and x/y/z
float arguments, marks the function inline/was-inlined, and source lines
920..926 identify `math/real_math.h`. The current real owner body at
`real_math.h:1046` performs exactly those stores and returns the point.
The exact name of the 2020 callee is **inferred**, not recovered from that
executable's symbols. This is nevertheless a concrete typed setter operation
with an existing, independently authenticated owner API; it is more than
byte-match plausibility.

The proposed body differs from the archived complete body at just this site:
replace the in-place `add_vectors3d` point/vector casts with
`set_real_point3d(&camera_position, position.x + displacement.i, ...y..., ...z...)`.
No new helper, invented layout, forced inline option, dummy state, or manual
helper expansion is proposed. The x/y/z fields and three scalar arguments are
real source operations, not slot steering. All input arguments are evaluated
before the setter writes, with no intervening side effect. Header inlining may
still alter emitted ownership and therefore requires an actual audit.

January has the corresponding scaled-displacement sums/stores at
+0x1f9..+0x237 but no setter relocation. That is compatible with inlining and
does not uniquely prove the helper spelling. The later build's source revision
is unverified; its SSE/unoptimized call staging cannot establish January VC7
x87 scheduling. In particular, this evidence does **not** explain or promise
closure of the old second-`normalize3d` push difference at +0x289.

## Prior falsifiers and bounded admission

Read the fresh-graphs and w1/w3 aim-assist ledgers, the fresh-graphs admission
manifest, and actual archived worker/probe files. Archived v9/v12 are direct
component assignments, not calls to `set_real_point3d`; their failure does not
duplicate this new call-boundary test. Already rejected scope, subtraction,
declaration, prototype-position and expression permutations remain excluded.

The archived `scratch/h5/head.h` versus `proto.h` trial put the cone prototype
after `fast_normals_interpolate` and before `normals_interpolate`. Root's one
replay uses that same position in the changed current header context. This is
a controlled replay, not an assertion that the position is original.
The first historical exact-function falsifiers, recorded by `h5/probe.sh`, are:

| Unit | Exact function |
| --- | --- |
| source/effects/decals | _decals_delete_permanent_from_cluster |
| source/rasterizer/xbox/rasterizer_xbox_draw_primitives | _rasterizer_dynamic_geometry_initialize |
| source/tool/error_geometry | _error_geometry_polygon |

The four historical park drifts were `_encounter_update_respawn`,
`__rasterizer_model_draw`, `_extract_plateless_cube_map`, and
`_bitmap_2d_uncompress_from_mipmap`. Their current admission status and inherited
fingerprints must be read from the current baseline, not copied from the old
park classifications. Include the real_math owner first; passing those early
checks is insufficient to admit a shared-header change. Root reports 272 actual
current Ninja consumer units, all requiring the final inherited-code/owner,
nondebug-data, COMMON and point-ownership checks if the first falsifiers pass.

The authorized replay is now complete. Independent read-only COFF comparison
of root's before/after objects confirms all 84 exact real_math controls and
every other emitted function there remain unchanged. Decals falls from 30 to
29 exact controls: only `_decals_delete_permanent_from_cluster` changes.
At +0x13d/+0x140 the January-exact baseline loads `esi` from `[ebp-4]` and then
`ebx` from `[ebp+8]`; adding the prototype reverses those loads. The latter is
the cluster-index parameter and the former is the loop-layer home immediately
before incrementing the layer. No source behavior or missing operation is
implicated by this transposition. No other emitted function changes in Decals.
Both tested units preserve code/named owner inventories, nondebug noncode,
COMMON and point ownership. See `decals-aligned.patch` and `review_final.py`.

Independently rehashed all 272 baseline consumer sources and production base
objects: each matches the frozen root baseline. `real_math.h` exactly matches
`cone-owner/real_math-before.h` and baseline SHA256
`274663e6c163ec3f561eccaa514d4187e92c1d0191b6a972bb94379d2c8d0637`.
No subsequent consumer probe or setter compile was performed. Root's receipt
is `../cone-owner/probe.json`; independent evidence is in
`preliminary-review.json`. This verifies restoration, not a full 272-consumer
candidate gate: the candidate test correctly stopped at the first loss.

Final recommendation: **HOLD the owner prerequisite and setter proposal.**
Reopen only for a separately authenticated coherent owner/source-context repair
that preserves inherited controls. Do not compensate with alternate declaration
positions, selective visibility, unrelated cleanup or speculative source shapes.
This independent review grants zero coverage credit.

## Evidence paths

- This folder: `inspect_primary.py`, `primary-receipt.json`,
  `setter-primary.asm`, `setter-only-versus-archive.patch`,
  `preliminary-review.json`, `review_final.py`, `decals-{before,after}.asm`,
  `decals-aligned.patch`.
- `../aim-assist/`: raw `target.asm`, `ghidra_00589db0.asm`, `primary.py/json`,
  `hcea_pin_normal_to_cone3d_{sym,lines}.txt`,
  `hcea_player_aim_projectile_{sym,lines}.txt`,
  `hcea_set_real_point3d_{sym,lines}.txt`, `typed-point-setter.c/json/patch`.
- `C:/halo-worktrees/opus5-30k-fresh-graphs-20260914/scratch/h5/`:
  `probe.sh`, `head.h`, `proto.h`; parent scratch `h1-snapshot.json`,
  `h5-snapshot.json`, `h5b-snapshot.json`; `workers/aim_assist.md` and
  `workers/aim_assist/pap_v9.txt`, `pap_v12.txt`.
- `C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/aim_assist/`
  `player_aim_projectile_candidate.c` (archived donor, not original-source proof).

The original preliminary recommendation allowed the single bounded prerequisite
test. The final disposition above supersedes it after the independently
verified negative result.

## Fixed cone plus projection composition addendum

Root subsequently authorized one fixed composition of the genuine cone
declaration with the January-authenticated three-return
`projection_from_vector3d` repair. The two changes have independent source
support and the same real owner; the cone ledger explicitly allowed reopening
with coherent whole-board real_math reconciliation. No location or spelling
was tuned. This was an admissible bounded test, not evidence that compiler
effects should cancel or permission to search combinations.

Read-only independent inspection reproduces its negative result. Units retains
all 189 exact controls and every emitted function; Decals again loses only
`_decals_delete_permanent_from_cluster`. That failed function is strict
fingerprint-identical to the cone-only failed candidate, so its difference is
the same two-load transposition above. Both units retain owner inventories,
nondebug noncode, COMMON and point ownership. Independent checks confirm all
272 production source/base-object hashes and the header restored again.

The fixed composition is **HOLD**, zero credit. The setter remains uncompiled.
No further combination or declaration-location rescue is authorized. Evidence:
`review_composition.py`, `composition-review.json`, `composition-header.patch`
and root's `../cone-projection/probe.json`.
