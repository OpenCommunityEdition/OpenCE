# `model_animations.obj` frame-size first-shot boundary (2026-08-30)

## Scope and authority

This strictly bounded lane froze one previously unwritten public leaf:
`source/models/model_animations.obj::_animation_set_frame_size`.  All standing
campaign exclusions, active agent reservations, headers, configuration,
target artifacts, compiler flags, and every sibling function remained outside
scope.

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated `model_animations.obj` has raw SHA-256
`095A61EC6E430DFAC62230D592C68A755FB03DE5445DAAD7AEC052466991CE3C`.
The sole candidate used the repository XDK 3911 VC7 `/O2 /Oy- /DDEBUG
/Dxbox` edge; `xbox/bin/vc7/CL.Exe` has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`.
Only complete padded bytes and ordered symbolic relocations through
`tools/coff_compare.section_infos_equal` decide retention.

The HCEA reference was authenticated at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  Its donor
`src/blam/models/animation_set_frame_size.c` is Git blob
`14cdc863c67f7dc1e5866cce25b769815ecde06c`, payload SHA-256
`9F41C4630CA73757B6E7989FFD8B90C5B00FD448D360771D2D0AD265D79A0BD3`.
HCEA authenticates the parameter, zero accumulator, node loop, flag order,
and `8/12/4` increments.  The read-only Pastudan tree at commit
`918af885935ec470a31256ecce9a977b12b01f80` independently corroborates the
January assertion, offsets, loop meaning, and store width.  Its raw-offset
form was evidence only and was not copied.

The canonical typed `struct animation` already proves all used named fields:
`frame_size +0x24`, `node_count +0x2C`, translation flags `+0x5C`, rotation
flags `+0x6C`, and scale flags `+0x7C`.  January disassembly confirms those
offsets, signed node-count test, long loop index, word result store, assertion
line 123, and ordinary cdecl ABI.  The function is link-closed: its only calls
are the standard assertion pair.

The complete acceleration playbook, campaign/audit tooling READMEs,
object-ledger doctrine, and applicable Stian/Pastudan `CLAUDE.md` and
`AGENTS.md` were read before edit.  No model-animations object ledger exists,
`git log --all -S animation_set_frame_size` finds only the initial inventory,
and a tree-wide campaign search finds no prior definition.  This was therefore
a genuinely new campaign compile.

## Baseline and January packet

The pre-lane source is Git blob
`0889b4ce1ea2bcf2ef0010bc89e45e9a0a039e2d`, working-tree SHA-256
`A339B5A4EE39D3B69E03FE461CAACB790E7CF89519D75921848AEB6B916123E5`.
Before the shot the TU was 16/32 strict-exact functions and
1,744/12,144 exact padded bytes; the target leaf was unwritten.

January's leaf packet is:

- 121 meaningful / 128 padded bytes;
- four ordered relocations at `+0x13,+0x18,+0x1D,+0x24`;
- normalized SHA-256
  `8c8032348f8be9804010b53446bd05549899ce34d686d0aa9383b82a1348e602`.

The relocations are the source-path and `animation` assertion literals,
followed by `_display_assert` and `_system_exit`.  There are no private
callees, globals, jump tables, or owned data.

## Sole candidate and result

One natural readable C89 definition was adapted from HCEA and the existing
typed layout.  It used `short frame_size`, a `long node_index`, a signed long
word index, an unsigned long bit, named flag arrays, explicit parameter/return
spelling, and the January assertion.  It contained no assembly, volatile,
register, pragma, intrinsic, barrier, raw address/offset access,
pointer/integer reconstruction, pun, undefined behavior, byte forcing, or
compiler/tool exception.

Exactly one production compile emitted:

- 114 meaningful / 128 padded bytes;
- the same four relocation identities and types, at
  `+0x14,+0x19,+0x1E,+0x25`;
- normalized SHA-256
  `221a1eb56a90766032c2d18fd63e00f38f262a0bb91f32f8c98bfed5ce20b0c4`;
- raw object SHA-256
  `564408B7B734B4C25279EE78EDD34676DBD4217296FDC8603A7A5BB0D24BA4D5`.

The transient candidate source is Git blob
`ebd57346dba767048ed770fb9e8744ddd566f61b`, working-tree SHA-256
`D953180CA670FA1E82A73744BCA525EAD7AB293F2078B89137818253BB40CD0D`.
Equal padded size and relocation identities do not override the differing
normalized bytes and relocation addresses.  `section_infos_equal` is false.

## Fixed boundary and reopen criterion

January keeps the frame-size accumulator in `EDI` and zeroes it before the
assert.  After confirming positive node count it pushes `EBX`, zero-extends
the count into `EBX`, keeps the long node index in `EDX`, and terminates with
`inc EDX; dec EBX; jne`.  The natural donor form instead saves `EBX` before
the assert, keeps the accumulator in `EBX`, uses `EDI` as the index, and
reloads/sign-extends `animation->node_count` on every iteration.  Its extra
early `push edi` moves all four otherwise-correct relocations by one byte.

This is a source-loop-topology/local-lifetime boundary, not an ABI, field,
signedness, constant, flag-order, or relocation-destination error.  No local
node-count copy, countdown loop, `do/while`, declaration order, assertion
placement, or register experiment followed.  Reopen only with authenticated
January or same-compiler source evidence fixing the hoisted-count countdown
shape before compilation; do not tune or sweep the recorded donor form.

## Restoration and validation

The complete candidate definition was removed by inverse patch.  The restored
source again hashes to Git blob
`0889b4ce1ea2bcf2ef0010bc89e45e9a0a039e2d`, and its scoped diff is empty.
The restored TU rebuild returns the leaf to unwritten and the board to 16/32
exact functions and 1,744/12,144 exact padded bytes.

The restored-tree `ninja halobetacache_build libcmt_build progress
semantic_progress` gate passed.  Semantic progress reported 470 units, 4,919
evaluated functions, 4,802 semantic-exact functions, 4,812 accepted-exact
functions, 131 hidden-exact functions, and zero unit errors.  The full strict
report remained 384/833 objects and 4,773/11,060 functions, with
606,875/2,198,102 meaningful code bytes and 1,856,050/4,176,062 data bytes.

The focused board reconfirmed all 16 exact siblings and the restored 16/32,
1,744/12,144-byte TU state.  Protected
`_unit_preprocess_node_orientations` remained strict-exact at 1,920 padded
bytes, 87 ordered relocations, and matching normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reported zero candidates, contradictions, and revocations; parked
state reported 12 active, zero stale, and zero invalid entries.  Pytest passed
212 tests (with only the environment's non-semantic cache-write warning).
`git diff --check` passed.

The generated `build/report.json` is 1,599,057 bytes with SHA-256
`B9ECFA128C6E67E4D36B1DC1041CD53558AFEE06338AC0F623A3F7CBBF2E8B22`;
`build/semantic_report.json` is 3,267,795 bytes with SHA-256
`E5BB3F3990965E7527B94E6A8A414DC23541D3627E32D4FBE057CD4A17E2EC53`.
Only this additive boundary ledger is retained.  Nothing is pushed.
