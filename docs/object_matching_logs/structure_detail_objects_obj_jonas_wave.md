# `structure_detail_objects.obj` Jonas bounded wave

Date: 2026-08-20

Base: `6efbc916e2f34b9ad35e4d4d7dacf5aec68f9748`

Unit: `source/structures/structure_detail_objects`

## Retained result

This wave retained one newly exact defined-C function and the naturally
associated typed TU-owned storage:

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_structure_detail_objects_flush` | 13 | 16 | 1 | `fbb36761897a1c0eb2d6a8cb5cfe2437339527d4186855f36ee77c394e8db621` |

The two pre-existing one-byte return leaves remain exact, so the object moves
from 2/15 to 3/15 exact functions. Direct objdiff credit moves from 2 to 15
meaningful code bytes.

The target and rebuilt candidate also have the same `.bss` shape:

- 20 bytes, four-byte alignment, zero-filled, no relocations;
- sole external owner `_debug_detail_objects` at offset zero;
- direct objdiff reports the section 100% exact, adding 20 data bytes.

The retained function has the target instruction shape
`mov runtime_pointer; mov byte [runtime_pointer+0x520E], 0; ret`. Its sole
relocation resolves to `_debug_detail_objects+4` in both objects.

## Layout evidence

The typed layout is constrained jointly by the target COFF and the supplied
binary-backed HCEA oracle:

- one player's runtime record is `0x5210` bytes;
- its initialized byte is at `+0x520E`;
- two player records occupy `0xA420` bytes;
- the global runtime record's default-Z `real_vector4d` begins at `+0xA420`,
  making the complete record `0xA430` bytes;
- the TU storage has the public debug boolean at `+0x00`, the runtime pointer
  at `+0x04`, the offset flag at `+0x08`, and two `real` offsets at `+0x0C`
  and `+0x10`.

Unknown runtime payload remains explicitly reserved bytes. Compile-time size
checks cover the `0x5210`, `0xA430`, and `0x14` records. No integer-address
reconstruction, union reinterpretation, optimizer control, or scheduling
device is used.

## Bounded candidate result

The requested compile was performed once. Two candidates were rejected and
their bodies were removed without residual tuning:

| Candidate | Target / candidate shape | Result |
| --- | --- | --- |
| `_structure_detail_objects_initialize` | both 80 bytes and 3 relocations | Nonexact: target places the `_debug_detail_objects+4` relocation at byte 51, while the candidate places it at byte 48 because the stack cleanup and global store are scheduled in the opposite order. Target hash `99e687328f033302450f35257d1339d168279f9876fcd14b9180ddaa52f618d5`; candidate hash `46c693d13661a9d71dd5516590b0c9d7c29e22dc1cc38eea6ff94d60c88ddd82`. |
| `_detail_object_offset` | both 48 bytes and 4 relocations | Nonexact: the target stores the final offset before the flag, while VC7 emits the flag before the final offset. Target hash `02472075f67e06f60e5d248b9bce9d31fe5d55eeb6c40f06d9fd33753bca165d`; candidate hash `dfd0dbb77ddb40830499c0d89ba0f83585a795706973d99902f8871b75097311`. |

`_dot_product4d` and `_structure_detail_objects_initialize_for_new_map` were
excluded from the wave and were not authored or tested as candidates.

## Validation

- Full `halobetacache_build` and `libcmt_build`: pass. The emitted warnings
  are pre-existing warnings in unrelated translation units.
- Strict COFF comparison: the new flush and both baseline dispose leaves are
  exact; sizes, normalized bytes, and relocation semantics all agree.
- Direct report for this object: 3/15 functions, 15/2508 meaningful code
  bytes, and 20/200 data bytes.
- Semantic report: 470 units, 3,967 evaluated functions, 3,827 semantic exact,
  101 hidden exact, 36 ordinary-only, 0 unit errors, and 3,888 accepted exact.
- Aggregate progress in this worktree: 375/833 objects, 3,875/11,060
  functions, 471,577/2,198,102 code bytes, and 1,834,828/4,176,062 data
  bytes.
- Object-admission audit: zero candidates, one known unrelated contradiction,
  zero revoked entries.
- Parked-function audit: 3 active, 0 stale, 0 invalid.
- Tool test suite: 179 tests pass.
