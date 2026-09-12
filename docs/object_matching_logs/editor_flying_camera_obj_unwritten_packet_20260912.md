# `editor_flying_camera.obj` unwritten camera packet

## Result

This packet reconstructs all four previously unwritten January functions in
`source/camera/editor_flying_camera.c`. January's split object and assertion
metadata are authoritative; HCEA commit
`c168af2e747d3095d9a29418ae401f3a39544863` and Fable commit `740bbc33a`
were used only as semantic references.

The strict object gate advances from 15 exact / 2 residual / 4 unwritten to
16 exact / 5 residual / 0 unwritten. `_editor_camera_move_to_point` is newly
strict for **120 meaningful / 128 padded bytes**. The other three bodies are
complete, readable best-known source but remain fuzzy and receive zero exact
credit.

| Function | Target meaningful / padded / relocs | Candidate meaningful / padded / relocs | objdiff | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_editor_camera_move_to_point` | `120 / 128 / 10` | `120 / 128 / 10` | `100.00000%` | `a9a76937ef6744f3ed0a1dece0ad93e77619b3e70e22f23f0e8e2c9409396958` | same |
| `_editor_camera_set_position_and_roll` | `431 / 432 / 30` | `449 / 464 / 30` | `80.06040%` | `a209aa201db09ef8600e6aa329e259b552acb2fe89532ea3bc18f4c007912b82` | `67b4c733c5db98d9e9c7ca7b21869fa63800ef10c912d24d1adbfc1e101b115a` |
| `_editor_camera_update` | `297 / 304 / 26` | `299 / 304 / 27` | `92.08696%` | `3f3b95ee1bff1748d5b911a1ba0f7c307742058e9bc8b8c7e67649cc3288ed7e` | `32fc230529e51df0ddd6f8a4a35a74b31ca746efeedbc7a654cfdb6d57b8347c` |
| `_editor_camera_set_scripted` | `428 / 432 / 40` | `455 / 464 / 43` | `88.78740%` | `925a9c065ee97f4b157c20efb60ed226b0852123c626eb2e78b6cbbca0a124df` | `8eb8e394df47250950f18742f107706f614e8594d8e3830d9b390a0072b302d4` |

## Semantic reconstruction

- `editor_camera_move_to_point` asserts its point and positions the current
  camera 2.5 world units behind that point along the current facing vector.
- `editor_camera_set_position_and_roll` copies the point, derives yaw/pitch
  and signed roll from the requested rotation matrix, records a focused-unit
  offset, and requests a full camera reset. Its no-camera path deliberately
  copies yaw/pitch into a correctly typed `real_euler_angles2d`; it does not
  alias a `real_euler_angles3d` object through a different pointer type.
- `editor_camera_update` follows January's scripted/free-flight state machine,
  mode hooks, hard-cut flags, and zero blend timer. The flag updates use
  `SET_FLAG` rather than hand-written bit operations.
- `editor_camera_set_scripted` enters or leaves scripted mode, synchronizes
  the render and editor cameras, installs the relative scripted-camera point,
  and reports the transition. Its focused-unit path copies the vector offset
  into a real point rather than representation-punning the vector pointer.

The two clean typed copies explain the extra candidate instructions in the
two larger residuals. They are retained as honest fuzzy reconstruction rather
than using view casts or source-shape pressure to reproduce January bytes.
No `volatile`, register forcing, no-inline annotation, optimizer pragma,
assembly, raw address, or manual object bytes are present.

## Prototype ownership audit

The new editor-camera public declarations are in
`source/camera/editor_flying_camera.h`. The public
`scripted_camera_set_camera_point_relative` declaration was moved from the
implementation-only prototype block to its true owner,
`source/camera/camera_scripting.h`. The header uses forward-declared union
tags, so it does not add a broad math-header dependency.

All three includers were gated before and after the owner move:

| Includer | Before | After | Exact regression |
| --- | ---: | ---: | ---: |
| `camera_scripting.obj` | `13 exact / 0 residual / 0 unwritten` | same | none |
| `director.obj` | `25 exact / 3 residual / 0 unwritten` | same | none |
| `editor_flying_camera.obj` | `16 exact / 5 residual / 0 unwritten` | same | none |

Every emitted function in `camera_scripting.obj` and `director.obj` is
COFF-shape identical before and after the header move. In the editor object,
the only changed body is the already-fuzzy `_editor_camera_set_scripted`:
receiving the correct `void` prototype reduces it from 480 to 464 padded
bytes and improves objdiff from 88.20473% to 88.78740%.

Every focused gate passed the `_point_from_line3d` emitted-symbol guard. The
fake-match scanner reports zero review leads, and `git diff --check` passes
apart from checkout line-ending notices. The root integration subsequently
passed the full Ninja build and zero-regression stable sweep; aggregate
publication evidence is in the sixth 10K batch ledger.
