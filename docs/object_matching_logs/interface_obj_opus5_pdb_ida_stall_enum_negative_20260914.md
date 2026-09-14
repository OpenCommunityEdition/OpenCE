# `interface.obj` — PDB/IDA stall-enum search: negative result (2026-09-14)

## Scope

This was a bounded search (under 20 minutes) for the authoritative 27-value
profiler stall enum. That enum is the only blocker recorded in
`interface_obj_profiler_symbol_and_enum_audit_20260913.md` for
`_render_debug_profile_stall_tick` (271 meaningful) and `_render_debug_profile`
(2,266 meaningful). No source, header or configuration was changed. Exact
credit: 0.

## Evidence examined

1. **January PDB** (`cachebeta.pdb`, matching `cachebeta.exe` SHA-256 `4cc87b45…`),
   through the pdb200 extraction regenerated in this lane
   (`scratch/pdb200-current/`, 18,674 symbols, 1,836 types):
   - `pdb_types.json` has 1,545 structs, 170 enums and 121 unions, **all from
     XDK/NT/D3D modules**. **None of the 170 enums has enumerators**, and no type
     name relates to Halo profiling or stalls.
   - Halo compilands carry no type or local records, consistent with
     `january-pdb`'s "compiled without debugging info".
   - Stall-related **public labels only**: `_profile_frame_get_stalls`,
     `_profile_rasterizer_stalls`, `_profile_graph_toggle`, `_profile_graph`,
     `_profile_graph_values`, `_profile_graph_value_count`, and string labels
     `"stall%6.2f "`, `"stall"` and `"\t\tstall_start_time %d"`. None names a stall
     category.
2. **IDA Free 9.4 with the symbolized January database** (a private copy under
   `scratch/ida/`): its names and type library derive from the same PDB. IDA Free
   rejects `-S` ("Unknown switch '-S'") and ships no IDAPython or `idat`, so Local
   Types and Names could not be exported headlessly. The PDB-derived type library
   is the one characterized above, so it cannot contain Halo enum members either.
3. **Ghidra 12.1.2** decompilation of the January `interface.obj` split
   (`scratch/ghidra/out/interface.obj.decomp.c`) shows the numeric switch but no
   enumerator names, as expected from a COFF object without debug types.

## Conclusion

The new PDB/IDA evidence does **not** recover the stall enum or its enumerator
names. Per the house rules (enum constants in switches; no raw numeric cases, no
invented enum names), both functions remain blocked with zero credit, and the
prior reopen criterion stands. `interface.c` was not modified, and the exact
`_interface_draw_screen` is untouched.
