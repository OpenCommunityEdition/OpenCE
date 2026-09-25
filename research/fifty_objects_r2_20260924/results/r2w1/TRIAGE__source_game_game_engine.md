# source/game/game_engine

## decision
OWNER

## est_probability
0.8

## route
Owner-queue item 6: Q1 is the in-loop alias (production line 2693, not /Od-attested, load-bearing) versus the /Od-attested `*(real_point2d *)&global_goal[i].position` view copy (load-bearing, so it fails rule-24 byte-inertness). Q2 is the one-consumer headers versus the TU-local variant. Q3 is the debug_player_color pad aggregate and the descriptive teleport statics. Everything is already 180/180 with audit PASS.

## why_new
Checked whether first-party evidence could remove the ruling: in HCEX, netgame_goal.position is union real_point3d with only x/y/z, u/v/w and n[3] members. It has no 2D member, so the /Od 8-byte copy requires the cast and the ruling is still needed.

## prior_negatives_checked
round-1 wave3/wave4 game_engine results and reviews; owner queue item 6; game_engine_obj_large_closeout_20260919.

## notes
On a yes, the packet is research/fifty_objects_20260925/w/owner_queue/pick_game_engine; re-sweep it against any concurrent game_engine.h edits.