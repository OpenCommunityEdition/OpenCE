# `shell_xbox.obj` SEH relocation-provenance closeout

## Disposition

`source/shell/shell_xbox.c` is corrected from `Matching` to `NonMatching`.
Seven of its eight functions are independently strict-exact. `_main` has
byte-identical padded code, but the January split object does not preserve
three compiler-special `__except_list` relocations that the XDK 3911 rebuild
emits. The object therefore cannot satisfy the campaign's strict relocation
identity rule and receives no complete-object credit.

No production C source, compiler flag, splitter binary, target object,
comparator, or admission rule is changed. In particular, this closeout does
not weaken equality, patch an object, manufacture a pseudo-symbol, or infer
missing relocation records from the candidate alone.

## Direct hardened evidence

The target and candidate `_main` sections are both 144 padded bytes and have
the same normalized SHA-256:

`95f187afcd93ff231b332230226f531403adae11cb1fc9b5d08897329fb966a3`

Their ordinary call relocations agree at the same addresses and destinations:

- `+11` `__except_handler3`;
- `+39` `fuck_code`;
- `+44` `rasterizer_preinitialize`;
- `+49` `physical_memory_allocate`;
- `+61` `shell_initialize`;
- `+70` `main_loop`;
- `+75` `shell_dispose`;
- `+91` `generic_exception_filter`;
- `+103` `halt_and_catch_fire`.

The strict relocation inventories nevertheless differ. The January split
section has 10 records; the candidate has 13. The candidate's additional
`IMAGE_REL_I386_DIR32` records target compiler pseudo-symbol
`__except_list` at `+17`, `+25`, and `+122`. Its first scope-table relocation
targets compiler-local `$T18245`, while the splitter attributes the
corresponding target address to `_rdata_002a0a88`. Equal normalized bytes do
not make these ownership and relocation identities equal.

## Splitter audit

The repository pins csplit `v0.0.2`, commit
`f688c49957e2c12e8c3871d7c2387abed0a91c5c`, binary SHA-256
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`.
That version emits only relocations represented by linked-image file offsets
in `config/relocs.json`; its schema cannot name an undefined compiler-local
SEH pseudo-symbol. Neither `__except_list` nor `$T18245` exists in
`config/symbols.json`.

A full available target/candidate corpus scan searched same-named code
sections for candidate `__except_list` relocations with equal padded size and
normalized bytes. It found only `source/shell/shell_xbox::_main`, with the
same 10/13 inventory and `+17/+25/+122` gap. There is therefore no independent
second case from which to derive a fail-closed general reconstruction rule.

## Reopen boundary

Reopen complete-object admission only with target-independent provenance for
the original COFF SEH relocations or a splitter/config representation that can
faithfully preserve undefined compiler-special relocation targets. Do not add
candidate-derived relocation exceptions, synthetic linked-image symbols,
comparator normalization, or object patches.

The source itself already expresses the natural XDK structured-exception
topology with `__try`, `__except`, `generic_exception_filter`, and the normal
terminal return. Source scheduling experiments are not indicated by this
residual because all 144 padded code bytes are already identical.
