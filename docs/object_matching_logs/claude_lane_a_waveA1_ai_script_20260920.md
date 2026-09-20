# Lane A wave A1 — `ai_script.obj` closes at 116/116

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, baseline
`6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.

## Result

| | before | after |
|---|---:|---:|
| `source/ai/ai_script` census | `exact 115 residual 1 unwritten 0` | **`exact 116 residual 0 unwritten 0`** |
| Halo accepted functions | 7,291 / 7,574 | **7,292 / 7,574** |
| Halo accepted meaningful bytes | 1,443,979 | **1,444,193** |
| overall accepted | 7,558 / 1,465,211 | **7,559 / 1,465,425** |

**New strict meaningful bytes: 214. New strict padded bytes: 224. New strict
functions: 1. Regressions: 0.**

| field | value |
|---|---|
| unit | `source/ai/ai_script` |
| function | `_ai_scripting_migrate_and_speak` |
| meaningful / padded | 214 / 224 |
| target section | `build/split/source/ai/ai_script.obj`, stable key `section:51` |
| relocations | 16 / 16, `relocdiff` reports **0 differing rows** |
| normalized SHA | `relocdiff` reports **`sha equal`** |
| before / after verdict | `residual [sha]` -> `EXACT` |
| owner / data / COMMON / point | unchanged; `--forbid-emitted-symbol _point_from_line3d` passes |

## Evidence

January's call site in `_ai_scripting_migrate_and_speak`:

    5a  lea  edx, [ebp-0x204]     ; target_name
    60  push edx
    61  lea  eax, [ebp-0x404]     ; source_name
    67  push eax
    68  call _hs_runtime_get_executing_thread_name
    6d  push eax
    6e  push <format>
    73  push 2                    ; _error_silent
    75  call _error
    7a  add  esp, 0x14            ; FIVE dwords
    7d  mov  esi, dword ptr [ebp+0x10]   ; speech_type loaded only AFTER the call

Our build emitted `add esp, 0x18` — six dwords — because the source passed
`speech_type` as a fourth value.

The format literal, read directly out of January's `.rdata`, is
`"%s: ai_migrate_and_speak %s %s %s"` (33 bytes + NUL = 0x22, matching the `_0CC`
length token in its mangled name): **four `%s` conversions, three values**.

`speech_type` is loaded at +0x7d, *after* the call, so it provably is not one of
the pushed arguments.

## Why this is admitted as a preserved original defect

This is Bungie's own source shape recovered from January's bytes, not a
byte-steering construct. It is landed under the campaign's original-bug policy
(`docs/matching_methodology.md`, house rule 24) with a `BUG (preserved for exact
matching)` comment that states the evidence, the runtime consequence and the
corrected form.

Supporting facts:

- **Cross-build stability.** The 2020 `halo_cache_symbols.exe` carries the same
  format literal and also emits `push imm32 / push 2 / call / add esp,0x14`, with
  `speech_type` still loaded only after the call. Different compiler, ~18 years
  later, defect unrepaired. (Corroboration only, never January proof.)
- **In-tree precedent for disclosing a UB-class January defect.**
  `source/bitmaps/bitmap_extract.c:2101` already ships
  `// BUG (preserved): January does not bound the final run before reading its next sequence index`
  over an out-of-bounds read. `source/ai/ai_script.c:1597` already ships a
  January diagnostic-argument defect **in this same file**. 54 `BUG` disclosures
  exist in the tree today.
- **Blast radius is a debug-only branch.** The call sits inside
  `if (ai_debug.print_migration || ai_debug.print_scripting)`, so no shipping
  configuration executes it. This distinguishes it from the two cases the owner
  previously excluded (`_dead_camera_update`'s uninitialized read and
  `_compare_profile_sections`'s uninitialized return), which are on
  always-executed paths.
- **The concrete effect is disclosed, not glossed.** From January's prologue
  (`push ebp / mov ebp,esp / sub esp,0x404 / push ebx / push esi / push edi`),
  `esp = ebp-0x410`; after the five pushes `esp = ebp-0x424`, so the fourth
  conversion's slot is `[esp+0x14] = ebp-0x410`, the saved-EDI home. `vsprintf`
  therefore dereferences the caller's entry EDI as a `char *`. The BUG comment
  says so explicitly.

### Owner ratification

**This is the one item in Lane A that turns on a policy reading rather than on
measurement, and it is flagged for the owner.** `astra_90pct_rejected_hypotheses_20260920.md`
(Wave 6) says "this is not permission to introduce UB in another residual". That
sentence was written about a different concession (two stock SDK no-op copies)
and is not a blanket override of house rule 24, and the `bitmap_extract`
precedent is directly on point — but the owner may read it more broadly.

If the owner declines, the revert is a **single token**: restore `speech_type` as
the fourth argument. `ai_script` returns to `exact 115 residual 1` and the lane's
strict total drops by exactly 214 meaningful / 224 padded bytes / 1 function.
Nothing else in the lane depends on it.

## Shapes refuted before landing this one

Every alternative that could emit `add esp, 0x14` against a four-`%s` format was
enumerated and refuted from January's own bytes:

- substituting a different third value (e.g. passing `source_name` twice);
- reordering the four values, or moving `speech_type` to another position;
- reading the format as three conversions plus a literal `%` (refuted by the
  raw `.rdata` bytes: `pct_count 4`, `pct_s_count 4`);
- dropping a different value and keeping `speech_type`;
- a wrapper or macro supplying the fourth conversion (no such owner exists);
- a historical source revision with a lawful shape (23-blob `branch_sweep`
  census; best historical result 113).

Also measured inert and **not** to be repeated: declaration-order permutations;
a frame-size hypothesis (both sides are `sub esp, 0x404`); a scheduling or
register-tie hypothesis (zero register-permutation events, 16/16 relocations,
0 differing rows).

## Verification performed

    ninja -j4 all_source progress semantic_progress
      -> halobetacache 1,444,193 / 1,770,166 (7,292 / 7,574)
      -> overall 1,465,425 (7,559)

    python -m tools.campaign.stable_verdicts diff scratch/lane-a-before.json scratch/lane-a-waveA1.json
      -> gained source/ai/ai_script::section:51 _ai_scripting_migrate_and_speak 224
      -> gained 1 224 bytes; regressions 0     (exit 0)

    scratch/orch/ledger_diff.py   -> +1 function / +214 meaningful, regressions 0
    python -m tools.parked_functions        -> active 184, stale 0, invalid 0
    python tools/audit_object_admission.py  -> 19 candidates / 0 contradicted / 8 rejected / 0 revoked
    python tools/fake_match_scan.py --format json source/ai/ai_script.c  -> 0 findings
    python tools/fake_match_scan.py --format json source/ai               -> 2 findings (both inherited,
                                                                             the ai.c #pragma optimize pair)
    git diff --check                        -> clean

No header, configuration, comparator, target object, build tool or park was
changed. `config/parked.json` is untouched: this function was never parked.
