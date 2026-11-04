# DualSense format references and notices

The feature was imported from [OpenCE PR #549](https://github.com/OpenCommunityEdition/OpenCE/pull/549),
authored by PeterRichardsonGolf, with source commits
`80a1964`, `303a99f` and `2944896` recorded by `cherry-pick -x`.
The game profiles, scheduling and integration live in `port/linux/`, not here.
This folder retains notices for the public format references used by that code.

## Trigger effects

John "Nielk1" Klein's MIT-licensed TriggerEffectGenerator, revision 6:
[ExtendInput.DataTools.DualSense.TriggerEffectGenerator.cs](https://gist.githubusercontent.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db/raw/74fc790e5fa3cd6ad278233a7636553eddca900d/ExtendInput.DataTools.DualSense.TriggerEffectGenerator.cs).
Gist revision: `f73a299fdc52f43be84d39e62734192ad82bb393`.
SHA-256 of the UTF-8 source: `7ad1cc12615f7cc4657c72a797b798857ec176ad32fd91e812e10fd1f454e56e`.
The complete copyright and permission notice is preserved in
[TriggerEffectGenerator-LICENSE.txt](TriggerEffectGenerator-LICENSE.txt),
with only the C comment delimiters removed.

`port/linux/src/dualsense_rules.c` uses the Feedback, Weapon and Vibration
packing: the ten-zone mask and packed three-bit force values. No C# source
is compiled. The notice accompanies source and binary distributions.

## SDL PS5 output

[SDL release-3.4.16](https://github.com/libsdl-org/SDL/tree/release-3.4.16),
commit `fa2c02bb6e21974a89ea9824bc53c9932abe5f9c`, zlib licence.
Reference: `src/joystick/hidapi/SDL_hidapi_ps5.c`;
SHA-256: `31bcd932cec0b223422beda3c9b5e46835d85dfcae8607c836865828e56241e2`.
Its output-body layout, player-light patterns and report framing inform
`dualsense_rules.c`; `dualsense.c` also reproduces the default player-index
lightbar palette. [SDL-LICENSE.txt](SDL-LICENSE.txt) is copied unchanged.
SDL's source is otherwise obtained by the existing platform build scripts.

These are public third-party sources, not proprietary Xbox SDK or MCC material.
