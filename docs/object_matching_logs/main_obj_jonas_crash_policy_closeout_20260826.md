# `main.obj`: `main_crash` policy closeout (2026-08-26)

## Outcome

`source/main/main:_main_crash` remains absent.  January's body is an
intentional null-pointer write, so every faithful C reconstruction violates
the campaign ban on undefined behavior.  No candidate source was retained and
no object-emitting candidate compile was spent on this function.

## Frozen January evidence

Clean baseline manifest:

- `build/audit/main_crash_baseline_20260826.json`
- baseline commit `92cc247cd12061ba0330eeda5d85c2d5adb59bac`
- meaningful size 11, padded size 16
- normalized SHA-256
  `abe944925d4f3b974a0bd6e1ec5523804233ecf1d23afb41bf05be96ee3ddc59`
- one `IMAGE_REL_I386_DIR32` relocation at byte 6 to the string literal
  `chucky was here!  NULL belongs to me!!!!!`

The target bytes decode as the equivalent of:

```text
mov dword ptr [0], <address of the literal>
ret
```

The relocation is on the stored value at byte 6; the destination address is
the literal absolute zero encoded directly in the instruction.  This is not a
call wrapper and it does not consume the script-visible `reason` parameter.

## Provenance review

The missing historical blob from Aerocatia/demon commit `a5bdf381` was fetched
by its Git object ID
`a7ec701ef297d57294b2e349775832a6d05efdac`.  Its source writes the same string
through a null `char **`, confirming the instruction-level interpretation.

The later Halo CEA reference also implements an intentional null write, but it
is only corroboration: January's own bytes and relocation are authoritative.

## Decision

Do not revisit this leaf through a null cast, raw address, pointer/integer
reconstruction, `volatile`, assembly, pragma, intrinsic, or object-byte patch.
Reopen only if the house rules explicitly change or new provenance supplies a
defined, non-null fault mechanism that also reproduces the January bytes.
