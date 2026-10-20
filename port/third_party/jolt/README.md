# Jolt Physics

A rigid body physics engine in C++17, by Jorrit Rouwe and its other
authors, MIT licensed (see `LICENSE`).

Upstream: https://github.com/jrouwe/JoltPhysics, release `v5.6.0`, commit
`e77f175595e64cb44218cc9d9d56fc365ad0e36a`, from the release archive
(`v5.6.0.tar.gz`) SHA-256 6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e.
`Jolt/` and `LICENSE` are copied unchanged from it; this README is ours.

Tree checksum: `28620a7079e04a38bde6432fb4332137a57e40ce9c3359d45650bc6db1b94081`
(the SHA-256 of each file's path and SHA-256, in path order, without this
README), checked by `tools/test_macos_physics.py`.

Only the macOS host uses it: the ragdolls and the effects' debris
(`port/macos/host/physics`, reached through the C interface `physics.h`).
It is built single-threaded, as C++17, with
`JPH_CROSS_PLATFORM_DETERMINISTIC` and without fused multiply-add, so that a
run repeats exactly. Its compute and GPU back ends (`Jolt/Compute`, the
`.hlsl` shaders) are not built.
