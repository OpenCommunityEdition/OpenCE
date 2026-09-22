# path_obstacles worker report (Lane B)

Baseline: `== exact 15  residual 1  unwritten 0  (of 16 listed)`; `_obstacles_test_circle` residual 160 [sha].

## Evidence (before compiling)
- January vs ours: identical except [0x4d,0x5d): J = `movsx/lea/lea eax` ; `fld [eax+0x10]` ; `fadd [ebp+0x14]`; ours = `fld [ebp+0x14]` ; `movsx/lea/lea` ; `fadd [eax+0x10]`. No frame both, 2 rets both, reloc multiset equal (4).
- /Od fn 0x4c9290 = obstacles_test_circle: locals disc_index [ebp-4] (short), disc [ebp-8]; for-loop, `!= ignore`, accessor REAL call 0x4c8870, then `movss [ebp+0x14]; addss [eax+0x10]` = `radius + disc->radius`, real call point_in_circle(center,&disc->center,sum); 2 return sites (`mov ax,[ebp-4]` / `or eax,-1`). No other locals, no other statements.
- /Od siblings: test_pill (0x4c9360) `disc->radius + radius`, J emits param-first; disc_tangents (0x4c8730) `disc->radius + radius + K`, J emits param-first. test_circle is the only J site that is field-first.

## Attempts
- DIAG M8 (instrument only, never a landing): D=1..8 dead `long x = 0;` locals -> all `residual 160 [sha]`. M8-INSENSITIVE.
- try_1 (/Od-attested `radius + disc->radius`): `residual 160 [sha]`, byte-identical diff to baseline. Inert (confirms ledger C1).
- DIAG H3 (instrument only): K=1..32 filler prototypes before the function, whole-unit gate each: all `== exact 15 residual 1`, target row unchanged `residual 160 [sha]`. H3-INSENSITIVE. Neither count key reaches this row.
- DIAG (not a shape for the target): test_pill real-call arg swapped to `radius + disc->radius` -> test_pill still EXACT. Source operand order is inert for real-call args too; VC7 canonicalises bare `param + field` to param-first.
- NEW EVIDENCE (in-tree counterexample, brief s3.6): `_objects_in_sphere` (source/objects/objects.c:2103, EXACT 464) is the same construct `point_in_sphere(center, &x->center, (x->radius)+radius)`; January there emits `fld [esi+0x5c]` (field) ; `fadd [ebp+0x18]` (param) = field-first, and the landed source parenthesises the FIELD operand. Second in-tree site: collisions.c:733 `(vector->j) + point->y`.
- try_2 `(disc->radius) + radius`            -> `== exact 16 residual 0 unwritten 0` EXACT
- try_3 `radius + (disc->radius)` (/Od order) -> `== exact 16 residual 0 unwritten 0` EXACT
- try_6 `radius + ((disc)->radius)` (canonical accessor-macro expansion) -> `== exact 16 ...` EXACT  (idempotent, not graded)
- try_4 `(disc->radius + radius)` whole-sum parens -> `residual 160 [sha]`, diff identical to baseline
- try_5 `(radius) + disc->radius` param parens     -> `residual 160 [sha]`, diff identical to baseline
- LAW measured: VC7 loads a PARENTHESISED x87 operand first in a commutative add; bare operands canonicalise param-first regardless of source order.

## STRIP TEST: FIRES
Plain spellings (`disc->radius + radius`, `radius + disc->radius`) are residual; only parentheses around the FIELD operand reach January.
/Od (0x4c9290) attests operand order `radius + disc->radius`, exactly two locals, direct call - it can neither show nor refute parentheses or a field-accessor macro.
No first-party-attested accessor-macro name exists (path.h has no macros; no surviving Bungie source; HCEX PDB lists params only), so the narrow owner admission is NOT satisfied by me.
=> candidate.c left BYTE-IDENTICAL to source (no admissible change). NEEDS OWNER RULING.
Witness: `owner_ruling_witness.c` (= try_3.c, one-line diff at line 414: `radius + (disc->radius)`):
  gate `== exact 16 residual 0 unwritten 0 (of 16 listed)`, guard passed, unitcheck gains=1 regressions=0, newsyms OWNERSHIP PASS (0 new), fake scan 0 leads.

## Final (candidate.c)
`== exact 15  residual 1  unwritten 0  (of 16 listed)`; guard passed; unitcheck NO CHANGE; newsyms PASS; fake scan 0 leads.
Shapes measured on target: 6 (try_1..try_6). Diagnostics: M8 sweep, H3 sweep, test_pill swap.
