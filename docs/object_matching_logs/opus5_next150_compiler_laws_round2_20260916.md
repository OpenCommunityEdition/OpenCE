# VC7 13.00.9254 laws, ROUND 2 (2026-09-16) - consolidated and self-corrected

Produced by a second three-angle probe on `_hs_compile_and_evaluate` (reference tables, corpus validation, residual
mining) plus a consolidation pass that RE-RAN the load-bearing measurements and changed two of the first round's
conclusions. Evidence and the detector inventory live in `scratch/hs_research/`.

**This document supersedes parts of `opus5_next150_compiler_laws_20260916.md`; its section 8 lists exactly what.**
Tags: `[M]` measured this round, `[V]` re-verified by the consolidation pass, `[I]` inferred.

---

**Lane** `C:\halo-worktrees\opus5-150k-house-clean-20260914`, baseline `ae10935da`, 2026-09-16.
**Supersedes** the LAW Z section of `docs/object_matching_logs/opus5_next150_compiler_laws_20260916.md`
(see §8 for the exact list of superseded clauses) and the reopen criterion in `scratch/hs_pin/impl.md`.

Source angles: `scratch/hs_research/angleA.md` (surplus-use hunt, ~120 whole-TU probes),
`angleB.md` (corpus validation + detector, 10,568 January functions scanned),
`angleC.md` (rest of the hs delta + corpus law mining, ~110 micro-TUs + 2 corpus scans).
Nothing under `source/`, `config/`, `tools/`, `build/` was modified by any angle or by this consolidation
(`git status --porcelain -- source config tools build` is empty).

**Evidence tags used throughout.** `[M]` = measured on January's bytes or on a gated object in this lane.
`[M-local]` = measured, but only within one function's shape (a calibration, not a law).
`[I]` = inferred from measurements, not directly measured. `[R]` = refuted.
`[V]` = re-verified by this consolidation pass with its own tool run (commands recorded in §9).

---

## 1. Executive summary

1. **LAW Z's quantitative boundary (`stores >= 2*U + 1`) is dead.** It is refuted in both directions on
   January's own bytes, with named strict-exact counterexamples. Delete it from the ledger and from every
   triage path. Only the *qualitative* half of Z2 survives.
2. **LAW Z1 is the real law and is now exact**: no January function with a whole-function zero pin has
   MAXLIVE >= 3 (343/343, zero counterexamples), and **the pinned register carries exactly one definition
   in the whole function** (Z1b) — the half that actually detects defects.
3. **Three new codegen laws**: Z4 (copy coalescing precedes web ranking), Z5 (web splitting is the only
   source lever on a join phi, and it forfeits the parameter home / arm cross-jump), Z6 (dead-parameter
   home packing shrinks the frame by 4).
4. **Two new laws outside the pin family**: LAW S (two-output frame-slot assignment is decided by which
   home is referenced *last*, so a slot mirror is never an independent residual) and LAW X (the
   cross-jumper merges arm suffixes compared *after* register allocation).
5. **`_hs_compile_and_evaluate` did NOT close**, and the three angles' dispositions conflicted. Adjudicated
   in §6: the function is **one legitimate zero-valued source statement away from a byte-exact front end**
   on January's own shape; `scratch/hs_research/lab_R6.obj` measures **592 padded / 580 meaningful / 40 of
   40 relocations / 183 instructions vs January's 183**, with the *entire* residual being that one 3-byte
   store plus a net-zero error-tail packet `[V]`. No such statement exists in January's bytes, and buying
   one is a fake match. It stays NOT LANDED.
6. **The highest-value product of the probe is not hs.** It is `lawz.py` (a validated triage detector, 0
   false positives on 227 strict-exact controls) and a named, census-corroborated source defect in
   `game_engine_king::_king_calculate_hill_state` — see §10.

---

## 2. LAW Z, consolidated and corrected

**Scope.** "Pin" means a *whole-function* constant-zero register: one definition of a callee-saved
register in the entire function, by `xor r,r` / `sub r,r`, live to the epilogue. A zero register that is
redefined partway through is not a pin (`actors::_actor_new`).

### Z1 — AVAILABILITY (CONFIRMED, sharpened) `[M]`
> VC7 forms a whole-function zero pin only when the function's other callee-saved values pack into the
> remaining two registers. Define **MAXLIVE** = the largest number of *non-pin* callee-saved webs
> simultaneously live.

| MAXLIVE | January functions **with** a pin | **without** |
| ---: | ---: | ---: |
| 0 | 70 | 4,002 |
| 1 | 96 | 1,884 |
| 2 | 177 | 1,457 |
| 3 | **0** | 2,882 |

343/343 pins across all 10,568 January functions in `build/split`, **no counterexample**. In the 240-row
cross-tab our build breaches the ceiling in exactly one function (`_king_calculate_hill_state`), and that
function is one of the four pin losses.

Supporting census `[M]`: pin register is EBX 259 / EDI 51 / ESI 33. EBX dominates because it is the only
byte-addressable callee-saved register (ESI/EDI have no 8-bit alias), so a pin serving any byte store is a
two-way contest for EBX. **172 of 343 pins leave a third callee-saved register entirely unused** — a pin is
normally taken with slack, not at the margin, which is why "availability" is not a tight predictor on its own.

### Z1b — SINGLE-DEFINITION COROLLARY (new; the operationally useful half) `[M]`
> The pinned register carries **exactly one definition** in the whole function. Any second web our spelling
> parks on the register January pins kills the pin by itself, even when MAXLIVE is within the ceiling.

Detects three of the four known pin losses. It is what `_hs_compile_and_evaluate`'s production body
actually suffers: our EBX carries `mov ebx,[ebp+8]` (def 0xa) and `lea ebx,[ebp-0x80c]` (def 0x137) where
January's carries only `xor ebx,ebx` (def 0x1a).

### Z2 — DEMAND (qualitative half survives; quantitative boundary REFUTED)
**Survives** `[M]`:
- Only zero **stores** seed and grow the web. `push 0`, NULL arguments and `cmp x,0` never seed it, though
  the web is spent on them once it exists (why January appears to have 25-31 "uses").
- Stores of different widths unify into one web.
- The seed position is inert (this is Z3).

**REFUTED** `[R]` — `stores(zero) >= 2 * uses(strongest interfering competitor) + 1`, in both directions:
- *Not necessary*: violated in **200 of 236** both-pin cross-tab functions. Among the 112 paired functions
  whose pin carries >= 1 store, the **minimum** store count is **1** at every competitor-use value U = 0..7;
  there is no monotone relation anywhere in the U = 0..40 table. Cleanest counterexample is a strict-EXACT
  control on both sides: `memory/zlib/gzio::_gz_open` pins EBX with **17 stores against a 38-use ESI
  competitor** (the boundary demands 77) and our build reproduces it byte for byte.
- *Not sufficient*: a whole-image scan finds **1,132** January functions with >= 1 constant-zero store and a
  callee-saved register the function never writes, that still do not pin; restricting to MAXLIVE <= 2 with
  >= 1 call still leaves **768**, of which 55 have >= 4 zero stores and 20 have >= 8. Extreme:
  `ai/action_obey::_action_obey_describe_command` — 49 immediate-form zero stores, 33 calls, EBX never
  written, **no pin**. At demand >= 8 with Z1 clear the split is 34 pin / 20 no-pin.
- *No correction term exists* `[I, from M]`: no monotone function of U, store count and block count can
  simultaneously admit `_gz_open` and exclude `_action_obey_describe_command`. The ledger's "extra basic
  blocks raise the threshold" note is therefore moot. The only block/size signal is a **selection effect**:
  pinning functions are simply bigger (median 57 insns / 10 blocks vs 31 / 5), as expected when the pin must
  pay for a `push`/`pop` pair.

### Z2-CARRIER — ZERO-DEMAND CARRIER LAW (new; a required measurement correction) `[M]`
> The same source statement can be lowered through the callee-saved pin on one build and through a volatile
> register or an immediate on the other. **Zero demand must be counted as the number of constant-zero STORE
> SITES regardless of carrier.**

Counting stores *through the pin* manufactures phantom "extra/missing zero statement" verdicts. It does so
on `main::_main_frame_rate_debug`, where the nine sites are identical and only the carrier of the store at
0x3c differs (January `mov byte ptr [0x386], dl`, ours `..., bl`). `lawz.py` counts carrier-agnostically
and then correctly reports CONSISTENT. **Corollary used in §6**: a carrier-agnostic census is also what
proves January has no hidden ninth zero store in `_hs_compile_and_evaluate` — a second, independent
confirmation of angle A's `prof.py` census `[V]`.

### Z3 — INTERFERENCE, NOT ORDINAL POSITION (unchanged) `[M]`
What competes is the **span** from a competitor's definition to its last use. A competitor whose uses lie
entirely before or after the zero stores does not compete at all; a one-use competitor never competes. The
zero variable's own seed position is inert.

**Regime warning (new, from angle A's I1/C5 rows)** `[M-local]`: deleting a competitor's *entry* reference
does not merely decrement its count — it moves the web's *start* past the pre-loop zero stores, so the
competitor stops overlapping them and Z3's "does not compete at all" regime takes over. A triage tool must
ask *where does the competing web start* before it asks *how many references does it have*.

### Z4 — COPY-COALESCING INVARIANCE (new) `[M]`
> VC7 coalesces `p = q` pointer copies **before** register allocation ranks webs, so a competing web's
> reference count is invariant under every spelling of the copy.

Measured on 18 house-rule-clean spellings of the compile-source pointer in `_hs_compile_and_evaluate`:
parameter reuse; `source = expression;` before the switch; the same after the switch; `char const *source =
expression;` at function scope, in a nested block, and at inner-block scope; the length staged through a new
local or through an existing one. Ten of them are **bit-identical**, sha `975490025660aada`, including the
two decisive rows Q1 and Q2 (the literal "bind the pointer once and let the call read the local" forms).
**Consequence:** you can never weaken a register competitor by re-spelling a copy — only by changing which
*values* reach a join. This is a structural refutation, not exhaustion.

### Z5 — WEB SPLITTING IS THE ONLY SOURCE LEVER, AND IT FORFEITS THE PARAMETER HOME (new) `[M]`
> A join phi can be separated from the incoming parameter only by assigning it on the fall-through
> (`case 0:`) edge. That reliably wins the zero pin. But the moment the join value is a variable distinct
> from the parameter, VC7 stops using the parameter home for the phi, carries it in a scratch register
> across the join, materialises it on every edge, and the switch arms **stop cross-jumping**.

Six independent spellings (N1, N2, N3, N4/M4, N6, P7) are all 608 padded / 599 meaningful / pin EBX /
sha `46533e07b511fb25` — declaration scope and position of the local are completely inert. Cost of the
split: **+22** meaningful bytes for the N-family (January's shared 24-byte tail `lea ecx,[expanded]; push
ecx; call _sprintf; lea edx,[expanded]; add esp,0xc; mov [ebp+8],edx` disappears), **+8** for `v1.c`
(selection after the switch).

**Reusable detector**: a January-shaped `mov [ebp+8], <ptr>` on a merged tail **proves the source reassigns
the parameter itself**, and therefore rules out every local-variable phi spelling for that function.

### Z6 — DEAD-PARAMETER HOME PACKING (new) `[M]`
> When a parameter is genuinely dead after its last read, VC7 packs a later local into its home and shrinks
> the frame accordingly.

`char const *source = expression;` at function scope moves `success` to `[ebp+0xb]` (the top byte of the
home) and the frame from `sub esp,0x80c` to `sub esp,0x808`. **Triage consequence:** a target frame 4 bytes
*larger* than a candidate's is evidence that the target's parameter is still live, not evidence of a
missing local.

### Z2-REGION — the in-region calibration `[M-local]`, NOT a law
Angle A measured, on this one function, that only zero stores inside the **competing region** seed the
constant web. The region runs from function entry through the conditional arms of the block containing the
competitor and closes at the join after that if/else chain. Seventeen single-store placements: stores before
the copy, after the copy, in the loop body, inside the `!= '('` block, after the switch, in the success arm,
in the error arm and inside the nested newline block **all buy the pin**; a store at the top level of the
same block after the if/else join, anywhere after `hs_compile_dispose()`, in the recompile-pending tail, or
before the final `return` buys **nothing**. Our eight zero stores split 5 in-region / 3 out-of-region.

Within that region, and for a competitor whose web starts at function entry, six calibration rows fit
`in-region zero stores >= competitor references + 2` (refs 3 needs 5; refs 4 needs 6).

**Status: `[M-local]` only.** Angle B's corpus scan shows no count-based boundary is a law of VC7 at all.
Read this as *how far this particular function sits from its flip point* (exactly one unit), never as a
triage rule. Do not port it to another function without re-calibrating there.

### Z7 — the bistable-allocation reading (new, `[I]` from A's calibration rows)
In `_hs_compile_and_evaluate` the zero web and the compile-source phi contend for the **single** free
callee-saved register, and the loser goes to memory. January's outcome is phi-in-memory (parameter home,
3 references because the two argument pushes share one CSE'd load) + zero-in-EBX; ours is the inversion.
Every one of the four phi references is load-bearing — deleting any single one flips the pin (I1 608/595,
I2 576/568, I3 576/576, I4 576/574, all pin EBX) — and so is every unit of zero demand. The function sits
exactly **one reference, or one zero store, from the flip**. This is inference from calibration, not a
measured compiler rule; it is recorded because it explains why both escape routes are one unit short.

---

## 3. LAW S — two-output frame-slot assignment (new) `[M]`

> When a function passes the addresses of two same-sized locals as output parameters of one call, VC7 gives
> the **nearer** frame slot to the home whose **last memory reference occurs later** in the emitted code.

Inert (33 micro-TUs, 8/8 paired declaration-order TUs byte-identical): declaration order; which local is
`&`-passed in which argument position (it renames which local sits where, never the rule); the order in
which the two outputs are *tested*. **The only source-reachable lever is the printf/diagnostic argument
order** — in the un-cached form the last references are the two argument loads at the diagnostic call,
emitted in reverse argument order, so the value printed FIRST gets the nearer slot.

**Applied to hs**: January's last reference to each home is the *test* load (it then copies both into
ESI/EDI at 0x168/0x16a and never touches the frame again); ours is the *argument* load at `error()`
(0x180/0x183). Same rule, different last reference, hence the mirror.

**Consequence (this is the useful part):** a two-output slot mirror is **never an independent residual**.
It is downstream of one caching/rematerialisation decision. Closing the caching closes the slots for free,
and the one lever that does move slots reaches January only by inverting January's own print order — i.e.
it can never be spent.

**Detector** (manual, ~2 minutes): list the *last* `[ebp-N]` reference to each of the two homes on both
sides. If the sides differ only in which reference is last, do not report the frame census as a structural
gap.

**Corpus rarity** `[M]`: `slotmirror.py` scanned all 572 paired objects for "non-exact function that becomes
January's once one pair of `[ebp-N]` offsets is exchanged" — **one hit, and it is a false positive**
(`render_debug::_render_debug_player`, whose real defect is a 4-byte frame gap). **The "slot-mirror residual
family" that several ledgers assume is widespread does not exist at corpus scale.**

---

## 4. LAW X — the cross-jump tail merge (new) `[M]`

> VC7's cross-jumper merges the longest common instruction **suffix** of two switch arms, compared **after**
> register allocation. Arms merge only where the corresponding argument materialisations landed in the
> **same** register. The register is not chosen for the argument; it is inherited from the value.

Measured (7 micro-TUs, `scratch/hs_research/merge/`): with 0-3 unrelated live longs across the switch, both
arms reload their arguments into *different* registers (`eax/ecx` vs `edx/eax`) and merge only at `call`.
With a local `source = buffer;` live before the switch, `buffer` is pinned in ESI, both arms emit an
identical `push esi`, and the merged tail becomes `mov edi,[expanded]; push edi; call _sprintf` — January's
shape. Assigning the *parameter* instead does not do it in the reduced model.

**Applied to hs** `[M]`: January's arms both use `eax` for the `buffer` lea and merge at
`lea ecx,[ebp-0x80c]; push ecx; call _sprintf` (0x112); ours emits `lea edx/lea eax` in one arm and
`lea ecx/lea edx` in the other — a one-register rotation, because each arm needs two scratch temporaries
instead of one.

**COROLLARY — lab limit** `[M]`: the hs merge is a **whole-function register-pressure effect**. Neither
January's parameter reuse (`tail8/q1`) nor the EBX pin added on top (`tail8/z1..z12`) reproduces it in a
reduced TU; the pin makes the reduced model *worse* (the arms diverge into two separate call sites).
Micro-TU screening is valid for slot assignment and for scheduling, and **invalid for cross-jump merges in
a pressure-bound function**. Measure those on the whole `hs.c` at ~20 s per probe.

**Detector**: compare the two arms' argument-materialisation registers. Differ by a rotation -> the residual
is register pressure, not a missing statement. Identical -> the merge point is the first instruction after
the last differing operand.

---

## 5. Where the angles disagreed, and the adjudication

Three genuine conflicts. None is papered over.

### 5.1 "Is there a ninth zero store to find?" — A says the question is the wrong one; B says it is already answered
- **A**: the pin needs a sixth *in-region* zero store on the parameter-home shape; only five exist.
- **B**: `scratch/hs_pin/v1.obj` already pins EBX with the eight stores we write; there is no ninth store and
  none is needed.
- **Adjudication — both are right about different routes, and the route is the whole question** `[V]`:
  - *Route 1 (web split; `v1.c`, N-family)*: pin won with the existing eight stores, no extra text.
    Verified this pass: `lawz.py ... --obj scratch/hs_pin/v1.obj` -> `PIN ebx def@0x1a uses=26 -> 8 stores`,
    `>>> CONSISTENT`, MAXLIVE 2 vs 2, 185 insns vs January's 183. **Ceiling 585 meaningful (+8).**
  - *Route 2 (parameter home = January's shape, corroborated by Ghidra and by January's own
    `mov [ebp+8],edx` on the merged tail per Z5)*: the pin needs a sixth in-region zero store.
    **Ceiling 580 meaningful (+3)** and everything else exact.
  So B's "no ninth store is needed" is true of route 1 and false of route 2; A's "the ninth store cannot
  reach 577" is true and is the binding constraint on route 2.

### 5.2 "Is the cross-jumped sprintf tail a separate reconstruction task?"
- **B**, Shape 2: reproduce January's cross-jumped tail and the parameter-home phi — "an ordinary structural
  reconstruction question".
- **A**, LAW Z5: on a split-web shape the cross-jump is *mechanically unavailable*; it returns only when the
  phi is the parameter itself, which is the shape that loses the pin.
- **Adjudication: A is right and B's Shape 2 is not an independent task** `[V]`. `lab_R6.obj` (parameter
  home + a sixth zero store) has the cross-jump, the parameter home, the pin, 40/40 relocations and **183
  instructions against January's 183**, with no work done on the tail at all. The cross-jump is not
  something to reconstruct; it is a *free consequence* of keeping the phi in the parameter home. Pursuing it
  as a separate shape on top of `v1` would be spending on an effect Z5 says cannot appear there.

### 5.3 "Is there a count-based boundary at all?"
- **A**: `in-region zero stores >= competitor references + 2` (6 calibration rows + 17 positional rows).
- **B**: no count-based boundary is a law of VC7; `2U+1` is refuted in both directions and no correction
  term can exist.
- **Adjudication: no conflict once scoped, and B's scope wins for triage.** A's formula is a *within-shape
  calibration* of one function (A says so explicitly in its §5b scope warning); B's refutation is
  corpus-wide and uses strict-exact controls. **Rule: never classify a function on a store-count formula.
  Classify on Z1/Z1b (MAXLIVE + single definition), then use a local calibration only to measure distance
  from the flip point in a function you are already inside.**

### 5.4 One inherited claim that both angles narrowed
The ledger's "our compiler never invents a pin January lacks (0 of 240)" was **re-confirmed only on the 240
pre-selected cross-tab rows, not re-measured image-wide** `[M, scope-limited]`. Also: **128 of those 240
"pin" rows are `xor ebx,ebx` used only as a return value or compare operand, with zero stores**, and must
not be counted as evidence about the pin mechanism. The real-pin subset is **112 paired / 189 image-wide**.
Ledger prose quoting "236 / 227 exact" as 236 pin mechanisms is wrong.

---

## 6. `_hs_compile_and_evaluate` — disposition

**NOT LANDED.** Real file untouched; baseline re-verified this pass `[V]`:
`python -B tools/campaign/gate.py source/hs/hs --all --forbid-emitted-symbol _point_from_line3d` ->
`residual 592 _hs_compile_and_evaluate [size 608!=592, sha]`, `residual 96 _real_random_range_evaluate [sha]`,
`== exact 446 residual 2 unwritten 0`.

### 6.1 The two routes, measured

| route | candidate | padded | meaningful | relocs | pin | legitimacy |
| --- | --- | ---: | ---: | ---: | --- | --- |
| production | `source/hs/hs.c` (HEAD) | 608 | 601 | 40 | no | landed, complete |
| 1: web split, selection after switch | `scratch/hs_pin/v1.c` | **592** | **585** | 40 | **EBX** | clean; +8 = the lost arm cross-jump |
| 1: web split, assigned in case 0 + arms | N1..N6 / P7 | 608 | 599 | 40 | EBX | clean but +22; `default:` arm must also assign or VC7 emits an uninitialised read `mov eax,[ebp-8]` @0xf4 |
| 2: parameter home + a 6th zero store | `scratch/hs_research/lab_R6.obj` | **592** | **580** | **40/40** | **EBX** | **FORBIDDEN** — the store is not in January's bytes |
| 2: parameter home, no 6th store | `scratch/hs_pin/v2.c` | 608 | 601 | 40 | no | clean, but no pin |
| January | `build/split/source/hs/hs.obj` | 592 | **577** | 40 | EBX | — |

### 6.2 Exactly what remains on route 2 (re-measured this pass) `[V]`

`relocdiff lab_R6.obj --count-by-target` -> **TOTAL 40 target / 40 candidate**, every target matching.
`lawz.py --obj lab_R6.obj` -> January 183 insns / ours **183**, MAXLIVE 2 / 2, demand 8 vs **9**, verdict
`SOURCE DEFECT: extra zero statement`. `alndiff` -> the front end is **byte-identical through 0xd2**, and
the entire remainder is:

1. **the extra store itself**, `O 138 mov dword ptr [ebp-8], ebx` — **3 bytes, the whole size delta**;
2. **the `[ebp-8]` / `[ebp-0xc]` slot mirror** — by LAW S this is *not* a degree of freedom, it is
   downstream of item 3;
3. **the error-tail caching packet, net 0 bytes**: January `mov ecx,[ebp-0xc]; cmp ecx,ebx` … `mov esi,eax;
   mov edi,ecx` @0x168/0x16a, `nop` @0x17f, `push esi; push edi`, `pop edi` @0x196; ours
   `cmp dword [ebp-8],ebx` … reloads `mov edx,[ebp-0xc]; mov eax,[ebp-8]` after `_strchr` and pops EDI at
   0xd2. It is **one** decision: whether the web that held `type` in EDI is extended across `_strchr`.
   Refuted as the cause (angle C, 6 instruments + angle-3 `slotlab` + impl.md M7's 14): the aliasing store
   `*newline = 0`, spelling it `newline[0] = 0`, an early return, the ternary form, and **doubling** the
   post-call uses by calling `error()` twice. Our build never enregisters these two address-exposed outputs
   regardless of demand.

### 6.3 Why neither route can reach 577

- Route 1 is capped at +8 by LAW Z5: splitting the web is what wins the pin, and the split is what destroys
  the arm cross-jump. Selection spellings on that base are all 585 (impl.md M8); +8 is the floor of the
  family.
- Route 2 is capped at +3 by cost: with the pin already won, the cheapest zero store VC7 emits is 3 bytes
  (`mov [mem],bl` / `mov [mem],ebx`), so any *purely additive* ninth store lands at >= 580.
- And January has no ninth store to copy. **Two independent censuses agree** `[M]+[V]`: angle A's `prof.py`
  (January, `v1` and `N1` pin the same eight source statements: `success=FALSE`, `buffer[1023]=0`,
  `buffer[0]=0`, `*space=0`, `*newline=0`, `hs_syntax_data_allocated`, `hs_syntax_data`,
  `hs_recompile_pending`) and angle B's carrier-agnostic `lawz.py` count (January total zero-store demand
  **8**, any carrier). HCEA's zero-initialised error outputs (`const char *error_text = 0; char
  *error_message = 0;`) are the only credible sixth candidate and January's bytes contain **no** store to
  `[ebp-8]` or `[ebp-0xc]` before the compile call — so R5/R6 are instruments, not candidates.
- `config/config.json` has no per-file cflags for `hs.c` (global `/O2 /Oy-`), so no flag explanation exists.

### 6.4 Reopen criterion (replaces the ones in the 2026-09-16 ledger and in `impl.md`)

Reopen **only** on one of:
- **(a)** January-side evidence of a **sixth in-region zero-valued statement** — it must lie at or before the
  conditional arms of the compile block (Z2-REGION) — **that is net-zero-byte**, i.e. one that replaces
  existing emitted text rather than adding a store. A purely additive statement lands at 580 and cannot
  close. (Note the carrier law: such a statement need not be lowered through EBX in January, but a
  carrier-agnostic census already shows January has only 8 store sites, so this is a high bar.)
- **(b)** a measured VC7 law under which the **two call-site reads of a memory variable count as one
  reference** in the allocator's ranking, i.e. a legitimate source form whose argument list names the
  pointer once. (`hs_compile_expression(csstrlen(expression), expression, ...)` names it twice; Q1/Q2 prove
  no copy spelling helps — Z4.)
- **(c)** a measured law for VC7's callee-saved **caching decision across a call** (the error-tail packet).
  Required *in addition to* (a) or (b) for strict exactness, though it is net-zero-byte on its own.

**Carry forward unchanged**: `scratch/hs_pin/v1.c` (592 padded / 585 meaningful / 40 relocs, `[sha]` only,
byte-identical to January through 0x103, unit still 446 exact, guard passes). It carries **zero credit** and
under lane fuzzy policy must not replace a complete body, so it stays in scratch.

### 6.5 hs.obj's other residual — `_real_random_range_evaluate` (source/hs/hs.c:6370)

**Verdict: genuine tie (class E), CLOSED to further spending.** 96 padded / 4 relocs both sides; identical
relocation multiset, identical 8-byte frame, identical 35-instruction multiset. `alndiff` is one insert +
one delete of the *same* instruction: ours hoists `mov ecx,edx` into the x87 `fstp`->reload gap (ours 0x27,
January 0x2b). 46 source spellings across four families and 14 flag settings measured inert; `/G5` moves the
copy to a *third* order that is not January's and would break the object's 446 exact rows.
**Reopen only on** a measured VC7 list-scheduler priority law explaining why a register copy feeding the
*second* argument push is ready at the `fstp` in our build and not in January's.

**Free house-rule cleanup found here** (measured **byte-identical**, zero risk): the current source
hand-expands `real_seed_random_range(get_global_random_seed_address(), lower_bound, upper_bound)`; compiling
`real_random_range(lower_bound, upper_bound)` instead produces the same bytes. The hand-expansion is a
prohibited "hand-expanded helper body" and can be removed for free.
Also measured: the object's house idiom (`struct hs_arguments_real_real const *` + `double value1 =
arguments->value1;` + `arguments->value0` passed directly) collapses to 80 bytes, 7 short of January — the
current function-scope-`real` shape is the right family, and probe `e1` isolates that a function-scope
`upper_bound` alone is sufficient while a function-scope `lower_bound` alone is not.

---

## 7. Negative register — do not re-spend on any of this

**Pin / LAW Z**
- `stores >= 2*U + 1`, and any search for a correction term for it (`_gz_open` / `_action_obey_describe_command`).
- "Add a zero statement to win a pin" as a general strategy: 768 January functions have demand + a free
  callee-saved register + MAXLIVE <= 2 and still do not pin.
- Buying a pin with a redundant `= 0`, a carrier variable, `volatile`, or an operand permutation (fake match
  by construction).
- Treating `bink_playback::_bink_alloc@4`, `transport_endpoint_winsock::_endpoint_readable` or
  `transport_endpoint_set_winsock::_transport_initialize` as register-allocation questions: the first two are
  incomplete bodies (137 vs 67 and 61 vs 72 instructions) and the third is missing four zero statements.
- Reading the 236/227 cross-tab as 236 pin mechanisms (128 are zero-store "pins").

**`_hs_compile_and_evaluate`**
- Any copy spelling of the compile-source pointer, declaration scope or order, `expanded` scoping, length
  staging, statement order, or the `case 0:` assignment family (Z4/Z5; 18 + 8 rows, ten bit-identical).
- The loop respellings, "seeded flag" placements and 20+ spellings from the prior wave.
- The error tail as two packets, or as declaration order / argument position / consumption order / newline
  spelling / the aliasing store: 30 spellings measured inert and LAW S collapses it to one decision.
- The frame census of the error tail as a structural gap — the multiset is identical.
- The loop-alignment pad as a separate defect: the decision reads only the head address, and every byte of
  our 3-byte-longer pre-loop region is a consequence of the missing pin. (Refinement worth keeping: only ONE
  pre-loop byte is needed to reach distance 9, so the first zero-register site recovered is worth **9 bytes,
  not 1**.)
- Modelling the switch-arm tail merge in a reduced TU (`tail8/q1`, `z1..z12`).

**Other**
- `_real_random_range_evaluate` spellings and flags (46 + 14).
- Assuming a corpus-wide "slot-mirror residual family": the scan of 572 paired objects found **zero** real
  instances outside hs.
- Attributing the x87 store->reload gap difference to compiler or flags: corpus histograms are
  statistically identical (January `{0:127,1:17,2:9,3:5,4:2}` vs ours `{0:122,1:20,2:9,3:4,4:2}`).

---

## 8. What this supersedes in `docs/object_matching_logs/opus5_next150_compiler_laws_20260916.md`

| clause in the 2026-09-16 ledger | status |
| --- | --- |
| Z2's boundary `stores >= 2*uses + 1` and its U-table | **DELETE** — refuted both directions (§2, Z2) |
| "Extra basic blocks raise the threshold" | **DELETE** — moot; the block signal is a selection effect |
| Z1 as "no January function carries three callee-saved webs and a pin", measured on 236 | **REPLACE** with the 343/343 MAXLIVE table + Z1b single-definition corollary |
| "178 of 214 parsed pins are EBX" | **REPLACE** with EBX 259 / EDI 51 / ESI 33 of 343 |
| "236 both-pin / 227 exact" read as pin mechanisms | **QUALIFY** — 128 of 240 rows are zero-store pins; real-pin subset 112 paired / 189 image-wide |
| "Parameter reuse ... loses the pin (608, no pin)" as the whole story | **QUALIFY** — true, and route 2 (parameter reuse + one more in-region zero store) is the only route that reaches 580 with 40/40 relocations and January's instruction count |
| "`source` assigned in every switch arm is strictly worse (608 / 41 relocations)" | **CORRECT** — the N-family is 608 / **40** relocations / 599 meaningful and **wins the pin**; it is worse on size, not on the pin, and its prologue is byte-identical to January through 0xd1 |
| hs reopen criterion ("a ninth zero-valued statement" or "remove one competitor use") | **REPLACE** with §6.4 (a)/(b)/(c) |
| The LOOP-HEAD ALIGNMENT LAW | **UNCHANGED** — nothing in this round touches it |
| Z3 | **UNCHANGED**, plus the span-vs-count regime warning (§2, Z3) |

---

## 9. Detector inventory (all read-only, all under `scratch/hs_research/`)

| tool | what it does | when to run it |
| --- | --- | --- |
| **`lawz.py`** | the deliverable. Per side: instruction/call/block counts, MAXLIVE, pin register + use kinds, the complete constant-zero store list **with each store's carrier**, total zero references, and every callee-saved web with defining instruction, use count, span, call-crossing and interference flags. Then one classification line and the named instructions to remove. Modes: `--webs`, `--obj <cand.obj>` (screen a candidate before landing), `--corpus [--json]`, `--selftest`. **0 false positives on 227 strict-exact controls.** | after tinfo + frame census, **before** proposing any shape, on any `[sha]`/`[size]` residual where either side contains `xor ebx,ebx`. A `CONSISTENT` answer removes the pin from the hypothesis list in one command |
| `scanall.py` / `scanall.json` | whole-image scan (10,568 January functions) building the DEMAND-NEGATIVE set — the class the cross-tab structurally cannot contain | when someone proposes a count-based codegen law; this is what refutes sufficiency |
| `explore.py` / `explore.json` | register-pressure exploration (MAXLIVE, call-crossing webs) that located the real Z1 quantity | corpus work on register pressure |
| `fndis.py` | relocation-annotated per-function disassembly from any COFF object (January, ours, or a gated candidate) | building line-for-line reference tables |
| `lab.py` | splice a whole replacement body for one function into the `hs.c` TU, gate it, report padded/meaningful size, instruction count, sha16, and which callee-saved register carries a whole-function zero pin | whole-TU shape screening (~20 s/probe) |
| `rlab.py` | micro-TU lab: extracts `hs.c`'s exact cflags from `build.ninja`, compiles a `.c`, dumps one symbol; `--batch <dir> <symbol>` with same-as grouping (~0.15 s/probe) | slot assignment and scheduling questions **only** — invalid for cross-jump merges (LAW X corollary) |
| `prof.py` | enumerate a candidate's zero web: every def, store site, push count | proving two candidates pin the same source statements |
| `batch6.py` / `batch7.py` | positional detector: one artificial zero store at each of 17 source positions, reporting which buy the pin | mapping a competing region's boundary in a function you are already inside |
| `slotmirror.py` / `slotmirror.json` | corpus slot-mirror detector over 572 paired objects (result: 1 hit, a false positive) | do **not** re-run; the family does not exist |
| `gapscan.py` / `gapdetail.py` / `gapscan.json` | corpus x87 `fstp [ebp-N]` -> `mov r32,[ebp-N]` gap histogram, per function, January vs ours; flags the 13 differing profiles | ranking x87-heavy residuals. Symptom-level, not causal |

**Verification commands run by this consolidation pass** (`[V]` tags above):
`gate.py source/hs/hs --all --forbid-emitted-symbol _point_from_line3d`;
`lawz.py source/hs/hs _hs_compile_and_evaluate --obj scratch/hs_pin/v1.obj`;
`lawz.py source/hs/hs _hs_compile_and_evaluate --obj scratch/hs_research/lab_R6.obj`;
`relocdiff source/hs/hs _hs_compile_and_evaluate scratch/hs_research/lab_R6.obj --count-by-target --allow-structural`;
`alndiff source/hs/hs _hs_compile_and_evaluate --ours-object scratch/hs_research/lab_R6.obj`;
`gate.py source/game/game_engine_king --all`;
`relocdiff source/game/game_engine_king _king_calculate_hill_state build/base/... --count-by-target --allow-structural`.

---

## 10. Follow-on targets these findings make attackable, in priority order

**1. `game_engine_king::_king_calculate_hill_state` — HIGHEST VALUE, two independent censuses agree.**
448 target. Gate row re-measured this pass `[V]`: `residual 448 _king_calculate_hill_state [relocs 30!=32, sha]`
— note this is a **relocation** gap, not a pure sha tie, which the ledger did not record.
`relocdiff --count-by-target` `[V]`: `_king_globals+404` is referenced **9 times by January and 7 by us**;
every other target matches. `lawz.py`: ours is the corpus's **only** MAXLIVE breach (3 vs January's 2) —
we `xor ebx,ebx` (def 0x9, span 6..57) and then **reuse EBX for a cached global** `mov ebx,dword ptr [0x194]`
(def 0xbf, 8 uses, 4 of them stores, span 62..125); our zero demand is also short (1 store vs January's 6).
**The register census and the relocation census name the same defect from opposite directions**: our source
caches the `_king_globals+404` field in a local where January re-reads the global at each use. Removing the
cached local predicts **+2 relocations at exactly that target**, restores MAXLIVE 2, frees EBX and returns
the pin — same size (448/448), so this is a single-shape, high-confidence structural fix.
File is neither protected nor reserved.

**2. `saved games::_player_profile_new` — named single instruction.**
`SOURCE DEFECT: extra zero statement`: 11 zero stores against January's 10, surplus at `0x145`
(`mov byte ptr [ebp-1], bl ; succeeded = FALSE`), independently named by
`docs/object_matching_logs/player_profile_obj_opus5_next150_n5_20260915.md`. Secondary signal in the same
region: January's frame clear is `lea edi,[ebp-0x203]`, ours `lea edi,[ebp-0x206]`, and our `[ebp-8]` reload
replaces January's `[ebp-4]` — a local-layout difference worth a frame census before spending a shape.

**3. `render_debug::_render_debug_player` (source/render/render_debug.c:1392) — cheap and concrete.**
82 instructions, only 4 differ, and the cause is a **frame-size gap**: January `sub esp,0x404` vs ours
`sub esp,0x400` — January homes one extra dword at `[ebp-4]` that our `char text[1024]` build does not.
Frame-census defect (an extra declared local, or an elided aggregate member), **not a tie**. Plus a separate
**relocation-identity naming gap** on the empty-string literal `??_C@_00CNPNBAHC@?$AA@` (January `symbol:`,
ours `defined-noncode:.rdata:`) — that half is an ORCHESTRATOR `symbols.json` item, not a source edit.

**4. `bink_playback::_bink_alloc@4` — reconstruct, do not triage the pin.**
137 instructions against January's 67; the ESI/EBX pin disagreement is a *consequence*. Four intruder webs
on ESI. `docs/object_matching_logs/bink_playback_obj_opus5_next150_n5_20260915.md` already attributes it to
two source inline-boundary defects and reports a candidate back at 67 instructions with the pin restored —
check that ledger's state before spending anything.

**5. `hs::_real_random_range_evaluate` — free house-rule cleanup, zero byte risk.**
Replace the hand-expanded `real_seed_random_range(get_global_random_seed_address(), lower_bound,
upper_bound)` with `real_random_range(lower_bound, upper_bound)`: measured **byte-identical**, and it removes
a prohibited hand-expanded helper body. The byte residual itself stays closed (§6.5).

**6. The x87 gap-profile shortlist (symptom-level ranking only, from `gapscan.json`).**
Ours fills a gap January leaves empty: `_actor_start_burst`, `_path_state_traverse`,
`_render_camera_build_frustum`, `_biped_update_physics` (2 sites) (+ `_real_random_range_evaluate`, closed).
Reverse direction: `_effect_generate_particles`, `glow::_get_particle_world_position`. Only 13 of thousands
of paired functions have a differing gap profile and all are already non-exact — use it to rank x87-heavy
residuals, never as a cause.

**7. Evidence-only (PROTECTED — never edit, recorded so no future wave re-triages them).**
`transport_endpoint_set_winsock::_transport_initialize` = missing zero statements (demand 8 vs 4, zero
references 31 vs 22, MAXLIVE equal). `transport_endpoint_winsock::_endpoint_readable` = incomplete body
(61 vs 72 instructions), **not** a pin problem.

**8. Corpus-wide reuse of the detector.** Nine both-pin non-exact functions were classified; seven are
`CONSISTENT` (`libcmt::__getstream`, `actor_looking::_actor_look_valid_look_vector`,
`actor_perception::_actor_perception_aiming_vector_test_blockage`, `ui_widget::_render_ui_widgets`,
`ui_widget::_render_ui_widgets_postgame`, `virtual_keyboard::_virtual_keyboard_select`,
`main::_main_frame_rate_debug`) — for all of these **the pin is not the residual**; go straight to `alndiff`
and do not open a register hypothesis. (`_render_ui_widgets` and `_render_ui_widgets_postgame` remain on the
brief's do-not-spend list absent new January-side evidence.)
