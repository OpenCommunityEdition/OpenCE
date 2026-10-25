//! The checks the tests run with, in any profile they are built in (the
//! optimised `test-opt` as well as `test`): debug assertions and overflow
//! checks stay on, so an optimised run still catches what a debug one does.

use std::hint::black_box;
use std::panic::catch_unwind;

#[test]
fn debug_assertions_are_on() {
    assert!(cfg!(debug_assertions));
}

#[test]
fn an_overflow_panics() {
    let full = black_box(u8::MAX);
    assert!(catch_unwind(|| black_box(full) + black_box(1)).is_err());
}
