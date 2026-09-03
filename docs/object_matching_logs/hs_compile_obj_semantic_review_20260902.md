# `hs_compile.obj` semantic review — 2026-09-02

## Boolean parser

The January `_hs_parse_boolean` packet stores the local parsed-value byte even
after all accepted spellings fail.  The byte-exact C shape left that local
uninitialized on the error path, so its exactness depended on undefined
behavior.

Two independent source lines reject that interpretation:

- the HCEA Xbox reconstruction at repository commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, locally cached as
  `work/research-cache-halocea/src/blam/hs/hs_parse_boolean.c`;
- the later PC implementation at
  `work/halo/src/halo/hs/hs_compile.c` in local reference commit
  `afc14e5b1d016560ac5808a7da5aa9f2adb2b885`.

Both write the node only for a recognized false or true spelling and return
immediately.  The retained implementation follows that corroborated behavior.
It is intentionally a fuzzy match rather than preserving an exact packet with
an uninitialized read.

The following defined source shapes were compiled against the January target:

| source shape | result |
|---|---|
| initialize the local value to `FALSE` | 288/304-byte residual |
| initialize the local value to `TRUE` | 288/304-byte residual |
| store the local value only when parsing succeeds | 288/304-byte residual |
| HCEA/later-PC early returns | 272/304-byte residual |
| derive the value from the true-spelling comparison expression | 288/304-byte residual |
| initialize true and clear it on rejection | 288/304-byte residual |

No defined, plausible variant reproduced the target packet.

## `hs_compile` output order

`hs_compile` owns the public contract
`(source_size, source, error_message, error_source)`.  The January caller in
`hs_compile_source` passes the last two addresses in reverse order; the HCEA
Xbox source independently reproduces the same call.  Following it literally
causes subsequent error reporting to treat source text as a mutable error
message and the actual error message as a source pointer.

Because both target and source evidence prove this is original behavior, the
decompilation retains the reversed call and marks it `BUG (original)`.  A safe
corrected build should instead use:

```c
hs_compile(source_size, source, &error_message, &error_source);
```

The corrected build should also initialize both outputs and test
`error_message` for failure.  Those safety changes are documented here rather
than silently folded into the historical exact-match source.
