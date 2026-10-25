//! Builds Jolt Physics (port/third_party/jolt) and its C interface
//! (port/macos/host/physics) into the crate, as tools/macos_build.py builds
//! them into the host: C++17, without fused multiply-add, in Jolt's
//! cross-platform deterministic mode, so that every run computes the same
//! results.

use std::path::{Path, PathBuf};

/// Jolt's folders the engine does not build: its compute and GPU back ends.
const SKIPPED: [&str; 5] = ["CPU", "DX12", "VK", "MTL", "Shaders"];

fn sources(folder: &Path, found: &mut Vec<PathBuf>) {
    let Ok(entries) = std::fs::read_dir(folder) else {
        return;
    };
    for entry in entries.flatten() {
        let path = entry.path();
        if path.is_dir() {
            let name = path
                .file_name()
                .and_then(|name| name.to_str())
                .unwrap_or("");
            if !SKIPPED.contains(&name) {
                sources(&path, found);
            }
        } else if path.extension().is_some_and(|extension| extension == "cpp") {
            found.push(path);
        }
    }
}

fn main() {
    // (OPENCE_ROOT for a copy of the workspace built outside the
    // repository, as cargo-mutants builds one)
    println!("cargo::rerun-if-env-changed=OPENCE_ROOT");
    let manifest = PathBuf::from(std::env::var_os("CARGO_MANIFEST_DIR").unwrap_or_default());
    let repository = std::env::var_os("OPENCE_ROOT")
        .map(PathBuf::from)
        .unwrap_or_else(|| manifest.join("../../.."));
    let jolt = repository.join("port/third_party/jolt");
    let wrapper = repository.join("port/macos/host/physics");
    let mut jolt_sources = Vec::new();
    sources(&jolt.join("Jolt"), &mut jolt_sources);
    jolt_sources.sort();
    if jolt_sources.is_empty() {
        println!("cargo::error=no Jolt sources in {}", jolt.display());
        return;
    }
    let mut build = cc::Build::new();
    build
        .cpp(true)
        // (the defaults add --target, which nix's compiler wrapper warns of)
        .no_default_flags(true)
        .std("c++17")
        .flag("-O2")
        .flag("-ffp-contract=off")
        .flag("-fno-exceptions")
        .flag("-fno-rtti")
        .define("JPH_CROSS_PLATFORM_DETERMINISTIC", None)
        .define("NDEBUG", None)
        .include(&jolt)
        .include(&wrapper)
        .warnings(false)
        .files(&jolt_sources)
        .file(wrapper.join("physics.cpp"));
    build.compile("opence_jolt");
    println!("cargo::rerun-if-changed={}", jolt.join("Jolt").display());
    println!("cargo::rerun-if-changed={}", wrapper.display());
    println!("cargo::rerun-if-changed=build.rs");
}
