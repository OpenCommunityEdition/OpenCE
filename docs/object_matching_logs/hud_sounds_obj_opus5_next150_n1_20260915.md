# `hud_sounds.obj` opus5 next-150K lane, wave n1 (2026-09-15)

Worker `n1:hud_draw_interface_etc`. Unit `source/interface/hud_sounds.c`. Baseline `ae10935da`.
Real file UNCHANGED (`git diff --stat` empty) — `_hud_play_sound` is parked, so the work is a
**park-reopen proposal**, not a landing. Notes: `scratch/workers/n1_hud_draw_interface_etc.md`.

## Baseline / final (real file)

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 0 | 1 | 0 |
| final | 0 | 1 | 0 |

Point guard pass; park drift 0; owner census clean (`candidate-only vs base []`); fake scan 0 leads.

## PARK-REOPEN: `_hud_play_sound` — strict EXACT in scratch

Candidate: `scratch/workers/n1_hud_draw_interface_etc/hs_v3.c`
(the current real file plus the reopened body and nothing else).

| measure | January | parked base | candidate |
| --- | ---: | ---: | ---: |
| padded / meaningful | 352 / 349 | 384 | **352** |
| relocations | 13 | 13 | **13** |
| normalized sha16 | `1d026cea1652a463` | `5ad2867ca1866a…` | **`1d026cea1652a463`** |
| instructions | 125 | 139 | **125** |

Gate: `python -B tools/campaign/gate.py source/interface/hud_sounds --source
scratch/workers/n1_hud_draw_interface_etc/hs_v3.c --all --forbid-emitted-symbol _point_from_line3d`
→ `EXACT 352 _hud_play_sound` / `exact 1 residual 0 unwritten 0`.

### Census (the gap was never a frame or relocation gap)

Frame: January and the parked base both emit `push ebp; mov ebp,esp; push ecx` plus `push esi/ebx/edi`;
one dword local at `[ebp-4]` holds the `long` loop counter (`inc eax; movsx esi,ax; mov [ebp-4],eax`).
Identical in the candidate. Relocations: 13 vs 13, identical multiset and order
(`_tag_block_get_element_with_size`, the file literal ×2, the `!"unreachable"` literal ×2,
`_display_assert` ×2, `_system_exit` ×2, `_sound_stop_impulse`, `_unspatialized_impulse_sound_new`,
`_unattached_looping_sound_start`, `_unattached_looping_sound_stop`).

The one structural fact the census exposes is a **parameter-enregistration swap**:

| parameter | slot | January | parked base |
| --- | --- | --- | --- |
| `sound_indices` | `[ebp+0x14]` | **held in EBX** (`mov ebx,[ebp+0x14]` at `+0x18`); used as `[ebx+esi*4]` at 5 sites | reloaded at 5 sites |
| `played_flags` | `[ebp+0x18]` | reloaded at all 4 sites (`+0x69`, `+0x91`, `+0xd6`, `+0x129`) | held in EBX |

## The two source hunks

**1. The `long index` locals are a reconstruction artefact; January names
`sound_indices[absolute_sound_index]` at every site.**
January re-tests the handle inside the guarded block (`+0x9b cmp eax,-1; +0x9e je 0xa9`) — a comparison that
is provably redundant once a cached `index` local holds the value, and which VC7 deletes in the local-bearing
form. January nevertheless emits only ONE load per block (`mov eax,[ebx+esi*4]`): it CSEs the load but not the
compare. January also cross-jumps the shared store `mov [ebx+esi*4],eax` at `+0xd3` between the
`'snd!'` and `'lsnd'` cases, which only works when both cases end in an identical instruction. This is
laws_w2 **A5** (alias / hand-strength-reduction removal) applied to a value alias.
Measured alone (`hs_v1.c`): 384/13 but 136 instructions instead of 139, with the duplicated compare restored.

**2. The `default:` arm ends in `break;`, not `SET_FLAG(...) ; goto next_sound;`.**
January's default-arm `SET_FLAG` copy (`+0x69..+0x7b`) is a tail duplicate of the single shared one at
`+0xd6..+0xe5`: its assert cleanup `add esp,0x14` is scheduled BETWEEN `shl edx,cl` and `or word [eax],dx`,
which is compiler interleaving, not an independent source statement. Writing `break;` and letting the arm fall
into the one shared `SET_FLAG(*played_flags, absolute_sound_index, TRUE)` removes the `goto` and the
`next_sound:` label, drops `played_flags` from four source references to three, and hands EBX to
`sound_indices` — which makes the two cases' tails identical so VC7 cross-jumps them exactly as January does.

Semantics are unchanged: the old default arm set the flag and jumped past the shared set; the new one falls
into the shared set. Nothing else in the function changes.

## Experiment matrix

| shape | result |
| --- | --- |
| `hs_v1` hunk 1 alone | 384/13, 136 insns (base 139) — strictly closer |
| `hs_v2` hunk 1 + `default:` moved last in both switches | 384/13, 136 — case/default order inert |
| `hs_d1` (diagnostic, discarded) hunk 1 + `long *sound_index_array = sound_indices;` | 384/13, 136 — an explicit pointer alias does NOT flip EBX, so no steering device can do this |
| **`hs_v3` hunk 1 + hunk 2** | **352/13 strict EXACT, 125/125 instructions** |

## Park criterion and why it is met

`config/parked.json` records class `unclassified` and *"Reopen only for authentic source/type/control-flow
evidence preserving these house rules."*

Both hunks are control-flow / lvalue facts read off January's own bytes — the surviving redundant compare, the
cross-jumped shared store, the enregistered EBX base, and the interleaved assert cleanup. Nothing the park was
created to protect is undone: no `volatile` pointer returns, `SET_FLAG`/`TEST_FLAG` are still used, the
explicit `return;` stays, and the five-argument prototype stays in `source/interface/hud.h`. Nothing prohibited
is used: no register/pointer steering (the alias diagnostic was measured and DISCARDED because it does not
work), no redundant store, no fake branch, no UB, no expression-order search, no assembly, no header or config
change. The candidate is strictly simpler than the parked body — it deletes two locals, a `goto` and a label.

## Checks on the candidate

- `--forbid-emitted-symbol _point_from_line3d`: PASSED.
- Owner census vs `build/base` and `build/split`: candidate-only `[]`, base-only `[]`, candidate-only vs target `[]`.
- `tools/fake_match_scan.py`: 0 review leads.
- `CL /Zs /W3`: 12 warnings before and after, all from shared headers; none new, no C4013.
- `scratch/parkcheck.py`: reports the EXPECTED reopen drift `_hud_play_sound 384 13 -> (352, 13, 1d026cea1652a463)`.
- No sibling rows exist in this TU, so there is no sibling loss.

## Disposition

`hud_sounds.obj` would be **1/1 strict exact (352 padded / 349 meaningful)** once the orchestrator verifies and
unparks in a dedicated commit. The worker landed nothing.
