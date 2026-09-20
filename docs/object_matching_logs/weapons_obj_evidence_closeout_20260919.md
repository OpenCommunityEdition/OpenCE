# `weapons.obj` evidence closeout (2026-09-19)

## Scope and strict result

- Unit: `source/items/weapons.c`; January target:
  `build/split/source/items/weapons.obj`.
- Final strict gate: **75 exact, 4 residual, 0 unwritten** out of 79 listed
  functions.  The `_point_from_line3d` emitted-owner guard passes.
- No production source or configuration was changed in this pass and no fuzzy
  body receives exact credit.

## Evidence searched

1. January target instructions, relocations, symbol owners, public names, and
   the `cachebeta.exe` / `cachebeta.pdb` records.  The January PDB supplies the
   public `_weapon_place` name/RVA, but it has no function-type, argument,
   local, scope, or line record for the function.
2. Existing IDA material, Ghidra decompilations, January map-atlas reports, the
   Halo symbol-build executables, and HCEA.  Ghidra's inferred return value is
   derived from the final `mov eax, ebx`, not from type metadata.  HCEA and the
   datum placement callback in `objects/object_types.h` both retain the
   authentic `void (long, scenario_object_datum *)` contract.
3. Every locally preserved candidate-sized `.c` file beneath
   `C:\halo-worktrees`: 679 files reduced to 357 unique SHA-256 contents and
   recompiled against the current January object.  The only copy that raises
   the raw strict count does so by emitting a forbidden helper owner, described
   below.
4. The complete 100K/150K/throughput-recovery experiment sets, including the
   prior D1+rD `weapon_update` graph and all magazine reload spelling probes.

## Exact-but-inadmissible findings

### `_trigger_create_projectiles`

The preserved throughput-recovery candidate
`scratch/workers/w14_weapons/d_pfl.c` makes the 1,824-byte function strict
exact by calling `point_from_line3d` naturally.  VC7 also emits a 48-byte code
definition named `_point_from_line3d`.  January `weapons.obj` has no symbol of
that name at all, defined or undefined.  Therefore the candidate changes the
object's owner set and violates the January inline/COMDAT schedule.  Renaming
or hand-expanding the helper would merely hide the ownership error and was not
admitted.  The production 1,808-byte manual-coordinate body remains fuzzy.

### `_weapon_place`

Changing the reconstructed function's return type to `long` and returning
`weapon_index` reproduces all 208 January bytes.  This is not admissible
evidence for the source contract: the stripped January PDB has no type record,
the value is not consumed by any known callback caller, HCEA declares `void`,
and the owner callback typedef is `void`.  The final register residue is
therefore insufficient to justify a return-ABI rewrite.  The clean void body is
kept as a 208-byte zero-credit residual.

## Remaining residuals

| Function | Target / candidate padded bytes | Evidence result |
|---|---:|---|
| `_trigger_create_projectiles` | 1,824 / 1,808 | The only exact candidate emits the surplus helper owner above.  All no-owner macro/manual spellings remain nonexact. |
| `_weapon_magazine_finish_reload` | 224 / 224 | January keeps `rounds_total` in DX and the chosen load count in EDI; the reconstruction allocates the load count to EDX and rematerializes the reserve value.  The HCEX symbol PDB confirms only the private signature `static void (long, short)`.  MIN, ternary, if-clamp, short/long local, compound-assignment, scope, and declaration-order probes do not close the allocator tie. |
| `_weapon_place` | 208 / 208 | Exact only through the unauthenticated return-ABI change above; retained as void and zero credit. |
| `_weapon_update` | 2,800 / 2,816 production | The best preserved D1+rD graph is 2,800 bytes with exact relocation count and differs in only the recharge-block allocator web.  It is still nonexact.  Repeated array access versus a `trigger_down` local, recharge-local scope/type/order, statement order, MIN/clamp, and operand-order probes are exhausted without primary local records. |

The permitted `/QIfist` diagnostic was measured.  It closes none of these four
rows and regresses five already-exact weapon helper functions, so it is not a
valid unit option here.

## Admission result

No exact function or object was gained.  Two tempting byte-exact results were
rejected for concrete ownership/type reasons rather than credited
inappropriately.  Reopening this object requires either authentic source/type
records for the two ABI/allocation questions or an evidenced no-owner inline
mechanism that reproduces January's trigger code without defining a COMDAT.

## Exact-January Ghidra second pass

After the first closeout, the exact `cachebeta.exe` was imported and analyzed
in a fresh Ghidra project.  The read-only export is
`scratch/ghidra_weapons_residuals.c`; its program header identifies
`cachebeta-jan.exe` at image base `0x00400000`.  This is new **primary binary
evidence** for the four January control-flow graphs.  Ghidra's generated local
names and types remain inference and were not promoted to source authority.

- Reload's target graph reads `rounds_total`, reads `rounds_reloaded`, chooses
  the smaller signed value, adds it to the loaded-round word, clamps that word,
  and computes the reserve from the pre-clamp total and old loaded count.  The
  exact assembly keeps the original total in `DX`, the selected promoted value
  in `EDI`, and the new loaded word in `CX`.
- Update's target graph confirms a single short reload increment and the
  whole-round/fractional-round arithmetic.  The preserved D1+rD candidate
  `C:\halo-worktrees\opus5-throughput-recovery-20260916\scratch\workers\w2_weapons\wu1.c`
  remains the best lawful fuzzy result: 2,800 padded bytes and the exact
  relocation count, with a one-byte surplus caused by the recharge-block
  store/register schedule.  The source is not credited exact.
- Place still has only return-register residue, while the authentic datum-place
  callback and HCEA declaration both say `void`; the binary decompiler's
  inferred integer return does not override those type records.
- Trigger still proves the same natural helper call, but every tested VC7
  header/body spelling emits the raw `_point_from_line3d` COMDAT that January
  `weapons.obj` does not own.

The second pass measured the Ghidra-shaped reload graph, saved-total forms,
short/long intermediate forms, declaration permutations, all natural ternary
comparison orientations, and compound reserve expressions.  None closes the
allocator tie.  In particular, the target's `DX`/`EDI`/`CX` web could not be
obtained without unsupported register steering.  All such probes remain under
`scratch/weapons_finish_reload_*.json`; no production body was changed.
