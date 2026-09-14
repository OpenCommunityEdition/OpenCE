# `ai.obj` Opus 5 100K consolidated, wave D park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/ai/ai.c`. Target `build/split/source/ai/ai.obj`
  (January 2002 Xbox debug build, csplit).
- Compiler: XDK 3911 `CL.Exe` 13.00.9254, repository flags
  `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` (plus the TU's existing function-scoped
  `#pragma optimize` bracket around `ai_handle_spatial_effect`, unchanged).
- Lane `opus/100k-consolidated-20260914`, wave D worker `waveD:ai`. Scope was this
  one `.c` file. No header, config, park or source change was made; every
  experiment ran on scratch copies in `scratch/workersD/ai/`.
- Evidence read first: `ai_obj_large_visibility_spatial_packet_20260909.md`,
  `ai_obj_482743d8a_reconciliation_audit_20260909.md`,
  `ai_obj_opus_object_event_reconciliation_20260909.md`,
  `ai_profile_shared_owner_migration_20260909.md`,
  `lane_reconciliation_10k_batch_3_20260909.md`,
  `players_obj_opus5_house_clean_20260913.md` (FPU note),
  `sound_dsound_xbox_obj_opus5_100k_waveC_20260914.md` (paren law),
  `docs/exact_match_acceleration_playbook.md` ("Original bugs"), the five
  `config/parked.json` entries, `git log --all -- source/ai/ai.c`
  (44a36bec9 / 603f5b41c editor history), HCEA
  `halocea-review/src/blam/ai/ai_{test_line_of_fire,test_line_of_sight,test_ballistic_line_of_fire,disconnect_from_structure_bsp,handle_editing}.c`.
- Worker notes with every measurement: `scratch/workersD/ai.md`.

## Validated baseline (unchanged at finish)

Real-file gate with `--forbid-emitted-symbol _point_from_line3d` (guard passed):
**41 exact / 5 residual / 0 unwritten**. All five residuals are parked and locked.

| owner | target / base padded | relocs | base normalized sha16 | target sha16 |
| --- | ---: | ---: | --- | --- |
| `_ai_disconnect_from_structure_bsp` | 1280 / 1280 | 62 / 62 | `19404fc01508e0ef` | `50551a92f7dfebd3` |
| `_ai_handle_editing` | 1296 / 1296 | 70 / 70 | `5a0356c0457d3b50` | `652338b1724b0f70` |
| `_ai_test_ballistic_line_of_fire` | 944 / 944 | 49 / 49 | `c8f5527aacb22c83` | `6c90076033e91f26` |
| `_ai_test_line_of_fire` | 336 / 336 | 10 / 10 | `a2c988c9fb9cb73a` | `0aef44a538a12480` |
| `_ai_test_line_of_sight` | 1008 / 1040 | 50 / 50 | `61a3ce7f829fcafe` | `aecb662c1c4868e2` |

Final real-file gate: identical listing, all five parks at base hashes, owner census
of `scratch/workersD/ai/final.obj` equal to `build/base/source/ai/ai.obj`, fake scan
2 pre-existing pragma leads and 0 new. `git diff --stat -- source/ai/ai.c` is empty.

## Park-reopen proposals (scratch candidates, strict EXACT)

The combined candidate `scratch/workersD/ai/reopen_all3.c` gates
**44 exact / 2 residual**. No baseline-exact row changes. The two remaining parks stay
at their base hashes, the owner census passes, and the fake scan shows no new lead.

### 1. `_ai_disconnect_from_structure_bsp` (candidate `dis1.c`)

- **Park criterion.** "Authoritative January source/symbol context or a natural
  same-compiler donor that explains the residual."
- **January evidence.** The slot census is identical to ours, so this was never a
  frame gap. At 0x226 January emits
  `mov eax,[edi+edx*4+0x18]; lea esi,[edi+edx*4+0x18]; push eax; call _object_get_ultimate_parent`.
  At the `components_outside_pvs` store (0x28d) it emits `mov ecx,[esi]`. The swarm
  unit element is read through memory again after the calls. That is the
  store/call-order law: a pointer read cannot be CSE'd across a CALL. So the
  source indexes `swarm->unit_indices[component_index]` directly at both sites.
- **Our old source.** It copied the element once into a `long unit_index` local.
  Every other diff block was downstream register pressure from that extra carrier:
  the `actor_create_for_unit` argument loads, the `error()` argument registers, the
  disconnect tail and the encounterless loop.
- **Change.** Delete the local and read the element at both sites. The body has
  one fewer local and its logic is unchanged. It was exact on the first shape.

### 2. `_ai_handle_editing` (candidate `ed1.c`, policy decision)

- **Code change.** Pass both `csmemmove` lengths unscaled, as January does. Each
  call gets a `BUG (original)` comment that names the corrected
  `* sizeof(struct squad_datum)` / `* sizeof(struct platoon_datum)` form. This is
  the byte-identical body from 44a36bec9 plus those comments.
- **January proof of an original defect.** The `csmemset` lengths are scaled:
  `shl eax,5` at 0x144 for squads and `shl eax,4` at 0x223 for platoons. The
  `csmemmove` lengths at 0x12a and 0x206 push the raw record-count difference.
- **Why this is new evidence for the 20260909 park.** The ledger README lane rule
  and the playbook "Original bugs" section both say to preserve proven original
  behaviour and to comment the corrected build. Lane precedent has since landed
  exactly that: `bitmap_extract.c` `_extract_cube_maps` in wave C, plus
  `encounters.c`, `ai_communication.c`, `ai_script.c` and `ai_profile.c`.
- **Who decides.** The owner explicitly chose the safe copy on 20260909, so
  reopening this park is an owner/orchestrator decision.

### 3. `_ai_test_line_of_fire` (candidate `lof2.c`, admissibility decision)

The candidate makes two changes:

1. **Statement order.** `line_of_fire = FALSE;` now comes before
   `blocking_prop_index = pills[pill_index].prop_index;`. January loads the prop
   index first and then stores the flag, the index and `hit`, in that order. HCEA
   uses the same `clear = 0; prop_index = ...` order.
2. **Paren law.** The radius argument of `fast_vector_intersects_sphere` is written
   `(pills[pill_index].width)`. In January, the sphere arm pushes the width through
   the x87. The `vector_intersects_pill3d` arm reads the same field with the same
   index register and integer-copies it.

**Cross-TU fact.** Every January caller of `_fast_vector_intersects_sphere` that
passes a plain lvalue radius pushes it through the x87:

- `ai`
- `players::_player_examine_nearby_device` (closed by the same paren in the wave C
  transfer probe)
- `collisions::_object_test_vector`
- `bipeds::_biped_update_moving`

The two integer-copy callers show nothing when parenthesised. We measured scratch
copies only: `xfer/real_math_paren.c` (the `radius`/`pill_width` parameters) and
`xfer/cbsp_paren.c` (`data->radius`, passed while st0 is busy). Their rows and call
sites were unchanged.

**Inference.** A macro scoped to this callee that parenthesises its parameters would
fit all seven January caller objects. No such macro is attested. Other spellings
tested inert: the HCEA `?:` form, and a `real const radius` prototype probe.

**Decision needed.** Bare call-site parentheses do not read naturally. The
orchestrator must either accept them or introduce an owner-header spelling.
Such a spelling would also address the players, collisions and bipeds sites.

## Not reopened

### `_ai_test_line_of_sight`

**Residual 1: an original uninitialized read.**

- **What January does.** On the clear path (0x10d) it stores only the clear flag
  and never assigns `collision_fraction`. The fog>0.8 path (0x139 → 0x32f) then
  reads `[ebp-0xc]` at 0x36f and 0x391.
- **Corroboration.** HCEA's header comment describes the later-build decompile
  reading an uninitialised-looking slot.
- **Safe form.** Our initialized form measures 1040 bytes against January's 1008.
- **Policy.** The playbook's original-uninitialized-read exception may apply. That
  is an owner decision.

**Residual 2: a structural frame gap, still open.**

- **Frame size.** January's frame is `sub esp,0x80`: collision_result 0x50, three
  12-byte cells and three scalars. Ours is 0x8c.
- **January's cell sets:**
  - C=-0x18 holds perpendicular, the expand-source left point and the
    expand-target down point.
  - B=-0x24 holds the vector and both right points.
  - A=-0x30 holds the expand-target left point.
- **Ours.** Perpendicular shares its cell with nothing.
- **Shapes tried**, all on the UB draft:
  - drop the init: 1024 bytes, frame 0x8c;
  - perpendicular hoisted to function scope: 1024, 0x8c;
  - vector hoisted: 1040, 0x98;
  - right/left shared at perpendicular-block scope: 1024, 0x8c;
  - all points at function scope: 1024, 0x8c. Vector then shares perpendicular's
    cell, and the function-scope points share with nothing.

### `_ai_test_ballistic_line_of_fire`

**What January does:**

- `fsubr [end]` keeps `segment_time` on the x87 (`fst [ebp+0x14]` spill), and x
  and y use `fmul st(1)`.
- end.z is `(seg*k + p.z) + ((seg*seg)*gravity)*0.5f`, evaluated left operand
  first with no factoring.
- `collision_flags` is homed at `[ebp+0xc]`.

**Shapes tried:**

- **Dropping the parentheses (`bal1`)** reproduces the x87 hold and both homes.
  But VC7 factors the expression into `((g*seg)*0.5+k)*seg+p.z`, giving 928 bytes.
- **Other paren placements** (`z2`, `z3`, `z8`) are the same as base.
- **Constant or operand reorders** (`z5`-`z7`) and **velocity-update respellings**
  (`w1`-`w3`) still factor.
- **The two-statement form (`z9`) and the HCEA order (`h1`, `h2`)** fix the
  arithmetic order, but spill `segment_time` with `fstp` instead of holding it.
- **Loop-scoped `segment_time` (`s1`)** is inert.
- **Diagnostic only (`h3`):** an unnamed `(end-start)` fixes the homes but still
  spills. It is not a natural spelling.

**Classification:** x87 enregistration tie. Stopped.

### `_ai_handle_editing` (real file)

Locked. The proposal above is the only avenue.

## Do-not-repeat list

- The line_of_fire `?:` arm form and `real const` radius prototypes are inert.
- Parenthesising real parameters, or a radius pushed while st0 is busy, does not
  change integer-copy call sites.
- line_of_sight:
  - declaration scope sweeps (items above) with the uninitialized draft;
  - the safe initialized form is 16 bytes larger by construction.
- ballistic: the z-expression spellings listed above. Removing the parentheses always
  factors. Every paren form and the two-statement form spill `segment_time`.
- editing: no byte shape is needed; the question is purely policy.

## Reopen criteria

- **line_of_sight:** owner acceptance of the original indeterminate read, AND a
  natural scope/type shape that lets perpendicular share its cell with the later
  points (frame 0x80).
- **ballistic:** a natural single-expression arc step that VC7 neither factors
  nor paren-orders, or a measured x87 enregistration law for named real locals
  with five or more uses.
- **line_of_fire:** an attested or owner-approved parenthesising spelling for
  `fast_vector_intersects_sphere` radius arguments.

## Disposition

- **No new exact credit:** the real file is unchanged.
- **Three park-reopen proposals** (disconnect, editing, line_of_fire) are strict
  EXACT in scratch. Together they are 44/46 with no sibling or park loss.
- **Two parks remain:** line_of_sight and ballistic.
