> Research evidence only; no source change or exact credit from this packet. Its measurements precede the separate vertex admission.

# January XBE variants: instrumentation, not an alternate compiler witness

The user confirmed that the supplied directory contains all local January artifacts, that other copies are byte-identical, and that no additional original headers/source archives or full-symbol build exist locally. The known incomplete PDB remains the only January PDB. This closes the optional artifact question; it does not establish that further reconstruction is impossible.

Read-only script: `scratch/astra_wave6_january_variants.py`. The game images were never executed or modified. `headers.json` preserves file hashes, section layouts and embedded debug paths; `comparison.json` preserves per-function measurements and disassembled instrumentation stubs. Header fields were checked against the [Cxbx XBE implementation](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/master/src/common/xbe/Xbe.h).

## Direct findings

- The ordinary XBE's 1,924,796-byte `.text` matches the original PE exactly after applying its 47,408 HIGHLOW relocations with the XBE load-base adjustment. There are zero unexplained byte differences in that section.
- Of 283 current unaccepted function-section-owner rows, 282 map unambiguously through the existing symbol manifest. `_update_potentially_audible_looping_sound` has two same-name manifest records and is explicitly omitted from this comparison; no address is guessed.
- Every one of those 282 function extents is byte-identical in `cachebeta_instrumented.xbe` and the ordinary XBE. The instrumented image has only four changed bytes elsewhere in `.text`; this packet does not claim their purpose from their count alone.
- In `cachebeta_CG.xbe`, 254 of the 282 extents are byte-identical. The other 28 differ only at their entries. Each detour's added `.bststub` code reproduces the original displaced instruction bytes exactly and jumps back to the unchanged remainder of the function. All 28 pass that check, including frame statistics, path avoidance and the ARGB converter.
- Original `.data` section bytes and sizes are identical across all three XBEs. The variants supply no alternative fog or obstacle-snapshot storage layout.
- Added `.bistro` strings explicitly name Intel VTune/Bistro instrumentation commands, an external `D:\Halo\cachebeta.pdb`, recorder DLLs, and kernel imports. They do not provide a recovered Halo local-variable/type/line inventory. This is a statement about recovered evidence, not a claim that every undocumented binary field has been decoded.

The CG command includes `-func_select` and a temporary `.fsl` path. That string is a reference to an instrumentation input, not an available source archive or a second local file. No search for another byte-identical game copy or repeated query of the incomplete January PDB is warranted by it.

## Consequence

These variants corroborate the January machine code and selected function boundaries. They do not independently expose the original lexical scopes, aggregate-copy source spelling, private global identities, or compiler options needed to resolve the studied residuals. No compiler probe, source edit, symbol change or coverage gain follows from this packet.
