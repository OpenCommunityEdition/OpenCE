# `recorded_animation_playback.obj` scalar event handlers

## Result

Five playback event handlers are strict semantic-COFF exact:

- `code_00081ef0` (animation state), 139 meaningful / 144 padded bytes;
- `code_00081f80` (aiming speed), 140 meaningful / 144 padded bytes;
- `code_00082010` (control flags), 144 meaningful / 144 padded bytes;
- `code_000820a0` (weapon index), 144 meaningful / 144 padded bytes; and
- `code_00082130` (throttle), 151 meaningful / 160 padded bytes.

Together they add 718 matched meaningful code bytes. The object advances from
3/14 to 8/14 exact functions and from 115 to 833 matched meaningful code
bytes. It remains `NonMatching`; no completion label, semantic exception,
data credit, or parked entry is added.

All five functions reproduce their target padded bytes and all twelve REL32
relocations, including relocation address, type, symbol ownership, and addend.
Their target/rebuilt normalized SHA-256 values are, respectively:

- `6e402720f57b3f225404e2009a257ac0dd1f57810fe09f2e47d54d560791e020`;
- `5ae6e6a144e8ea9cc1d31492c1a3394e492627480132a41f4b03fe0af09685a8`;
- `0308ef6d71c78870ca6452b9deac923023229140003a13417b34c227b5fa8ce8`;
- `94be717be57a6941630142216322c5270942bc7bfd56033182209433d023f5d5`; and
- `4b5eb28056964440e8985a218be61632eafba3f8bd730c52b6f00f7f67845f7a`.

The two initializer functions and byte-swap stub remain strict exact.

## Evidence and provenance

January's split-object disassemblies establish the complete assertion order,
source-line constants, payload widths, stream advances, and control-record
stores. The exact line sequences are 25/25/25 for animation state,
26/26/26 for aiming speed, 27/27/27 for control flags, 28/28/28 for weapon
index, and 33/35/36 for throttle. The target's one-byte event header naturally
decomposes into a two-bit time delta and six-bit event type.

The later-build HCEA donors under
`build/audit/refs/halocea/src/blam/models/apply_animation_state_0.c` and the
sibling playback-handler files independently authenticate the handler
semantics and typed control fields. They are semantic/type provenance only;
January's target bytes and relocations remain authoritative.

Every reachable Claude branch was searched before the shot. Hits were limited
to skeleton/configuration/residual-frontier material and the string synopsis.
The synopsis independently authenticates the exact assertion strings. No
prior source candidate, compile measurement, exclusion matrix, fixed point,
or object-specific ledger was found for these handlers.

The first animation-state candidate read through `**playback_stream` directly.
It matched the target's 144 padded bytes and twelve relocation identities but
placed the relocations at different offsets. Target/candidate disassembly then
showed that the target keeps the payload pointer live before entering the
assertion sequence. A genuinely new evidence wave introduced the named
`event_data` local before the assertions; the retry became strict exact. That
exact object was frozen as the sibling baseline. The other four handler
families were then compiled once together and were exact on their first
production shot.

The two- and eight-byte payloads use ordinary standard-C `memcpy`, avoiding
unaligned typed dereferences and representation punning. The retained code has
no assembly, explicit intrinsic, raw address, pointer/integer reconstruction,
pointer or union pun, `register`, `volatile`, pragma, annotation, barrier,
synthetic reference, byte forcing, or compiler-flag change. Parameters are
vertical and every void function ends with an explicit `return;`.

## Reproducibility and gates

- Frozen pre-handler object:
  `build/audit/recorded_animation_playback_animation_state_baseline_20260827.obj`,
  raw SHA-256
  `644ec65670740dbdc5b3611329fb2a3233c0a9f6b0408dd619a7edb9b8140476`.
- Direct-read miss:
  `build/audit/recorded_animation_playback_animation_state_direct_miss_20260827.obj`,
  raw SHA-256
  `abd91209729bcc699d241cca168f00ee5cb08e3c944f5fb86c397236ac6ad21f`.
- Frozen exact animation-state baseline:
  `build/audit/recorded_animation_playback_scalar_handlers_baseline_20260827.obj`,
  raw SHA-256
  `d1942a8fc126f28019e3586207285ef22cd165360e74902b3baf4b63a462b7b5`.
- Retained five-handler object:
  `build/audit/recorded_animation_playback_scalar_handlers_retained_20260827.obj`,
  raw SHA-256
  `8f01fe91f9b5cba988fb72aed7fbf9ff52c8dbf9c15d65838c9b22ab155faba2`.
- Retained source SHA-256:
  `a7316afc8e1bb9b82bf0f379e1cb6af013fb0ba96da106a5c3e690b1f27040c7`.
- Production-flag XDK 3911 compilation: pass.
- Hardened comparison: all eight emitted functions exact, `all_equal: true`.
- Halo and libcmt build graphs: pass.
- Semantic audit: 470 units, 4,408 functions evaluated, 4,262 semantic exact,
  4,322 accepted exact, and zero unit errors.
- Campaign progress: 377/833 objects, 4,295/11,060 functions, and
  516,190/2,198,102 code bytes. Halo progress: 275/468 objects,
  4,128/7,574 functions, and 503,276/1,770,166 code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: three active, zero stale, and zero invalid entries.
- Tooling tests: 179/179 pass, with only the inherited unwritable pytest-cache
  warning.
- Protected source, configuration, semantic ledgers, comparator/admission
  tooling, compiler flags, and object status are unchanged.

This checkpoint claims five exact functions and 718 meaningful code bytes,
not a completed object or any data ownership.
