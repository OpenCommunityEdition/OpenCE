# H1 tags cross-build intake (2026-09-06)

## Status and authority

The user supplied `C:/Users/isabe/Downloads/haloleak2024.rar`, specifically
`halo1/h1_tags`. Five explicitly selected executables were extracted for local
analysis. No executable was run and no tag assets were extracted. These inputs
are **CROSSBUILD_ONLY** evidence, not the January 2002 Xbox target.

The user subsequently supplied the extracted folder
`C:/Users/isabe/Downloads/haloleak2024/halo1/h1_tags`. All five symbol-executable
SHA-256 hashes match the archive copies below exactly, so the saved analysis is
reusable. The extracted `halo1` subtree also contains no PDB/DBG files at this
checkpoint. The source folder remains read-only.

All five PE timestamps report 2020-11-23; their CodeView paths identify a
`build/v140/Win32` build. A timestamp is a claim in the file, not independent
build provenance. Each image has zero COFF symbols and zero exports. The
CodeView records are **RSDS references to external PDBs**, not embedded PDB
contents. The full archive listing contained 211,474 entries, no `.pdb` or
`.dbg` files, and three `.map` files that are Halo 2 game assets rather than
linker maps. Do not describe the `_symbols.exe` suffix as proof of a complete
debug-symbol payload.

## Exact input identities

| Executable | Bytes | SHA-256 |
| --- | ---: | --- |
| `guerilla_symbols.exe` | 9,764,864 | `44538c3effe5a740f6995e7f6867995e81ec7d32ee8bb0bb74f04c2b9c203b83` |
| `halo_cache_symbols.exe` | 6,672,896 | `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55` |
| `halo_tag_symbols.exe` | 7,450,624 | `c5f58fe29cdde668d0e30f6848ad50b8f49a28e99c30de2b1bb75cf7bbfb0712` |
| `sapien_symbols.exe` | 11,303,424 | `b1f0b207f5f49cb0ce90f5be7957197d3be612593ff52e7a50fc17e2b88a4828` |
| `tool_symbols.exe` | 10,631,680 | `079d20d7792e9565b24ab2cff8115f76a6e48749a8e8ffd190f57954495f0a97` |

The missing PDBs must match **both GUID and age**, not just basename:

| PDB basename | GUID | Age (decimal) |
| --- | --- | ---: |
| `guerilla_symbols.pdb` | `9b751e93-d090-4fbf-9a7c-d8e3cbcb1866` | 125 |
| `halo_cache_symbols.pdb` | `7f03f998-b7b0-4ea6-b18f-a6506e3442e5` | 14 |
| `halo_tag_symbols.pdb` | `eac025b6-d3f8-4ae9-a47e-9882d7c58906` | 107 |
| `sapien_symbols.pdb` | `82b677b3-b983-4c49-912e-ae5641ce5f76` | 87 |
| `tool_symbols.pdb` | `3e2e627d-3e3b-4396-afd6-8ded91e9d954` | 80 |

## Local evidence and tools

Research root:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/symbol-build-h1-tags-20260906`

- `manifest.json`: PE sections, debug-directory records, exact PDB paths,
  identities, and explicit limits.
- `inspect_builds.py`: regenerates the manifest using `pefile`, without
  executing or modifying the five input images.
- `ghidra-scripts/CrossBuildInventory.java`: exports observed function names,
  their Ghidra source classification, and body ranges. It performs no renames.
- `ghidra-projects/h1-tags-crossbuild.gpr`: new, separate cross-build project.
- `ghidra-import.log`: successful automatic analysis and save of
  `halo_cache_symbols.exe` (187 seconds, no timeout).
- `ghidra-import-remaining-retry.log`: the other four executables were imported
  and saved with `-noanalysis`. The already-imported cache executable was
  rejected as a duplicate and was **not overwritten**.
- `ghidra-export-explicit-bundle.log` and
  `ghidra-export-cache-verified.log`: successful read-only inventory exports.
  Earlier export attempts failed independently of the saved imports. The
  explicit `ghidra-scripts/META-INF/MANIFEST.MF` and a subsequent run resolved
  script-bundle loading. The installed headless README names an unavailable
  timeout method; the installed Java source calls it `analysisTimeoutOccurred`.
  The inventory script now uses plain `GhidraScript`; analysis completion is
  established by the analysis log, not by a no-analysis export run.

Verified project status at handoff:

| Program | Analysis status | Observed function records |
| --- | --- | ---: |
| `halo_cache_symbols.exe` | Automatic analysis completed | 17,858 |
| `halo_tag_symbols.exe` | Imported only; analysis pending | 18 |
| `guerilla_symbols.exe` | Imported only; analysis pending | 5 |
| `sapien_symbols.exe` | Imported only; analysis pending | 11 |
| `tool_symbols.exe` | Imported only; analysis pending | 32 |

Function inventories are `<executable>.functions.tsv`. These counts include
analysis-created functions/thunks and imports, **not recovered original Halo
function counts**. Preserve each row's source classification and do not count
names lacking a `FUN_` prefix as original symbols.
For the analyzed cache executable, 17,525 records have Ghidra source `DEFAULT`,
324 have `ANALYSIS`, and nine have `IMPORTED`; none establishes missing PDB
contents. The `.gpr` file is a zero-byte project marker; its sibling `.rep`
directory holds the databases and must stay with it.

The existing Ghidra installation was found by following its saved application
log, not by assuming it was on PATH:
`C:/Users/isabe/Documents/Codex/halo-tools/apps/ghidra_12.1.2_PUBLIC`.
Its configured JDK is the sibling `jdk-21.0.11+10` directory. The live RPC
endpoint at `127.0.0.1:18081` was offline at intake; the headless launcher is
usable independently. Only Ghidra's local settings/cache directories needed
additional write permission; the new project and outputs stay in the workspace.

IDA has not been located in the available tools, PATH, ordinary installation
directories, registry installation entries, or the local Halo tool bundle.
**The user's requirement to use both IDA and Ghidra remains outstanding.** Do
not claim that Ghidra, Capstone, or a PE parser satisfies the IDA half. A usable
installation/license or further location evidence is needed. Do not upload
these inputs to a cloud decompiler or symbol service without authorization.

Keep the executables, databases, and generated bulk inventories local. This
ledger records pointers and hashes only; it is not authorization to publish
the supplied binary corpus.

## Reconstruction plan and limits

1. Identify the source build and validate any future PDB against the recorded
   GUID/age before loading its names/types into its own donor database.
2. Preserve evidence categories: imported API names, disassembler inference,
   diagnostic literals, genuine debug records, and independently January-proven
   facts are different things. A generated `FUN_...` name is not an original
   name and must not become a production `code_<address>` substitute.
3. Use diagnostic strings, callers, data references, and instruction/CFG
   comparisons to propose January correspondences. Verify each mapping locally;
   never add one global address delta across builds.
4. "Un-inline" means recover and annotate candidate helper operations and their
   call-site relationships where evidence permits. It cannot recreate all lost
   source, local variables, or original inline boundaries. Do not rewrite the
   January target, force new calls, or emit a `point_from_line3d` COMDAT to make
   the donor's decompilation look tidier.
5. Names and types can accelerate source reconstruction, but exact credit still
   requires the January COFF byte/relocation, source-authenticity, whole-TU,
   whole-tree regression, and object-admission gates. No canonical code bytes
   are earned by this intake or by automatic analysis alone.

## Related networking evidence

[Exploring The Halo 1 System Link Protocol](https://hllmn.net/blog/2023-09-18_h1x-net/)
provides retail PAL packet/debugger evidence. Its decoded advertisement retains
the suffix `age in a bot`. This corroborates, but does not independently prove
for January, our source-attested join-token implementation: the first four
bytes are transformed while the remaining twelve survive. The article's
two-second search cadence is another comparison lead. Its PAL timing and
absolute addresses must not replace January constants or addresses.

Use the article's emulator-boundary inspection method as a future local
runtime-validation aid. No live interception or traffic manipulation was
performed for this intake. The canonical token implementation and runtime tests
are documented in
`docs/object_matching_logs/network_server_manager_obj_source_attested_packet_20260906.md`.
