# Biped physics diagnostic block: independent source and object review

The final candidate is suitable as an authenticated source correction at **zero
exact credit**. Its source SHA-256 after newline normalization is
`e01e5db2eefb6e4f7e329826aa73bbeec4547d515a5e7ca2d71ccb93e3e48b27`.
It restores the omitted collision diagnostic without changing the safety guard
against `collisions[-1]`. No moving-body change is included. There is no new
strict function or whole-object admission.

Baseline is commit `1ec9171e757e8aab5038f26ec9797744da947c52`. Independent
script `scratch/astra-wave3/bipeds-independent-review.py` obtains the frozen C
file using `git show`, rebuilds it and the proposed candidate into separate
scratch objects, and uses the root-reviewed canonical `collision_debug.h`.
The header digest for this review is
`810847a427b51a13cc965955e312d9eb24eb072759f86871bf5425da64fa30a0`.
Only the root added the shared header declarations. This reviewer made no
production/header/configuration changes, ran no Ninja, and made no commit.

## Primary branch and operand evidence

`bipeds-independent-primary.py` and `.json` freshly read the January target
COFF and raw symbol executable. The PE digest is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`;
its source revision relative to January remains unverified. The raw spans
supplement the worker's actual Ghidra extraction without treating inferred
decompilation as original source.

January copies `physics.position` at `+0x792..+0x7a9` into the three local
components later used by the diagnostic. It copies `physics.new_velocity`,
adds `physics.crouch_velocity` to its k component at `+0x7b8..+0x7cf`, and
passes these input locals to collision movement. The skip-collision arm jumps
from `+0x88c` to `+0x8c3`; the collision-call arm also reaches `+0x8c3`.
Thus the diagnostic follows both branches and precedes the result-flag updates.

At `+0x8c3`, January tests the diagnostic flag. It calls
`object_get_and_verify_type(physics->biped_index, -1)` at `+0x8d1`, reads the
32-bit owner field at object offset `0x70`, and compares it with `NONE`.
Both false conditions bypass the entire store block. The final source uses
the existing canonical `object_get` macro in `objects.h`, which returns
`struct object_datum *` and supplies `_object_mask_all`. This is a typed
accessor and preserves the original mask. The temporary `biped_get` variant
was withdrawn; its narrower type mask is not part of the reviewed candidate.

The existing `object_datum` and `biped_datum` layouts share the definition-index
prefix and embedded `_object_datum`; the selected named field is
`object.owner_player_index`, not an animation, unit-controller, object-owner,
or clipped-position field. In the current caller, `biped_update_moving`, the
biped index has already been acquired through `biped_get`.

The January stores at `+0x8e1..+0x93b` select the original position and the
crouch-adjusted input velocity. They do not select the clipped outputs. There
are ten corresponding global-store relocations: three point components,
one repeat byte, three vector components, length, width, and height. Length
has exact bits `0x3f800000`; width and height are loaded from physics offsets
`0x58` and `0x54`. The candidate retains that distinction and uses `1.0f`.

Fresh raw /Od evidence confirms the same branch merge and source store order
at `0x8cae39..0x8caedb`: repeat, point, vector, length, width, height. It
performs the same all-object lookup and NONE test. Its owner offset is `0x7c`
in the later layout; this is not asserted to equal January's layout. RTC
descriptor `0x8cc4fc` has 39 protected records and identifies `position`
at -420 and `velocity` at -460, both 12 bytes, plus the 704-byte collision
array. Direct input initialization at `0x8cac14..0x8cac6e` supports the
diagnostic operands. These records do not supply complete January lexical
scopes or original scalar declarations.

## Initialized data and source safety

The input position, height, and width are filled by `biped_get_physics_pill`
before the caller invokes this function. Crouch velocity starts at zero and
is optionally updated from defined crouch values. Within physics update,
each flying, absolute-movement, airborne, and ground-motion arm initializes
all three new-velocity components before the local copy. The diagnostic
therefore does not read an uninitialized clipped result or an uninitialized
velocity component. `collision_move_pill` takes these position and velocity
inputs as const and has separate clipped-output pointers.

The new block adds no locals, helper definitions, forced-inline directives,
fake dependencies, volatile steering, or arithmetic filler. It writes existing
diagnostic globals through their genuine shared declarations. It does not
change collision indexing, and both existing `best_collision_index != NONE`
checks remain present. This review establishes no new unsafe read in the
added block; it does not claim to prove every inherited behavior in this
large fuzzy function. The fake scan reports one file and zero leads.

## Independent gate and complete ownership result

Both fresh builds retain **42 exact functions / nine residuals out of 51**.
Only `_biped_update_physics` changes among 82 candidate code owners; all other
81 code sections and their auxiliary records remain identical, including
the residual moving function. No code owner is added or removed. The 193
named definitions retain their names, offsets, types, storage classes, section
flags and ownership. All 111 runtime noncode sections retain normalized bytes,
relocations, symbols, flags and auxiliary records.

Four one-byte COMMON definitions remain identical: debug physics, collision
skip, update skip, and limp-body disable. The six new global references are
declarations, not new COMMON or data definitions. `_point_from_line3d` is
absent as a code owner in both objects; the emitted-symbol guard passes.
The inherited 31 target-absent helper owners are unchanged and enumerated in
`bipeds-independent-review.json`. This preservation is not a new whole-object
ownership admission.

| Physics section | Padded bytes | Relocations | Normalized SHA prefix |
|---|---:|---:|---|
| Frozen baseline | 5248 | 112 | fccc72d132c2 |
| Candidate | 5376 | 124 | bf945c9301d7 |
| January | 5376 | 124 | f202fb20dd4d |

The corrected candidate reaches January's extent and relocation count, but
its normalized code remains different. The 5,366 target meaningful bytes
remain at zero exact credit. Equal envelope and a recovered diagnostic block
do not justify a new strict function, park retirement, whole-object admission,
or a general compiler-context law.
