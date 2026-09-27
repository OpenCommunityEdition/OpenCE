# E1 results (worker E1, wave 3; released Codex units)

Base: worktree HEAD 26684ca8; the nine E1 units are byte-identical to published canonical a8854940 (`git diff` empty).
Toolchain VC7 13.00.9254 (CL 483e00c4 / C1 02843d31 / C2 9dbf908b), objdiff 3.3.1. Codex worktrees were read only
through `git log/diff/show` from this worktree; nothing was written there. Per-card record: CARDS.md (cards/ holds
the immutable pre-registrations). Owner questions: OWNER_QUESTIONS.md.

## Per target
| target | B | verdict | evidence |
|---|---:|---|---|
| `@periodic_function_build_variable_period_x_table@4` | 244 (256 padded) | **EXACT** (P3) | /Od term-1 spelling `real_random()*0.25f + 0.25f`; T1 trace + L1-L3 labs decode the residual |
| `_connected_geometry_find_or_add_edge` | 240 | EXACT only with the held uninitialised `direction` (owner E1-Q2) | C1; reachability: assert-protected |
| `_connected_geometry_find_or_add_vertex` | 189 | EXACT only with the unattested `realcmp_epsilon` macro (owner E1-Q3) | Lane D candidate re-verified |
| `_triangle_coplanar` | 380 | not attempted (no new evidence; Lane D stop) | hypothesis recorded |
| `_sound_cache_debug_render` | 368 | not attempted (structural 119 vs 126 insns; no new evidence) | diff read |
| `_rasterizer_bitmap_new` | 388 | bounded research, 0 shapes; reloc delta = naming only | T5 (IL at MARK) |
| `_dead_camera_update` | 1,235 | held (original bug); packet re-verified 4/4 (owner E1-Q4) | battery/dead_camera_ownerQ.txt |
| `_cinematic_render` | 1,272 | bounded research, 0 shapes; OR-order key decoded | T3/T3b/T3c/T3e |
| `_lightning_submit` | 2,571 | bounded research, 0 shapes | T4 |
| path_structure_bsp (admission) | 0 | battery PASS; cleanup patch; owner E1-Q1 (neighbour-read disclosure) | battery/path_structure_bsp.txt, P1/P1b/P1c/P2 |
| hardware_geometry (admission) | 0 | still blocked by the held MoveResourceMemory call; H1 cleanup ready | battery/hardware_geometry_H1.txt |

## Patches (my units; LF, `git apply --check` OK at HEAD)
| patch | sha256 | effect | review_patch (lead reviewer) |
|---|---|---|---|
| `periodic_functions.patch` | bdd1e79c... | builder EXACT; object 7/7; admission battery PASS | 1 gain, 0 losses, 0 added/removed |
| `periodic_functions_ALT_cosine.patch` | 9ed0ba54... | same + builder uses cosine() (/Od); adds `_cosine` COMDAT (identical, link PASS) | 1 gain, 0 losses, 1 added |
| `path_structure_bsp.patch` | dca5c196... | zero credit; /Od flag loop replaces the goto; object identical | 0/0/0 |
| `rasterizer_xbox_hardware_geometry.patch` | 92678e08... | zero credit; must land WITH the symbols.json proposal | vs current split 13/4 unwritten (the 4 renames); vs regenerated split 17/17 |

## Lead-owned proposals
- `proposals/symbols_hardware_geometry_H1.patch`: 11 in-place lines (5813-5825), line surgery only: 4 renames
  (`_code_00158470@4` -> `_D3DVertexBuffer_Unlock@4`, `_code_001584b0@4` -> `_IDirect3DVertexBuffer8_Unlock@4`,
  `_code_001584d0@4` -> `_D3DIndexBuffer_Unlock@4`, `_code_00158510@4` -> `_IDirect3DIndexBuffer8_Unlock@4`) and
  `"static": true` on those 4 + the 7 named wrappers (all absent from cachebeta publics). Scratch csplit: only
  rasterizer_xbox_hardware_geometry.obj changes (833 compared; control split == build/split).
- config/parked.json: retire `@periodic_function_build_variable_period_x_table@4` (entries[18]) after the lead's fresh
  exact verification (tools.campaign.unpark), and flip periodic_functions to Matching after the batch gate.
- Observation only (not patched): periodic_functions.h declares periodic_function_evaluate/transition_function_evaluate
  on one line (rule "Put each function parameter on its own line"); a byte-inert shared-header style fix.

## Mechanisms (new; sealed stock-equal traces + labs)
- C2 opcode 0x267 (0 bytes) = FP barrier for a parenthesised FP subexpression or a named-float assignment; the list
  scheduler treats it as unit class 0 (one integer op per cycle).
- VC7 /O2 factors `x*c + c` into `(x + 1.0f)*c` (`fadd [1.0]; fmul [c]`) with no barrier node.
- Commutative OR key in cinematic_render: SHL inner = alpha key + 0x61, AND inner = ((base id & 3) << 14) + 0xfe3a
  (storage-3 base symbol), 16-bit; January needs a high-id alpha temp and base id = 1 mod 4.
- Tools: tools/sched_trace.py, node_dump.py, sort_trace.py, il_walk.py, mark_names.py (all on W7's sealed w7trace
  driver; runs under scratch/campaign/workers/E1/runs), sgate.py / altroot.py / splitcmp.py (scratch-split gating and
  battery redirection), mkpatch.py / splice.py.
