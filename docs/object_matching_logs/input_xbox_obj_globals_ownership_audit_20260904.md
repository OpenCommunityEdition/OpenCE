# Input Xbox global naming and ownership audit — 2026-09-04

This independent read-only audit recommends replacing the inherited
address-labelled COMMON object with the real `input_globals` owner. The root
accepts the static-linkage inference below; the source owner and root handle
source/configuration changes and verification. This reviewer changed only this
ledger, performed no compilation or build, and claims no admitted code/data
gain here.

## Name and linkage evidence

The name **`input_globals` is directly authenticated by January assertion
literals**, including `input_globals.gamepad_handles[gamepad_index]`,
`input_globals.keyboard_handle`, and the buffered-key-index expression. The
source/header census `rg -l '\binput_globals\b|bss_004536a0' source -g '*.c'
-g '*.h'` found only `source/input/input_xbox.c`.

The later HCEX PDB directly describes `input_globals` as file-static. Exact
PowerShell command, runnable without modifying the repository:

```powershell
& 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe' -sym input_globals 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb'
```

The relevant output is:

```text
DataKind: 0x5
LocationType: 0x1
AddressSection: 0x5
AddressOffset: 0x2A35E8
RelativeVirtualAddress: 0x16D35E8
Data: static, [016D35E8][0005:002A35E8], File Static,
      Type: struct input_globals_xbox, input_globals
```

This is linkage/name corroboration, **not authority to import the later type's
layout**. The pinned HCEA `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
`src/headers/input_globals.h` and `src/headers/input_globals_xbox.h` use the
same name but a materially different later representation. Keep January's
existing authenticated 0x40C-byte structure.

## January debug records: honest inference boundary

January PDB:
`C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb`.

From the canonical root, the bounded module inspection was:

```powershell
python '..\..\research\pdb200\inspect_module.py' tools/pdb200_extract.py 'C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb' input_xbox
```

Module stream 286 is `\halo\objects\halobetacache\input_xbox.obj`, with
120 symbol bytes. It contains only one OBJNAME record (`0x0009`) and one
COMPILE record (`0x1013`). It retains **no LDATA/GDATA declaration proving this
global's January source linkage**.

The extracted January symbol inventory is
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\pdb200\pdb_symbols.json`,
SHA-256
`7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
Reproduce the bounded name/address census from the canonical root:

```powershell
python -c "import json;from pathlib import Path;s=json.loads(Path('../../research/pdb200/pdb_symbols.json').read_text());print([x for x in s if x['name'] in ('input_globals','_input_globals') or 4535744 <= x.get('rva',0) <= 4537008])"
```

Neither spelling nor any named symbol at decimal `4535968` appears. The nearby
public symbols do appear: `_input_abstraction_globals` at `4535744`,
`_hud_globals` at `4537004`, and `_hud_scripted_globals` at `4537008`.

Together with January's name-bearing assertions, the single-TU reference
census, and HCEX's explicit File Static record, this strongly supports a
private January owner. **Static linkage remains a triangulated inference**,
not a claim that the stripped January PDB retained its private declaration.
The current synthetic `_bss_004536a0` split symbol has external storage because
its manually supplied configuration record omitted static metadata; that
default is not independent original-linkage evidence.

## Exact target storage boundary and proposed reconstruction

Before renaming, `build/split/source/input/input_xbox.obj` has SHA-256
`a956f90624ce49ef7ecbdaf6005c5f008695170ebbee05687ed0dd08e4316457`.
Its `_bss_004536a0` owner is offset zero of section 47:

| Property | January split |
| --- | --- |
| Start / exclusive end | `0x4536A0` / `0x453AAC` |
| Logical BSS extent | `0x40C` = 1,036 bytes |
| Section flags | `0xc0400080` |
| Relocations | zero |
| Normalized zero-storage SHA-256 | `693195cf289838146418e1bd05fd1a482c36ff75a77874609d615247285d5b99` |

The old built candidate instead represents the name as section zero, value
1036, storage class 2: a tentative COMMON allocation, not a defined BSS owner.

Approved ordinary source recommendation:

```c
static struct input_globals input_globals = {0};
```

Remove `#define input_globals bss_004536a0` and rename the remaining direct
address-labelled accesses; do not add a wrapper structure, union facade,
padding member, alias variable, fake reference, or new public extern.
The root's configuration proposal is solely:

```json
{"file_offset":4535968,"flags":0,"name":"_input_globals","static":true}
```

No contribution boundary or target bytes should change. The renamed candidate
must own one static/class-3 zero-filled BSS section of the same 1,036-byte
extent, with no residual COMMON owner or duplicate allocation. Preserve all
16 inherited exact functions, every already-frozen poller result, and the
no-`point_from_line3d` emission rule in the renamed-target scratch gate and
full dependent regression sweep. Do not weaken comparison or vary unrelated
declarations if changing the storage owner exposes a compiler-context issue.
Production build/report/park validation and publication remain root actions.
