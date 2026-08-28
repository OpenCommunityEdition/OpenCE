# `virtual_keyboard.obj` residual one-shot — 2026-08-28

## Scope and provenance

- Lane base: `9c006fff3ae73abaa3d83ca208aa20ab90785163`
- Target object SHA-256: `9fe76d4242f8bed88da28e037005dc476167363984b09ed3bd6af00790244a4b`
- HCEA donor repository: `research-cache/halocea-cseries-20260820` at `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
- Donors: `src/blam/interface/virtual_keyboard_select.c` (blob `e955705049304a3c2723237e214ceb696fb0b968`), `virtual_keyboard_render_internal.c` (`3faef697514b094368524413a62a40727c7ce396`), `virtual_keyboard_render_text_box.c` (`170cc55b62b5184daa0d181e2b3eb8fa17e73f27`), and `virtual_keyboard_process_internal.c` (`e88c2ea8107bbb0d77f190598be877004b74b0c4`).

Target disassembly corrected the donor-label mapping: `_code_000e50b0` is the
renderer, including the inlined text-box path; `_code_000e57a0` is select; and
`_code_000e5be0` is process.

## Frozen one-shot gate

One natural typed-C batch was compiled. No spelling, control-flow, or
optimization retries were made. The ephemeral candidate raw object was
`scratch/_gate_32708.obj`, SHA-256
`0cbc4f2de599924454605c8894e1b10644047b0d7dd36ffccccefc67b415321c`;
it was removed with the rejected batch after its fingerprints were recorded.

| Function | Target size / relocs / normalized SHA-256 | Candidate size / relocs / normalized SHA-256 | Result |
| --- | --- | --- | --- |
| `_code_000e50b0` | 1616 / 95 / `f52389e69e9dbb4f8cbeaf41e8ad788704fec98dc5fbff916156df5285960425` | 1568 / 87 / `e0f2654cc84c926dde2b408c7e5311b79ee58358a352956cdbd4fb821525a258` | rejected |
| `_code_000e57a0` | 1088 / 123 / `28fa08bf4fb555466006d076f32c672ca72ddd07a165e4c51e1cb54f02a0e141` | 1120 / 123 / `23b8d854478f93e35882ac24aced508614cf6c98f8ceb71f2373fefb041bd02c` | rejected |
| `_code_000e5be0` | 848 / 72 / `21ed27cb544a6130f43b2ca43956ab9a748fc2f881469c0ead2c60a144f8cad8` | 864 / 72 / `45ad982ceff498f5b21fe55ea9b601ddb3451a8233e2966145e2b99488e15df1` | rejected |

The renderer also naturally emitted a named rectangle-table owner instead of
the target's `_data_00306338` / `_data_00306498` ownership topology, but code
and relocation counts already differed before ownership could be decisive.

## Cleanup and replay

All three rejected bodies and their support declarations, structure
refinements, and initialized rectangle table were removed.
`source/interface/virtual_keyboard.c` hashes to the base blob
`16a3584b7717279fb91b061c8e47daf9ca0c1646`. Final strict replay is
`17 exact, 0 residual, 3 unwritten`. No production source, header,
configuration, Units, Vehicles, or Claude-active file survives changed by this
lane.
