# `wcstok.obj` exact-match log

## Plain-English behavior

This legacy two-argument CRT tokenizer separates a writable UTF-16 string into
tokens. It skips delimiter characters, replaces the delimiter following the
next token with a zero terminator, and remembers the continuation pointer in
the current thread's CRT state. A later call with a null `string` resumes from
that saved pointer.

## Screening and provenance

At authoritative campaign tip `114ad5d0`, `libs/libcmt/wcstok.c` was marked
`MISSING` and had no parked-ledger entry. No active lane owned this file.
Completed local commits `0f25ef8c` and `235bfb83` contained the same proposed
source, but that source was treated only as an untrusted reconstruction
hypothesis. This lane independently extracted the January object, rebuilt the
candidate, and rechecked every runtime-owned section and symbol.

## January target inventory

- Toolchain flags are the repository's unchanged libcmt flags: `/O1 /Gy`.
- The target owns one 165-byte `.text` COMDAT and no `.rdata`, `.data`, or
  `.bss`.
- `_wcstok` is the only defined function and has external COFF ownership.
- The only relocation is `IMAGE_REL_I386_REL32` at offset `0x08`, targeting
  `__getptd` with addend zero.
- The target accesses the saved wide-token pointer at thread-data offset
  `0x1C`. The source models the complete named CRT prefix through that field;
  it does not use raw offsets or padding blobs.

## Source-shape evidence

The January control flow contains two delimiter scans. The first advances past
leading delimiters. The second advances through the token, writes a UTF-16
terminator when it reaches a delimiter, and updates the thread continuation
pointer on every exit. The single final conditional expression returns null
for an empty token and the token start otherwise.

| ID | Shape | Result |
| --- | --- | --- |
| E01 | Named thread-data prefix, two explicit delimiter scans, one final return | Accepted: strict-exact |

The rebuilt candidate is `165/165` bytes with `1/1` relocation. Its normalized
SHA-256 is
`e90fd8ec6f954d2ae6e44020a5f43155313066ead56fc4d58118b70862ee54df`,
identical to January. The strict comparator resolves the relocation to
`__getptd+0`, not merely to an arbitrary call site.

## House/Berth and safety audit

Every parameter is on its own line, the no-argument `_getptd` declaration has
`void` on its own line, and the function has one explicit final return. The
source uses no assembly, `volatile`, forced inlining, undefined behavior,
byte-forcing cast, magic offset, alignment directive, compiler-flag change, or
comparator exception. No original bug has been identified that needs a
preservation/fix comment.

## Final admission gates

- The lane was rebased onto authoritative campaign tip `7d2d26ea`; the config
  merge retained the already-admitted `wcsstr` result as well as this new
  `wcstok` result.
- Forced XDK rebuild and hardened whole-object/ownership audit: passed.
- Full Halo and libcmt builds: passed.
- Semantic progress: 3,452 functions evaluated, 3,368 accepted exact, zero
  unit errors.
- Ordinary progress credits the object; the final observed aggregate was
  281/833 objects, with libcmt at 14/212 objects and 15/476 functions.
- Admission audit: no `wcstok` contradiction or revocation.
- Fail-closed regression manifest/check: `_wcstok` remains exact with no
  failures or warnings.
- Tool tests: all 177 passed.
- `git diff --check`: passed.

Nothing is pushed and the canonical worktree is untouched.
