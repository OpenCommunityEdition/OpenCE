# Authored MCC biped seats

FloodSlayer (`c10_flood.map`, Workshop 3629556661) gives several bipeds a
`stand` seat and gives its playable infection form matching entry/exit
animations. This is authored unit data, rather than a missing script function.
The Xbox nearby-biped interaction was empty, its camera lookup accepted only
vehicles, and its biped update treated all biped attachments as clinging
infection forms. That last case interrupted the seat animations.

`mcc_seats` enables ordinary action-button boarding for these MCC seats, uses
the authored seat camera, and keeps real seat riders out of the infection
attachment animation. Native seat animations and script-controlled exit
remain in use; a dead parent releases its riders. Attachments without a valid
seat keep their original infection behavior. The module is stateless and checks
the active MCC map before accessing objects.

Small shared hooks are necessary in nearby-biped interaction, action handling,
camera lookup and biped animation selection: those engine paths have no
registration interface. The network seat receiver additionally admits validated
MCC biped parents through its existing identity, bounds and cycle checks. Xbox
and Custom Edition retain their existing branches. No loader, tag limit, script
table, wire format or client authority is changed.

The host alone accepts boarding and action-button exit. Clients receive the
existing seat and object state; they cannot upload authoritative biped movement
through vehicle prediction. Network version 39 excludes peers that would reject
the biped parent. This uses the common co-op/multiplayer simulation.

## Validation

- The MCC/cache suite passed 1,151 tests, with six skipped. New tests execute
  the real MCC adapter and shared seat receiver, covering legacy isolation,
  authority, proximity revalidation, malformed identities/indices, attachment
  cycles, occupancy, camera selection and parent death/exit.
- Hidden two-machine Windows tests exercised remote action-button boarding,
  animated exit, scripted exit and parent death. Host/client samples agreed on
  rider, parent and seat. Co-op used a private FloodSlayer fixture with its
  background scripts dormant. The multiplayer fixture additionally supplied
  multiplayer globals/spawns and one common starting BSP; it is not a claim
  that the unmodified campaign is a multiplayer map. No fixtures are shipped.
- A late-joining client received an already-seated host player and subsequent
  boarding, exits and parent-death release without assertions in the targeted
  rerun. An earlier late-join run logged one nonfatal `desired_angles` assertion
  during player initialization. It did not recur with seats disabled or with
  added camera diagnostics; its cause remains unconfirmed. Retain this as a
  playtest caveat, rather than claiming every late-join run was clean.
- The normal portable release ran the original FloodSlayer opening, then
  transitioned to Xbox A10, Custom Edition Hugeass and back to FloodSlayer.
  Delayed script markers passed in each stage, with no detected assertion,
  conversion refusal or script-recompile error; the process exited normally.
- Android/Web Ninja targets were unavailable in the Windows configuration.
  Browser transport tests could not start without Playwright. The attempted
  broader touch/Web checks also encountered Windows harness linking, missing
  POSIX headers and absent Web outputs. These are not reported as passing.

Review was a component and combined-diff self-review. Loopback tests exercise
the common network path, but do not establish WAN/browser interoperability or
a full FloodSlayer campaign playthrough. Human visual/gameplay testing remains
necessary, particularly with AI-controlled bodies and checkpoint reverts.
