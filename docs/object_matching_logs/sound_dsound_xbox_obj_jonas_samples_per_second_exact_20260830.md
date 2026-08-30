# `sound_dsound_xbox.obj` sample-rate getter exact recovery — 2026-08-30

## Result

The first and only production candidate recovers
`_sound_samples_per_second` as strict byte-exact C.  The two inherited exact
functions remain exact, so `source/sound/sound_dsound_xbox.obj` advances from
2/39 to 3/39 exact functions and from 36 to 113 meaningful exact code bytes.
It remains `NonMatching`; this wave changes no object label, semantic
exception, parked record, shared header, or protected source.

| Function | Meaningful / padded | Ordered relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_sound_samples_per_second` | 77 / 80 | 6 | `e8b9fca56aaa9d922301b77d72c6debc700ceb7de1d899966324a31b29e24547` |
| `_dsound_angle_from_angle` | 18 / 32 | 2 | `ea6279c6d854d47a52aaff3dd5306713f73870f27fe26028f56f70f200e2386f` |
| `_dsound_get` | 18 / 32 | 2 | `384e7aada8b4cd93cf43c6609bf1a179a34d1843d6afb2fb90a8cd29c5f6c103` |

The retained three-function set is therefore 113 meaningful bytes, 144
padded bytes, and 10 ordered relocations.

## Preflight and provenance

- Base commit: `3ccb70e7714c80641820fe951e4d28daf6150a7d`.
- Accepted pre-wave `source/sound/sound_dsound_xbox.c` blob:
  `918e3b9b51f896607f53052e434cb99a906c92d8`,  SHA-256
  `b029240b735bdbe92872b15ce96fcab195b9171267f181bbc4fe04e2f337573c`.
- Retained source blob: `17417718ecfd94a9180b972ac0c8a3e1fc320031`,
  SHA-256
  `bf9701ba7a7e4d2947a887c0abdd2cc6a6d1a86706ed59d522a0d6bb8f1cf552`.
- January split object: 53,937 bytes, SHA-256
  `c7b04e9b80b2a397fb3c5562129c2c5b0681f483a659a05b6076d057fdea7aac`.
- Pre-wave candidate object: 1,897 bytes, SHA-256
  `29b67cf192816bb6f3c3904d0a3ec1c9299f285e50444465b08a508e7d8e27a5`.
- First-shot retained object: 2,694 bytes, SHA-256
  `a78ecfa96f5c458d9666498342ed24833f710883701ef4d0c239f099d69040da`.
- Compiler: Microsoft XDK 3911 C/C++ Optimizing Compiler
  `13.00.9254.1`; `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The project flags remain `/O2 /Oy- /DDEBUG /Dxbox` with the unchanged
  include graph.

Canonical `git log --all -S sound_samples_per_second` contains only the
initial symbol inventory.  A filesystem census of 1,557 campaign copies of
`source/sound/sound_dsound_xbox.c` found zero production definitions.  The
three existing object ledgers were read in full: they recover `_dsound_get`
and `_dsound_angle_from_angle`, and freeze the unrelated attenuation helper
closure, but contain no prior attempt or boundary for this getter.

The Stian corpus is pinned at
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; its readable sound file is blob
`b6e0b885935258230625a1617f56368275add556`, SHA-256
`a2c69c447b62c5074a9224977bff55f62576da8e8e7cf1c6593951e2255e835f`.
The equivalent Pastudan corpus is pinned at
`918af885935ec470a31256ecce9a977b12b01f80`; its file is blob
`f9b8eb2d7b531e174493292a575e7c7804cbc971`, SHA-256
`c34c7bc22df59613238581cb7d488cf9606e2afd3c0611571ca5aeeacf97a692`.
Those later PC lifts corroborate the two-rate lookup and assertion, but use a
32-bit parameter, a raw PC address, and a different symbol name.  They are
semantic provenance only and were not transplanted.

The Aerocatia/Demon corpus at
`ef8cbebd2d122b8876c9ad4c14e5503afb8b7c7e` independently supplies the named
`NUMBER_OF_SOUND_SAMPLE_RATES` enum and a typed two-entry table.  Its header
and source blobs are respectively
`79c54d0ca972b878a1c2333d2455af9d02acd898` and
`b64e0762fddc73630056ce9a4dfe3942a3af0bcf`; their SHA-256 values are
`5215f86889ab77084e67cb3fdd82defb2c2c8b9c2524b515b397d911fe2a0f53`
and
`3ccd849ba6e4c7902beccfa8c08eb62440b695d18ebe0b9f9646f817a26dd5f5`.
This is type and naming corroboration, not an Xbox byte claim.

## January contract and link closure

January defines one external type-`0x20`, storage-class-2 cdecl symbol.  Its
complete instruction stream reads a signed 16-bit stack argument, asserts
that it is in `[0, 2)`, and returns the indexed 32-bit table entry through
`EAX`.  The failure path falls through after `system_exit(-1)`, as emitted by
the canonical `match_assert` macro; no private or register-ABI callee is
involved.

The target's six relocations, in order, are:

1. DIR32 at `+27` to the exact
   `"c:\\halo\\source\\sound\\sound_definitions.h"` COMDAT;
2. DIR32 at `+32` to the exact
   `"sample_rate>=0 && sample_rate<NUMBER_OF_SOUND_SAMPLE_RATES"` COMDAT;
3. REL32 at `+37` to `_display_assert`;
4. REL32 at `+44` to `_system_exit`;
5. DIR32 at `+54` to `_sound_sample_rate_samples_per_second`; and
6. DIR32 at `+70` to the same table.

The canonical owner `source/sound/sound_definitions.c` already defines the
typed two-entry table as `{ 22050, 44100 }`.  Its target and candidate
`.rdata` are byte-identical (`22 56 00 00 44 AC 00 00` for the table, followed
by the inherited four-byte constant).  The getter therefore forms a natural
link-closed public leaf using an already exact external data owner.

The candidate adds only the named local rate-count enum, the typed external
table declaration, and this ordinary C89 body:

```c
unsigned long sound_samples_per_second(
	short sample_rate)
{
	match_assert(
		"c:\\halo\\source\\sound\\sound_definitions.h",
		0x135,
		sample_rate>=0 && sample_rate<NUMBER_OF_SOUND_SAMPLE_RATES);

	return sound_sample_rate_samples_per_second[sample_rate];
}
```

No shared header was changed.  The candidate adds exactly the two assertion
string COMDATs to the pre-wave non-code owner set and removes none.  Both are
byte-identical to their January sections: 59 bytes with SHA-256
`815c8859d4957b14f5e3e192d888c7b591784c25669b53eeb7bda39ff2d15bdf`
and 41 bytes with SHA-256
`d1c75e1e8c42300b2a1be3f9ab8445b9b571278645e1defe90d8257a4390f873`.

## One-shot discipline and strict comparison

The frozen source passed a no-output `/Zs` syntax check.  Exactly one normal
Ninja compiler edge then emitted the production candidate.  No expression,
declaration, condition, control shape, type, constant, flag, or tool was
changed after that compile.

Hardened comparison proves complete padded-byte and ordered-relocation
identity for the new function and both inherited exact siblings.  The
whole-TU exact-set gate reports no lost exact function and exactly the new
named getter (plus the comparator's anonymous `.text` pseudo-symbol).  The
protected Units `_unit_preprocess_node_orientations` sentinel remains strict
at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

The retained source uses readable typed C89 and named macro/data access.  It
contains no assembly, `volatile`, `register`, pragma, intrinsic, attribute,
optimizer barrier, raw address or offset, pointer/integer reconstruction,
pointer or union pun, undefined behavior, synthetic anchor, object patch,
compiler exception, comparator exception, or byte-forcing construct.

## Full validation

- Complete `halobetacache_build` and `libcmt_build`: pass.  The frozen object
  was already current and was not compiled a second time.
- Semantic audit: 470 units, 4,922 functions evaluated, 4,805 semantic exact,
  4,815 accepted exact, 131 hidden exact / 78,940 hidden code bytes, one
  ordinary structural entry, and zero unit errors.
- Campaign progress: 384/833 complete objects, 4,776/11,060 functions,
  607,087/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 matched data bytes.
- Halo progress: 282/468 complete objects, 4,609/7,574 functions,
  594,173/1,770,166 meaningful code bytes, and
  1,850,858/3,923,451 matched data bytes.
- Object admission: zero candidates, zero contradictions, zero revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Complete tooling suite: 212/212 tests pass; only the inherited optional
  pytest cache-permission warning remains.
- `git diff --check`, deleted-path review, protected-scope review, direct
  target comparison, exact-set loss gate, and prohibited-construct review:
  pass.

The regenerated report is 1,599,382 bytes, SHA-256
`d1b0fea1a8005d96c6bf9d2321317befd5dfcb514bdefe06ed6ca0fcd72d7cfc`.
The semantic report is 3,269,893 bytes, SHA-256
`43eb5b0b97bd3c8cdf5dd81671b1cb0a891ddaa1d8519705b9bed32427420044`.
The admission and parked records have SHA-256 values
`da978eb6caf00407c64347ef70362870c6429f551c6ac4e63563487a6e77eee5`
and `95028a9797685b4cadb863970db3ea9e4c940bdf7f0ab89df83699276844a849`.

Only `source/sound/sound_dsound_xbox.c` and this additive ledger are retained.
Nothing is pushed.
