//! The contract between the engine and the Swift app.
//!
//! Calls (load a map, start a game, input, settings) go through UniFFI;
//! the per-frame data goes through the `#[repr(C)]` types of the
//! snapshot, in the header cbindgen writes.
// (the UniFFI scaffolding is unsafe code; this crate and physics are the
// only ones allowed it)
#![allow(unsafe_code)]

uniffi::setup_scaffolding!();

/// The engine's version, as the app shows it.
#[uniffi::export]
pub fn engine_version() -> String {
    env!("CARGO_PKG_VERSION").to_owned()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_version_is_the_crates() {
        assert_eq!(engine_version(), "0.1.0");
    }
}
