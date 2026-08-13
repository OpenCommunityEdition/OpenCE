# `following_camera.obj` exact-match log

Target: January 14, 2002 Xbox build. Compiler: XDK 3911 VC7 13.00.9254.1 with the repository's unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags.

## Baseline

- `_following_camera_new`: strict exact, 96 bytes, 4 relocations, normalized SHA-256 `a76acdb80fe34f1fb6ea7dc0e0165c82f5c6893bc6b973eedcc25941fdd3adc4`.
- Seven target functions were absent: `_code_00077f20`, `_arcsine`, `_uniform_cubic_spline`, `_uniform_cubic_spline_vector3d`, `_code_00078120`, `_following_camera_deterministic`, and `_following_camera_update`.
- `_following_camera_zoom_levels` is a 16-byte externally-owned `.data` array containing `31.29f`, `12.78f`, `5.13f`, and `2.05f`.

## Evidence and source foundation

- Exact-name HCEA routines provided control-flow and readable topology for the camera lookup, track spline, deterministic camera placement, and per-frame update. All types, offsets, calls, assertions, constants, relocation destinations, and private calling conventions are independently adjudicated against the January COFF/disassembly before admission.
- The camera-track reference element is 0x1c bytes in January (`tag_reference` plus 12 reserved bytes); camera-track control points are 0x3c bytes.
- `uniform_cubic_spline` owns the January `real_math.h` assertions at source lines 1508 and 1509. The vector helper calls the scalar helper once for each component.

## Measured shapes

This section is append-only. Each compile experiment records the exact strict result, first divergence, and whether it was retained or reverted.

### Experiment 1 — initial HCEA-topology reconstruction

- Strict exact immediately: `_following_camera_new` (96/96, 4/4), `_code_00077f20` (128/128, 5/5), and `_arcsine` (16/16, 1/1).
- `_uniform_cubic_spline`: 224/224 and 11/11 but x87 expression scheduling differed. Retained structure and replaced the compact algebra with the readable forward-difference locals visible in the reference topology.
- `_uniform_cubic_spline_vector3d`: 144 target versus 112 candidate and 3 versus 1 calls. The compiler retained the loop; replaced it with three readable component assignments.
- `_code_00078120`: 336 target versus 320 candidate, 17/17.
- `_following_camera_deterministic`: 208 target versus 224 candidate, 7/7. The separate inverse-magnitude local forced x87 spills; changed to reuse `horizontal_magnitude` as the reciprocal and scale the two planar components in place.
- `_following_camera_update`: 1584/1584 and 59/59, with normalized bytes still different.

### Experiment 2 — scalar spline expression and deterministic x87 lifetime

- Three explicit component calls made `_uniform_cubic_spline_vector3d` strict exact: 144/144, 3/3, SHA-256 `4e7df7e991c0e6daab8f07435174105497dfacdce920bffe0ce58170047e343e`.
- Reusing `horizontal_magnitude` improved the deterministic x87 lifetime but it remained 224 bytes versus 208 target.
- The readable `d0/d1/d2` scalar form remained 224/224 and 11/11. The target evaluates the three adjacent first differences in descending source-operand order, so the next bounded variant changes only their statement order to `d2`, `d1`, `d0`.

### Experiment 3 — in-place finite differences

- Independent locals did not induce January's parameter-home reuse. Rejected.
- Mutating `f3`, `f2`, and `f1` through successive forward-difference passes reproduced the target byte-for-byte through offset `0xA2`; only multiplication/division grouping remained different.
- Reassociated the third-difference term from `delta / (3*h) * difference` to the semantically equivalent and January-ordered `delta * difference / (3*h)`.

### Experiment 4 — spline middle-term grouping

- The third-difference grouping reproduced the target through `0xBF`; the remaining difference was division before multiplication in the middle term.
- Changed `delta / (2*h) * inner` to `delta * inner / (2*h)`. `_uniform_cubic_spline` is now strict exact: 224 bytes, 11 relocations, normalized SHA-256 `bf67145760bf0b619b492a2a9eadb82e313628e5613f0592aa7cbdccdec7681f`.
- For the deterministic function, the target squares the already-loaded `forward_x` and `forward_y` locals. The candidate reloaded the struct fields because the expression named them again; the next shape uses the locals consistently.

### Experiment 5 — deterministic local reuse and NaN-aware predicate

- Reusing `forward_x`/`forward_y` in the magnitude expression reduced `_following_camera_deterministic` from 224 to the exact 208-byte size with 7/7 relocations.
- Every instruction then matched except the floating comparison branch: target `test ah,5; jnp`, candidate `test ah,1; jne`. The target's form is the project's established NaN-aware negated predicate, so `fabs(x) >= epsilon` is respelled as `!(epsilon > fabs(x))` without changing ordinary finite-value behavior.

### Experiment 6 — deterministic predicate and track sampler topology

- The negated comparison made `_following_camera_deterministic` strict exact: 208 bytes, 7 relocations, normalized SHA-256 `318a7028b460ab6e36d06e5fac054405b3d5404cc00371d5ca627967a8883d9e`.
- January's track selection computes `MIN(0, count-1)` and explicitly tolerates a null returned element; the initial HCEA decompilation's `1 & (count-1)` interpretation was disproven by the target instruction sequence.
- January retains both the full converted frame index and a short clamped index, and reloads the tag-block count inside the loop. Added those distinct readable locals and reordered `t`, frame, and `h` calculations to match the measured dependency chain.

### Experiment 7 — track sampler and update position copy

- The measured track-sampler topology made `_code_00078120` strict exact: 336 bytes, 17 relocations, normalized SHA-256 `8a2b4279a34d605628acca2f74cade2ee9cae8f3e39f0eae745f3e8076e9bb0d`.
- `_following_camera_update` first diverged in the camera-info position transfer. Three explicit scalar assignments and `set_real_point3d` both regressed January's load/store schedule; both shapes were rejected and the original aggregate assignment was restored.

### Experiment 8 — update local aliases and crouch predicate

- Three scalar position assignments regressed the copy schedule; `set_real_point3d` regressed it further. Both were reverted.
- Removing the redundant `unit_index` and `seat_index` aliases and addressing `camera_info` directly made the update byte-identical through the camera-presence branch at offset `0xBF`; the aggregate position assignment is the correct source shape.
- The next difference is January's branch-materialized boolean for crouch/jump. The combined mask produced `setne`; two readable `TEST_FLAG` terms joined with `||` express the original predicate topology.

### Experiment 9 — transition timer operands and facing aggregate

- The two `TEST_FLAG` terms reproduced January's crouch boolean exactly.
- Reversing both `MAX` operands to put the literal first reproduced January's transition-timer load/compare/selection order and made the update identical through the facing-angle fetch at `0x17D`.
- January copies the returned two-angle aggregate into the local and then increments its fields. The pointer-plus-expression spelling was optimized directly from memory and rejected; use `facing = *player_control_get_facing_angles(...)`, followed by `+=` on yaw and pitch.

### Experiment 10 — pitch lifetime and assertion ownership

- A separate `facing.pitch += camera->facing_offset.pitch` statement forced an intermediate `fsts` spill before the clamp. Combining the addition into the readable `PIN(facing.pitch + camera->facing_offset.pitch, -_pi/2, _pi/2)` expression retained the x87 sum and made every normalized code byte exact.
- Code bytes, size, and relocation addresses were then exact, but the hardened comparator correctly rejected the function because the reconstructed assertion stringized `command->forward` while January owns the literal `result->forward`. The predicate is unchanged; `match_vassert` supplies the January-authored diagnostic literal explicitly and preserves readable typed access through the local `command` alias.
- `_following_camera_update` is now strict exact: 1584 bytes, 59 relocations, normalized SHA-256 `6364564632b5ad32b02133b5921d64c52d18f76b67dde777c42601f49963b46c`.

## Final strict object result

| Function | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_following_camera_new` | 96 | 4 | `a76acdb80fe34f1fb6ea7dc0e0165c82f5c6893bc6b973eedcc25941fdd3adc4` |
| `_code_00077f20` | 128 | 5 | `3a49cfb40f78852b932f354ecc308b983498bcdf3f2cdd1434fec01e025031a9` |
| `_arcsine` | 16 | 1 | `bbe139825c1f5012bc08ffc81ed8a634feef80fe809ac3c6959eee118ef7edf5` |
| `_uniform_cubic_spline` | 224 | 11 | `bf67145760bf0b619b492a2a9eadb82e313628e5613f0592aa7cbdccdec7681f` |
| `_uniform_cubic_spline_vector3d` | 144 | 3 | `4e7df7e991c0e6daab8f07435174105497dfacdce920bffe0ce58170047e343e` |
| `_code_00078120` | 336 | 17 | `8a2b4279a34d605628acca2f74cade2ee9cae8f3e39f0eae745f3e8076e9bb0d` |
| `_following_camera_deterministic` | 208 | 7 | `318a7028b460ab6e36d06e5fac054405b3d5404cc00371d5ca627967a8883d9e` |
| `_following_camera_update` | 1584 | 59 | `6364564632b5ad32b02133b5921d64c52d18f76b67dde777c42601f49963b46c` |

- `_following_camera_zoom_levels` is strict exact as an externally owned 16-byte `.data` section with no relocations and SHA-256 `7db52fa8b18dce563a6fca57cbd9009a91097b2151e86812e5135c090dbb045b`.
- All eight other January-owned `.rdata` sections are exact in size, content, linkage, and ownership. Candidate-only compiler COMDATs are ordinary header-instantiation artifacts and do not replace or alias a January-owned destination.
- The whole target-owned object is strict exact under `section_infos_equal` after rebasing onto campaign commit `ea55318e`.
- Full `ninja halobetacache_build`: success, all 466 source-object actions completed.
- `ninja progress`: success. The strict semantic audit scanned 379 units and accepted 3,171 exact functions with zero unit errors; campaign progress advanced from 252 to 253 completed Halo objects and from 3,159 to 3,166 credited Halo functions. The added meaningful code is 2,605 bytes, exactly the seven functions that were absent at baseline.
- A clean post-admission `tools.regression_gate` snapshot/check for `source/camera/following_camera` passes with all eight functions `still_exact`, no failures, and no warnings. The previously exact constructor was also compared directly before and after the reconstruction and retained identical size, relocations, and hash.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 173/173 tests pass.
