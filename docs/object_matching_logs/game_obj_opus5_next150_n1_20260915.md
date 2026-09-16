# `game.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

## Result

No source change. `source/game/game.c` is byte-identical to the lane baseline
`ae10935da`; `git diff --stat -- source/game/game.c` is empty.

| gate | exact | residual | unwritten | point guard | park drift |
| --- | ---: | ---: | ---: | --- | --- |
| baseline (HEAD) | 25 | 2 | 0 | pass | — |
| final (real file) | 25 | 2 | 0 | pass | 0 of 2 |

`tools/fake_match_scan.py`: 0 review leads. Both experiments below were run with
`gate.py --source` against scratch copies; the two parked bodies were never
touched in the real file.

Worker notes with every probe: `scratch/workers/n1_game_engine_ctf_game_etc.md`;
candidates in `scratch/workers/n1_game_engine_ctf_game_etc/`.

## `_game_set_game_variant_from_name` (parked, unclassified) — structural decode, no admissible shape

Target 96 padded / 92 meaningful / 4 relocations; candidate 48 / 2.
This is the first recorded frame + relocation census for the row.

**Frame census.** January `sub esp,0xd0`, ours `sub esp,0x68`.
`0xd0 == 2 * sizeof(struct game_variant)`. `[ebp-0xd0]` is the out-buffer handed
to `game_engine_get_variant_by_name`; `[ebp-0x68]` is the copy destination that
is then installed (`lea edx` as the inlined setter's parameter, `lea edi` as the
copy destination, `lea esi` as the copy source in the install arm). Ours keeps
only the out-buffer, at `[ebp-0x68]`.

**Relocation census.** Ours is missing exactly `_csmemset` (REL32 @0x3a) and one
`_game_variant_global` (DIR32 @0x35): the complete null arm of the inlined
`game_set_game_variant`. January retains that arm although the tested pointer is
`lea edx,[ebp-0x68]`, a frame address that can never be zero (January even
reuses EDX as the literal `0` memset value inside it). HCEA's later 360
reconstruction records the same dead arm ("the decompiler emits a never-true
stack-address guard around a memset of the global variant"), so the construct is
authentic to the original source rather than a January artefact.

**Shapes measured (6 in-tree).** declaration-order swap; pointer local for the
call result; nested call `game_set_game_variant(game_engine_get_variant_by_name(&variant, name))`;
single self-referencing local; pointer local initialised to `&variant`; callee
buffer in an inner block. All are 48/2 except the nested call, which is
**80 bytes with all 4 relocations, both arms and January's control flow** and
differs only by the missing second local and its copy.

**Mechanism probes (11, out-of-tree lab TU compiled with `game.obj`'s own
cflags; the lab reproduces both in-tree fixed points exactly).** Two-level
inlining through a static wrapper, one-element array + decay, a `void *` setter
parameter, swapped buffer/installed locals, caller defined before the callee, an
`__inline` setter, and a `p ? &variant : p` ternary all fold to 48. The only lab
form that reaches 96 bytes with frame `0xd0` and two `rep movsd`
(`if (p) { variant = *p; p = &variant; } set(p)`) branches on EAX *before* the
copy, moves the register saves into the arm and pushes a literal 0 in the memset
arm — the right size for the wrong reason, and it needs a source `if` January
does not have.

**Measured law.** VC7 13.00.9254 always folds the inlined `if (!p)` when the
actual argument is the address of a local, and then forwards the aggregate copy
so the local disappears. The test survives only when the pointer is a call
result or a phi — but then the intermediate copy is dead. January's object proves
C2 did keep a dead `test lea,lea`, so its IL path for this function is not
reproducible from any admissible spelling found here.

**Reopen criterion.** A January-side source/local record, or a natural donor,
showing how the setter's null test survives substitution of a frame address
(e.g. an intermediate January helper between `from_name` and the setter, or a
measured pass-ordering lever that defers the address materialisation). Start
from `scratch/workers/n1_game_engine_ctf_game_etc/gv_c.c`: it already matches all
four relocations and January's control flow.

## `_game_load` (parked, unclassified) — one-instruction schedule tie

Target and candidate are both 208 / 19 with an identical relocation census by
target (4x `_game_globals`, 3 assert sites, `_random_seed_debug_log`,
`_csmemcpy`, `_scenario_load`) and identical frames. The entire residual is the
position and register of one load:

```
January: call _scenario_load ; add esp,0x14 ; test al,al ; mov eax,[_game_globals] ; pop esi ; je
ours   : call _scenario_load ; mov ecx,[_game_globals] ; add esp,0x14 ; test al,al ; pop esi ; je
```

January can reuse EAX because the load is emitted after the last use of the call
result. Three direct-global shapes were measured (`loaded` local + two direct
reads; no locals at all; cached pointer moved after the `if`): each is 224 / 21,
because VC7 never CSEs two direct `game_globals` reads here. January's single
load therefore proves the cached-pointer local the current body already has, and
only its emission position differs — which follows the statement position.

No w2/w3 law fires: no frame gap, no width fact, no missing call, no inline
decision, and the body is already single-exit.

**Reopen criterion.** A measured VC7 lever that sinks a post-call global load
past the call-result test (nothing in A1-A45 covers it), or January local records.

## Checks

- Final real-file gate with the `_point_from_line3d` guard: 25 / 2 / 0, unchanged.
- `scratch/parkcheck.py source/game/game`: parks 2, drift 0.
- Owner census unchanged (no candidate object was applied).
- `tools/fake_match_scan.py source/game/game.c`: 0 leads.
