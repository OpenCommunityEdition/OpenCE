# `actor_stimulus.obj` — opus5 next-150K lane, final implementation wave, Tier B (B5) (2026-09-16)

HEAD `6d6529d98`. Scope: `_actor_stimulus_prop_acknowledged` only (parked, `register-allocation`).

| gate | result |
| --- | --- |
| baseline real file | `== exact 21 residual 1 unwritten 0`, `_point_from_line3d` guard passes |
| final real file (unchanged) | `== exact 21 residual 1 unwritten 0`, guard passes, park drift 0, fake scan 0 leads |

`git diff --stat -- source/ai/actor_stimulus.c` is empty. No credit is claimed. 0 new landing shapes spent.
Worker notes: `scratch/workers/tB_actor_stimulus.md`; artefacts in `scratch/workers/tB_actor_stimulus/`.

## `_actor_stimulus_prop_acknowledged`: SKIPPED-EXHAUSTED (Tier B lenses do not hold as a source defect)

Target 800 / 32 / `b66ac2399346e003`, 243 insns, `sub esp,0x38`. Real file 800 / 32 / `0f62d6950730557f`, `sub esp,0x34`.

### Census

- **Relocations:** 32 / 32 with an identical multiset by symbolic target. There is no missing or extra call or global.
  At HEAD the only order difference is the combat ternary (w2 tH fixes it).
- **Registers:**
  - January: ESI=prop, EBX=actor (spilled at its definition), EDI=prop_index, then surprise_level.
  - Ours: ESI=prop, EDI=actor (never spilled), EBX=prop_index, then surprise_level (spilled at every definition).
- **Frame:**
  - January cells: definition -4, `'<'/'>'` -8, close/far -0xc, shoot -0x10, **combat_status spill -0x14**,
    **actor spill -0x18**, direction -0x28 (16 B), surprise_names -0x38 (32 B).
  - Ours: **surprise_level spill -4**, then the same declared objects, each one cell nearer.
  - Every declared local exists on both sides. Only spill cells differ.
- `scratch/research/frame-slot-ranking/frameslot.py` gives `SPILL-MISSING / REGISTER-FAMILY` on the real file and on
  the tH base.
- **Mechanism, from the bytes.** In both builds the printf push sequence borrows EBX as its fourth scratch register:
  - The web that owns EBX is split-spilled.
  - Ours keeps actor alive in EDI through that block, so VC7 rematerialises `combat_status` from `[edi+0x6e]`.
  - January's actor register is clobbered there, so combat_status gets a home.
  - The +4 frame and the +4 code bytes of the tH base are therefore consequences of the EBX/EDI choice.

### Lens re-verification

1. **Frame lens.** The byte fact holds: January stores at +0xBF and reads `cmp word ptr [ebp-0x14],0` at +0x21E. The
   inference that January has an extra declared local does not hold, because the F3 detector classifies the gap
   REGISTER-FAMILY.
2. **M8 lens.**
   - **At HEAD.** Re-run confirmed: D=1..7 fix the dot (106 -> 105 differing instructions).
   - **Where the odd ordinal comes from.** Our `(word)actor_index` cast adds it.
     - Diagnostic d1 (HEAD plus only w2 tE, `DATUM_INDEX_TO_ABSOLUTE_INDEX(actor_index)`) has January's dot at D=0.
       January proves tE with `mov ecx,[ebp+8] ... and ecx,0xffff`.
     - Diagnostic d2 (HEAD plus only the tH ternary polarity) does not.
   - **On the January-proven tE+tH base.**
     - The base reproduces w2 exactly: 816/32 `cb3b25a1`.
     - `mod8.py` gives **NOT-MOD8**, and **D=1 re-breaks the dot**.
     - M8 therefore refutes adding one genuine local.

Neither lens supports the implied change ("add January's genuine extra local"). Per the owner's requirements 1 and 3
the row was stopped without spending shapes.

### Residual on the best-evidence base (tH)

Every differing instruction is one of:
- an EBX<->EDI operand;
- a frame offset shifted by one cell;
- a spill store (ours 4x `mov [ebp-4],ebx`; January `xor eax,eax`, `mov [ebp-0x14],eax`, `mov [ebp-0x18],ebx`);
- `cmp word [ebp-0x14],0` vs `cmp word [edi+0x6e],0`;
- a relocation displacement.

There is no control-flow, call, width, literal or x87 difference. The +4 code bytes push the padded size to 816.
The 21 siblings stay exact on tH, the guard passes, and parkcheck drifts only the park.

Classification: REGISTER-ALLOCATION. Research section 2.3 says the REGISTER-FAMILY class has no declaration lever by
construction.

### Evidence consulted

- January COFF alndiff, tinfo, relocdiff, frameslot and mod8 (all re-run at HEAD).
- HCEX.pdb `DIA2Dump -sym`:
  - params: `actor_index`, `prop_index`, `reappearance`, `expected`;
  - the only non-enregistered local is `struct direction_specification direction`;
  - there is no scalar local list, so it gives no evidence of an extra owner.
- HCEA `blam/ai/actor_stimulus_prop_acknowledged.c`:
  - a retail lift with no debug block;
  - its float `aim_dot`, re-tested at each site, contradicts January's single fcomp into DL, so `boolean flanked` stays.

### Do not repeat

- Adding one local to the tE+tH base: refuted by M8, since D=1 breaks the dot.
- Treating the combat_status home as a missing declaration: frameslot says REGISTER-FAMILY.
- Everything w2 and n2 already record: tA `word`, tB `long`, tG early load, and a local read in the late status
  test (January reads the field).

### Correction for the research findings table

B5 is not a named structural defect:
- Lens 1 is a register-family spill cell.
- Lens 2's odd ordinal is our `(word)actor_index` cast temp, which the January-proven tE spelling removes.

Recommend reclassifying B5 to the register-allocation pool. Record tE+tH as the required fidelity base for any
future reopen.

### Reopen criterion

- The park criterion is unchanged: authoritative January local records, or a natural same-compiler donor that
  explains the EBX/EDI lifetime choice.
- Start from `scratch/workers/tB_actor_stimulus/tH.c` (tE + tH).
- Respect the M8 constraint: January's IL count != tH+1 (mod 8).
- tE+tH alone are +16 padded, so they can land only together with the allocation fix.
