# Native scripting scalar endian fix

2026-10-15, based on OpenCE 8102e711. This fixes native scripting slot
readers/writers; it does not convert serialized little-endian map data.

## Contract and changes

Every script value occupies a four-byte slot. Boolean and short values use
the numeric low 8/16 bits. Signed short reads retain their signed type, while
short results are zero-extended into the slot (so -1 is stored as 0x0000ffff).
Real and pointer slots retain all 32 bits. Existing boolean conversion behavior,
including the source's `n == 0` tests, is preserved.

`source/hs/hs_value.h` provides endian-aware typed fields/aliases plus narrow
stores that preserve unused high bits and a size-aware equality helper.

- `hs.c`: all 18 narrow typed argument structures now explicitly occupy four
  bytes per field; narrow members retain their signed/unsigned types. Result
  and evaluation unions place narrow aliases at the numeric low bits. The
  one raw unsigned-short argument reader now reads a full slot and casts.
- `hs_scenario_definitions.h`: syntax-node boolean/short aliases share the
  same layout. This corrects existing parser writers and Custom Edition
  script readers/writers without duplicating architecture branches at every
  call site. Syntax nodes remain 20 bytes with data at offset 16.
- `hs_runtime.c`: fixed conversions, equality and logical reads/results,
  sleep-until defaults, and all 19 narrow-short external-global reads plus
  the boolean read. Existing external-global numeric-cast writers now agree
  with the readers. External storage itself remains normally typed.

Little-endian typed argument offsets are preserved; trailing narrow argument
structures now include their full slot padding. They describe argument buffers,
not serialized structures. No sizeof uses of these argument structures were
found. Equality preserves old width-limited comparison, including raw real-bit
equality: positive and negative zero still compare as different bit patterns.
Unused upper bits remain ignored for boolean/short equality and retained by
narrow external-global stores. No network format changed; existing supported
little-endian behavior is retained, so the protocol version was not changed.

## Verification

The modern host generates `script_slots.c` by extracting actual current engine
functions, macros and layouts with `tools/mac_script_probe.py`. The harness
supplies only a small fake stack, syntax table, globals and engine services.
It tests independent numeric results, not just matching writers/readers.

Built on the Intel build Mac with Apple GCC 4.0, -O2, -fms-extensions and
-fno-strict-aliasing. PPC uses the 10.4u SDK and deployment target 10.4;
the Intel control uses -arch i386. Actual results:

| Host | Patched code | Forced legacy field layout | Restored legacy short store |
| --- | --- | --- | --- |
| PowerBook G4, Leopard | 318 checks, 0 failures | 45 failures, exit 1 | 36 failures, exit 1 |
| iBook G4, Leopard | 318 checks, 0 failures | 45 failures, exit 1 | 36 failures, exit 1 |
| MacBook, Snow Leopard i386 | 318 checks, 0 failures | 318 checks, 0 failures | 318 checks, 0 failures |

Coverage includes:

- Signed extrema, -7/-1/0/1/7/32767, conversion chains and zero extension.
- Actual parser boolean and integer writes, syntax aliases, and a high-bit
  external-global designator as used by Custom Edition conversion.
- Every typed argument structure's fields and slot indices, including following
  long/real/string fields; actual short/boolean return macros and word-to-real
  evaluator macro.
- Actual short/boolean external-global read/write functions, dirty upper bits,
  and all additional narrow external-global types.
- Actual equality for sizes 0/1/2/4, including signed-zero real bit patterns.
- Logical initialization, resumed evaluation and result handling; actual
  sleep_until default condition/ticks and resumed completion.

Two deliberately broken variants are generated from the same source. They
fail on both PPC machines but pass on Intel, reproducing the architecture
dependency. Their exact exit status 1 was checked, not merely any failed SSH
command. An independent source review found no actionable correctness defects.

During harness development, fake-service macros initially shadowed fixture
variables, and a boolean-only stack fixture was initialized as a numeric long.
Those harness errors were corrected before the results above. They were not
production engine fixes. The iBook has no tar in its SSH PATH; use stdin file
transfer there instead of repeatedly retrying tar.

## Reproduce

On the modern host, from the repo root:

```sh
python3 tools/mac_script_probe.py build/mac-audit/script_slots.c
python3 tools/mac_script_probe.py build/mac-audit/script_slots_legacy_layout.c --legacy-layout
python3 tools/mac_script_probe.py build/mac-audit/script_slots_legacy_store.c --legacy-store
```

Copy those generated files and `tools/mac_script_checks.sh` to the MacBook.
Run the script with MAC_CHECK_DIR set to the generated files' directory. It
builds all PPC binaries and builds/runs all three Intel controls. Copy the PPC
binaries to each G4, run the patched executable (expect 0), and run each
negative control (expect 1). Probe staging remains
`$PROBES/20261015`; raw local evidence is in `build/mac-audit/`.

`git diff --check` passed. The required Android check was attempted after
configuration, but `ninja android` reported an unknown target in this local
environment; it did not compile Android. Neither a full native game build nor
an in-game scripting session was possible. These tests establish the covered
native scalar boundaries, not overall engine or map correctness.

## Next toward Blood Gulch

The reproduced scalar bug is no longer the active blocker. Next establish a
complete map-field conversion inventory and decode little-endian map/script
data into this native layout before use. The runtime aliases cannot be used
as on-disk byte offsets. Continue full-engine compiler/linkage/TLS work and
actual NV2A shader translation on GLSL 1.20. A playable map and game-data path
remain outstanding; do not report map execution based on these isolated tests.
