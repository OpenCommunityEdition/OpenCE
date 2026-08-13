# `libs/libcmt/wcsicmp.obj` exact-match ledger

## Target inventory

- Runtime section: one 136-byte `/Gy` `.text` COMDAT
- Function: `__wcsicmp`
- Runtime relocations: three
  - `+0x02`: `IMAGE_REL_I386_DIR32` to `___lc_handle + 8`
  - `+0x5b`: `IMAGE_REL_I386_REL32` to `_towlower`
  - `+0x6a`: `IMAGE_REL_I386_REL32` to `_towlower`
- Owned data: none

The `___lc_handle + 8` addend is the `LC_CTYPE` slot. A zero handle selects
the fast C-locale path; a nonzero handle selects the locale-aware `towlower`
path.

## Recovered source shape

The January target is the XDK 3911 CRT member itself. Extracting
`obj\i386\wcsicmp.obj` from the SDK's `libcmt.lib` and comparing it to the
split target gives strict equality: `136/136` bytes, `3/3` relocations, and
the same normalized SHA-256. Its COFF debug records identify the historical
source as
`d:\xbox-finalfre\private\sdktools\crt\crtw32\string\wcsicmp.c`, timestamped
2001-08-11, and name the two arguments `dst` and `src`.

The function compares two wide strings after folding each character to lower
case. The C-locale path uses the historical CRT ASCII-fold expression:

```c
character = (*string >= (wchar_t)'A' && *string <= (wchar_t)'Z')
	? *string + (wchar_t)('a' - 'A')
	: *string;
```

Both pointers advance after the two fold expressions, and the loop continues
while the first folded character is nonzero and both folded characters are
equal. The locale-aware path performs the same loop through `towlower`. The
function has one final return containing the unsigned-wide-character
difference.

This repeated-expression ternary is not cosmetic. Under XDK 3911 VC7 it emits
the target's `EBX`/`ESI`/`EDI` allocation, while equivalent temporary-plus-if
spellings produce a three-register permutation. It is also the ordinary
historical CRT macro topology, corroborated by the shipped sibling
`wcsnicmp` family, rather than a byte-forcing construct.

The object owns only `__wcsicmp`, emitted from the C identifier `_wcsicmp`.
The SDK's separate `oldnames.lib` member has zero runtime bytes and expresses
`_wcsicmp` as a weak external alias linked to undefined `__wcsicmp`. No alias,
duplicate body, or forwarding thunk belongs in this object.

No assembly, volatile forcing, `__forceinline`, undefined behavior, byte
patching, compiler-flag change, or comparator exception is used.

## Strict verification

- size: `136/136`
- relocations: `3/3`
- normalized SHA-256, both sides:
  `9c7e512541538c84ed894c8763b7ff1611bb41970a1ca0fcdcad3e2b872f6fa5`
- relocation addresses, types, destinations, and addends: identical
- `section_infos_equal = true`
- candidate runtime ownership: one 136-byte `.text` COMDAT
- owned data: none on either side

## Experiment record

| ID | Source shape | Size/relocs | Normalized SHA / result |
|---|---|---:|---|
| E01 | Post-increment each pointer at its character load; temporary-plus-if ASCII fold | `136/3` | `0455aff618b14c20a1297faaf9d0de2f87369945b41ec01952d60d66d9582786`; register permutation and early pointer increments |
| E02 | Delay both C-locale pointer increments until after both temporary-plus-if folds | `136/3` | `01b0942d95732c53aacdda4f7c9b6c75dee88d451c36fe7cb8daeb372de5e8d7`; scheduling fixed, three-register permutation remains |
| E03 | Reverse declarations or combine them on one declaration | `136/3` | identical to E02; declaration spelling is not a control |
| E04 | Historical repeated-expression ternary fold, then advance both pointers | `136/3` | strict exact |

E04 matches the function bytes, relocation identity, and object ownership. The
object is safe to mark `Matching`.
