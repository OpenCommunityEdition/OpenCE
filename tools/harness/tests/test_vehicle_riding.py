"""vehicle_riding.c compiled whole against fake settings, connection and look impulses: a player standing on a
moving vehicle (docs: port/linux/README.md, network.vehicle_riding).

    python -m pytest -q tools/harness/tests/test_vehicle_riding.py
"""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, inline, mutated, read, run, structure  # noqa: E402

CASES = ["carry", "dyaw-vertical", "node-carry", "stuck", "contact-point", "guards", "gate", "deferred",
         "look-carry", "two-local-players", "latch", "momentum", "no-double-count", "spans", "double-speed",
         "landing-frame", "seat-exit-onto-deck", "refresh", "elevator", "local-frame", "node-change", "falling",
         "jump-deck-frame", "contact-record", "collision-others", "claim", "damage-speed-default", "suspended",
         "leg-stomp", "turret-with-travel", "turret-past-limit", "limit-edge", "body-never-judged",
         "take-off-body", "drop-opens-window", "grip-limit", "air-frame-constant", "parked-jump", "grip-standing",
         "off-edge", "tilted", "turn-relative-velocity", "skipped-tick-held", "riding-off",
         "air-control-relative", "window-holds-in-air", "node-turret-test", "air-frame-asymmetric",
         "enclosure-ray", "record-lifetime", "contact-sequence", "contact-crush", "player-keyed",
         "deck-make", "deck-frame-carried", "deck-frame-air", "deck-frame-other-node", "deck-make-cabin", "deck-values", "deck-node", "network-gate",
         "cabin-tilt", "axis-hysteresis", "cabin-verdict", "grace-jump", "cabin-keys", "grace-setting",
          "steep-riding-sequence", "cabin-recovery-rules", "cabin-overlap-retry", "run-jump-shared", "rebase",
          "correction-record", "correction-guards", "correction-two-riders", "correction-skipped-tick",
          "correction-message-time", "correction-rotation", "correction-noncommuting", "node-judged-moved",
          "correction-renormalised", "correction-identity", "correction-since-same", "correction-keys",
         "followed-elapsed", "followed-elapsed-zero", "followed-stopped", "followed-jumper", "followed-role-change",
          "followed-snap", "followed-role-change-teleport", "followed-node", "leash-setting", "transition-agreed-table",
         "cabin-host-verdict", "cabin-host-jump-verdict", "deck-make-cabin-bit", "cabin-take-off-host"]

COOP = "port/linux/game/network_coop.c"
CASES += ["claim-grace", "write-carry-decision", "write-carry-window-next-step"]
CASES += ["cabin-hold-no-touch", "cabin-hold-ends", "cabin-client-crossing", "crossing-rest-place-free"]
CASES += ["window-base-matches-step", "window-base-write-left-out", "window-base-followed", "window-base-role-change"]
CASES += ["map-wins-local", "map-wins-network", "map-client-own-call", "map-script-startup-only",
          "map-per-map", "map-latch", "map-revert-kept", "map-persist", "map-restore-server", "map-editor",
          "map-unallocated", "map-global", "map-global-then-call", "map-global-set-later",
          "map-global-client", "map-global-editor"]
CASES += ["seq-coop-then-silent-mp", "seq-riding-mp-then-coop-off", "seq-riding-mp-then-silent-mp", "seq-dedicated-rotation",
          "seq-coop-entry-rule", "seq-override-base-off", "seq-late-joiner", "map-decision-held", "local-gametype", "map-on-film"]
CASES += ["host-values-win", "host-no-message-keeps-own", "host-bad-value-clamped", "host-joiner-latched",
          "host-local-unchanged", "host-latch-clamped"]

# a fault in vehicle_riding.c, and the case that must catch it
NEGATIVE_CONTROLS = {
    "takeoff-keeps-jump-host": (("\t\tcabin->take_off = TRUE;\n\t\t/* (the host's verdict ends with the phase it was for: a jump's starts unknown, until the host's next part) */\n\t\tcabin->host = FALSE;\n",
                                 "\t\tcabin->take_off = TRUE;\n\t\tif (cabin->host && cabin->host_phase == _vehicle_riding_cabin_sample_standing)\n\t\t\tcabin->host = FALSE;\n"),
                                "cabin-take-off-host"),
    "client-overwrites-host": (("\t\tif (cabin->host && cabin->host_phase == _vehicle_riding_cabin_sample_standing &&\n\t\t\tvehicle_riding_is_riding(state) && !cabin->take_off)\n\t\t\treturn cabin->host_verdict;\n", ""),
                               "cabin-host-verdict"),
    "old-window-seeds": (("if (!cabin || !current_window || state->ticks <= 0", "if (!cabin || state->ticks <= 0"),
                         "cabin-host-verdict"),
    "jump-sample-overrides-host": (("\t\tif (cabin->host && cabin->host_phase == _vehicle_riding_cabin_sample_jump)\n\t\t\treturn cabin->host_verdict;\n\t\tif (!(cabin->host_legacy && state->ticks < RIDING_TICKS) &&\n\t\t\tcabin->jump != _vehicle_riding_enclosure_unknown)\n\t\t\treturn cabin->jump;\n",
                                    "\t\tif (cabin->jump != _vehicle_riding_enclosure_unknown)\n\t\t\treturn cabin->jump;\n\t\tif (cabin->host && cabin->host_phase == _vehicle_riding_cabin_sample_jump)\n\t\t\treturn cabin->host_verdict;\n"),
                                   "cabin-host-jump-verdict"),
    "legacy-air-becomes-host": (("if (phase == _vehicle_riding_cabin_sample_jump && !cabin_evidence)", "if (FALSE)"),
                                "cabin-host-jump-verdict"),
    "client-judges-provenance": (("if (phase == _vehicle_riding_cabin_sample_jump && !cabin_evidence)",
                                  "if (phase == _vehicle_riding_cabin_sample_jump && cabin->jump == _vehicle_riding_enclosure_unknown)"),
                                 "cabin-host-jump-verdict"),
    "cabin-bit-unset": (("made.flags |= FLAG(_vehicle_riding_deck_cabin_bit);", ";"), "deck-make-cabin-bit"),
    "cabin-bit-on-riding": (("if (state->ticks < RIDING_TICKS && cabin &&", "if (cabin &&"), "deck-make-cabin-bit"),
    "landing-agrees-vehicle-change": (("(host_before <= 0 || host_vehicle_before == host_vehicle))", "TRUE)"), "transition-agreed-table"),
    "leash-nan-kept": (("!valid_real(riding_transition_leash) || riding_transition_leash < 0.f",
                         "riding_transition_leash < 0.f"), "leash-setting"),
    "correction-counted-as-motion": (("vector_from_points3d(&corrected_last, &carried, &motion);",
                                       "vector_from_points3d(&last, &carried, &motion);"), "correction-record"),
    "teleport-on-whole": (("distance = magnitude3d(&motion);", "distance = magnitude3d(&step->displacement);"),
                           "correction-guards"),
    "correction-replaced": (("matrix4x3_multiply(&write, &accumulated, &composed);", "composed = write;"),
                             "correction-message-time"),
    "correction-composed-right": (("matrix4x3_multiply(&write, &accumulated, &composed);",
                                   "matrix4x3_multiply(&accumulated, &write, &composed);"), "correction-noncommuting"),
    "skipped-tick-drops-correction": (("\t\tstep->velocity_now = state->velocity;",
                                        "\t\tstep->displacement.i = step->displacement.j = step->displacement.k = 0.f;\n"
                                        "\t\tstep->velocity_now = state->velocity;"), "correction-skipped-tick"),
    "snap-launches": (("\t\tstate->velocity = *frame;",
                        "\t\tscale_vector3d(&step->displacement, 1.f/step->player_ticks, &state->velocity);"), "correction-guards"),
    "mark-not-snapshotted": (("\tstate->mark.correction = vehicle->correction;\n", ""), "correction-two-riders"),
    "run-jump-scale": (("speed = (real)sqrt(run*run + jump_velocity*jump_velocity);",
                        "speed = 1.9f*(real)sqrt(run*run + jump_velocity*jump_velocity);"), "run-jump-shared"),
    "rebase-keeps-old-place": (("\triding_snapshot(state, vehicle);\n\tvehicle_riding_point_to_local(&vehicle->position, &vehicle->forward, &vehicle->up, base, &state->local_point);\n",
                                "\triding_snapshot(state, vehicle);\n\t(void)base;\n"), "rebase"),
    "rebase-other-node": (("if (!vehicle || state->object_index != vehicle->object_index || state->node_index != vehicle->node_index)",
                           "if (!vehicle || state->object_index != vehicle->object_index)"), "rebase"),
    # (the harness's own step, left out: the case's rebase after the carry, whose absence counts the write twice)
    "write-carry-no-rebase": (("define", "WRITE_CARRY_NO_REBASE"), "write-carry-window-next-step"),
    "touching-miss-is-door": (("else if (!check->touching)", "else"), "cabin-overlap-retry"),
    "failed-recovery-drops-anchor": (("if (settle == _vehicle_riding_cabin_settle_failed)\n\t\treturn;",
                                       "if (settle == _vehicle_riding_cabin_settle_failed)\n\t\t{ cabin->have_anchor = FALSE; return; }"),
                                      "cabin-overlap-retry"),
    "claim-only-relaxed": (("\t\tif (objects[index] == vehicle_index)\n\t\t\treturn index;\n", ""), "steep-riding-sequence"),
    "jump-kept-as-deck": (("if (!exception || !cabin || cabin->take_off)", "if (!exception || !cabin)"),
                          "steep-riding-sequence"),
    "carrier-any-vehicle": (("\t\tif (objects[index] == vehicle_index)\n", "\t\tif (objects[index] != NONE)\n"),
                            "steep-riding-sequence"),
    "carrier-without-protection": (("if (!exception || !cabin || cabin->take_off)", "if (!cabin || cabin->take_off)"),
                                   "steep-riding-sequence"),
    "hold-needs-touch": (("return cabin_vehicle != NONE && carried;", "return FALSE;"), "cabin-hold-no-touch"),
    "hold-in-window": (("return cabin_vehicle != NONE && carried;", "return cabin_vehicle != NONE;"), "cabin-hold-ends"),
    "hold-any-verdict": (("define", "HOLD_ANY_VERDICT"), "cabin-hold-ends"),
    "host-open-beats-own-hit": (("\treturn cabin && cabin->vehicle_index == state->object_index && vehicle_riding_is_riding(state) && !cabin->take_off &&\n\t\tcabin->standing == _vehicle_riding_enclosure_enclosed;",
                                 "\treturn FALSE;"), "cabin-client-crossing"),
    "own-miss-revokes-host": (("\tif (vehicle_riding_cabin_verdict(cabin, state, air, TRUE) == _vehicle_riding_enclosure_enclosed)\n\t\treturn TRUE;\n", ""),
                              "cabin-client-crossing"),
    "falling-stricter-off-rising": (("return relative > world_velocity_k ? relative : world_velocity_k;",
                                     "return relative;"), "falling"),
    "tracked-not-used": (("\telse if (support_object_index == tracked_object_index)\n\t\t*support_velocity = *tracked_deck_velocity;\n",
                          ""), "landing-frame"),
    "suspended-adds": (("\t\t\t*new_velocity = *frame;\n", "\t\t\tadd_vectors3d(new_velocity, frame, new_velocity);\n"),
                       "suspended"),
    "last-from-vehicle-now": (("vehicle_riding_point_to_world(&state->vehicle_position, &state->vehicle_forward, "
                               "&state->vehicle_up, &state->local_point, &last);",
                               "vehicle_riding_point_to_world(&vehicle->position, &vehicle->forward, "
                               "&vehicle->up, &state->local_point, &last);"), "carry"),
    "whole-angle": (("return rotation.k;", "return magnitude3d(&rotation);"), "dyaw-vertical"),
    "carry-about-body-origin": (("vehicle_riding_point_to_world(&vehicle->position, &vehicle->forward, &vehicle->up, "
                                 "&state->local_point, &carried);",
                                 "vehicle_riding_point_to_world(&vehicle->origin, &vehicle->forward, &vehicle->up, "
                                 "&state->local_point, &carried);"), "node-carry"),
    "carry-from-velocity": (("vector_from_points3d(&last, &carried, &step->displacement);",
                             "step->displacement = vehicle->translational_velocity;"), "stuck"),
    "carry-from-base": (("vector_from_points3d(&last, &carried, &step->displacement);",
                         "vector_from_points3d(base, &moved_base, &step->displacement);"), "contact-point"),
    "moved-alone-reversed": (("distance3d(base, &last) > riding_moved_alone",
                              "distance3d(base, &last) < riding_moved_alone"), "guards"),
    "step-not-per-update": (("distance > riding_teleport_step*step->updates",
                             "distance > riding_teleport_step"), "guards"),
    "unupdated-move-kept": (("(step->updates == 0 && distance > 0.000001f) ||", ""), "guards"),
    "teleport-step-reversed": (("distance > riding_teleport_step*step->updates",
                                "distance < riding_teleport_step*step->updates"), "guards"),
    "drop-forgets-falling": (("\tif (falling)\n", "\tif (FALSE)\n"), "guards"),
    "local-setting-everywhere": (("\treturn connection == _game_connection_local ? riding_local_decision : riding_network_decision;\n",
                                  "\treturn riding_local_decision;\n"), "gate"),
    "network-uses-local-setting": (("return connection == _game_connection_local ? riding_local_decision : riding_network_decision;", "return connection == _game_connection_local ? riding_local_decision : riding_local_decision;"),
                                   "network-gate"),
    "network-client-off": ((" && connection != _game_connection_network_client)", ")"), "network-gate"),
    "network-film-on": (("\tif (connection != _game_connection_local && connection != _game_connection_network_server && connection != _game_connection_network_client)\n\t\treturn FALSE;\n",
                         ""), "network-gate"),
    "network-editor-on": (("\tif (game_in_editor())\n\t\treturn FALSE;\n", "\tif (FALSE)\n\t\treturn FALSE;\n"),
                          "network-gate"),
    "network-null-game-on": (("return game && TEST_FLAG(", "return !game || TEST_FLAG(", COOP), "network-gate"),
    "network-bit-inverted": (("return game && TEST_FLAG(", "return game && !TEST_FLAG(", COOP), "network-gate"),
    "network-wrong-bit": (("_network_game_cooperative_vehicle_riding_bit);", "_network_game_cooperative_no_player_collisions_bit);",
                           COOP), "network-gate"),
    "dead-ride": (("return has_player && !dead && vehicle_riding_enabled();",
                   "return has_player && vehicle_riding_enabled();"), "gate"),
    "defer-riders-only": (("return applies && state->ticks > 0;",
                           "return applies && vehicle_riding_is_riding(state);"), "deferred"),
    "kept-not-reset": (("riding_kept_dyaws[local_player_index] = dyaw;",
                        "if (dyaw != 0.f) riding_kept_dyaws[local_player_index] = dyaw;"), "look-carry"),
    "single-kept": (("riding_kept_dyaws[local_player_index] = dyaw;", "riding_kept_dyaws[0] = dyaw;"),
                    "two-local-players"),
    "impulse-to-first": (("player_control_permanent_impulse(local_player_index, &impulse);",
                          "player_control_permanent_impulse(0, &impulse);"), "two-local-players"),
    "latch-keeps-turn": (("\t\triding_kept_dyaws[local_player_index] = 0.f;\n", "\n"), "latch"),
    "no-momentum": (("\t\t\tadd_vectors3d(new_velocity, frame, new_velocity);\n", "\t\t\t;\n"), "momentum"),
    "seed-no-subtraction": (("subtract_vectors3d(world_velocity, frame_velocity, velocity);",
                             "*velocity = *world_velocity;"), "no-double-count"),
    "no-span-divisor": (("per_tick = 1.f/step->player_ticks;",
                         "per_tick = 1.f/riding_period(vehicle, double_speed);"), "spans"),
    "no-period": (("per_tick = 1.f/step->player_ticks;",
                   "per_tick = 1.f/step->updates;"), "double-speed"),
    "skipped-tick-not-held": (("\t\tstep->tracked_deck_velocity = state->velocity;\n",
                               "\t\tstep->tracked_deck_velocity = step->displacement;\n"), "double-speed"),
    "deck-point-per-update": (("scale_vector3d(velocity, 1.f/riding_period(vehicle, double_speed), velocity);", ""),
                              "double-speed"),
    "frame-not-added": (("add_vectors3d(velocity, frame_velocity, &relative);", "relative = *velocity;"),
                        "landing-frame"),
    "support-ridden-always": (("else if (support_object_index == riding_object_index)",
                               "else if (support_object_index != NONE)"), "landing-frame"),
    "unclipped": (("riding_clip(new_velocity, support_velocity, support_normal);", ";"), "landing-frame"),
    "refresh-only-on-landing": (("\t\tstate->velocity = *frame;\n\t\tif (supported",
                                 "\t\tif (supported"), "refresh"),
    "no-elevator-clear": (("\t\t\t&state->local_point);\n\t\treturn TRUE;",
                           "\t\t\t&state->local_point);\n\t\treturn FALSE;"), "elevator"),
    "node-not-taken": (("\t\tstate->node_index = support->node_index;\n", "\n"), "node-change"),
    "left-sign-in-world": (("\tmatrix4x3_transform_point(&frame, local, point);",
                            "\tnegate_vector3d(&frame.left, &frame.left);\n\tmatrix4x3_transform_point(&frame, local, point);"),
                           "local-frame"),
    "window-rederives-place": (("\t\telse\n\t\t\triding_snapshot(state, &step->vehicle);\n",
                                "\t\telse\n\t\t{\n\t\t\triding_snapshot(state, &step->vehicle);\n"
                                "\t\t\tvehicle_riding_point_to_local(&step->vehicle.position, &step->vehicle.forward, "
                                "&step->vehicle.up, new_base, &state->local_point);\n\t\t}\n"), "local-frame"),
    "falling-only-while-riding": (("if (vehicle_riding_is_riding(state) || state->falling)",
                                   "if (vehicle_riding_is_riding(state))"), "falling"),
    "falling-whole-window": (("if (vehicle_riding_is_riding(state) || state->falling)",
                              "if (state->ticks > 0 || state->falling)"), "falling"),
    "jump-world": (("\t\t*jump_frame = state->velocity;\n", "\n"), "jump-deck-frame"),
    "centre-velocity": (("cross_product3d(&vehicle->angular_velocity, &offset, &turning);",
                         "turning.i = turning.j = turning.k = 0.f;"), "contact-record"),
    "window-for-every-vehicle": (("\tif (state->object_index != vehicle_index)\n", "\tif (FALSE)\n"), "collision-others"),
    "window-where-not-applied": (("\tif (!applies || state->ticks <= 0)\n", "\tif (state->ticks <= 0)\n"),
                                 "collision-others"),
    "second-vehicle-as-pedestrian": (("return vehicle_riding_is_riding(state) ? _vehicle_riding_collision_judge : _vehicle_riding_collision_original;",
                                      "return _vehicle_riding_collision_original;"), "collision-others"),
    "claim-ignored": (("\tif (riding_object_index == NONE || best < 0 || objects[best] == riding_object_index)\n\t\treturn best;\n",
                       "\treturn best;\n"), "claim"),
    "claim-takes-walls": (("objects[index] == riding_object_index && normal_ks[index] >= minimum_normal_k",
                           "objects[index] == riding_object_index"), "claim"),
    "claim-strict-limit": (("normal_ks[index] >= minimum_normal_k", "normal_ks[index] > minimum_normal_k"), "claim"),
    "no-factor-two": (("return 2.f * vehicle_riding_run_jump_speed(", "return vehicle_riding_run_jump_speed("), "damage-speed-default"),
    "per-second": (("/TICKS_PER_SECOND;", ";"), "damage-speed-default"),
    "node-gate-world-speed": (("distance3d(carried, &carried_body)/step->player_ticks",
                                "distance3d(last, carried)/step->player_ticks"),
                              "turret-with-travel"),
    "node-gate-removed": (("distance3d(carried, &carried_body)/step->player_ticks > node_speed_limit",
                            "(distance3d(carried, &carried_body)/step->player_ticks > node_speed_limit && FALSE)"),
                          "turret-past-limit"),
    "node-gate-inclusive": (("step->player_ticks > node_speed_limit)", "step->player_ticks >= node_speed_limit)"),
                            "limit-edge"),
    "record-keeps-rejected-node": (("if (step && step->node_rejected && support->object_index == state->object_index &&",
                                    "if (FALSE && step && step->node_rejected && support->object_index == state->object_index &&"),
                                   "leg-stomp"),
    "rejected-velocity-from-node": (("scale_vector3d(&motion, per_tick, &step->velocity_now);",
                                     "{ real_point3d node_point; vehicle_riding_point_to_world(&vehicle->position, "
                                     "&vehicle->forward, &vehicle->up, &state->local_point, &node_point); "
                                      "vector_from_points3d(&corrected_last, &node_point, &step->velocity_now); "
                                     "scale_vector3d(&step->velocity_now, per_tick, &step->velocity_now); }"),
                                    "leg-stomp"),
    # (with no node the body's two frames are one, so the judgement is inert: the fault also measures from standing still)
    "body-judged": (("if (!vehicle->node_kept && state->node_index != NONE && !role_changed && (followed ? elapsed > 0 : step->updates > 0))",
                     "if (step->updates > 0 && (state->node_index != NONE || "
                     "(state->body_position = vehicle->origin, TRUE)))"), "body-never-judged"),
    "take-off-node-velocity": (("&step->velocity_now : &step->body_deck_velocity;",
                                "&step->velocity_now : &step->velocity_now;"), "take-off-body"),
    "take-off-keeps-node": (("\t\t\triding_anchor_body(state, &step->vehicle, new_base);\n",
                             "\t\t\triding_snapshot(state, &step->vehicle);\n"), "take-off-body"),
    "body-velocity-from-node": (("vector_from_points3d(&corrected_base, &body_moved_base, &step->body_deck_velocity);",
                                  "vector_from_points3d(&corrected_base, &moved_base, &step->body_deck_velocity);"), "take-off-body"),
    "anchor-keeps-node": (("\triding_snapshot(state, &body);\n\tstate->node_index = NONE;\n",
                           "\triding_snapshot(state, &body);\n"), "drop-opens-window"),
    "anchor-at-node": (("\tbody->position = vehicle->origin;\n", "\tbody->position = vehicle->position;\n"),
                       "drop-opens-window"),
    "moved-alone-clears": (("riding_moved_alone)\n\t{\n\t\triding_open_window(state, vehicle, base);",
                            "riding_moved_alone)\n\t{\n\t\tvehicle_riding_drop(state);"), "drop-opens-window"),
    "teleport-clears": (("\t\tif (riding)\n\t\t\triding_open_window(state, vehicle, base);\n",
                         "\t\tif (FALSE)\n\t\t\triding_open_window(state, vehicle, base);\n"), "drop-opens-window"),
    "window-not-falling": (("\tstate->ticks = RIDING_TICKS - 1;\n\tstate->falling = 1;\n",
                            "\tstate->ticks = RIDING_TICKS - 1;\n"), "drop-opens-window"),
    "gone-rider-not-falling": (("\t\tif (riding)\n\t\t\tstate->falling = 1;\n", ""), "drop-opens-window"),
    "grip-unlimited": (("\tif (length > limit)\n", "\tif (FALSE)\n"), "grip-limit"),
    "grip-per-component": (("\t\tscale_vector3d(&change, limit/length, &change);\n",
                            "\t\tchange.i = change.i > limit ? limit : change.i < -limit ? -limit : change.i;\n"
                            "\t\tchange.j = change.j > limit ? limit : change.j < -limit ? -limit : change.j;\n"),
                           "grip-limit"),
    "player-ticks-no-period": (("step->player_ticks = step->updates*riding_period(vehicle, double_speed);",
                                "step->player_ticks = step->updates;"), "grip-limit"),
    "player-not-given-change": (("\tvelocity->i += change.i;\n\tvelocity->j += change.j;\n", ""), "grip-limit"),
    "frame-k-frozen": (("\tstate->velocity.k = step->tracked_deck_velocity.k;\n", ""), "grip-limit"),
    "grip-nan-kept": (("\tif (!(riding_air_grip >= 0.f))\n", "\tif (riding_air_grip < 0.f)\n"), "grip-limit"),
    "grip-negative-kept": (("\tif (!(riding_air_grip >= 0.f))\n", "\tif (isnan(riding_air_grip))\n"), "grip-limit"),
    "grip-on-ground": (("\tif (!airborne)\n\t\treturn;\n", ""), "grip-standing"),
    "air-frame-off-deck": ((" || !step->over_deck)\n\t\treturn;\n", ")\n\t\treturn;\n"), "off-edge"),
    "reach-no-jump": (("return jump_velocity*jump_velocity/(2.f*gravity) + height;", "return height;"), "off-edge"),
    "reach-ignored": (("hit && distance <= reach && ", "hit && "), "off-edge"),
    "upright-ignored": ((" && body_up->k >= RIDING_UPRIGHT_K;", ";"), "tilted"),
    "hold-ignored": (("\t\tif ((started_riding || (supported && !step->over_deck)) && --state->ticks <= 0)\n",
                      "\t\tif (--state->ticks <= 0)\n"), "window-holds-in-air"),
    "hold-everywhere": (("\t\tif ((started_riding || (supported && !step->over_deck)) && --state->ticks <= 0)\n",
                         "\t\tif (started_riding && --state->ticks <= 0)\n"), "window-holds-in-air"),
    "take-off-held": (("\t\tif ((started_riding || (supported && !step->over_deck)) && --state->ticks <= 0)\n",
                       "\t\tif ((supported && !step->over_deck) && --state->ticks <= 0)\n"), "window-holds-in-air"),
    "hold-supported-over-deck": (("\t\tif ((started_riding || (supported && !step->over_deck)) && --state->ticks <= 0)\n",
                                  "\t\tif ((started_riding || supported) && --state->ticks <= 0)\n"), "grip-standing"),
    "no-falling-on-leaving-support": (("else if (started_riding || (step && step->over_deck))",
                                       "else if (started_riding)"), "grip-standing"),
    "turn-in-window": (("\tif (step && vehicle_riding_is_riding(state))\n", "\tif (step)\n"), "turn-relative-velocity"),
    "turn-reversed": (("vehicle_riding_turn_vector(velocity, step->dyaw);",
                       "vehicle_riding_turn_vector(velocity, -step->dyaw);"), "turn-relative-velocity"),
    "over-deck-not-reset": (("\tstep->over_deck = FALSE;\n", "\tstep->over_deck = TRUE;\n"), "off-edge"),
    "skipped-body-not-held": (("\t\tstep->body_deck_velocity = state->velocity;\n",
                               "\t\tstep->body_deck_velocity = step->displacement;\n"), "skipped-tick-held"),
    "air-control-world": (("\t\tdesired_velocity.i += physics->air_frame_velocity.i;\n", "", "source/units/bipeds.c"),
                          "air-control-relative"),
    "air-control-world-j": (("\t\tdesired_velocity.j += physics->air_frame_velocity.j;\n", "", "source/units/bipeds.c"),
                            "air-control-relative"),
    "closing-magnitude": (("closing = dot_product3d(deck_velocity, &direction) - dot_product3d(biped_velocity, &across);",
                           "{ real_vector3d relative; subtract_vectors3d(deck_velocity, biped_velocity, &relative); "
                           "closing = magnitude3d(&relative); }"), "contact-record"),
    "closing-inclusive": (("\tif (closing > damage_speed || ", "\tif (closing >= damage_speed || "), "contact-record"),
    "closing-sign": (("closing = dot_product3d(deck_velocity, &direction) - dot_product3d(biped_velocity, &across);",
                      "closing = dot_product3d(biped_velocity, &across) - dot_product3d(deck_velocity, &direction);"),
                     "contact-record"),
    "closing-biped-3d": (("closing = dot_product3d(deck_velocity, &direction) - dot_product3d(biped_velocity, &across);",
                          "closing = dot_product3d(deck_velocity, &direction) - dot_product3d(biped_velocity, &direction);"),
                         "contact-record"),
    "match-vertical": (("\tacross.k = 0.f;\n", "\tacross.k = direction.k;\n"), "contact-record"),
    "carry-deck-per-update": (("scale_vector3d(velocity, 1.f/riding_period(vehicle, double_speed), velocity);", ""),
                              "contact-sequence"),
    "node-shift-ignored": (("step->node_shift > RIDING_NODE_SHIFT || ", ""), "node-turret-test"),
    "node-shift-inclusive": (("step->node_shift > RIDING_NODE_SHIFT", "step->node_shift >= RIDING_NODE_SHIFT"),
                             "node-turret-test"),
    "node-tilt-ignored": ((" || step->node_tilt > RIDING_NODE_TILT", ""), "node-turret-test"),
    "node-shift-vertical": (("*shift = magnitude3d(&moved);", "*shift = moved.k > 0.f ? moved.k : -moved.k;"),
                            "node-turret-test"),
    "node-shift-signed": (("*shift = magnitude3d(&moved);", "*shift = moved.k < 0.f ? 0.f : magnitude3d(&moved);"),
                          "node-turret-test"),
    "node-shift-world": (("&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, &vehicle->position,",
                          "&state->body_position, &state->body_forward, &state->body_up, &vehicle->position,"),
                         "node-turret-test"),
    "node-shift-per-update": (("\tstep->node_shift /= step->player_ticks;\n", ""), "node-turret-test"),
    "slow-limited": (("\t\tscale_vector3d(&way, along, &slowed);\n\t\tsubtract_vectors3d(&change, &slowed, &change);\n", ""),
                     "air-frame-asymmetric"),
    "ahead-followed": (("\telse if (air->enclosure != _vehicle_riding_enclosure_enclosed)\n",
                        "\telse if (FALSE)\n"), "air-frame-asymmetric"),
    "enclosed-ignored": (("\telse if (air->enclosure != _vehicle_riding_enclosure_enclosed)\n",
                          "\telse if (TRUE)\n"), "air-frame-asymmetric"),
    "no-peak": (("peak = air->have_peak ? air->peak : speed;", "peak = speed;"), "air-frame-asymmetric"),
    "peak-not-kept": (("\tif (!air->host_peak)\n\t\tair->peak = speed > peak ? speed : peak;\n\tair->have_peak = TRUE;\n",
                       "\tif (!air->host_peak)\n\t\tair->peak = speed > peak ? speed : peak;\n"), "air-frame-asymmetric"),
    "way-from-deck-missing": (("\t\tway.i = step->tracked_deck_velocity.i;\n\t\tway.j = step->tracked_deck_velocity.j;\n",
                               "\t\tway.i = 0.f;\n\t\tway.j = 0.f;\n"), "air-frame-asymmetric"),
    "still-threshold": (("\tif (!(speed >= RIDING_AIR_STILL))\n", "\tif (!(speed > 0.f))\n"), "air-frame-asymmetric"),
    "enclosure-reach": (("(hit && distance <= jump_height)", "hit"), "enclosure-ray"),
    "enclosure-inside-ignored": (("inside || (hit", "(hit"), "enclosure-ray"),
    "enclosure-recast": (("\treturn air->enclosure == _vehicle_riding_enclosure_unknown;\n", "\treturn TRUE;\n"),
                         "enclosure-ray"),
    "enclosure-not-reset": (("\t\tair->enclosure = _vehicle_riding_enclosure_unknown;\n\t\treturn FALSE;\n",
                             "\t\treturn FALSE;\n"), "enclosure-ray"),
    "air-kept-riding": (("\tif (!step || vehicle_riding_is_riding(state))\n\t{\n\t\triding_air_clear(air);\n",
                         "\tif (!step || vehicle_riding_is_riding(state))\n\t{\n"), "enclosure-ray"),
    "air-latch-kept": (("\t\triding_air_clear(&riding_airs[player_index]);\n", ""), "enclosure-ray"),
    "air-one-player": (("\treturn &riding_airs[player_index];\n", "\treturn &riding_airs[0];\n"),
                       "enclosure-ray"),
    "under-while-not-over": ((" && over_deck)\n", ")\n"), "contact-record"),
    "under-for-every-vehicle": (("state->object_index == vehicle_index && over_deck", "over_deck"), "contact-record"),
    "normal-closing-unchecked": ((" || normal_closing > damage_speed)", " || (normal_closing > damage_speed && FALSE))"), "contact-record"),
    "normal-ignored": (("*normal = contact_normal ? *contact_normal : direction;", "*normal = direction;"),
                       "contact-record"),
    "take-keeps": (("\triding_contact_counts[player_index] = 0;\n\treturn count;\n", "\treturn count;\n"),
                   "record-lifetime"),
    "record-appends-same": (("\t\tif (riding_contacts[player_index][index].vehicle_index == vehicle_index)\n",
                             "\t\tif (FALSE)\n"), "record-lifetime"),
    "record-fifth-replaces": (("\tif (riding_contact_counts[player_index] >= RIDING_CONTACT_RECORDS)\n\t\treturn FALSE;\n",
                               "\tif (riding_contact_counts[player_index] >= RIDING_CONTACT_RECORDS)\n\t{\n"
                               "\t\triding_contacts[player_index][RIDING_CONTACT_RECORDS - 1].vehicle_index = vehicle_index;\n"
                               "\t\treturn TRUE;\n\t}\n"), "record-lifetime"),
    "air-local-bound": (("player_index >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)\n\t\treturn NULL;",
                         "player_index >= MAXIMUM_LOCAL_PLAYERS)\n\t\treturn NULL;"), "player-keyed"),
    "record-local-bound": (("player_index >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)\n\t\treturn FALSE;",
                            "player_index >= MAXIMUM_LOCAL_PLAYERS)\n\t\treturn FALSE;"), "player-keyed"),
    "take-local-bound": (("player_index >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)\n\t\treturn 0;",
                          "player_index >= MAXIMUM_LOCAL_PLAYERS)\n\t\treturn 0;"), "player-keyed"),
    "latch-clears-local-airs": (("player_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; player_index++",
                                 "player_index < MAXIMUM_LOCAL_PLAYERS; player_index++"), "player-keyed"),
    "clear-local-only": (("player < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; player++",
                          "player < MAXIMUM_LOCAL_PLAYERS; player++"), "player-keyed"),
    "clear-skipped": (("\t\triding_contact_counts[player] = 0;\n", ""), "record-lifetime"),
    "defer-never-marked": (("\triding_contacts[player_index][index].deferred = TRUE;\n", ""), "record-lifetime"),
    "defer-kept-on-new-record": (("\triding_contacts[player_index][index].deferred = FALSE;\n", ""), "record-lifetime"),
    "latch-keeps-contacts": (("\tvehicle_riding_contacts_clear();\n", ""), "record-lifetime"),
    "contact-absolute": (("\tsubtract_vectors3d(&motion, &own, &motion);\n", ""), "contact-sequence"),
    "contact-own-fall": (("\t\town.k = 0.f;\n", "\t\town.k = biped_velocity->k*ticks;\n"), "contact-sequence"),
    "contact-pulls": (("\tif (into < 0.f)\n\t\tinto = 0.f;\n", ""), "contact-sequence"),
    "contact-tracked-world": (("\tif (step && vehicle->object_index == step->vehicle.object_index)\n", "\tif (FALSE)\n"),
                              "contact-sequence"),
    "contact-per-tick": (("\t\tticks = riding_period(vehicle, double_speed);\n", "\t\tticks = 1.f;\n"),
                         "contact-sequence"),
    "contact-rider-own": (("\tif (step && carried)\n", "\tif (FALSE)\n"), "contact-sequence"),
    "contact-ahead-lost": (("\t\t*ahead = own;\n", ""), "contact-sequence"),
    "settle-ignores-touch": (("\tif (!touching)\n\t\treturn _vehicle_riding_settle_moved;\n", ""), "contact-crush"),
    "settle-always-fix": (("\tif (!candidate_touching)\n\t\treturn _vehicle_riding_settle_candidate;\n", ""),
                          "contact-crush"),
    "settle-no-crush": (("fixed ? _vehicle_riding_settle_fixed : _vehicle_riding_settle_crushed",
                         "_vehicle_riding_settle_fixed"), "contact-crush"),
    "peak-kept-landed": (("\t\t\tair->have_peak = FALSE;\n", ""), "enclosure-ray"),
    "deck-frame-live": (('if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)', "if (FALSE)"), "deck-frame-carried"),
    "deck-frame-in-air": (('if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)', "if (state->ticks > 0 && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)"), "deck-frame-air"),
    "deck-frame-any-node": (('if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)', "if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index)"), "deck-frame-other-node"),
    "deck-frame-any-vehicle": (('if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)', "if (vehicle_riding_is_riding(state) && state->node_index == vehicle->node_index)"), "deck-frame-other-node"),
    "deck-other-vehicle": (("vehicle->object_index != state->object_index || ", ""), "deck-make"),
    "deck-other-node": ((" || vehicle->node_index != state->node_index)\n\t\treturn FALSE;",
                         ")\n\t\treturn FALSE;"), "deck-make"),
    "deck-stored-place": (("vehicle_riding_point_to_local(&vehicle->position, &vehicle->forward, &vehicle->up, base, "
                           "&made.place);", "made.place = state->local_point;"), "deck-make"),
    "deck-relative-world": (("subtract_vectors3d(world_velocity, &state->velocity, &made.velocity);",
                             "made.velocity = *world_velocity;"), "deck-make"),
    "deck-air-world": (("\t\tmade.velocity = state->velocity;\n", "\t\tmade.velocity = *world_velocity;\n"),
                       "deck-make"),
    "deck-falling-dropped": (("made.flags = state->falling ? FLAG(_vehicle_riding_deck_falling_bit) : 0;",
                              "made.flags = 0;"), "deck-make"),
    "deck-verdict-dropped": (("\tmade.flags |= (byte)(vehicle_riding_cabin_verdict(cabin, state, air, FALSE) << VEHICLE_RIDING_DECK_VERDICT_SHIFT);\n",
                              ""), "deck-make"),
    "verdict-air-only": (("\tmade.flags |= (byte)(vehicle_riding_cabin_verdict(cabin, state, air, FALSE) << VEHICLE_RIDING_DECK_VERDICT_SHIFT);\n",
                          "\tif (!vehicle_riding_is_riding(state))\n\t\tmade.flags |= (byte)(vehicle_riding_cabin_verdict(cabin, state, air, FALSE) << VEHICLE_RIDING_DECK_VERDICT_SHIFT);\n"),
                         "deck-make-cabin"),
    "verdict-cabin-evidence-only": (("vehicle_riding_cabin_verdict(cabin, state, air, FALSE) <<",
                                     "vehicle_riding_cabin_verdict(cabin, state, air, TRUE) <<"), "deck-make-cabin"),
    "deck-peak-unknown-zero": (("made.peak = magnitude3d(&way);", "made.peak = 0.f;"), "deck-make"),
    "deck-peak-kept-dropped": (("\t\t\tmade.peak = air->peak;\n", "\t\t\t;\n"), "deck-make"),
    "deck-partial-output": (("\tif (!vehicle_riding_deck_values_valid(&made))\n\t\treturn FALSE;\n\t*deck = made;\n",
                             "\t*deck = made;\n\tif (!vehicle_riding_deck_values_valid(&made))\n\t\treturn FALSE;\n"),
                            "deck-make"),
    "deck-zero-ticks": (("deck->ticks >= 1 &&", "deck->ticks >= 0 &&"), "deck-values"),
    "deck-over-window": (("deck->ticks <= RIDING_TICKS &&", "deck->ticks <= 127 &&"), "deck-values"),
    "deck-verdict-three": (("<= _vehicle_riding_enclosure_enclosed &&", "<= 3 &&"), "deck-values"),
    "deck-place-unchecked": (("valid_real_point3d(&deck->place) &&", "TRUE &&"), "deck-values"),
    "deck-velocity-unchecked": (("valid_real_vector3d(&deck->velocity) &&", "TRUE &&"), "deck-values"),
    "deck-peak-unchecked": (("valid_real(deck->peak) &&", "TRUE &&"), "deck-values"),
    "deck-peak-negative": (("deck->peak >= 0.f;", "TRUE;"), "deck-values"),
    "deck-salt-zero": (("((unsigned long)vehicle_index >> 16) != 0", "TRUE"), "deck-values"),
    "deck-none-named": (("return vehicle_index != NONE &&", "return TRUE &&"), "deck-values"),
    "deck-unresolved": (("return node_count != NONE &&", "return TRUE &&"), "deck-node"),
    "deck-node-past": (("deck->node_index < node_count", "deck->node_index <= node_count"), "deck-node"),
    "deck-node-negative": (("(deck->node_index >= 0 &&", "(TRUE &&"), "deck-node"),
    "deck-node-values-skipped": (("vehicle_riding_deck_values_valid(deck) &&\n", "TRUE &&\n"), "deck-node"),
    "threshold-m-only": (("real limit = minimum_normal_k > RIDING_UPRIGHT_K ? minimum_normal_k : RIDING_UPRIGHT_K;",
                          "real limit = minimum_normal_k;"), "cabin-tilt"),
    "pitch-only-tilt": (("return length > 0.f ? body_up->k/length : 1.f;",
                         "return length > 0.f ? (real)sqrt(1.f - body_up->i*body_up->i/(length*length)) : 1.f;"), "cabin-tilt"),
    "tilt-equality-protected": (("return vehicle_riding_cabin_cosine(body_up) < limit;",
                                 "return vehicle_riding_cabin_cosine(body_up) <= limit;"), "cabin-tilt"),
    "exception-ignores-enclosure": (("return eligible && valid_window && enclosed &&", "return eligible && valid_window &&"),
                                    "cabin-tilt"),
    "world-up-always": (("\tif (k < minimum_normal_k)\n\t\tcabin->body_axis = TRUE;\n",
                         "\tif (FALSE)\n\t\tcabin->body_axis = TRUE;\n"), "axis-hysteresis"),
    "axis-flips-at-m": (("else if (cabin->body_axis && k >= back)", "else if (cabin->body_axis && k >= minimum_normal_k)"),
                        "axis-hysteresis"),
    "axis-band-past-one": (("real back = minimum_normal_k + RIDING_CABIN_AXIS_BAND < 1.f ? minimum_normal_k + RIDING_CABIN_AXIS_BAND : 1.f;",
                            "real back = minimum_normal_k + RIDING_CABIN_AXIS_BAND;"), "axis-hysteresis"),
    "node-up-for-body": (("return &step->vehicle.body_up;", "return &step->vehicle.up;"), "axis-hysteresis"),
    "inverted-ends-unswapped": (("\tif (axis->k < 0.f)\n", "\tif (FALSE)\n"), "axis-hysteresis"),
    "hit-ceiling-only": (("return ceiling && rays->floor && rays->floor_distance <= jump_height + height;", "return ceiling;"),
                         "cabin-verdict"),
    "hit-inside-skips-floor": (("return ceiling && rays->floor && rays->floor_distance <= jump_height + height;",
                                "return rays->inside || (ceiling && rays->floor && rays->floor_distance <= jump_height + height);"),
                               "cabin-verdict"),
    "hit-floor-unbounded": ((" && rays->floor_distance <= jump_height + height;", ";"), "cabin-verdict"),
    "hit-zero-jump-ray": (("(jump_height > 0.f && rays->ceiling &&", "(rays->ceiling &&"), "cabin-verdict"),
    "grace-refresh-on-miss": (("\t\tcabin->misses++;\n", "\t\tcabin->misses = 0;\n"), "grace-jump"),
    "grace-reset-on-jump": (("\tcabin->jump = hit ? _vehicle_riding_enclosure_enclosed : _vehicle_riding_enclosure_open;\n",
                             "\tcabin->misses = 0;\n\tcabin->jump = hit ? _vehicle_riding_enclosure_enclosed : _vehicle_riding_enclosure_open;\n"),
                            "grace-jump"),
    "age-per-vehicle-update": (("\tif (!step || state->ticks <= 0", "\tif (!step || step->updates == 0 || state->ticks <= 0"),
                               "grace-jump"),
    "freeze-grace-as-air-verdict": (("cabin->jump = hit ?", "cabin->jump = hit || cabin->standing == _vehicle_riding_enclosure_enclosed ?"),
                                    "grace-jump"),
    "standing-grace-overrides-shallow-air": (("\t\tif (vehicle_riding_is_riding(state) && !cabin->take_off)\n\t\t\treturn cabin->standing;\n",
                                              "\t\tif (vehicle_riding_is_riding(state) || cabin->jump == _vehicle_riding_enclosure_unknown)\n\t\t\treturn cabin->standing;\n"),
                                             "grace-jump"),
    "legacy-air-becomes-cabin": (("if (cabin_evidence || !air)", "if (!air)"), "grace-jump"),
    "jump-resampled": (("if (tilted && cabin->jump == _vehicle_riding_enclosure_unknown &&", "if (tilted &&"), "grace-jump"),
    "standing-on-take-off": (("\tif (vehicle_riding_is_riding(state) && !cabin->take_off)\n\t\treturn _vehicle_riding_cabin_sample_standing;\n",
                              "\tif (vehicle_riding_is_riding(state))\n\t\treturn _vehicle_riding_cabin_sample_standing;\n"),
                             "grace-jump"),
    "cabin-kept-across-vehicles": (("if (!window || cabin->unit_index != unit_index || cabin->vehicle_index != state->object_index)",
                                    "if (!window || cabin->unit_index != unit_index)"), "cabin-keys"),
    "cabin-kept-across-units": (("if (!window || cabin->unit_index != unit_index ||", "if (!window ||"), "cabin-keys"),
    "cabin-latch-kept": (("\t\tvehicle_riding_cabin_clear(&riding_cabins[player_index]);\n", ""), "cabin-keys"),
    "cabin-local-bound": (("player_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS))\n\t\treturn NULL;\n\treturn &riding_cabins",
                           "player_index < MAXIMUM_LOCAL_PLAYERS))\n\t\treturn NULL;\n\treturn &riding_cabins"), "cabin-keys"),
    "take-off-in-window": (("if (cabin && vehicle_riding_is_riding(state) && cabin->unit_index == unit_index",
                            "if (cabin && cabin->unit_index == unit_index"), "cabin-keys"),
    "acquire-every-tick": (("\treturn cabin->standing == _vehicle_riding_enclosure_unknown;\n", "\treturn TRUE;\n"),
                           "cabin-keys"),
    "grace-narrows-early": (('grace = config_real("debug.riding_enclosure_grace_ticks");',
                             'grace = (real)config_real("debug.riding_enclosure_grace_ticks");'), "grace-setting"),
    "grace-nan-kept": (("\tif (!(grace == grace && grace - grace == 0.))\n", "\tif (FALSE)\n"), "grace-setting"),
    "grace-unclamped": (("\telse if (grace >= (double)RIDING_CABIN_GRACE_MAXIMUM)\n\t\triding_cabin_grace = RIDING_CABIN_GRACE_MAXIMUM;\n", ""),
                        "grace-setting"),
    "grace-rounded": (("riding_cabin_grace = (short)grace;", "riding_cabin_grace = (short)(grace + .5f);"), "grace-setting"),
    "no-anchor-recovers": (("return check->touching ? _vehicle_riding_cabin_settle_failed : _vehicle_riding_cabin_settle_none;",
                            "return _vehicle_riding_cabin_settle_none;"), "cabin-recovery-rules"),
    "overlap-only-return": (("\telse if (check->crosses)\n", "\telse if (check->crosses && check->touching)\n"),
                            "cabin-recovery-rules"),
    "return-through-door": (("\telse if (check->crosses)\n", "\telse if (TRUE)\n"), "cabin-recovery-rules"),
    "return-limited-two-widths": (("\t\tif (check->anchor_valid)\n\t\t\treturn _vehicle_riding_cabin_settle_anchor;\n",
                                   "\t\tif (check->anchor_valid && check->anchor_distance <= 2.f*check->width)\n"
                                   "\t\t\treturn _vehicle_riding_cabin_settle_anchor;\n"), "cabin-recovery-rules"),
    "overlap-anchor-unbounded": (("if (check->anchor_valid && check->anchor_distance <= 2.f*check->width)",
                                  "if (check->anchor_valid)"), "cabin-recovery-rules"),
    "door-keeps-anchor": (("\t\tcabin->have_anchor = FALSE;\n\t\treturn;\n", "\t\treturn;\n"), "cabin-recovery-rules"),
    "failure-moves-anchor": (("\tif (settle == _vehicle_riding_cabin_settle_failed)\n\t\treturn;\n", ""),
                             "cabin-recovery-rules"),
    "defer-other-vehicle": (("state->object_index != vehicle_index || cabin->unit_index != unit_index ||\n\t\tcabin->vehicle_index != vehicle_index)",
                             "cabin->unit_index != unit_index)"), "cabin-recovery-rules"),
    "defer-without-history": (("return exception || cabin->have_anchor;", "return exception;"), "cabin-recovery-rules"),
    "same-vehicle-still-ignore": (("return defers && decision != _vehicle_riding_collision_original ? _vehicle_riding_collision_resolved : decision;",
                                   "return decision;"), "cabin-recovery-rules"),
    "watched-without-evidence": ((" &&\n\t\t(cabin->exception || cabin->have_anchor || cabin->standing == _vehicle_riding_enclosure_enclosed ||\n"
                                  "\t\tcabin->jump == _vehicle_riding_enclosure_enclosed);", ";"), "cabin-recovery-rules"),
    "followed-counts-updates": (("step->player_ticks = (real)elapsed;", "step->player_ticks = (real)step->updates;"),
                                "followed-elapsed"),
    "elapsed-zero-divides": (("held = elapsed == 0 || magnitude3d(&off_path) > RIDING_SNAP_STEP;",
                              "held = magnitude3d(&off_path) > RIDING_SNAP_STEP;"), "followed-elapsed-zero"),
    "snap-refused": (("held = elapsed == 0 || magnitude3d(&off_path) > RIDING_SNAP_STEP;",
                      "held = elapsed == 0;\n\t\t\trefused = magnitude3d(&off_path) > RIDING_SNAP_STEP;"),
                     "followed-snap"),
    "snap-as-motion": (("held = elapsed == 0 || magnitude3d(&off_path) > RIDING_SNAP_STEP;", "held = elapsed == 0;"),
                       "followed-snap"),
    "snap-by-teleport-step": (("held = elapsed == 0 || magnitude3d(&off_path) > RIDING_SNAP_STEP", "held = elapsed == 0 || magnitude3d(&off_path) > riding_teleport_step"),
                              "followed-snap"),
    "role-change-velocities": (("held = !refused;", "held = FALSE;"), "followed-role-change"),
    "role-change-no-teleport-guard": (("refused = distance > riding_teleport_step;", "refused = FALSE;"),
                                      "followed-role-change-teleport"),
    "role-change-guard-scaled": (("refused = distance > riding_teleport_step;",
                                  "refused = distance > riding_teleport_step*(vehicle->time - state->mark.time);"),
                                 "followed-role-change-teleport"),
    "node-judged-at-zero": (("(followed ? elapsed > 0 : step->updates > 0)", "(followed ? TRUE : step->updates > 0)"),
                            "followed-node"),
    "since-same-not-bitwise": (("if (now->rotation.v.i == then->rotation.v.i &&", "if (FALSE &&"), "correction-since-same"),
    "window-base-keeps-writes": (("if (vehicle->role != _vehicle_riding_role_followed)\n\t{\n\t\tvehicle_riding_correction_since",
                                  "if (FALSE)\n\t{\n\t\tvehicle_riding_correction_since"), "window-base-write-left-out"),
    "window-base-role-ignored": (("\tif (vehicle->role != state->mark.role)\n\t\treturn;\n", ""), "window-base-role-change"),
    "window-base-snap-carried": (("if (magnitude3d(&off_path) > RIDING_SNAP_STEP)\n\t\t\treturn;", "if (FALSE)\n\t\t\treturn;"),
                                 "window-base-followed"),
    "window-base-no-time-carried": (("if (elapsed <= 0)\n\t\t\treturn;", "if (elapsed < 0)\n\t\t\treturn;"),
                                    "window-base-followed"),
    "window-base-translation-only": (("vehicle_riding_point_to_world(&vehicle->position, &vehicle->forward, &vehicle->up, &local_base, &moved_base);\n\tcorrected_base = *base;",
                                      "moved_base.x = local_base.x + vehicle->position.x;\n\tmoved_base.y = local_base.y + vehicle->position.y;\n\tmoved_base.z = local_base.z + vehicle->position.z;\n\tcorrected_base = *base;"),
                                     "window-base-matches-step"),
    "window-base-any-vehicle": (("if (!vehicle || vehicle->object_index != state->object_index)\n\t\treturn;", "if (!vehicle)\n\t\treturn;"),
                                "window-base-matches-step"),
}


NEGATIVE_CONTROLS.update({
    'map-any-script': (('hs_runtime_port_executing_script_type()!=_hs_script_startup', 'FALSE'), 'map-script-startup-only'),
    'map-late-call': (('game_time_get()!=0', 'FALSE'), 'map-script-startup-only'),
    'map-client-sets': (('game_connection()==_game_connection_network_client || !riding_map', '!riding_map', 'map_script'), 'map-client-own-call'),
    'map-ignored-local': (('\tif (map != NONE)\n\t\treturn map != FALSE;\n', '\tif (FALSE && map != NONE)\n\t\treturn map != FALSE;\n', 'enabled'), 'map-wins-local'),
    'map-ignored-network': (('\tif (map != NONE)\n\t\treturn map != FALSE;\n', '\tif (FALSE && map != NONE)\n\t\treturn map != FALSE;\n', 'enabled'), 'map-wins-network'),
    'client-own-slot': (('return riding_host_map;', 'return riding_map ? (short)*riding_map : NONE;'), 'map-wins-network'),
    'map-not-reset': (('*riding_map = NONE;', ';'), 'map-per-map'),
    'latch-ignores-map': (('return vehicle_riding_enabled();', 'return riding_local_decision;', 'latch'), 'map-latch'),
    'latch-local-only': (('return vehicle_riding_enabled();', 'return game_connection() == _game_connection_local && vehicle_riding_enabled();', 'latch'), 'map-restore-server'),
    'map-in-static': (('riding_map = (long *)game_state_malloc("vehicle riding map", NULL, sizeof(long));', '{ static long own; riding_map = &own; }'), 'map-persist'),
    'map-before-editor': (('\tif (game_in_editor())\n\t\treturn FALSE;\n', '\tif (vehicle_riding_map() != NONE)\n\t\treturn vehicle_riding_map() != FALSE;\n\tif (game_in_editor())\n\t\treturn FALSE;\n', 'enabled'), 'map-editor'),
    'log-every-call': (('if (!riding_map_refusal_logged)', 'if (TRUE)'), 'map-script-startup-only'),
    'global-ignored': (('*riding_map = map;', ';'), 'map-global'),
    'global-on-client': (('game_connection()==_game_connection_network_client || !riding_map', '!riding_map', 'map_start'), 'map-global-client'),
    'global-overrides-call': (('!riding_map)\n\t\treturn;\n\t/* the first tick', '(!riding_map || *riding_map != NONE))\n\t\treturn;\n\t/* the first tick'), 'map-global-then-call'),
    'latch-rereads-global': (('return vehicle_riding_enabled();', 'vehicle_riding_map_start();\n\treturn vehicle_riding_enabled();', 'latch'), 'map-global-set-later'),
    'start-or-only': (('SET_FLAG(game->cooperative_flags, _network_game_cooperative_vehicle_riding_bit, riding);', 'if (riding)\n\t\tgame->cooperative_flags |= FLAG(_network_game_cooperative_vehicle_riding_bit);', 'start'), 'seq-coop-then-silent-mp'),
    'start-or-only-coop': (('SET_FLAG(game->cooperative_flags, _network_game_cooperative_vehicle_riding_bit, riding);', 'if (riding)\n\t\tgame->cooperative_flags |= FLAG(_network_game_cooperative_vehicle_riding_bit);', 'start'), 'seq-riding-mp-then-coop-off'),
    'start-wrong-gametype-bit': (('_game_variant_vehicle_riding_bit),', '_game_variant_generic_starting_equipment_bit),', 'start'), 'seq-riding-mp-then-silent-mp'),
    'start-override-always': (('config_boolean("server.vehicle_riding_override_gametype") != 0', 'TRUE', 'start'), 'seq-dedicated-rotation'),
    'start-entry-rule-unread': (('\t\triding_entry_rule,\n', '\t\tNONE,\n', 'start'), 'seq-coop-entry-rule'),
    'override-map-skipped': (('\tif (map != NONE)\n\t\treturn map != FALSE;\n', '\tif (FALSE && map != NONE)\n\t\treturn map != FALSE;\n', 'enabled'), 'seq-override-base-off'),
    'decision-read-per-call': (("return connection == _game_connection_local ? riding_local_decision : riding_network_decision;", "return connection == _game_connection_local ? riding_local_decision : network_game_port_vehicle_riding();"), "map-decision-held"),
    'latch-decides': (('riding_look_carry = config_boolean("debug.riding_look_carry") != 0;', 'vehicle_riding_map_decide();\n\triding_look_carry = config_boolean("debug.riding_look_carry") != 0;', 'latch'), 'map-decision-held'),
    'local-never-multiplayer': (('boolean multiplayer = game_engine_running();', 'boolean multiplayer = FALSE;', 'map_decide'), 'local-gametype'),
    'client-reads-own': (("return connection == _game_connection_local ? riding_local_decision : riding_network_decision;", "return connection == _game_connection_local ? riding_local_decision : riding_local_decision;"), "seq-late-joiner"),
    'map-before-connection': (('\tif (connection != _game_connection_local && connection != _game_connection_network_server && connection != _game_connection_network_client)\n\t\treturn FALSE;\n\tmap = vehicle_riding_map();\n\tif (map != NONE)\n\t\treturn map != FALSE;\n',
                               '\tmap = vehicle_riding_map();\n\tif (map != NONE)\n\t\treturn map != FALSE;\n\tif (connection != _game_connection_local && connection != _game_connection_network_server && connection != _game_connection_network_client)\n\t\treturn FALSE;\n', 'enabled'), 'map-on-film'),
    'global-wrong-name': (('"vehicle_riding"', '"vehicle_ride"'), 'map-global'),
    'host-values-ignored': (('\triding_host_held = TRUE;\n\triding_take_host_values(host);', '\triding_host_held = TRUE;'), 'host-values-win'),
    'host-values-unlogged': (('error(_error_silent, "vehicle riding: values from the host', 'if (0) error(_error_silent, "vehicle riding: values from the host'),
                             'host-values-win'),
    'host-values-not-held': (('\triding_host_held = TRUE;', ''), 'host-values-win'),
    'host-values-never-forgotten': (('\triding_host_held = FALSE;\n}', '}'), 'host-values-win'),
    'host-held-from-start': (('static boolean riding_host_held;', 'static boolean riding_host_held = TRUE;'), 'host-no-message-keeps-own'),
    'host-values-taken-everywhere': (('if (game_connection() != _game_connection_network_client)\n\t\treturn;\n\triding_host_values', 'riding_host_values'),
                                     'host-local-unchanged'),
    'wire-air-grip-unchecked': (('if (valid_real(host->air_grip) && host->air_grip >= 0.f)\n\t\triding_air_grip = host->air_grip;',
                                 'riding_air_grip = host->air_grip;'), 'host-bad-value-clamped'),
    'wire-damage-speed-unchecked': (('riding_damage_speed = riding_checked(host->damage_speed, 0.f);',
                                     'riding_damage_speed = host->damage_speed;'), 'host-bad-value-clamped'),
    'wire-teleport-step-unchecked': (('riding_teleport_step = riding_checked(host->teleport_step, 4.f);',
                                      'riding_teleport_step = host->teleport_step;'), 'host-bad-value-clamped'),
    'wire-moved-alone-unchecked': (('riding_moved_alone = riding_checked(host->moved_alone, 0.25f);',
                                    'riding_moved_alone = host->moved_alone;'), 'host-bad-value-clamped'),
    'wire-grace-unclamped': (('riding_latch_grace((double)host->cabin_grace);', 'riding_cabin_grace = (short)host->cabin_grace;'),
                             'host-bad-value-clamped'),
    'latch-damage-speed-unchecked': (('riding_damage_speed = riding_checked((real)config_real("debug.riding_damage_speed"), 0.f);',
                                      'riding_damage_speed = (real)config_real("debug.riding_damage_speed");'), 'host-latch-clamped'),
    'latch-teleport-step-unchecked': (('riding_teleport_step = riding_checked((real)config_real("debug.riding_teleport_step"), 4.f);',
                                       'riding_teleport_step = (real)config_real("debug.riding_teleport_step");'), 'host-latch-clamped'),
    'latch-moved-alone-unchecked': (('riding_moved_alone = riding_checked((real)config_real("debug.riding_moved_alone"), 0.25f);',
                                     'riding_moved_alone = (real)config_real("debug.riding_moved_alone");'), 'host-latch-clamped'),
    'latched-reads-live-config': (('latched->air_grip = riding_air_grip;', 'latched->air_grip = (real)config_real("debug.riding_air_grip");'),
                                  'host-joiner-latched'),
})


def without_includes(text):
    return re.sub(r"^#include .*$", "", text, flags=re.M)


NL = chr(10)
REAL_MATH_INLINES = ["square_root", "arctangent", "set_real_point3d", "vector_from_points3d", "scale_vector3d",
                     "magnitude_squared3d", "magnitude3d", "normalize3d", "distance_squared3d", "distance3d",
                     "dot_product3d", "cross_product3d", "add_vectors3d", "subtract_vectors3d", "negate_vector3d",
                     "scale_vector2d", "magnitude_squared2d", "magnitude2d", "normalize2d", "point_from_line3d",
                      "valid_real", "valid_real_point3d", "valid_real_vector3d", "reciprocal_square_root", "set_real_quaternion"]
MATRIX_FUNCTIONS = ["matrix4x3_inverse", "matrix4x3_multiply", "matrix4x3_rotation_to_quaternion",
                    "matrix4x3_rotation_from_vectors", "matrix4x3_from_point_and_vectors", "matrix4x3_transform_point",
                    "matrix4x3_inverse_transform_point", "vector_from_matrices4x3", "matrix4x3_rotation_from_quaternion"]


def engine():
    """The engine's own vector and frame maths, extracted: the code under test calls it and does not copy it."""
    real_math = read("source/math/real_math.h")
    matrix_math = read("source/math/matrix_math.c")
    text = "".join(re.search(r"^#define " + name + r".*$", real_math, re.M).group(0) + NL
                   for name in ("_real_epsilon", "_pi"))
    for name in ("real_quaternion", "real_matrix4x3", "real_plane3d"):
        text += structure(real_math, name) + NL + f"typedef struct {name} {name};" + NL
    text += "".join(inline(real_math, name) + NL for name in REAL_MATH_INLINES)
    text += function(read("source/math/real_math.c"), "quaternion_to_angle_and_vector") + NL
    text += function(read("source/math/real_math.c"), "quaternion_normalize") + NL
    # (the 4x3 next-index table: one blob in the original, matrix_math.c)
    text += re.search(r"^static short data_0030790c.*$", matrix_math, re.M).group(0) + NL
    text += "#define matrix4x3_next (data_0030790c)" + NL
    text += "".join(function(matrix_math, name) + NL for name in MATRIX_FUNCTIONS)
    return text


AIRBORNE = "else if (TEST_FLAG(in_flags, _biped_physics_in_airborne_bit))"


def block_at(source, start):
    """The block that opens at source[start] (a "{"), braces included."""
    end = start + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def braced(source, head):
    """The block that follows a line of the sources, braces included; LookupError unless the line is there once."""
    if source.count(head) != 1:
        raise LookupError(f"not once in the sources ({source.count(head)} times): {head}")
    return block_at(source, source.index("{", source.index(head)))


def airborne(fault=None):
    """biped_update_physics's airborne branch (bipeds.c), as a function of the physics and the in flags."""
    bipeds = read("port/linux/game/vehicle_riding_bipeds.h") + read("source/units/bipeds.c")
    branch = braced(bipeds, AIRBORNE)
    if fault:
        branch = mutated(branch, *fault)
    return (enum_with(bipeds, "_biped_physics_in_slipping_bit") + NL
            + enum_with(bipeds, "_biped_physics_out_slipping_bit") + NL
            + "static void airborne_branch(struct biped_physics *physics, unsigned short in_flags)" + NL + branch + NL)


def ground():
    """biped_update_physics's ground branch (bipeds.c: the else after the airborne branch), as a function of the
    physics and the in flags, with the two locals of biped_update_physics it reads."""
    bipeds = read("port/linux/game/vehicle_riding_bipeds.h") + read("source/units/bipeds.c")
    braced(bipeds, AIRBORNE)
    start = bipeds.index("{", bipeds.index(AIRBORNE))
    rest = bipeds[start + len(block_at(bipeds, start)):]
    if not re.match(r"\s*else\s*\{", rest):
        raise LookupError("the airborne branch is no longer followed by the ground branch")
    return ("static void ground_branch(struct biped_physics *physics, unsigned short in_flags)" + NL + "{" + NL
            + "\treal_vector2d facing = { 0.f, 0.f };" + NL
            + "\tboolean climbing = TEST_FLAG(in_flags, _biped_physics_in_climb_anything_bit);" + NL
            + block_at(rest, rest.index("{")) + NL
            + "\t(void)facing;" + NL + "}" + NL)


def generated(fault=None):
    """The includes, with a fault in vehicle_riding.c, or (a fault naming a file third) in bipeds.c's airborne
    branch or in network_coop.c's network_game_port_vehicle_riding."""
    game = read("source/game/game.h")
    enums = enum_with(game, "_game_connection_local") + "\n"
    enums += enum_with(read('source/hs/hs_scenario_definitions.h'), '_hs_script_startup') + '\n'
    enums += enum_with(read('source/cseries/errors.h'), '_error_silent') + '\n'
    enums += (f"#define HALO_PORT_MAXIMUM_OBJECTS_PER_MAP "
              f"{constant(read('port/linux/include/halo_port_capacity.h'), 'HALO_PORT_MAXIMUM_OBJECTS_PER_MAP')}\n")
    enums += f"#define MAXIMUM_LOCAL_PLAYERS {constant(read('source/game/players.h'), 'MAXIMUM_LOCAL_PLAYERS')}\n"
    enums += (f"#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS "
              f"{constant(read('port/linux/include/halo_port_limits.h'), 'HALO_PORT_MAXIMUM_NETWORK_PLAYERS')}\n")
    enums += enum_with(read("source/networking/network_game_manager.h"),
                       "_network_game_cooperative_no_player_collisions_bit") + "\n"
    enums += enum_with(read("source/game/game_engine.h"), "_game_variant_draw_object_in_motion_sensor_bit = 0") + "\n"
    enums += enum_with(read("source/game/game_engine.h"), "game_engine_none = 0") + "\n"
    # (as the fixtures that use it define it themselves, so the two are one)
    enums += "#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= (unsigned)FLAG(bit)) : ((flags) &= ~(unsigned)FLAG(bit)))\n"
    code = read("port/linux/game/vehicle_riding.c")
    coop_source = read(COOP)
    start = NL.join((re.search(r"^static short riding_entry_rule = NONE;$", coop_source, re.M).group(0),
                     function(coop_source, "network_game_port_vehicle_riding_entry"),
                     function(coop_source, "network_game_port_vehicle_riding_start")))
    coop = function(coop_source, "network_game_port_vehicle_riding")
    branch_fault = None
    if fault and fault[0] == "define":
        enums += f"#define {fault[1]} 1\n"
        fault = None
    if fault and len(fault) == 3:
        if fault[2] in ('enabled', 'latch', 'map_start', 'map_script', 'map_decide'):
            name = 'vehicle_riding_' + fault[2]
            scope = function(code, name)
            code = code.replace(scope, mutated(scope, *fault[:2]), 1)
            fault = None
        elif fault[2] == COOP:
            coop, fault = mutated(coop, *fault[:2]), None
        elif fault[2] == 'start':
            start, fault = mutated(start, *fault[:2]), None
        else:
            branch_fault, fault = fault[:2], None
    if fault:
        code = mutated(code, *fault)
    text = (without_includes(read("port/linux/game/vehicle_riding.h")) + "\n" + without_includes(code) + "\n"
            + coop + "\n" + start + "\n" + function(read('port/linux/game/vehicle_riding_bipeds.c'), 'bipeds_riding_latch') + '\n')
    return (("enums.inc", enums), ("engine.inc", engine()), ("under_test.inc", text),
            ("airborne.inc", airborne(branch_fault)), ("ground.inc", ground()))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("vehicle_riding", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("vehicle_riding", generated(fault)), case)
    assert status == CHECK_FAILED and "check failed:" in output, f"vehicle_riding.c with a fault ({control}) passed '{case}': {output}"


if __name__ == "__main__":
    sys.exit(pytest.main(["-q", __file__]))
