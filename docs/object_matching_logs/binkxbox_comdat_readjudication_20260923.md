# `binkxbox.obj`: COMDAT-selection re-adjudication evidence (2026-09-23)

The prior admission hold has now been lifted; the source and compiler flags
remain unchanged. The old `config/object_admission_rejections.json` entry
identified **only** the
17 string-literal `.rdata` COMDAT selections: January's *csplit reconstruction*
marks them NODUPLICATES (1); VC7 compiling the current source marks them
SELECT_ANY (2). Claude's 2026-09-23 provider census established that csplit
assigns NODUPLICATES to **every** reconstructed January section. Selection 1
in that object therefore does not prove selection 1 in Bungie's original
pre-link `.obj`. The prior hold treated synthetic selection as original
compiler evidence and should be re-adjudicated, not silently waived.

Fresh comparisons against `build/split/libs/binkxbox/binkxbox.obj` and
`build/base/libs/binkxbox/binkxbox.obj`:

| Check | Result |
| --- | --- |
| Five named functions | All five strict `section_infos_equal`: bytes and ordered relocations. Both sides' code section flags and selection are identical (`0x60501020`, NODUPLICATES). |
| Seventeen literal sections | All 17 have the same mangled owner name, size, normalized hash, and ordered relocations. Only synthetic-vs-compiled selection differs. |
| `BINKDATA` | 72/72 bytes, 16/16 relocations, equal normalized SHA-256 `834a709ba2534ebe3ee1397fd4f7bd288b2acc1d20a08d6c862dcd99b6f04400`. |
| COFF owner census | Exactly 23 defined external names and four undefined external names on each side; neither side has a missing or surplus name. No other runtime section differs. |
| Current-tree duplicate census | All 17 literal names have exactly one definer among the 610 currently built base objects. |
| Actual VC7 pair links | `binkxbox.obj` with `binkread.obj`, both input orders: zero `LNK2005`/`LNK1169`. Both links stop at expected unresolved imports (`LNK2001`/`LNK2019`/`LNK1120`), so this is **not** a complete final-link proof. |

The negative `/GF-` control removes the 17 literal COMDATs but changes the
otherwise exact `BinkLoadUnloadConverter` section from 352 to 400 bytes. It
also combines literal storage into other sections. Suppressing pooling is
not an authentic fix. Keep the compiler's natural SELECT_ANY output.

**Narrow admission ruling:** the splitter's blanket NODUPLICATES selection is
not evidence of the original pre-link selection. The source emits natural
SELECT_ANY literals, and their content, relocations, owner names, and bounded
duplicate-definition check pass. `binkxbox.obj` is admitted `Matching` on
that evidence; this does not license candidate-only helper owners or a claim
of complete final-link equivalence. No extra code or data bytes are credited
solely from the status change; its five functions were already exact.
