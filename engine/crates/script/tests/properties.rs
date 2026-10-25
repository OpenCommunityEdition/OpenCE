//! Properties of the console's compiler, which takes whatever is typed:
//! any text is compiled and run or refused with an error, never a panic;
//! the same line gives the same answer each time; and the limits on its
//! length and its depth hold at their edges.

use opence_content::{Scripts, ValueType};
use opence_script::{ConsoleError, Registry, Runtime, Value, World, MAX_DEPTH, MAX_TEXT};
use proptest::prelude::*;
use proptest::test_runner::{Config, RngSeed};

#[derive(Default)]
struct Fake {
    flag: bool,
}

impl World for Fake {}

fn set_flag(world: &mut Fake, arguments: &[Value]) -> Value {
    world.flag = arguments[0].as_boolean();
    Value::Void
}

fn flag(world: &mut Fake, _: &[Value]) -> Value {
    Value::Boolean(world.flag)
}

fn runtime(world: &mut Fake) -> Runtime<Fake> {
    let mut registry = Registry::new();
    registry.function("flag", ValueType::Boolean, &[], flag);
    registry.function("set_flag", ValueType::Void, &[ValueType::Boolean], set_flag);
    registry.global("game_speed", ValueType::Real, Value::Real(1.0));
    Runtime::new(Scripts::default(), registry, world)
}

fn evaluate(text: &str) -> Result<(Value, ValueType), ConsoleError> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    runtime.evaluate_text(text, &mut world)
}

/// A fixed seed, so that each run tries the same cases.
fn config(cases: u32) -> Config {
    Config {
        cases,
        rng_seed: RngSeed::Fixed(0x0CE0_0CE0),
        failure_persistence: None,
        ..Config::default()
    }
}

/// Lines made of the console's own words: its operators, functions,
/// globals, literals and parentheses, nested a few deep.
fn line() -> impl Strategy<Value = String> {
    let word = prop_oneof![
        Just("+".to_owned()),
        Just("-".to_owned()),
        Just("*".to_owned()),
        Just("/".to_owned()),
        Just("<".to_owned()),
        Just("=".to_owned()),
        Just("and".to_owned()),
        Just("or".to_owned()),
        Just("not".to_owned()),
        Just("if".to_owned()),
        Just("begin".to_owned()),
        Just("set".to_owned()),
        Just("max".to_owned()),
        Just("flag".to_owned()),
        Just("set_flag".to_owned()),
        Just("game_speed".to_owned()),
        Just("true".to_owned()),
        Just("false".to_owned()),
        Just("none".to_owned()),
        any::<i16>().prop_map(|number| number.to_string()),
        (-1.0e6f32..1.0e6).prop_map(|number| number.to_string()),
        "\"[ -~&&[^\"]]{0,8}\"",
        "[a-z_]{1,8}",
    ];
    word.prop_recursive(4, 32, 6, |inner| {
        prop::collection::vec(inner, 0..6).prop_map(|words| format!("({})", words.join(" ")))
    })
}

proptest! {
    #![proptest_config(config(1024))]

    #[test]
    fn any_text_is_run_or_refused(text in ".{0,200}") {
        let _ = evaluate(&text);
    }

    #[test]
    fn any_line_of_its_words_is_run_or_refused_the_same_each_time(line in line()) {
        let first = evaluate(&line);
        // (a Real compared by its bits: NaN is the same answer as NaN)
        let same = |a: &Result<(Value, ValueType), ConsoleError>, b: &Result<(Value, ValueType), ConsoleError>| {
            match (a, b) {
                (Ok((Value::Real(x), xt)), Ok((Value::Real(y), yt))) => x.to_bits() == y.to_bits() && xt == yt,
                _ => a == b,
            }
        };
        prop_assert!(same(&first, &evaluate(&line)), "{line}: {first:?}");
    }

    #[test]
    fn a_line_nested_past_the_deepest_is_refused(depth in 1usize..(MAX_DEPTH * 3)) {
        // (`depth` one-argument `and`s around `(flag)`: one more parenthesis)
        let line = format!("{}(flag){}", "(and ".repeat(depth), ")".repeat(depth));
        let result = evaluate(&line);
        if depth < MAX_DEPTH {
            prop_assert_eq!(result, Ok((Value::Boolean(false), ValueType::Boolean)));
        } else {
            prop_assert!(matches!(result, Err(ConsoleError::Syntax(_))), "{depth}: {result:?}");
        }
    }

    #[test]
    fn a_line_longer_than_the_longest_is_refused(extra in 1usize..64) {
        let text = format!("(+ 1 2){}", " ".repeat(MAX_TEXT - 7 + extra));
        prop_assert!(matches!(evaluate(&text), Err(ConsoleError::Syntax(_))));
    }
}
