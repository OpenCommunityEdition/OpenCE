# `units.obj` — `unit_export_function_values` closed WITHOUT a compiler barrier

## Summary

`unit_export_function_values` is strict-exact with no `_ReadWriteBarrier`, no
`#pragma`, and no intrinsic. The three lines that UEF-8 introduced
(`void _ReadWriteBarrier(void);`, `#pragma intrinsic(_ReadWriteBarrier)`, and
the `_ReadWriteBarrier();` call) are removed from `source/units/units.c`.

| Metric | January target | Retained candidate |
| --- | ---: | ---: |
| Meaningful bytes | 380 | 380 |
| Padded bytes | 384 | 384 |
| Relocations | 18 | 18 |
| Normalized SHA-256 | `4149e4cd…b20c35` | `4149e4cd…b20c35` |
| `section_infos_equal` | — | **True** |

The emitted section is **raw byte-for-byte identical** to the previously
accepted barrier build, so nothing else in the object moves.

## What the barrier was actually doing

Only one thing: it stopped VC7's list scheduler from hoisting the output-pointer
formation above the `tag_get` call. Without it the preamble came out as

```
001e  lea  ebx,[esi+0xd4]        <- output pointer hoisted INTO the pre-call slot
0024  call tag_get
0029  mov  edi,eax               <- copy fills the post-call slot
002b  add  esp,0x10              <- cdecl cleanup deferred
002e  add  edi,0x198
```

against January's

```
001e  call tag_get
0023  add  esp,0x10
0026  lea  ebx,[esi+0xd4]
002c  lea  edi,[eax+0x198]
```

30 normalized bytes differed, all in the preamble.

## The two source-level controls that replace it

### 1. Index the arrays; let VC7 strength-reduce them

The reconstruction walked two explicit pointers (`function_mode`,
`function_value`) and a `function_count` down-counter. **The sibling
`object_export_function_values` in `source/objects/objects.c` — which is
strict-exact — indexes instead**, and January's object shows VC7 turning that
into exactly the same walking pointers plus a homed down-counter:

```
objects.obj  _object_export_function_values
  001e  call tag_get
  0023  add  esp,0x10
  0026  add  eax,0x108
  002b  lea  ebx,[edi+0xd4]
```

Cleanup immediately after the call, then both bases, no copy — January's unit
exporter has the identical shape. Writing the unit exporter the same way (a
`for` over `function_index`, subscripting
`unit_definition->unit.function_modes[]` and
`unit->object.incoming_function_values[]`) makes the whole preamble byte-exact
and drops the residual from 30 bytes to 9. The walking pointers and the
`[ebp-4]` counter in the target are strength-reduction output, not source.

This also explains why every pointer-form experiment in the UEF matrix
plateaued: they were all reconstructing by hand what the compiler derives.

### 2. Name `private_loop_frame_index` and use it twice

The last 9 bytes were a load-order swap in the shield-sapping arm:

```
JANUARY  0102 mov ax,word[eax+0x2e]     0106 mov cx,word[esi+0x80]
OURS     0102 mov cx,word[esi+0x80]     0109 mov ax,word[eax+0x2e]
```

Source operand order does not control this — spelling the predicate
`animation->private_loop_frame_index > unit->object.animation.state.index`
keeps the same load order and merely flips the compare to `cmp ax,cx; jg`
(11 bytes, worse). Binding the animation field to a local **used twice** — once
in the predicate and once in the division — does control it:

```c
short private_loop_frame_index =
    animation->private_loop_frame_index;
boolean before_private_loop;

before_private_loop =
    unit->object.animation.state.index <
    private_loop_frame_index;
...
value =
    (real)unit->object.animation.state.index /
    (real)private_loop_frame_index;
```

UEF-12 had tested named locals and found VC7 eliminated them — but it bound
**both** operands, each with a single use. A single-use copy is propagated away;
a **two-use** local is a real value, and materializing it before the predicate
is what puts the `eax`-dependent load first. Binding both operands two-ways
(`both2`) regresses to 9 bytes again, so it is specifically the animation-side
local that matters.

## Experiment record (this lane)

| Shape | Result |
| --- | --- |
| Barrier removed, otherwise unchanged | 384/18, nd=30 |
| Vehicles idiom (initializers, `value` before `mode`, `for` loop) | nd=30 |
| All six orders of the three preamble assignments | nd=26 whenever `mode` precedes `value`, else 30; count placement irrelevant |
| Unnamed `unit_definition` (direct getter into the member) | nd=26, object identical to the named form |
| Count from the definition; `&…[0]` spelling; declaration initializers | nd=26, all identical |
| **Indexed loop** | **nd=9** — preamble byte-exact, 117/117 instructions |
| Indexed + `private_loop_frame_index > state.index` | nd=11 (compare flips, load order unchanged) |
| Indexed + single-use local | nd=9 (propagated away) |
| Indexed + locals for **both** operands, each used twice | nd=9 |
| **Indexed + two-use `private_loop_frame_index`** | **nd=0, `section_infos_equal` True** |

## Verification

- `section_infos_equal(_unit_export_function_values)` = **True**; normalized
  SHA-256 equals `4149e4cd…b20c35`; 384/384 bytes, 18/18 relocations.
- `tools/audit/compare_object_exact_sets_20260827.py`, barrier build versus
  barrier-free build: **189 exact both, `lost_exact` empty, `newly_exact`
  empty**. Raw section bytes for the exporter are identical between the two.
- Clean forced rebuild (`rm -rf build/base` then ninja): reproduces.
- `tools/campaign/verdicts.py --diff` over all **8,246** functions:
  **gained 0, REGRESSIONS 0**. Board unchanged at 271/619 objects,
  4,451/8,246 functions.
- `halobetacache_build`, `libcmt_build`, `semantic_progress`, `progress`: pass.
- `python -m tools.audit_object_admission` (`"revoked": []`),
  `tools.audit_semantic_matches` (`accepted_exact: 4487`),
  `tools.parked_functions` (`"stale": []`, `"invalid": []`): pass.
- `pytest tools`: **203 passed**.
- `git diff --check`: clean.
- `build/split/source/units/units.obj` SHA-256 is
  `de70778800ba600aa7365d9fa789b2d1147baf3b67e50d2f9a70ff958c9fa757`, matching
  the corrected `last_time` split.
- No `_ReadWriteBarrier`, `#pragma`, intrinsic, inline asm, `volatile`,
  `register`, `__declspec`, raw address or punning remains in
  `source/units/units.c`.

### A note on objdiff's percentage

`build/report.json` reports `source/units/units` at 97.4489% with 20 functions
below 100%, including this one at 93.396%. That is **not** a contradiction and
is **not** changed by this work: objdiff's `fuzzy_match_percent` compares raw
instruction bytes without the campaign's normalization, so every relocation
slot (zero in a CL object, resolved in the split) counts against it. The same
20 functions score below 100% before and after this change, the exporter's raw
section bytes are identical between the barrier and barrier-free builds, and
`section_infos_equal` — the designated acceptance authority — is True for all
189 code owners. `tools/coff_compare.py` remains the authority; no comparator
exception was added or used.
