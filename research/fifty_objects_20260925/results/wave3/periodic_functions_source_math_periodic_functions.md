# source/math/periodic_functions

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/cand2.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/production_combined.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/LEDGER.md",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/v/FINAL2.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/v/B1.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/pg.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/od_builder.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/cand2.obj"
]

## production_changes
This is a zero-credit fuzzy improvement plus an authenticity fix. The integrator must apply it AFTER the ready data_gap packet or in its place.

(1) Source. Choose one of these:
  - (a) Apply scratch/w/data_gap/patches/periodic_functions.patch, then scratch/w/periodic_functions/production.patch. I verified this in a temporary repo: the result is byte-equal to cand2.c.
  - (b) Apply scratch/w/periodic_functions/production_combined.patch directly against the current tree. `git apply --check` is OK. It contains data_gap's storage/ownership changes plus mine.
  The resulting file is scratch/w/periodic_functions/cand2.c. It is CRLF, as in production.
  My changes on top of data_gap:
  - (i) Builder prototype and definition: the parameter is renamed `real *x_table` (the HCEX.pdb parameter name). The body is replaced by the shape of the /Od build (0x6c8460):
```
real sum = 0.0f;
short index;

for (index = 0; index < PERIODIC_FUNCTION_TABLE_SIZE; index++)
{
	x_table[index] = sum;
	sum += (real_random()+1.0f)*0.25f +
		((real)cos(8.2f*_pi*index/PERIODIC_FUNCTION_TABLE_SIZE)+1.0f)*real_random() +
		((real)cos(10.2f*_pi*index/PERIODIC_FUNCTION_TABLE_SIZE)+1.0f)*real_random() +
		((real)cos(14.6f*_pi*index/PERIODIC_FUNCTION_TABLE_SIZE)+1.0f)*real_random();
}

for (index = 0; index < PERIODIC_FUNCTION_TABLE_SIZE; index++)
	x_table[index] = x_table[index]/sum;

return;
```
  - (ii) In _periodic_function_build_table, the noise case `real_seed_random(get_global_random_seed_address())` becomes `real_random()`. This is byte-inert, and build_table stays exact.

(2) config/parked.json: re-baseline the `@periodic_function_build_variable_period_x_table@4` entry.
  - base: size 256, relocation_count 18, normalized_sha256 c5280305149a6466d123f479355c2a15167f284c1c358403a9d5b4f20e48a54d.
  - objdiff_percent: recompute with `python -B -m tools.parked_functions` after the build.
  - Evidence text: one int/x87 transposition remains, at 0x6f.

(3) symbols.json: no edits beyond data_gap's periodic ops in scratch/w/data_gap/patches/symbols_json_all.patch. The builder's symbol is unchanged.

(4) No header is edited. Only periodic_functions.obj changes. The new surplus is the COMDAT `_real_random` (16 B):
  - It is identical to January's selected copy, in action_charge.
  - provider_link passes in both orders.
  - January lists it in the cachebeta publics.
  - The exact caller _periodic_function_build_table justifies it under Rule 6. The builder also references it but is not exact; flag this for the reviewer.

(5) Status: stays NonMatching. The park stays.

## evidence
Baseline: the production builder was residual af8e65f5 against January 802ef4da. It differed in the EDX/ECX register plus the positions of `inc` and the store.

Negative oracles:
- TU name-count oracle (0..48 struct tags before the private code, on three shapes): inert.
- Preceding function IL, float literals and string literals: inert.
- Unused and dead-assigned locals 1..8: inert.
- /G6, /G7, /GB: same as the default.

Step 1, B1. Swapping to `index++; destination++;` gives EDX and January's integer order. The integer ops' register comes from the IL order of the two increments. D-before-I gives ECX in the 117-compile lab grid.

Step 2, /Od readout.
- 0x6c8460 calls fn 0x42e1c0, whose body is real_seed_random(get_global_random_seed_address()), i.e. real_math.h `real_random()`, four times. It calls fn 0x455220, a `cosine()` wrapper, three times.
- It uses a SHORT index, `values[index] = sum`, one accumulation expression, and no x/amplitude/cosine locals.
- Its constants are K = 25.761061 / 32.044247 / 45.867256, and it divides by 1024.0f.
- The folded-constant law holds: K = float(8.2f)*_pi, float(10.2f)*_pi and float(14.6f)*_pi, because the exact 8.2π and 14.6π values round 1 ULP lower. January's constants are exactly K/1024.

Step 3, the one-expression form (Y/N0/cand2).
- It reproduces the frame 0x1c.
- The dead `fst` stores after each fcos turn out to be the inlined helper return temporaries.
- It reproduces slot sharing, the call order, the fild placement between the 3rd and 4th calls, the cosine order, store-before-fcos and add/dec-before-fmul.
- `real_random()` is load-bearing: the hand-expanded call gives a completely different body.
- The fully /Od-faithful short-index `for` loop with x_table[index] and `/sum` compiles to the SAME bytes. VC7 itself strength-reduces x_table[index] to ESI, turns the loop test into the EDI down-counter, keeps the widened short index in memory, and converts /sum into *(1/sum).
- The invented count, destination, x, value, amplitude0-2 and cosine0-2 locals are all gone.

Audit (object_audit_split against the data_gap cfg_periodic2_split):
- 7 of 8 January-owned sections ok; only the builder differs.
- 46/46 symbols.
- All surplus passes provider_link.
- pdb_storage: the builder is static (HCEX static, absent from publics), and build_table is static via data_gap.
- /W3 /Zs: no TU-local warnings.

Full probe log: scratch/w/periodic_functions/LEDGER.md (P0-P38).

## blockers
@periodic_function_build_variable_period_x_table@4 is still residual: 256/18, sha c5280305. The only machine difference is one dependency-free transposition at 0x6f/0x70:
- January: `fmul 0.25; inc edx; fld [ebp-4]`
- ours: `fmul 0.25; fld [ebp-4]; inc edx`

It is a tie in C2's DAG scheduler (dag.c) between the index increment and the reload of the (real)index CSE temporary, in the cycle after the 0.25 multiply.

About 150 natural respellings did not move it:
- term, operand and declaration orders
- loop forms, and the placement of the increments in the loop header or body
- index types, store forms and second-loop forms
- the cosine() vs cos spelling
- shadow real_math.h helper bodies

The name-count, IL, constant and local-count oracles were all inert.

The object cannot be Matching until this closes. The methodology stop rule fired.

## reopen_criteria
Reopen if any of these arrives:
- A decoded tie-break rule for the C2 dag.c scheduler. Use tools/c2_live_debug.py / tools/c2dbg32 on a cand2 compile and trace the ready-list choice between `inc edx` and `fld [ebp-4]` after `fmul 0.25`. That would name the IL property January differs in.
- Authentic January periodic_functions.c or real_math.h text.
- A same-compiler exact donor where a memory-homed IV increment at the loop latch issues before a ready x87 CSE-temp reload.

Ignore the recorded inert classes: term, operand and declaration orders, loop forms, index/count types, helper bodies, the TU name/IL/constant/local counts, and the CPU flags.

## task notes
1. gate.py only lists '_'-prefixed functions, so it silently skips fastcall '@' names. Its fn_syms filter needs '_' or '@'. My wrapper is scratch/w/periodic_functions/pg.py; it takes an optional --target <scratch split>.

2. Rule-6 interpretation needs a reviewer ruling. The `_real_random` COMDAT is caused by the exact _periodic_function_build_table, whose /Od-attested `real_random()` spelling is byte-inert. The non-exact builder uses it too but adds no new emission. `cosine()` / `sine()` in build_table break its exactness (4243566c / 43174e14). That is why the builder uses `(real)cos`: it is byte-identical to `cosine()` there and emits no `_cosine`.

3. Side observations, not landed:
   - transition_function_build_table's literals 3.1415927f and 1.5707964f are `_pi` and `_pi/2`.
   - HCEX names build_table's locals real_table, x_table, value, value0 and value1.
   - An alternative constant spelling, `(_pi*2.f)*4.1f/5.1f/7.3f`, folds to the same January constants.

4. Landable without the COMDAT question: B1 (swap `index++; destination++;` in the current builder, sha 44f594de). It fixes the register but keeps the invented locals and the hand-expanded helper. cand2 is preferred.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/periodic_functions/LEDGER.md
