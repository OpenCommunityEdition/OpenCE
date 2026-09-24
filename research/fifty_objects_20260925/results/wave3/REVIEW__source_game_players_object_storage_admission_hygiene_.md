# REVIEW source/game/players (object storage / admission hygiene)

approve: True

## per_object
[
 {
  "unit": "source/game/players",
  "approve": true,
  "issues": "Approved: production.patch and the 3-line symbols.json static flags, landed together. It changes storage only and is score-neutral: object_audit symbol diffs go from 1 to 0, and pdb_storage disagreements from 5 to 2. Optional, verified supplement: scratch/w/review3__admission_hygiene_/production_plus_common.patch adds the missing COMMON definitions of players_globals and team_data, which January's linker_common attributes to players.c. The object is still not complete: device and teleport code residuals remain, and so do the header-declared statics teleport_internal and handle_powerup_equipment."
 }
]

## checks
All checks were independent re-runs on tree 954eebd2. Slug: scratch/w/review3__admission_hygiene_/ (LEDGER.md, REVIEW.md).

(1) Patch applies and matches the candidate. `git apply --check` of scratch/w/players/production.patch on the current tree is OK. The patched players.c is identical to the worker's ps3.c (cmp).

(2) Gate is unchanged. `gate.py --all` gives 68 exact / 2 residual / 0 unwritten for both head.c and cand.c, and every row is identical. The residuals are device [size 160!=176] and teleport [size 1312!=1296, relocs 62!=61].

(3) Only storage changes in the object. A positional COFF compare (seccmp.py) of cand.obj against head.obj, where head.obj equals build/base, shows:
- all 159 sections identical: bytes, plus raw relocation records resolved to symbol names;
- no $L label churn;
- of 470 symbol rows, only two differ, both storage 2->3: _players_static_data and _machine_to_player_table.

(4) Storage evidence supports all three flags.
- None of the three symbols is in cachebeta publics: 0x453408, 0x2DEE08 and 0xAA180. The publics RVA equals the symbols.json file_offset, anchored on the public _debug_render_player_teleport at 0x453464.
- No public exists at 0xAA180, so the stub is not an ICF-folded external.
- HCEX gives machine_to_player_table DataKind 5 (DataIsFileStatic).
- January emits _player_examine_nearby_unit with zero references and no public name, which fits a static function (VC7 static-emission class).

(5) No outside references. xref_check finds 0 UNDEF references in either the January split or build/base. A symbol-row scan of both trees finds the names only in players.obj. `git grep` finds no other reference.

(6) csplit emulation. A control run with an unedited config reproduces build/split exactly (833 objects). The worker's symbols.json changes only players.obj: all 110 sections are identical and exactly 3 symbol rows go from storage 2 to 3. The file edit is 3 in-place line edits with CRLF preserved, the same line count, and +48 bytes.

(7) object_audit, four combinations:
- production vs current split: FAIL(3);
- A+B (cand.obj vs emulated split): FAIL(2), only the 2 code residuals, 118 symbols / 0 differ;
- A alone: 3 symbol diffs;
- B alone: 2 symbol diffs.
So A and B must land together, as the worker says.

(8) Surplus. The surplus list is the same as production. surplus_identity, re-pointed at cand.obj and the emulated split, finds 13 COMDATs with 0 not identical. provider_link(cand.obj) passes all 32 rows.

(9) Score-neutral. A mini objdiff 3.3.1 report gives identical unit entries for current, A+B, A-only and B-only: 13122/15390 code, 66/70 functions, data 5349/5349. That equals build/report.json.

(10) Warnings, fake scan and strip test. CL /W3 gives 18 warnings for both head and candidate, zero delta. fake_match_scan finds 0 leads. The strip test does not apply because no byte depends on the edit.

(11) The worker's errors.h claim holds. With the include added, the gate rows are identical and only $L label names in the relocations of sections 85 and 155 differ.

(12) House rules and section-8 blockers. The diff contains only storage keywords backed by the PDB, plus removal of a duplicate consumer-local extern. That extern is identical to main/main.h:192, which players.c includes at line 256; January's main.obj defines _player_spawn_count. There is no header edit, so there is no consumer sweep. There is no new scope, name, cast or aggregate.

(13) Optional supplement, checked separately. I verified the extra COMMON definitions described under issues: gate, sections, audit, score, W3, fake scan, and a link against January's linker_common.obj in both orders.

## issues
The worker's packet is APPROVED as proposed. Every factual claim checked out.

Land together:
- A: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/production.patch
- B: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/config/symbols.json, with `, "static": true` added in place on lines 2598, 22521 and 23014.

The integrator should diff B against config/symbols.json to get the 3 line ops, then run a csplit-only regeneration. After landing, the players object_audit symbol diffs go from 1 to 0, and pdb_storage disagreements go from 5 to 2. The two left are teleport_internal and handle_powerup_equipment: their prototypes are declared in players.h and the move is owner-gated, as disclosed.

OPTIONAL SUPPLEMENT (not required for approval):
C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review3__admission_hygiene_/production_plus_common.patch

This is the worker patch plus bare tentative definitions `struct players_globals *players_globals;` and `struct data_array *team_data;`. It replaces the consumer-local `extern struct data_array *team_data;`. `git apply --check` is clean. Land it instead of A, with the same B.

Evidence for the supplement:
- January's linker_common COMMON cluster, sections 154-156, holds _players_globals, _team_data and _player_data as players.c's records.
- players.obj is the only object that references _players_globals and _team_data. Both are PDB publics.
- HCEX has all three contiguous, as DataIsGlobal.
- Our tree defines neither symbol anywhere. object_audit cannot see this because it skips section<=0 symbols.

Measured result of the supplement:
- gate 68/2 with identical rows;
- all 159 sections positionally identical;
- the only symbol change is those two going from UNDEF to COMMON (val 4), the same as the existing _player_data;
- audit 0 symbol diffs, score identical, W3 delta 0;
- no LNK2005 against January's linker_common.

ADVISORY (existed before this packet; not blockers here, but relevant before any players object admission):
- players.c:941 has a C4013 implicit declaration of `error`. The genuine owner is cseries/errors.h. Adding the include does not change code, but it renumbers $L labels, so it is held.
- The name `players_static_data` and its 0xC18 aggregate are not confirmed by first-party sources (cachebeta publics, HCEX name lookup or the atlas). They were introduced in commit 677d0982.
- Device (owner RB2 wrapper macro) and teleport (allocator ties) remain residual and are not part of this approval.
