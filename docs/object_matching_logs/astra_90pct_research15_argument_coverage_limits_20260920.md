> Research-only packet15: zero production change and zero exact credit. Validated production remains wave13.

# Witness coverage follow-up

**Complete and RELEASED.** `coverage_diagnostic.py` / `coverage-diagnostic.json` trace only the five existing witness windows. The latest executed scanner SHA is `5b36146f30d32fe1121b4171827f8691a0cc5222ba992d68162f4bf0298529cb`; the frozen initial scanner SHA is `3130bcc329be43200eef2fe4247b0c74abf5bbcb180193bb1da94de84a515982`. Provider inventories and bodies are actual COFF inputs, with hashes. No frame or callee-pop override was supplied.

| Witness | Latest observed result | Relevant unsupported summary |
|---|---|---|
| Purple output throttle | No argument facts; ESP unknown, EBP retained | Earliest relevant unsupported callee by caller offset is `_system_exit` at `+1e25` on both sides. Its provider is present and consists of a relocated `JMP _halt_and_catch_fire` (`e9 00000000`, relocation `+1`); there is no classified RET. This is an unsupported tail exit, not a missing provider or loop. |
| Green vector, `_code_000383d0+186` | TRUE, global vector address, 1.0f and green pointer all recovered | None; frame is known. |
| Blue vector, `_code_000383d0+1e7` | TRUE, global vector address, 1.0f and blue pointer all recovered | None; frame is known. |
| Circle, `_render_debug_circle+e3` | No argument facts; ESP unknown | `_system_exit` first appears at `+2b`, then `+52` and `+79`. The first changes ESP from known to unknown; modeled assertion fallthrough merges that uncertainty into the normal arm. |
| Matrix, `_render_debug_matrix+60` | Blue word4 preserved, overwritten size word3 unknown | None; the required overlap safeguard still passes. |

Purple has additional modeled routes through `_system_exit` at January `+2eab/+32e5/+3aff` and current `+2ec0/+32f8/+3b15`. `_unit_is_busy`, called at January `+3d6f` / current `+3d85`, is another present but unsupported provider: its `+29` instruction is `ff 24 8d 00000000`, an indirect jump through an internal table. A reachable ordinary RET exists, but the unresolved table prevents an exhaustive exit summary. No provider is missing for these blockers.

`_action_obey_describe_command` also has an unresolved internal jump table (`+3d`, `ff 24 85 00000000`), called at January `+38c8` / current `+38de`. The current caller frame pass does **not** reach that call because of its own unsupported control flow; the receipt explicitly marks it `reachable_in_frame_model: false`. It is a potential additional coverage boundary, not the cause of the observed modeled ESP loss. Unknown frame state at the earlier actor assertion is already propagated through loop joins, so the first unsupported call by offset is not necessarily the first instruction numerically displaying an unknown coordinate.

The initial scanner's first actor blocker was `_ai_debug_get_last_path+71`: the callee has a counted-loop backedge at `+3a` to `+17` and ordinary zero-pop RETs at `+48/+58`. `_build_circle_points`, called at circle `+92`, similarly has a loop and a zero-pop RET at `+65`. Initial summaries rejected cycles. The current conditional-normal-return policy summarizes both as zero, while making no termination claim. This explains why those earlier blockers disappear; it does not justify treating tail exits or indirect tables as known. No conclusions are drawn from the initial scanner's known-flawed coverage totals.

Keep the current fail-closed results. Distinguishing assertion nonreturning paths, following tail exits, or resolving jump tables would each require separately justified analysis; this follow-up proposes none. Source shapes, C compiler calls, production changes and matching gains are all **zero**.
