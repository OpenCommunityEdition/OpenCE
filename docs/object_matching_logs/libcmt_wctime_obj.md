# `libs/libcmt/wctime.obj` exact-match ledger

This atomic lane independently revalidates an older completed result against
the current January target and campaign gates.  Historical evidence is treated
as a lead, not as admission proof.

## Target inventory

- `_wctime`: one 25-byte public function with two `REL32` relocations.
- No owned `.data`, `.bss`, or `.rdata` section.

## Recovered behavior

The wrapper converts a `time_t` through `localtime`.  A successful conversion
is formatted as a wide string by `_wasctime`; failure returns a null wide-string
pointer.  The implementation has one source return.

## Measured source shapes

| Shape | Measurement | Disposition |
|---|---|---|
| Initialize `result = 0`, then conditionally replace it | 26/25 bytes, 2/2 relocs; `_wasctime` reloc moved from `+0x10` to `+0x14` | rejected |
| Uninitialized result with explicit `if/else` assignments | 25/25 bytes, 2/2 relocs at the target addresses and destinations | accepted |

VC7 retains the `localtime` result in `EAX`, branches to a separate null-result
block on failure, and passes the same register to `_wasctime` on success.

## Current-campaign strict evidence

The final source was transplanted onto campaign tip `b7bde2b6` after the
project-wide libcmt `/O1 /Gy` configuration landed.  The generated command was:

```text
xbox/bin/vc7/CL.Exe /nologo /c /O1 /Gy /I"xbox/include" \
  /Fobuild\base\libs\libcmt\wctime.obj libs\libcmt\wctime.c
```

The configured compiler is XDK 3911 `CL.Exe` 13.00.9254.1.  No lane-local flag
or build-rule change was used.

| symbol | target | candidate | relocations | normalized SHA-256 | result |
|---|---:|---:|---:|---|---|
| `__wctime` | 25 B | 25 B | 2 / 2 | `af079735ba08d7b5bf0650ba9accdaec0c7e7b85ffc983c1cd450eceeb688e78` | exact |

The ordered relocation records match exactly:

| offset | type | destination | addend |
|---:|---|---|---:|
| `+0x05` | `IMAGE_REL_I386_REL32` | `_localtime` | 0 |
| `+0x10` | `IMAGE_REL_I386_REL32` | `__wasctime` | 0 |

The target contains one 25-byte `.text` COMDAT, owned externally by
`__wctime`, and no runtime `.data`, `.bss`, or `.rdata`.  The candidate has the
same code ownership and only adds normal compiler `.drectve`/debug metadata.
Thus there is no unverified target-owned data or anonymous section hidden by
the function result.

## Gates and blast radius

- The entire configured libcmt project built successfully.
- The full 467-object `halobetacache_build` completed successfully.
- `ninja progress` reported libcmt at 3 / 476 functions and 95 / 55,015 code
  bytes before this new object was committed; the subsequent semantic report
  completed with zero unit errors.
- The strict comparator returned `all_equal: true` for `__wctime`.
- The whole-object symbol and section inventory was inspected with
  `llvm-readobj`; this unit has no sibling function or owned runtime data.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 177 / 177 passed.

## House/Berth and readability audit

The function is ordinary readable C with its parameter on its own line and a
single explicit final return.  `(void)` formatting is not applicable because
the function has one parameter.  It introduces no assembly, `volatile`, forced
inlining, undefined behavior, byte patch, alignment directive, raw structure
offset, comparator exception, or per-function compiler workaround.  There are
no `tag_get` or `object_get` calls requiring typed access macros.  No original
January behavioral bug was found, so no bug-preservation comment is needed.

In plain English, this function asks the CRT to convert a timestamp into broken-
down local time.  If that conversion succeeds, it formats the result as a wide
date/time string; otherwise it returns null.  Leaving the result local
uninitialized until both branches assign it is safe here and is the natural
source shape that produces the January control flow.

