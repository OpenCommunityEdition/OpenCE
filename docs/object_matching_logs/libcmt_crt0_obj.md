# `libs/libcmt/crt0.obj` exact-match ledger

This atomic reconstruction starts from campaign commit `fc8e1b07`. At lane
start the object was `MISSING`, absent from `config/parked.json`, had no
production source or object ledger, and had no implementation commit or
dedicated worktree in any local ref.

## Plain-English behavior

This is the CRT's fatal-startup-error exit adapter. `_amsg_exit` prints the
standard runtime-error banner, prints the requested runtime error, and then
invokes the configured exit callback with status 255. The callback is public
data initialized to `_exit`, allowing the runtime to replace it deliberately.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its actual `obj\i386\crt0.obj` member has SHA-256
`3a97b6ecc5887dc91a6a536b586b9dc9c71a92f926dd62b4c3a7c75944a9a3d2`.
CodeView identifies ordinary Microsoft C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\startup\crt0.c`, not assembly.
The archive member independently compares strict-equal to every January
runtime section before any source reconstruction.

The exact inventory is:

| Owner | Kind | Bytes | Relocs | Normalized SHA-256 |
|---|---|---:|---:|---|
| `__amsg_exit` | code | 28 | 3 | `302d8ff1d85bf917139c3acf63d37d5a336028b5a656493d7786dd776b29ed38` |
| `__aexit_rtn` | writable data | 4 | 1 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

The code relocations are `__FF_MSGBANNER` at `+0x01`, `__NMSG_WRITE` at
`+0x0a`, and `__aexit_rtn` at `+0x15`. The data relocation is `_exit` at
`+0x00`. Addresses, i386 relocation types, destination spellings, addends,
external storage, section flags, and owner offsets are exact.

## Measured source shapes

| ID | One changed factor | Strict result | Decision |
|---|---|---|---|
| E00 | Compare January directly with the authenticated XDK member | Function 28/28 with 3/3 relocs; data 4/4 with 1/1; hashes and ownership identical | Provenance accepted |
| E01 | Express the authenticated behavior directly with a typed exit-function pointer initialized to `_exit` | Function and data both pass hardened `section_infos_equal` on the first compile under unchanged flags | Accepted |

No compiler-shaping construct was needed. The explicit function-pointer type
describes the public callback ABI, and the initializer is required to own the
January `.data` relocation rather than emitting tentative COMMON data.

## Validation and house/Berth audit

- Compiler: XDK 3911 CL `13.00.9254.1`, unchanged libcmt flags `/O1 /Gy` and
  the existing Xbox include path.
- Hardened comparison covers meaningful size, normalized bytes, relocation
  address/type/symbol/addend, section flags, storage class, and data ownership.
- The source is ordinary readable C with vertical declarations, named types,
  and one natural return. It contains no assembly, `volatile`, forced
  inline/noinline, undefined behavior, byte forcing, compiler flag change,
  comparator exception, or symbolic-relocation waiver.
- No original behavioral bug was found: routing fatal startup errors through
  a replaceable exit callback is intentional CRT behavior.
- Final forced rebuild through the generated Ninja edge passed. The complete
  Halo and libcmt build graphs then passed with no additional work required.
- Fresh objdiff and semantic reports credit exactly one new object, one
  function, 28 code bytes, and four data bytes. The semantic audit evaluated
  3,592 functions, accepted 3,508 exact results, and reported zero unit
  errors. Project progress became 364/833 objects and 3,503/11,060 functions;
  libcmt became 97/212 objects and 155/476 functions.
- The admission audit reported no candidate or revocation. Its sole
  contradiction remains the unrelated pre-existing `shell_xbox` label.
- All 179 Python tool tests passed, and `git diff --check` passed. The strict
  production recheck still reports `section_infos_equal` for both owners.
- A clean-commit regression snapshot must be refreshed after this patch is
  committed. The root desktop sandbox denied Git ref/index writes during this
  lane, so claiming that specific commit-bound gate here would be dishonest;
  an isolated commit-capable lane performs it before handoff.
