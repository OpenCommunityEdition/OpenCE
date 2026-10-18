# Asset-free regression tests

The engine's real functions, taken from the sources and compiled against a small fake world: no game assets, SDK or
graphics, so a fix can carry a test that CI runs.

    python -m pytest -q tools/harness

They need `clang` (or `CC`) and a 32-bit C runtime (`lib32-glibc`, `gcc-multilib`), as the Linux build does. pytest
finds every `tests/test_*.py`, so a new test needs no change to CI.

On an arm64 Mac, which runs no 32-bit code, each test is built as the macOS host's guest code (ILP32 AArch64, as the
game runs there) and run in its VM (`tools/macos_guest_image.py`, docs/macos_host.md), with the same flags and
configuration as here; it needs `ninja macos` in `nix develop`. A test that runs its executable itself goes through
`process` (as `run` does).

## A test

- `tests/<name>.c`: the fake world, only what the code reads, with what it calls stubbed as macros that name only the
  arguments they use; then `#include "under_test.inc"` and a `main` that runs the case named on its command line,
  checked with `CHECK`.
- `tests/test_<name>.py`: takes the code, enums and constants from the sources (`function`, `inline`, `enum_with`,
  `constant`) rather than copying them, so the test checks what the game builds; `build` compiles it (warnings are
  errors) and each case is a pytest test.
- Negative controls: the code with a deliberate fault (`mutated`) must fail a named case's `CHECK`. A test that cannot
  fail proves nothing.

## Limits

Functions are taken by matching braces; one that is not found fails the test. Code with many dependencies is better
compiled as whole files against stub headers. Whatever needs real maps or the whole game is checked in the game
(offline: `debug.null_renderer`).
