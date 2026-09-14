# actor_stimulus.obj — opus5 150K lane, wave w2 (2026-09-14)

## Baseline and final

| gate | result |
| --- | --- |
| baseline real file | `== exact 21 residual 1 unwritten 0`, guard passes |
| final real file (unchanged) | `== exact 21 residual 1 unwritten 0`, guard passes, park drift 0, fake scan 0 leads |

`git diff --stat -- source/ai/actor_stimulus.c` is empty. No credit is claimed.

## `_actor_stimulus_prop_acknowledged` (parked, register-allocation)

Target 800 / 32 / `b66ac2399346e00a`, 243 instructions, `sub esp,0x38`. Candidate 800 / 32 / `0f62d6950730557f`, 244 instructions, `sub esp,0x34`.

### January facts recovered in this wave

1. **Frame homes.** January homes six locals:
   - definition at -4;
   - the `'<'`/`'>'` character at -8;
   - close/far at -0xc;
   - shoot at -0x10;
   - `combat_status` at -0x14 (dword store after `xor eax,eax; mov ax,[ebx+0x6e]`);
   - actor at -0x18 (spilled right after datum_get).

   Ours homes `surprise_level` instead and forwards `combat_status` to the field.
2. **`combat_status` is signed `short`.** January compares with `cmp ax,5; jl`. A `word` local gives `jb` (measured, tA) and a `long` local gives movsx and 32-bit tests (tB).
3. **The printf `%d` argument is `DATUM_INDEX_TO_ABSOLUTE_INDEX(actor_index)`.** January emits `mov ecx,[ebp+8] ... and ecx,0xffff`, not `movzx ecx, word ptr [ebp+8]`. The same idiom appears in encounters.c; tE reproduces the January schedule exactly.
4. **The combat ternary is `!combat_status ? "noncom" : "combat"`.** January relocations 22/23 load "noncom" first with `je`. The other ternaries in the same call keep source polarity.

### Shapes (5, budget reached)

| shape | result |
| --- | --- |
| tA `word combat_status` | 800/32 `a6efbdc3`, `jb` (refutes word) |
| tB `long combat_status` | 800/32 `6a0fd368` (refutes long) |
| tE `DATUM_INDEX_TO_ABSOLUTE_INDEX(actor_index)` | 816/32 `bf2c5a6b`; the printf index schedule now matches |
| tH tE + `!combat_status ? "noncom" : "combat"` | 816/32 `cb3b25a1`; ternary block, relocation order and dot-product operand order now match |
| tG tH + `combat_status` initialised before `flanked` | 816/32 `4f936a3a` (worse, early load) |

Best-evidence scratch: `scratch/workers/model_animations_actor_stimulus_etc/as_tH.c` (patch `as_tH.patch`). It is not exact, so nothing is proposed for landing. Every scratch gate kept the 21 siblings exact and passed the guard.

### Residual

The actor EBX+home / surprise EDI allocation versus ours, plus the `combat_status` home. In tH the extra 4 bytes cross the pad boundary.

### Reopen criterion

A natural source fact that makes the short `combat_status` local non-forwardable (January homes it), combined with tE and tH. Keep tE and tH in any future attempt; do not repeat tA, tB or tG.
