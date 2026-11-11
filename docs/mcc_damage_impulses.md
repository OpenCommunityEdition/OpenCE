# MCC damage acceleration

## Native MCC verification, 9 November 2026

The world-space interpretation in the historical record below is superseded.
The shared MCC adapter now treats the three values as per-axis multipliers of
the damage direction. There are no map or tag name checks.

Read-only inspection of the installed Steam `halo1/halo1.dll` established the
following behavior. SHA-256:
`0a12dc561780f449d3f4d0df10bb8d3bc7be7840a5beb2b236f672eb6cd42e6c`.
Addresses below use its preferred image base `0x180000000`; they identify this
binary only. These are observations, not copied engine code or SDK declarations.

- At RVA `0xBA0988`, damage direction is copied, 0.45 is added to Z, and the
  vector is normalized through RVA `0xA9BD08`.
- At RVA `0xBA09B8`, damage flag 16 selects three floats at tag offsets
  `0x1F4`, `0x1F8`, `0x1FC`; otherwise the first float is used on all axes.
- At RVA `0xBA0A09`, each value is multiplied by the object's acceleration
  scale, optional native skull multipliers, and `0.033333335f`.
- At RVA `0xBA0AA7`, each scaled value is multiplied by its corresponding
  normalized direction component. The result is not normalized again.
- Unit admission calls RVA `0xC49A5C`: the squared three-component tag
  magnitude (or absolute scalar) must be at least `0.0001f`. Negative vector
  components do not require flag 17. Flag 17 is passed separately to object
  acceleration routines; it does not select world-space coordinates.
- The biped routine at RVA `0xBAFA80` still halves all three components for
  living bipeds. This change preserves that behavior.

The port retains its legacy scalar calculation and admission outside MCC
extension effects. Invalid or overflowing extension results are rejected.
No native skull behavior is introduced. Peers must agree on the impulses:
network version 43 separates builds with this calculation from version 42
builds, which had the inferred world-space vector.

Regression coverage extracts the production adapter and damage dispatch.
It checks cardinal and opposite directions, signed and unequal components,
zero direction, admission thresholds, object scale, immunity, object classes,
invalid values, and map-family transitions. Aerial's melee, shotgun, pull and
pad values and the previous Mercury2 vector are fixtures, not special cases.
Negative controls restore world-space handling and the old admission gates.
This verifies the calculation, not native MCC gameplay or full mod playthroughs.

Local validation: 194 focused tests passed, including the damage dispatch,
stale-player damage, player-indicator and MCC tag-validation suites. Windows
and web builds passed. Both changed Android guest units compiled and assembled;
the full Android build remains blocked by Windows-host compiler-launcher and
POSIX-command incompatibilities. A hidden Windows run loaded Blood Covenant
Mirrored with its bundled resources and advanced more than 500 simulation
ticks. It was a direct map-load smoke test, not a multiplayer movement test.

## Historical investigation (superseded coordinate interpretation)

> Component evidence from the source branch the MCC support came from.
> Protocol numbers below are that branch's; MCC maps now play with OpenCE's
> network version (`HALO_PORT_NETWORK_VERSION`, [NETCODE.md](../port/linux/NETCODE.md)).
> The [reading guide](reading/README.md#scope-and-provenance) follows the sources.

Issue #27 concerns Mercury2's gravity wrench. The supplied version-13 map's
`weapons\gravity_wrench\projectile\beam` damage effect has flags `0x30000`
and acceleration components `(0, -0.1, 2.45)`. Its `gravity_up` effect has
`(0, 0, 2.1)`. The author screenshot in the issue identifies bit 16 as
"use 3d instantaneous acceleration" and bit 17 as
"allow any non-zero acceleration value". The reporter confirms upward lift
rather than pulling toward the player's aim.

The implementation interprets these as world-space components. That coordinate
interpretation is inferred from the map data and reported behavior, not from
MCC executable code. The legacy interpreter reads only the first component,
which is zero in both effects, and admits only positive unit impulses.

`mcc_damage.c` reads the extra components only while an MCC cache is active
and the extension flags are present. It retains the object's acceleration
scale and the native 30 Hz conversion. Flag 17 admits signed nonzero impulses;
without flag 16 the original directional scalar calculation remains in use.
The MCC metadata reader bounds-checks damage roots and rejects non-finite
extension components. The runtime adapter also checks scaled results before
passing them to physics.

The small `port:` addition in `object_damage_aftermath` is necessary because
a three-component impulse cannot be converted to the existing single scalar
at load time. Projectile, item, biped and vehicle movement still use their
existing functions, including unit immunity and native biped scaling. Ordinary
MCC damage and all Xbox/Custom Edition damage keep the original calculation
and admission condition. There is no persistent adapter state to carry across
map loads.

No network message or client permission is added. Existing damage admission,
host decisions and client replay call the same aftermath path. MCC protocol
48 prevents peers using the previous physics behavior from joining these
sessions; the verified OpenCE protocol remains 29.

Regression tests compile the production adapter and acceleration dispatch,
including upward and signed motion, object classes, immunity, non-finite data,
overflow and legacy/map-transition behavior. Negative controls remove the
vector handling or unit admission and must fail. Reader tests exercise the
actual MCC metadata preparation with valid and truncated damage tags.

## Validation for issue #27

- Windows release build; 239 focused tests passed, one asset-dependent test
  skipped. These include existing damage ownership, packet direction, MCC
  protocol, boat, recording, reader and lifecycle checks.
- Hidden Mercury2 singleplayer and two-instance co-op probes used the map's
  actual wrench damage effect. A private copy exposed that tag in the scenario's
  console-reference list; no acceleration values or compiled scripts were
  changed. Temporary diagnostics confirmed the same `(0, -0.0023, 0.0572)`
  player impulse on host and client. Diagnostics are not shipped.
- The clean build loaded Mercury2, Xbox A10, Custom Edition Death Island,
  then Mercury2 again in one process without assertions.
- The initial console probe lacked that reference and asserted; a forced
  teleport probe also asserted before applying damage. Neither is counted as a
  successful test. Normal-flow probes completed without assertions. Logs and
  compressed private test saves were retained locally.
- These are headless integration checks, not a visual comparison with native
  MCC or a complete campaign playthrough. Weapon feel and arbitrary mod-authored
  combinations still need player testing.
- Android and web build targets were unavailable in this Windows configuration;
  Linux was not built locally. Browser transport testing could not start because
  Playwright was absent. No packet layout or transport implementation changed.
