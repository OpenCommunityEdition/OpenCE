# triage_ai_b MANIFEST (round 2, base f6d00a8c) - TRIAGE ONLY
No candidate source written; no tracked file edited; no claim taken. All compiles were lab `gate.py --edits` probes of the
production file, outputs in this directory (p*.json/obj actor_moving, q*.json/obj ai_communication, base_*.obj production rebuilds).
git status: no tracked modification.

| object | verdict | credit |
|---|---|---|
| ai/actor_looking 15/16 | RETIRE (_actor_look_update allocator/colouring; admission also needs RESERVED _cross_product2d provider packet) | 0 |
| ai/actor_moving 31/36 | RETIRE for closure; zero-credit hygiene ready: drop REAL_MATH_EXTERNAL_POINT_FROM_LINE3D (row-neutral, COMDAT == action_charge copy, provider link PASS) + /Od transform form (stays EXACT) | 0 |
| ai/ai_communication 45/48 | RETIRE (talk_weight Q1-Q3 inert; finished 19-shape floor; event 74 real hunks + held play_type read) | 0 |
| ai/ai_debug 59/60 | RETIRE (render_actor x87 staging, 97 REAL regions) ; naming/storage prerequisites listed | 0 |

Admission prerequisites recorded (integrator, for when functions close):
- ai_communication: symbols.json "static": true for _actor_reset_idle_vocalization_timer, _ai_communication_update_speech_timers;
  _ai_conversation_new is NOT a cachebeta public and no January object references it -> static + drop header prototype (measure ABI);
  consumer-local prototypes of foreign functions in ai_communication.c; .data 276 is coff-identical ('$' scorer defect, v3.3.1 only).
- ai_debug: 19 atlas renames (_code_ -> _ai_debug_*; 25 anchors verified), 33 symbols.json "static": true (split 2 / PDB not public),
  _ai_debug_render_actor_jmptable is a label inside the parent section; .data 56 coff-identical (scorer defect).
