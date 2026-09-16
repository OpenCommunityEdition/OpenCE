# `interface.obj` opus5 next-150K lane, wave n1 (2026-09-15)

Worker `n1:hud_draw_interface_etc`. Unit `source/interface/interface.c`. Baseline `ae10935da`.
Real file UNCHANGED (`git diff --stat` empty). Notes: `scratch/workers/n1_hud_draw_interface_etc.md`.

## Baseline / final (real file)

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 15 | 1 | 2 |
| final | 15 | 1 | 2 |

Point guard pass; park drift 0 (one park, `_interface_get_weapon_hud_index`); owner census clean; fake scan
0 leads. `_render_debug_profile` and `_render_debug_profile_stall_tick` are do-not-spend in the brief and were
not touched.

## `_interface_get_weapon_hud_index` (448 / 20) — park stays; the mechanism is now MEASURED and inadmissible

January 448 padded / 444 meaningful / 20 relocations / sha `0673f867b219cdbd`; ours 448 / 20 /
sha `57151d28c46aeee1`. Target 144 instructions, ours 145 (+2 pad nops).

### Census

Frame: January `push ebp; mov ebp,esp; push ecx` plus `ebx/esi/edi`; ONE dword local at `[ebp-4]`
(`flashlight`), the out-parameter at `[ebp+8]`. Ours is byte-identical here. Both builds write the local with
INTEGER stores at both producers — `mov dword ptr [ebp-4], 0` at `+0x1b` and
`mov ecx,[eax+0x2f8]; mov dword ptr [ebp-4], ecx` at `+0x147`/`+0x150`. **No frame or slot gap.**

Relocations: 20 vs 20, identical kinds, destinations and order. The last two addresses shift by two bytes
purely because of the instruction-stream drift below. No missing or extra call.

### The single difference, duplicated into the two tail-duplicated exits

```text
January   mov edx,[ebp-4]  …  mov eax,[ebp+8]  …  mov dword ptr [eax], edx
ours      fld dword ptr [ebp-4];  mov edx,[ebp+8];  fstp dword ptr [edx]
```

Everything up to and including `mov eax,[eax+0x48c]` at `+0x16d` is byte-identical.

### New measurement: what makes VC7 use the integer copy

Twelve spellings of "float local, integer producers, one `*out = local;` sink" were compiled in micro-TUs with
`interface.obj`'s own cflags (`scratch/workers/n1_hud_draw_interface_etc/lab1.c`, `lab2.c`, driver `lab.py`):

| lab | spelling | lowering |
| --- | --- | --- |
| L1 | the current `interface.c` form | x87 `fld`/`fstp` |
| L2 | declaration and `f = 0.0f;` split | x87 |
| L3 | `out[0] = f;` | x87 |
| L4 | explicit else arm before the sink | x87 (duplicated) |
| L5 | local pointer copy `real *sink = out;` | x87 |
| L6 | no local; `*out` written at each site | GP, but `[ebp-4]` disappears (wrong shape) |
| M1 | if/else phi, no declaration initialiser | x87 |
| M2 | ternary straight into `*out` | x87, and the local disappears |
| M3 | local is a 1-member struct, sink `*out = h.power;` | x87 |
| M4 | an extra `real` parameter (FP present elsewhere) | x87 |
| **M5** | **1-member struct local AND whole-aggregate sink `*out = h;`** | **GP: `mov edx,[ebp-4]; mov dword ptr [ecx],edx`, with the local's integer producers unchanged — January's exact shape, value loaded before the pointer** |
| M6 | initialiser copied from another float local | x87 |

`/Op`, `/Oi`, `/Og`, `/Ot` and `/Gf` on L1 all stay x87, confirming and extending the 2026-09-12 `/Op`
negative.

### Why the one working mechanism is inadmissible

M5 requires the out-parameter to stop being `real *`. That is refused by three independent facts: the January
map atlas and HCEA both attest `long interface_get_weapon_hud_index(real *flashlight_power)`; the in-tree
caller `_interface_draw_screen` is baseline EXACT and declares `real flashlight_power;` with
`&flashlight_power`; and `interface_obj_weapon_hud_best_fuzzy_20260912.md` already rejected the
"aggregate-copy facade" by name. **Zero shapes were spent in the real file** — the twelve lab spellings are
scratch micro-TUs.

**Classification: float-copy lowering tie with a measured, inadmissible mechanism** — not an untried shape.

### Reopen criterion (replaces the vague 2026-09-12 wording)

The previous criterion was "original January source or an independently attested source construct that
naturally causes VC7 to use the integer copy". That construct is now identified: a **4-byte whole-aggregate
assignment through an aggregate pointer**. So the criterion becomes concrete — January-side or PDB-side
evidence that `flashlight_power` is a named 4-byte aggregate (a one-member struct/typedef in a genuine owner
header) used by BOTH `interface_get_weapon_hud_index` and `interface_draw_screen`; `M5` in
`scratch/workers/n1_hud_draw_interface_etc/lab2.c` is the proven codegen. Absent that evidence this row is a
permanent honest residual and no further shapes should be spent on it.
