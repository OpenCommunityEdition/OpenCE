# `input_xbox.obj` lifecycle `+0x230` evidence boundary, 2026-09-04

## Scope and disposition

This read-only follow-up starts from published canonical commit
`4c862ae20ddee514c75e73adc196d6a89260b288`. That commit retains the verified
InputXbox public-key wave at 20 exact, four parked, and three unwritten
functions. The only remaining owners are the coherent initialization cluster:

| January owner | Padded bytes / relocations | Normalized SHA-256 |
| --- | ---: | --- |
| `_input_initialize` | 256 / 22 | `cf5e2e92a28cbbf77bea5849fe3771d78c9a3aaebe152b3abac243e465870a4c` |
| `_code_000bf0a0` | 176 / 8 | `f473cb2d504d90b7ac4014fa2d05f189e9ef27ace087fd7fb23d69a6c9c19949` |
| `_code_000bf540` | 32 / 3 | `bb9c454c6ca60f04e12b08b0ebe14f2a43c22eb08e8488d72cf2e13a9ab563d0` |

The lifecycle cluster remains **unwritten**, not parked. Its behavior is mostly
understood, but the field at `_input_globals+0x230` has no recoverable semantic
name or declared type. Adding an address-derived member, casting into reserved
bytes, or omitting the observed write would violate the source-fidelity rules.
No C, header, configuration, park, build, or symbol metadata was changed by
this audit.

## Authoritative January all-object census

The relocation-aware census covered every one of the **833** January split
objects. It found 80 relocations to `_input_globals` in total and exactly one
whose section-relative destination is `+0x230`:

```text
objects 833 input_global_refs 80 plus_230_hits 1
('build\\split\\source\\input\\input_xbox.obj', 27, 153, 6, 47, 0, 560,
 ['.text', '_input_initialize'])
```

The sole reference is at relocation offset `+0x99` in
`_input_initialize`. Disassembly proves that it is a four-byte zero store, not
a load:

```text
0090  c6 05 39 01 00 00 01  mov byte ptr [0x139], 1
0097  c7 05 30 02 00 00 00 00 00 00  mov dword ptr [0x230], 0
00a1  c7 45 f0 07 00 00 00  mov dword ptr [ebp - 0x10], 7
```

The earlier `csmemset(_input_globals+0x138, 0, 0x2D4)` already covers this
address. January nevertheless emits the explicit second zero after creating
the event/thread and before initializing the keyboard-queue parameter block.
That establishes an aligned four-byte **access boundary**, but one write and
zero reads cannot distinguish an original `long`, `DWORD`, enum, pointer, or
other four-byte state, and cannot authenticate a member name or purpose.

Run the census from the canonical root with:

```powershell
@'
from pathlib import Path
import struct
import sys

root = Path.cwd()
sys.path.insert(0, str(root))
from tools import coff_compare

paths = sorted((root / "build" / "split").rglob("*.obj"))
references = []
hits = []
for path in paths:
    obj = coff_compare.load(path)
    for section in obj["sections"]:
        raw = coff_compare._section_bytes(obj, section)
        for index in range(section["reloc_count"]):
            offset = section["reloc"] + index * coff_compare.RELOC_ENTRY_SIZE
            address, target_index, kind = struct.unpack_from(
                "<LLH", obj["data"], offset)
            target = obj["by_index"][target_index]
            if address + 4 > len(raw):
                continue
            addend = struct.unpack_from("<i", raw, address)[0]
            if target["name"] != "_input_globals":
                continue
            references.append((path, section, address, target, addend))
            if target["value"] + addend == 0x230:
                owners = [
                    symbol["name"] for symbol in obj["symbols"]
                    if symbol["section"] == section["index"]
                    and symbol["value"] == 0
                ]
                hits.append((
                    str(path.relative_to(root)), section["index"], address,
                    kind, target["section"], target["value"], addend,
                    owners[:6]))

print(
    "objects", len(paths),
    "input_global_refs", len(references),
    "plus_230_hits", len(hits))
for hit in hits:
    print(hit)
'@ | python -
```

The exact instruction context is reproducible with:

```powershell
python tools/audit/dump_coff_disasm.py `
  build/split/source/input/input_xbox.obj _input_initialize
```

## Why `thread_id` and keyboard-queue guesses are excluded

The genuine XDK declaration in `xbox/include/WinBase.h` defines
`CreateThread`'s sixth argument as `OUT LPDWORD lpThreadId`. January pushes
zero for that argument at function offset `+0x63`, pushes the other five
arguments, calls `CreateThread`, and stores its returned `HANDLE` at
`_input_globals+0x224`. Therefore `+0x230` is not a thread-ID output buffer:

```text
0063  push 0                 ; lpThreadId == NULL
0065  push 4                 ; CREATE_SUSPENDED
0067  push 0                 ; lpParameter == NULL
0069  push _code_000bf540    ; thread procedure
006e  push 0x4000            ; stack size
0073  push 0                 ; security attributes
007a  call _CreateThread@24
0082  mov dword ptr [_input_globals+0x224], eax
```

The genuine `XINPUT_DEBUG_KEYQUEUE_PARAMETERS` in `xbox/include/XKbd.h` is a
separate 16-byte structure of four `DWORD`s. January materializes it on the
stack at `[ebp-0x10]` with values `7`, `64`, `500`, and `100`, then passes its
address to `_XInputDebugInitKeyboardQueue@4`. Consequently `+0x230` is not the
keyboard-queue parameter object either. Reproduce the declarations with:

```powershell
rg -n "XINPUT_DEBUG_KEYQUEUE_PARAMETERS|XInputDebugInitKeyboardQueue" `
  xbox/include/XKbd.h
rg -n "CreateThread|lpThreadId" xbox/include/WinBase.h
```

## PDB and donor evidence

The January PDB does not retain this translation unit's private types or data
symbols. The exact module inspection is:

```powershell
python ..\..\research\pdb200\inspect_module.py `
  tools/pdb200_extract.py `
  'C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb' `
  input_xbox
```

Module 286 is `\halo\objects\halobetacache\input_xbox.obj`, but its 120-byte
symbol stream contains only OBJNAME record `0x0009` and COMPILE record
`0x1013`. There is no January LDATA/GDATA/type record from which to recover
the member.

The Stian and Pastudan retail reconstructions independently preserve the same
layout address, but both leave the statement as the raw
`*(uint32_t *)0x46bb30 = 0;`. They are therefore corroboration for the write,
not evidence for a source name or meaning:

```powershell
rg -n "46bb30" `
  ..\..\research-cache\stian-halo-full-history-20260828\src\halo\input\input_xbox.c `
  ..\..\research-cache\pastudan-halo-full-history-20260828\src\halo\input\input_xbox.c
```

The later HCEX PDB is also not a layout donor. Its
`input_globals_xbox` begins with `gamepads[4]` at `+0x0`, then key arrays at
`+0xA0`, `+0x10D`, and `+0x17E`; it has no event, thread, device-handle, or
homologous `+0x230` lifecycle region. The described layout ends at `0x27E`,
not January's `0x40C`. Reproduce that boundary with:

```powershell
& '..\..\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe' `
  -type input_globals_xbox `
  '..\..\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX_Release.pdb'
```

The HCEX PDB remains useful corroboration that the global itself was
file-static, as documented in
`input_xbox_obj_globals_ownership_audit_20260904.md`; it does not authenticate
this older structure's member topology.

## Private lifecycle topology and narrow source scope

January's `_code_000bf540` is a wait thread: it waits indefinitely on the
event handle at `+0x228`, calls `_code_000bf0a0`, and repeats. The 176-byte
`_code_000bf0a0` updates four `XINPUT_FEEDBACK` motor states and suppresses
rumble while input is suppressed, the console is active, the game is paused,
or gameplay is inactive. Those behaviors support the inferred private names
`input_update_thread` and `input_update_gamepad_rumbler_states`; they do not
solve `+0x230`.

The three owners cannot be admitted separately. With the initializer absent,
ordinary VC7 dead-function elimination removes both static private functions.
Making them external, adding an anchor, or inventing another caller solely to
retain them would be fake ownership. If new evidence authenticates the field,
the least-scope production change is confined to
`source/input/input_xbox.c`: replace only the relevant portion of the existing
`reserved3[6]` interval with a real member and add the three natural bodies.
`input_initialize(void)` is already declared in its owning
`source/input/input.h`; the two helper declarations should remain private in
the C file. No shared layout header is justified because the January global is
TU-private and the 833-object census finds no outside consumer.

## Durable reopen conditions

Do not repeat compiler-shape searches or assign a plausible-sounding name from
the lone zero store. Reopen only when at least one of these supplies semantic
evidence:

1. a contemporaneous January/retail PDB, object, map, or source record names a
   member at the homologous offset in the same `0x40C` layout;
2. another binary with the same layout contains a read or nontrivial write
   whose control/data flow identifies the state;
3. an authentic Bungie/XDK call contract uses that exact storage and proves
   both its type and role.

Evidence from a materially different later layout, a decompiler's raw address,
the observed four-byte access width alone, or codegen similarity is
insufficient. Until one of the conditions above is met, keep all three owners
unwritten and preserve their valid target byte inventory without claiming a
fuzzy semantic completion.
