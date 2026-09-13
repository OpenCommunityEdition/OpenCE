# `action_vehicle.obj` evaluate-seat symbol reconciliation (2026-09-13)

## Scope and ruling

January target file offset `0x000094C0` is authentically
`action_vehicle_evaluate_seat`.  `config/symbols.json` therefore maps the old
address placeholder `_code_000094c0` to
`_action_vehicle_evaluate_seat`.  This is a symbol-identity correction only;
no function body, prototype, linkage, compiler flag, or source schedule changes
in this packet.

The corrected mapping makes the already reconstructed private caller
`action_vehicle_find_impromptu_seat` strict-exact.  The evaluate-seat body
itself remains fuzzy and receives no exact credit.

## Same-build name evidence

The exact January/Halo CE atlas record is:

```text
{"off":"0x4094c0","name":"_action_vehicle_evaluate_seat","lib":"action_vehicle.obj","tier":"exact"}
```

It is record 53 of:

```text
research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl
```

The file's SHA-256 is
`851802ac508bbcf7cd2896c6cfddd1e0f81510b7b9ef9ea02f39f913c123578e`.
Subtracting the image base `0x400000` places the symbol at target file offset
`0x94C0`, exactly the `config/symbols.json` record being corrected.  The
adjacent same-build records independently anchor the object topology:

- `0x4093B0`: `_action_vehicle_desirable`;
- `0x409890`: `_action_vehicle_find_destination`;
- `0x40A1B0`: `_action_vehicle_find_impromptu_seat`.

This direct same-build map, not a behavioral guess, authorizes the production
name and relocation identity.

## Corroboration and limits

Stian's commit `32a94ebf59066e34057e6e6bfe625d64c61b1be5` introduced the
same name after decoding the action-definition table and cross-referencing the
call graph.  That is useful independent semantic analysis, but it is not
original-symbol evidence and was not used alone to admit this mapping.

The HCEA atlas also contains exact PDB records for
`action_vehicle_evaluate_seat`, including `0x83821698` in
`567b63bd5f1c5aed0f99b8bc2bb70c9be655d8f6df81d2cbb20c725b33fd6084.jsonl`.
Those records corroborate the spelling and role across builds; they are not
interchangeable January layout evidence.

The supplied `h1_tags/*_symbols.exe` images contribute no name proof here.
They contain references to missing external PDBs, zero COFF symbols, and no
embedded original function-name records.  Analysis-generated Ghidra names are
not treated as Bungie symbols.

## Focused verification

Before target-map regeneration, the universal identity-only scan reports the
sole mismatch in the 304-byte caller:

```text
source/ai/action_vehicle::_action_vehicle_find_impromptu_seat
    @0x0072  target _code_000094c0 + 0
             ours _action_vehicle_evaluate_seat + 0
```

The ordinary focused gate reports 9 exact, 5 residual, and 3 unwritten
functions because its existing split target still carries the old placeholder.
Running the same gate with the authenticated in-memory alias
`action_vehicle_evaluate_seat=code_000094c0` proves the post-regeneration
identity result without disturbing the shared build directory:

```text
EXACT       304  _action_vehicle_find_impromptu_seat
residual    976  _code_000094c0  [size 960!=976, relocs 31!=30, sha]
== exact 10  residual 5  unwritten 2  (of 17 listed)
```

Thus the caller is strict-exact at 292 meaningful / 304 padded bytes with
three matching relocations after the map correction.  The current semantic
ledger had already admitted those 292 meaningful bytes through its ordinary
normalized-code proof despite the unresolved callee spelling.  This rename
upgrades that existing result to strict relocation identity; it must **not**
be booked again as a new 292-byte campaign gain.

The reconstructed evaluate-seat body is 960 padded bytes against January's
976 and has 31 relocations against 30.  It remains a best-source fuzzy body,
earns zero exact bytes here, and must not be described as closed merely because
its original name is now known.
