# `items.obj`: recover the external `object_get_type` owner

Date: 2026-08-27

## Result

Restored the 32-byte external `_object_get_type` owner in
`source/items/items.c`. The unit advanced from 13/18 to 14/18 exact
functions, with zero residual functions and no loss of an existing exact
owner.

- Strict padded code gain: 32 bytes.
- Ordinary progress gain: 29 code bytes and one function.
- Remaining unwritten owners: `_code_000e6370`, `_item_accelerate`,
  `_code_000e6900`, and `_item_update`.

## Provenance and implementation

The target disassembly is a direct call to `_datum_get` through
`_object_header_data`, followed by a zero-extending load of the datum type
byte at offset 3. This is the exact logic already present in the typed
`object_get_type` inline in `source/objects/objects.h`:

```c
return object_header_get(object_index)->type;
```

`items.obj` nevertheless owns a separate external copy in the January
image. To restore that owner without changing any other translation unit,
`items.c` temporarily renames the header inline while including the shared
headers, then defines the external function with the same typed
`object_header_get` access. No raw `object_get` cast, raw address, assembly,
intrinsic, barrier, pragma, volatile/register steering, or byte forcing was
introduced.

The next 192-byte private owner was also bounded. Target disassembly and the
named HCEA donor at
`../work/halocea-reference/src/item_adjust_for_angular_velocity_change.c`
agree that it caches a normalized angular-velocity axis plus sine/cosine.
The target passes its item index in EAX, however, which is the compiler's
private local-function convention produced with its currently unwritten
caller. Restoring that helper alone would not preserve its January ABI, so
it was deliberately left unwritten.

## Exactness evidence

The scratch gate reports:

```text
UNWRITTEN   192  _code_000e6370
UNWRITTEN   560  _code_000e6900
UNWRITTEN   944  _item_accelerate
UNWRITTEN  2352  _item_update
== exact 14  residual 0  unwritten 4  (of 18 listed)
```

For `_object_get_type`, target and candidate both have 32 padded bytes,
one `_object_header_data` relocation, one `_datum_get` relocation, and the
same 14-instruction disassembly through the three trailing alignment NOPs.

- January target object SHA-256:
  `D7C23F2AC171FBEDC9C2FBDFA3053D663145D69B3DC19D402BB8080379C4314C`
- Production candidate object SHA-256:
  `209D4E7E07DE731547D4A21C71A39574D2DB5EDFF7923EDA6CF6B137941691EC`
- Source SHA-256:
  `58416DE8889207E49FFBA8F49233D84C377491286B8C9192E3712C9BAA289FF6`
- Source Git blob:
  `c767ffc8e0df481cd3bb81518aae1c093e1c728c`

The frozen production target was compiled exactly once:

```text
[1/1] CL build\base\source\items\items.obj
```

## Repository gates

- Full Ninja build: pass.
  - 470 semantic units scanned;
  - 4,470 functions evaluated;
  - 4,348 semantic-exact and 4,376 accepted-exact functions;
  - zero unit errors;
  - campaign progress: 377/833 objects, 4,349/11,060 functions,
    520,436/2,198,102 code bytes;
  - report SHA-256:
    `F9C7EFB4A98B542850C975B02F6ABC1BBA582D9240187BECF376956C6FD90A29`;
  - semantic report SHA-256:
    `DE78F52BFF27E08FB91FEAB2C5B444452DF76EC6C5F51CA1E8F510AABF578BC7`.
- `python -m tools.audit_object_admission`: pass, 0 candidates,
  0 contradicted, 0 revoked.
- `python -m tools.parked_functions`: pass, 3 active, 0 stale, 0 invalid.
- `python -m pytest -q`: 194 passed.
- Policy scan and `git diff --check`: pass; only the repository's normal
  line-ending notice was emitted.
