# source/ai/ai_communication

## decision
RETIRE

## est_probability
0.03

## route
None for closure. talk_weight: the single real region at +0x35f is the operand order of `cause_matches | subject_matches` (Jan `mov al,[ebp-3]; or al,[ebp-2]`); frame slots are identical. From the /Od (0x48ca30, found by scanning for the 10.0/5.0 constants) I took subject declared before cause and the expression `subject | cause`. Q1 (declaration order), Q2 (operand order) and Q3 (both) are all byte-inert. finished is the recorded cross-jump floor. event: the structural lane's v10h candidate was never landed. It reaches size 8064 (equal to January), 336/336 relocations and the 0x126c frame, but still has 74 real hunks. Beyond that it needs the allegiance_incident_type enum moved into game_allegiance.h and has register ties left. The /Od-attested uninitialised play_type read is a HELD January-bug class, and even with it the frame goes +8. So an owner ruling would not close event either.

## why_new
New facts: (1) the talk_weight /Od declaration and operand order were measured inert (Q1-Q3), which strengthens the park. (2) event's normalize2d blocker from 0923 (the action_charge NODUP provider) is probably gone: action_charge.c now uses the header normalize2d after the round-1 provider repair. Not measured here, and it does not fix event's ties. (3) Admission prerequisites, confirmed from first-party data: _ai_conversation_new is not a cachebeta public, and none of 833 January split objects references it, so it was file-static in January; ours is external and has a prototype in ai_communication.h. _actor_reset_idle_vocalization_timer and _ai_communication_update_speech_timers are already static in ours and need symbols.json "static": true. .data 276 is coff_compare-identical (69 relocations); its 50% score is the objdiff 3.3.1 '$'-literal defect, a scorer-version effect. The TU also carries consumer-local prototypes of foreign functions (unit_test_speech, unit_speak, actor_iterator_new/next, actor_audibility_at_point, etc.).

## prior_negatives_checked
config/parked.json entries for talk_weight (instruction-scheduling) and finished (register-allocation; 19 shapes, claude_lane_a_crossjump_ai_communication_finished_20260920); ai_communication_obj_opus5_150k_w3 (tw0-tw5); commit 3fdad47e (event owner ruling 0920); claude_structural_HANDOFF_20260923 + claude-structural-20260923 scratch/lane/w/ai__ai_communication/REPORT.md (v1-v10h, v6 play_type frame +8, v8 normalize2d); Lane B rejected hypotheses (update_speech_timers is a declaration-count tie on real_math.h); docs/objdiff_data_relocation_defect_20260922.md.

## notes
event v10h (+ v8 normalize2d, now that provider link should pass) could land as a zero-credit fidelity packet after the game_allegiance.h enum sweep. Making _ai_conversation_new static changes its ABI, so measure that (it is currently EXACT at 304) before the integrator applies it.