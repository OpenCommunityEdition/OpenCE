# `game_state_xbox.obj` persistent-storage deletion recovery (2026-08-29)

## Result

Starting from pushed canonical commit
`fb3fe09198e38e46a3e5534465879dc99aed1e40`, the independently callable
January body `_code_001b0240` was identified as the cross-build
`delete_persistent_storage` leaf and compiled once. It is strict exact on that
first compile: 48 padded bytes, two ordered relocations, and normalized
SHA-256
`e2592d1a629bf58bcc495a46b41c191202f64365b8460c825461fd90b91f5e71`.
`game_state_xbox.obj` advances from 4/17 to 5/17 exact functions, with no
residual body retained.

## Evidence and source shape

The complete prior target, compiler, tooling, type, ownership, and HCEA
provenance audit is recorded in
`game_state_xbox_obj_jonas_triad_20260821.md`. That bounded triad explicitly
excluded every private helper, including this function, and records no prior
candidate or compile for it.

The clean HCEA tree at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` provides the readable
`src/blam/interface/delete_persistent_storage.c` donor, whose file SHA-256 is
`84a9f13bda4b51fd42500ae16c9f6dbcc8791443d3cf3fd5eb05c9efdfd1d943`.
It proves the two-call conditional topology and void return. January COFF
independently proves the PC-specific 256-byte path buffer, zero local-player
index, helper signature, stdcall `DeleteFileA` target, branch direction,
padding, and relocation schedule.

Production declares both callees with their typed ABIs, allocates a named
256-byte path buffer, asks for local player zero's profile path, and deletes
the file only when path resolution succeeds. The two exact relocations are
REL32 `+0x13` to
`_player_ui_get_path_to_local_player_profile_directory` and REL32 `+0x26` to
`_DeleteFileA@4`.

The retained body is readable defined C89 with an explicit return. It uses no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
attribute, raw offset or address, representation cast, pointer/integer
reconstruction, union pun, undefined behavior, object patch, compiler/tool
exception, or byte forcing. The adjacent private-EAX
`_code_001b0220` helper remains unwritten because its authentic same-TU caller
cluster is not reconstructed. Units, Vehicles, Matrix Math, AI Debug,
configuration, and unrelated source are untouched.

## Strict retained inventory

Direct hardened comparison returns `all_equal: true` for the four inherited
exact functions and the new leaf:

```text
_game_state_get_persistent_storage_filename
_game_state_free_buffer
_game_state_close_file
_game_state_create_persistent_storage
_code_001b0240
```

Thus the object retains five exact, zero residual, and twelve unwritten
functions. The generic campaign-gate convenience wrapper cannot currently
decode this unit's Ninja-escaped `saved$ games` path, so no result is inferred
from that wrapper; the underlying hardened comparator was invoked directly on
both complete object paths and all five emitted functions.

The full Ninja build, report, semantic scan, and progress generation pass.
The campaign reports 382/833 linked objects, 4,729/11,060 exact functions,
599,293/2,198,102 meaningful code bytes, and 1,856,018/4,176,062 data bytes.
The semantic scan evaluates 4,885 functions across 470 units, with 4,758
semantic exact, 129 hidden exact, 4,768 accepted exact, and zero unit errors.
Object admission is 0 candidates / 0 contradictions / 0 revocations; the
parked ledger is 12 active / 0 stale / 0 invalid; and all 205 tooling tests
pass. `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes and 87 relocations, normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

## Bounded adjacent rejection

After the exact deletion leaf was integrated, the independently public-cdecl
`game_state_read_core_header` HCEA topology was reconciled with January's
1,024-byte PC path buffer and compiled once. Target and candidate both emit
128 padded bytes and the same five ordered relocation identities. The first
four relocation addresses are exact; candidate `CloseHandle` is at `+0x6C`
instead of January's `+0x6D`. January's normalized SHA-256 is
`f8442ec838d3920fe4f124ee3b18cc9d552dcaf02c06d69ab8134fc63ab5ad65`;
the candidate is
`dd5b12faec53da7b4a3481a62bf37b11f74774f5da6f040a591774665d34c4ce`.

The body and support declarations were removed completely, the production
object was rebuilt, and all five retained exact functions reverified. Reopen
that function only with distinct original declaration/control-flow provenance;
do not tune the result comparison, declaration order, or boolean spelling.
