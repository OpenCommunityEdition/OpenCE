# `observer.obj` second-pass park re-investigation (Opus5 150K house-clean lane, wave w3, 2026-09-14)

## Result

No real-file change and no new exact credit. `source/camera/observer.c` stays at **24 exact / 2 residual / 0 unwritten**
(guard passes, parks drift 0, owner census unchanged, fake scan 0 leads). `git diff --stat -- source/camera/observer.c`
is empty.

## Scope and provenance

- Targets: the two remaining parks, `_observer_update_command` (1040/33) and `_observer_update_positions` (1568/40).
- Prior ledgers read: `observer_obj_opus5_150k_w2_20260914.md`, `observer_obj_opus5_100k_waveD_20260914.md` and older.
  Also read:
  - worker notes `scratch/workers/observer.md`;
  - `scratch/w2/laws_w2.md`;
  - `physics_obj_opus5_150k_w3_20260914.md`, which records an `_observer_update_positions` row under an axes2 header fix.
- `branch_sweep`: no better blob. The w2 best bodies were re-gated on the current file: `v1.c` update_command 1040/33
  sha-only; `a3.c` update_positions 1568/40 sha-only (18 differing instructions).
- Worker notes: `scratch/workers/w3_observer_editor_flying_camera.md`.

## `_observer_update_command`: skipped, exhausted

**Law detector pass.**
- January computes `lea edx,[esi+0x54]` (timer), `lea eax,[esi+0x4c]` (flags) and `lea edi,[ebx+0x5c]` (last_timer) before
  the assert. All three are therefore top-of-function pointer locals, which is already the w2 body. This refutes mixed
  indexed/pointer spellings: A5 hand-strength-reduction removal does not apply.
- The remaining difference is VC7 merging the timer induction variable into last_timer (`esi = timer0 - last0`, with the
  counter homed). January keeps four induction variables, with the flags pointer homed at `[ebp-4]`.
- No law A1-A22 addresses induction-variable merging. No new shapes were spent (waveD and w2 spent
  pointer/indexed/const/declaration/increment-order/do-while).

**Reopen.** Unchanged: a VC7 induction-variable-merge rule, or January-side live-out evidence.

## `_observer_update_positions`: not landed

**New evidence.** The w3 physics worker showed that January's out-of-line `valid_real_vector3d_axes2` COMDAT is the
conjunction `return valid_real_normal3d(forward) && valid_real_normal3d(up) && valid_realcmp(dot_product3d(forward, up), 0.f);`
and that this body inlines. That lab also found the header fix flips 2 exact bitmap rows, and it is not landed.

**Shape s1** was measured in a scratch shadow header tree
(`scratch/workers/w3_observer_editor_flying_camera/shadow`, where only the axes2 body is replaced, via
`scratch/workers/w3_physics/sgate.py`). It is w2 `a3` with `if (!valid_real_vector3d_axes2(&observer->forward, &observer->up))`.
- The helper inlines; 1568/40 sha-only; the 24 exact siblings are kept.
- The axes dot product order becomes i,k,j against January's k,i,j (a3 gives k,j,i).
- The repair cross-product load orders shift.
- It is not closer than a3, so there is no header proposal.

**Classification.** An x87 reassociation / operand-rank tie. The same inner-pair order difference also appears in the
inlined `normalize3d` magnitude.

**Reopen.** Unchanged from w2: a measured VC7 rule for the three-term sum rank in context. Start from
`scratch/workers/observer/pos_a3.c`.

## Checks

- Final real-file gate identical to the baseline row by row (`obs_final.gate.txt`).
- `parkcheck`: parks 2, drift 0.
- Owner census vs `build/base`: unchanged.
- Fake scan: 0 leads.

## Do-not-repeat (this wave)

- update_command: mixed indexed/pointer IV spellings, refuted by the pre-assert `lea` placement.
- update_positions: the conjunction-header inline axes2 call on the a3 body (not closer).
