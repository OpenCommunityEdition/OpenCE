# `source/camera/bored_camera.obj` exact-match strategy ledger

## Plain-English outcome

The object is complete. All nine functions and all six pieces of read-only data
owned by the January 2002 object reproduce exactly with the configured XDK 3911
compiler.

Most of the object is a dormant camera mode which points the camera in a random
direction when a local player has been idle. The original code also contains a
real bug: it can set a timer to 10,000--30,000 seconds and then immediately
assert that the timer is no greater than 3,600 seconds. The reconstruction keeps
that behavior for historical accuracy and documents it beside the code.

The last two mismatches were both consequences of ordinary, readable source
structure. Keeping the random field-of-view result in a named local gives VC7
the same floating-point store schedule as January. Expanding the final camera
validation into an explicit failure block gives its diagnostic flags value a
natural block-local lifetime and reproduces January's `EAX` choice. No assembly,
volatile qualifiers, force-inlining, byte patches, undefined behavior, or
compiler-flag changes were used.

## Scope and verification baseline

- Translation unit: `source/camera/bored_camera.c`
- January target: `build/split/source/camera/bored_camera.obj`
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Starting campaign commit: `e2dafe97` (`Match errors object`)
- Starting state: 5/9 functions exact
- Missing functions at the start:
  - `_code_00073000`: `0x20`, 0 relocations
  - `_code_00073020`: `0x20`, 0 relocations
  - `_valid_real_vector3d_axes2`: `0x70`, 3 relocations
  - `_bored_camera_update`: `0x500`, 52 relocations

The target has no `.data` or `.bss`. It owns six `.rdata` sections: the camera
diagnostic, constants 3600/5000/-5000, the `camera` assertion string, and the
January source path.

## Current-campaign salvage audit

The earlier closeout commit was not admitted on the current campaign branch, so
it was treated as an untrusted source candidate rather than assumed correct.
Commit `cee59511` was transplanted onto authoritative campaign tip `a8114eb7`,
rebuilt independently with XDK 3911, and compared again against the pristine
January object. All nine functions and all six target-owned `.rdata` sections
still pass `section_infos_equal`, including relocation destinations and symbol
ownership.

The audit also replaced the opaque `byte data[0x1c]` camera-track placeholder
with the recovered, readable 0x1c-byte `unit_camera_track` layout: a
`tag_reference` followed by three reserved longs, guarded by a compile-time size
assertion. The referenced element remains intentionally unused because January
does the same incomplete lookup. This source-only cleanup leaves the entire
object byte-identical.

House/Berth audit: parameter declarations are one per line, no-argument lists
put `void` on its own line, void routines end in an explicit `return;`, and each
function has one return where practical. The reconstruction contains no inline
assembly, volatile/codegen barriers, force-inlining, undefined behavior, raw
field-offset access, byte patches, or compiler-flag changes.

## January-authoritative reconstruction

The private timer helpers are the compiler's branchless `MIN` forms:

- `MIN(boredom_count, 3) * 1000`
- `MIN(boredom_count + 1, 3) * 10000`

`valid_real_vector3d_axes2` validates both normalized axes and their dot
product. Its direct conjunction form is important: it gives both success and
failure paths full-width `EAX` results while preserving the byte-sized tests of
the two `boolean` callees.

`bored_camera_update`:

1. Updates its timer using January's backward-looking expression
   `last_update_milliseconds - now`.
2. Obtains the local player's aiming unit and camera information.
3. Preserves an apparently unfinished camera-track lookup.
4. Randomizes pitch, yaw, field of view, and depth using the local random seed.
5. Constructs the forward/up axes and initializes the camera command.
6. Preserves the original timer-validation bug and its exact diagnostic.

The recovered layouts used by this translation unit are:

- `bored_camera`: 12 bytes; last update at `+0`, timer at `+4`, count at `+8`.
- `camera_action`: local-player index at `+0`.
- `camera_command`: the proven 76-byte layout already shared by the camera
  subsystem.

## Experiment matrix

| ID | One changed source factor | Strict result | Disposition |
| --- | --- | --- | --- |
| A01 | Implement the two timer helpers as the measured `MIN` expressions and compile them in the same TU as their caller. | Both became strict exact immediately: `0x20/0` each with hashes `b87d9765...` and `a14be3e2...`. | Accepted. |
| A02 | Implement axes2 with a byte `boolean result = FALSE`, nested validation, and one final return, mirroring the header literally. | `0x80/3`; VC7 kept the byte result in another callee-saved register and emitted a larger epilogue. | Rejected. |
| A03 | Use early success/failure returns while retaining a byte return type. | `0x70/3`; topology matched, but success/failure materialized only `AL` (`mov al,1` / `xor al,al`) rather than full `EAX`. | Rejected. |
| A04 | Use a long local result and one return. | `0x80/3`; the long live range introduced an extra saved register. | Rejected. |
| A05 | Return the complete short-circuit conjunction directly. | **STRICT EXACT:** `0x70/3`, hash `7e597133...`; the compiler naturally emitted the measured full-width `EAX` exits. | Accepted. |
| U01 | First update reconstruction without a visible prototype for `real_local_random_range`. | `0x590/56`; C's implicit-int rule inserted four `__ftol2` calls. | Rejected. The correct float prototype was moved before the caller. |
| U02 | Correct prototype plus direct field-of-view and depth assignments. | `0x500/52`; every relocation matched, with two local differences: FOV store scheduling and diagnostic `EDX` versus `EAX`. | Near-exact baseline. |
| U03 | Store the random field of view through a named `real field_of_view` local before assigning the result field. | Preserved `0x500/52` and changed only the intended schedule to January's `add esp,0xc; push 6.f; fstp [result+0x20]`. | Accepted. |
| U04 | Move the field-of-view local among the four local declarations. | All measured orders produced byte-identical output. | Neutral; retained a readable declaration order. |
| U05 | Cache `result->flags` before the entire validation expression. | `0x510/52`; lengthened its lifetime and changed the validation prefix. | Rejected. |
| U06 | Use a private getter for the final diagnostic flags argument. | The update became exact, but VC7 also emitted an unwanted link-dead `0x10` helper section. | Rejected; proved the return-value lifetime mechanism. |
| U07 | Expand the assertion macro into its ordinary explicit failure block and bind `long flags = result->flags` inside that block. | **STRICT EXACT:** `0x500/52`, hash `6a8652ad...`; no extra section. | Accepted. |

## Final strict evidence

| Function | Size | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_bored_camera_new` | `0x20` | 1 | `6a7a02f7791a1f641ecf76b4a11005918da34197e743ec8954fc423d4a1340e5` |
| `_code_00073000` | `0x20` | 0 | `b87d9765691954cdbc77cfe2ff570c97e10b5bb7663a8d8b8c78649fc2f5e7b2` |
| `_code_00073020` | `0x20` | 0 | `a14be3e2cd3d931031ffc3ff16820469b1cdf19bb50f1ceb4bc2cf7e2ddf55a5` |
| `_is_bored` | `0x10` | 0 | `1c93847ee0f29c8102c08f41566abe2ae27a71ab314eec709e90e3a28353e543` |
| `_is_still_bored` | `0x10` | 0 | `1c93847ee0f29c8102c08f41566abe2ae27a71ab314eec709e90e3a28353e543` |
| `_real_local_random_range` | `0x20` | 2 | `7ed2e50e8e1a701a98fcceeb6a6b67e05d997f21bdb9ff5ee2e848fd1a8f409a` |
| `_valid_real_vector3d` | `0x60` | 0 | `4358678c294d7b527862c193054a291eb171373d7f41b1f130671cedfa1a66f9` |
| `_valid_real_vector3d_axes2` | `0x70` | 3 | `7e597133c38681bf0d80770aa4e4f59a38ab71d87035accf6ff338c0b2f49eff` |
| `_bored_camera_update` | `0x500` | 52 | `6a8652addccf7e892bc84cd16cc112949a5851bf082b76f23c9de5ffd1a8f3ab` |

All six target-owned `.rdata` sections match in size, contents, relocation
shape, and normalized hash. The consolidated `halobetacache_build` completes
successfully.

### Current-tip admission gates

The completed object was re-admitted from campaign tip `a8114eb7`; no result
from the older branch was trusted without rebuilding it. After regenerating
the isolated lane's build description, the following final gates passed:

- `ninja halobetacache_build`: exit 0 for the complete 466-object build (the
  final confirmation reported the tree already up to date).
- `ninja progress`: exit 0; 379 units scanned, 3,274 functions evaluated,
  3,184 accepted semantic-exact functions, and zero unit errors. Campaign
  progress is 255/468 Halo objects, 3,179/7,574 Halo functions, and
  355,974/1,770,166 Halo code bytes.
- `python -m tools.regression_gate snapshot --unit
  source/camera/bored_camera` followed by `check`: all nine functions are
  `still_exact`; there are no failures, changed non-exact siblings, newly
  exact warnings, or ownership warnings.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 173 tests passed.

The unit owns no `.data` or `.bss`. Its six target-owned `.rdata` sections were
also rechecked independently with `section_infos_equal`; all six retain their
exact payloads, relocation identities/addends, storage classes, and section
ownership. No parked credit, fuzzy percentage, or semantic exception is used
for this object.

## General lessons

1. In C89, a missing float-return prototype silently becomes an implicit-int
   declaration. An unexpected cluster of `__ftol2` calls can therefore be a
   prototype defect rather than an arithmetic defect.
2. A direct boolean conjunction can produce a full-width return where a named
   byte result produces only `AL`; inspect both the expression topology and the
   declared return width before chasing register allocation.
3. Named locals are not automatically harmful. A short, semantically meaningful
   floating-point local can preserve the value across stack cleanup and expose
   exactly the scheduling window used by the original compiler.
4. When a diagnostic-only value needs a different register lifetime, give it a
   natural scope inside the failure block. Do not use identity expressions,
   casts, or artificial volatile barriers to force a register.
