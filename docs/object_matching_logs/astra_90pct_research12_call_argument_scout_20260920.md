> Research-only packet12: zero retained production changes and zero admitted exact credit. Latest validated production remains wave9.

# Packet 12: bounded call-argument scout

**No new actionable argument mismatch was found.** Two concrete, already-known differences are preserved below as positive diagnostic controls. Neither justifies a new source shape. No source, header, configuration or comparator was changed; no C compiler, Ninja or tests were run; admitted credit is zero.

This manual review covered seven unresolved AI functions from the frozen packet 5 >=95% / equal-direct-call-multiset set, totaling **5,805 meaningful bytes**, plus one already-exact AI control and the previously established packet 11 biped width mismatch outside that set. Current objects still have equal decoded direct-call multisets in all nine reviewed functions. The historical 104-function / 131,184-byte pool was used to select cases, not re-certified as the current whole-project residual total. This is a bounded scout, not proof of argument equality across that pool. No generic symbolic-argument implementation was added alongside root's diagnostic work.

## Ranked positive windows — both previously known

| Rank | Function and ceiling | January versus current consumer | Prior status and required follow-up |
| --- | --- | --- | --- |
| 1 | `ai_script::_ai_scripting_migrate_and_speak`, 214 meaningful bytes, in the >=95 pool | `_error` at target +0x75 versus current +0x79. January supplies only three values for the four `%s` conversions in `"%s: ai_migrate_and_speak %s %s %s"`. Current loads `speech_type` from parameter EBP+0x10 into ESI and pushes it at +0x5d. Cleanup is 0x14 versus 0x18. | Explicitly recorded in the September 14 fresh-graphs, w1 and w3 ai-script ledgers. No new primary work is needed to establish the omission. Reproducing it would require a lawful consumer interpretation that does not read the missing argument; the genuine variadic API and four-conversion format provide no such basis. No omission probe is proposed. |
| 2 | `units/bipeds::_biped_update_moving`, 4,071 meaningful bytes, **outside** the >=95 pool | `_actor_is_leaping` at target +0x9ce versus current +0x9a3. January loads dword `[biped+0x1a4]` at +0x9c2 and pushes it unchanged at +0x9cd. Current loads the same dword, sign-extends its low word at +0x99f and pushes the narrowed result at +0x9a2. The genuine parameter is `long actor_index`. | Packet 11 directly checked January, actual /Od 0x8c693d/6943 and HCEA's long parameter. Old `workersD/bipeds/um1.c` already removes the short cast. This is a useful known consumed-width control, not a newly discovered exact-gain lever. A new experiment would need distinct primary provenance for other moving gaps; repeating um1 or the cast correction is not justified. |

These ceilings identify unresolved function sizes, not predicted gains. They must not be added to campaign progress. `results.json` preserves raw instruction windows, target/current callee identities, argument roles, prior status and the specific missing follow-up evidence.

## Negative controls and stale-history filter

| Function | Window reviewed | Why the apparent difference is not a new argument-value mismatch |
| --- | --- | --- |
| `_actor_input_update` | `_game_team_is_enemy`, both calls +0x525 | Both zero-extend the same object+0x68 and actor+0x3e words and push them in the same order. EDX/EAX versus ECX/EDX is a register substitution. The current September 19 body already contains the prior structural corrections. |
| `_ai_test_line_of_sight` | `_ai_debug_lineofsight`, target +0x25/current +0x2c | Target EDI/ESI hold parameters EBP+8/EBP+0x10; current ESI/EDI hold those same parameters. Reversed register names preserve the four arguments and their order. Other stack/vector differences were not promoted to argument defects without provenance. |
| `_ai_communication_actor_talk_weight` | `_actor_visibility_at_point`, both calls +0x29f | Both pass the same EAX result from `_actor_get_perception_knowledge`. The push moves from +0x289 to +0x283 across instructions that write only ECX. The later OR operand swap also combines the same boolean bytes. Prior cast/OR and boolean-initializer probes remain exhausted. |
| `_prop_get_active_by_unit_index` | `_datum_get`, both calls +0x4e | Both fetch the next index from the same `[object+0x50]` dword. Its register changes ECX to EDX; moving the push ahead of the table load does not change its value. |
| `_actor_aim_projectile` | Result-copy window +0x277..+0x28c | Two loads/stores within the same 12-byte output copy are reordered. No differing named global, immediate, field value or width reaches a corresponding external call in the reviewed residual. |
| `_action_vehicle_setup_impromptu` | x87 window +0xe2..+0xea | The remaining difference is accumulation order. No concrete differing call argument was recovered. |
| `_ai_scripting_follow_target_ai` | Two `_ai_index_to_string` calls at +0x29/+0x44 | **Current function is already strict exact.** Both calls intentionally use the first `ai_reference`; the second writes the target-name buffer. Current source discloses this January diagnostic bug. Historical residual/hold descriptions must not reopen an issue already reproduced and accepted. |

The bounded prior read used the relevant function sections of `actors_obj_large_closeout_20260919.md`, ai-script fresh-graphs/w1/w3, actor-moving fresh-graphs/w3/n4, actor-combat w3c, props n2, action-vehicle w1/w3d and ai-communication waveD/w3, plus the packet 11 biped receipt. It did not revisit active ai-debug, perception or weapons source work. Existing shared-schedule evidence was used to avoid reopening unrelated x87/order families.

## Evidence and release

`export.py` uses the existing COFF reader and disassembler to save full January/current streams and unmodified alignments. `*.diagnostic.txt` separately omits relocation display annotations to make manual inspection compact; it is not an admission comparator. `export.json` binds every current object/function hash and call multiset. `results.py` rechecks those object hashes, validates the seven-function pool selection and exact control, and writes `results.json` with the reviewed windows and classifications. Neither script infers a general argument-flow law or compiles source.

Run `python scratch/astra-wave12/call-argument-scout/export.py` and then `python scratch/astra-wave12/call-argument-scout/results.py` from the worktree root to reproduce the read-only evidence. This scout ends with **zero novel leads, zero shapes and zero credit**. Evidence ownership is **RELEASED** to root.
