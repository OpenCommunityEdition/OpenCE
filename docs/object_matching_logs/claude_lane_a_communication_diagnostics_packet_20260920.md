# Lane A research packet: the two ai_communication residuals are missing whole diagnostic blocks, and January's bytes name every field

Baseline `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`. Evidence is January's own
split object; nothing here comes from a decompiler or a later build.

## How this was found

A per-function string-literal relocation diff over **all 34 Lane A residuals**
(`scratch/orch/fn_string_diff.py`, keyed on the content-derived MSVC mangled
name so nothing is lost to name truncation):

- **0 invented strings.** No Lane A unit emits a string literal that is absent
  from the whole January image. That is a strong source-fidelity result.
- **32 of 34 residuals have exactly matching string sets.** Their divergences
  are not missing diagnostics; do not look for one there.
- **2 residuals diverge, both in `ai_communication`.**

## A. `_ai_communication_finished` - January has 16 string references, we have 0

Ours is 912 bytes / 29 relocations / 305 instructions.
January is 1,584 bytes / 87 relocations / **484** instructions.
Frame: ours `sub esp, 0x50`; January **`sub esp, 0x650`**.

The 0x650 frame is a large local message buffer at `[ebp-0x450]`. January builds
a human-readable explanation of the reply decision into it and appends a reason
token at each rejection point.

Missing strings (exact text, read from January's `.rdata` bytes):

    "REPLY %s: "   "%s:"   ">>%s<<"   "u-%s-%s "   "rand %.2f>%.2f "
    "filter "   "nobody "   "0-chance "   "override-scripted "
    "wrong-dmg "   "playercant "   "rand-failed "   "dis"   "n/a"
    "!reply_to_player"   "c:\halo\SOURCE\ai\ai_communication.c"

### The assert is fully recovered

January at 0x1b6:

    push 1                     ; fatal = TRUE
    push 0x9e5                 ; line 2533
    push "c:\halo\SOURCE\ai\ai_communication.c"
    push "!reply_to_player"
    call _display_assert
    push -1
    call _system_exit
    add esp, 0x14

preceded by `mov al, byte ptr [ebp+0x14]` / `test al, al`, i.e. the parameter
`reply_to_player`. This is exactly the `match_assert` form with file
`c:\halo\SOURCE\ai\ai_communication.c`, line **2533**, predicate
`!reply_to_player`.

Line 2533 is a hard source-line anchor for this function's position in
January's original `ai_communication.c`.

### The two string-building idioms

January starts the buffer with plain `sprintf`, then appends with
`csstrcat(message, csprintf(temporary, ...))`:

    ; start
    push 0 / push edi                       ; dialogue_get_vocalization_name(v, 0)
    call _dialogue_get_vocalization_name
    push eax
    lea ecx, [ebp-0x450]
    push "REPLY %s: "
    push ecx
    call _sprintf
    add esp, 0x14

    ; append
    push 0 / push ecx
    call _dialogue_get_vocalization_name
    push eax
    push "%s:"
    push _temporary
    call _csprintf
    push eax
    lea edx, [ebp-0x450]
    push edx
    call _csstrcat
    add esp, 0x1c

Every append is guarded by a byte test on the same `ai_debug` flag.

### Every `_ai_debug` offset resolves to a real named field

`source/ai/ai_debug.h` already defines `struct ai_debug_state` and pins it with
`offsetof(struct ai_debug_state, print_speech) == 0x93` (147). Counting the
`boolean` run from that anchor:

| January operand | field |
|---|---|
| `_ai_debug:143` | `ai_debug.print_communication` - guards **every** trace append |
| `_ai_debug:132` | `ai_debug.communication_focus_enable` |
| `_ai_debug:133` | `ai_debug.communication_random_disabled` |
| `_ai_debug:100` | `ai_debug.communication_focus_vector` - used with `BIT_VECTOR_TEST_FLAG` |
| `_ai_debug:149` | `ai_debug.print_allegiance` (used by `_ai_communication_event`) |

No new type, header or name is needed: all five fields already exist in the
current `ai_debug.h`.

### Local state January's prologue proves

    mov byte ptr [ebp-1], 0        ; boolean, initialized FALSE
    mov byte ptr [ebp-2], 0        ; boolean, initialized FALSE
    ...
    mov byte ptr [ebp-2], 1        ; set TRUE later
    mov byte ptr [ebp-1], al       ; = BIT_VECTOR_TEST_FLAG(communication_focus_vector, v)

The `[ebp-1]` boolean is the focus-vector test result; `[ebp-2]` is set when a
priority is taken. Both are declared and zero-initialized at the top.

### Reason tokens in emission order

`"REPLY %s: "` (start), `"%s:"`, `"wrong-dmg "`, `"override-scripted "`, the
assert, then `"rand %.2f>%.2f "`, `"rand-failed "`, `"0-chance "`, `"u-%s-%s "`
(with `"dis"` / `"n/a"` selected by a ternary into its two `%s`), `"playercant "`,
`"nobody "`, and finally `">>%s<<"`.

## B. `_ai_communication_event` - January has 75 string references, we have 68

Ours is 7,776 bytes / 319 relocations. January is 8,064 / 336 - **17 missing
relocations**, of which 7 are these strings, plus the
`_game_allegiance_get_incidents` call and `_global_game_team_names`.

Missing block, January 0x329-0x409, guarded by `ai_debug.print_allegiance`:

    if (ai_debug.print_allegiance)
    {
        console_printf(FALSE, "incident between teams %s and %s: %s, %s",
            global_game_team_names[team_a],
            global_game_team_names[team_b],
            <betrayal_flag> ? "betrayal" : "accident",
            <observed_flag>  ? "observed" : "unobserved");
    }

and after the `game_allegiance_incident` call:

    ; January 0x3d1
    lea ecx, [ebp-8]
    push ecx
    push edx
    call _game_allegiance_get_incidents
    mov cx, word ptr [ebp-8]
    cmp cx, -1
    jne .have
    mov ecx, 0x3e7          ; 999 when the threshold is NONE
    jmp .print
    .have:  movsx ecx, cx
    .print:
    push ecx
    movsx eax, ax
    push eax
    push "still holds"
    push "allegiance %s, %d incidents (threshold %d)"
    push 0
    call _console_printf
    add esp, 0x14

`console_printf` is already declared in `source/main/console.h` as
`void console_printf(boolean clear, const char *format, ...)` and is already
used by `source/ai/actor_looking.c` and `source/ai/actor_combat.c`, so the
first block needs no header change. The second block calls
`game_allegiance_get_incidents`, which is declared in no header in the tree -
see the header-prerequisite split at the end of this packet.

Note the NONE-to-999 substitution: it is a real January behaviour
(`threshold != NONE ? threshold : 999`), not an invented constant.

## Why this is new work rather than a replayed hypothesis

`astra_90pct_rejected_hypotheses_20260920.md` records the communication-event
work as "primary-backed direct indexing, nineteen store roles/order and
previous-entry diagnostic reads produce 8112/335, not January 8064/336", and
warns against declaration / field-order / inlining sweeps. None of those is what
this packet proposes. This is a *missing-code* finding derived from string
relocation identity; it names the specific guarded blocks and the specific
already-declared fields and functions, and it is not a spelling permutation.

`_ai_communication_finished` at 42.12% with 672 missing bytes and 58 missing
relocations is the largest purely structural gap in the whole 79,657-byte
portfolio.

## Warnings for whoever implements this

- The buffer is a local, not `temporary`. `temporary` is the 256-byte shared
  buffer passed to `csprintf`; the message buffer is separate and lives at
  `[ebp-0x450]`. Do not collapse the two.
- The first write is `sprintf`; later writes are
  `csstrcat(msg, csprintf(temporary, ...))`. That asymmetry is January's, not a
  mistake.
- Restoring diagnostics changes the frame size dramatically. Get the block
  structure right first; only then look at register / schedule residue.
- The assert must use the `match_assert` form so the baked file and line stay
  `c:\halo\SOURCE\ai\ai_communication.c` / 2533, and the predicate must be
  `!reply_to_player` (the required invariant), never `reply_to_player`.
- Reproduce all of this with `scratch/orch/fn_string_diff.py` and
  `scratch/orch/finished_alndiff.txt` / `scratch/orch/event_alndiff.txt`.

## Header-prerequisite split (checked against the real headers)

Every symbol `_ai_communication_finished` needs is **already declared**:

    csstrcat            source/cseries/cseries.h:185
    strupr              source/cseries/cseries.h:178
    temporary           source/cseries/cseries.h:269
    csprintf / sprintf  source/cseries/cseries.h
    game_connection     source/game/game.h:191
    dialogue_get_vocalization_name  source/units/dialogue_definitions.h:34
    ai_debug            source/ai/ai_debug.h

So the 1,583-byte `_ai_communication_finished` reconstruction is **not**
header-blocked and can be attempted entirely inside `source/ai/ai_communication.c`.

For `_ai_communication_event`, only the *second* block is blocked:

- `global_game_team_names` is declared (`source/game/game_allegiance.h:44`) and
  `console_printf` is declared, so the first block -
  `console_printf(FALSE, "incident between teams %s and %s: %s, %s", ...)` -
  needs **no header change**.
- `game_allegiance_get_incidents` is declared in **no header in the tree**. Its
  genuine owner is `source/game/game_allegiance.h` (which already owns
  `global_game_team_names` and the rest of the allegiance API). That one block is
  therefore a **deferred header prerequisite** for a later header-owner lane, and
  must not be worked around with a consumer-local prototype.
