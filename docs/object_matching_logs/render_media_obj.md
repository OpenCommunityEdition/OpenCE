# render_media.obj matching log

## Result

`source/render/render_media.obj` is strict-complete: all 4 functions and all
three target-owned data sections match the January 14, 2002 object under XDK
3911 CL 13.00.9254.1 with `/O2 /Oy- /DDEBUG /Dxbox`.

The object was already labeled `Matching`, but the hardened comparator found
that the two lifecycle functions referenced a private static symbol named
`_render_media_globals`. January instead owns the 44-byte aggregate as the
external BSS symbol `_bss_004c004c`. Giving the aggregate its January owner
and an explicit zero initializer preserves BSS allocation while closing the
relocation-identity false positive.

## Current-campaign salvage audit

Local commit `f4621ba9` was not reachable from authoritative campaign tip
`9b7f8c54`. It was therefore treated as an untrusted candidate, transplanted
into an isolated worktree, rebuilt with the campaign compiler, and rechecked
from scratch. The pre-transplant object demonstrates why ordinary percentage
credit was insufficient: its code hashes, sizes, and relocation addresses all
looked exact, but the two lifecycle relocations named the private static
`_render_media_globals`. January's relocations name external
`_bss_004c004c + {0,4}`. `section_infos_equal` consequently rejected both
lifecycle functions before this correction.

In plain English, the instructions were already right but they pointed at a
global owned under the wrong COFF name. That distinction matters to the linker
and to later translation units, so this is an integrity repair rather than a
cosmetic rename.

## Measured source-shape matrix

| ID | One changed factor | Measured result | Disposition |
| --- | --- | --- | --- |
| `O00` | Campaign baseline: file-static `render_media_globals`, with the two explicit alignment declarations retained. | Code bytes and relocation addresses matched, but strict relocation identity resolved to `_render_media_globals + {0,4}` instead of January's `_bss_004c004c + {0,4}`. | Rejected. |
| `O01` | Rename the aggregate to external `bss_004c004c` without an initializer. | The four functions still compared equal by same-name relocation fallback, but VC7 emitted a 44-byte COMMON symbol (`section=0`, `value=44`) and removed the target-owned `.bss` section. | Rejected: function-only checking would miss a destructive ownership regression. |
| `O02` | Give external `bss_004c004c` an explicit `= {0}` initializer. | Restored a 44-byte uninitialized section with external storage class 2. Both lifecycle relocations and the full BSS section became strict exact. | Accepted. |
| `O03` | Remove both `__declspec(align(8))` declarations from the two initialized frame-level-of-detail globals. | The compiler retained the target 172-byte `.data`, offsets 0 and 88, section flags, payload hash, and ownership. | Accepted; the directives were redundant and violate the house preference against gratuitous alignment controls. |
| `O04` | Add a compile-time assertion that `frame_level_of_detail` itself is 88 bytes. | Compilation failed: the actual structure is 84 bytes; the observed 88-byte second-symbol offset includes four bytes of inter-object alignment rather than tail fields. | Rejected as a false layout inference. |
| `O05` | Assert the proven 84-byte frame structure and 44-byte globals structure; rename the opaque hexadecimal byte count to an honest 36-byte `unknown` tail. | Rebuilt byte-identically. All four functions and all three non-code sections remained strict exact. | Accepted as readable, byte-neutral type documentation. |

## Strict evidence

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `render_media_initialize_for_new_map` | 32 | 2 | `57c233928260a5763e7d1b8b2bd907a874cfab8bedda4d4095e84a1c679a61aa` |
| `render_media_dispose_from_old_map` | 16 | 1 | `6d756ab123ca36d54d960697fa16bc6dc86933084aa5b92457f2b58c5174722d` |
| `render_media_build_render_target` | 16 | 0 | `1c93847ee0f29c8102c08f41566abe2ae27a71ab314eec709e90e3a28353e543` |
| `render_media` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

Target-owned data also passes `section_infos_equal`:

- `.rdata`: 4 bytes (`media_wave_amplitude`).
- `.data`: 172 bytes (both frame-level-of-detail definitions at offsets 0
  and 88).
- `.bss`: 44 bytes, owned by `_bss_004c004c`.

The lifecycle relocations now resolve exactly to `_bss_004c004c + 0` and
`_bss_004c004c + 4`, at the same addresses and with the same COFF types as
January.

## Alignment and source-shape audit

The original reconstruction put `__declspec(align(8))` directly on both
frame-level-of-detail globals. A bounded removal experiment produced the same
172-byte `.data`, the same symbol offsets, the same `0xC0400040` COFF section
flags, and strict equality for every code and data section. The directives
were therefore unnecessary and removed per the project house rules.

No assembly, volatile qualifier, codegen cast, pragma, flag change, undefined
behavior, or byte patch is used. No-argument definitions put `void` on its own
line, and every void function ends with an explicit `return;`.

The four routines also use a single return where practical. There are no tag
or object datum accesses in this unit, so the typed `tag_get`, `object_get`,
and `try_and_get` macro rules are not applicable. The remaining unknown bytes
are represented honestly inside a size-asserted structure; no function uses a
raw field offset or magic-offset cast.

## Final current-tip admission gates

- Source base: authoritative campaign tip `9b7f8c54`; candidate commit was
  treated as untrusted and independently rebuilt with XDK 3911 CL
  13.00.9254.1 under unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags.
- Strict function sweep: 4/4 `section_infos_equal`, with the sizes,
  relocations, and hashes listed above.
- Strict non-code sweep: `.rdata` 4 bytes, hash
  `0b9d79ff0fc4d0a77ef398763bbe45c445501e826219f2b6a1aa709411ab631d`;
  `.data` 172 bytes, hash
  `4d659bb0aa00516bc11d04b523145fdc0c6e63c63fad53affec287cc09b0a3aa`;
  `.bss` 44 bytes, hash
  `85759b3811ff7dc47b03792ac85317be51431a3f9e01dcafce317ed736a391b0`.
  All target/candidate owner symbols have external storage class 2 and matching
  section-relative values (`0`, `0/88`, and `0`, respectively).
- `ninja halobetacache_build`: exit 0 for the complete 466-object build.
- `ninja progress`: exit 0; 379 units scanned, 3,274 functions evaluated,
  3,184 accepted exact, and zero unit errors. The repair moves the two
  lifecycle functions out of the ordinary-only ownership gap; campaign object
  count does not rise because this unit had already been prematurely labeled
  `Matching`.
- Whole-TU regression snapshot/check: all four functions `still_exact`, zero
  changed non-exact siblings, failures, or warnings; the manifest also freezes
  the corrected `.rdata`, `.data`, `.bss`, and symbol ownership.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 173/173 tests pass.

No semantic exception, parked credit, fuzzy percentage, or comparator
weakening is used for this admission.
