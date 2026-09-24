# res9 / alloc-oracle - VC7 integer allocator reachability oracle

## Instruments (all in this directory; production tree untouched)

- `dbg32c.c/.exe` - copy of res8's debugger + three additions:
  * `dbg_chain.txt`: up to 4 pointer-chain dumps per light hit (`reg n off1 off2..;...`)
  * `dbg_force.txt`: `<bp> <hit> <kind> <reg> <off> <val>` - set a register (kind 0) or
    write a dword to [reg+off] (kind 1, reg 6 = esp) at a given breakpoint hit. This
    FORCES an allocator decision inside the real C2.Dll.
  * built with the tree's VC7 CL/Link + Win10 SDK kernel32.lib (PowerShell).
- `trace.py` - per-TU trace: MARK 0x10720825 (one hit per function, in object section
  order - verified 18/18 props ... every unit), ACC 0x1072a527/0x1072a54c (priority +=
  refs x P), SUB 0x1072ab30 (priority -= P for live-unreferenced points), NEIGH 0x1072d9c0
  (each UNCOLOURED neighbour id the chooser visits), CHOOSE 0x1072da96 (eax=web, ebx=reg,
  cost table 0x10864600, allowed-set chain [[W+0x20]]), FREE 0x10719244 (web record
  destroyed; ebx=W, edx=id - ids are RECYCLED).
- `oracle.py <src> <unit> <fn>` - per-function webs in pop order + model checks R0-R3.
- `validate.py` - lane-wide validation (all 11 units, production sources).
- `forcesearch.py <src> <unit> <fn>` - compiler-in-the-loop counterfactuals: force each
  decision to each allowed alternative, recompile, measure vs January (strict EXACT, size,
  register-aware and register-blind instruction diffs).

## Decoded model (what the oracle asserts)

Web record W (0x48 bytes): +0xc priority, +0x14 next-in-list, +0x18 refs in current point,
+0x1c id, +0x20 -> allowed-register set, +0x38 own preference list {next, regnode, weight},
+0x40 weight used for the 100x single-register neighbour cost, +0x44 position (tie key).

- R0 PRIORITY. At every IL point p with pressure P(p) = (popcount of a point set + number
  of webs referenced at p + flagged live webs) x 2^depth:
  referenced webs:           pri += refs_p(W) * P(p)
  live but unreferenced webs: pri -= P(p)          (0x1072ab30)
  i.e. priority = weighted references MINUS weighted live-through span. Not divided by
  length, but the live-range DOES cost priority. Exception: webs appended to a special
  list inherit the tail's priority (0x1072f6c2..f706) - 7/3961 decisions.
- R3 ORDER. Pop order = sort by (priority, [W+0x44], id) all DESCENDING, within a pricing
  batch (the allocator prices and colours in several batches per function).
- R2 ALLOWED. The allowed set at a decision excludes every register already given to a
  neighbour (neighbour edges read from the chooser's uncoloured-neighbour iteration, id
  generations tracked across FREE), plus static class exclusions (ebp; scratch regs for
  call-crossing webs; esi/edi for byte webs).
- R1 CHOICE. cost[r] = sum over uncoloured neighbours n (with [n+0x40] > 0) of n's
  preference weight for r (if r in n.allowed) + 100*[n+0x40] if n has exactly one allowed
  register r; MINUS W's own preference weights. Choose the minimum-cost allowed register in
  class order eax, ecx, edx, esi, edi, ebx (ties keep the earlier).

## VALIDATION (production sources, all 11 units, 524 EXACT functions, 3,961 decisions)

    R0 priority  3954/3961  (7 = tail-inherited priority, identified mechanism)
    R1 chooser   3961/3961
    R2 allowed   3961/3961
    R3 order     3018/3020  (2 = equal pri AND pos, id order reversed; recycled ids)
    every rule on every decision: 517/524 functions

## FIRST RESULT: forcing proves single-decision residuals
_actor_destination_update: forcing decision k=1 (web id 7, eax -> ecx) makes the
function STRICT EXACT (976/976, d=0). Confirms res8's claim with the compiler itself.

## ROUND 2 (00:30) - the SKIP gate, constant webs, gated tracing, greedy counterfactuals

### Gated tracing
`trace.run_gated()` uses dbg32c gate mode (MARK counts, everything else planted only in
the target function's window): 1.3 s per traced compile instead of 11.5 s, and FORCE hit
numbers become 1-based inside the function.

### R4 - the colour/skip gate at each pop (0x10727ce3..), decoded + validated
Every popped web goes through [W+0x40] = BENEFIT (not priority):
  benefit < 0  -> skip (spilled / rematerialised)
  benefit == 0 -> colour only if single-block ([W+0x28]==[W+0x2c]) AND not constant AND
                  refcount [W+0x24] > 2; else skip
  benefit > 0  -> colour (if flag 0x400 clear an extra check 0x10795bb6 runs - undecoded)
  then colour only if the allowed set is non-empty (else spill).
BENEFIT = sum over uses b(use)*2^depth - sum over defs c(def)*2^depth
  (0x1072a6bb/0x1072b437 add, 0x1072b086 subtract, 0x107794ca bonus)
  - variable webs: b = 2 per use; a def from a param home / constant costs 2
  - CONSTANT webs (operand kind 0xd): def costs 1; b(use) = 1 iff the use's OTHER operand
    is memory (kind 2, or kind 6 with base/index), else 0 (register compare, return copy);
    +1 per use later when the other operand's web is SPILLED (0x10779432 -> 0x107794ca).
  => a constant is pinned in a register iff (memory-operand uses) > (materialisations),
     weighted by 2^loop-depth.
Validation on the 524 EXACT functions: R4 6612/6620 pops (8 misses, all benefit==0),
plus 26 pops on the undecoded benefit>0/no-0x400 branch (12 coloured / 14 skipped).

### Constant -1 in _actor_create_for_unit (measured)
The -1 constant web EXISTS in our build (last round, id 21 gen 3) with benefit exactly 0:
  def -1, uses +0 (cmp eax,-1: register operand), +1 (cmp [ebp+0x10],-1: memory),
  +0, +0 (the two return copies).  -> skipped -> rematerialised as immediates.
Forcing benefit := 50 at that pop reproduces January's pin (`or esi,-1; cmp eax,esi;
cmp [ebp+0x10],esi; mov eax,esi`) but NOT the push-ebx sinking / fifth epilogue:
d 41->22, dblind 16->14. So create_for_unit = constant-web gate + an independent
return-block merge (the guard and biped returns share one epilogue in our IL).

### Greedy compiler-in-the-loop search (moves: chooser register, skip, colour)
  _encounter_create   N1 (spill web id 28 = encounter_definition, esi, pri 62, benefit 20)
                      -> d 86->13, dblind 18->2. Remaining diff = ONE non-allocator region:
                      our IL threads the `actor_type == _actor_marine` re-test in the
                      fall-through random case (jmp 0x22b); January re-tests
                      (mov ecx,[ebp-0x14]; cmp cx,7; jne). Threading happens in the forced
                      build with actor_type SPILLED too, so it is not an allocator effect.
                      setpri(id28 := 30) gives the identical result (model-predicted spill:
                      esi/edi/ebx all taken by higher-priority neighbours).
  _actor_customize_unit N2 (skip id 12) -> size 640 == January, d 124->12, dblind 29->2;
                      + C3 (id 25 eax->edx) -> dblind 0 (register-blind IDENTICAL), d 27.
  _actors_spawn_from_unit C9 (the -1 web id 35 ecx->edx) -> size 672 == January, d 26,
                      dblind 20: the remaining difference is structural (arm order/merge).
  _actor_perception_friend_prop_is_attacking: no chooser/skip move helps (d 4 floor);
                      forcing the FALSE-constant web (pop 8) coloured makes the byte temp CL
                      (January's register) but moves the jump targets.
  _prop_get_active_by_unit_index: no move helps (dblind 2 = copy placement, not colour).
  _ai_test_line_of_sight: size stays 1040 vs 1008 under every move (frame/structure).

## ROUND 3 (01:30) - end-to-end replay validation, setpri thresholds, per-residual facts

### R5 end-to-end replay (oracle.replay): from the allocator's INPUTS (priority terms,
benefit, flags, static class, interference edges, cost table) re-run the pop loop:
  pop outcome + register for every popped web: 6638/6646 on the 524 EXACT functions
  functions predicted perfectly: 518/524; with >=10 coloured webs: 102/105
  (misses = the 8 benefit==0 gate edge cases of R4). Pop ORDER 6470/6646: re-pops of
  webs skipped with benefit<0 are re-appended at the list tail (0x1072f411 -> 0x1072f6c2,
  priority := tail priority) - modelled; residual order slips are positional cascades.

### setpri (force a web's final priority at its last pricing event) - thresholds MEASURED
 * _prop_get_active_by_unit_index, NATURAL a1 shape `while ((prop_index = next_prop_index)
   != NONE)`: forcing decision k=1 (state web id 13 ecx->edx) -> STRICT EXACT 144/144.
   setpri: state <= 36 -> EXACT, 37 -> not; cursor >= 40 -> EXACT, 39 -> not. Tie broken by
   [W+0x44] (cursor 0x1f > state 0x1d) exactly as R3 says. Needed swing: 4 priority points.
   state = 4x8 + 2x4 = 40 ; cursor = 2x2 + 4x6 + 4x8 - 24 (live-through) = 36.
 * _encounter_create: web id 28 = actor_type (NOT encounter_definition). setpri(actor_type):
   <= 34 -> January's allocation (d 13, dblind 2); 35..36 -> other; >= 50 floor. Needed:
   62 -> 34 (-28). Competitor = initial_variant (id 25, 34 = 160 - 126 live-through).
 * _actor_customize_unit: web id 12 = unit_index early piece (ebx). setpri <= 52 -> January
   allocation (size 640, d 12, dblind 2); 53 -> floor. Needed 64 -> 52 (-12); competitor =
   the actor_variant_definition piece id 2 (53, allowed {ebx}).
 * _actor_create_for_unit: spilling the unit_index early web (pop 15) makes the -1 constant
   web pinned in esi WITHOUT forcing it (the 0x107794ca spill bonus lifts its benefit 0->1),
   but then unit_index is reloaded at 0x87 where January reuses eax -> d 141. So January's
   benefit came from elsewhere: the NONE must be a VARIABLE web in January (actor_index not
   constant-propagated into the guard/biped returns: benefit -1 + 2 per use), cf. res6 s1.
 * _actors_spawn_from_unit: skipping the -1 web (pop 29) does not remove the pins: the next
   round re-splits it per region into two constant webs (ids 38, 13), each benefit +1
   (two memory-operand compares, one materialisation) -> pinned again (ecx, eax).
   The pin is decided by the compares' CO-OPERAND FORM (memory folds), i.e. expression shape.
 * _actor_input_update: allocator moves C9+N36 -> d 83->17 but register-blind 12 remain:
   frame slot [ebp-8]/[ebp-4], `lea edx,[ebx+0x1ec]` vs `add ebx,0x1ec` (accessor-binding
   law), x87 operand order in two products -> not a register-only residual.

## ROUND 4 - pressure decoded, variant probes, CLASSIFICATION (final)

### Pressure P, decoded (pressure.py): the "point" is a BASIC BLOCK
  P_b = n_b * 2^depth_b, n_b = number of DISTINCT webs referenced in block b (the popcount
  set was empty in every block measured). Closed form:
    priority(W) = SUM_{b refs W} refs_b(W) * n_b * 2^d_b  -  SUM_{b where W live, unreferenced} n_b * 2^d_b
  (refs counted 2 per operand occurrence). Validated as R0 (3954/3961).

### Variant probes (all ZERO bytes, none proposed)
  var/cu_nest.c  (grenade_count nested as the argument, January's 0x18e push order):
       inert on the floor; with the unit_index spill forced it is WORSE (dblind 5 vs 2).
  var/ec_e1.c    (normal case without fall-through): 768 B, worse; greedy cannot recover.
  var/cfu_s1.c   (res6 single-exit guard, which pins -1): greedy best dblind 16, d 107.

### Old claim tested: "allocator record has no spill-cost field (definition-position driven)"
  REFUTED for the integer web record. [W+0x40] is a benefit (spill-cost) field accumulated
  per use/def (0x1072a6bb, 0x1072b437, 0x1072b086, 0x107794ca) and it GATES colouring;
  [W+0xc] priority = refs x block pressure minus live-through pressure; [W+0x44] position is
  only the second sort key. The 2026-08-28 watchpoint result was on the 96-byte
  interference/home node family (+0x24 home offset), a different record.

### Classification (details in the structured report)
  _prop_get_active_by_unit_index  UNREACHABLE (natural source): natural a1 shape is ONE
      forced decision from STRICT EXACT; needs cursor priority >= state priority (+4); every
      term sits on blocks/operands byte-identical to January; the a1 family is one IL.
  _encounter_create               UNDETERMINED: allocator part = actor_type spilled (62->34);
      plus an IL threading difference (fall-through re-test) that no allocator move changes.
  _actor_customize_unit           UNDETERMINED: allocator part = unit_index early web spilled
      (64->52), plus one emission-order block (0x18e) that no allocator move changes.
  _actor_create_for_unit          UNDETERMINED: constant-web benefit 0 -> >=1 (NONE must be
      a variable web) plus the guard/biped return merge + push-ebx sinking.
  _actors_spawn_from_unit         UNDETERMINED (not a colouring residual): the -1 pin is the
      benefit gate on MEMORY co-operand compares; January compares registers.
  _ai_test_line_of_sight          UNREACHABLE by allocation: no allocator counterfactual
      moves the 1040 vs 1008 size; the gap is the frame/structure (R26, ruling-blocked).
  _actor_perception_friend_prop_is_attacking  UNDETERMINED: register-blind identical; the
      differing register is a block-local temp (al vs cl), not a global web; no single or
      greedy move reproduces January (cross-jump target also differs).
  _actor_input_update             UNDETERMINED: allocator moves d 83->17 but register-blind
      12 remain (frame slot, accessor-binding lea/add, x87 operand order).
  control _actor_destination_update: forced single decision -> STRICT EXACT (confirms res8).
