# `source/units/vehicles.obj` — current integration checkpoint (2026-08-27)

**Current source result: 33 of 39 functions byte-exact; all 39 written.**
**Exact target text: 9,008 / 17,232 bytes (52.3%).**
**Five of the six residuals are size- and relocation-exact.**
**Object status remains `NonMatching`.**

This checkpoint supersedes the status line in the historical Claude ledger below,
while retaining that ledger verbatim for provenance and source-shape research.

## Integration provenance

The following worktrees and lineages were audited read-only before integration:

- `work/libcmt-stream`, especially `5a0a12c4` (34/39 exact) through
  `65f8c9d2` (the committed 704-byte / 51-relocation `vehicle_update`
  frontier);
- `C:\halo-worktrees\claude-near-complete-objects-20260816`, the clean
  34/39 checkpoint;
- `work/campaign-integration`, Claude's active, uncommitted vehicles work.
  Its files were never edited, built, cleaned, staged, or otherwise mutated.
  The live source was used only as a read-only semantic reference.

The committed `65f8c9d2` source was integrated into
`units-integration-20260824/source/units/vehicles.c`. The active Claude
`vehicle_update` reconstruction was then ported into this integration tree,
with its object access converted to `vehicle_runtime_get`, its vehicle tag
access converted to `vehicle_specific_definition_get`, and its animation graph
access converted to `animation_graph_definition_get`.

## Measured result

The strict whole-unit scratch gate reports:

| target owner | target size | relocation result | status |
|---|---:|---:|---|
| `_code_001a5e50` | 1,088 | 32 / 32 | residual bytes |
| `_code_001a6290` | 768 | 22 / 22 | candidate is 752 bytes |
| `_code_001a6910` | 656 | 22 / 22 | residual bytes |
| `_code_001a7ac0` | 928 | 34 / 34 | residual bytes |
| `_code_001a7e60` | 2,464 | 91 / 91 | residual bytes |
| `_vehicle_update` | 2,320 | 98 / 98 | residual bytes |

Everything else is strict-exact: 33 exact, 6 residual, 0 unwritten.
`vehicle_update` advanced from the committed 704-byte / 51-relocation
scaffold to exact target size and relocation count. Its normalized body still
differs in 2,005 bytes, and its relocation address/destination sequences are not
yet exact, so it receives no exact credit.

The newer `code_001a7e60` source was also adopted. A lawful pair of explicit
`real_vector2d` temporaries replaces the historical `.ij` union view and
retains the target's exact 2,464-byte size and 91-relocation count.

## No-punning decision for `code_001a6290`

The historical 34th exact owner depends on assigning
`vehicle->object.translational_velocity.ij` through a union overlay added to
`real_vector3d`. That overlay was not restored because the current campaign
rules prohibit union-based type punning.

Two lawful alternatives were measured:

- explicit `i` / `j` assignments: 752 bytes, 22 relocations;
- `memcpy` of the two components: 768 bytes, but 23 relocations because VC7
  emits an unwanted `_csmemcpy` call.

The `memcpy` experiment was rejected. The tree retains the clear 752-byte
source rather than violating the no-punning rule or accepting a false
size-only improvement.

## Admission evidence

- one production target compile:
  `ninja build\\base\\source\\units\\vehicles.obj` — passed;
- strict scratch whole-unit census:
  `33 exact, 6 residual, 0 unwritten`;
- exact text:
  `9,008 / 17,232 bytes`;
- prior-branch named-function comparison:
  28 newly exact, no named exact owner lost;
- `git diff --check` — passed;
- raw `tag_get` and raw/casted `vehicle_get` uses in
  `source/units/vehicles.c` — zero.

The production compile retains pre-existing point/vector compatibility warnings
in `vehicle_stuck` and `code_001a5e50`. Rewriting those expressions
semantically reduced `vehicle_stuck` from 368 to 352 bytes and removed one
`vehicle_update` relocation, so that experiment was rejected and the exact
owners/topology were restored.

## Remaining frontier

- `code_001a5e50`: target size/topology exact; residual is a local-slot
  permutation.
- `code_001a6290`: 16 bytes short under the lawful component-copy form.
- `code_001a6910`: target size/topology exact; two independent stores remain
  ordered differently.
- `code_001a7ac0`: target size/topology exact; one compiler spill/temp slot
  remains.
- `code_001a7e60`: target size/topology exact; normalized body remains
  residual.
- `vehicle_update`: full semantic reconstruction, exact size and relocation
  count; instruction scheduling, branch layout, and relocation placement remain
  the high-leverage closeout.

---

# Historical Claude/Jonas research ledger

# `source/units/vehicles.obj` — partial, in progress

**Status: 25 of 39 functions byte-exact. Five more are size- and relocation-exact.**
**The `vehicle_update` cluster blocker is solved — see §6.**
**Lane:** `claude/libcmt-stream` · **Object status in `config/config.json`:** `NonMatching` (unchanged)

**Toolchain:** XDK 3911 `CL.Exe` 13.00.9254.1 · **Flags:** the game set, `/O2 /Oy- /DDEBUG /Dxbox`

| | start | now |
|---|--:|--:|
| functions exact | 5 / 39 | **25 / 39** |
| `.text` bytes exact | 80 / 17232 (0.5%) | **3696 / 17232 (21.4%)** |
| sections exact | 5 / 68 | **25 / 68** |

This is a large object — 17.2 KB of code across 39 functions, plus a 1528-byte
`.data` table — and it is **not** finished. Read §6 first: the remaining work is
**one dependency cluster**, not 24 independent functions.

## 1. Exact (25)

`_vehicles_initialize`, `_vehicles_initialize_for_new_map`,
`_vehicles_dispose_from_old_map`, `_vehicles_dispose` (all empty),
`_vehicle_place`, `_vehicle_delete`, `_vehicle_causes_collision_damage`,
`_vehicle_build_update`, `_vehicle_parse_update`, `_vehicle_hover`,
`_vehicle_is_flipped`, `_vehicle_new`, `_vehicle_render_debug`,
`_vehicle_stuck`, `_code_001a4f50`, `_code_001a4fa0`, `_code_001a5810`,
`_code_001a6ba0`, `_code_001a70e0`, `_vehicle_find_pathfinding_surface_index`,
`_vehicle_accelerate`, `_vehicle_reset`, `_code_001a58e0`, `_code_001a5a30`,
and the `_set_real_quaternion` COMDAT.

## 2. Structure layout recovered

Section order is source order again (function-level COMDATs), which fixes the file
layout. Every offset below was confirmed by compiling an `offsetof` probe and
reading its `.data` — none of it is guesswork:

| probe | value |
|---|--:|
| `sizeof(struct _object_datum)` | `0x1a0` |
| `sizeof(struct _unit_datum)` | `0x280` |
| `sizeof(struct unit_datum)` | `0x424` |
| `sizeof(struct unit_definition)` | `0x2f0` |

So in `struct vehicle_datum { long definition_index; _object_datum object;
_unit_datum unit; _vehicle_datum vehicle; }` the vehicle-specific part begins at
exactly **`0x424`**, which is why every vehicle field in the disassembly is
`[reg+0x424…]`. `players.c` models only the first 8 bytes of it; this object
reaches `+0x478`, and `vehicle_reset` writes the whole thing, which hands over the
layout directly:

```
+0x424 word flags          +0x42c..+0x448  reals (see below)
+0x426 word                +0x44c byte[8]  suspension shift, one byte per entry
+0x428 byte  (saturating counter)
+0x429 byte                +0x454 real_point3d  hover position
+0x42a byte                +0x460 real_quaternion
+0x42b byte  (saturating counter)
+0x470..+0x478  three longs
```

`sizeof(unit_definition)` being exactly `0x2f0` proves that the
`[definition+0x2f0]` read in `vehicle_causes_collision_damage` is the **first
vehicle-specific definition field** — the vehicle flags word — so
`struct vehicle_definition { struct unit_definition unit; unsigned long flags;
short vehicle_type; }` — `vehicle_type` at `0x2f4` is read by
`vehicle_find_pathfinding_surface_index`.

Other confirmations:

* `[vehicle+0x38]` is `object.up.k` — `vehicle_is_flipped` is
  `return vehicle->object.up.k<0.2f;` (`__real@3e4ccccd`).
* `[definition+0x8c]` is `object.physics.index` (the `tag_reference physics` sits
  at `0x80` in `_object_definition`), and `[definition+4]` is
  `object.bounding_radius`. Both are used by `vehicle_new`.
* `[phys+0x74]` is `physics_definition.mass_points.count`, and `[phys+0]` is
  `radius`. The `physics_definition` in `physics_definitions.h` was already
  correct.

## 3. Findings so far

1. **`TEST_FLAG` is right for the definition flags.** `vehicle_causes_collision_damage`
   emits `shr eax,7 / and eax,1`, which looks like a bitfield read but is exactly
   what `TEST_FLAG(definition->flags, 7)` produces here.
2. **`vehicle_reset`'s tail is `set_real_quaternion`.** The object owns a
   `_set_real_quaternion` COMDAT, which only exists if something calls that
   `real_math.h` `__inline`. It is the `+0x460` group. Making that call moved the
   deferred `add esp,0x14` five instructions later, exactly as the target has it —
   though not yet all the way (see below).
3. **`vehicle_accelerate` uses `add_vectors3d`, not three `+=`.** Three separate
   `+=` statements emit the `.j` component with its `fadd` operands reversed;
   `add_vectors3d(&v, acceleration, &v)` reproduces all three.
4. **`vehicle_render_debug` really does contain an empty loop.** It walks
   `physics->mass_points.count` with a `short` index and no body — MSVC keeps it.
   The debug global `debug_objects_vehicle_powered_mass_points` has no header, so
   it is declared locally.
5. **The `_code_*` statics take register arguments.** `code_001a8800` receives
   three parameters in `ESI`, `EAX` and `EBX` — `EBX` is pushed as an *argument*
   and never restored, which is how you can tell. See §6: this turned out not to
   block reconstruction at all.
6. **`vehicle_find_pathfinding_surface_index` switches on the vehicle type.** The
   jump table at `+0x120` has two destinations and the byte index table at `+0x128`
   is `[0,0,1,1,0,1,0]`, so types 0, 1, 4 and 6 do a downward BSP raycast and types
   2, 3 and 5 fall to the default and return `NONE`. That is exactly
   tank/jeep/scout/turret versus boat/plane/fighter — the vehicles with no
   pathfinding surface are the ones that float or fly, which confirms both the
   table reading and the standard vehicle-type enum ordering.
7. **The `0x434` frame is `collision_bsp_test_vector_result` (1048 B) plus `0x1c`
   exactly**, which pins that call's argument list with no guesswork:
   `collision_bsp_test_vector(1, bsp, 0, NULL, &origin, &vector, FLT_MAX, &result)`.

## 4. The residuals

All but one are size-exact and relocation-exact, differing only in instruction
selection or scheduling; `_vehicle_moving_near_any_player` is the sole function
where our output is *larger* than January's.

### Adjacent field pairs are typed vectors, not loose scalars — SOLVED

This one cracked two functions at once and is the most reusable finding in the
object.

`vehicle_reset` sat two instructions from exact for the whole session: January
places the deferred `add esp,0x14` between the `+0x474` and `+0x478` stores, we
placed it two stores earlier. Every reordering of the assignments failed. The
actual cause was the *type* of the fields:

```c
long unknown470;  long unknown474;      /* two loose scalars  -> add esp too early */
real_point2d unknown470;                /* one 2D field       -> exact */
    set_real_point2d(&vehicle->vehicle.unknown470, 0.0f, 0.0f);
```

The same reasoning had already fixed the `+0x460` quaternion group earlier
(`set_real_quaternion`), which is what should have made the pattern obvious.

Applying it to buffer B's powered-mass-point state — where `+0x1c`/`+0x20` and
`+0x24`/`+0x28` are adjacent real pairs, and `+0x24`/`+0x28` is a **sine/cosine
rotation** — made `code_001a58e0` byte-exact:

```c
real_vector2d unknown1c;    /* +0x1c, +0x20 */
real_vector2d unknown24;    /* +0x24, +0x28  = (sin, cos) */
    set_real_vector2d(&state[n].unknown24, 0.0f, 1.0f);
```

`code_001a5a30` then fell to the same lens plus one ordering fix: with the pair
typed, binding **`cosine` before `sine`** finishes it —

```c
real cs = cosine(angle);
real sn = sine(angle);
set_real_vector2d(&state[0].unknown24,  sn, cs);
set_real_vector2d(&state[1].unknown24, -sn, cs);
```

I had tried that exact binding order earlier in the session and recorded it as a
failure. It was not — my sweep harness was replacing the anchor text in a
*different* function that shares the same `angle` expression, so the variant never
actually reached `code_001a5a30`. Worth remembering: when a sweep says a plausible
hypothesis failed, check that the edit landed where you think it did.

### `_vehicle_preprocess_node_orientations` — 1024 B / 42 rel, structurally identical

Written from scratch. Size- and relocation-exact, and with registers and
displacements normalised away the instruction sequence is **identical to
January's — diff = 0**. The only difference is which callee-saved registers MSVC
chose (`esi`/`edi`/`ebx` permuted), worth 7 raw instruction differences. All six
declaration-order permutations of the three pointers were tried; none moves it.

It applies six overlay animations from `vehicle_animations[0]`, indexed through a
`short` array, then walks the suspensions:

| index | driver |
|---|---|
| 0 | `aiming_screen_apply` with `+0x434` |
| 1 | `triple_product3d(up, forward, velocity)/+0x2f8`, biased and halved |
| 2 | `+0x42c` signed, divided by `+0x2fc` or `+0x2f8` |
| 3 | `dot_product3d(velocity, forward)`, pinned, over `fabs(+0x2f8)` |
| 4 | element fetched, result discarded |
| 5 | `+0x438 / +0x310` when `+0x310 > 0` |
| loop | `+0x44c[i]` byte, `0xff` meaning `1.0f` |

Two structure corrections came out of it: `vehicle_animation` has a **second**
`tag_block` at `+0x5c` (the animation indices) ahead of the suspensions at
`+0x68`, and `vehicle_definition +0x2fc` is a `real`.

Three source-shape points, each worth a rebuild:

* **`PIN` around a call forces the call out of line.** `PIN(dot_product3d(...),
  0, 1)` duplicates the argument and MSVC then emits a real `call
  _dot_product3d`, costing a relocation January does not have. Binding it first
  helps; writing the dot product out by hand is what finally inlines it. Note
  January *does* call `triple_product3d` out of line in the same function, so the
  inline decision is per-callee, not global.
* **The suspension loop index is a `short`.** As a `long` the function is 32 bytes
  short; `movsx edi, ax` at the loop bottom is the tell.

### `code_001a8800` — body exact, ABI blocked on `code_001a6290`

Written, and every instruction between the prologue and the epilogue matches
January byte for byte: the `vehicle_get` / `vehicle_definition_get` /
`physics_definition_get` chain, the `physics->radius>0.0f` test, the hoisted
shared `push` of the last argument, and both call arms. Size and relocation count
land exactly (112 B / 8 rel). The only difference is the calling convention:
January passes all three parameters in registers and needs no frame, mine spills
`state` and therefore grows an `ebp` frame.

What does *not* move it:

- **Parameter order.** All six permutations of the three parameters, sweeping the
  declaration and its call site together: identical output every time.
- **The call site.** Position within `vehicle_update`, calling it twice, and even
  reducing `vehicle_update` to nothing but this one call: identical every time.
  So the convention is decided from the callee, not from what is live at the
  caller — which is worth stating plainly because an earlier note in this log
  claimed the same thing on much weaker evidence.

What does move it: **whether `state` is used on both arms of the branch.** Make
the `code_001a6290` arm take `state` as well and MSVC promotes it to a register
and drops the frame — January's shape class. So January's `code_001a6290`
receives `state` too. Its January call site cleans only two stack slots, so
`state` must arrive in a *register*, and the file already has a house convention
for exactly that: `code_001a58e0` and `code_001a5a30` both take
`(vehicle_index, mass_points, state)` with `state` in `EDI`, and both are
byte-exact. That pins a signature for a function not yet written:

```c
static void code_001a6290(
	long vehicle_index,
	struct vehicle_mass_point_state *mass_points,
	struct vehicle_powered_mass_point_state *state);   /* EDI = state */
```

Defining `code_001a6290` as a static with that signature does make
`code_001a8800` frameless and all-register, but a stub body does not reproduce
January's exact `ESI`/`EAX`/`EBX` assignment — that depends on which registers
the real 768-byte body ends up needing. **`code_001a8800` is therefore genuinely
blocked on `code_001a6290`**, and it is left in the tree written but
`NonMatching`. Nothing is credited for it.

`code_001a5e50` and `code_001a6290` are declared rather than defined for now.
January passes `code_001a5e50` all three arguments on the stack, which is what a
plain declaration gives; a declared-but-undefined *static* would instead invite
MSVC to invent a convention. The relocation carries the same symbol name either
way, which is what the comparator checks.

### A file-local helper inlines differently from a header `__inline`

`vehicle_preprocess_node_orientations` is now **byte-exact**, and the fix is a
genuine source-shape recovery rather than a codegen trick.

The dot product was written out longhand, which left a 16-instruction
register-allocation difference. Replacing it with the existing
`dot_product3d` from `real_math.h` makes it **much worse** — raw 218. Replacing
it with a *file-local* static helper of an identical body makes it **exact**:

```c
static real vehicle_dot_product3d(
	real_vector3d const *a,
	real_vector3d const *b)
{
	return a->i*b->i+a->j*b->j+a->k*b->k;
}
```

| form | raw diff |
|---|--:|
| longhand | 16 |
| `dot_product3d` (header `__inline`) | 218 |
| file-local `static` helper | **0** |

That is the finding worth keeping: **a `static` helper in the translation unit and
an `__inline` in a header do not produce the same code even with identical
bodies.** MSVC inlines them at different points, and the resulting register
allocation differs. So when a longhand expression is close but the header inline
is far, a local helper is the third option to try — it is not equivalent to
either.

Object: 26/39 -> **27/39**, 3808 -> **4832 bytes (28.0%)**, sections 44 -> 45.

### A `result` parameter in EAX means the function returns it

Cracked in `matrix_math.obj` and worth recording here because it is the first
register-allocation tie in this project to fall to a source change.

`matrix3x3_multiply` differed from January in **register allocation only** —
118 instructions against 118, with `a` in EAX and `result` in EDX where January
has them swapped. The cause was the signature: it returns `result` rather than
`void`, which pins `result` to EAX. Changing only the return type took the raw
diff **86 -> 0**.

It was found by calibration. The matched `matrix3x3_transform_vector` in the same
file has the identical three-pointer-plus-aliasing-guard shape, also keeps
`result` in EAX, and its declared signature returns `result`.

**Checked against this object: it does not apply.** None of `code_001a6290`,
`code_001a7ac0`, `code_001a5e50`, `code_001a6710`, `code_001a6910` or
`code_001a5b40` writes EAX before its epilogue — every one is genuinely `void`,
so their residuals are not this.

### On the `volatile` cast suggestion for `code_001a6590`

An outside suggestion proposed
`position.x = (*(volatile real *)&velocity->i)+vehicle->object.position.x;`
to force the operand order. **The diagnosis is right and the remedy is not
usable.** The difference is real — January emits `fld [esi] / fadd [edi+0xc]`
where ours emits the reverse — but `volatile` is named in this project's standing
constraints alongside asm and UB, because it coerces codegen by changing what the
program means rather than recovering what the source said.

The legitimate test was run: swapping the addends to
`vehicle->object.position.x+velocity->i` produces **byte-identical output**. So
unlike multiplication, addition operand order is not source-controlled here, and
the remaining six instructions in `code_001a6590` are a genuine tie.

### Why the corpus cannot calibrate the remaining ties — measured

Both remaining residuals were put to the corpus scan. Both came back empty, and
the reason turns out to be structural rather than bad luck.

**`code_001a7ac0`** — the `quaternion_to_angle_and_vector` / `scale_vector3d`
idiom. **Zero** of the 255 matching objects call
`quaternion_to_angle_and_vector` or `matrix4x3_rotation_to_quaternion` at all.

**`code_001a6290`** — January caches `translational_velocity.j`/`.i` with integer
moves into `[ebp-4]`/`[ebp-8]` while x87 code reads the same addresses. A detector
for "integer load of an address an adjacent x87 op also reads, then spilled to an
`ebp` slot" finds **zero** instances across all 255 objects. The detector was
self-tested first and correctly finds **both** occurrences in January's own
`code_001a6290`, so the zero is real and not a broken scan.

The measurement that explains it:

| | x87 instructions |
|---|--:|
| all 255 matching objects, combined | 3,128 |
| **`vehicles.obj` alone** | **2,001** |

Only 43 of 255 matched objects contain any x87 code at all, and only 7 contain 100
or more. The verified corpus is overwhelmingly integer, tag and string code —
`vehicles.obj` by itself carries about **64% as much floating-point work as
everything matched so far combined**. There is no body of verified heavy-FP
codegen to calibrate against because this object is the one pioneering it.

The whole FP calibration set that exists today, worth knowing for any future
float-heavy object:

```
1083  source/objects/objects.c
 295  source/scenario/scenario.c
 165  source/rasterizer/rasterizer.c
 128  source/camera/flying_camera.c
 121  source/physics/physics_variables.c
 117  source/camera/orbiting_camera.c
 105  source/objects/object_types.c
```

`physics_variables.c` is the interesting one: it is matched, it is float-heavy,
and it defines the very `physics_variable_speed_update_seek` this object calls.
It is the natural place to look first for any future vehicle-side codegen
question.

So the honest verdict on the three ties: not crackable by calibration today, for
a measurable reason. What would change that is matching another float-heavy
object — the residual shapes here need ground truth that does not yet exist.

### Calibrating against the project's 255 byte-exact objects

A method worth reusing. When a residual is a codegen tie with no local ground
truth, the project itself is a corpus: 255 objects are already `Matching`, so
their source in the repo provably produces January's bytes. Scanning those
objects' `.text` for a specific instruction pattern finds verified examples of
what source shape generates it.

Applied to `code_001a6dc0`'s single transposition — an immediate `push` sitting
between two x87 ops in a call setup:

- **Zero** of this object's 26 byte-exact functions contain the pattern, so there
  is no local calibration at all. That alone explains why a dozen source
  perturbations moved nothing.
- Scanning all 255 matching objects found **exactly one** instance:
  `object_force_inside_bsp` in `source/objects/objects.c`.

Its verified source is a call to `collision_test_line`, an `__inline` wrapper in
`collisions.h` that builds a vector with `vector_from_points3d` and forwards to
`collision_test_vector`. The emitted code pushes `&vector` *before* the vector is
filled, and the flags immediate lands between the `fld` and `fsub` of the inlined
body. So the pattern arises when the floating-point work belongs to an **inlined
helper inside the call expression**, not to a preceding statement.

That does not transfer to `code_001a6dc0`: its vector comes from
`scale_vector3d` (multiplies) rather than `vector_from_points3d` (subtracts), so
it cannot be `collision_test_line`, and nesting the helpers into the call
measures worse — raw 89 against 4. The tie stands.

Ruled out this round, each with a run rather than an argument:

| hypothesis | result |
|---|---|
| wrong compiler build | CL is exactly **13.00.9254.1** |
| storage class (`static` vs extern) | every `code_001a*` is EXTERNAL in January and STATIC here, yet 26 match — csplit cannot recover linkage, so it is an artifact and is not compared |
| statement order among independent computations | 7 permutations, all neutral or worse |
| all 41 compiler flags | all neutral or worse than baseline |
| declaration order | byte-identical output |

The residuals in `code_001a6290`, `code_001a6dc0` and `code_001a7ac0` are MSVC
scheduling and spill decisions with no reachable source lever found. The corpus
scan is the tool that would crack them, given a pattern that occurs in a matched
object — that is the next thing to try for the other two residual shapes.

### `code_001a7e60` — aerodynamic core, 1152/46 against 2464/91

The last unwritten function in the object, and the hardest. Written as a skeleton;
the aerodynamic core is deliberately not attempted yet.

Decoded and written: the tag chain; the entry gate, which is
`scenario_location_water_depth(&location, &position)<0.5f && up.k>-0.2f`; the
powered mass point initialisation loop, which is the same
`unknown18`/`unknown1c`/`unknown24` block as every other vehicle function; and the
tail, which counts mass points whose `+0x20` is not `NONE`, counts how many of the
corresponding `vehicle_mass_point_state` have `FLAG(4)` set, forms the grounded
fraction, and slews `vehicle->vehicle.unknown444` toward it by at most `0.1f` per
tick.

Not written: everything between those, roughly 0x1ab to 0x855. It is dense
aerofoil arithmetic — two `normalize2d` calls on 2D cross products of forward and
velocity, a `limit3d` clamp, `matrix4x3_transform_vector` and its inverse against
a body frame built by `matrix4x3_from_point_and_vectors`, a three-way sign term
computed with `fimul` on an integer -1/0/+1, and comparisons against double
constants `3f7c987120000000`, `3f1a36e2e0000000` and `3fe921fb60000000` — the
last being `(float)(pi/4)` widened. Twenty `__real@` constants belong to that
region and are the exact list of what is still missing.

`physics_mass_point_definition` gains a `short` at `+0x20`, the field the tail
tests against `NONE`.

**Every function in `vehicles.obj` now has a reconstruction.**
**The aerodynamic core**, written from the disassembly. `edi` is
`&vehicle->object.up` and `ebx` is `&vehicle->object.forward` throughout, which
unlocks the whole block. Gated on `vehicle->vehicle.unknown444<1.0f`, it takes
`cross_product3d(&up, &forward, &perpendicular)`, projects `forward` and
`perpendicular` into 2D, normalises both with `normalize2d`, and then branches on
`up.k>0.0f` into a full lift/drag path or a bare throttle path. Coefficients
recovered: `15.0f` on the angular term, the `PIN(x+1.0f, 0.3f, 2.5f)` clamp,
`0.00155f` on throttle and `0.00388f` on the attitude term.

A repeated idiom decodes as an explicit **signum**: MSVC emits
`fld x / fmul y / fcom 0 / test ah,0x44 / jnp` then a second compare, storing
`-1`, `0` or `1` to a stack slot and applying it with `fimul`. That is

```c
product = x*y;
direction = product==0.0f ? 0 : (product<0.0f ? -1 : 1);
result = (real)fabs(x)*direction;
```

There is no `SIGN` macro in `cseries.h`. Writing one produced an **undefined
external `_SIGN`** — MSVC accepts an implicit function declaration in C and
compiles it happily, so the build succeeded and the object was silently wrong.
Only the relocation list caught it. Worth checking the reference list, not just
the exit code, after introducing any new identifier.

Still not written: the body-frame block — `matrix4x3_from_point_and_vectors`,
`matrix4x3_transform_vector` and its inverse, `limit3d`, `normalize3d` — which
the call-sequence sweep now names exactly.


### `vehicle_update` — 2336/96 against 2320/98

Written end to end, from a 256-byte stub. Instruction counts are 689 against 686.
It is **not** exact — the auto-righting block is a best effort and the raw diff is
still large — but every call, every field and every structural decision below is
read off January rather than guessed.

Recovered while writing it:

- **`_data_00316228` is a `struct profile_section`**, byte-exact now. `profile.h`
  declares it as 0x5f8 = the section's 1528 bytes, `active` at offset 8 is what
  the prologue tests, and the `"vehicle_update"` string COMDAT is its `name`.
  Non-zero bytes gave the initialiser outright: `{ "vehicle_update", NONE, TRUE }`.
- **`structure_bsp.vehicle_floor` / `vehicle_ceiling`** at `+0x10`/`+0x14` — the
  water-plane clamp, real names from `structure_bsp_definitions.h`.
- `game_globals_falling_damage` gains `+0x38` (the damage passed to
  `damage_data_new`) and `+0x8c` (the velocity threshold).
- `vehicle_definition` gains the flip limits at `+0x340`/`+0x344`.
- The seven-way `vehicle_type` jump table at `+0x8f0`.

Two codegen lessons:

**All seven switch cases are needed for a jump table.** With three written MSVC
emitted a comparison ladder and reordered them — which the call-sequence sweep
caught immediately.

**MSVC reassociates and folds adjacent constant multiplies.** Writing
`turn_rate*(_pi/180)*(1.0f/30)` folded the two constants into `__real@3a18825b`
where January keeps `fmul 3c8efa35` then `fmul 3d088889`. Binding the first
product to a local restores both.

Open: January loads the auto-righting +/-0.3 from `__real@3fd3333333333333` and
`bfd3333333333333`; ours materialises the same doubles with immediate stores and
`fld qword [ebp-N]`. A `double` variable, a ternary and an if/else all produce the
inline form.

### The speed / turn / slide blocks are `physics_variable_*`, from a first-party header

`_data_00316228` is a **`struct profile_section`**. `source/cseries/profile.h`
declares it as 0x5f8 bytes — exactly the section's 1528 — with `active` at offset
8, which is what the prologue tests (`_data_00316228+0x8`). The `profile_enter`
macro expands to precisely the emitted sequence, and the string COMDAT
`"vehicle_update"` is its `name`. Dumping the non-zero bytes gave the initialiser
directly: offset 4 is `0xffffffff` and offset 8 is `1`, so

```c
static struct profile_section data_00316228 = { "vehicle_update", NONE, TRUE };
```

That section is now **byte-exact**, and the string with it. Sections 38 -> 42.

The `vehicle_type` jump table at `+0x8f0` gives all seven dispatch targets:

| type | handler |
|---|---|
| human_tank | `code_001a58e0` |
| human_jeep | `code_001a5a30` |
| human_boat | `code_001a5b40` |
| human_plane | `code_001a7ac0` |
| alien_scout | `code_001a7e60(vehicle_index, angle, state, mass_points)` |
| alien_fighter | `code_001a8800` |
| turret | `physics_update(vehicle_index, NULL, mass_points, NULL, NULL)` |

All seven cases are needed for MSVC to emit a table rather than a comparison
chain — with only three written it produced an if-else ladder and reordered them.

Also named from the `physics_variable_*_update_seek` signatures, which take
`real *speed` and `real *position`: `_vehicle_datum`'s `unknown42c`, `unknown430`
and `unknown434` are **`speed`**, **`slide`** and **`turn`**. 45 use sites.

**Open anomaly: `code_001a4f50` has no caller anywhere in January's object** —
an exhaustive scan of every section's relocations finds zero references — yet it
is emitted with a register-argument convention, which requires a known call site.
A static with no caller would have been discarded. Making it non-static gives it
an `ebp` frame and stack arguments, which does not match. For now a clearly
marked scaffold call keeps it emitted with the convention that does match; the
contradiction is unresolved.

### The speed / turn / slide blocks are `physics_variable_*`, from a first-party header

`vehicle_update`'s call list contains `physics_variable_speed_update_seek` x4 and
`physics_variable_position_update_seek` x1, and `source/physics/physics_variables.h`
declares both, along with:

```c
struct physics_variable_speed_parameters
{
	real positive_scale;
	real negative_scale;
	real acceleration;
	real deceleration;
};
```

Sixteen bytes, mapping exactly onto `0x2f8`/`0x2fc`/`0x300`/`0x304` — the block
halocea's header calls `physics_variable_speed speed`. And
`physics_variable_position_update_seek(real *position, real *limits, ...)` makes
the `turn` block at `0x308` a **two-real limits array**, not two loose fields.

This is confirmed rather than inferred, three ways over: the sign test in
`code_001a5e50` picks `positive_scale` for forward and `negative_scale` for
reverse; `vehicle_export_function_values` normalises against
`MAX(fabs(positive), fabs(negative))`; and `code_001a5810`'s two float arguments —
which the reconstruction already passed as `unknown300` and `unknown304` — are
`acceleration` and `deceleration`, matching that helper's `maximum`/`minimum`
parameters.

Applied: `speed`, `turn_limits[2]`, `slide` typed; `turn_rate`, `blur_speed` and
`fixed_gun_pitch` named from the corroborated halocea offsets. 27 use sites,
byte-neutral — 26/39, 3808 bytes and 41/68 sections identical before and after.
`vehicle_definition` now has essentially no unnamed fields left in the recovered
range.

### Operand-order fixes do not transfer between functions

`code_001a7ac0` contains all three of the expressions whose operand order had to
be flipped in `code_001a6290` — the `-(facing.i*facing.k)` projection, the
`dot_product3d` argument order, and the yaw cross term. Applying the same three
flips there gives:

| variant | raw diff |
|---|--:|
| baseline | 335 |
| `-(facing.k*facing.i)` | **333** |
| + `dot_product3d` swapped | 335 |
| + yaw cross term swapped | 335 |

Only the first transfers. The other two make it worse and were reverted. So the
float operand-selection rule is **contextual, not a property of the expression**:
the same source spelling of the same helper over the same fields resolves
differently depending on what surrounds it. Do not batch-apply these; sweep them
one at a time and keep only what measures better.

Neither the constant sweep nor the call-sequence sweep can see operand order,
which is why `code_001a7ac0` passed both while still carrying this.

`code_001a7ac0`'s frame decomposes exactly on January's side: 3 matrices (156) +
quaternion (16) + 6 vectors (72) + **one** 4-byte scalar = 248 = `0xf8`. `angle`
must own that slot since its address is taken. Ours spills a second scalar, which
is the whole +4.

### Ordered call sequences confirm statement order is right everywhere

Extracting each function's `REL32` relocation targets sorted by address gives the
exact sequence of calls it makes. Differencing that against January is a direct
test of *statement ordering*, independent of registers, slots and scheduling.

Result: **every written function calls the same things in the same order.** The
only two differences are already understood — `vehicle_moving_near_any_player`
has one extra `object_iterator_next` from MSVC's loop inversion, and
`vehicle_update` is still a scaffold.

That closes the last structural class. Combined with the earlier sweeps —
frame delta, relocation delta, `fstp`/`movsx`/slot-set tells, and now constants
and call order — the remaining residuals in `code_001a6290`, `code_001a6dc0`,
`code_001a7ac0`, `code_001a5b40` and `vehicle_export_function_values` are **not**
missing code, wrong types, wrong constants or wrong statement order. They are
instruction scheduling and slot assignment only.

Worth doing on any object early: it is one script run and it would have caught
the `vehicle_export_function_values` case 33/34 ordering immediately rather than
via address comparison.

### Sweep every written function's constants against January's

Cheap and it found two real bugs that had survived every other check. For each
`.text` symbol, collect the `__real@*` relocation targets on both sides and
difference the sets. Two encodings have to be normalised first: January spells a
constant reference as `['symbol', '__real@X', 0]` while our build spells the same
thing `['defined-noncode', '.rdata', '__real@X', 0]`. Reading field `[1]` on both
compares a symbol name against a section name and reports every function as
broken — which is exactly the false alarm I raised before catching it.

Found:

- **`code_001a5e50` had a sign error.** January uses `__real@3ea2f983` (`+1/pi`),
  ours `bea2f983`. Writing `-angle*x*(1.0f/_pi)` let MSVC fold the negation into
  the constant; January negates the angle (`fld / fchs / fmul / fmul`). Spelling
  it `(-angle)*x*(1.0f/_pi)` fixes it.
- **`VEHICLE_ANGULAR_ACCELERATION` was one ULP low**, `3c64c388` against
  January's `3c64c389`, and the doubled form differed too:
  `3f9c987103b761f5` against `3f9c987120000000`.

The second is the more useful finding. **MSVC folds constant expressions in
double and only rounds at the final assignment** — the inner `(real)` cast in
`((real)(0.8*_pi/180))` is ignored, so the fold uses double pi. Six spellings were
tried, including casting each operand and using `0.8f`/`180.0f` literals; all give
`3c64c388`. January's value is reachable only from a value that is *already a
float at compile time*:

```c
#define VEHICLE_ANGULAR_ACCELERATION 0.0139626344f
```

That literal is the only spelling giving both `3c64c389` **and**
`2.0*it == 3f9c987120000000`. `(0.8f*3.14159274f/180.0f)` gets the float right but
the double wrong, because `2.0*expr` re-folds from the unrounded expression rather
than from the rounded float.

So: whenever a constant is consumed both as a `real` and inside a
double-typed expression, only a float literal makes both exact. Sections went
38 -> 41, and every written function now uses exactly January's constants.

### `vehicle_export_function_values` — written, 1328/65 against 1312/67

Frame matches exactly (`0x4c`). 16 bytes over, two relocations short, 427
instructions against 401.

It is a four-iteration loop walking two pointers — `definition->function_modes`
and `vehicle->object.incoming_function_values` — with a **36-case switch** whose
jump table lives inside the function at `+0x488`. Reading that table out of the
section bytes gave the case-to-handler map directly: 28 distinct handlers, with
`{1,28,29,30,31}` sharing one, `{24,25,26,27}` another, `{18,19}` a third, and
case 13 falling to the default.

Each handler is a normalised ratio clamped by a shared `PIN(result, 0.0f, 1.0f)`
tail. The three denominators are computed once up front as
`MAX(fabs(forward), fabs(reverse))` over the `speed`, `slide` and `turn` blocks —
which independently confirms `unknown2f8`/`unknown2fc` as forward and reverse
maxima, the same reading `code_001a5e50` produced from its sign test.

Fields confirmed on the way:

- `definition->function_modes[4]` at `+0x31c` — exactly where the halocea header
  puts `function_modes`, reached independently from `lea ebx, [edi+0x31c]` and a
  loop that strides 2 bytes four times
- `vehicle->object.incoming_function_values` at `+0xd4`, with
  `parent_object_index` at `+0xcc` corroborating from `vehicle_moving_near_any_player`
- `1.0f/(0.9f-0.05f)` folds to `__real@3f969697` where `1.0f/0.85f` gives
  `...96` — the range-normalisation idiom again, third time in this object

**Case bodies are laid out in source order.** Comparing handler addresses showed
January emits case 34 before case 33; swapping them in the source is what brought
the frame to `0x4c`. That makes the jump table a free read of January's *source*
case ordering, not just its semantics.

Remaining: ours assigns an extra 4-byte slot at `[ebp-8]`, pushing the nine
`MAX`/`fabs` temporaries down by one slot each, while ending at the same total
frame. Hoisting `result` out of the loop body changes nothing. Two relocations
short suggests two `__real@00000000` references our version does not emit,
probably where January shares one `fld 0.0` between the flag-guarded cases and the
clamp tail.

### Declaration order does not control frame layout

Five orderings of `code_001a5e50`'s nine locals — including one built to match
January's observed layout exactly, shallow to deep — all produce **byte-identical
output**. MSVC assigns slots from its own liveness analysis, not from the order
the source declares them.

That matters because it rules out the obvious lever for every remaining frame
delta in this object. The levers that *do* work are the ones already recorded:
lexical scoping of a dying local, and naming or un-naming a value. Positional
reordering is not one of them, and is not worth trying again.

`code_001a5e50`'s residual 4 bytes trace to `[ebp-4]`: January frees it after the
`physics` pointer dies, by moving `physics` into `ESI` — which is free because
January re-reads `vehicle_index` from `[ebp+8]` at both later uses instead of
holding it. Ours keeps a register on `vehicle_index` and therefore has to spill
`seat_power[0]` to `[ebp-8]`, one slot more than January needs. That is an
allocator decision downstream of a register choice, with no obvious source-level
handle.

### `code_001a5e50` — written, 1104/32 against 1088/32

Relocation count matches exactly; size is 16 over and the frame is `0xa8` against
January's `0xa4`.

The clamp tail decodes as a **rate-limited approach**, not the arbitrary threshold
ladder it looks like. `spin = magnitude3d(&angular_velocity)/definition->unknown314`
is compared against `unknown448`, and each arm computes a step limit:

```c
if (spin>vehicle->vehicle.unknown448)
{
	maximum = PIN((1.0f-vehicle->vehicle.unknown448)*
		(1.0f-vehicle->vehicle.unknown448)*0.2f, 0.01f, 0.05f);
	vehicle->vehicle.unknown448 += MIN(spin-vehicle->vehicle.unknown448, maximum);
}
else
{
	maximum = -MAX(vehicle->vehicle.unknown448*vehicle->vehicle.unknown448*0.05f, 0.005f);
	vehicle->vehicle.unknown448 += MAX(spin-vehicle->vehicle.unknown448, maximum);
}
```

Both arms converge on one `fadd [edi+0x448]`, which is why the disassembly looks
like six thresholds rather than two clamped deltas. Every constant confirms it:
`3e4ccccd` 0.2, `3c23d70a` 0.01, `3d4ccccd` 0.05, `3ba3d70a` 0.005, plus
`3ea2f983` = 1/pi scaling the recovered angle and `3eaaaaab` = 1/3 averaging the
three moments.

January's frame, fully accounted: `4 + desired(36) + velocity(12) + thrust(12) +
force(12) + rotation(16) + actual(36) + difference(36) = 164 = 0xa4`, with
**`thrust` and `axis` sharing `-0x40`** and **`scaled` sitting inside the dead
`rotation` at `-0x5c`**. Reproducing that took scoping both dying locals into
lexical blocks, which moved the frame +20 -> +8.

The last 4 came off by **deleting a local**, not adding one: we held
`seat_power[0]` in a `power` variable across the whole body, where January caches
it only for the force scaling and re-reads `[edi+0x2e8]` fresh at both later uses.
Naming it kept a slot alive that January frees — the same spill-on-naming rule as
`code_001a5b40`, running the other way. Worth remembering that the rule cuts both
directions: name a value to force it into a slot, drop the name to free one.

Tried and reverted: inlining the clamp delta instead of binding `maximum`. `MIN`
and `MAX` evaluate their arguments twice, so the `PIN` expands twice and the
function grows to 1216. January's delta is never stored, which suggested it was
unnamed, but that reasoning was wrong — MSVC simply keeps a named local on the
x87 stack here.

### Binding a subexpression forces it before a branch — `code_001a5b40`

Ours emitted the `CEILING(speed, 1.0f)` clamp first and kept the other factor on
the x87 stack; January computes the other factor **first**, spills it to `[ebp-4]`,
then branches, then reloads with `fmul [ebp-4]`. Same arithmetic, opposite
schedule.

Swapping the two factors in the source does nothing — both spellings emit
identically. What works is **binding the factor to a named local**:

```c
maximum_angle = vehicle->vehicle.unknown434*0.5f;
angle = (1.0f-CEILING(speed, 1.0f))*maximum_angle;
```

Raw diff **125 -> 86**, and the whole clamp region — the spill, the reload, and the
`ebx`/`edi` allocation around it — comes into agreement. `code_001a5a30`, which is
byte-exact, has the same `unknown434*0.5f` bound to a local, so this is the
house style rather than a trick.

This is the answer to the x87-spill-across-a-branch question left open earlier in
this log: a value needed on both sides of a branch gets spilled when the source
*names* it, and stays on the stack when the source inlines it. No intervening call
is required.

Left in this function: the `cross_product3d` and `triple_product3d` block has every
multiply's operands in the opposite order from ours — January emits
`fld up.j / fmul forward.k` where we emit `fld forward.k / fmul up.j`. Both come
from the same `real_math.h` inline over the same arguments, so the operand-selection
rule behaves differently when one side is a `lea`-ed pointer and the other a struct
field. Not yet characterised; this is the pointer-vs-field case of the typed-pair
question.

### Sweeping the tells across every near-miss

Running the four tells over all nine near-misses at once — `fstp st(0)` count,
`movsx` count, and the `[ebp-N]` slot sets — is one script run and it collapsed
the backlog to a short list:

| function | size J/ours | fstp | movsx | slot diff |
|---|--:|--:|--:|---|
| `code_001a5b40` | 784/768 | 5/5 | 0/0 | none |
| `code_001a6590` | 384/384 | 1/1 | 0/0 | none |
| `code_001a6710` | 512/512 | 1/1 | 3/3 | none |
| `code_001a6910` | 656/656 | 2/2 | 2/2 | none |
| `code_001a6290` | 768/768 | 2/2 | 0/0 | none |
| `code_001a6dc0` | 800/800 | 7/7 | 4/4 | none |
| `code_001a7ac0` | 928/928 | 4/5 | 0/0 | shifted 4 |
| `vehicle_moving_near_any_player` | 384/432 | 6/6 | 2/2 | none |
| `vehicle_preprocess_node_orientations` | 1024/1024 | 11/11 | 13/13 | none |

Every counter width, every inline-helper pop and every local slot already agrees
except in `code_001a7ac0`. So the remaining residuals are **not** missing locals,
wrong counter types or longhand helpers — those classes are closed. What is left
is instruction selection and scheduling.

### `code_001a5b40` — the pointer form of an inline helper

Same frame, same slots, same relocation count, same `fstp` count, yet 16 bytes
short. The tell was in the addressing mode: January does `lea edi, [ebx+0x24]`
once and then indexes `[edi]`, `[edi+4]`, `[edi+8]`, while ours emitted
`[edi]`, `[edi+0x28]`, `[edi+0x2c]` — a pointer versus three struct references.
That is the signature of a helper taking `real_vector3d const *`, and the
arithmetic identifies it:

```c
point_from_line3d(global_up3d, &vehicle->object.forward, negative_k, &up_relative);
```

replacing three longhand component assignments. Raw instruction diff **197 -> 125**
at identical instruction count — the addressing now matches throughout. Note this
is the *general* form of the same projection that appears specialised in
`code_001a6290`, where `global_up3d` is folded to a literal `1.0f` in the third
component.

Still 768 against 784. Ours additionally emits a `MIN(x, 1.0f)` clamp — `fcom`
against `__real@3f800000` with `test ah,0x41` — that January does not have at that
point, so a `PIN`/`MIN` in our reconstruction is one clamp too many. Not yet
resolved.

### `code_001a7ac0` — 928/34, frame +4

Two fixes, both found by pointing the diagnostics at it rather than reading the
diff.

**Slot-set differencing localises a frame delta to specific locals.** Listing every
`[ebp - N]` displacement each side references and subtracting the sets showed
January using `-0x48` and `-0x4c` where we used neither — exactly the 8 bytes the
frame delta reported. Reading their two uses gave the cause immediately: they are
`axis.j` and `axis.i` feeding the same
`quaternion_to_angle_and_vector` -> `scale_vector3d` idiom already recovered in
`code_001a6290`, with the constant `__real@3d088889` = `1/30`. We had written the
three torque components inline instead of scaling into a vector. This is worth
running whenever the frame delta is non-zero — it converts "eight bytes missing"
into "these two slots, used here".

**The multiply order fix from the float rule applied again.** January selects a
factor from `{0.25f, 1.0f, 0.75f}` through two flag tests and then computes
`factor*(1.0f-throttle*throttle)*seat_power[0]` — factor leading. Ours had
`(1.0f-throttle*throttle)*factor*...`, which reorders the whole x87 sequence and
made the three constants appear to be loaded in the wrong places. Swapping the
spelling fixed the entire block.

Size and relocation count now both land (928/34, 299 vs 300 instructions). The
frame is +4: adding the `scaled` vector took it 0xf0 -> 0xfc where January is
0xf8, so January pays 8 for that vector and we pay 12. Scoping the quaternion into
a lexical block — the fix that worked in `code_001a6290` — overshoots to 0xf0,
because here January overlaps `scaled` onto **`angle`** rather than onto the
quaternion: `angle` lives at `-0x14`, and `scaled.k` is written to that same slot
after `angle` dies, with `scaled.i` never stored at all. Reverted; the +4 variant
is kept as the closer of the two. Still `NonMatching` and uncredited.

### `code_001a6dc0` — 800/28 exact, one transposed instruction

After the single-exit rewrite this closed to a one-instruction tie, via three
findings that all generalise.

**`movsx` in a loop increment means a `short` counter.** January increments with
`inc eax / movsx edi, ax / cmp edi, [ebx]` — a sign-extended 16-bit value. Ours
declared `long suspension_index`. Changing it to `short` recovered 16 bytes *and*
closed the 4-byte frame gap in one edit, because the narrower local repacked the
frame. Any `movsx`/`cmp word ptr` around a loop variable is this tell.

**A one-ULP constant mismatch encodes the source's arithmetic.** January had
`__real@3fd55556` where ours emitted `__real@3fd55555` — adjacent floats. Ours
came from `1.0f/0.6f`; the fix is `1.0f/(0.9f-0.3f)`, which folds to `3fd55556`
exactly. So January does not hard-code the reciprocal, it normalises to a range:
`(x-lo)*(1/(hi-lo))` with `lo=0.3f`, `hi=0.9f`. The wrong constant was evidence of
the wrong *idiom*, not a typo, and the corrected form is both bit-exact and more
readable. Worth checking with a two-line Python probe whenever a `__real@` differs
in the last hex digit — a mismatched constant section also shows up as a missing
`.rdata` in the gate's section count.

**A trailing `fstp st(0)` after a run of `fld X / fmul st(1)` means a real_math
inline.** January keeps the shared scalar on the x87 stack across all three
components and then *discards* it; writing the components out longhand lets MSVC
consume it on the last multiply instead, which is two instructions shorter.
`point_from_line3d(&point, &normal, offset, &start)` and
`scale_vector3d(&normal, extent+extent, &vector)` reproduce it exactly.

What is left is a single transposition: `push 0xc0a0` — the flags argument to
`collision_test_vector`, pushed last under cdecl — lands one slot before
`fadd [ebp-0x50]` in ours and one slot after in January. Instruction counts,
size and relocation count all agree (278/278, 800/28). Tried without effect:
nesting `point_from_line3d` into the call, nesting both helpers, and a statement
break before the call. **It stays `NonMatching` and uncredited** — a
one-instruction scheduling tie is still not a match.

### A relocation-count surplus means a duplicated call

`vehicle_moving_near_any_player` is our only function *larger* than January — 432
against 384 — and the gate already prints the reason for free: **14 relocations
against 13**. Relocations do not appear from nowhere. One extra means one extra
call site, and the disassembly confirms it: MSVC inverted the object-iterator loop,
peeling the `object_iterator_next` test above the body and emitting a second copy
at the bottom, then duplicating the epilogue to serve the bottom exit. That
duplicated call and epilogue are the whole 48-byte surplus.

January did not invert. Its loop top *is* the call, at `0xb3`, entered by
fallthrough and re-entered by a single `jmp 0xb3` — one call site, one epilogue,
which is also why its two stack cleanups stay separate (`add esp,0xc` then
`add esp,4`): the loop top is a branch target, so MSVC cannot merge across it.
Ours merges them into `add esp,0x10`, which is a *symptom* of the peeling, not a
separate defect.

Rewriting the `while ((object = object_iterator_next(&iterator))!=NULL)` as
`for (;;) { object = object_iterator_next(&iterator); if (!object) break; ... }`
produces **byte-identical output**. The inversion is not reachable from the loop's
source form, at least not by that route. Not yet solved.

The reusable part is the diagnostic. Reloc delta is already in every gate line:

- **ours > January** — a call or data reference is duplicated; suspect loop
  inversion or a tail MSVC copied.
- **ours < January** — a call site is missing outright, so the reconstruction is
  incomplete rather than mis-shaped.

Together with the frame delta above, two numbers already on screen classify most
near-misses before reading a single instruction.

### Frame-size delta is a cheap standing diagnostic

Comparing our `sub esp, N` against January's for every function costs one script
run and points straight at the kind of error. Current deltas:

| function | Jan frame | ours | delta | Jan size | ours |
|---|--:|--:|--:|--:|--:|
| `code_001a6dc0` | 0xd4 | 0xd0 | -4 | 800 | 768 |
| `code_001a7ac0` | 0xf8 | 0xf0 | -8 | 928 | 912 |
| `code_001a5b40` | 0x3c | 0x3c | 0 | 784 | 768 |
| `vehicle_moving_near_any_player` | 0x58 | 0x58 | 0 | 384 | 432 |

This corrected a direction error: an earlier note assumed ours were *larger* than
January. They are mostly **smaller** — we are missing code, not emitting extra.
A negative frame delta means January binds a value to a local that we inline.

### Single exit with a result variable — `code_001a6dc0`

January's prologue does `push ebx` then **`xor bl, bl` before any branching**, and
every early exit jumps to one shared epilogue. That ordering is the tell: if the
source had `return FALSE` at each exit, MSVC would set `AL` at each one and would
have no reason to initialise `BL` up front. A boolean living in a callee-saved
register across the whole body means a **single-exit function with a result
variable**, and MSVC merged the exits.

Ours had six `return` statements. Rewriting to one `return result;` with the
guards inverted into nesting, and `result = TRUE` set on the success path, brought
`push ebx`, `xor bl, bl`, the whole `esi`/`edi` assignment and the shared epilogue
into agreement at once. Everything now differing is a displacement or branch-target
shift except two things: the frame is still 4 bytes short, and a loop rotation
downstream of it (ours emits an extra `jmp` into the loop and a `lea ecx,[ecx]`
alignment pad).

Worth trying first on any function whose disassembly zeroes a byte register in the
prologue. Tried and rejected for the missing 4 bytes: binding the twice-computed
`shift-current` to a local — MSVC keeps it in a register and the frame does not
move.

### Field names adopted from the halocea corpus — provenance

Names in `struct vehicle_definition` for `wheel_circumference`, `suspension_sound`,
`crash_sound`, `material_effects` and `effect` come from
`surreptitiousresearch/halocea`, a decompilation of the June 2011 Halo CEA Xbox 360
prototype. **Names only. No code shape was taken from it, and none should be** —
it is a non-matching reconstruction of a different binary on a different
architecture, so it carries no information about MSVC 13.00.9254.1 codegen, which
is what every open residual in this object turns on.

Why the names are trustworthy, and why only these:

- Their per-function files use `desired_facing_vector`, `aiming_vector`,
  `seat_power`, `control_flags`, `persistent_control_timer`,
  `gunner_inactive_ticks`, `integrated_light_battery`, `grenade_throw_state` —
  every one matching this project's existing `units.h` exactly and independently.
  Their headers state the layouts are DB-verified. These are recovered names.
- **Runtime datums do not map.** They put `unit` at `0x1f4` and `vehicle` at
  `0x4cc`; our probed 2002 build has `0x1a4` and `0x424`. The datums grew ~88
  bytes between 2002 and 2011, so `_vehicle_datum`'s `unknown42c` and friends
  **cannot** be named this way and were left alone.
- **Tag definitions do map**, because they are the on-disk format. Both sides put
  the vehicle payload at `0x2f0`.

Corroboration from our side, before adopting anything:

| ours (absolute) | payload | theirs |
|---|---|---|
| `unknown310` | 0x20 | `wheel_circumference` |
| `unknown3bc`/`3cc`/`3dc`/`3ec` | 0xcc/0xdc/0xec/0xfc | four `tag_reference` `.index` |

Our four `long unknown*` already sat at a 16-byte stride with 12 bytes between —
`tag_reference` with `.index` at `+12`, a layout recovered from the disassembly
alone. `wheel_circumference` is used as `fmod(vehicle->vehicle.unknown438,
definition->wheel_circumference)`: wheel rotation reduced modulo its
circumference. And after modelling the four references, `sizeof(struct
vehicle_definition)` probes as **`0x3f0` = 1008 bytes, exactly the size they
state** — two independent derivations agreeing.

The rename touched 19 use sites and is byte-neutral: 26/39 and 3808 bytes before
and after. That the 26 exact functions still match is itself the proof no offset
shifted.

Not adopted: the `speed` (0x2f8, four reals) and `turn` (0x308, two reals) blocks
are named as blocks in their header but their sub-fields are not recovered, so
ours keep offset names with a comment. Function names cannot be adopted at all —
`relocation_infos_equal` compares symbol *names*, and the gate looks up
`_code_001a6290` in the object built from the January split, so renaming a
function makes the comparator stop finding it and mismatches every caller's
relocation.

### VC7 packs a dead aggregate's storage — and a lexical block is the lever

`code_001a6290` was 768 B / 22 rel with the right size and relocation count but a
frame of `0xfc` against January's `0xf0`, which shifted every matrix displacement
and made the raw instruction diff 149.

January's frame, read off the displacements, allocates downward as scalars (8),
`facing` (12), `perpendicular` (12), `force` (12), `torque` (12), the quaternion
(16), `axis` (12), three matrices (156) — exactly 240 = `0xf0`, with **no slot at
all** for the scaled axis vector. Its three stores land at `-0x44`/`-0x40`/`-0x3c`,
inside the quaternion's sixteen bytes. Not on a field boundary either:
`real_quaternion` is `{real_vector3d v; real w;}`, so those are `v.j`, `v.k` and
`w`. It is byte-level reuse of a dead local's storage, not aliasing.

Ours declared the quaternion at function-block scope, so MSVC kept it live to the
end of the function and gave the scaled vector its own twelve bytes. Wrapping just
the matrix/quaternion work in a lexical block that closes before the scaled vector
is born reproduces January's packing exactly:

```c
{
	real_quaternion rotation;

	matrix4x3_rotation_from_vectors(&actual, ...);
	...
	quaternion_to_angle_and_vector(&rotation, &angle, &axis);
}

scale_vector3d(&axis, angle*(4.0f/30.0f), &scaled);
```

Frame `0xf0`, raw diff 149 -> 87. Putting the *scaled vector* in an inner block
instead does nothing — it is the dying local that has to be scoped, not the one
being born. Worth remembering as a general lever: when our frame is larger than
January's by exactly the size of one local, look for an aggregate whose live range
should have ended.

Residual is now one cause. January caches `translational_velocity.j` and `.i` into
`[ebp-4]`/`[ebp-8]` during the dot product and reuses them in the yaw angle; ours
re-reads them from `[esi+...]`. That one choice also rearranges the x87 stack for
the force block, which accounts for the rest. Swapping the two addends of the force
terms is a **tie** — both spellings emit identically, so the operand-order rule
does not reach the addition level here.

### `code_001a6290` — signature confirmed from the disassembly

The signature inferred above from `code_001a8800` is confirmed directly by
January's own `code_001a6290`, which is unambiguous about it:

- `[ebp+8]` is `vehicle_index`, `[ebp+0xc]` is the second stack argument.
- `EDI` is **written** at `+0x18`, `+0x1c`, `+0x20`, `+0x24`, `+0x28` and again at
  `+0x78`, `+0x7c`, `+0x80`, `+0x84`, `+0x88`, and is pushed to `physics_update` --
  but it is **never loaded and never saved**: the epilogue is `pop esi / pop ebx /
  pop ebp`. A register that is used, never initialised and never preserved is an
  incoming parameter.
- The second group of offsets is the first group plus `0x60`, which is exactly
  `sizeof(struct vehicle_powered_mass_point_state)` as already modelled. So `EDI`
  is that array and the function writes elements `[0]` and `[1]`.

So the convention is `(vehicle_index, mass_points)` on the stack with
`EDI = state`, matching `code_001a58e0` and `code_001a5a30`.

The body is a session's work in its own right: `normalize3d`, `yaw_vectors`, two
`matrix4x3_rotation_from_vectors`, `matrix4x3_inverse`, `matrix4x3_multiply`,
`matrix4x3_rotation_to_quaternion`, `quaternion_to_angle_and_vector`, and two
`physics_update` tails selected by `physics->unknown68==2`. Not started.

### How close is each near-miss, really

Measured with registers and displacements normalised away, which separates "the
code is right, MSVC picked other registers" from "the code is actually different":

| function | normalised | raw | verdict |
|---|--:|--:|---|
| `vehicle_preprocess_node_orientations` | **0** | 7 | register choice only |
| `code_001a6590` | 6 | 6 | real difference |
| `code_001a6710` | 20 | 78 | real difference |
| `vehicle_moving_near_any_player` | 21 | 31 | real difference |
| `code_001a6910` | 46 | 83 | real difference |
| `code_001a6dc0` | 46 | 141 | real difference |
| `code_001a5b40` | 107 | 116 | real difference |
| `code_001a7ac0` | 118 | 201 | real difference |

**Only one function — 1024 bytes, 5.9% of the object — is blocked purely on
register choice.** An earlier note in this log guessed that `6710`, `6dc0` and
`7ac0` were in that class too, on the strength of their raw diffs being dominated
by register-name cascades. They are not: normalising shows real structural
differences underneath. Measure before grouping.

Nudges tried on the one that does qualify, all without effect: all six
declaration-order permutations of its three pointers, hoisting `graph` above
`vehicle`, and splitting the declarations from their initialisers.

### `_vehicle_export_function_values` — 1312 B / 67 rel, not written

Started and deliberately stopped. The opening is a run of
`MAX((real)fabs(definition->X), (real)fabs(definition->Y))` pairs — note `MAX`
expands its arguments twice, so each `fabs` needs binding to a local first, which
is exactly what January's `[ebp-8]`/`[ebp-0xc]` slots are. That yields four more
`vehicle_definition` reals at `+0x308`, `+0x30c`, `+0x330`, `+0x334`.

Beyond that it is a large `switch` — the relocation list contains **twelve
self-references** into the function body, so multiple jump tables — with five
`magnitude3d` calls and a `component_vectors_from_normal3d`. It wants a session of
its own rather than the tail of this one.

### `code_001a6590` — parked at six instructions

384 B / 15 rel, size- and relocation-exact, **six** differing instructions in two
independent clumps. Both were measured exhaustively rather than guessed at, using
instruction distance rather than pass/fail:

1. **One `fadd` operand pair.** January emits `fld velocity.i / fadd position.x`;
   we emit the reverse. **Seven** spellings of the three position assignments —
   every combination of operand order across `x`/`y`/`z`, plus `add_vectors3d` in
   both argument orders — give *identical* output, diff=6 every time. It is
   spelling-invariant.
2. **The `axis` struct copy schedule.** January batches three loads then three
   stores; we interleave load/store. `axis = *angular_velocity` is correct —
   replacing it with three explicit component copies is far worse (diff 6 -> 119).
   Moving the copy before the position block changes nothing.

The producer lens was tried too: `velocity` is written by literal `*= 0.835f`
stores, which is exactly the pattern that unfroze the operand choice in
`vehicle_find_pathfinding_surface_index`. Here `scale_vector3d(velocity, 0.835f,
velocity)` compiles to byte-identical output — MSVC canonicalises the helper and
the literal stores to the same thing in this context, so the lever does not exist.

**The rule: when consecutive fields of the same type are written together, model
them as the `real_math.h` vector type and set them with the matching `set_real_*`
helper.** Loose scalars produce the right values but the wrong schedule. This is
the same mechanism as the earlier helper-versus-literal-stores finding for float
operand selection — inlined `real_math.h` helpers and hand-written component
stores are not interchangeable to MSVC.

### The float operand-selection rule — SOLVED

> **Correction.** An earlier revision of this log stated that "MSVC canonicalises
> which operand of a float multiply gets loaded, and the source spelling cannot
> override it." **That was wrong**, and stated far too confidently. The source
> controls it. The rule below was recovered with isolated probes — one small
> function per hypothesis, project cflags, dumping only the `f*` instructions —
> after a dozen edits to the real function had produced nothing.

For `A*B` where both operands are stack memory:

| situation | emitted |
|---|---|
| both opaque, single use | `fld B / fmul A` — the **right** operand is loaded |
| one operand shared across 3 expressions, `shared*varying` | `fld varying / fmul shared` for all three |
| same, `varying*shared` | shared loaded for the **first** component only |
| **varying operand built by literal component-wise stores** | `fld shared / fmul varying` always — **source order stops mattering** |
| **varying operand built by an inlined `real_math.h` helper** | source order controls it again |

The last two rows are the whole answer, and they interact:

**Writing a vector with three literal component assignments freezes MSVC's operand
choice; building the same vector with the `real_math.h` helper leaves it free.**

`vehicle_find_pathfinding_surface_index` had

```c
vector.i = global_down3d->i+global_down3d->i;   /* freezes the choice */
vector.j = ...
vector.k = ...
```

Replacing exactly those three lines with

```c
add_vectors3d(global_down3d, global_down3d, &vector);
```

made the function **byte-exact** — with no change at all to the multiply that was
actually differing, which is why every edit aimed at the multiply itself had
failed. Ruled out along the way, all with no effect: operand order at the
multiply, addend order, struct members versus arrays, store recency, declaration
order, where the big struct is declared, and a scalar copy of the shared operand.

Both compiler suites and every arch flag were already eliminated (above), so this
is purely a source-shape rule.

### `code_001a5b40` — steering thrust (768 B / 25 rel vs 784 B / 25 rel)

Reconstructed; relocation count matches, 16 bytes short. It is the three-powered-
mass-point path: it fills `state[0..2]` (steering angle from
`unknown434*0.5f` damped by `1 - CEILING(fabs(magnitude3d(velocity)*2.5f), 1)`),
builds an up vector relative to the forward axis, rotates it about the forward
axis by `triple_product3d(forward, velocity, global_up3d)*2*_pi`, measures
`angle_between_vectors3d` against the vehicle's up, signs it by
`dot_product3d(cross, up_relative)`, and converts that into a forward thrust
`PIN(sqrt(fabs(angle)*2.0*K)*sign - dot_product3d(forward, angular_velocity),
-K, K) * physics->xx_moment` with `K = (real)(0.8*_pi/180)` — 0.8 degrees. The
`2.0*K` term is a **double** constant `__real@3f9c987120000000`, which is what
`2.0 * <float K>` folds to; a float `2.0f*K` gives different bits.

Three lessons, all the same shape — **bind repeated float subexpressions to a
local**, because MSVC will otherwise recompute them:

* **`CEILING`/`PIN` expand their argument twice.** Writing
  `CEILING(fabs(magnitude3d(v)*2.5f), 1.0f)` made MSVC compute the whole
  magnitude twice — 96 wasted bytes. Binding it to a local first fixes it, and
  MSVC keeps the local in `st0` so the frame does not grow.
* **`sine(x)` and `cosine(x)` on the same argument duplicate it too.** Two
  `triple_product3d` calls became two full inline expansions; one `spin` local
  collapses them and brought the relocation count from 27 to 25.
* **A negated operand reused across components needs binding too.** Writing
  `up_relative.i = -forward.k*forward.i+global_up3d->i` three times makes MSVC
  refold each one into `global_up3d->i - forward.k*forward.i` (`fsubr`).
  January computes `-forward.k` once with a single `fchs` and duplicates it on
  the stack. `negative_k = -vehicle->object.forward.k;` reproduces that.

Progress on this function went 880 -> 832 -> 768 against a target of 784.

**The last 16 bytes are unsolved — but the search space is now mapped.** January spills `unknown434*0.5f` to `[ebp-4]`
before the `CEILING` compare and reloads it with `fmul [ebp-4]` afterwards; ours
keeps the whole computation on the x87 stack, which is shorter. Consequently
January's `1.0f - x` uses the memory form `fsubr [1.0f]` while ours uses
`fld 1.0 / fsub st(2)`. Tried without effect: binding the factor to a local and
assigning it before the magnitude, after the magnitude, and on the left of the
final multiply; and moving the two scalar `state[0]` stores ahead of the angle
computation to reproduce January's interleaving; a compound-assign form
(`angle = ...; angle *= 1.0f-CEILING(speed,1.0f);`); and replacing the `CEILING`
macro with a statement-level `if (speed>1.0f) speed = 1.0f;`. None of them makes
MSVC spill.

#### What forces an x87 spill across a branch

Probed in isolation, six variants of "compute V, branch, use V after":

| probe | shape | result |
|---|---|---|
| V bound to a local, used only after the branch | — | **no spill**; MSVC keeps V on the x87 stack and *tail-duplicates* the join |
| V also used inside one arm | — | no spill, simpler code |
| three live float values across the branch | — | no spill, more stack juggling |
| V computed from a call | — | no spill |
| **V passed to a call before the branch** | — | **spill**, and the join becomes `fsubr [const]` + `fmul [mem]` — January's exact shape |
| the branch operand computed from a call | — | no spill |

So the one trigger found is **an intervening call that forces V to memory**;
everything else keeps it in the x87 stack. January's `code_001a5b40` has *no* call
between computing `unknown434*0.5f` and the branch — the bytes were re-read to be
sure — so its spill has some other cause that six controlled variants did not
reproduce.

Our version is genuinely the shorter code here; January is spilling where it does
not need to. That is worth stating because it means the remaining 16 bytes are
**not** a missing optimisation on our side, and the next attempt should look for
what pins the computation in place rather than for a cheaper way to write it.

### How far the rule reaches

Applied to every remaining stuck function. It fixed **two**:

* `vehicle_find_pathfinding_surface_index` — `add_vectors3d` for the down-vector
  doubling.
* `vehicle_accelerate` — `add_vectors3d(&angular_velocity, &torque,
  &angular_velocity)` in place of three `+=` statements. Note the differing
  instructions were in the *cross product*, three statements earlier, exactly as
  in the other case.

It did **not** fix the remaining five, and the reason matters: **their residuals
are not float operand selection at all.** Tried and rejected on each —
`add_vectors3d` both argument orders, `scale_vector3d`, `negate_vector3d`,
uniform operand order, and explicit component copies in place of struct
assignment:

| function | actual residual |
|---|---|
| `_code_001a6710` | register allocation (which register accumulates the marker count) and push scheduling |
| `_code_001a6590` | x87 scheduling of a struct copy, plus one mixed-order add |
| `_code_001a5a30` | `fsin`/`fcos` evaluation order |
| `_code_001a58e0` | store scheduling; the source order already matches January |
| `_code_001a6910` | where MSVC sinks an `fchs` |

So the float rule is real and worth applying, but it accounts for two of the seven
stuck functions rather than all of them. The other five need their own
investigation, and lumping them together as one residual class — as an earlier
revision of this log did — was wrong.

**`_vehicle_moving_near_any_player` — 384 B / 13 rel target, 432 B / 14 rel ours.**
Semantically complete and correct in every detail except loop shape: MSVC inverts
the outer `while ((object = object_iterator_next(&iterator))!=NULL)` loop and
duplicates the `object_iterator_next` call plus the epilogue at the bottom, which
is the whole 48-byte and one-relocation difference. January keeps a single loop
head with three predecessors. Tried and rejected: `for(;;)` + `break`,
`while (object = ...)`, `break` instead of `goto`, a flag test on the outer loop,
and hoisting the inner index to function scope.

Two real fixes did come out of this function and are worth reusing:
`if (unit_count)` not `if (unit_count>0)` (January uses `je`, not `jle`), and the
velocity threshold constant `__real@3a91a2b4` is `(real)((1.0/30.0)*(1.0/30.0))`
folded in **double** — the float-folded `(1.0f/30.0f)*(1.0f/30.0f)` is one ULP off
at `3a91a2b5`.

## 5. Predicate cheat-sheet

The x87 compare-and-branch idioms cost real time to re-derive, so:

| emitted | meaning |
|---|---|
| `test ah,0x41 / jne` | `<=` (C0 or C3) — i.e. the `> 0.0f` form jumps away |
| `test ah,0x44 / jnp` | **equality** — this is `!= 0.0f`, *not* `> 0.0f` |
| `test ah,0x05 / jp`  | `<` (C0 or C2) |
| `test ah,0x01 / je`  | `>=` (C0 alone) |

`vehicle_stuck` went from a one-instruction miss to exact purely by changing
`normalize3d(direction)>0.0f` to `!=0.0f` on the strength of the second row.

## 6. The cluster blocker — solved

Walking every internal relocation gives this call graph:

```
vehicle_update  ->  code_001a4fa0, code_001a58e0, code_001a5a30, code_001a5b40,
                    code_001a6590, code_001a6ba0, code_001a6dc0, code_001a70e0,
                    code_001a7ac0, code_001a7e60, code_001a8800
code_001a8800   ->  code_001a5e50, code_001a6290, code_001a6910
code_001a5e50   ->  code_001a5810
code_001a7ac0   ->  code_001a6710
code_001a7e60   ->  code_001a6910
```

Every `_code_*` static is reachable only from `_vehicle_update`, and **MSVC
discards an unreferenced static entirely** — so none of those 17 sections could
even be made to appear, let alone gated. They also take their arguments in
registers under VC7's custom convention, chosen by the compiler from the call
sites, so the convention looked un-pinnable in isolation too.

**Both problems are solved by a stub caller, and this is now demonstrated rather
than assumed.** `vehicle_update` in this file is deliberately *not* a
reconstruction — it is four lines that declare the two stack buffers and call the
statics. With only that in place:

* `_code_001a4fa0` compiled at **160 B / 3 rel — the target's exact size and
  relocation count — on the first build**, and reached **byte-exact** after two
  ordinary source fixes (a local `mass_point` pointer instead of repeated
  indexing, and moving the `+0x428` store ahead of the `+0x42b` saturating
  increment).
* `_code_001a6590`, `_code_001a5a30` and `_code_001a58e0` likewise reached
  **384 B / 15 rel**, **272 B / 8 rel** and **336 B / 9 rel** — all size- and
  relocation-exact, each with only a handful of scheduled instructions out of
  place.

MSVC assigned exactly January's registers (`EAX` = vehicle index, `EDI` = mass
points, stack = state) purely from the parameter list. `_code_001a5810` then
confirmed the stronger form of this: it is called only by `code_001a5e50`, which
does not exist yet, so it was called from the stub with **invented arguments** —
and MSVC still gave it January's exact `ECX`/`EAX`/`ESI` convention and it came
out **byte-exact**. The convention follows the callee's own signature, not the
details of the call site, so a static can be reconstructed before its real caller
exists. **The remaining statics can
now be reconstructed and gated one at a time, in any order, without touching
`vehicle_update`.**

### `vehicle_mass_point_state` (buffer A element, `0x130`)

Recovered from `code_001a70e0`, which is the richest consumer:

```
+0x00 unsigned long flags      bit 1 = in contact
+0x04 real_point3d position
+0x54 real_vector3d velocity
+0x60 real_vector3d normal
+0x70 short material_index
+0x74 real                     penetration depth
```

`code_001a70e0` raises a material effect for every contacting point whose
`magnitude3d(velocity)` exceeds `0.03f`, at
`position + (depth - definition->+0x68 + 0.003f) * normal`, with a contact normal
of `velocity * (0.8660254f/speed) + normal*0.5f` and a scale of
`PIN((speed-0.03f)*4.5454545f, 0.0f, 1.0f)`.

Two source-shape lessons from it, both generally useful:

1. **`scale_vector3d` then three `+=`, not three fused expressions.** Writing
   `normal.i = mass_point->normal.i*0.5f + inverse*velocity.i` interleaves the
   products; January computes all three scaled velocities first and then adds the
   halves, which is `scale_vector3d(&velocity, inverse, &normal)` followed by
   three `+=` statements.
2. **A PIN into its own variable is not the same as a PIN into a new one.**
   `scale = PIN(scale, 0.0f, 1.0f)` lets MSVC drop the pass-through arm, because
   the store is a self-copy. January keeps a redundant `mov edx,[ebp-4] /
   mov [ebp-4],edx` there, which is what you get from
   `sound_scale = PIN(scale, 0.0f, 1.0f)` with a *separate* destination that the
   allocator later coalesces onto the same slot. This was the last 16 bytes of
   `code_001a6ba0`.
3. **A local element pointer is sometimes required and sometimes forbidden.**
   `code_001a4fa0`, `code_001a70e0` and `code_001a6710` all need
   `p = &array[i]` materialised; indexing in place folds the offset into the
   addressing mode (`lea eax,[ebp+esi-0x750]` instead of
   `lea esi,[ebp+eax-0x78c]` then `lea ecx,[esi+0x3c]`). But `cheats_update` and
   `cheats_load` in `cheats.obj` needed the exact opposite. Check the disassembly
   rather than assuming either way.
4. **A count that feeds a 32-bit `test` is a `long`.** `code_001a6710`'s
   `total_marker_count` as a `short` gives `test ax,ax` and a 16-byte-shorter
   function; as a `long` it gives January's `test esi,esi`. The per-array count
   next to it stays `short` because it is compared with `cmp bx,word ptr [...]`.
5. **`cond ? 10 : 9` is not `9 + TEST_FLAG(...)`.** The latter lets MSVC fold the
   test into a bare `and` because it can prove `x&1` is already 0 or 1. January's
   `neg cl / sbb ecx,ecx / neg ecx / add ecx,9` is the boolean materialisation
   MSVC only emits for a two-constant ternary. This was the last instruction
   difference in the function.

### `code_001a6710` — thruster impact effects (size- and reloc-exact)

Collects up to 16 `"hover thrusters"` and `"jet thrusters"` markers, fires a
random vector within a `_pi/12` cone from each marker's forward, ray-casts it, and
on a hit spawns `effect_new_unattached_from_markers` with three markers named
`"incident"`, `"normal"` and `"reflected"` — hence the `"reflected"` string COMDAT
this object owns. The cast length is
`(hover ? +0x444 : +0x448) * 6.0f + 2.0f`, and the effect scale is
`1.0f - collision.t`.

The cone angle is worth noting: January's `__real@3e860a92` is `_pi/12` folded in
double. A literal `0.2618f` gives `3e860aa6` — close enough to look right and
wrong in the last two bytes.

Its residual is 171 instructions against January's 171, same size and relocation
count, differing only in which register accumulates the two marker counts and one
push ordering. Several spellings of the addition were tried.

### `code_001a6910` — hover-thruster ground effects (size- and reloc-exact)

The sibling of `code_001a6710`: same marker / cone / ray-cast shape, but only
`"hover thrusters"`, a `15.0f` cone, four effect markers (`"incident"`,
`"normal"`, `"reflected"`, `"midpoint"`) and a scale of
`PIN(-forward.k * (1 - collision.t) * seat_power[0], 0, 1)` gated on `> 0`.

Two things it pinned down:

* **The `PIN`'s lower arm can vanish.** January tests `scale < 0` and branches
  straight to the loop increment without ever storing `0.0f`, because the
  following `> 0.0f` guard would reject it anyway. That is
  `PIN(x, 0.0f, 1.0f)` followed by `if (scale>0.0f)`, not two hand-written
  comparisons.
* **The midpoint needs its own local.** `midpoint3d(...)` writes a standalone
  `real_point3d` which is then copied into `marker_points[3]`; writing it
  directly into the array element makes the frame 12 bytes small.

Residual: 200 instructions against January's 200, same size and relocation count.
January negates `forward.k` before multiplying; MSVC sinks the `fchs` past both
multiplies no matter how the expression is spelled (parenthesised unary minus,
`0.0f - x`, and reordered operands were all tried).

### `_unit_datum` fields this object uses

Located by auto-generating an `offsetof` probe over every field name parsed out of
`units.h`, which is much faster than counting by hand:

| vehicle_datum offset | field |
|---|---|
| `+0x1b8` | `unit.control_flags` |
| `+0x1d4` | `unit.desired_facing_vector` |
| `+0x228` | `unit.throttle` |
| `+0x2e8` | `unit.seat_power[0]` |
| `+0x2ec` | `unit.seat_power[1]` |

### Buffer B's element stride, recovered

`code_001a5a30` and `code_001a58e0` write buffer B at `+0x00/+0x1c/+0x20/+0x24/
+0x28` and then again at `+0x60/+0x7c/+0x80/+0x84/+0x88` — the same five fields
`0x60` apart. **Buffer B is an array of `0x60`-byte powered-mass-point states**,
and `32 * 0x60 = 0xc00`, which fits the `0xc7c` offset with 124 bytes of ordinary
locals below it. Both functions fill exactly two elements, guarded by
`physics->powered_mass_points.count==2`.

The two are near-twins and worth reading together — they are the per-vehicle-type
wheel drivers:

* `code_001a5a30` (steered pair): advances `+0x438` by `+0x42c`, wraps it with
  `fmod` against `definition+0x310`, and writes `sine`/`cosine` of
  `+0x434 * 0.5f` into the two states with the second's sine negated.
* `code_001a58e0` (tread pair): forms `left = +0x42c - +0x434` and
  `right = +0x434 + +0x42c`, advances and wraps `+0x43c` and `+0x440`
  independently, and writes a fixed `0.0f`/`1.0f` orientation into both states.

Both then call `physics_update(vehicle_index, state, mass_points, NULL, NULL)`, or
the same with `NULL` state when the count is not 2. That also pins
`physics_update` at five parameters, matching the two-argument
`code_001a4f50` shim.

Typing note: `_vehicle_datum` fields `+0x42c`, `+0x430`, `+0x434`, `+0x438`,
`+0x43c` and `+0x440` are all `real`. Leaving them as `long` produced integer
arithmetic and an `__ftol2` call — an obvious and quick tell.

### The two stack buffers

`vehicle_update`'s frame is `0x327c` (it uses `__chkstk`). The two buffers passed
to every static are:

| buffer | address | size | meaning |
|---|---|--:|---|
| A | `[ebp-0x327c]` | `0x2600` = 9728 | mass-point runtime array — **exactly 32 x `0x130`** |
| B | `[ebp-0xc7c]` | ~`0xc50` | vehicle physics state, layout not yet recovered |

### Static signatures, read off the call sites

| static | stack args | register args |
|---|---|---|
| `code_001a58e0` | (vehicle_index, A) | EDI = B |
| `code_001a5a30` | (vehicle_index, A) | EDI = B |
| `code_001a5b40` | (vehicle_index, A) | ESI = B |
| `code_001a70e0` | (B, A) | EAX = vehicle_index |
| `code_001a6dc0` | (vehicle_index) | — (returns boolean) |
| `code_001a6ba0` | (vehicle_index, &vector, A) | — |
| `code_001a4fa0` | (B) | EAX = vehicle_index, EDI = A |
| `code_001a7ac0` | (vehicle_index, B, A) | — |
| `code_001a7e60` | (vehicle_index, real, B, A) | — |
| `code_001a8800` | none | ESI = vehicle_index, EAX = B, EBX = A |
| `code_001a6590` | (vehicle_index) | — |

Twelve of the sixteen statics call no other static, so they are independently
reachable right now. Only `code_001a8800`, `code_001a5e50`, `code_001a7ac0` and
`code_001a7e60` have outgoing static calls and must be done after their callees.

### `vehicle_update` itself

Still 2320 B / 98 rel and untouched. It needs roughly fifteen further fields
modelled — `unit_definition`-relative `0x2f8`, `0x308`, `0x30c`, `0x314`, `0x330`,
`0x340`, `0x344`, and `unit_datum`-relative `0x1b8`, `0x1d4`-`0x1dc`, `0x228`,
`0x22c`, `0x2e8`, `0x2ec` — plus a `profile_enter_private` block keyed off
`_data_00316228`, a second `vehicle_type` jump table at `+0x8f0`, and the
`global_current_collision_user_depth` assert. It is the last thing to do, not the
first.

The only other genuinely independent work is
`_vehicle_preprocess_node_orientations` (1024 B / 42 rel),
`_vehicle_export_function_values` (1312 B / 67 rel) and the `.data` table
`_data_00316228` (1528 B / 1 rel).

## 7. `code_001a6dc0` — suspension animation (768 B / 28 rel vs 800 B / 28 rel)

Written. Relocation count matches exactly; 32 bytes short, so **not credited**.

It drives the vehicle's suspension animation and confirmed the `+0x44c` byte
array: `mov al, byte ptr [edi + ecx + 0x44c]` indexes it by the loop counter,
which is why `vehicle_reset` `csmemset`s exactly 8 bytes there.

Per suspension entry it transforms the mass point's position and normal into
world space, ray-casts along the suspension travel, and writes the compressed
result back:

```c
shift = PIN((1.0f-collision.t)+(1.0f-collision.t), 0.0f, 1.0f);
if (shift-current > maximum_shift)
    maximum_shift = shift-current;
vehicle->vehicle.unknown44c[i] =
    quantize_real_to_byte_lower_bound(0.0f, 1.0f, (shift+current)*0.5f);
```

then plays an impulse sound when `maximum_shift > 0.3f`, scaled by
`PIN((maximum_shift-0.3f)*(1.0f/0.6f), 0.0f, 1.0f)`.

Structures recovered: `vehicle_animations` elements are **`0x74` bytes with a
`tag_block` at `+0x68`** — *not* `animation_graph_unit_seat`, which is `0x64`;
the suspension entries in that block are `0x14` bytes
(`short mass_point_index; short animation_index;` then two reals); and
`physics_mass_point_definition` has a `real_vector3d` normal at `+0x50`.

The `tag_get` of the `0xb4` animation element has its result discarded — the
third instance of the dead-call pattern in this object.

**The 32-byte residual is register allocation, and it is not source-reachable.**
January allocates the `result` variable to `bl`, pushes `ebx` at entry, and
therefore shares two epilogues at `0x2ff` and `0x308`; we constant-fold `result`
(it is provably always `FALSE`) and emit a local epilogue at each early return.
Confirmed by tracing `bl`: it is zeroed at `0x12` and **clobbered at `0x68`**, so
it is live only on the two early-exit paths — exactly the shape written. Tried
without effect: dropping the variable for plain `return FALSE`, and routing every
exit through the variable. As with `code_001a5b40`, ours is the shorter code and
January is the one leaving a register tied up.

## 8. `code_001a7ac0` — ground-vehicle drive (912 B / 34 rel vs 928 B / 34 rel)

Written. Relocation count matched on the first build; 16 bytes short, so **not
credited**. It is the densest function in the object outside `vehicle_update` —
roughly fifty FPU operations and six matrix helpers.

Entry guard, which is why it is the only static that calls another one:

```c
if (TEST_FLAG(vehicle->vehicle.flags, 1))       /* hovering */
{
    csmemset(mass_points, 0,
        physics->mass_points.count*sizeof(struct vehicle_mass_point_state));
    code_001a6710(vehicle_index);
    return;
}
```

Then, with `throttle = PIN(+0x42c, 0, definition->+0x2f8) / definition->+0x2f8`:

```c
factor = TEST_FLAG(flags,2) ? 0.25f : TEST_FLAG(flags,3) ? 1.0f : 0.75f;
vehicle->vehicle.unknown444 += PIN((1.0f-throttle*throttle)*factor*seat_power[0]
    - vehicle->vehicle.unknown444, -0.05f, 0.05f);
vehicle->vehicle.unknown448 = throttle*throttle*seat_power[0];
```

`throttle*throttle` stays live on the x87 stack across the whole `+0x444` update
and is reused for `+0x448` — worth knowing, because it means the two statements
must be in that order.

It then builds a ground-plane basis from `unit.desired_facing_vector`:

```c
v.i = -(facing.i*facing.k);  v.j = -(facing.j*facing.k);
v.k = 1.0f-facing.k*facing.k;
if (normalize3d(&v)==0.0f)  { v.i = 1.0f; v.j = 0.0f; v.k = 0.0f; }
```

computes a drive force along `A*up + B*forward` where `B` involves
`(+0x42c - dot_product3d(velocity, forward)) * throttle2 * mass * 0.05f` and `A`
is `(fabs(dot/max)*1.05f + (+0x444)*1.3f) * global_gravity * mass`, yaws the basis
by `(velocity.j*facing.i - velocity.i*facing.j) * (_pi/2) / fabs(max)`, and then
runs the rotation through

```
matrix4x3_rotation_from_vectors(&m1, &forward, &up);
matrix4x3_rotation_from_vectors(&m2, &facing, &v);
matrix4x3_inverse(&m2, &m2);
matrix4x3_multiply(&m1, &m2, &m3);
matrix4x3_rotation_to_quaternion(&m3, &q);
quaternion_to_angle_and_vector(&q, &angle, &axis);
```

to produce an angular impulse `axis * (angle/30) * radius*radius*mass*0.05f`
minus the current angular velocity, finally scaling both the force and the torque
by `seat_power[0]` and calling
`physics_update(vehicle_index, NULL, mass_points, &torque, &force)` followed by
`code_001a6710(vehicle_index)`.

`vehicle_definition +0x2f8` is a `real` (it was inside `unknown2f8[0x18]`).

**Residual: two spill slots.** January copies `translational_velocity.j` and
`.i` into `[ebp-0x14]` and `[ebp-0x18]` after the dot product and reuses them for
the yaw term; we re-read both from the object, which is cheaper and 8 bytes
shorter, and the frame comes out `0xf0` against `0xf8`. The rest of the 16 bytes
is the `csmemset` stack cleanup, which January issues before the
`code_001a6710` call while we merge it into one `add esp,0x10`.

Tried and rejected: binding `throttle*throttle` to a local. January does square it
before the factor branch (`fld st(0) / fmulp st(1)` straight after the `fdiv`), so
this looked like the repeated-subexpression rule, but the bound form goes the
*wrong* way — 912 to 896. Left inline.

## 9. Decoded but not yet written## 8. Decoded but not yet written

* **`_code_001a8800`** dispatches on `physics->radius > 0.0f` into
  `code_001a6290` or `code_001a5e50`, then calls `code_001a6910` on both paths.
  It needs those three written first.
* **`_code_001a6590`** (size- and relocation-exact) damps both velocities by
  `0.835f`, integrates position, builds a rotation from the angular velocity via
  `matrix4x3_rotation_from_axis_and_angle`, zeroes both velocities when the
  `+0x426` counter reaches zero, and calls `object_set_position`. Its two residual
  instruction pairs are the same float-operand canonicalisation as §4.

Still needed by the cluster: the mass-point runtime array (stride `0x130`) and the
physics mass-point *definition* (stride `0x80`, `real_point3d` at `+0x38`, already
modelled here as `struct physics_mass_point_definition`).

> Note: `ninja` cannot launch `xbox/bin/vc7/CL.Exe` in this checkout
> (`CreateProcess failed`); pre-existing, reproduces in `campaign-integration`.
> Verification used CL directly with the exact cflags ninja generates.

## 10. 27/39 -> 31/39 (7,712 bytes, 44.8%)

Four functions became byte-exact. Every edit below was re-tested by reverting it
individually from the finished state; all are load-bearing, so none is an
incidental change carried along with a real fix.

**`vehicle_export_function_values` (1,312 B).** Five independent edits, all
required together — the function only flips once the last one lands, and
reverting any one of the five breaks it again:

* `MAX(fabs(x)/s, fabs(y)/s)` without the `(real)` casts on the operands.
* `_velocity_moving` and `_velocity_sliding` are ternaries yielding `0.0f`, not
  `if` statements guarding an assignment. Same value, different codegen.
* `_speed_blend` binds `dot_speed`, `forward_value` and `blend` to three locals
  and computes `blend` *last*, after the two it multiplies against.
* `_boost` splits the division into its own statement and reassigns `result`.

**`vehicle_moving_near_any_player` (384 B).** The loop is a `do { ... } while`
with the iterator call and a null test inside the body, not a `while` with the
call in the condition. This is the loop inversion the extra relocation was
pointing at in §6 — January tests the pointer twice because the source does.

**`code_001a6590` (384 B).** Two edits, both required: bind
`&vehicle->object.position` to a local pointer and read the position through it,
and give `axis` its own block scope so its slot is reused. The pointer local is
what the earlier `volatile` proposals were reaching for; it gets the same operand
order legitimately, because the addressing mode changes rather than the meaning.

**`code_001a6dc0` (800 B) — byte-exact; shape defensible, cause unexplained.**

January does not call `point_from_line3d` at this site — `_code_001a6dc0` has no
relocation to it — so it is expanded. But our header's `__inline` does not
reproduce the expansion, and the header is not at fault: the same `__inline` is
inlined correctly at the `vehicle_find_pathfinding_surface_index` call site,
which is byte-exact. What does reproduce it is expanding the body at the call
site with all four parameters bound to locals first.

Swept, all non-matching: calling the header inline; longhand `start.x =
(normal.i*offset)+point.x` with and without parentheses; binding only the source
pointers; binding only the scalar; binding everything except the scalar; calling
through prebound pointers; and a file-local `static` helper in both `void` and
pointer-returning forms — including the exact shape that worked for
`vehicle_dot_product3d` in §3, which does not work here.

Only the full four-way materialization matches. Names and declaration order are
both inert — `p`/`v`/`t`/`r`, `origin`/`direction`/`distance`/`result`, and
`start_point`/`suspension_point`/`surface_normal`/`distance` are byte-identical,
in either result-first or source-first order. What is load-bearing is that all
four operands are named and that the aliases die at the block boundary.

The committed form therefore uses geometric names. `start` here is the origin of
a suspension collision ray, not the object's position, so naming it after the
object would be misleading; the §6 `object_position` rule does not transfer to
this site. Four named locals in a tight block for a geometry calculation is
ordinary game-source C, so the earlier reading of this as a mere placeholder was
too harsh — the spelling stands on its own.

What remains unexplained is *why* the header `__inline` does not produce it while
producing correct code elsewhere in the same file. The likely cause is a
difference between our `real_math.h` and January's that only shows under this
call's register pressure. Treat the shape as settled and the header discrepancy
as the open question.

## 31/39 -> 32/39 (8,480 bytes, 49.2%)

Two union recoveries unlocked this round, both checked against the rest of the
tree (objects 121/121, physics_variables 7/7, matrix_math unchanged):

* `real_vector3d` gains `struct { real_vector2d ij; real unused_k; }`.
* `vehicle_powered_mass_point_state` gains a `real_quaternion` view over the
  contiguous `unknown1c`/`unknown24` pair. They span sixteen bytes from 0x1c and
  the value written is (0,0,0,1), so the field is a quaternion and the two
  `set_real_vector2d` calls were spelling an identity.

**`code_001a6290` (768 B) — byte-exact.** The change that closed it is binding
the horizontal part of the velocity to a `real_vector2d` local through the new
`ij` view and computing the heading from that. Alongside: the single `angle`
splits into a `yaw` for the heading and a block-scoped `angle` for the rotation,
`scale_vector3d` moves inside that block, `perpendicular.i` drops its
parentheses, and `lift` groups the mass with the `fabs` rather than with gravity.

**`code_001a5b40` (784 B) — size and relocations now match; 21 bytes residual.**
Was 768 against 784. The `sign` chain became a nested ternary, which puts the
`sign = 0` case out of line where January has it and fixed every branch
displacement in the second half. `angle *= maximum_angle` splits out so it lands
after `state[0].unknownc`. File-local cross and triple products, plain `static`
rather than `__forceinline`, replace the header inlines at their call sites, and
the forward/angular dot product swaps operands through the existing
`vehicle_dot_product3d`.

`zero` must be three separate statements: chaining evaluates right to left and
stores j, k, i where January stores i, j, k ascending. Rejected: chained in four
orders, six placements through the `state[0]` block, `set_real_vector3d`
(41 bytes), and `zero = *global_zero_vector3d` which adds a 26th relocation.

**`code_001a5e50` (1088 B) — 70 bytes residual down to 30.** Reusing
`desired.left` as scratch for the thrust vector before the cross product
overwrites it, with a separate `thrust` local for the scaled result, plus the
quaternion assignment replacing the `*(real_quaternion *)&...` cast.

Reusing an existing local instead of adding `thrust` costs 16 bytes, so the extra
local is real. What is left is a stack-slot *permutation* -- same instructions,
different `[ebp-N]` -- e.g. January passes `eax=[ebp-0x10], ecx=[ebp-0x28]` where
we have the two swapped.

**Declaration order is inert, confirmed hard.** Eight orderings of the five
vector locals in `code_001a5e50` all produce byte-identical output, on top of the
five measured earlier. Hoisting either block-scoped local out of its brace costs
16 bytes, so the scoping is right too. Whatever drives MSVC's slot assignment
here, it is not the order the locals are written in.

## 32/39 -> 33/39 (9,264 bytes, 53.8%) — `code_001a5b40` byte-exact

Four changes, all four verified load-bearing by reverting each from the finished
state. Worth recording that the intermediate measurements got *worse* before the
last one closed it -- 21 differing bytes to 53 to 49 to 311 to 0 -- so this set
only makes sense applied together. Judging any one of them on its own would have
rejected it.

* The cross product reads through `b->n` and `a->n` with array indices rather
  than the `i`/`j`/`k` members and pointer aliases.
* A **separate** dot product helper, `vehicle_forward_dot_product3d`, also
  array-indexed. It must not replace `vehicle_dot_product3d`: that one is shared
  with `vehicle_preprocess_node_orientations`, which is byte-exact *because* of
  its current body, and rewriting it would trade one match for another.
* The triple product is reshaped -- parameters named for what they are
  (velocity, forward, normal), array indices, and `return` directly out of the
  inner block instead of through a `result` local -- and its call site swaps the
  first two arguments.
* `state[0]`'s rotation is written through a new `real unknown1c_values[4]` view
  on the union, in the order 0, 2, 1, 3, replacing the two `set_real_vector2d`
  calls.
* `point_from_line3d` is expanded at the call site against a
  `real const *forward = vehicle->object.forward.n` local. Same situation as
  `code_001a6dc0` in §10: January does not call the header inline here.

`__forceinline` was not used anywhere; plain `static` helpers give the same
result, as they did for `vehicle_dot_product3d` originally.

## 33/39 -> 34/39 (9,776/17,232 bytes, 56.7%)

External checkpoint, verified here: their C rebuilt with this repository's XDK CL
and compared with `tools/coff_compare.py`. **34/39 exact, 9,776/17,232 bytes.**
`code_001a6710` (512 B) closed.

**`__forceinline` removed on adoption.** The package declared its four local
helpers `static __forceinline`, which is on the prohibited list. Rebuilt with
plain `static` and the result is identical -- 34/39, 9,776 bytes, same five
functions outstanding -- so the qualifier was doing nothing. This is the third
time a contributed patch has carried `__forceinline` that measures neutral; plain
`static` has been sufficient every time.

Also recovered in this checkpoint: `physics_mass_point_definition.powered_mass_point_index`
at offset 0x20, with the following offsets preserved.

### `code_001a7e60` reconstructed but not exact

Was half-written (1,152 of 2,464). Now a complete reconstruction at 2,448 bytes
against 2,464 with **91/91 relocations**, so the call graph is right; 2,012 raw
bytes still differ. Counted as reconstruction progress, not a match. Its call was
added to the `vehicle_update` scaffold so VC7 emits the file-static body, which
is why that scaffold grew.

The package also spells two unsigned byte tests (`unknown42b>0`, `unknown428>0`)
in the form that produces January's `jbe` idiom.

### Remaining: 5 functions, 7,456 bytes

| bytes | relocs (Jan vs ours) | function | note |
|--:|---|---|---|
| 2464 | 91 vs 91 | `_code_001a7e60` | reconstructed, 2,012 bytes differ |
| 2320 | 98 vs — | `_vehicle_update` | scaffold only |
| 1088 | 32 vs 32 | `_code_001a5e50` | 30-byte slot permutation |
| 928 | 34 vs 34 | `_code_001a7ac0` | tie |
| 656 | 22 vs 22 | `_code_001a6910` | tie |

Three of the five have exact size *and* exact relocation counts. Try the EAX
return-value rule from `docs/house_rules.md` §6 on those, sweeping signature
factors together rather than singly -- that is what closed both matrix_math
residuals after each had been written off as an unreachable permutation.

## Residual triage: the three size-and-relocation-exact functions

No new match this pass. The EAX return-value sweep from `docs/house_rules.md` §6
does not apply to any of the three -- all are `static void` with no output
parameter -- so the recommendation in the previous section is closed out
negative. What each one actually differs by is recorded below so the next
attempt starts from the diff rather than re-deriving it.

### `code_001a6910` (656 B) -- two instructions, store order

Everything matches except the order of two independent stores after the
`object_get_marker_by_name` call:

```
        call    object_get_marker_by_name
        movsx   eax, ax
        add     esp, 0x10
        test    eax, eax
Jan:    mov     dword ptr [ebp-4], 0        ; marker_index = 0
        mov     dword ptr [ebp-0x24], eax   ; marker_count  = result
ours:   mov     dword ptr [ebp-0x24], eax
        mov     dword ptr [ebp-4], 0
```

`[ebp-4]` is confirmed to be `marker_index`: the loop tail is
`mov eax,[ebp-4] / inc eax / mov [ebp-4],eax / movsx eax,ax / cmp eax,ecx / jl`,
i.e. a `short` counter held in a 4-byte slot. The current `short marker_index`
and `long marker_count` types are therefore both correct.

January wants the zero-store **between** the call and the count-store. Four
source forms were measured; none produces that placement:

| form | result |
|---|---|
| `long marker_count = call(); short marker_index;` + `for (marker_index = 0; ...)` | 656/656, the 2-instruction swap above (current committed state -- the best) |
| declaration order swapped (`marker_index` declared first) | identical, inert |
| `short marker_index = 0;` at declaration + `for (; ...)` | zero-store sinks **before** the call; 5 instructions differ |
| `long marker_index` (either form) | frame shrinks 0x7b4 -> 0x7b0, 640 B; the `short` slot is load-bearing |

Do not re-try those four. The remaining idea is that the zero-store placement is
a consequence of something else in the enclosing block, not of how the counter
is declared.

### `code_001a7ac0` (928 B) -- one extra 4-byte local

The concrete signal here is the frame, not a permutation:

* `sub esp, 0xfc` against January's `sub esp, 0xf8` -- **we allocate one extra
  4-byte slot**, and one saved register spills to `[ebp-0x38]` where January
  uses `[ebp-0x14]`.
* At the tail, January cleans the previous call separately
  (`mov eax,[ebp+8] / add esp,0xc / push eax / call / add esp,4`) where we defer
  and combine (`push edx / call / add esp,0x10`).

Eighteen locals are declared at the top of the function (three `real_matrix4x3`,
one quaternion, six `real_vector3d`, eight `real`); their declared sizes total
264 bytes against a 248-byte frame, so VC7 is already overlapping several. One
of the eight `real`s has a live range that overlaps where January's does not --
the dying-local rule in §6. Finding which one is the next step; the extra slot,
not the register naming, is what to chase.

### `code_001a5e50` (1088 B) -- pure slot permutation, mapping recovered

Not a codegen tie: the locals are simply at different offsets. The full mapping
(January -> ours) is

| January | ours | what |
|---|---|---|
| -0x4c/-0x48/-0x44 | -0x40/-0x3c/-0x38 | a `real_vector3d` |
| -0x40/-0x3c/-0x38 | -0x1c/-0x18/-0x14 | a second `real_vector3d` |
| -0x24 | -0x10 | a `real` |
| -0x10 | -0x24 | a `real` (swapped with the above) |
| -0x54/-0x50 | -0x48/-0x44 | compiler temporaries |

January's two vectors are **adjacent** (-0x4c and -0x40, 12 bytes apart); ours
are 36 bytes apart, so we place other vectors between them. Two of the four
`real`s (`throttle`, `average`, `spin`, `maximum`) are transposed. This is a
declaration-order problem in the `powered_mass_points.count==2` block and should
be reachable by permuting those declarations rather than by any codegen lever.
One FPU-stack difference remains at the end (`fld st(0)/fmul st(1)` against
`fld st(2)/fmulp st(3)`) which may well fall out once the slots line up.

### `code_001a5e50` follow-up -- the declaration-order hypothesis is wrong

**Correction to the section above.** I described the slot permutation as "a
declaration-order problem ... reachable by permuting those declarations". That is
disproven. Eight orderings of the `powered_mass_points.count==2` locals were
built and measured, including splitting `real_matrix3x3 actual, desired,
difference;` into separate declarations and moving `desired` to the end:

```
M,axis,thrust,scaled,force,torque                                  71 diffs
M,force,axis,thrust,scaled,torque                                  71
M,torque,axis,thrust,scaled,force                                  71
m:difference,m:actual,force,axis,thrust,scaled,torque,m:desired    71
m:actual,m:difference,force,axis,scaled,thrust,torque,m:desired    71
m:difference,m:actual,m:desired,force,axis,scaled,thrust,torque    71
m:actual,m:desired,m:difference,torque,axis,scaled,thrust,force    71
m:actual,m:desired,m:difference,force,torque,axis,scaled,thrust    71
```

Every ordering produces **byte-identical output**. VC7 assigns frame slots by
live range and first use, not by declaration order, so no permutation of these
declarations can move a slot. Do not sweep declarations again on this or any
other residual in this object -- it is a dead lever.

(The 71 above is against a modified body, not the committed one; see next.)

### `axis`/`scaled` instead of `desired.left` -- also wrong

The address-taken mapping shows January passing `[ebp-0x40]` to
`quaternion_to_angle_and_vector` where we pass `[ebp-0x1c]`, and `[ebp-0x1c]` is
exactly `desired.left` (`desired` sits at `[ebp-0x28]`, `.left` at `+0xc`). That
suggested January writes the rotation axis into a standalone vector, and the
byte-exact sibling `code_001a6290` does use an `axis`/`scaled` pair for the same
shape. Measured:

| body | masked diffs |
|---|--:|
| committed (`&desired.left` for both) | **29** |
| `&axis` for the quaternion, `&thrust` for the scale | frame grows 0xa4 -> 0xb0, 1104 B |
| `&axis` for the quaternion, `&scaled` for the scale | 1088 B but **71** |

The committed form is the best known. `&desired.left` is right; the standalone
vector is not the difference. Note this also means the `[ebp-0x40]` slot January
passes to `quaternion_to_angle_and_vector` is *`desired.left` living at a
different offset*, not a different variable -- i.e. the whole `desired` matrix is
placed elsewhere, which is a consequence of the permutation rather than a clue to
its cause.

**Where that leaves it.** 29 masked bytes, all of them slot numbers plus one
FPU-stack pair at the tail. Both source-level hypotheses are now eliminated, and
the remaining candidates are properties that change a *live range*: the point at
which `force`/`magic_force` is first written, whether `thrust` exists at all, and
the placement of the inner `{ real_quaternion rotation; real angle; }` block.
Those change what overlaps what, which is the only thing VC7 is responding to.

### `code_001a5e50` residual 30 -> 22 bytes: the `magic_force` alias

`real_vector3d *magic_force = &force;` was an **initialiser at the declaration**
-- house rules §6, the dying-local rule -- and it was extending `force`'s live
range to the top of the block. Removing the alias and passing `&force` directly
to both `scale_vector3d` and `physics_update` drops the residual from 29 masked
bytes to 22, and removes the FPU-stack pair at the tail (`fld st(0)/fmul st(1)`
against `fld st(2)/fmulp st(3)`) that the previous section listed as a separate
problem. It was the same cause.

The alias was introduced when this function was first written and had no other
purpose; nothing else referenced it.

**Re-swept on top of the fix, because factors interact.** The eight
quaternion-output/scale-output vector combinations were measured again with the
alias gone:

| quaternion out | scale out | result |
|---|---|--:|
| `desired.left` | `thrust` | **22** (kept) |
| `desired.left` | `scaled` | 31 |
| `axis` | `scaled` | 64 |
| `axis` / `scaled` / `thrust` (other pairings) | | frame grows to 1104 B |

So `&desired.left` is confirmed correct after all, and the `[ebp-0x40]` January
passes to `quaternion_to_angle_and_vector` is `desired.left` sitting at a
different offset -- a consequence of the remaining permutation, not its cause.

**Also ruled out: hand-expanding `cross_product3d`.** January's inlined cross
product computes the k component as `forward.j*up.i - up.j*forward.i` -- operands
reversed in the first product only, where i and j use the standard `a.x*b.y`
order. That asymmetry looked like a hand-expansion at the call site (as with
`point_from_line3d` in §10). Writing it out with exactly that operand order
explodes the residual to **424 bytes**. Keep the `cross_product3d` call.

**Remaining: 22 bytes**, entirely slot numbers -- `force` at `[ebp-0x4c]` against
our `[ebp-0x40]`, and the `desired` block one vector lower to match. The two
levers that moved anything so far were both live-range properties, so that is
where to keep looking.

### `code_001a7ac0` -- the extra slot is not a named `real`

The frame is `sub esp,0xfc` against January's `0xf8`. Accounting for the declared
locals -- three `real_matrix4x3` (144), one quaternion (16), six `real_vector3d`
(72) = 232 bytes -- leaves January 16 bytes for its eight `real`s (four slots)
and us 20 (five). So VC7 coalesces one pair for January that it does not for us.

Twelve source-level merges of provably disjoint-lifetime reals were built and
measured (`throttle<-dot`, `factor<-dot`, `dot<-yaw`, `drive<-yaw`, `lift<-yaw`,
`yaw<-angle`, `yaw<-scale`, `dot<-angle`, `dot<-scale`, `throttle<-yaw`,
`factor<-yaw`, `throttle<-angle`). **Every one leaves the frame at 0xfc.** VC7 is
already coalescing those intervals; forcing the merge in source changes nothing.
The fifth slot is therefore a compiler temporary or a spill, not one of the eight
named reals, and merging declarations is a dead lever here too.

Note that a 4-byte frame difference shifts every `ebp`-relative displacement, so
the masked byte count for this function (752 of 928) is not a useful progress
metric until the frame matches. Chase `sub esp` first, and only then look at the
byte count.

### `code_001a5e50`: the 22 bytes are one root cause, and four levers are exhausted

The remaining 22 bytes are **not** several independent problems. Reading the
slot maps side by side:

```
January  quaternion(-0x5c..-0x4c)  force(-0x4c)  X(-0x40)      torque(-0x34)  desired(-0x28)  angle(-4)
ours     quaternion(-0x5c..-0x4c)  thrust(-0x4c) force(-0x40)  torque(-0x34)  desired(-0x28)  angle(-4)
```

Both frames are identical and both pack perfectly; the only difference is that
January gives the first post-quaternion slot to `force` and we give it to
`thrust`. Everything else in the diff follows from that:

* the three `fstp` at `[-0x4c/-0x48/-0x44]` vs `[-0x40/-0x3c/-0x38]` -- `force`;
* `lea ecx,[ebp-0x4c]` vs `[ebp-0x40]` at the `physics_update` call -- `force`;
* the two scratch slots (`[-0x54/-0x50]` vs `[-0x48/-0x44]`) -- January reuses
  the **dead quaternion's** tail, we reuse the dead `thrust`;
* the `fld/fmul` operand order in the cross product's k component -- both builds
  use the same `cross_product3d` header inline (`a->i*b->j - a->j*b->i`), so a
  commutative `fld X;fmul Y` vs `fld Y;fmul X` here is induced by the surrounding
  stack/register state, not by the source. It should fall out with the slot fix.

VC7 assigns these slots in order of first address-taken, and in our source
`&thrust` is taken first (`scale_vector3d(&axis, physics->mass, &thrust)`) while
`&force` is taken one statement later.

**Levers swept and exhausted** (~90 builds; none beats 22):

1. *Declaration order* -- 8 orderings, byte-identical output. Inert.
2. *Vector routing* -- the full matrix of {early-scale destination} x {quaternion
   output} x {late-scale destination} over `thrust`/`scaled`/`axis`/`force`/
   `desired.left`, ~60 builds. `early=thrust, q=desired.left, dest=thrust`
   (the committed form) and `early=scaled, q=desired.left, dest=scaled` tie at
   **22**; every other routing is 24 or worse, and routings that make the late
   scale write in place cost 16 bytes of frame. `q=desired.left` dominates
   universally -- the quaternion output is confirmed correct.
3. *Block scope* -- hoisting `rotation` alone is inert (22); hoisting `angle`, or
   both, adds a slot (1104 B).
4. *The `magic_force` alias* -- fixed, 29 -> 22 (previous section).

What has not been tried: anything that makes `&force` the first address taken in
the block without adding a vector. Routing `axis -> force` directly (dropping the
early `thrust` hop) does exactly that but measures 39, because it also removes a
live range VC7 was using. The shape that both takes `&force` first *and* keeps
three vector live ranges is the thing still missing.

### `code_001a6910`: the two-store tie is scheduler-only -- ten forms measured

Restating the difference precisely. Both builds emit the same four instructions
between the call and the loop guard; only the middle two are transposed:

```
        call    object_get_marker_by_name
        movsx   eax, ax
        add     esp, 0x10
        test    eax, eax                    ; flags for the loop guard
Jan:    mov     dword ptr [ebp-4], 0        ; marker_index = 0
        mov     dword ptr [ebp-0x24], eax   ; marker_count  = result
ours:   mov     dword ptr [ebp-0x24], eax
        mov     dword ptr [ebp-4], 0
        jle     <skip loop>
```

Neither `mov` touches flags, so both orders are legal after the `test`; VC7 picks
one. Ten source forms have now been measured, and they collapse into exactly two
outcomes -- there is no form that lands the zero-store *between* the call and the
count-store:

| form | result |
|---|--:|
| `long c = call(); short i;` + `for (i = 0; ...)` (committed) | **8** |
| `c = call();` then `for (i = 0; ...)` | 8 |
| `c = call(); i = 0;` then `for (; ...)` | 8 |
| `for (c = call(), i = 0; ...)` (comma, count first) | 8 |
| `i = 0; c = call();` then `for (; ...)` | 18 |
| `for (i = 0, c = call(); ...)` (comma, index first) | 18 |
| `short i = 0;` at declaration + `for (; ...)` | 18 (zero-store sinks *before* the call) |
| declaration order swapped | 8, byte-identical -- inert |
| `long marker_index` | frame 0x7b4 -> 0x7b0, 640 B |
| `short marker_count` | 640 B |

The rule VC7 is following is plain: **whichever assignment the source performs
first, that store is emitted first** -- and when the index is assigned first its
store sinks to *before* the call, never between. January's placement is not
expressible from either ordering. `marker_count` must be spilled (it is read by
the loop condition across calls in the body) and `[ebp-4]` is confirmed to be the
`short` counter in a 4-byte slot, so the slots themselves are already correct.

Recording this as a scheduler tie, in the same class as the register-naming
coin-flips elsewhere in this repository. 8 masked bytes, everything else exact.

### `code_001a7ac0`: the frame difference is a packing hole, not an extra variable

Extracting every `ebp`-relative displacement from both builds (22 distinct slots
for January, 23 for us) shows the two frames are **not** simply "ours has one
more local". They contain the same objects in the same order; what differs is
where the one 4-byte alignment hole lands.

```
ours  [3 x matrix4x3 @-0xfc,-0xc8,-0x94]  quat@-0x60  vec@-0x50  HOLE@-0x44  ...  force@-0x24,-0x20,-0x1c  [6 reals -0x18..-0x4]
Jan   [3 x matrix4x3 @-0xf8,-0xc4,-0x90]  quat@-0x5c  vec@-0x4c  ...          force@-0x28,-0x24,-0x20  HOLE@-0x1c  [6 reals -0x18..-0x4]
```

Both builds space the three matrices 0x34 apart and both end up with exactly one
unused 4-byte slot and the same six near-`ebp` real slots. In our layout the hole
sits at `-0x44`, immediately after the 12-byte vector at `-0x50`; in January's it
sits at `-0x1c`, immediately after `force`. Everything above the hole is 4 bytes
more negative in ours and everything below it is 4 bytes less negative -- which is
exactly what the instruction diff shows, and why the masked byte count (752 of
928) is dominated by displacement bytes rather than by real disagreement.

So the lever is not "remove a local". It is whatever decides that our 12-byte
vector lands on an offset that forces a pad, and January's does not. Confirmed
not responsible:

* the eight named `real`s -- twelve disjoint-lifetime merges, all still 0xfc;
* `(real)fabs(...)` -- the byte-exact `code_001a6290` in this same file uses the
  identical `(real)fabs(x)` idiom twice, so it introduces no stray temporary.

`code_001a6290` is the reference to work from: it is byte-exact, structurally the
same shape (lift/drive, three matrices, quaternion, torque, `physics_update`),
and it declares its first two vectors **with initialisers** (`real_vector3d
facing = vehicle->unit.desired_facing_vector;`) where `code_001a7ac0` declares
them bare and assigns in the body. That difference in where a vector's live range
starts is the most likely candidate for the packing change and has not yet been
measured.

**Vector-declaration sweep on `code_001a7ac0` -- also inert.** Four more shapes
measured, none moves `sub esp` off 0xfc:

| shape | result |
|---|---|
| baseline | 752 diffs, frame 0xfc |
| `facing` initialised at its declaration (the `code_001a6290` style) | 782, 0xfc |
| `facing` declared first *and* initialised | 782, 0xfc |
| all six vectors declared before the three matrices | 752, 0xfc (byte-identical) |

This is the third independent confirmation in this object that **declaration
order is inert** for VC7 frame layout, and it closes the "initialiser on the
first vector" hypothesis raised in the previous section: copying
`code_001a6290`'s style makes `code_001a7ac0` worse, not better.

Levers now eliminated for this function: named-`real` merging (12 builds),
`(real)fabs`, vector declaration order and initialisers (4 builds), matrix/vector
grouping. The packing hole at `-0x44` persists through all of them, so whatever
places that 12-byte vector on a padded offset is not expressible through the
declarations at all -- consistent with the `code_001a5e50` finding that VC7 lays
out by live range and first use only.

## `code_001a7e60`: it is not a permutation, it is ~31% wrong code

The previous section recorded this as "reconstructed but not exact, 2,012 raw
bytes differ". Measuring it properly changes the diagnosis.

**Baseline, measured here: 1,996 masked bytes differ** (prefix-aligned, relocations
zeroed), JAN 2,464 B / ours 2,448 B, 775 vs 769 instructions, frame `0x94` vs our
`0x9c`. Quote 1,996 rather than 2,012 -- the older figure was measured a different
way and a "-15" against it is really a +1 regression (see below).

Wildcarding tells us what kind of problem this is:

| normalisation | multiset difference |
|---|--:|
| raw | 502 of 775 |
| `ebp` slots wildcarded | 280 |
| slots **and** registers wildcarded | **242** |

For `code_001a7ac0` and `code_001a5e50` that last number collapses to nearly
zero, because those really are permutations. Here **242 instructions (31%) differ
in opcode and operand shape**, so no amount of slot or register work will close
it. `code_001a7e60` needs source reconstruction, not levers, and the 91/91
relocation match only proves the call graph is right -- it says nothing about the
arithmetic between the calls.

### The diverging regions

Aligning the two instruction streams (branch targets ignored, slots wildcarded)
gives 216 instructions inside runs of >= 6:

| JAN offset | size | what differs |
|---|--:|---|
| `0x35f`-`0x372` | 8 | a 2-component cross/dot term evaluated in a different component order |
| `0x3ee`-`0x404` | 10 | January has an extra `fchs` + two `fsub st(n)` reductions we lack |
| `0x40d`-`0x423` | 8 | ours collapses to `fsubp/fst` where January keeps a `fsubr`/`fmul const`/`fsubr st(1)` chain |
| `0x592`/`0x5b3` | 6+9 | same terms, different order; ours materialises through memory |
| `0x5e9`/`0x60a` | 73/70 | **largest.** The `TEST_FLAG(flags, 3)` block. January guards, then dots `[ebx]` against `object.forward` and **divides** by `definition->unknown2f8`; ours dots and **multiplies by a stack local**, with the last term's operands swapped |
| `0x7e7`-`0x819` | 16 | a `(1.0f - unknown444) * ...` accumulation January performs inline |

### Measured and rejected

`dot_product3d(&forward, &translational_velocity)` in the `TEST_FLAG(flags, 3)`
block is argued the wrong way round -- `code_001a7ac0` and `code_001a6290` both
write velocity-first, and January's emission loads `[ebx+8]` before
`[esi+0x20]`. Swapping it moves the raw multiset from 502 to 494 and the
slot-wildcarded from 280 to 272, **but the masked byte count goes 1996 -> 1997
and the fully-wildcarded count goes 242 -> 246.** Reverted. This is a good
reminder that the wildcarded counts are diagnostics, not objectives: only the
masked byte count and `section_infos_equal` decide anything.

Start with the `0x5e9`/`0x60a` region -- it is a third of the divergence, it is a
single well-delimited `if` block, and the `fdiv` against our `fmul` says the
source expression there is genuinely different rather than merely reordered.

### `code_001a7e60`: the `TEST_FLAG(flags, 3)` block -- PIN was evaluating its argument three times

`PIN(n,floor,ceiling)` is `((n)<(floor) ? (floor) : CEILING((n),(ceiling)))` and
`CEILING(n,c)` is `((n)>(c)?(c):(n))`, so **`n` is textually expanded three
times**. Our source passed the whole expression:

```c
real speed = PIN(dot_product3d(&vehicle->object.forward,
    &vehicle->object.translational_velocity)/definition->unknown2f8, 0.0f, 1.0f);
```

and VC7 duly emitted the dot product **three times** -- once per clamp branch --
plus a hoisted `1.0f/unknown2f8` reciprocal to share between them. January emits
it once and keeps the value in `st0` across both `fcom`s, replacing st0 in the
out-of-range arms. Binding the value first fixes it:

```c
real speed = dot_product3d(&vehicle->object.forward,
    &vehicle->object.translational_velocity)/definition->unknown2f8;
real_vector3d left;

speed = PIN(speed, 0.0f, 1.0f);
```

**Result: instructions inside diverging runs (>=6) fall from 216 to 80**, and the
two largest regions -- 73 ours-only at `0x5e9` and 70 January-only at `0x60a`,
together a third of the whole divergence -- disappear entirely. Structural
multiset difference drops 502 -> 464 raw, 280 -> 244 slots-wildcarded, 242 -> 224
slots+regs.

Two corrections that come out of this:

* **The "91/91 relocations" in the earlier section was a coincidence, not
  evidence.** Each redundant clamp arm carried its own constant relocation; with
  the duplication removed we sit at 90 against January's 91, so one referenced
  constant is still missing. Do not read a matching relocation count as "the call
  graph is right".
* Size moved 2,448 -> 2,400 against 2,464. That is the correct direction of
  travel even though the number got further away: the remaining diverging runs
  are now mostly *January-only* (10+8+19+9+16 = 62 instructions we do not emit),
  i.e. we are missing identifiable code rather than emitting wrong code.

The argument order `dot_product3d(&forward, &translational_velocity)` is
**confirmed correct** here -- January loads `[ebx+8]` (forward.k) against
`[esi+0x20]` (translational_velocity.k), forward first. The earlier suggestion to
swap it was wrong and measured +1.

### Remaining divergence in `code_001a7e60`: 80 instructions, 7 runs

| JAN offset | JAN | ours | note |
|---|--:|--:|---|
| `0x3ee` | 10 | 0 | `fchs` + two `fsub st(n)` reductions |
| `0x40d` | 8 | 2 | January keeps an `fsubr`/`fmul const`/`fsubr st(1)` chain |
| `0x592` | 0 | 6 | ours emits extra |
| `0x5b3` | 19 | 2 | largest remaining |
| `0x6c2` | 9 | 0 | |
| `0x6eb` | 0 | 12 | ours emits extra |
| `0x7e7` | 16 | 0 | `(1.0f - unknown444)` accumulation |

`code_001a7e60` still contains six more multi-evaluation macro call sites that
have not been checked for the same problem: `MIN(unknown42b*0.05f, 0.98f)`,
`PIN(desired-current, ...)`, two `PIN((real)fabs(pd_x)*sign_y+1.0f, 0.3f, 2.5f)`,
`PIN(1.0f-unknown428*(1.0f/30), ...)` and `PIN(ratio*maximum, ...)`. Note the fix
is **not** "always bind to a local" -- five of the seven remaining runs are
January emitting *more* than us, so in those places January is passing the
expression and we are the ones who bound it. Check each against the emission.

### The other PIN/MIN/MAX sites in `code_001a7e60` are already fine

The previous section flagged six more multi-evaluation macro call sites as
suspects. They are not. Each was toggled between "expression passed to the macro"
and "value bound to a local first", rebuilt and measured:

| site | runs>=3 | multiset (slots+regs) | size |
|---|--:|--:|--:|
| baseline | 143 | 108 | 2400 |
| `MIN(unknown42b*0.05f, 0.98f)` bound | 175 | 110 | 2400 |
| `PIN(desired-current, ...)` bound | 175 | **100** | 2400 |
| `PIN(ratio*maximum, ...)` bound | 171 | 114 | 2400 |

**The size never moves.** That is the decisive observation: unlike the
`dot_product3d(...)/unknown2f8` case, VC7 already common-subexpressions these
arguments, so binding them to a local changes nothing about how much code is
emitted -- only scheduling and register choice, which is why `runs` and `multiset`
disagree about whether each variant is "better". None is taken.

The lesson is narrower than "PIN triple-evaluates, so always bind": VC7's CSE
copes with a subtraction, a scalar multiply, or a field read appearing three
times. It did **not** cope with a fully inlined `dot_product3d` -- three `fld`/
`fmul`/`faddp` chains plus a division -- which is why that one site was worth
4% of the whole function. Look for macro arguments that expand to *large inline
expressions*, not for macro arguments generally.

So the remaining 80 instructions of divergence are in the arithmetic itself, not
in macro expansion. The `0x5b3` run (19 January instructions against our 2) is
the `left_scale`/`forward_scale`/`scale` torque accumulation and remains the
largest single target.

### `code_001a7e60`: the `0x5b3` torque accumulation -- two separate faults

January's shape, read off the emission:

```
    fmul [physics+0x54]              ; left_scale, left in st0 -- never stored
    fld left.i ; fmul st(1) ; fadd torque.i ; fstp torque.i
    fld left.j ; fmul st(1) ; fadd torque.j ; fstp torque.j
    fld left.k ; fmul st(1) ; fadd torque.k ; fstp torque.k
    fstp st(0)                       ; left_scale dies here
    fmul [physics+0x50] ; fchs       ; forward_scale, computed only NOW
    fld st(0) ; fmul [ebx]   ; fadd torque.i
    fld st(1) ; fmul [ebx+4] ; fadd torque.j ; fstp torque.j
    fxch st(1); fmul [ebx+8] ; fadd torque.k ; fstp torque.k
    fld 1.0f ; fsub [unknown444] ; fstp scale
    fmul scale ; fadd magic_torque.i ; fstp magic_torque.i
    ...
```

Our single fused expression
`torque.i += (left.i*left_scale + forward.i*forward_scale)*scale;` was wrong in
**two independent ways**, and only fixing both helps:

1. **The accumulation is split.** January adds the `left` contribution to
   `torque` for all three components, then adds the `forward` contribution in a
   second pass, and applies `scale` only when folding `torque` into
   `magic_torque` (`magic_torque.i += torque.i*scale`, not `torque.i += (...)*scale`).
2. **`forward_scale` must be assigned after the left pass, not initialised at its
   declaration.** With the initialiser, VC7 computes `forward_scale` immediately,
   which needs the x87 stack and **spills `left_scale` to `[ebp-4]`**, so all
   three left multiplies become `fmul [ebp-4]` reloads. January keeps
   `left_scale` in `st0` and uses `fmul st(1)`. Declaring `real forward_scale;`
   bare and assigning it between the two passes restores that.

Fault 1 alone measured *worse* on the run metric (80 -> 102) while barely moving
the multiset -- it is fault 2 that pays, and only once the passes are split.
Together:

| state | runs>=6 | multiset (slots+regs) | size |
|---|--:|--:|--:|
| before this section | 80 | 224 | 2400 |
| split accumulation only | 102 | 219 | 2400 |
| split **and** deferred `forward_scale` | **55** | **209** | 2416 |

The entire `0x595`/`0x5a0`/`0x5b3`/`0x5b8` cluster is gone. This is the third
time in this file that *where a value is assigned* -- not how the expression is
written -- turned out to be the lever (`magic_force` in `code_001a5e50`, PIN's
argument above, and now `forward_scale`).

**Remaining: 55 instructions in five runs** -- JAN `0x3ee` (10), `0x40d` (8 vs our
2), `0x6c2` (9), ours `0x6af` (12), JAN `0x7e7` (16). Size 2,416 against 2,464,
relocations 90 against 91.

### `code_001a7e60`: 216 -> 18 diverging instructions

Three more faults found after the PIN one, all the **same lever**: an initialiser
at a declaration pins a value's computation to that point, which costs the x87
stack and forces the previous scalar to spill to a slot. January computes each
scalar immediately before the loop that consumes it, uses it from `st0`
(`fmul st(1)`), pops it, and only then computes the next.

| fix | runs>=6 | multiset (slots+regs) | size |
|---|--:|--:|--:|
| after the PIN fix | 80 | 224 | 2400 |
| torque accumulation split + `forward_scale` deferred | 55 | 209 | 2416 |
| `up_scale` split + recomputed (no shared `scale` local) | 39 | 195 | 2432 |
| `lift` deferred past the torque accumulation | **18** | **194** | 2432 |

Relocations are back to **91/91** and size is **2,432 against 2,464**. Note the
relocation count returned to 91 only because January genuinely re-references the
constant when it recomputes the scale product -- the earlier 91/91 was a
coincidence of triple-evaluated clamp arms, this one is real.

The three fixes in source terms:

1. `torque` block -- split the `left` and `forward` passes, apply `scale` when
   folding into `magic_torque` rather than inside the component expression, and
   declare `real forward_scale;` bare, assigning it between the two passes.
2. `up_scale` block -- do **not** share a `scale` local between `axis_scale` and
   `up_scale`; January recomputes `(1.0f-unknown444)*mass*fade` for each, and the
   two accumulations are separate passes.
3. `torque`/`lift` block -- declare `real lift;` bare and assign it after the
   three `magic_torque` accumulations.

### Remaining: 18 instructions in two runs, both in the `pd_a`/`pd_b` block

```
JAN @0x3ee (10, we emit none)      fchs / fstp / fld / fsub st(2) / fstp /
                                   fld / fsub st(1) / fstp st(2) / fstp st(0) / fld
JAN @0x40d (8, we emit 2)          fsubr [slot] / fstp / fld / fmul 15.0f /
                                   fsubr st(1) / fstp / fstp st(0) / fld
```

Source is

```c
real pd_a = 15.0f*rate_forward-align_left;
real pd_b = -align_forward-15.0f*rate_left;
```

January uses `fsubr` (`st0 = mem - st0`) where we emit `fsub`, and reaches `pd_b`
via `faddp; fchs` -- the negation of a *sum of two products*, i.e. the
`dot_product2d` for `align_forward` inlined directly into the negation rather
than through the named local. It also performs two `fsub st(n)` reductions
against `[ebp-0x1c]`/`[ebp-0x18]` before the `pd` values are formed, which have no
counterpart in our source at all.

This needs the four `dot_product2d` results traced to their slots before guessing
at operand order -- the two runs are small but they are the *only* thing left, so
they are worth doing properly rather than by sweep.

### `pd_a`/`pd_b`: the slot trace, and why it is not a codegen difference

Traced. `edi` is `&vehicle->object.up.ij`; `[esi+0x3c]`/`[esi+0x40]` are
`angular_velocity.i`/`.j`; `[ebp-8]`/`[ebp-4]` and `[ebp-0x1c]`/`[ebp-0x18]` are
the two `real_vector2d` locals (`forward2d` and `left2d`). January's four inlined
`dot_product2d`s, in order:

| JAN | value | destination |
|---|---|---|
| `@3a5`-`@3ba` | `dot(forward2d, up.ij)` = `align_forward` | left in `st1` |
| `@3bc`-`@3c7` | `dot(left2d, up.ij)` = `align_left` | left in `st0` |
| `@3c9`-`@3dd` | `dot(left2d, angvel.ij)` = `rate_left` | `[ebp-0x10]` |
| `@3e0`-`@3f0` | `dot(forward2d, angvel.ij)`, **negated** (`fchs`) | `[ebp-0xc]` |

Then, and this is the part with no counterpart in our source:

```
@3cc/@3d5  mov [ebp-0x18],edx / mov [ebp-0x1c],ecx   ; level_torque = *global_zero_vector2d
@3f3       fld [ebp-0x1c] ; fsub st(2) ; fstp [ebp-0x1c]   ; [ebp-0x1c] = 0 - align_forward
@3fb       fld [ebp-0x18] ; fsub st(1) ; fstp st(2)        ;             0 - align_left
@404       fld [ebp-0x10](rate_left)   ; fmul 15.0f ; fsubr [ebp-0x1c] ; fstp [ebp-0x1c]
           => pd_b = (-align_forward) - 15.0f*rate_left
@413       fld [ebp-0xc](-rate_forward); fmul 15.0f ; fsubr st(1)      ; fstp [ebp-0x18]
           => pd_a = (-align_left) - (-15.0f*rate_forward)
                   = 15.0f*rate_forward - align_left
```

Both `pd` values are arithmetically **identical to ours** -- the formulas check
out. The difference is that January forms the two negations `-align_forward` and
`-align_left` by *subtracting the dot products from the freshly zeroed
`level_torque`*, and reuses `level_torque`'s two slots (`[ebp-0x1c]`,
`[ebp-0x18]`) to hold `pd_b` and `pd_a`. It also negates `rate_forward` at the
point of computation and subtracts, rather than multiplying and subtracting in
our order.

**This is not reachable by reordering our expressions.** Our source computes four
named `real`s and then two independent `pd` expressions; January threads the
zero-initialised `level_torque` through the negations. Reproducing it means
working out what January's source actually said about `level_torque` -- note that
our `level_torque` is later accumulated into (`level_torque.i += level_scale*pd_b`),
so if January really does `level_torque.i -= align_forward` first, our subsequent
arithmetic is wrong too, and that would be a **semantic** bug rather than a
codegen mismatch.

Do not guess at this one. The next step is to check `level_torque`'s later use
and its final consumer against January to establish whether the `-=` is real; the
18-instruction gap is small but it sits on top of a possible semantic error, and
a sweep that merely makes the bytes agree would bury it.

### Correction: there is no semantic bug in `level_torque`

The previous section warned that January might do `level_torque.i -= align_forward`,
which would make our later `level_torque.i += level_scale*pd_b` arithmetic wrong.
**That warning was mistaken.** Following January past the `pd` values settles it:

```
@4f0   fmul [0.0015514038f] ; fadd [ebp-0x54]      ; control_torque.i, NOT stored -- stays in st0
@53a   fld 1.0f ; fsub [esi+0x38] ; fmul [0.0038785094f]   ; level_scale (up.k at +0x38)
@549   fst [ebp-4]                                  ; level_scale kept
@54c   fmul [ebp-0x1c](pd_b) ; faddp st(1)          ; + control_torque.i still in st1
```

So January computes `torque_a = control_torque.i + level_scale*pd_b` directly,
with `control_torque.i` never leaving the x87 stack. There is no persistent
`level_torque` accumulator holding `-align_forward`: `level_torque` **is**
zero-initialised exactly as our source says, VC7 folds the `0 + x` in
`level_torque.i += level_scale*pd_b`, and the two zeroed slots are then recycled
to hold `pd_b`/`pd_a`. The `fsub` against those slots at `@3f3`/`@3fb` is VC7
opportunistically using a known-zero slot as the source of `0.0` to negate
`align_forward`/`align_left` -- a codegen artefact, not a different formula.

Our source is therefore semantically correct here, and `control_torque`
(`[ebp-0x54]`/`[ebp-0x50]`), `level_scale` (`[ebp-4]`) and `up.k` (`+0x38`) are
all confirmed.

**Two reorderings measured, both rejected** (baseline is 31 instructions in runs
>= 4, 18 in runs >= 6):

| variant | runs>=4 |
|---|--:|
| committed baseline | **31** |
| `level_torque` declared after the four dot products | 31 -- inert |
| ...and `pd_a`/`pd_b` written as `level_torque.j-align_left+...` to force the zero-slot negation | 39 -- worse, adds two new runs |

Writing the negation through `level_torque` explicitly is *not* how January gets
there; it falls out of VC7's own use of the zeroed slot. The remaining 18
instructions are the last two runs at `@0x3ee` and `@0x40d`, and they are a
codegen artefact of that zero-slot reuse rather than anything expressible in the
source -- which puts them in the same do-not-chase class as the ties in
`code_001a5e50` and `code_001a6910`.

## `vehicle_update`: started -- entry sequence and structural decode

Scaffold was 272 B / 16 relocations against January's 2,320 B / 98. Now
**384 B / 27 relocations**, and the entry sequence tracks January instruction for
instruction apart from the frame size and one register name.

### What went in

* `profile_enter(data_00316228)` / `profile_exit(data_00316228)` around the body.
  **`_data_00316228` is a `struct profile_section`** -- the manifest gives its size
  as `0x5f8`, which is exactly `sizeof(struct profile_section)` as established
  this session in `profile.h`. It is declared exactly like `objects_update`'s:
  `static struct profile_section data_00316228 = {"vehicle_update", NONE, TRUE};`
  and the `??_C@_0P@GMMIJBCG@vehicle_update?$AA@` literal in the manifest is its
  `name`.
* **It must be at file scope, not a function-local `static`.** As a local static
  MSVC mangles it to `?data_00316228@?1??vehicle_update@@9@9`; January's is the
  plain `_data_00316228`, so it belongs in the globals section.
* **`profile_enter` comes after the `vehicle`/`definition` locals**, not first:
  January calls `object_get_and_verify_type` and `tag_get('vehi')` at `@d`-`@2d`
  and only reaches `profile_enter_private` at `@4e`.

### Structure recovered from the relocations and call order

The 98 relocations decode as follows. **Eight of them are self-references to
`_vehicle_update` and form a jump table at `@0x8f0`** (the function is `0x910`
bytes, so the table is the last `0x20` = 8 entries), reached by
`jmp dword ptr [eax*4 + 0x8f0]` at `@0x57b`. That is a `switch` over the seven
`_vehicle_type_*` values already declared at the top of this file.

Call order:

| offset | call | note |
|---|---|---|
| `@8` | `__chkstk` | frame is 0x327c |
| `@16` | `object_get_and_verify_type` | `vehicle_get` |
| `@28` | `tag_get` | `'vehi'` pushed as `0x76656869` |
| `@4e` | `profile_enter_private` | |
| `@1e5`,`@24a` | `cross_product3d` x2 | |
| `@267` | `point_from_line3d` | |
| `@27b`,`@2e8` | `scale_vector3d` x2 | |
| `@2db` | `dot_product3d` | |
| `@335`,`@348`,`@36a`,`@46d` | `physics_variable_speed_update_seek` x4 | three `*0.0174532925f` (deg->rad) conversions at `@3a8`,`@3c7`,`@3ec` |
| `@40d` | `physics_variable_position_update_seek` | |
| `@4ff`-`@53e` | collision-user **push** | `match_assert(depth < 0x20)`, then `global_current_collision_users[depth] = 0x11`, `depth++` |
| `@57b` | **switch jump** | 8-entry table at `@0x8f0` |
| `@593`,`@5ae`,`@5c9`,`@5e4`,`@5ea`,`@602`,`@619` | `code_001a58e0`, `001a5a30`, `001a5b40`, `001a70e0`, `001a6dc0`, `001a6ba0`, `001a4fa0` | the seven case bodies |
| `@655`,`@660` | `global_structure_bsp_get` x2 | |
| `@6ee`-`@718` | collision-user **pop** | `match_assert(depth >= 1)`, `depth--` |
| `@72f`,`@73a` | `scenario_get_game_globals`, `tag_block_get_element_with_size` | |
| `@765`,`@777`,`@789` | `object_get_and_verify_type`, `damage_data_new`, `object_cause_damage` | |
| `@7b5` | `unit_update_animation` | |
| `@7fe` | `object_permute_region` | uses the `"~blur"` literal |
| `@82f`,`@852`,`@86e` | `code_001a7ac0`, `code_001a7e60`, `code_001a8800` | |
| `@88c` | `physics_update` | |
| `@8ab`,`@8b1` | `code_001a6590`, `code_001a6dc0` | `code_001a6dc0` is called **twice** in this function |
| `@8df` | `profile_exit_private` | |

Two `match_assert`s, both on `global_current_collision_user_depth`, with the two
long assert strings in the manifest supplying their exact text.

Next increment: the collision-user push/pop pair and the switch skeleton, since
both are well delimited and the switch is what gives the seven case bodies their
call sites.

### `vehicle_update`: collision-user bracket and switch skeleton

**384 B / 27 relocations -> 592 B / 49 relocations** against January's 2,320 / 98.
Halfway on relocations, and two sub-structures now match exactly:

* `global_current_collision_user_depth` / `global_current_collision_users`:
  **6 references, same as January**, and both `match_assert`s (2 `display_assert`
  references) are in place. The pattern is the house one already used in
  `collisions.c`, with the vehicle's own user id and line numbers:

  ```c
  match_assert("c:\halo\SOURCE\units\vehicles.c", 308,
      global_current_collision_user_depth<MAXIMUM_COLLISION_USER_STACK_DEPTH);
  global_current_collision_users[global_current_collision_user_depth++] = 17;
  ...
  match_assert("c:\halo\SOURCE\units\vehicles.c", 372,
      global_current_collision_user_depth>1);
  --global_current_collision_user_depth;
  ```

  Line numbers come straight off the `push 0x134` / `push 0x174` in January, the
  id `17` off `mov word ptr [users+edx*2], 0x11`, and the bound from
  `cmp word ptr [depth], 0x20` = `MAXIMUM_COLLISION_USER_STACK_DEPTH`.
  `#include "physics/collision_usage.h"` was missing and had to be added.

* The **switch over `definition->vehicle_type`** (a `short` at definition+0x2f4,
  which the existing `struct vehicle_definition` already declares) with the seven
  `_vehicle_type_*` cases dispatching to `code_001a58e0`, `5a30`, `5b40`, `70e0`,
  `6dc0`, `6ba0`, `4fa0` in that order. Our dispatch is
  `movsx eax,word ptr [ebx+0x2f4] / cmp eax,6 / ja default / jmp dword ptr [eax*4+...]`
  against January's identical sequence off `edi`.

**On the jump table**: January's eight `_vehicle_update` self-relocations are its
table, sitting at `@0x8f0` inside the function's own `0x910` bytes. Ours forms a
table too, but MSVC emits it against a generated label (`$L10478`) rather than
inside the function, so the eight self-references do not appear. Treat the
"8 self-relocs" line in the reference table above as *csplit's attribution of
January's table*, not as something our source must reproduce directly -- the
dispatch code is what matters and it matches.

Still absent: everything before the bracket (the `cross_product3d` /
`point_from_line3d` / `scale_vector3d` / `dot_product3d` geometry and the four
`physics_variable_speed_update_seek` + one `physics_variable_position_update_seek`
seek calls with their three deg->rad conversions), and everything after the pop
(`global_structure_bsp_get` x2, the `scenario_get_game_globals` /
`tag_block_get_element_with_size` / `damage_data_new` / `object_cause_damage`
damage path, `unit_update_animation`, and `object_permute_region` with `"~blur"`).

### `vehicle_update`: the throttle seeks

**592 B / 49 relocations -> 704 B / 51.** The flag-guarded throttle pair is in:

```c
if (TEST_FLAG(vehicle->vehicle.flags, 3))
{
    physics_variable_speed_update_seek(&vehicle->vehicle.unknown42c,
        (struct physics_variable_speed_parameters *)&definition->unknown2f8, 0.0f, 1.0f);
}
else
{
    physics_variable_speed_update_seek(&vehicle->vehicle.unknown42c,
        (struct physics_variable_speed_parameters *)&definition->unknown2f8,
        vehicle->unit.throttle.i, 1.0f);
    physics_variable_speed_update_seek(&vehicle->vehicle.unknown430,
        (struct physics_variable_speed_parameters *)&definition->unknown330,
        vehicle->unit.throttle.j, 1.0f);
}
```

confirming the `0x2f8` "speed" block and the `0x330` "slide" block are both
`physics_variable_speed_parameters`, and that the `1.0f` delta is hoisted by
January above the branch (`push 0x3f800000` at `@324`, before the `je`).

Note we emit **2** `physics_variable_speed_update_seek` relocations for three
source call sites -- VC7 tail-merges two of them. January has 4, so the missing
two are the merge plus the one call site not yet written.

### Reference-count scoreboard against January

Everything written so far now matches exactly:

| symbol | JAN | ours |
|---|--:|--:|
| the seven `code_001a*` case bodies | 1 each | 1 each |
| `code_001a6dc0` | 2 | 2 |
| `global_current_collision_user_depth` | 5 | 5 |
| `display_assert` | 2 | 2 |
| `profile_enter_private` / `profile_exit_private` / `data_00316228` | 1/1/4 | 1/1/4 |
| `physics_variable_speed_update_seek` | 4 | 2 |
| `physics_variable_position_update_seek` | 1 | 0 |
| `__real@3c8efa35` (deg->rad) | 3 | 0 |

**The two assert strings do not match.** `display_assert` is called the right
number of times but January's `??_C@_0CI@OJMAHDFE@` (40 chars) and
`??_C@_0EJ@EEKGOMEC@` (73 chars) are still listed as JAN-only, so our stringized
expressions differ in text. Note `collisions.c` has its own 40-char
`global_current_collision_user_de...` string with a *different* hash
(`PANLECBF`), so the two files genuinely spell their asserts differently and the
`collisions.c` wording cannot simply be copied. Use the length arithmetic from
the mangled prefix to pin them down: 40 and 73 characters respectively.

### Still to write

* The steering seek at `@372`-`@472`: branches on
  `definition->vehicle_type != _vehicle_type_human_tank`, applies sign handling
  and two clamps against `definition->unknown30c` and `unknown308` (both
  converted with `*0.0174532925f`), then either
  `physics_variable_position_update_seek(&vehicle->vehicle.unknown434,
  &definition->unknown308, 0, ..., ...)` on the non-tank path or a fourth
  `physics_variable_speed_update_seek` on the tank path, where the target is
  clamped to +/-1 via `*0.6366197f` (`__real@3f22f983`, 2/pi) and `delta` is
  `2.0f`. This is the most intricate remaining piece -- decode it against the
  emission rather than guessing at the clamps.
* Everything after the collision pop: `global_structure_bsp_get` x2, the damage
  path, `unit_update_animation`, `object_permute_region` with `"~blur"`.

### `vehicle_update`: the steering seek at `@372`-`@475`, decoded

Register state entering the block: `ebx` = vehicle, `ebp-4` = definition,
`esi` = `&vehicle->vehicle.unknown42c`, and **`edi` = definition + 0x2f8** (set at
`@313`/`@31c` and still live), i.e. the speed-parameters block. `ebp-0xc` holds a
steering value computed earlier in the function; `ebp-8` is its working copy.

The block opens by loading `unknown42c` and comparing it with `0.0f`, then
branching on the *vehicle type* -- note the `cmp` at `@377` sets the flags the
`je` consumes, while `fnstsw` only loads AX, so the two tests are interleaved:

```
@372  fld [esi]                  ; unknown42c
@377  cmp word ptr [ecx+0x2f4],0 ; vehicle_type == _vehicle_type_human_tank ?
@37f  fcomp [0.0f]              ; and the float compare, result in AX
@387  je 0x417                   ; tank -> tank path
```

**Non-tank path (`@38d`-`@415`)** -- signed steering, two clamps, position seek:

```c
target = (vehicle->vehicle.unknown42c<0.0f) ? -steering : steering;   /* fchs arm */
target = MIN(target, definition->unknown30c*0.0174532925f);           /* @3a2-@3bd */
target = MAX(target, definition->unknown308*0.0174532925f);           /* @3c1-@3dc */

physics_variable_position_update_seek(
    &vehicle->vehicle.unknown434,
    &definition->unknown308,                       /* limits, lea ecx,[edi+0x308] */
    FALSE,                                         /* wrap */
    target,
    definition->unknown314*0.0174532925f*0.0333333351f);   /* @3e3-@405 */
```

The `0.0333333351f` is `__real@3d088889`, i.e. a per-tick `1/30`. Both clamps
convert their definition field from degrees with `__real@3c8efa35`, which is why
that constant appears three times.

**Tank path (`@417`-`@472`)** -- normalised steering, speed seek:

```c
if (vehicle->vehicle.unknown42c!=0.0f)            /* test ah,0x44 / jp  => != */
    physics_variable_speed_update_seek(&vehicle->vehicle.unknown434,
        (struct physics_variable_speed_parameters *)&definition->unknown2f8,
        PIN(vehicle->vehicle.unknown42c*0.63661975f, -1.0f, 1.0f)*definition->unknown2f8,
        2.0f);
else
    physics_variable_speed_update_seek(&vehicle->vehicle.unknown434,
        (struct physics_variable_speed_parameters *)&definition->unknown2f8,
        0.0f, 1.0f);
```

`0.63661975f` is `__real@3f22f983` = `2/pi`; the `*definition->unknown2f8` at
`@45a` is `fmul dword ptr [edi]` with `edi` still holding definition+0x2f8. The
two arms are tail-merged by VC7 onto the single `call` at `@46d`, and **the delta
genuinely differs between them** -- `1.0f` when `unknown42c` is exactly zero,
`2.0f` otherwise. That asymmetry is real, not a misread: the `==0` arm pushes
`0x3f800000` at `@41c` and the other pushes `0x40000000` at `@45c`.

**Not yet resolved:** what `[ebp-0xc]` is. It is written before `@372` and is the
raw steering input to both paths; identifying it needs the earlier part of the
function (the `cross_product3d` / `dot_product3d` geometry at `@1e5`-`@2e8`) to be
decoded first. Writing the block before then would be guessing at its source
expression, so the two seeks above are complete but their `steering` operand is
still an unknown -- that is the next thing to pin down, not the seeks themselves.

### `vehicle_update`: the steering input is an `arctangent` of two dot products

`[ebp-0xc]` -- the value the whole steering seek hangs off -- is written once, at
`@16d`, and the instruction before it settles what it is:

```
@14d  fld [ebx+0x1dc] ; fmul [edi+8]
@156  fld [ebx+0x1d8] ; fmul [edi+4] ; faddp
@161  fld [ebx+0x1d4] ; fmul [edi]   ; faddp     ; X = dot(V, edi)
@16b  fpatan                                      ; atan2(st1, st0)
@16d  fstp [ebp-0xc]
```

`fpatan` computes `atan(st1/st0)` with quadrant correction and pops -- i.e.
`atan2(Y, X)`. `real_math.h` already provides the wrapper this compiles from:

```c
__inline real arctangent(real y, real x) { return atan2(y, x); }
```

So the source is `arctangent(<Y>, <X>)` where **X** is the dot product of the
vehicle-unit vector at `+0x1d4` with the vector `edi` points at, and **Y** is the
same vector dotted against a second direction, accumulated at `@131`-`@14b` with
the same `0x1d4`/`0x1d8`/`0x1dc` triple. Both dot products are written out
inline, not through `dot_product3d` -- the two `dot_product3d`/`cross_product3d`
relocations in this function belong to the geometry further down at `@1e5`-`@2e8`.

`+0x1d4` is one of `desired_facing_vector` / `desired_aiming_vector` /
`aiming_vector`, which `units.h` declares consecutively; resolve it with the
offset-oracle trick in `docs/house_rules.md` §6 (`char probe[offsetof(...)+1];`
compiled once and read back out of the COFF) rather than by counting members.

Also decoded on the way past, `@1ed`-`@238`, which rescales `[ebp-8]`:

```c
[ebp-8] *= PIN(vehicle->object.up.k*-2.0f,
               definition->unknown340, definition->unknown344);
```

(`__real@c0000000` = `-2.0f`.) Note `[ebp-8]` is a scratch slot VC7 reuses: here
it is this running value, and later in the steering seek it is the `+/-` copy of
`[ebp-0xc]`. Do not read the two uses as the same variable.

**Where `vehicle_update` stands:** 704 B / 51 relocations of 2,320 / 98. The
profile wrapper, collision-user bracket, seven-case switch and throttle seeks are
in and their reference counts match January exactly. The steering seek is fully
decoded and blocked only on naming the `+0x1d4` vector and the two directions
dotted against it; after that the remaining unwritten regions are the `@1e5`-`@2e8`
geometry and everything past the collision pop.
