# Console commands across checkpoints

A sleeping console command must resume with the expression and text that
belonged to it when the game was saved. This walkthrough follows the shared
interpreter fix in the MCC refactor. It applies to Xbox, Custom Edition and
MCC maps because all three use the same console compiler and saved game
service. Their map readers, authored script permissions and runtime limits
remain separate.

## The lifetime problem

The interpreter represents an expression as linked **syntax nodes**. A
thread's **stack frames** record the calls it must finish, including handles
to those nodes. A handle contains both a slot and an allocation identifier.
Reusing the slot does not preserve the old expression: the identifier must
still match the allocation being referenced.

Historically, checkpoints saved the threads in the CPU game-state image,
but their temporary console syntax lived outside that image. After the
command finished, `hs_node_gc` could collect its nodes. Restoring the older
thread then restored handles to expired or reused nodes. Console commands
also shared one text buffer, so a later command could overwrite a sleeping
command's string literals before it resumed.

The correction gives console text a saved owner and snapshots the syntax
allocator with the threads. This fixes an existing shared lifetime contract;
an MCC-only adapter could not repair Xbox or CE console continuations.

## Open these files in order

| Step | File and functions | Question to follow |
| --- | --- | --- |
| 1 | [hs_compile.c](../../source/hs/hs_compile.c): `hs_compile_expression`, `hs_parse_string` | Where does a command's source text live, and how does a string node refer to it? |
| 2 | [hs_runtime.c](../../source/hs/hs_runtime.c): `hs_runtime_evaluate`, `hs_thread_main`, `hs_runtime_update`; [hs.c](../../source/hs/hs.c): `hs_node_gc` | Which thread retains the expression, and when can its temporary nodes be collected? |
| 3 | [hs_console_state.h](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/hs_console_state.h), then [hs_console_state.inl](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/hs_console_state.inl): `hs_console_source_acquire`, `hs_console_sources_mark`, `hs_console_source_finish` | What prevents a new command from reusing live text? |
| 4 | [game_state.c](../../source/saved%20games/game_state.c): `game_state_save`, `game_state_save_core`; `hs_console_state_capture` in the console module | What is captured before either kind of save is written? |
| 5 | `game_state_image_accept`; `hs_console_state_validate`, `hs_console_saved_threads`, `hs_console_saved_graph`, `hs_console_state_restore` | What is checked before saved bytes become live, and how are the original syntax handles restored? |
| 6 | `hs_runtime_initialize`, `hs_runtime_initialize_for_new_map`, `hs_runtime_dispose_from_old_map` | Who allocates the state and clears it when the map changes? |

The console module is included in `hs_runtime.c` because it needs the
interpreter's private thread and stack layouts. Its interface is shared;
MCC thread storage and checkpoint extensions keep their existing owners.

## A sleeping command

With an accepted map running and access to the developer console, consider:

```lisp
(begin (sleep 150) (print "saved continuation"))
```

At the normal 30 simulation ticks per second, this command waits five
seconds before printing. Save a core while it sleeps, let the command
finish, then issue another command and load the saved core. The restored
command should wait the remainder of its saved delay and print its original
text again.

`hs_compile_expression` first acquires a source slot. Once compilation
finishes, `hs_console_source_finish` records its root expression handle.
While the command sleeps, its outermost stack frame identifies that root,
so another command must acquire a different slot even if the sleeping
thread has not yet evaluated the string literal.

The save captures the syntax array's allocation metadata and nodes, along
with the already saved source slots and roots. On load, validation checks
the candidate image without publishing it. After the CPU image is copied,
`hs_console_state_restore` restores the syntax allocator and nodes into the
currently loaded map's original array, preserving that array's data pointer.
The saved thread's handles therefore name their saved expressions again.

Seeing a successful load message is insufficient evidence for this case.
A regression run must allow the original expression to finish, wait for
collection opportunities, compile a replacement, load, and observe the
saved continuation actually print again. Normal console printing need not
include a newline, so capture the replay before sending another command
whose echo could join the same output line.

## Bounded ownership and map lifecycle

`hs_console_state_initialize` allocates one block through `game_state_malloc`
when the interpreter's saved thread and global arrays are initialized. It
contains the syntax snapshot, 256 source slots of 1,024 bytes each, and one
root handle per slot. Commands retain the existing limit of fewer than
1,024 source bytes, including room for a terminating zero. The snapshot
can hold 32,767 nodes; native maps still use their existing 19,001-slot
syntax arrays and MCC retains its own capacity.

Source storage is reclaimed when another command is compiled, without
adding a scan to each simulation tick. `hs_console_sources_mark` retains
slots referenced by active console roots, thread results, live stack
values or script global values. Stack values are untyped and can be
unaligned, so the scan conservatively treats any value inside a source
slot as a possible reference. A coincidental integer can retain a slot;
it cannot justify overwriting live text. If no slot is available, compilation
returns a bounded storage error. It does not evict a pending command.

The source pool and snapshot reset during map initialization and old-map
disposal. Native threads keep their inline stacks; MCC threads keep the
MCC-owned enlarged storage selected by `mcc_hs_threads_activate` and bounded
by `mcc_hs_stack_end` in
[mcc_script_threads.inl](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_script_threads.inl).
The generic fix neither transfers that ownership nor combines the map
loading paths.

## Refusal before restoration

`hs_console_state_validate` checks the snapshot signature, allocator sizes
and counts, occupied-node identifiers and permanent syntax against the
currently loaded map. It validates saved thread storage, frame bounds,
previous-frame links and result destinations. A bounded graph walk from
live frame expressions rejects stale handles and cycles; temporary string
constants must point to terminated text within the saved source pool.
Temporary calls also pass the existing structural and argument checks.
`hs_console_saved_source_owner` additionally checks that a sleeping console
root still owns the slots containing its unevaluated string literals. A
terminated pointer alone would not prevent another command from reclaiming
that slot after restoration.

Permanent scenario nodes must match the loaded map byte for byte. Inactive
graphs are not reinterpreted merely because a checkpoint is being saved:
the existing map admission code can disable a damaged script while leaving
its unused nodes in the array. Requiring every such graph to be executable
would introduce a new map refusal policy.

In [hs.c](../../source/hs/hs.c), `hs_syntax_node_check` supplies common
structural checks. `hs_syntax_node_refusal` retains the map restrictions,
including permanent-node requirements, the native function allowlist and
downloaded-map restrictions. `hs_console_syntax_node_valid` applies console
permissions to saved console expressions. This does not grant authored
Xbox or CE scripts access to console-only commands, or bypass MCC's
separate function validation.
In [mcc_scripts.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_scripts.c),
`mcc_script_call_arguments_valid` checks that grammar for temporary console
nodes. The map wrapper, `mcc_script_call_valid`, still requires the
permanent-node bit before delegating to it.

Adding the saved allocation changes the engine's allocation-size checksum.
`game_state_header_valid` therefore refuses cores and checkpoints from
the older allocation layout. There is no migration that invents the missing
console text or syntax. The file remains an old save; refusal is not proof
that it was corrupt. A rejected ordinary revert follows the existing map
restart path. Network agreement changes and their versions are documented
in [NETCODE.md](../../port/linux/NETCODE.md).

## Evidence and limits

[test_hs_console_state.py](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/tools/harness/tests/test_hs_console_state.py)
extracts the actual compiler ownership path, allocator, interpreter,
collector, checkpoint support and validation functions. Its fixture in
[hs_console_state.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/tools/harness/tests/hs_console_state.c) supplies a
small fake world. Cases cover collection and reuse, concurrent text,
global references, capacity limits, malformed saved references, inactive
permanent nodes and deliberate faults that the tests must detect.

These controls do not establish a complete game session or cross-platform
compatibility. Required runtime checks include replay on Xbox, CE and
affected MCC maps, ordinary checkpoint/revert, same-build cross-process
core loading, old-layout refusal and map-family transitions. Platform
builds and native/browser networking need separate testing; this walkthrough
does not claim those checks have all passed.
