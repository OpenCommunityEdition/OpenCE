# Five small Halo object admissions (2026-09-23)

## Result and scope

Five Halo-owned translation units outside the concurrent Lane D allowlist are
promoted from `NonMatching` to `Matching`. Their 93 January function bodies,
12,876 meaningful code bytes, and 4,116 target-owned data bytes were **already
strict-exact/credited** before this admission. This batch adds **zero** exact
code or data bytes; it changes the audited whole-object count from 408/833 to
413/833. Candidate-only pooled literals receive no January-byte credit.

| Unit | Strict functions | Meaningful code | Target data | Candidate-only runtime code |
| --- | ---: | ---: | ---: | ---: |
| `source/ai/action_converse` | 9/9 | 856 | 36/36 | 0 |
| `source/scenario/wind` | 9/9 | 1,917 | 100/100 | 0 |
| `source/memory/stack_memory_pool` | 33/33 | 4,678 | 1,000/1,000 | 0 |
| `source/input/input_xbox` | 27/27 | 3,557 | 2,748/2,748 | 0 |
| `source/game/game_allegiance` | 15/15 | 1,868 | 232/232 | 0 |

The fresh PID-isolated `gate.py --all` comparison passes for every unit.
`input_xbox` has an ordinary objdiff false negative for the 192-byte
`_input_key_is_down`; the hardened COFF comparator and existing accepted
semantic exception both classify it strict exact. None of these five has an
active park or object-admission rejection. The full `ninja` build, stable
function sweep, park validator, and admission audit were rerun after the
status changes.

## Target-owned COFF and pooled-literal boundary

`action_converse`, `wind`, and `stack_memory_pool` have exact target-owned
section identities, including normalized payload, size, flags, COMDAT
selection, symbol storage and offsets, and ordered relocation identities:
respectively 9/9 code plus 1/1 data; 9/9 plus 6/6; and 33/33 plus 24/24.
No target-owned COMMON or candidate-only code is present. Each surplus
candidate section is a naturally emitted SELECT_ANY pooled literal that the
January split imports and another January object selects with identical bytes
and relocations. Twenty ordinary VC7/XDK base/base pair links, in both input
orders against the selected-provider equivalents, report no `LNK2005` or
`LNK1169`; expected unrelated `LNK1120` unresolved externals remain. No
forced link, patch, stub, or attribution of these literal bytes is used.

`input_xbox` likewise has 27/27 exact target code sections, 23/23 exact
target-owned noncode sections, and no candidate-only code or COMMON. Its only
candidate-only runtime section is the seven-byte SELECT_ANY `"buffer"`
literal. January imports it from this object and selects the identical copy
in `cseries.obj`; ordinary base/base links in both orders pass the duplicate
phase and stop only on unrelated unresolved imports. The 0x40c-byte private
`input_globals` BSS owner has January's symbol class and offset.

`game_allegiance` has 15/15 exact code sections and no extra code. Its 40-byte
team-name table has ten resolved relocations to the same image addresses as
January. Its private four-byte pointer BSS owner has the same logical zero
contents, storage, size and offset; csplit's materialized BSS and VC7's
fileless BSS differ only physically. Ten candidate-only SELECT_ANY team-name
strings (77 bytes) are identical to January's selected providers. Twelve
ordinary provider-pair link probes (both orders) have no duplicate-definition
failure, though they stop on unrelated unresolved imports. HCEX PDB separately
corroborates the static pointer versus global team-name table. These
representation and literal-selection facts are disclosed; none earns new
data credit.

## Source-quality and verification boundary

The only source edit in this batch changes the private `stack_memory_pool_block`
metadata fields from misleading `void **handle` / `long lock_count` to
`char const *file` / `unsigned long line`, removing an unrelated pointer cast.
The allocation API already supplies those exact types; all 33 target functions
remain strict exact and the layout is unchanged. Other four source files are
unchanged. Existing provisional names in `input_xbox` and reserved scenario
layout spans in `action_converse` remain explicitly inferred, not claimed as
original spelling or structure members. Fake-match scanning finds no new
lead; no manual helper expansion, assembly, pragma, or new inline COMDAT was
introduced. The user's held `path_obstacles` parenthesis lever is untouched.

After regeneration, `python -m tools.campaign.stable_verdicts diff` reports
**zero gained, zero regressed** over all 8,245 function rows; the strict
snapshot remains 7,493 exact. The admission audit reports zero contradicted
Matching labels, and park validation reports zero stale/invalid entries.
`python -m pytest -q tools` passes 1,152 tests, 5 skips and 26 subtests;
plain `pytest -q` is not the repository's supported invocation because it
also collects inaccessible scratch paths and omits the project import root.
`git diff --check` passes. No Lane D source file is touched.
