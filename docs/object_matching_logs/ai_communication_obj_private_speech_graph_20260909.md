# AI communication private speech graph (2026-09-09)

## Verdict

This packet advances `source/ai/ai_communication.obj` by **three strict
functions, 879 meaningful bytes, and 896 padded bytes**.  The focused gate
preserves every strict owner present at published base `b74f7a2d7` and rejects
any emitted `_point_from_line3d` definition.

Two complete or useful ordinary-C reconstructions remain explicitly fuzzy and
receive **zero exact credit**.  They are recorded in `config/parked.json` with
their current measurements so later agents can resume from the best coherent
source instead of rebuilding the same graph.

## Exact additions

| January owner | semantic private name | meaningful | padded | relocs | normalized SHA-256 |
| --- | --- | ---: | ---: | ---: | --- |
| `_code_000316e0` | `ai_communication_consider_speech` | 407 | 416 | 23 | `25863d0d4a8dfd0ea6e6a8ac6d2ca0f0f7db7ec5765802b6764ee1ad4eade286` |
| `_code_00033e40` | `ai_communication_find_specific_actor_to_talk` | 191 | 192 | 6 | `14076ea7ef7d6b7e429df6acb9ab15edf972046cff71f1fe5d419adef216c325` |
| `_code_00033f00` | `ai_communication_find_global_actor_to_talk` | 281 | 288 | 11 | `03e48c206448b68b65e6ebcbb2363218bc9d0f5a7a445a8b38d412a88b16716c` |

`config/symbols.json` also replaces six address-derived private labels with
their authenticated names and marks them static:

- `ai_communication_consider_speech`;
- `ai_communication_look_secondary_at_unit`;
- `ai_communication_look_secondary_at_object`;
- `ai_communication_actor_talk_weight`;
- `ai_communication_find_specific_actor_to_talk`;
- `ai_communication_find_global_actor_to_talk`.

The two secondary-look bodies were already exact and are only renamed here.

## Reconstruction evidence

The semantic map came from the independent HaloCEA owners under:

- `research-cache/halocea-full-blobs-20260830/src/blam/ai/ai_communication_actor_talk_weight.c`;
- `research-cache/halocea-full-blobs-20260830/src/blam/ai/ai_communication_find_specific_actor_to_talk.c`;
- `research-cache/halocea-full-blobs-20260830/src/blam/ai/ai_communication_find_global_actor_to_talk.c`;
- `research-cache/halocea-full-blobs-20260830/src/blam/ai/ai_communication_finished.c`.

January disassembly, exact relocation identities, existing table structures,
and the older same-compiler work documented in
`ai_communication_obj_jonas_speech_research_wave8_20260828.md` were the
authorities for the January source shape.  HaloCEA was used as a semantic map,
not copied as an exact-code oracle.

`ai_communication_consider_speech` only becomes exact when the genuine
`ai_debug.communication_unit_repeat_disabled` field is used.  The nearby
`communication_random_disabled` field is not equivalent.  The timer tolerance
table is now expressed with its semantic index order
`[communication_priority][team][timer_type]`; its flat initialized bytes do not
change.

The global actor finder preserves a target-proven January behavior in its
cause-position setup: that branch asks for `subject_unit_index` and stores the
result in `subject_point`.  This looks suspicious in isolation, so it is called
out here rather than silently “corrected”; the exact January instruction and
relocation stream establishes that it is real behavior, not matching bait.

## Honest fuzzy work

### `ai_communication_actor_talk_weight`

- January: 912 padded bytes, 24 relocations, 302 instructions,
  `9e4e417453eb75eb5f5a4d4fa433f75006e5a17ba9c8c84f3921a9d0cb108279`.
- Candidate: 912 padded bytes, 24 relocations, 302 instructions,
  `4386615ddee912defcf37785c3542a0ac4bcc261c3f6e10ba497d77f154d4053`.
- Ordinary objdiff: 99.31293 percent.

The remaining code delta is one independent perception-argument push schedule
and the final two boolean-local loads.  An archived 2026-08-28 experiment
reduced this to only the boolean load order, but it declared
`actor_get_perception_knowledge` as returning `short`.  The actual definition
in `actor_perception.c` returns `long`; retaining that conflicting TU-local
prototype would violate source ownership and ABI rules.  This packet therefore
uses the correct declaration in `actor_perception.h` and parks the three-order
residual rather than claiming the older artificial improvement.

### `ai_communication_finished`

- January: 1,584 padded bytes, 87 relocations,
  `79de598402e5831bf7376d7471f92958b7ecf34ee356b0110ec16127875e524b`.
- Candidate: 896 padded bytes, 29 relocations,
  `44eba783428926e28e6c68010f33daa6f131348a390fa28dca3a14a0fc79a212`.
- Ordinary objdiff: 40.492752 percent.

The retained body is a coherent complete reply-dispatch path: it resolves the
replying actor, applies damage/script/chance/filter gates, constructs speech,
updates timers, and schedules the replying look.  January's much larger
relocation envelope shows that its diagnostic/debug-string work is not yet
reconstructed.  It is therefore parked as unclassified, not described as
complete or assigned a speculative compiler mechanism.

## Header ownership and blast radius

`actor_get_perception_knowledge` and `actor_visibility_at_point` are declared
in their real owner, `source/ai/actor_perception.h`, with signatures verified
against their definitions.  Before the edit, the complete `.c` includer
inventory was only `source/ai/action_flee.c`; this packet adds
`source/ai/ai_communication.c`.

The PID-safe focused `action_flee` compile after the header edit remains 9
exact / 3 residual / 1 unwritten.  Comparing all twelve emitted owners against
the frozen pre-edit `build/base/source/ai/action_flee.obj` is all-equal.  Thus
the header position introduces no C2 definition-position regression.

## Verification

No Ninja, configure, or shared-build command was run in this lane.

```text
python -B tools/campaign/gate.py source/ai/ai_communication --all \
  --alias ai_communication_consider_speech=code_000316e0 \
  --alias ai_communication_actor_talk_weight=code_00033ab0 \
  --alias ai_communication_find_specific_actor_to_talk=code_00033e40 \
  --alias ai_communication_find_global_actor_to_talk=code_00033f00 \
  --alias ai_communication_look_secondary_at_unit=code_000318c0 \
  --alias ai_communication_look_secondary_at_object=code_00031970 \
  --forbid-emitted-symbol _point_from_line3d
```

Result: **40 exact / 4 residual / 4 unwritten**, compared with the frozen
baseline **37 exact / 2 residual / 9 unwritten**.  The two additional residuals
are the honest fuzzy bodies above; the five removed unwritten slots are exactly
the three strict and two fuzzy reconstructions.  Checkpoint object SHA-256:
`3992570A2D3B765D0AB67DBCA4271105656C2B401DDDB92D1F6C59CABD4517D9`.

Additional checks:

- PID-safe focused gate for every `actor_perception.h` includer;
- all twelve emitted `action_flee.obj` owners equal before/after;
- exact COFF comparison for all three new strict owners;
- `tools/fake_match_scan.py` over both edited source/header files: zero leads;
- JSON parse checks for `config/symbols.json` and `config/parked.json`;
- `git diff --check`.

The root orchestrator owns the post-integration configure, full build, complete
semantic sweep, and publication decision.
