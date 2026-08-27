# `input_xbox.obj` Jonas dead-zone and tick helper pair

## Result

This bounded one-shot wave retains two strict January helpers:
`_fix_dead_zone` and `_update_ticks`. Both were byte- and relocation-exact in
the first normal code-producing candidate. The same frozen wave also tested
the private analog hysteresis helper `_code_000be800` and the broad device
poller `_code_000bebd0`; both were nonexact and were removed without a tuning
compile. `_input_frame_begin` was not added because its exact target body calls
the rejected private poller and therefore is not link-closed over exact code.

`input_xbox.obj` advances from 9/27 to 11/27 exact functions and from
133/3,557 to 247/3,557 meaningful code bytes. The strict gain is 114
meaningful bytes, 128 padded bytes, two functions, and zero relocations. The
object remains incomplete and receives no data credit.

| Symbol | Meaningful / padded | Relocations | Normalized SHA-256 | Decision |
| --- | ---: | ---: | --- | --- |
| `_fix_dead_zone` | `75 / 80` | 0 | `9cb7f2e9518611e8eaf13b6f0ebd21cd4da1ddb71bc5485300354f8665ed758b` | retained strict exact |
| `_update_ticks` | `39 / 48` | 0 | `a3153c5a06cb67390d81d7112942a0a0a8f2e485c554823319d45848502b504a` | retained strict exact |
| `_code_000be800` | `38 / 48` | 0 | target `5850a17e99380c5499a719ba1667ef91f992a37284afe5049ba6e7f04e79c826`; candidate `78f85bb297cc663f10c1fcea301827288980a1f2dc6cf99b0012b4fe641a1bb9` | removed |
| `_code_000bebd0` | `1217 / 1232` target; `1680` padded candidate | 28 target; 32 candidate | target `56316233dc2ab683b304fa910b38ac5a82e29e8a13850a48cf31af300c81d580`; candidate `74d9acf5ca8c72cd96c579ca57caec164d6342796688bf6a1e1affd95164f5d3` | removed |

## Evidence and reconstruction boundary

The authoritative byte oracle is the January split COFF at
`build/split/source/input/input_xbox.obj`. Existing repository declarations,
the prior gamepad-state ledger, the target symbol/relocation inventory, and
target disassembly fixed the local ABI and object boundary. The preserved
Stian reconstruction at `research/stian-main:src/halo/input/input_xbox.c`
supplied independent high-level Xbox input semantics, but it is retail-Xbox
reconstruction rather than January source and was not treated as a byte
oracle. HCEA/public repository searches supplied interface and provenance
context only.

The two retained helpers are ordinary defined C:

- `_fix_dead_zone` removes a signed dead range, rescales the positive and
  negative residual intervals to the full signed-short range, and returns
  zero inside the dead zone;
- `_update_ticks` resets a released button to zero and saturating-increments
  a held button at `UCHAR_MAX`.

Their final hardened COFF comparison proves equal padded sections, normalized
bytes, and empty relocation inventories. The retained object SHA-256 before
commit is
`5b48bb01489d209a97986d87b5bffd0f738a9a41b34b9c25c65bda9ffaa50d02`.

The four house-style-only changes to inherited exact functions put `void` on
its own line for no-argument lists, put the mouse-button parameter on its own
line, and add explicit terminal returns to both void stubs. Hardened replay
proves that all nine inherited exact functions remain strict:
`_code_000bebb0`, `_code_000bebc0`, `_input_suppress`, `_input_activate`,
`_input_deactivate`, `_input_get_mouse_state`,
`_input_mouse_button_is_down`, `_input_get_gamepad_state`, and
`_input_frame_end`.

## One-shot rejection record

The frozen evidence wave performed one normal code-producing compile. The
small private hysteresis candidate had the correct 48-byte padded boundary,
private register ABI, control flow, and zero-relocation shape, but differed at
one instruction spelling: the compiler emitted `add al,-32` (`04 E0`) where
January emitted `sub al,32` (`2C 20`). Equal length and semantics do not make
those normalized bytes exact, so the function was removed. No cast,
expression, declaration, or arithmetic-spelling retry was attempted.

The device poller was a broad topology rejection, not a near miss. January is
1,232 padded bytes with 28 relocations; the candidate is 1,680 padded bytes
with 32 relocations. Among the concrete contradictions:

- the first `XGetDeviceChanges` call was at relocation `+25` instead of
  January `+30`;
- direct indexed global access created extra `_bss_004536a0` references;
- the candidate initialized state/raw/handle pointers in a different order
  from January's raw/state/handle sequence at `+541/+548/+555`;
- the candidate placed the `XInputGetState` error block before the success
  body, while January branches to a shared tail error block;
- button-map relocations and helper-call/inlining topology disagreed.

The poller, its XDK/error includes, typed support-only raw storage, private
hysteresis helper, and local read-only button maps were all pruned. A retained
replay was then permitted solely to reproduce the already exact pair after
removing rejected material; it did not alter either retained body. The final
candidate defines none of `_code_000be800`, `_code_000bebd0`, or
`_input_frame_begin`.

## Policy and scope

The retained source uses only readable C89-compatible arithmetic, shared
types/constants, and explicit returns. It introduces no assembly, `register`,
`volatile`, pragma, intrinsic, attribute, optimizer barrier, raw address,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, object patch, comparator exception, or compiler/tool
substitution.

Only `source/input/input_xbox.c` and this ledger are tracked changes. No
shared header, configuration, admission exception, semantic exception, parked
record, completion label, protected source, or concurrent Units work changes.

## Validation

- Direct hardened comparison: the two new helpers and all nine inherited
  functions are strict equal in padded bytes, normalized hashes, relocation
  counts, offsets, types, destinations, and addends.
- Rejected-symbol absence: `_code_000be800`, `_code_000bebd0`, and
  `_input_frame_begin` are absent from the final candidate object.
- Complete `halobetacache_build` and `libcmt_build`: pass.
- Semantic audit: 470 units, 4,341 functions evaluated, 4,195 semantic exact,
  114 hidden exact / 64,705 hidden code bytes, 4,255 accepted exact, and zero
  unit errors.
- Campaign progress: 377/833 complete objects, 4,229/11,060 exact functions,
  and 510,731/2,198,102 meaningful code bytes. Halo contributes 275/468
  objects, 4,062/7,574 functions, and 497,817/1,770,166 code bytes.
- Object admission: zero candidates, zero contradictions, zero revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling: 179/179 unit tests pass; `git diff --check` passes.

No push is performed.
