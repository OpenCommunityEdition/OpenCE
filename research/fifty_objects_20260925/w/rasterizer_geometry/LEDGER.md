# rasterizer_geometry worker ledger (lane claude/fifty-objects-20260925)

Target: _uncompress_int32_to_real_vector3d (144 padded, park instruction-scheduling).
Prior negatives (do NOT repeat): member-wise stores; `1.0f + x*s`; scale-first product;
fraction-first product; `>>=` folded into j / j+k; definition position (January order 3rd);
long-hand assignment; hoisting shift ahead of j; division form of k scale; independent field
extraction (128 B); scalar reals (112 B). Current production: by-value real_vector3d return (b6 70e1a15b).

## Probes

### P0 baseline (2026-09-23)
gate --all on production: 18 EXACT, 1 residual [sha] _uncompress_int32_to_real_vector3d.
alndiff (base obj): 49/49 insns; only diff = January `fadd __real@3f800000` @0x28 BEFORE
`shl eax,0x16` @0x2e; ours shl @0x28 then fadd @0x2b. Everything else equal (by-value return
already landed, frame 0xc = local v).

### E1 evidence: HCEX + /Od readout
- HCEX DIA2Dump: single Param `unsigned long i` (param named `i`), by-value return; no locals listed
  (HCEX lists params only for this family, e.g. get_normal also params only).
- /Od build fn 0x82ea80 (callee of compress_real_vector3d_to_int32 @0x82d080, found via
  "fabs(v2.i - v->i)<0.01f"): saved at od_uncompress.txt. Shape:
    v.i = ((real)(long)((i & 0x7ff) << 21) / 1048576.0f + 1.0f) / 2047.0f;   (and+shl+cvt+div+add+div)
    i >>= 11;  (shr, store back to param)
    v.j = same with & 0x7ff << 21, /1048576, +1, /2047
    i >>= 11;
    v.k = ((real)(long)(i << 22) / 2097152.0f + 1.0f) / 1023.0f;   (no mask on k)
    return v;   (RTC aggregate [ebp-0x14] size 12 = v)
  Constants read: 1048576.0, 2047.0, 1.0, 2097152.0, 1023.0 -> DIVISIONS, and an `& 0x7ff` mask
  on i and j fields. Neither the full division form nor the mask was among recorded negatives
  (only "division form of the k scale" alone).

### P1 cand1.c: full /Od shape (`(x&0x7ff)<<21` masks on i/j + divisions /1048576,/2047,/2097152,/1023)
Hypothesis: /Od-attested masks/divisions change IL node shape and hence the DAG emission slot.
Result: residual [sha], IDENTICAL to baseline (fadd/shl transposition unchanged). Masks fold,
divisions fold to reciprocal multiplies with identical constants. INERT (new negative).
Harness: probe.py (replaces function body in prod_copy.c; probes/ dir).

### P2 ORACLE (lab only, never landable): declaration-count sensitivity
- N = 1..17,20,24,31,32,33 dummy `extern long` declarations immediately before the function: ALL
  residual, identical transposition. Pre-function declaration count is NOT the key.
- 1..8 unused `long` locals after `v`, 1..3 unused `real` locals before `v`: ALL identical residual.
  Unused locals do not move it (consistent with M8 needing IL-live locals).
=> the tie is decided by the function's own IL, not TU context.

### P3 flags oracle (lab): /G5 /G6 /G7 /GB /Op /Ot /Os /Oi /QIfist /arch:SSE -> all identical transposition. Not a flag.
### P4 double-intermediate type lever (units tgr/cpmd lever): `+ 1.0` double, `(double)` conversion,
double scale constants, double `/2047.0` division -> all emit qword constants (relocation targets
__real@3ff0.., __real@3f4002.. etc; January has only dword __real@ targets) AND the fadd stays after
the shl. Double intermediates refuted by January relocations and inert on the slot.

### P5 cast variants on the conversions (all residual, identical transposition):
k1 `(real)((long)compressed<<22)`; k3 `(int)` k; k5 `(long)compressed<<N` on all three; k6 `(int)` all;
k7 `/1023.0f` k; k8 `(real)(...)` wrapped k. INERT.
### P6 LAB (not landable; mechanism study): chain-length sensitivity
labA k chain shortened to bare conversion; labB/C/F extra constant multiply (folds away);
labE j chain bare; labH/I/J i chain lengthened by 2-4 non-foldable x87 ops.
In EVERY case our compiler emits `shr eax,11; shl eax,22` back-to-back before the i fadd.
=> the slot is NOT decided by FP chain height; the integer k shift is scheduled ASAP after the
second shr regardless of FP chain shape. January's extra delay of `shl` by one slot suggests an
extra (code-invisible) node in the int chain between the 2nd shr and the k shl, or a different
value feeding the shl (hypothesis H-copy).

### P7 k mask `(x&0x3ff)<<22` (m1/m2 incl. full /Od masks+divisions): INERT.
### P8 copy/temp variable shapes (c1 working copy `bits`; c2 non-destructive field longs -> `and` insns,
different code, reject; c3 three long field locals computed first then FP (identical to base: statement
order of independent int work is irrelevant); c4 j_bits/k_bits chain -> register swap + same slot). INERT.

### P9 ORACLE live copy chains b0..b7 of compressed (1..8 live locals): listing byte-identical to base (copies propagate away). IL-local ordinal count is not the key.
### P10 LAB mechanism finding (labptr.c, not landable):
- `_lab_ptr_direct` (pointer param, direct `result->i=` stores, 112B form) emits JANUARY's local order
  `shr ecx,0xb; fadd; shl ecx,0x16` because an extra early int op (`mov eax,[ebp+8]` for fstp [eax])
  occupies one filler slot after fild, shifting the int stream by one slot.
- `_lab_ptr_local` (*result = v copy) and `_lab_escape` (v through a pointer alias) = ours.
=> model: after each x87 op the emitter places up to ~3 ready int ops; January's shl sits one slot
later as if ONE extra (code-invisible) int node occupied a slot in the fmul(i) filler group.

### P11 LAB types: plain struct return, v.n[] array access, member-wise copy-out through pointer: all = ours. Alias/type not the key.
### P12 LAB x1/x2 (extra int op `add eax,5` / `xor` in the k chain before shl): emits
`mov [c],edx; shr; add; fadd; shl; fmul` = January's pattern with the add visible.
=> STRONG mechanism: January's k chain has ONE extra int node between the 2nd shr and the k shl
that emits no bytes (coalesced copy / no-op conversion) -- the filler group after fmul(i) is 3 nodes.
x3 (extra op in the j chain) does not move the k shl.

### P13 LAB IL-normalized int spellings (all identical to base): e1 `(compressed>>11)<<22` in k (-> and+shl, different code);
e2 `k_bits = compressed>>11`; e3 `long k = compressed`; e4 `bits = compressed>>11` chain; g1 `(c<<11)<<11`;
g2/g3 `c*4194304` / `c*(1<<22)`; g4 j as multiply. Copies/multiplies normalize before the emitter.
### P14 LAB invisible-FP-node spellings: f1/f2 `(real)(double)(long)` conversions, f3 double product narrowed
(fmul qword, reject), f4 redundant (real) cast: all keep ours order.

### P15 LAB filler model (x4/x5): exactly 3 int fillers are placed between dependent x87 ops
(fild->fmul, fmul->fadd, fadd->fmul); ints are otherwise ASAP. `mov eax,[ebp+8]` waits for eax to be
free (post-RA register dependences). So January's fmul(i) group = {mov [c],edx; shr; X} with X
invisible (no bytes) and X preferred over shl. h1 `v = {0}` emits visible zero stores (reject).
Candidate X: an independent x87 constant load (fld [1.0]) later folded by fppeeps into `fadd m32`.

### P16 *** t4: EXACT *** (probes/t4.txt, full TU probes/t4.c -> cand_t4.c)
Shape: one reused `real t;` holding the int->real CONVERSION of each field:
    t = (real)(long)(compressed<<21);
    v.i = (t * (1.0f/1048576.0f) + 1.0f) * (1.0f/2047.0f);
    compressed >>= 11; (same for j) ; compressed >>= 11; t = (real)(long)(compressed<<22); v.k = ...
gate: _uncompress_int32_to_real_vector3d EXACT 144. (t1 temp = scaled product, t2 temp = biased
sum, t3 double temp: all ours order; only the CONVERSION temp moves the fadd one slot.)
Mechanism consistent with the units FP-intermediate lever: the named real conversion is the
code-invisible x87 node that occupies the 3rd filler slot.

### P17 variants of the lever (all EXACT 144): t5 three temps `real i,j,k`; t6 staged in-place
`v.i = (real)(long)(..); v.i = (v.i*s+1)*c`; t7 `real value` + /Od masks & divisions; t8 `real value=`
initialised at declaration then reused; t9 temp only for i and j (k inline). => the lever is the i
component's int->real conversion being its own statement stored to a real (code-invisible node).
Choosing t4 shape with descriptive name `value` (minimal diff; production arithmetic unchanged).

## FINAL candidate: cand.c (= probes/final.c; t4 shape, temp named `value`)
- gate --all: 19/19 EXACT, 0 residual, 0 unwritten (gate_cand.txt).
- object_audit.py: 44 January symbols 0 differ, every January-owned section ok -> OBJECT AUDIT: PASS
  (object_audit.txt). Candidate-only surplus identical to the production base (literal COMDATs +
  pre-existing owner-admitted `_fast_ftol`, 32 B).
- surplus_identity.py: `_fast_ftol` IDENTICAL to January's selected copy (source\ai\actor_combat.obj).
- provider_link.py cand.obj: every surplus symbol PASS in both link orders; SELECTED-PROVIDER LINK: PASS.
- pdb_storage.py: 44 split symbols, 0 disagreements with cachebeta publics.
- build/report.json (current): data 864/864 (100%); code 3919/4054 -> this closes the missing 135 B.
- object_admission_rejections.json: no entry for the unit. semantic_*: none.
- fake_match_scan.py: 0 leads (cand and base). CL /Zs /W3: 12 warnings each (all shared headers), no new.
- production.patch (CRLF-preserving unified diff) verified: `git apply --check` exit 0 and
  `patch --binary -p1` reproduces cand.c byte-for-byte.
- Park to retire: config/parked.json entry unit source/rasterizer/rasterizer_geometry,
  function _uncompress_int32_to_real_vector3d, class instruction-scheduling.
- Disclosure: the later /Od revision (0x82ea80) has NO scalar local (frame 0x18 = v + RTC guards +
  GS cookie) and spells the arithmetic with `&0x7ff` masks and divisions; January's bytes need the
  conversion staged as its own statement. Every natural staging (reused temp, three temps, in-place
  v.i staging, declaration-initialised temp, temp + /Od masks/divisions) is EXACT, while every
  un-staged spelling (15+ recorded) is not -> the separate conversion statement is a real January
  source-structure fact, consistent with the file's own staged-real style (compress_real_to_int16's
  `z = (real)floor(z*...)`). Not a filler/dummy: `value` is assigned and read three times.
  Alternative landing if the owner prefers /Od arithmetic: probes/t7.txt (EXACT).
