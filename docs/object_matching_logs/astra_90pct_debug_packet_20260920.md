# Debug/compiler-context evidence packet — 2026-09-20

This packet changes no production source, metadata, comparator, ownership rule, or admission. It supplies fresh CodeView/DIA/Ghidra evidence. It does not itself claim exact bytes.

## January authority: compiler records, but no local/line records

Fresh raw MSF2 parsing of `C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb` found 848 DBI modules, including 467 `halobetacache` modules. Every one of those 467 streams contains exactly one `S_OBJNAME` (`0x0009`) and one `S_COMPILE2_ST` (`0x1013`), with zero old/C13 line bytes and zero source-file entries. No procedure, local, lexical-block or line record is present in these Halo compiland streams.

PDB SHA256: `8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`.

All 19 priority objects were found. Each reports C, CPU `0x6` (Pentium Pro), FE/BE `13.0.9254`, flags `0x200` (`NoDbgInfo`), and `Microsoft (R) Optimizing Compiler`. AI streams: ai_debug 444, ai_communication 445, actor_perception 462, actor_moving 463, actor_looking 464, action_obey 474.

Evidence: `scratch/astra-debug-evidence/january_priority_modules.json`; reproducible reader `inspect_priority_debug.py`. It imports the existing read-only `pdb200_extract.py` with bytecode writes disabled. These records do not encode a full command line: they do not authenticate `/Ob`, `/QIfist`, exact optimizer switches, header order or PCH usage. Matching compiler versions do not prove every unrecorded switch identical.

## Supplied symbol executable: source-revision claim remains unauthenticated

Fresh PE parsing independently agrees with the earlier asset manifest:

- `halo_cache_symbols.exe` SHA256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
- x86 PE image base `0x400000`; PE timestamp 1606092379, corresponding to 2020-11-23.
- Zero COFF symbols. CodeView data is an external RSDS PDB reference, not embedded source/local records.
- RSDS path `D:\P4\Halo1\build\v140\Win32\Symbols Cache\halo\out\halo_cache_symbols.pdb`, GUID `7f03f998-b7b0-4ea6-b18f-a6506e3442e5`, age 14.

The date and v140 path establish a different build context. They alone do **not** disprove a shared source revision. Neither same source revision nor identical January local layout is authenticated. Its first-party assertion strings and RTC descriptors remain useful cross-build evidence. January bytes/relocations remain final authority.

Evidence: `symbol_executable_provenance.json`.

## Fresh Ghidra: 2,517 frames and all six AI targets

The installed Ghidra 12.1.2 was actually run against the existing `h1-tags-crossbuild` project. The program was acquired through `getReadOnlyDomainObject`; the script opens no transaction and never saves the program. It independently reports the PE SHA256 above and language `x86:LE:32:default`.

The script follows Ghidra call references to `_RTC_CheckStackVars`, extracts the descriptor passed in EDX, and validates descriptor counts, signed offsets, sizes, and terminated ASCII names. It recovered **2,517 call-linked frame descriptors / 6,350 local records**, plus 163 assertion-source-path xrefs for the six AI files. All 2,517 descriptors agree exactly on `(frame offset, size, name)` with the independent raw-byte extractor `astra-context-rtc-all.json` when keyed by function and descriptor. The raw extractor has 2,528 records; its additional records are not automatically admitted as distinct functions by this packet.

| Proposed cross-build correspondence | PE entry | RTC records | Source-path xrefs |
|---|---:|---:|---:|
| ai_debug_render_actor | 0x0049c470 | 94 | 4 |
| ai_communication_event | 0x0048d470 | 34 | 13 |
| actor_perception_update | 0x00475f00 | 12 | 11 |
| actor_look_update | 0x0045f5d0 | 9 | 13 |
| action_obey_command_begin | 0x00438120 | 12 | 2 |
| actor_move_vector_avoidance | 0x00469ce0 | 20 | 3 |

These are strongly supported **correspondences**, not restored original symbol-table names. They combine the correct first-party source assertion path, distinctive aggregate/local-name combinations, and independent HCEA function evidence where available. All six were freshly decompiled; complete Ghidra disassembly is also preserved. Decompiler output is inference, not original C.

Evidence: `priority_debug_atlas.json`, `ghidra_rtc_locals.tsv`, `ghidra_priority_source_xrefs.tsv`, `ghidra_<entry>.asm`, `ghidra_<entry>.c`, and `ghidra_direct.log`, all under `scratch/astra-debug-evidence/`.

Useful examples:

- ai_debug exposes 94 protected local records, including repeated scoped vectors/points, `textstring[512]`, `textbuffer[1024]`, repeated `tempbuf` arrays, iterator instances, and movement-debug arrays. These enable local identity research, but RTC does not record complete scalar declarations or lexical scope boundaries.
- actor_perception exposes two separate `sense_position` records, `iterator`, `became_acknowledged_from_orphan`, `information` and debug buffers.
- actor_moving exposes `avoidance_weights` (32 bytes), `avoid_ray_result` (4), `avoid_ray_t` (8), two `ray_origin`/`ray_direction` pairs, approximation variables, and final rotation vectors. HCEA independently names corresponding arrays and types.

## HCEA typed locals, lines, and the animation-width finding

Fresh DIA2Dump runs read `HCEX_Release.pdb`, SHA256 `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`. Five AI targets have typed parameter/local and line records; `ai_debug_render_actor` was absent by that name in both supplied HCEX PDBs. Results are preserved, including empty unsuccessful lookups.

| Function | Source line records |
|---|---:|
| ai_communication_event | 295 |
| actor_perception_update | 234 |
| actor_look_update | 189 |
| action_obey_command_begin | 278 |
| actor_move_vector_avoidance | 127 |

Paths are under `d:\projects\code\hcex\sources\ai\`. Each line dump includes the original source-file MD5. This is 2011 Xbox 360 evidence, not January x86 evidence. DIA2Dump renders several PPC register identifiers using x86-looking names (`bl`, `cl Relative`, etc.); do not interpret those spellings as x86 allocation.

The decisive new type corroboration requested for the communication candidate is explicit in three named private helpers:

- `ai_communication_actor_talk_weight`, RVA `0x017CDF78`: named parameter `animation_type`, **Type: short**.
- `ai_communication_find_specific_actor_to_talk`, RVA `0x017CE478`: named parameter `animation_type`, **Type: short**.
- `ai_communication_find_global_actor_to_talk`, RVA `0x017CE578`: named parameter `animation_type`, **Type: short**.

The emitted static function signatures independently repeat the short widths. Raw `hcea_<full-function-name>_sym.txt` and `_lines.txt` files preserve the evidence. This supports a natural correction to the private forwarding chain when combined with the existing short parameter at its true `units.h` owner and a January strict gate. It does not independently admit any changed function.

## Limits of lexical-scope recovery

A fresh raw MSF7 parse also extracted the five AI HCEA compilands into `hcea_scope_records.json`. None contains `S_BLOCK32` (`0x1103`) records. `action_obey_command_begin` occupies module stream 2573, procedure record offset `0xe9c` through END at `0x10d4`; its local records are flattened `S_REGREL32` records. Three differently typed `iterator` locals share frame offset `0x50`; head_position has offset `0x78`. This supports lifetime reuse, but does not directly authenticate a particular pair of bare C braces. Do not claim an original lexical-block record was recovered.

## Tool availability and reproducible commands

- DIA: `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe` works. Run `-sym <name> <pdb>` and `-lines <name> <pdb>`; `dump_hcea.py` preserves commands and summaries.
- Ghidra: `C:\Users\isabe\Documents\Codex\halo-tools\apps\ghidra_12.1.2_PUBLIC`.
- JDK: sibling `jdk-21.0.11+10`.
- Reproduce the successful Ghidra extraction with `python -B scratch/astra-debug-evidence/compile_ghidra.py` followed by `python -B scratch/astra-debug-evidence/run_ghidra_direct.py`. Output is written beside the scripts. Ghidra application settings/cache stay there. The compiler may print a Windows `ZipFileSystem.close/toRealPath` access exception during cleanup despite exit code zero and valid emitted classes; preserve the log and verify the actual subsequent extractor output.

The ordinary headless `-postScript` path reached the read-only program but failed to resolve its fresh OSGi script bundle. PyGhidra then failed with a JPype jar-filesystem lookup error. The successful fallback compiles `AstraLaunch.java` and `AstraRtcEvidence.java`, supplies the Ghidra jars on the startup classpath, and invokes the real Ghidra Java API directly. Do not repeat the failed launch variants without new evidence of a fix.

IDA Free 9.4 is installed at `C:\Program Files\IDA Free 9.4\ida.exe`; no idat or IDAPython installation was found. An existing user IDA process (PID1124) was left untouched. It has no observed listening TCP socket, and no MCP/RPC plugin was found in the installed/user plugin locations. Reading its WMI command line was denied. A separate hidden autonomous `-A -c -L -o -S` IDC probe against an isolated output database produced no log, database or TSV and remained idle beyond 60 seconds. Only that new probe process was stopped. **No successful new IDA analysis was acquired.** The user's both-tools requirement therefore remains incomplete and must not be represented as satisfied.

User clarification after this probe: the user independently verified that only
IDA Free 9.4 exists and no callable IDA endpoint is available. The user explicitly
authorized continuing with Ghidra and DIA, with IDA GUI corroboration optional.
Current status: **IDA_AUTOMATION_UNAVAILABLE**. No current admission depends on
an IDA-only finding. The historical probe account above records what was
observed then; its last statement about a mandatory both-tools requirement is
superseded by this clarification.

## Do-not-repeat inventory

1. Do not mine January Halo compiland local/line records as though they existed: fresh enumeration shows they are absent.
2. Do not treat an external RSDS path or `*_symbols.exe` filename as embedded PDB content.
3. Do not equate RTC descriptor order with full original declaration order or lexical-block boundaries.
4. Do not infer January x86 frame offsets from HCEA PPC register spellings or reused frame slots.
5. Do not infer unrecorded `/Ob`, `/QIfist`, PCH, or header choices from `NoDbgInfo`.
6. Do not label Ghidra decompiled C as first-party source or inferred cross-build names as PDB symbols.
7. Do not turn compiler/build-date mismatch alone into proof of a changed source revision.
8. Do not count any source/compiler hypothesis as a law until already-exact controls and full admission checks establish no regression.

No production edit or progress admission was performed in this packet. All proposed landings require orchestrator review and whole-tree validation.
