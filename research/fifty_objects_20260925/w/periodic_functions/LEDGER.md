# LEDGER - periodic_functions (wave 3+, slug scratch/w/periodic_functions/)

Target: @periodic_function_build_variable_period_x_table@4 (256 B / 18 relocs, parked instruction-scheduling,
target sha 802ef4da, base sha af8e65f5). Data/ownership fix ready in scratch/w/data_gap (cand_c.c).
Read: WORKER_BRIEF 0-8, house rules, lane ledger, 3 periodic ledgers (20260827/0830/0831), park entry,
data_gap note. Recorded negatives: 21 bounded natural-source experiments (details NOT in docs; donor history),
std-call vs sine/cosine/power wrappers, explicit vs implicit promotions (for math lowering) - do not repeat.

## Setup / baseline (2026-09-24)
- gate.py skips '@' names (fn_syms requires '_' prefix) -> wrote scratch/w/periodic_functions/pg.py (gate --all + strict
  compare of every .text fn incl. '@', optional --target split).
- base.c (= HEAD source): 6/7 exact; builder residual sha af8e65f5 (= park base), size 256/256, relocs 18/18.
- dg.c (= data_gap cand_c.c = HEAD + patches/periodic_functions.patch): vs data_gap cfg_periodic2_split: 6/7 exact,
  builder af8e65f5 unchanged. dg.c is my working base (the integrator will land the data_gap packet).
- Residual (sbs): January `mov edx,[ebp-8]` @63, `inc edx` @6f BEFORE `fld [ebp-4]`, `dec edi` BEFORE the 0.0448 fmul,
  `mov [ebp-8],edx` BEFORE fcos. Ours: ECX, `inc ecx` after the fmul, `mov [ebp-8],ecx` after fcos. Everything else
  (frame 0x1c, slots, x87 order, EBX private ABI) identical. => index++ temp register + its 2 insn positions.
- Prior negatives found in C:/halo-worktrees/fable-exact-finish-20260829/scratch/periodic_r*.txt (24 distinct fa050
  bodies, Aug 30, TU then used code_000fa050 names and the bss_004561bc struct): for(index) vs count-down, index++
  placement (after value, end), destination++ placement, values[index] vs pointer, cosine single vs 3 vars, amplitudes[3]
  vs scalars, declaration order x/index, sum init separate, != loop test. No results file; final retained = base.
  None of those were run under the name-count oracle (decoded 2026-09-21).
## P0 name-count ORACLE (dummies, oracle only) - NEGATIVE
- Hypothesis: EDX/ECX tie is a TU declared-name-count (arena ordinal) tie. Inserted N opaque `struct periodic_oracle_k;`
  before `/* ---------- private code */` in dg.c, N = 0..48 (> 2 periods of 22). Builder sha af8e65f5 for EVERY N,
  other 6 fns exact throughout. => NOT a TU name-count tie; the residual is set by the function's own IL.
## /Od readout (halo_cache_symbols.exe 0x6c8460, called from build_table 0x6c79e1; later SSE revision)
- frame 0x3c; locals: sum [ebp-4] (= 0.0f init first), index SHORT [ebp-8]; loop `for (index=0; index<0x400; index++)`
  with `values[index] = sum` (indexed store, no pointer local); all other slots are x87->SSE call-return temps and
  spilled partial sums (one big expression per iteration, random() calls interleaved with cos). Second loop
  `for (index=0;...;index++) values[index] /= sum` (later revision divides). No named x/amplitude/cosine locals in /Od.
  Formula differs from January (later revision: index*K/D, r*0.25+0.25) -> shape/names only.
## P1 A1: natural /Od-shaped `for (index...) values[index] = sum` (no count/destination) - NEGATIVE (structural)
- size 240 (!=256), index enregistered in ESI, values moves to EDI, caller _periodic_function_build_table breaks
  (private ABI). => January's count-down EDI + ESI pointer + memory index are explicit source IVs (VC7 does not LFTR here).
## P2 B1: base with `index++; destination++;` (swap of the two end-of-body increments)
- Hypothesis: IL order of the three IV updates is January's integer-op order (inc idx, add esi, dec edi); ours is
  destination, index, count. Result: register flips to EDX (= January) but schedule still differs:
  Jan: fmul .25; inc edx; fld x; add esi; dec edi; fmul K1; mov [ebp-8],edx; fcos
  B1 : fmul .25; fld x; inc edx; fmul K1; add esi; dec edi; fcos; mov [ebp-8],edx   (sha 44f594de)
  => register choice follows IL order of index vs destination update; remaining = placement of the int ops
  relative to the first cosine (list-scheduler tie).
## P3 LAB grid (lab only): index++ (I) x destination++ (D) positions over the 13 body statements (117 compiles)
- No exact. Classes: I before `sum += value` -> increment hoisted ABOVE the 4th call (x/index slots swap) - not January.
  I after sum: D after I (B1, 44f594de) = EDX + January's relative int order (load, inc, add esi, dec edi, store) but
  every int op issues one x87 op later than January. D before I anywhere -> ECX/EAX. D early (1..12) with I=13 ->
  `add esi` hoisted, store+dec reach January's slots but register ECX and inc after fld.
  => January's IL: index update after the sum statement and before the destination update; the residual is the
  x87/int interleave of that tail (Jan: X1 inc X2 add dec X3 st X4; B1: X1 X2 inc X3 add dec X4 st).
## P4 C0/C1: real_math.h `cosine()` helper instead of `(real)cos()` (new purpose: IL shape, not lowering) - INERT
- C1 (B1 + cosine) = 44f594de, C0 (base + cosine) = af8e65f5. No COMDAT effect on the builder.
## P5 vlab (lab) on B1: 12 natural re-spellings - ALL INERT (44f594de)
- V1 split value statement; V2 `value +=`; V3 `value = value + ...`; V4 `while (count--)`; V5 `count>0`; V6 do/while(--count);
  V7/V8 increments in the for-header (count--,index++,destination++ / index++,destination++,count--); V9 `sum = sum + value`;
  V10 0.25f*(...); V11 ++index; V12 index += 1. => VC7 normalises all of these to the same IL for this residual.
## HCEX.pdb (DIA2Dump -sym): builder = `static void periodic_function_build_variable_period_x_table(float *x_table)`,
  param `x_table` enregistered, NO stack locals recorded (PPC). build_table locals: real_table[0x400], x_table[0x400],
  value, value0, value1; params function_type, integer_table.
## P6 dlab (lab): declaration order of index/count/sum/x/value/destination first/last on B1 - ALL INERT (44f594de)
## P7 M8 oracle (lab): 1..8 unused locals in the builder, B1 and B0 shapes - ALL INERT.
## P8 donor census (scratch/w/periodic_functions/donor_scan.py over build/split): memory-homed int IV increments
  interleaved with x87. Donor objects.c _object_choose_random_change_colors (exact, admitted): a plain
  `for (cc_index=0; cc_index<NUMBEROF(..); cc_index++)` with arrays[cc_index] -> VC7 itself created pointer IVs
  (add esi/ebx,0xc) + down-counter (dec eax) and memory-homed cc_index; tail order inc idx, add ptrs, dec count, store.
  => hypothesis: January's pointer/count IVs are COMPILER-created (strength reduction) and sit after the source
  index++ in IL, which is the January int order. Same-TU exact builders use `index = 0; for (count = N; count; count--)
  { ... table[index] = ...; index++; }` (indexed store, no destination pointer).
## P9 E2 (same-TU style: index=0; for(count){ values[index] = sum; ...; index++; }): size 240, index gets ESI, no
  pointer IV -> January's explicit destination pointer is required (it takes ESI and pushes index to memory).
## P10 flab: F1 for(index)+destination (240, no count IV), F3 both in for-header (240), F2 pointer-controlled
  `destination < values+N` (cmp esi,edi; B1-like interleave), F4 `destination != end` (53b922e8). None exact.
  => explicit down-count `count` is required (VC7 does not create it here).
## P11 glab: G1 `sum += (cosine2+1)*amplitude0 + value` (no final value store), G2 `value +=` + folded sum, G3
  `sum = value + sum` - ALL INERT (44f594de).
## P12 name-count ORACLE on B1 shape (0..40 struct tags before private code): ALL 44f594de -> TU count inert here too.
## P13 hlab: operand orders K*x (all/first), amplitude*(c+1), (1+rand), (1+cosine0) - ALL INERT (44f594de).
## P14 xlab: position of `x = (real)index;` - after amplitude2 (B1) and after the value statement (X4) give the
  SAME bytes (VC7 hoists the fild above the 4th call); earlier positions differ. NX0/NX1/NX2: no named x
  (`cos((real)index*K)` / `cos(index*K)`) = identical bytes to the named-x forms (x slot is a CSE temp).
  /Od 0x6c8460 has no x local (cvtsi2ss of index at each cos) -> the no-x spelling is the /Od-attested one.
## P15 slab: `short index` (HCEA decompile `(int16_t)(i+1)`, /Od word index) - NOT January: index enregistered
  (movsx+temp fild), private ABI moves param to the stack, caller build_table breaks. Rejected.
## P16 plab: explicit `count--` statement in while/do-while bodies, all 6 orders of {index++, destination++, count--}
  -> only I-vs-D order matters (I first = 44f594de, D first = af8e65f5); count position inert.
## P17 flags (diagnostic only): /G6 /G7 /GB = default (B1 44f594de, B0 af8e65f5); /G5, /Op, /QIfist break others.
  Scheduling model from the data: x87 ops keep IL order; int ops are inserted between them. January gives the int
  ops PRIORITY over ready x87 ops (inc before fld, add/dec before fmul K1, store before fcos); B1 gives x87 priority.
  Hypothesis: priority follows IL position, i.e. January's int statements precede the cosine statements in IL while
  staying after the 4th call (int statements written before `sum += value` float above the call in our lab).
## P18 klab: x assigned after the 4th call and index++ right after it (K1/K4) or x=(real)index++ (K3) -> hoisted
  above the call again (6513/bb82): the increment's load CSEs with the fild. Only increments written after
  `sum += value` stay after the call (own `mov reg,[ebp-8]`). In K2 (I hoisted, D at latch) add esi/dec edi issue
  BEFORE fmul .25 -> int ops in the latch are not intrinsically low-priority; priority depends on the IL mix.
## P19 elab (diagnostic, index escaped via extern call): increments IL-early (after value stmt) -> store moves before
  fcos but inc still after fld (E1); IL-late -> B1-like (E2). IL position alone does not give January.
## Status: B1 (`index++; destination++;`) = best: register EDX + January int order; remaining = 2 x87 ops (fld x, fcos)
  each one int-group later than January ([F1 I][F2 A C][F3 S][F4] vs B1 [F1][F2 I][F3 A C][F4 S]) - a scheduler
  tie at the cycle where `inc edx` and `fld x` are both ready (Jan picks inc, we pick fld).
## P20 fn-oracle: dummy external function with 0..12 statements before private code - INERT (TU IL numbering is not it).
## /Od callee identification (NEW EVIDENCE): builder 0x6c8460 calls fn 0x42e1c0 (4x, no args, x87 return) whose body
  is `real_seed_random(get_global_random_seed_address())` = real_math.h `real_random()`; and fn 0x455220 (3x, one
  real arg) = a one-line wrapper = real_math.h `cosine()`. => the genuine spelling is real_random() / cosine().
## P21 rlab: real_random() (/Od-attested) with/without cosine(): R1/RC1 = 44f594de, R0/RC0 = af8e65f5 - INERT but authentic.
## Hypothesis H-EXPR (from /Od 0x6c8460 shape): the later source is ONE accumulation expression with real_random()
  and cosine() calls inside it (no named amplitude/cosine locals). January's call order (1st random pairs with the
  LAST-computed cosine, 4th random = 0.25 term) is what right-to-left hoisting of calls out of
  `(r+1)*0.25 + (cos(a)+1)*r + (cos(b)+1)*r + (cos(c)+1)*r` gives, and the dead `fst` stores after each fcos can be
  the inlined cosine() return temporaries rather than named locals. Testing.
## P22 xplab H-EXPR: ONE `sum += (real_random()+1)*0.25f + (cosine(x*K)+1)*real_random() + ...` statement
  (no amplitude/cosine/value locals) -> frame 0x1c, dead `fst` after every fcos (inlined cosine() return temps), slot
  sharing, 4th call = 0.25 term, AND `mov [ebp-8],edx` before fcos all reproduced (sha 1e31a0ed). Remaining vs Jan:
  (a) fild of x before the 3 calls (x assigned before the statement), (b) cosine computation order is
  0.0252,0.0313,0.0448 (Jan 0.0448,0.0313,0.0252) - term order in source is INERT (X1 == X2: /O2 reassociates),
  (c) inc after fld. => the single-expression form is the right family.
## /Od constants (0x6c8460): cos args are `index*K/1024.0f` with K = 25.761061 (0x41ce16a7), 32.044247 (0x42002d4f),
  45.867256 (0x42377812); January's single constants 0x3cce16a7/0x3d002d4f/0x3d377812 are EXACTLY K/1024 (same mantissa)
  -> January's constants are folded K/1024 expressions (folded-constant law). /Od term order: 0.25-term, K=25.76,
  32.04, 45.87 (January computes cosines in the REVERSE order 45.87, 32.04, 25.76). /Od has no x local.
## P23 ylab (/Od-faithful H-EXPR, NO x local): `sum += (real_random()+1.0f)*0.25f + (cosine(index*K/1024.0f)+1.0f)*
  real_random() + ...` with folded, `K/1024.0f`, `K/PERIODIC_FUNCTION_TABLE_SIZE` constants, term order abc or cba:
  ALL sha c5280305 = ONE transposition from January: only `inc edx` <-> `fld [ebp-4]` at 0x6f/0x70 differ.
  Cosine order, fild position, store-before-fcos, add/dec-before-fmul now all match. BIG STEP.
## P24 zlab on Y: INERT (c5280305): random-left products, sum = sum + / + sum, separate value local, (1+r)*.25,
  (r+1)/4, (real)index, increments in for-header, (real)cos instead of cosine(), 0.25 term last, full parens.
  Z1 destination++ before index++ -> ECX (Jan EDX) confirms B1 order law. Z9 *destination++ = sum at top -> 80949b6e.
  Z12 real_seed_random(get_global_random_seed_address()) instead of real_random() -> totally different (calls stay
  interleaved, frame 0x14): the /Od-attested real_random() inline is LOAD-BEARING. Z15/Z16 (0.25 term 2nd, grouping)
  differ. Remaining in Y: only inc edx <-> fld [ebp-4] at 0x6f.
## P25 wlab: /Od loop forms with the expression: W1 for(index)+values[index] (240, = A1 c99820a0), W2 count+values[index]
  (240), W3 for(index,destination) (240), W4 `*destination++ = sum` in for(index) (ffc70890), W5 Y with values[index]
  second loop (240). => explicit count + destination (Y) stays the only full-envelope form.
## P26 qlab on Y: all 23 declaration-order permutations, while(count--), do/while(--count), ++pre, unsigned/short
  count, sum init as statement, both locals init-declared: INERT (c5280305). Qinit (only destination init-declared)
  reorders the prologue (Jan = `index = 0; destination = values;` statements). Name-count oracle on Y 0..44: inert.
## P27 q2lab on Y: `int index`, `(real)index*` INERT; `short index` -> index enregistered (S1 family), rejected.
## P28 olab: all 24 term orders of the 4 summands: only the 0.25 term's position matters (first/last = c5280305,
  2nd = 5e10353a, 3rd = 530be943; the latter two are farther). No exact.
## P29 oracles on Y: 1..8 unused long/real locals, 1..8 dead-assigned locals (before loop / in latch) - ALL INERT.
## P30 l2lab on Y: second-loop spellings (count-based, header increments, braces) INERT; values[index]*(1/sum) -> 240.
## P31 w2lab - MAJOR AUTHENTICITY FINDING: the fully /Od-faithful loop form
  `real sum = 0.0f; short index; for (index = 0; index < N; index++) { values[index] = sum; sum += <expr>; }`
  `for (index = 0; index < N; index++) values[index] = values[index]/sum;`
  (short index, indexed stores, division - exactly /Od 0x6c8460's topology) compiles to the SAME bytes as Y
  (c5280305): VC7 itself strength-reduces values[index] to ESI, LFTRs the short loop test into the EDI down-counter,
  widens the short IV to a memory-homed dword, and turns /sum into *(1/sum). No invented count/destination locals.
  Still one transposition (inc edx <-> fld [ebp-4]) from January.
## P32 w3lab (short-index family): sum = sum + / + sum, 0.25 term last, /PERIODIC_FUNCTION_TABLE_SIZE, `values[index] /= sum`,
  (real)cos, (r+1)/4, random-first products, ++index, sum init as statement - ALL c5280305 (inert).
## P33 w4lab (short family): while/do-while loops, unsigned short, (short)(index+1), *= 1/sum - ALL c5280305.
  Name-count oracle (0..34) and fn-oracle (1..8 stmts) on N0 - inert.
  Donor note: actions.c _actor_pursuit_find_nearby_actors (exact) issues `inc eax` between FP ops ahead of a ready fld,
  with `actor_count++` written BEFORE the FP statements and actor_count read again later in the block.
## P34 const-oracles on N0 (1..12 float literals, 1..8 strings in a preceding fn): inert.
## FOLDED-CONSTANT EVIDENCE: /Od K constants 0x41ce16a7/0x42002d4f/0x42377812 = float(8.2f)*float(_pi),
  float(10.2f)*_pi, float(14.6f)*_pi (exact 8.2*pi / 14.6*pi round 1 ULP LOWER: 41ce16a6 / 42377811) -> the source
  multipliers are FOLDED EXPRESSIONS `8.2f*_pi` etc. (equivalently float(2pi)*4.1f/5.1f/7.3f); January's constants are
  those /1024 exactly. real_math.h: `#define _pi ((real)3.14159265358979323846f)`.
## P35 pilab (N0 family): `8.2f*_pi*index/1024.0f`, `/PERIODIC_FUNCTION_TABLE_SIZE`, `index*(8.2f*_pi)/N`, `_pi*8.2f*index`
  = c5280305 (identical, constants fold to January's); `index*8.2f*_pi/1024` (ungrouped) = d258a28f (unfolded, wrong).
## P36 tlab2 (N0): `index != N`, decl order, *(values+index), 0.25f*(...) - inert.
## pdb_storage (production): 1 disagreement (_periodic_function_build_table public in ours, absent from cachebeta) -
  fixed by the data_gap packet (static). /Od build_table 0x6c7990 noise case calls fn 0x42e1c0 = real_random(),
  cos cases fn 0x455220 = cosine(), sin fn 0x455320 = sine() -> the production hand-expanded
  `real_seed_random(get_global_random_seed_address())` in build_table is a latent house-rule (hand-expanded helper) item.
## CANDIDATE cand.c (dg.c + /Od-faithful builder): 6/6 others exact vs data_gap split; builder c5280305 (1 transposition).
  Emits `_cosine` + `_real_random` COMDATs (16 B each): IDENTICAL to January's selected copies (actor_combat /
  action_charge), provider_link PASS both orders. Rule-6 blocker: header-inline COMDAT from a NON-exact caller.
## P37 shadow real_math.h oracle (sh/, never landed): cosine() as (real)cos / local / (double) = c5280305 (inert);
  real_random() with a `result` local = 6a1bf9de (farther). Helper bodies are not the lever.
## P38 build_table helper test (authenticity, /Od 0x6c7990 calls real_random/cosine/sine): noise case
  `real_random()` = BYTE-INERT (build_table stays exact); cosine()/sine() break build_table (4243566c / 43174e14).
  => with build_table's genuine real_random() the `_real_random` COMDAT has an EXACT caller in this TU. The builder
  can use (real)cos (N7: identical bytes to cosine()) so no `_cosine` COMDAT is emitted.
## FINAL CANDIDATE cand2.c (= data_gap cand_c + build_table noise `real_random()` + /Od-faithful builder with (real)cos)
- pg.py vs data_gap split: 6/6 exact, builder 256/18 sha c5280305149a6466d123f479355c2a15167f284c1c358403a9d5b4f20e48a54d
  (ONE transposition: Jan `inc edx; fld [ebp-4]` @0x6f/0x70, ours `fld; inc`). Production builder was af8e65f5
  (register ECX + inc/store positions).
- object_audit_split (SPLIT_ROOT=data_gap cfg_periodic2_split): all 8 January-owned sections ok except the builder;
  46/46 symbols; surplus = .rdata literals, _fast_ftol, _real_random (identical to action_charge's selected copy);
  provider_link PASS (all surplus incl. _real_random / _fast_ftol).
- /W3 /Zs: no TU-local warnings (no C4013). Only this TU changes (no header edits).
- Patches: production_combined.patch (vs HEAD; git apply --check OK), production.patch (on top of
  data_gap/patches/periodic_functions.patch; verified in a temp repo, result == cand2.c).
- Stop rule reached: after the /Od-faithful family (one expression, real_random(), no x, short index, folded
  8.2f*_pi constants) only one dependency-free int/x87 transposition remains, inert to ~150 natural respellings,
  TU name counts, preceding-IL/constant/string counts, local counts, helper bodies (shadow real_math.h) and CPU flags.
  C2 has a DAG scheduler (tools/c2dbg32/BACKEND_MODULE_MAP.md dag.c); decoding its tie-break is the reopen path.
