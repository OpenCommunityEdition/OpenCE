# `shader_transparent_chicago_preprocessor.obj` — Opus5 250K lane, wave w3c (2026-09-15)

Lane `opus/250k-house-clean-20260915`, worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
Scope: `source/rasterizer/xbox/shader_transparent_chicago_preprocessor.c` only.

## Result

**No source change landed.** `git diff --stat` for the file is empty.

| gate (real file, `--all --forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 1 | 1 | 0 |
| final | 1 | 1 | 0 |

Guard: pass. `scratch/parkcheck.py`: 1 park, drift 0. `tools/fake_match_scan.py`: 0 review leads.

## `_shader_transparent_chicago_create` — skipped, evidence-exhausted; the `[size]` tag is a single instruction

Target 432 padded / 17 relocs / `c465d59ce6f43d70…`; park base 416 / 17 / `c716cea5de50b243…`; objdiff 96.15504.

The gate prints this row as `[size 416!=432, sha]`, which reads like a structural gap. It is not. `alndiff` reports 143
target instructions against 128 of ours, but **14 of the target's are trailing padding `nop`s**: the real function bodies are
`0x1a2` and `0x1a0` bytes, a **2-byte, one-instruction** difference that happens to cross a 16-byte padding bucket. The whole
divergence is:

```
January:  mov ecx,[ebp+8] ; mov eax,0xc00 ; inc ecx ; <2 output stores>
          movsx esi,cx ; mov [ebp+8],ecx ; cmp esi,[ebx] ; mov al,1 ; jl loop
          ... loop exit jmps past the shared reload
ours:     mov eax,0xc00 ; <2 output stores> ; mov eax,[ebp+8] ; inc eax
          movsx esi,ax ; mov [ebp+8],eax ; cmp esi,[ebx] ; jl loop
          ... loop exit jmps to  mov al,[ebp-1]
```

Everything else — including the reuse of the `shader` parameter's incoming home `[ebp+8]` for `map_index`, the entry
`mov byte [ebp-1],1`, the error block, the `mov byte [ebp-1],0` / `mov al,[ebp-1]` pair and the final combiner stores — is
identical in both builds. The ECX-versus-EAX index choice is a **consequence** of AL being live; the single lever is whether
VC7 rematerialises the known-TRUE `result` as `mov al,1` in the loop latch or reloads it from `[ebp-1]` at the join.

### Law detector run (w2/w3 laws applied to this row)

- **A42** fires and *confirms the retained source*: VC7 does not hoist a store into the function entry, so January's
  `mov byte [ebp-1],1` at `+0xc` — before the first assert — proves a declaration initializer `boolean result = TRUE;`.
  The "assign `result` inside the `maps.count > 0` block" family is therefore refuted by bytes, not merely untried.
- **A29(d)** (constant result phi) describes January's `mov al,1`, but its remedy — collapse mid-function returns into one
  bottom return — is already the retained shape: the body has a single bottom `return result;` and still reloads. E15 in the
  object ledger further proves the `[ebp-1]` home is optimizer-selected rather than declaration-driven (deleting `result`
  entirely reproduces the same 416 bytes).
- **A24** (parameter-home offer) does not discriminate: both builds already pack `map_index` into `[ebp+8]`.
- **A3 / A10 / A23** add nothing: E10 (do/while), E14 (pre-increment), E20 (index width), E23 (update-clause assignment) and
  E24 (loop-invariant conditional assignment) already cover the loop and result-materialisation spellings, and E21/E22 close
  the "anchor the TRUE to a real value" family.

No w2/w3 law yields an untried, house-rule-admissible shape, so **no shape was spent** on top of the 28 recorded
experiments. The park's reopen criteria (authoritative January source; a strict same-toolchain donor with this joint
topology — a loop-carried known-true boolean, a call-free success path and a call on the failure path; or a compatible
VC7/QFE) are unchanged and none is available in this lane.

### Note for the orchestrator

When ranking `[size]` rows for structural work, this row should be excluded: its size delta is one 2-byte instruction across
a padding boundary, not a missing local, block or statement.

## Evidence used

January split COFF bytes and relocations; `tools.campaign.alndiff`, `tools/campaign/gate.py`, `scratch/parkcheck.py`,
`tools/fake_match_scan.py`; `config/parked.json`; `docs/object_matching_logs/shader_transparent_chicago_preprocessor_obj.md`
(the E01-E28 matrix, its "Do not repeat" list and its reopen criteria) and
`…_obj_jonas_credible_fuzzy_park_20260830.md`. No supplied binary was executed.
Worker notes: `scratch/workers/w3c_rasterizer_lights_shader_transparent_chicago_preprocessor_etc.md`.
