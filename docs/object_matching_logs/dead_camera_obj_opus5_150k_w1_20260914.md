# `dead_camera.obj` - opus5 150K house-clean lane, wave w1 (2026-09-14)

## Outcome

No source change landed. `source/camera/dead_camera.c` is byte-for-byte unchanged
(`git diff --stat` empty). The unit stays at **2 exact / 2 residual / 0 unwritten**,
the `_point_from_line3d` guard passes, and the unit has no parks.

| Function | January padded / relocs | Baseline | Result |
| --- | --- | --- | --- |
| `_player_has_allies` | 112 / 6 | EXACT | EXACT (untouched) |
| `_player_get_next_player_with_a_unit` | 176 / 6 | EXACT | EXACT (untouched) |
| `_dead_camera_new` | 288 / 19 | residual (sha) | NOT-LANDED: the tail was closed in scratch, one 3-instruction x87 schedule remains |
| `_dead_camera_update` | 1,248 / 46 | residual (sha) | SKIPPED-EXHAUSTED: needs an uninitialized read (undefined behaviour) |

## Prior ledgers and do-not-repeat items

These ledgers were read before any work:

- `dead_camera_obj_codex_checkpoint.md`
- `dead_camera_obj_fable_semantic_names_20260831.md`
- `dead_camera_obj_jonas_hcea_constructor_topology_rejection_20260826.md`
- `dead_camera_render_particles_obj_jonas_closure_freshness_audit_20260830.md`
- `safe_wave_residual_reconciliation_20260831.md`
- `legacy_residual_donor_truth_audit_20260913.md`

Other duplicate checks:

- `git log --all`: reviewed.
- `branch_sweep`: no history blob beats the current source.

The HCEA combined form (direct pitch negation plus current-player-before-unit) was not repeated.

## `_dead_camera_new`

### Evidence (alndiff against January)

There are two regions of difference.

1. **Tail, from `@0xf1` to `@0x111`.** January stores `unit_index` from EAX in both tail-duplicated arms, including `mov eax, edi` in the explicit-unit arm. That is a single phi value: one expression assigned once. The retained `if/else` form stores EDI directly.
2. **Around `@0x99`.** January emits `mov edi,[ebp+0x10]; mov edx,[dead_camera_constants]; fchs; add esp,0xc; fstp [esi+0x10]; cmp edi,-1; mov [esi+0x1c],edx`. VC7 negates immediately after the call.

### Shapes (scratch/workers/tiff_dead_camera/dc*.c)

| Shape | Change | Result |
| --- | --- | --- |
| A | `if (unit_index == NONE) unit_index = player_get(player_index)->dead_unit_index;` then a single store | 272 bytes (size regression), rejected |
| **B** | `camera->unit_index = unit_index == NONE ? player_get(player_index)->dead_unit_index : unit_index;` | 288 / 19. **Tail region now identical to January.** Only region 2 remains. |
| C | B + direct `camera->facing.pitch = -real_local_random_range(...)` before the timer store | fchs/fstp move earlier (worse) |
| E | B + `pitch = -rand; timer; facing.pitch = pitch` | identical to B |
| E2 | B + `pitch = -rand; facing.pitch = pitch; timer` | same as C |
| F | B + a single nested ternary for `switch_timer` | identical to B |
| G | F + C | same as C |

Shape B is a real structural recovery. It is still not strict exact, so the rule for already-written residuals keeps it out of the real file. Its fake scan is clean.

**Classification:** scheduler tie (class D). The tie is where the `fchs` falls relative to the integer copy of `dead_timer` and the load of the `unit_index` parameter.

**Reopen criterion:** apply any authentic pitch-range spelling (for example a genuine helper or macro) that delays the negation past the `dead_timer` copy. Apply it on top of `dcB.c`.

## `_dead_camera_update`

Exactly one instruction differs. January `@0x176` is `mov eax, dword ptr [ebp+0x10]` (the fallback when the next player is `NONE`); ours is `mov eax, esi`.

- `[ebp+0x10]` is the home slot of the `result` parameter. `result` is already live in ESI.
- The slot is first written at `@0x1db`, by the `valid_real` temporaries.
- January therefore reads an uninitialized local that VC7 coalesced into that slot. The original `next_unit_index` was never assigned on the `NONE` path.

Any defined C that reads `result` uses ESI. Reproducing January's read requires undefined behaviour, which brief section 5 forbids and the 20260913 donor audit rejected. No shapes were tried.

**Reopen criterion:** an owner ruling that admits this uninitialized-read original bug.

## Checks

- Final real-file gate equals the baseline: 2/2/0, guard passed.
- Siblings stayed EXACT in every scratch shape.
- No parks. Fake scan: 0 leads, both in the real file and in `dcB.c`.
- No header or config proposals.
