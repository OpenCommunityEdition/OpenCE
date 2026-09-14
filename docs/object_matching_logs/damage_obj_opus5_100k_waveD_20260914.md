# `damage.obj` wave D park re-investigation (opus5 100K consolidated lane, 2026-09-14)

## 1. Scope and provenance

- Translation unit `source/objects/damage.c`, target `build/split/source/objects/damage.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`).
- Lane `C:\halo-worktrees\opus5-100k-consolidated-20260914`, HEAD `548451f6d`; worker claim `waveD:damage`.
- Authority: January COFF bytes/relocations. HCEA `src/blam/objects/object_cause_damage.c`,
  `object_damage_body.c`, `area_of_effect_cause_damage_to_object.c` used for semantics only.
  No Ghidra/atlas output was prepared for this object.
- Ledgers read first: `damage_obj_area_of_effect_cluster_20260904.md`,
  `damage_obj_object_cause_damage_fuzzy_20260904.md`,
  `collision_bsp_damage_owner_reconciliation_20260907.md`, and the three `config/parked.json` entries.
  `branch_sweep.py source/objects/damage`: 21 historical blobs, none better than the current file.

## 2. Validated baseline

`gate.py source/objects/damage --all --forbid-emitted-symbol _point_from_line3d`:
**exact 31 / residual 3 / unwritten 0** (guard passed). All three residuals are parked and locked:

| Function | Target size / relocs / sha16 | Base size / relocs / sha16 |
| --- | --- | --- |
| `_area_of_effect_cause_damage_to_object` | 1328 / 51 / d3f929d4ce73b3cd | 1312 / 52 / 46952a116c873e6f |
| `_object_cause_damage` | 1984 / 75 / 68f55b08e71a95b9 | 1968 / 75 / 899b76c53282a521 |
| `_object_damage_body` | 1360 / 43 / 15116626eee783a5 | 1392 / 44 / 9e29dfe9abf1237a |

## 3. Accepted controls

**The real file was not changed.** Every target was a locked park, so all work stayed in scratch
(`scratch/workersD/damage/`). The final real-file gate matches the baseline exactly (31/3/0). Park drift:
all three parks are still at their base measurements. Owner census: identical to `build/base`.
Fake scan: 0 leads.

## 4. Experiment matrix

### `_object_cause_damage`: strict EXACT in scratch (park-reopen proposal)

Candidate `scratch/workersD/damage/ocd_park_reopen.c` (sha256 `ac8b2024...`) is the real file with only
this function's body changed. In the whole-TU gate it scores **exact 32 / residual 2**. No
baseline-exact row regressed, the point guard passed, and the other two parks did not drift.

January byte facts and the source spelling each one proves:

1. `0x2f4 cmp ebx,[edx+0x2d4]`: the seat test compares against `unit.driver_object_index`. The
   retained source compared `gunner_object_index` (+0x2D8), which was a semantic bug. HCEA agrees
   on the driver.
2. `0x2e3-0x30a`: player_index is read once and the SET_FLAG condition is folded by jump threading.
   This is a named `long player_index` local. Our re-read of the field produced a stray `cmp eax,eax`.
3. `0xd9-0xee`: the actor index is a phi into a fresh register, which means a conditional
   expression `swarm != NONE ? swarm : actor_index`. HCEA uses the same form.
4. `0x341-0x3a8`: the sequence `test di,di; jle; movzx edi,di; ...; dec edi; jne` is VC7 strength
   reduction of an index `for` loop. The hand-written pointer do-while produces different code. HCEA
   also uses a `for` loop.
5. `0x42d-0x43d`: the collision-model definition lookup happens before the kill-instantly flag is
   extracted.
6. `0x73-0x82`: both float bounds are copied through fresh registers into dead home slots. These
   are the inlined parameter homes of the existing `real_math.h` `__inline real_random_range`
   wrapper (January also has a public `_real_random_range`). The direct HCEA-style
   `real_seed_random_range` call gave 1968 bytes (worse).
7. `0x277-0x298`: the first-child load is interleaved before the multiplier `fstp`. This means
   `child_object_index` is initialised at its declaration.

| Shape | Result |
| --- | --- |
| c1 = facts 1, 3, 4, 5 | 1984/75 sha, 60 diff lines |
| + direct HCEA args (R1) | 1968 (reverted) |
| + fact 2 (P) | 1984 sha, 39 lines |
| + fact 6 (R2) | sha, only fact 7 left |
| + fact 7 (S1) | **EXACT** |
| S3 (inline unit definition + S1) | sha (rejected) |
| loop index declared at function top (no bare block) | still EXACT (retained) |

Owner-census note: the candidate adds a `_real_random_range` header-inline COMDAT. This is the same
systemic class as the base object's existing unreferenced `_cross_product3d`, `_magnitude3d`,
`_scale_vector3d` and `_vector_from_points3d` COMDATs, and base objects of `action_alert`, `actors` and
`action_obey` already emit it. It is not a new translation-unit function owner.

### `_object_damage_body`: size and relocations now equal, sha tie (not exact)

- January `0x357-0x38c` walks the vehicle's children with one `object_get(child)` per child, then
  reads `unit.player_index` from the same pointer.
- The park rejected this raw downcast as a violation of typed access. However, the same TU's
  admitted strict-exact `object_deplete_body` (damage.c:570) walks vehicle children with exactly
  `(struct unit_datum *)object_get(unit_index)`. `actors.c` also keeps a disclosed accessor for the
  same polymorphic chain. The park's premise is therefore contradicted by admitted house style.
- **v1** (that idiom in the child loop): **1360/43** sha `72b75755994a203b`. The first 0x456 bytes
  are identical.
- **v2** (hoisting `body_vitality` out of the bare block) and **v3** (no `region` local in the
  dying loop) compile identically to v1.
- Remaining register tie in the tail: the `edi` reload is placed before rather than after
  `object_destroy_region` in the dying-region loop; eax/ecx are swapped for
  `body_damaged_effect.index`; and `*body_damage = damage_amount` is copied via x87 instead of an
  integer `mov`.

### `_area_of_effect_cause_damage_to_object`: relocations now equal, x87 clamp tie (not exact)

**New measured law (VC7 tail recursion).** VC7 eliminates a self tail-call (parameter stores plus
`jmp` to the function top) only if no address-taken local is in scope at the tail call when that
local's address escapes into an out-of-line call. Probes are in `scratch/workersD/damage/probe.c`:

| Probe | Result |
| --- | --- |
| Function-scope `collision_result` passed to `collision_test_vector` | `call` kept |
| Same local declared in an inner block that closes before the tail call | `jmp 0x10` |
| Address-taken vector used only by an inlined helper | `jmp` |

Shapes tried:

- **A1**: the escaping vectors and collision results are scoped to the core-radius branch and the
  direct-ray branch. This reproduces January's `mov byte [ebp+0x10],1; mov [ebp+0xc],esi; jmp 0x10`,
  giving **relocs 51/51** and size 1312 (sha `2d73b0926b5b0ec1`).
- The only residual is the falloff clamp. January pops the dead radius delta with
  `fxch st(1); fstp st(0)` and places the else-arm after the zero arm. We emit `fstp st(1)`. This
  moves the jump table across a 16-byte padding boundary.
- **X1** (no `radius_delta` local), **X2** (explicit if/else clamp as in HCEA) and **X3** (two-step PIN)
  are all identical to A1.
- **X4** (inverted `<= 0` test) is worse. **X5** (?: expression) is identical.

## 5. Do-not-repeat list

- `_object_damage_body`:
  - hoisting the tail `body_vitality` declaration;
  - removing the dying-loop `region` local.
- `_area_of_effect_cause_damage_to_object`:
  - removing the `radius_delta` local;
  - an explicit if/else clamp;
  - a two-step PIN;
  - an inverted `<=` test;
  - a ?: clamp;
  - passing the HCEA-style direct args to `real_seed_random_range` (applies to `object_cause_damage`).
- `_object_cause_damage`: inlining the unit-definition lookup into the multiplier (S3).

## 6. Residual classification

- `_object_cause_damage`: none (exact in scratch).
- `_object_damage_body`: register/x87-copy scheduling tie in the tail (measured). The structural
  cause (the extra lookup) is solved.
- `_area_of_effect_cause_damage_to_object`: x87 stack-kill scheduling in the PIN clamp (measured). The
  structural cause (tail recursion) is solved.

## 7. Reopen criteria

- `_object_damage_body`: a source mechanism for the tail edi-reload placement and the x87-versus-integer
  float copy.
- `_area_of_effect_cause_damage_to_object`: a source mechanism for the clamp's
  `fxch st(1); fstp st(0)` dead-value pop.

## 8. Disposition

- `_object_cause_damage`: **park-reopen proposed** with strict EXACT scratch evidence. It receives no
  credit until the orchestrator unparks it and lands it.
- The other two remain rigorously parked. The orchestrator may refresh their evidence and base with
  the improved natural bodies (v1 1360/43, A1 1312/51). `scratch/workersD/damage/combined_all_parks.c`
  holds all three bodies together (exact 32; `object_cause_damage` stays EXACT).
- No strict-exact credit is claimed in this wave.
