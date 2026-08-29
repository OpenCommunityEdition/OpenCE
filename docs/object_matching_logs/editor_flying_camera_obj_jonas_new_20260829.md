# `editor_flying_camera.obj`: editor-camera constructor recovery (2026-08-29)

## Result

`_editor_camera_new` is newly strict-exact on the first and only frozen
production candidate compile. It contributes one function and 208 padded code
bytes. `source/camera/editor_flying_camera.obj` advances from 9/21 to 10/21
strict functions and from 448/5,184 to 656/5,184 strict padded bytes. The
object remains `NonMatching`; this wave claims no data, completion label,
semantic exception, parked function, or compiler-control credit.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_editor_camera_new` | 208 | 19 | `31a8ddca3286c30c9c3849506fd135150232c54418cf2255fcf884bf51681039` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with the January target. All nine inherited strict
functions in the object also remain exact.

## Authority and natural reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show the complete natural constructor topology:

- test the typed initialized byte at `_bss_0031d438+2`;
- when uninitialized, test the scenario players block count and address, then
  retrieve its first 0x34-byte element through
  `tag_block_get_element_with_size`;
- copy the starting point and facing into the persistent camera focus, or
  clear the complete 20-byte focus when the block is empty;
- mark the focus initialized, derive the forward vector from its two angles,
  and call `flying_camera_new_from_point_and_vector`;
- retain the camera pointer for local player zero; and
- when the editor mode is nonzero, invoke the typed mode transition table's
  incoming hook.

Independent semantic and layout evidence comes from:

- `C:/Users/isabe/Documents/Codex/reference/halocea/src/editor_camera_new.c`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/scenario.h`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/flying_camera.h`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/editor_camera_focus_t.h`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/data/translate_funcs.c`;
  and
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/data/update_funcs.c`.

HCEA confirms the spawn-seeding semantics, camera type, focus type, and the
2-by-2 transition table. January remains authoritative where the Xbox donor
differs: the PC fallback clears all 20 focus bytes, exactly as the PC call to
`csmemset` and its size argument show. No foreign address is imported.

The production source uses the repository's `struct scenario`, tag-block
access macro, `struct flying_camera`, and vector/camera helpers. A local typed
0x34-byte player-start record names the fields used by the constructor. The
existing grouped BSS declaration now uses the canonical flying-camera pointer
type. The dispatch envelope anchored at `_editor_custom_render` names its
custom-render pointer, two typed update hooks, and the independently attested
2-by-2 transition hooks; a compile-time assertion fixes the transition table
at offset 0xC. Consequently `[mode][1]` naturally produces January's final
`DIR32 _editor_custom_render+0x10` relocation without pointer arithmetic or a
representation trick.

The accompanying public prototype exposes the authenticated
`(struct flying_camera *, short)` signature. Updating the inherited roll
setter from the former private mirror type to `camera->facing.roll` preserves
its exact bytes and gives both functions one canonical camera representation.

## One-shot and policy boundary

The HCEA-confirmed body, the three repeated `global_scenario_get()` accesses,
the full-focus fallback clear, and the typed dispatch layout were frozen from
the January instruction and relocation evidence before one normal Ninja
candidate build. The candidate was strict immediately. No spelling sweep,
source-shape retry, compiler flag, comparator exception, or object editing was
used.

The retained implementation is readable C89-compatible C with one parameter
per line and an explicit terminal `return;`. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw address or
offset, pointer/integer reconstruction, cast or union pun, inactive-union
access, undefined behavior, synthetic anchor, or byte-forcing expression.
No repository `AGENTS.md` or `CLAUDE.md` applies in this worktree; the existing
camera ledgers and repository campaign tooling documentation were followed.

## Validation

- Canonical starting commit:
  `adc06ea07855660d14fd5a4a457c957b46733f0c`.
- January `editor_flying_camera.obj` SHA-256:
  `5e0066e8f28b6c2c79e9e546075922b263342dea5a50d9ef84e8818792d99945`.
- Final rebuilt `editor_flying_camera.obj` SHA-256:
  `9d0341035d04bb23b3963fa9110e43969a4cfd094ca9a1be532112889f27136d`.
- Retained source/header Git blobs:
  `473a74da467d7074b43445df7722ba045730fd37` and
  `131ee7526499b50255d10783a0d6964c845f77a8`.
- Complete `halobetacache_build` and `libcmt_build`, ordinary report,
  semantic report, and strict progress generation: pass.
- Strict campaign board: 276/619 whole objects, 4,710/8,246 functions, and
  652,631/1,922,669 padded code bytes. This wave changes only the latter two
  totals by one function and 208 bytes.
- Semantic audit: 470 units, 4,855 functions evaluated, 4,728 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,738 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `0c54111d1f72423a70372916ee8955fe0ea30d0df11cf64ee4f067df85803e9f`.
- `build/semantic_report.json` SHA-256:
  `05c3ca44169b4823d9dde3213d46fae9c2caf8a2218db1711f0d73282b2ec894`.
- `git diff --check`: clean apart from Git's checkout line-ending notices.
