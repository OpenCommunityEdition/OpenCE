> Research only: no admission or retained source/header/configuration change.

# Environment filter topology: bounded follow-up

**No independent witness found. Keep the environment candidate HOLD, zero credit.** Four previously unexamined, directly identifiable debug lightmap counterparts were read from the supplied corpus. All implement the normal DX9 lightmap path; none contains January's vector-test texture-selection path. No source candidate, compiler gate, header edit, Ninja operation, or production change was made.

## Prior evidence read before the lookup

- `docs/crossbuild_h1_tags_intake_20260906.md` and the existing `research/hs7/ANALYSIS_GUIDE.md`: eight PC-family images, saved Ghidra databases/inventories, exact identities, partial-analysis limits, no matching PDBs.
- `scratch/astra-wave2/crossbuild-artifacts.json`: supplied executable identities, including the older Speartest/e32k programs.
- `scratch/astra-wave6/january-variants/packet.md`: normal January XBE is the rebased PE, instrumented studied bodies are identical, CG detours reproduce displaced instructions and return to the same body. These are not alternate source/compiler witnesses and were not rescanned.
- Unit-specific September 12/14/15 matching history and the completed `../environment-independent/review.md` common-tail ablation.
- Existing cache-build actual Ghidra extraction `../environment/ghidra_007dfc50.asm/.c` and HCEA DIA stubs. These were reused, not re-extracted. The reconstructed HCEA `_rasterizer_environment_lightmap_draw_pp.c` also describes a normal DX9 effect path; it was not promoted into an original-source witness.

## Narrow lookup and actual raw extraction

`locate.py` reads only the four other supplied Symbols executables already listed in the cross-build census. It validates their existing hashes, searches the exact primary-animation-period assertion, and maps its code reference to the saved Ghidra function body range. `locators.json` preserves the original DEFAULT name/signature provenance; it does not claim those generated names are PDB symbols. No project reanalysis, disk-wide scan, binary execution, or artifact request occurred.

`extract.py` then reads those four identified raw body extents from each original PE and disassembles them with Capstone. This is a fresh raw-byte check using existing Ghidra boundaries, not a claim of a fresh Ghidra session. `raw-summary.json` records full hashes, every compare/test and branch, and original boundary metadata. Complete raw assembly is saved per executable.

| Supplied executable | Identified entry/end | Bytes | Result |
| --- | --- | ---: | --- |
| `halo_tag_symbols.exe` | `0x7ef570..0x7eff1e` | 2478 | Normal DX9 lightmap path; no vector-test branch |
| `sapien_symbols.exe` | `0xd4fca0..0xd506b0` | 2576 | Normal DX9 lightmap path; no vector-test branch |
| `tool_symbols.exe` | `0xceb220..0xcebc30` | 2576 | Same normal path/control offsets as Sapien; no vector-test branch |
| `guerilla_symbols.exe` | `0xa8e2d0..0xa8ece0` | 2576 | Same normal path/control offsets as Sapien; no vector-test branch |

The function entries check the device and a draw-enabled byte, then shader/vertex inputs, alpha-test/shader flags, the illumination-map choice, and effect validity. They proceed into normal texture setup and animation; there is no equivalent of January's drawing-mode9, vector-table/test-pattern selector and WRAP-versus-CLAMP/W stage0 setup. The three editor/tool bodies have the same compare/test schedule (20 operations) at the same offsets; Halo Tag has the corresponding 20-operation schedule with smaller debug-frame differences. The raw bodies, not that count alone, establish the absence in these mapped functions.

There is a potentially misleading nearby pair of filter arms. Halo Tag `0x7ef778..0x7ef7d9`, Sapien `0xd4feb5..0xd4ff16` (corresponding offsets in Tool/Guerilla), reads illumination flags and sets sampler **1**, states5/6/7, to **1 in one arm and 2 in the other**. That is ordinary point-versus-linear filtering for the self-illumination map. It is not stage0, is not the vector-test selection, and does not duplicate identical values. It therefore cannot authenticate the held candidate's two identical stage0 LINEAR blocks.

## Exhaustion boundary

The identified supplied debug counterparts now provide no missing branch witness. The known January variants and HCEA stubs are already exhausted for this question. The three optimized Play variants and the much earlier Speartest/e32k programs have no independently identified relevant vector-test entry in the existing records; this follow-up does not start a broad binary search to invent one. This is a bounded negative result for the mapped functions, not proof that every instruction in every supplied executable has been searched.

No further spelling, declaration, or filter-placement variants are justified. Reopen only with a concrete original/debug entry containing the specific vector-test texture-choice arms and their same-value filter writes, or another independently authenticated context mechanism. The useful shared-intensity proof remains valid; the combined exact candidate still receives zero admission credit because the separate duplication concern remains unresolved.
