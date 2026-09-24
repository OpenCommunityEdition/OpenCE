# REVIEW (wave 3): source/game/players storage packet (worker scratch/w/players, verdict ADMISSION_FIX_PROPOSED)

Reviewer slug: scratch/w/review3__admission_hygiene_/. Tree 954eebd2. No tracked file was edited; no ninja/configure/git mutation.
Scope: A = scratch/w/players/production.patch (players.c: `static` on machine_to_player_table and players_static_data;
delete the duplicate `extern short player_spawn_count;`), B = scratch/w/players/config/symbols.json (3 lines gain
`"static": true`). The device wrapper macro and the teleport o1 body are declared not landable by the worker and are not
part of this approval.

## Checks run (all independent re-runs)

1. **Patch.** `git apply --check` of production.patch against the current tree: OK. The patched players.c is identical to
   the worker's ps3.c (cmp).
2. **Gate.** `gate.py --all` gives 68 exact, 2 residual, 0 unwritten for both head.c and cand.c, and every row is
   identical. The residuals are unchanged: device [size 160!=176, sha] and teleport_internal [size 1312!=1296,
   relocs 62!=61, sha].
3. **Object bytes (seccmp.py, positional).**
   - head.obj equals build/base players.obj apart from .debug$S.
   - cand.obj against head.obj: all 159 sections are identical (bytes, plus raw relocation records resolved to symbol
     names). No `$L` churn.
   - Of the 470 symbol rows, only two differ, both storage 2->3: `_players_static_data` (section 4) and
     `_machine_to_player_table` (section 7).
4. **Storage evidence.**
   - None of the three symbols is in cachebeta publics: `_machine_to_player_table` 0x453408, `_players_static_data`
     0x2DEE08, `_player_examine_nearby_unit` 0xAA180. The publics RVA equals the symbols.json file_offset; I anchored
     this on the public `_debug_render_player_teleport` 0x453464.
   - No public exists at 0xAA180 at all, so this is not an ICF-folded external.
   - The neighbours at 0x453448..0x453460 are already static in symbols.json. The sibling
     examine_nearby_vehicle/device/objects are already static.
   - HCEX.pdb gives `machine_to_player_table` DataKind 5 (DataIsFileStatic), which corroborates. The other two have no
     HCEX symbol under those names, so there is no contrary evidence.
   - January emits `_player_examine_nearby_unit` with ZERO relocations referencing it anywhere, and it is not public.
     That is the VC7 static-emission class (a static that is used and inlined is still emitted). Our production source
     is already `static`.
5. **Cross-references.** xref_check finds 0 UNDEF references in either the split or build/base. A full symbol-row scan
   of both trees finds the three names only in players.obj. `git grep` finds no other source, header or config
   reference apart from the three symbols.json lines.
6. **csplit emulation.**
   - A control run with an unedited config copy reproduces build/split exactly (833 objects, `diff -rq` empty).
   - With the worker's symbols.json, only source/game/players.obj differs. Its 110 sections are positionally
     identical, and exactly 3 symbol rows change storage 2->3.
   - symbols.json edit: 3 lines, each with `, "static": true` inserted before ` },`. CRLF is preserved, both files have
     23,852 lines, and the size grows by 48 bytes (in-place line edits, no reserialisation).
7. **object_audit.**
   - production against the current split: FAIL(3), meaning 2 code residuals plus the examine_nearby_unit storage 2/3.
   - A+B (cand.obj against the emulated split): FAIL(2), only the 2 code residuals; 118 January symbols, 0 differ.
   - A alone: 3 symbol diffs. B alone: 2 symbol diffs. The worker's rule that A and B must land together is CONFIRMED.
8. **Surplus.** The list is identical to production: 13 .text COMDATs, the literals and .drectve.
   - surplus_identity, re-pointed at cand.obj and the emulated split: 13 code COMDATs, 0 not identical.
   - provider_link(cand.obj): all 32 rows PASS; SELECTED-PROVIDER LINK: PASS.
9. **Score and data.** In an objdiff 3.3.1 mini report (od/), the whole unit entry is identical across the current
   state, A+B, A-only and B-only: 13122/15390 code, 66/70 functions, data 5349/5349. That equals build/report.json, so
   the packet is score-neutral and data stays at 100%.
10. **Warnings and fake scan.** CL /W3 /Zs gives 18 warnings for head and 18 for the candidate, with zero delta. The
    `error` C4013 at players.c 941 already exists at HEAD and is not introduced here. fake_match_scan finds 0 leads.
11. **The errors.h claim.** Probe err.c adds `#include "cseries/errors.h"` to cand.c. Gate rows are identical. Sections
    85 and 155 differ only in `$L` label NAMES in their relocation records (11 `$L` renames). The worker's claim is
    confirmed, and dropping the include is the conservative choice.
12. **House rules and section-8 blockers on the diff.**
    - The diff contains only PDB-attested storage keywords and the removal of one consumer-local extern (a verbatim
      duplicate of main/main.h:192). main.h is included directly at players.c:256, and `_player_spawn_count` is defined
      by January main.obj, which is the genuine owner.
    - There is no new block scope, local, cast, decoration, name or aggregate. The strip test does not apply, because
      no byte depends on the edit.
    - No header is touched, so there is no consumer sweep.

## Verdict

**APPROVE the worker packet as proposed: A (scratch/w/players/production.patch) plus B
(scratch/w/players/config/symbols.json, lines 2598/22521/23014) landing together.** The integrator should derive the
3 line ops by diffing against config/symbols.json and then regenerate the split with csplit only. After it lands, the
players object_audit symbol diffs go 1 -> 0 and pdb_storage disagreements go 5 -> 2 (the remaining two are
teleport_internal and handle_powerup_equipment, which are header-declared and blocked as disclosed).

## Optional supplement (verified, NOT required for the approval)

**R8 finding (pre-existing and outside the packet; invisible to object_audit, which skips section<=0 symbols).**
- January's COMMON pool (build/split/source/linker_common.obj, sections 154-156) holds `_players_globals`,
  `_team_data` and `_player_data` as one cluster, sitting between the hs and game_engine_king clusters. That places the
  whole cluster in players.c (COMMON-pool ordering oracle).
- `_players_globals` and `_team_data` are referenced ONLY by players.obj. Both are PDB publics (0x5924EC, 0x5924F0).
- HCEX has all three contiguous as DataIsGlobal (0x560280/84/88), in the same order.
- Our tree defines NEITHER of them anywhere:
  - players.h:270 has only `extern struct players_globals *players_globals;`.
  - players.c:431 has the consumer-local `extern struct data_array *team_data;`, which has no definer anywhere in
    source/.

**Supplement.** scratch/w/review3__admission_hygiene_/production_plus_common.patch is the worker patch plus bare
tentative definitions `struct players_globals *players_globals; struct data_array *team_data;` next to `player_data`.
It replaces the extern line. Checks:
- `git apply --check` OK on 954eebd2; the round-trip result equals am1.c.
- Gate: 68/2 with identical rows.
- All 159 sections are positionally identical to cand.obj.
- The symbol delta is only `_players_globals`/`_team_data` going from UNDEF(val 0) to COMMON(val 4), the same form as
  the existing `_player_data` row.
- object_audit against the emulated split: 0 symbol diffs.
- objdiff score is identical; W3 delta is 0; fake scan 0.
- Linking with January's linker_common.obj in both orders gives no LNK2005/LNK1169.
- The declaration order inside the cluster is not a lever (COMMON law). The definitions are bare, with no
  initialiser.

**How to apply.** The integrator can land this INSTEAD of production.patch, together with the same symbols.json B.

## Advisory (pre-existing; not blockers for this packet; relevant before any players OBJECT admission)

- **C4013 `error`.** players.c 941 has an implicit declaration; the genuine owner header is cseries/errors.h. The
  include is code-inert but renumbers `$L` labels, so it is held.
- **`players_static_data` provenance.** The name and its 0xC18 aggregate are not attested in the cachebeta publics
  (they are static there), in the HCEX name lookup or in the atlas. The name dates from the 20260830 cleanup
  (677d0982). Before admission, run the aggregate-vs-loose-statics question through first-party evidence.
- **The two remaining storage disagreements.** `_player_teleport_internal` and `_player_handle_powerup_equipment`
  need the owner-gated header prototype move; the worker's shadow sweep flagged declaration-count canaries.
- **Code residuals.** Device (owner RB2 wrapper macro) and teleport (allocator ties) remain.
