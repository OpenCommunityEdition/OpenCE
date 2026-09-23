# January COMMON pool: ordering hypothesis and ownership limits

Lane C research, 2026-09-22; corrected during canonical reconciliation.

`linker_common.obj` is a synthetic split of tentative COMMON definitions.
Several independently attributed records appear in contiguous clusters whose
positions correlate with module order. This is useful as a **negative
constraint**, but it does not prove the owner of an individual symbol.

Every pooled record in `config/contribs.json` resolves to synthetic module 847.
The HCEX PDB lists these globals without a compiland. Declaration order and
first-reference order do not reliably reproduce the pool order, either.
Consequently, do not create a tentative definition merely because the symbol
falls between two apparently owned records.

The original Lane C packet applied that inference too strongly. Canonical
reconciliation excluded its 116-definition COMMON wave. An existing owner test
proved that `rasterizer_frame_statistics.c` **imports** its eponymous global,
despite the pool-order hypothesis assigning it to that TU. The same lack of
independent proof affects `hs_syntax_data`, `ai_globals`, and
`rasterizer_model_cortana_hack`; their Lane C moves were also not integrated.
These are unresolved ownership questions, not credited reconstructions.

The useful, narrower rule remains: a genuine tentative definition must be
uninitialised in source (`struct foo bar;`), while `= 0` or `= NULL` gives the
symbol different COFF ownership. Compare both the January object and the
rebuilt object's symbol class before accepting a declaration. The read-only
`tools/campaign/linker_common_base.py` can inspect the known pool records, but
its partial base earns **zero** matching credit under the all-or-nothing data
metric. Its output must not be used as an ownership oracle.
