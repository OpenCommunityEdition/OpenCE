# `actor_moving.obj` — opus5 250K house-clean lane, wave w3d (structural pass, 2026-09-15)

## 1. Scope and provenance

- Translation unit `source/ai/actor_moving.c`; target `build/split/source/ai/actor_moving.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, XDK 3911).
- Narrow structural pass over three rows only: `_actor_move_vector_avoidance`, `_actor_move_update`,
  `_actor_get_stopping_distances`. No other row was touched.
- Measurements: `tools/campaign/gate.py --forbid-emitted-symbol _point_from_line3d`,
  `tools.campaign.tinfo`, `tools.campaign.alndiff` (`--include-equal` for the frame census),
  `tools.campaign.relocdiff --count-by-target --allow-structural`, `tools/campaign/branch_sweep.py`,
  `tools/fake_match_scan.py`.
- Worker report with every census and shape: `scratch/workers/w3d_t3_actor_moving.md`.
  Candidates and dumps: `scratch/workers/w3d_t3_actor_moving/`
  (`base.c/base.obj`, `va_alndiff.txt`, `va_full.txt`, `mu_alndiff.txt`, `mu_full.txt`, `sd_alndiff.txt`,
  `slots.py`, `va1.c/va1.obj`, `sd1.c`).
- Prior ledgers read: all fifteen `actor_moving_obj_*.md`, plus `scratch/WORKER_BRIEF.md`,
  `scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md` (sections A–E).

## 2. Baseline and result

| | exact | residual | unwritten |
|---|---:|---:|---:|
| baseline (real file at HEAD) | 29 | 7 | 0 |
| final real-file gate | **29** | 7 | 0 |

`_point_from_line3d` guard passes at baseline and at the end. The unit has **no parks**, so there is no
park drift. `git diff --stat -- source/ai/actor_moving.c` is **empty**: nothing landed, by design.
`tools/fake_match_scan.py`: 0 review leads. Two shapes were spent in total (one per closable target),
both reverted in scratch.

Duplicate prevention: `git log --all --oneline -- source/ai/actor_moving.c` (21 commits; the newest is the
w3 landing `901a6ae73`, already in the baseline). `branch_sweep.py source/ai/actor_moving`: 18 unique blobs
across all branches; **HEAD is the best of them** (exact 29; next best 26). No donor body exists for any of
the three targets.

## 3. `_actor_move_vector_avoidance` — NOT LANDED; structural gap fully quantified, policy-blocked

Target 4144 / 135 relocs / `a8a8010c11375e82`; ours 4192 / 140. 1212 vs 1229 instructions; 236 hunks.

### Frame census
January `sub esp, 0x60e0`; ours `sub esp, 0x60ec` — **+12 = exactly one `real_vector3d`.**
The thirteen referenced cells of the `avoidance_data` aggregate have an identical shape in both builds
(4 cells, an 8-byte hole, then 9 cells) and are simply shifted by 0xc; the small-local region starts at
January `-0xa0` and ours `-0xac`. So ours allocates one surplus 12-byte cell and everything above it moves
down. The last six aggregate cells (`avoidance_data.left`, `.up`) are read **4× by January and 3× by us**.

### Relocation census
Only five rows differ, and they sum exactly to 140 − 135 = +5 (the `symbol:` vs
`defined-noncode:.rdata:` spelling of `__real@…` is a csplit artifact and is combined):

| target | JAN | OURS | delta |
|---|---:|---:|---:|
| `__real@00000000` | 21 | 24 | +3 |
| `__real@3f800000` | 24 | 28 | +4 |
| `_avoid_ray_avoidance_weights+4` | 4 | 2 | −2 |
| `_actor_move_transform_avoidance_vector` (REL32) | 1 | 2 | +1 |
| `_global_zero_vector3d` | 3 | 2 | −1 |

Everything else matches, including both `_actor_move_test_avoidance_vector` calls, both
`_actor_move_vector_avoidance_find_direction` calls, `_csmemset`, `_csmemcpy`, `_angle_between_vectors3d`,
`_avoidance_directions+0/+4/+8`, `_sense_rays`, `_avoidance_rays` and the five
`__real@3f1a36e2e0000000` normalize3d epsilons (**normalize3d inlines at the same 5 sites in both builds**).

### Defect (a): January inlines the transform at the `perpendicular` site — one mechanism, four deltas

January's only `_actor_move_transform_avoidance_vector` REL32 is at `+0xbff`, inside the `sharp_turn`
block (`_angle_between_vectors3d` follows at `+0xc92`). Its **third** `_global_zero_vector3d` reference is at
`+0xe74`, inside the `else if (maximum_sense_emergency > 0.f)` arm, and the bytes there are the transform
body expanded with the `forward` group folded away:

```
e73  mov eax,[_global_zero_vector3d]                     ; *direction_vector = *global_zero_vector3d
e7b  mov ecx,[eax]; edx,[eax+4]; eax,[eax+8]  -> [ebp-0x24]/[ebp-0x20]/[ebp-0x1c]   (= rotation)
e86  movsx eax, word [ebp-0x28]; lea eax,[eax+eax*2]; shl eax,2     ; best_avoidance_direction*12
e96  fld [eax+_avoidance_directions+8]; fchs             ; component = -avoidance_directions[best].k
ea5  fld [ebp-0x60bc] / [ebp-0x60b8] / [ebp-0x60b4]      ; avoidance_data.left.i/.j/.k
ecc  fld [eax+_avoidance_directions+4]; fst [ebp-0x44]   ; component =  avoidance_directions[best].j
ed5  fmul [ebp-0x60b0] / [ebp-0x60ac] / [ebp-0x60a8]     ; avoidance_data.up.i/.j/.k
```

No `perpendicular` storage exists (the two components are consumed straight off the x87 stack) and no
`forward` term is emitted, because `perpendicular.i` is the compile-time constant `0.f`. That single
mechanism explains every remaining census delta: the extra call in ours, the missing
`_global_zero_vector3d`, the six missing `left`/`up` reads, and the +12 frame (VC7 scalar-replaces
`perpendicular` once the call is inlined).

**Why it cannot be landed.** `__forceinline` is forbidden (forced inlining, brief §5). The transform body
cannot be made cheaper: January's own bytes prove the `real component;` local (`fld [edx]` once at
`+0x25/+0x45/+0x65`, then `fld st(0)` to duplicate) and the `*global_zero_vector3d` copy, and the row is
already strict EXACT at 144. Writing the site out by hand is manual inlining (house rule 13 / laws_w3 R15);
the w3 worker self-withheld exactly that form, and it is the only reason w1's `va6.c` reached 4128/135.

### Defect (b): byte-proven arm polarity in the avoid-ray weight loop (new, admissible)

January at `+0x4a0`: `cmp word [esi],0 ; fld 1.0f ; jne 0x51b` — the **clear** case is the fall-through
THEN arm and the obstructed case is the block at the end. Our source spells it
`if (avoidance_types[ray_index] != _actor_vector_avoidance_clear) {obstructed} else {clear}`, so we lay the
obstructed arm out first (`je 0x4e5`). January's source is therefore
`if (avoidance_types[ray_index] == _actor_vector_avoidance_clear) { … } else { … }`
(laws_w3 A26 / laws_w2 A16: block layout follows source arm order).

### Defect (c): the weight multiply is tail-duplicated in January

January emits `fmul dword ptr [ecx]` (the strength-reduced `avoid_ray_avoidance_weights+4` walker) at FOUR
sites (`+0x4be`, `+0x4f2`, `+0x511`, `+0x536`) — one per PIN arm of `clear_fraction` plus the obstructed
arm; we emit it twice because all clear sub-arms jump to a shared multiply. This is the
`−2 _avoid_ray_avoidance_weights+4` row.

### Shape measured

**va1** (`scratch/workers/w3d_t3_actor_moving/va1.c`) — defect (b) only. Result **4208 / 142**
(baseline 4192 / 140; target 4144 / 135), 239 hunks.
It **confirms (b) and (c) together**: the loop body becomes instruction-for-instruction January's shape and
the `_avoid_ray_avoidance_weights+4` relocation row goes 2 → **4 = January's 4**. The only residual left
inside the loop is a register fact: January keeps `obstructed` in **BL** (`xor bl,bl` in the preheader,
`mov bl,1` at `+0x53c`) while we spill it to `[ebp-0x39]`, which costs the +16 bytes.
NOT landed: an already-written residual may land only when strict EXACT (brief §6), and va1 is not strictly
closer on size, relocations or hunk count.

### Reopen criterion

(i) A measured VC7 mechanism that makes a same-TU, non-`__inline`, 13-statement function inline at ONE call
site — the one whose vector argument has a compile-time-zero component — while staying a call at the other;
or (ii) an owner ruling admitting the written-out transform at the `perpendicular` site. Start from `va1.c`
(loop shape already correct) plus a cause for `obstructed` living in BL.

## 4. `_actor_move_update` — SKIPPED-EXHAUSTED; the `[size]` tag is NOT structural

On the brief's DO-NOT-SPEND list; touched only for the mandated census, zero shapes spent.

- **Frame census: identical.** January `sub esp,0x44`, ours `sub esp,0x44`; every `[ebp-N]` cell exists in
  both builds, none is JAN-only or OURS-only.
- **Relocation census: 72 / 72, every row equal.** No missing or extra call, global, literal or assert
  string (including both `_normalize3d`, both `_normalize2d`, both `_valid_real_normal3d`,
  `_vector_from_points3d`, `_rotate_vector_about_axis`, `_actor_move_vector_avoidance`,
  `_actor_move_calculate_movement`).
- Only four slot use-counts differ: `[ebp-0x8]` (`crouch`) **20 vs 9**, `[ebp+8]` (the `actor_index`
  parameter home) 6 vs 4, `[ebp-0x3c]` 6 vs 5, `[ebp-0xc]` 6 vs 7.

The entire 48-byte / 19-instruction gap is **register-allocation write-back traffic**: January writes
`crouch` back to its home at every assignment (`xor bl,bl; mov byte [ebp-8],bl`, sixteen sites) even though
`[ebp-8]` is never read back as a byte, while our build elides eleven of those write-through stores; plus
two extra `[ebp+8]` re-reads and one zero-register pin (`cmp word [esi+0x15e], 0` vs our
`cmp word [esi+0x15e], di`). A25's join-store lever already fires correctly for the one if/else that assigns
`crouch` — both builds emit the merged `mov byte [ebp-8],bl` at the join (JAN `+0x400`, ours `+0x3f3`) — and
the remaining sixteen sites are switch-case constant assignments with no arm to merge.

**Recommendation:** reclassify this row from `[size]`-structural to an allocation tie so that no future wave
spends a structural pass on it. Reopen criterion: a measured VC7 lever that forces write-through of a byte
local sharing a recycled, address-taken 4-byte cell.

## 5. `_actor_get_stopping_distances` — NOT LANDED; the laws_w3 C1 reopen criterion is REFUTED

Target 464 / 11 relocs; ours **464 / 11**. 156 vs 156 instructions. Frame identical. Relocation census:
**0 differing rows**. Exactly one real hunk:

```
T  f6  fld  dword ptr [esi + 0x24]      ; biped->object.forward.i
T  f9  fmul dword ptr [esi + 0x18]      ; biped->object.translational_velocity.i
O  f6  fld  dword ptr [esi + 0x18]
O  f9  fmul dword ptr [esi + 0x24]
```

`laws_w3` C1 (A35(4)) asks for "an earlier by-name read of the same fields … an earlier hand expansion that
ages velocity/forward before the dot". A full scan of the **target** stream for `esi`-based field reads
gives `[esi] 2, [esi+0x18] 2, [esi+0x1c] 2, [esi+0x20] 2, [esi+0x24] 2, [esi+0x28] 2, [esi+0x2c] 2` —
each velocity/forward component is read exactly twice in January, once in the vehicle dot at `+0x89` and
once in the biped dot at `+0xf6`, and `[esi]` twice for the two `definition_index` loads. **There is no
earlier read of `translational_velocity` or `forward`, by name or otherwise, anywhere in January's
`_actor_get_stopping_distances`**, so the A35 leaf-aging mechanism cannot explain this row.

What January does show: the two dots are the same inlined `dot_product3d` spelling, and January flips the
i-term in the biped one only (vehicle `+0x89` `fld [esi+0x18]; fmul [esi+0x24]`, biped `+0xf6`
`fld [esi+0x24]; fmul [esi+0x18]`), with the j and k terms identical at both sites. The instructions
interleaved into the biped dot (`mov ecx,eax` / `mov al,[ecx+0x2f4]` / `add esp,8` / `test al,4`) are
byte-identical to ours, so the flip is decided inside VC7's commutative x87 lowering (laws_w3 A44, "the
inliner canonicalises inline-call evaluation order by liveness"), not by anything visible at the call site.
laws_w2 R3 forbids an operand swap justified only by x87 load order, and the helper body lives in
`source/math/real_math.h`, which workers may not edit.

Shape measured — **sd1** (`scratch/workers/w3d_t3_actor_moving/sd1.c`): move `biped_definition_get` after
the dot (`struct biped_definition *definition;` + assignment) so the leaf creation order changes.
Result **448 != 464 [size]** — structurally worse, refuted; it also positively proves that January's
definition fetch precedes the dot (`call tag_get` at `+0xd6` before the first `fld` at `+0xdb`).

**Reopen criterion (replaces the refuted A35(4) one):** a measured VC7 rule that decides the `fld` operand of
a commutative `mem*mem` product differently for two identically-spelled inlined `dot_product3d` calls inside
one function, or an owner ruling on editing `real_math.h`'s `dot_product3d` association.

## 6. Proposals

No header, `symbols.json`, `parked.json` or other config change is needed or proposed by this wave.

Two board-hygiene items for the orchestrator (no bytes, no source change):

1. Reclassify `_actor_move_update` (3136) from a `[size]`-structural residual to an allocation tie
   (census in §4: frame identical, relocations identical).
2. Retire the `laws_w3` C1 row for `_actor_get_stopping_distances` (464): its stated reopen criterion is
   refuted by January's own bytes (§5). It is a one-instruction-pair commutative x87 tie.

## 7. Do-not-repeat (added)

- `_actor_move_vector_avoidance`: do not re-derive the frame gap — it is `perpendicular`, and it disappears
  only when the transform is inlined at that site. Do not re-measure a call/`__inline`/`__forceinline` form
  of the transform (w1 already measured `__forceinline` at 4304). The arm polarity of the avoid-ray weight
  loop is settled by January's `jne` at `+0x4aa` (clear arm first); va1 measures it at 4208/142.
- `_actor_move_update`: do not spend a structural pass — frame and relocations are identical; the gap is
  `crouch` write-back traffic.
- `_actor_get_stopping_distances`: do not look for an earlier by-name read of the biped velocity/forward
  components; January has none. Do not re-order the `biped_definition_get` fetch (sd1, 448 [size]).

## 8. Disposition

`actor_moving.obj` remains active, not Matching: 29 / 36 strict exact, unchanged by this wave. No new exact
credit, no fuzzy landing, no park change, no owner change, and the real file is byte-identical to HEAD.
The wave's product is the three censuses, the two new measured facts in `_actor_move_vector_avoidance`, and
the refutation of one third-pass candidate.
