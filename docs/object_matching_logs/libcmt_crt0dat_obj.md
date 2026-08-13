# `libs/libcmt/crt0dat.obj` exact-match ledger

## Plain-English behavior

This is the Xbox C runtime's orderly-shutdown dispatcher. It owns the four
small state variables used during shutdown, serializes the callback lists with
the CRT exit lock, runs registered callbacks in reverse order, runs the two
linker-ordered termination ranges, and either returns to the caller or asks the
Xbox kernel to stop execution. The four public entry points select ordinary
versus quick shutdown and returning versus non-returning behavior.

## Scope and provenance

- Source: `libs/libcmt/crt0dat.c`.
- January target: `libs/libcmt/crt0dat.obj`, object index 756.
- Compiler: XDK 3911 CL 13.00.9254.1.
- Flags: the unchanged libcmt project flags `/O1 /Gy`.
- Primary source provenance: member `obj\\i386\\crt0dat.obj` extracted from
  the XDK 3911 `libcmt.lib`. Its CodeView records name `crt0dat.c`, identify
  the private functions as `_initterm` and `doexit`, mark both private, and
  carry source-line mappings. This proves the target is compiled C rather than
  vendor assembly.
- The extracted member's eight runtime function sections and 16-byte BSS are
  instruction-, relocation-, and ownership-identical to the January object.
  The two anonymous csplit labels were therefore renamed to the authentic CRT
  names and marked source-private in `config/symbols.json`.

## Validated baseline

Before reconstruction the object was `MISSING`; there was no candidate code or
owned data. January owns eight functions totaling 281 bytes and one 16-byte
zeroed `.bss` section:

| Function | Size | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `__lockexit` | 9 | 1 | `00714b156867424e2a315ffa7da3c441a96946d8785eb54c2cf4a181c14a97d9` |
| `__unlockexit` | 9 | 1 | `00714b156867424e2a315ffa7da3c441a96946d8785eb54c2cf4a181c14a97d9` |
| `__initterm` | 24 | 0 | `0b6174d87ebf9fb95863f92b795ae070082647b69cbc6cab3ff13d0f8ac62001` |
| `_doexit` | 170 | 14 | `82dcc5571d1778071dfa5dc6a2b2b57f56dafad2c00033e29a8bc4f5603309ed` |
| `_exit` | 18 | 1 | `3aa12b986f98be02b4cc6a9408cab94d042cadf4f067c2fc2bf45628ef825303` |
| `__exit` | 18 | 1 | `9d990b4867504649bbbc983d7138685453c355e770ce4900c590739a36579abd` |
| `__cexit` | 17 | 1 | `e588ef357981d220824cde8c5a5272e11cf708f199263b194abdeb273c652439` |
| `__c_exit` | 16 | 1 | `95d416f74df0de85fb8bb5e92c50745c034c4d33eeed64863a388bf528467389` |

The BSS is 16 bytes, aligned to four, has no relocations, and has normalized
SHA-256 `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb`.
Its external symbols are `_umaskval` at `+0`, the byte `_exitflag` at `+4`,
`_C_Termination_Done` at `+8`, and `_C_Exit_Done` at `+12`.

## Accepted source controls

- `_initterm` and `doexit` are source-private exactly as the XDK member's COFF
  symbol classes prove. This permits VC7 to inline the callback walker twice
  and to use the target's private EAX/stack and EBX/stack calling conventions.
- The four globals use explicit zero initializers. Under this compiler that
  creates the target's single owned `.bss` section; declarations without
  initializers become undefined COMMON symbols and lose object ownership.
- `__onexitend` is decremented as the global pointer itself. That is not a
  codegen trick: both the January bytes and XDK member reload and update the
  global around every callback, preserving mutations a callback could make.
- The public wrapper calls use the natural three-argument `doexit` interface.
  VC7 derives the exact private EBX convention without annotations or forced
  registers.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash / first divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| E00 | Direct strict comparison of the XDK 3911 library member to January | all 281/281 | 20/20 | Every section equal; authentic names and line provenance recovered | 8/8 exact, BSS exact | provenance accepted |
| E01 | Natural callback loops, but external `_initterm` and tentative uninitialized globals | `_initterm` 24/26; `_doexit` 170/138 | 0/0; 14/16 | `_initterm` `b94efae9...`, first divergence `mov esi,[esp+8]`; `_doexit` `527748a9...`, first divergence at prologue; BSS was absent and globals were COMMON | 6/8 exact | reverted |
| E02 | XDK-proven private `_initterm` plus explicit-zero BSS definitions | all 281/281 | 20/20 | All normalized hashes above equal; all relocation identities equal | 8/8 exact; BSS exact | retained |

## Whole-object ownership and alias audit

- Target and candidate contain the same eight runtime COMDATs, with the same
  sizes, flags, linkage, owner names, relocation tables, and normalized bytes.
- `_initterm` and `_doexit` are storage class 3 (private); the six public entry
  points are storage class 2 (external), matching the XDK member and regenerated
  split target.
- Target and candidate each own one 16-byte `.bss` section with flags
  `0xC0300080`, the same four external symbols at offsets `0/4/8/12`, and no
  relocations or aliases.
- There are no candidate-only runtime sections, helper copies, data aliases,
  string literals, or ownership spill. Debug sections are ordinary compiler
  metadata and are not linked runtime content.

## Do-not-repeat list

- Do not make `_initterm` external: it changes its ABI, prevents both inlines,
  and leaves `_doexit` 32 bytes short with two extra call relocations.
- Do not remove the explicit zero initializers: VC7 emits COMMON declarations,
  so the object no longer owns January's BSS even though all values are zero.
- Do not cache `__onexitend` in a local; the target intentionally reloads the
  global after callbacks.
- Do not replace the callback loops with library helpers, assembly, volatile
  accesses, pragmas, byte patches, or altered compiler flags.

## House-rule, Berth-rule, and bug audit

- The source uses descriptive CRT names and ordinary readable C; there are no
  anonymous raw offsets, magic structure layouts, assembly blocks, `volatile`,
  forced inline, undefined aliasing, byte forcing, or optimizer pragmas.
- The only numeric constant is the semantic CRT exit-lock index, named
  `_EXIT_LOCK`; zero and one are boolean/status values inherent to the API.
- Private versus public ownership follows the XDK member instead of csplit's
  placeholder defaults.
- The unused `code` parameter and the owned-but-unreferenced `_C_Exit_Done`
  global are authentic Xbox CRT behavior, not bugs. No original defect is
  proven, so no `BUG (original)` annotation is appropriate.

## Residual classification, reopen criteria, and disposition

There is no residual. The complete object is strict exact and admitted as
`Matching`. Reopen only if stronger primary source provenance contradicts the
XDK 3911 member; no such conflict exists.

## Final build and admission gates

- The consolidated `halobetacache_build` and `libcmt_build` completed with
  exit code zero under the unchanged generated build.
- Regenerated progress reports this unit at 8/8 functions, 281/281 code bytes,
  16/16 data bytes, and one complete unit. At this lane's snapshot libcmt is
  39/212 objects, 41/476 functions, 2,506/55,015 code bytes, and 264/8,637
  data bytes.
- The semantic audit scanned 411 available units and 3,478 functions, found
  zero unit errors, and retained 3,394 accepted exact functions. The admission
  audit found zero candidates, zero revocations, and only the unrelated,
  pre-existing `source/shell/shell_xbox` contradiction.
- The clean-tree fail-closed regression snapshot/check reports all eight
  functions `still_exact`, no changed non-exact sibling, no unexpected new
  match, no warning, and no ownership failure.
- All 179 tooling tests passed. JSON parsing, the prohibited-construct scan,
  `git diff --check`, final scope review, and working-tree cleanliness passed.
