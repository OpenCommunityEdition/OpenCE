# `libs/libcmt/wasctime.obj` exact-match ledger

## Target inventory

- `_code_001ccf70`: 26-byte two-digit wide-character helper, no relocations.
- `__wasctime`: 220-byte public routine, 11 relocations.
- `_bss_004e3e7c`: 52-byte zero-initialized fallback buffer.

The public relocations are `_getptd`, `malloc`, the fallback BSS buffer,
`__dnames`, `__mnames`, and six calls to `_code_001ccf70`. Addresses, types,
destinations, and addends match pairwise.

## Recovered source model

The implementation is the historical Microsoft CRT wide `asctime` routine:

1. Fetch per-thread CRT state and allocate its 26-wide-character output
   buffer on first use.
2. Fall back to the object's 52-byte static buffer when allocation fails.
3. Copy the three-letter weekday and month abbreviations.
4. Emit day, hour, minute, second, and year through the private two-digit
   helper, then append newline and null.

The thread-data offset `+0x40` is independently corroborated by historical CRT
`_tiddata` layouts as `_wasctimebuf`. The public Microsoft documentation
confirms the exact fixed-width 26-character result format. These references
were used for semantics/provenance; January COFF bytes remain authoritative.

## Codegen findings

- Defining the helper after its caller is required. With its body visible
  first, VC7 inlines the calls; the January object keeps six private-convention
  calls.
- `*destination++` in the helper reproduces January's two `inc ecx`
  instructions and exact 26-byte body.
- The allocation block needs the historical short-circuit expression. It
  yields `store TLS field -> test EAX -> select fallback`, keeping the moving
  output pointer in `EAX` and the returned base in `EBX`.
- The weekday/month loop increments its index before the output pointer.
  Writing the `for` increment explicitly in that order changes only the three
  independent loop-epilogue instructions and closes the final hash mismatch.

## Rejected measured families

- Helper definition before caller: helper inlined; public body grew broadly.
- Fixed-index digit helper: correct semantics, wrong pointer increment/store
  schedule.
- Direct buffer local and separate allocation stages: correct 11 relocation
  identities but five bytes short.
- Preloading fallback before allocation: exact overall size but wrong
  relocation order and buffer register.
- Chained local/TLS assignment: tests the allocation result before storing it,
  two bytes short.
- Assignment-in-condition plus a second TLS selection: introduces a stack
  frame and broad drift.

## Strict verification

- `_code_001ccf70`: `26/26`, `0/0` relocations, normalized SHA-256
  `bf83fe6802bdbf91b839a1b0d32393c0c44e3ffde76cc2ae0867a078aaa70b03`.
- `__wasctime`: `220/220`, `11/11` relocations, normalized SHA-256
  `787045c63e553d36b98163b730c4e667e22f48f9e01dcea9aada299e8456731e`.
- `_bss_004e3e7c`: 52/52 logical zero-initialized bytes, external ownership,
  no relocations.
- No assembly, volatile forcing, `__forceinline`, undefined behavior, byte
  patching, flag changes, or comparator exceptions.

## Independent current-campaign revalidation

The historical completion was treated as a source-shape lead, not as
admission evidence.  Its three object-specific paths were transplanted onto
campaign commit `a339a154`; the stale config conflict was resolved by keeping
the complete current config and changing only `wasctime.c` to `Matching`.
The object was then compiled afresh under XDK 3911 with libcmt's unchanged
`/O1 /Gy /I"xbox/include"` command.

- Both function hashes, sizes, and all 11 relocation destinations/addends are
  strict-equal to January.
- `_bss_004e3e7c` is `52/52` logical zero bytes, zero relocations, SHA-256
  `7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a`,
  with matching external storage/type/value ownership.
- Full Halo and libcmt builds, semantic progress, and the per-unit regression
  snapshot/check passed with zero failures or warnings.
- All 179 tooling tests passed.

## House and Berth audit

The recovered structures and constants are named, parameters are formatted
one per line, and both functions have a single final return.  The public body
uses typed calls and the private helper rather than magic byte operations.
No alignment directive, anonymous structure offset, codegen cast, assembly,
`volatile`, forced inline, undefined behavior, byte forcing, flag change, or
comparator exception was introduced.  The allocation-failure fallback is
intentional CRT behavior, not an original-game bug requiring a corrective
comment.
