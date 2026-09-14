# `model_animations.obj` Opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/models/model_animations.c`; January target
  `build/split/source/models/model_animations.obj`.
- Compiler: XDK 3911 VC7 `CL.Exe` 13.00.9254 (`xbox/bin/vc7`), repository flags
  `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus the generated include list.
- Lane `opus/100k-consolidated-20260914`, worker claim `waveC:model_animations`.
  **The real source file was not modified.** Every remaining non-exact row is
  parked and locked, so all work was scratch-only park re-investigation, following
  the brief's park re-investigation protocol. Candidates live in
  `scratch/workersC/model_animations/`.
- Evidence: January COFF (code, relocations, `.rdata` assert literals, frame
  sizes); prior wave-A notes `scratch/workers/model_animations.md` (the `movzx`
  frame-index finding, `kf_s3b.c`); HCEA `src/blam/models/overlay_animation_apply_continuous*.c`
  and `animation_get_keyframe_*.c` (semantics only; HCEA composed the continuous
  bodies from siblings, so they are not layout evidence); Stian
  `models/model_animations.c:1011` (mislabelled, no evidence); Ghidra
  `scratch/ghidra/out/model_animations.obj.decomp.c:1692`.

## 2. Validated baseline

```text
python -B tools/campaign/gate.py source/models/model_animations --all --forbid-emitted-symbol _point_from_line3d
== exact 26  residual 6  unwritten 0  (of 32 listed)
```

6,992 / 12,144 padded code bytes exact. All six residuals are parked:

| function | target padded/meaningful | base padded | relocs | base nsha16 | target nsha16 | first divergence (baseline) |
|---|---:|---:|---:|---|---|---|
| `_animation_get_keyframe_rotation` | 784/779 | 768 | 43 | b07863488ad7253e | a533276c9248c748 | signed `short` frame-index reads (16 B short) |
| `_animation_get_keyframe_translation` | 768/759 | 752 | 37 | 2261009b94bd607e | 2a4ed003246c4136 | same |
| `_animation_get_keyframe_scale` | 752/742 | 736 | 37 | 013ee0c2d8e47636 | 842fdd10dc6dcd11 | same |
| `_inverse_kinematics_adjust_matrices` | 864/852 | 864 | 7 | d9fc11bebc1ea09f | e1cd1ac47b76f423 | x87 operand order, 2nd shoulder cross product |
| `_overlay_animation_apply_continuous` | 960/953 | 960 | 34 | 9f9b2604c3c535b6 | 5ffa1215f54cc536 | frame `sub esp,0x70` vs `0x7c` |
| `_overlay_animation_apply_continuous_scaled` | 1024/1012 | 1008 | 37 | a35ac9998b7fb96b | 345a817b00662af3 | frame `sub esp,0x74` vs `0x80` |

## 3. Accepted controls (for the orchestrator; NOT landed, parks locked)

Park-reopen candidate: `scratch/workersC/model_animations/reopen_final.c`
(patch `reopen_final.patch`). Whole-TU gate `== exact 31 residual 1 unwritten 0`.
The five reopened functions reach exactly the `measurements.target` size, relocations
and normalized hash recorded in `config/parked.json`. The 26 baseline-exact rows are
unchanged (per-row listing diff), the IK park does not drift, the `_point_from_line3d`
guard passes, the owner census only replaces our space-less assert literal with
January's literal, fake scan finds 0 leads, and the `/W3` warning list is unchanged (31).

### 3a. Keyframe getters (rotation, translation, scale)

All four facts come from January bytes:

1. **Frame-index stream type `word const *`.** January reads every element with
   `movzx` (or `xor ecx,ecx; mov cx,[..]; movzx`) and compares it in 32 bits
   (wave A). The exact `animation_keyframe_search` keeps `short const *` (16-bit
   `cmp ax, word ptr`). The implicit `word const *` to `short const *` argument
   conversion raises no `/W3` warning.
2. **Assert text.** January `.rdata` owns
   `??_C@_0DC@FHNJEFLD@real_frame_index?$DM?5?$CIreal?$CJnext_key@`, which is
   `real_frame_index< (real)next_keyframe_frame_index` (50 bytes, with a space). All
   three getters reference it at their line 1492/1584/1673 asserts. The old source
   emitted a 49-byte literal without the space.
3. **Prologue order.** January loads the default-value offset and emits
   `add edi,esi` before `shr ebx,0xc`. Each getter therefore declares
   `default_<x>` directly after `header`, then `node_header`,
   `first_keyframe_index` and `keyframe_count`. All three use the same order.
4. **Branch statement order.** January schedules a keyframe's frame index before
   the keyframe value. In rotation/translation branch 2, `lea ebx,[ecx+1]` comes
   before `lea edi,[edi+eax*2]`. In branch 3, the cx/bx frame-index loads come
   before the pointer leas. In scale branch 1, `mov ebx,eax` comes before the
   `keyframe_scales[0]` load. A single order in all 9 branches of the 3 getters
   reproduces all three functions:
   `this_keyframe_frame_index; this_keyframe(_scale); next_keyframe_frame_index; next_keyframe(_scale)`.

### 3b. Continuous overlays (both)

By the frame-layout lever, the 12-byte frame gap in both functions is structural.
January's uncompressed paths show what is missing:

- Scale: `mov eax,[edi]; mov ecx,[ebx]; mov [ebp-0x40],eax; ...; add edi,4; mov [ebp-0x3c],ecx; add ebx,4; call _scalars_interpolate`.
  These are two `real` locals with stack homes, and each stream is advanced
  right after its own read, before the call.
- Translation: `mov ecx,ebx; mov eax,edi; add edi,0xc; add ebx,0xc; call _points_interpolate`.
  These are pointer copies, with both streams advanced before the call.
- The rotation path in the source already reads and then advances each stream
  (`decompress(data,&this_rotation); data += ...; decompress(next_data,&next_rotation); next_data += ...`).

Retained shape, used in the translation and scale paths of both functions:

```c
real this_scale;
real next_scale;

this_scale = *(real const *)data;
data += sizeof(real);
next_scale = *(real const *)next_data;
next_data += sizeof(real);
scalars_interpolate(this_scale, next_scale, fraction, &scale);
```

The translation path uses `real_point3d const *this_translation/next_translation`
in the same way. The candidate also re-indents the mis-indented loop body of
`overlay_animation_apply_continuous`. This is whitespace only: the per-function
object comparison against the un-indented candidate showed 0 differences across 44 symbols.

## 4. Experiment matrix

| candidate | change | result |
|---|---|---|
| kf0 | wave A `kf_s3b.c` + H1 `long` parameter | getters 784/768/752; about 9/9/4 differing insns |
| a1 | + January assert literal (space) | literal relocation fixed |
| t1/t2 | prologue permutations | translation: default-first (7); rotation: default before first/count (7); the other orders were much worse (67-71) |
| c1 | t1 translation + t2 rotation | 7/7/4 |
| b2 | rot/trans branch 2: next frame index before next keyframe | 5/5/4 |
| b3 | + branch 3: frame indices before pointers | 3/3/4 |
| k1 | `next_keyframe = this_keyframe+1` | neutral |
| u1 | scale branch 1: both frame indices first | worse (19) |
| u2/u4/u5 | scale branch 1: this, this_fi, next_fi, next | **scale EXACT**; branches 2/3 insensitive |
| perm p00-p23 | diagnostic: 24 orders of rotation branch 3 | EXACT iff this_fi comes first and next_fi is not second (p02-p05) |
| uni | uniform `this_fi, this, next_fi, next` in all 9 branches | **all three EXACT** |
| uni_rd (= kf_reopen) | + rotation default-first like the others | **all three EXACT** (chosen: uniform prologue) |
| o_t / o_s / o_S | continuous: decl+init copies for translation only / scale only / scale after call | sizes 944/976/1008 etc.: wrong |
| o_ts | decl+init copies in both paths, then both advances | frames 0x7c/0x80, sizes equal; 1 insn misplaced |
| o2_n | advance next_data first | residual |
| o3 skip-T / skip-S | interleaved shape in one path only | sizes wrong |
| **o2_i** | interleaved read/advance per stream, both paths, both functions | **both EXACT** (also exact on the real file alone: `ob_i.c`, 28/4/0) |
| ik1 | shoulder block named like the elbow block (forward, up; `&shoulder_matrix->left`) | still [sha]; second cross 1 term off, first cross 2 |
| ik2 / ik3 | all three shoulder basis pointers named (two declaration orders) | worse; the elbow block also drifts |
| reopen_final | o2_i + whitespace re-indent | identical objects; 31/1/0 |

## 5. Do-not-repeat list

- Signed `short` frame-index pointers in the getters (January uses `movzx`).
- Assert text `real_frame_index<(real)next_keyframe_frame_index` without the space.
- Treating the continuous overlays as a register tie: the frame gap is structural and is now fixed.
- Continuous overlays: declared-and-initialized this/next copies followed by both
  advances (one instruction out of place); copies in only one of the two paths;
  advancing `next_data` first.
- IK: renaming or adding shoulder basis pointer locals (ik1-ik3).

## 6. Residual classification

- Keyframe getters, continuous overlays: **closed in scratch** (measured strict exact).
  The authenticity argument is one uniform statement order per idiom across 3 or 2
  sibling functions, and it is consistent with January's own rotation-path idiom.
  Inference: the 24-order permutation run was diagnostic only. The chosen order is
  the single uniform pairing, not a per-branch pick.
- `_inverse_kinematics_adjust_matrices`: **scheduling tie** (measured). The only
  difference is the x87 commutative operand load order of 4 terms in the inlined
  second shoulder `cross_product3d`. Three shapes were tried. Stopped.
- Brief section 9 levers (inline frequency, x87 phi, FPU argument) do not apply:
  no out-of-line helper or argument-routing difference exists in these residuals.

## 7. Reopen criteria

- The keyframe getters and both continuous overlays meet their reopen criteria now.
  The orchestrator should unpark the five entries and land the five bodies from
  `scratch/workersC/model_animations/reopen_final.c`, together with the literal fix
  in the same three getters. This is source-only; no header or symbols.json change
  is needed.
- IK: new evidence about January's inlined `cross_product3d` term or operand order
  (for example a January-era `real_math.h`) that fixes the shoulder block without
  perturbing the elbow block.

## 8. Disposition

No strict-exact credit is claimed in this wave (real file unchanged; parks locked).
Final real-file gate:

```text
python -B tools/campaign/gate.py source/models/model_animations --all --forbid-emitted-symbol _point_from_line3d --out scratch/workersC/model_animations/final.obj
== exact 26  residual 6  unwritten 0  (of 32 listed)
```

Park drift: PASS for all six. Park-reopen proposal: five functions,
4,288 padded / 4,245 meaningful bytes, candidate gate `== exact 31 residual 1 unwritten 0`
(11,280 / 12,144 padded code bytes).
