# Fixtures

`sine.ogg`: a tenth of a second of a 441 Hz sine at half scale, mono at
22050 Hz, made for these tests (CC0, like the repository). It was written by
a short Python script as a WAV file and encoded with
`nix shell nixpkgs#vorbis-tools -c oggenc -Q -q 4 -o sine.ogg sine.wav`.
