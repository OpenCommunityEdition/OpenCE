# MCC damage acceleration

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
