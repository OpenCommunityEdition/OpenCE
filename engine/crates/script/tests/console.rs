//! Text typed at the console, evaluated against the running scripts: the
//! expression is parsed, typed by the functions it calls, and run to its
//! end; what it names that is not there is an error, not a default.

use std::error::Error;

use opence_content::{
    Constant, Group, NodeKind, Script, ScriptGlobal, ScriptKind, ScriptNode, Scripts, TagRef,
    ValueType,
};
use opence_script::{ConsoleError, Registry, Runtime, Sleep, Value, World};

#[derive(Default)]
struct Fake {
    flag: bool,
    killed: Vec<i32>,
    listed: Vec<i32>,
    tags: Vec<String>,
    printed: Vec<String>,
}

impl World for Fake {
    fn object_of_name(&mut self, name: i32) -> i32 {
        if name < 0 {
            -1
        } else {
            name + 100
        }
    }

    fn object_list_of(&mut self, objects: &[i32]) -> i32 {
        objects.first().map_or(-1, |object| object + 1000)
    }

    fn object_list_of_ai(&mut self, ai: i32) -> i32 {
        ai + 2000
    }

    fn print(&mut self, text: &str) {
        self.printed.push(text.to_owned());
    }

    fn name_index(&mut self, value_type: ValueType, name: &str) -> i32 {
        match (value_type, name) {
            (ValueType::UnitName, "marine") => 7,
            (ValueType::TriggerVolume, "landing") => 2,
            (ValueType::TriggerVolume, "start") => 0,
            (ValueType::ObjectName, "warthog") => 3,
            (ValueType::Ai, "x30") => 4,
            _ => -1,
        }
    }

    fn tag_of(&mut self, value_type: ValueType, path: &str) -> Option<TagRef> {
        (value_type == ValueType::ObjectDefinition && path.starts_with("weapons\\")).then(|| {
            TagRef {
                group: Group(*b"weap"),
                path: path.to_owned(),
            }
        })
    }
}

fn flag(world: &mut Fake, _: &[Value]) -> Value {
    Value::Boolean(world.flag)
}

fn set_flag(world: &mut Fake, arguments: &[Value]) -> Value {
    world.flag = arguments[0].as_boolean();
    Value::Void
}

fn kill(world: &mut Fake, arguments: &[Value]) -> Value {
    world.killed.push(arguments[0].as_index());
    Value::Void
}

fn objects(world: &mut Fake, arguments: &[Value]) -> Value {
    world.listed.push(arguments[0].as_index());
    Value::Void
}

fn place(world: &mut Fake, arguments: &[Value]) -> Value {
    if let Value::Tag(Some(tag)) = &arguments[0] {
        world.tags.push(tag.path.clone());
    }
    Value::Void
}

fn volume(_: &mut Fake, arguments: &[Value]) -> Value {
    Value::Short(i16::try_from(arguments[0].as_index()).unwrap_or(-1))
}

fn runtime(world: &mut Fake) -> Runtime<Fake> {
    let mut registry = Registry::new();
    registry.function("flag", ValueType::Boolean, &[], flag);
    registry.function("set_flag", ValueType::Void, &[ValueType::Boolean], set_flag);
    registry.function("unit_kill", ValueType::Void, &[ValueType::Unit], kill);
    registry.function(
        "objects",
        ValueType::Void,
        &[ValueType::ObjectList],
        objects,
    );
    registry.function(
        "place",
        ValueType::Void,
        &[ValueType::ObjectDefinition],
        place,
    );
    registry.function(
        "volume",
        ValueType::Short,
        &[ValueType::TriggerVolume],
        volume,
    );
    registry.global("game_speed", ValueType::Real, Value::Real(1.0));

    let mut scripts = Scripts::default();
    // (a static script `five` returning 5, and a dormant `later`)
    scripts.nodes.push(ScriptNode {
        value_type: ValueType::Short,
        kind: NodeKind::Constant(ValueType::Short, Constant::Short(5)),
    });
    scripts.scripts.push(Script {
        name: "five".into(),
        kind: ScriptKind::Static,
        return_type: ValueType::Short,
        root: 0,
    });
    scripts.scripts.push(Script {
        name: "later".into(),
        kind: ScriptKind::Dormant,
        return_type: ValueType::Void,
        root: 0,
    });
    scripts.globals.push(ScriptGlobal {
        name: "count".into(),
        value_type: ValueType::Short,
        initial: None,
    });
    Runtime::new(scripts, registry, world)
}

#[test]
fn arithmetic_and_comparisons() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    assert_eq!(
        runtime.evaluate_text("(+ 1 2.5)", &mut world)?,
        (Value::Real(3.5), ValueType::Real)
    );
    assert_eq!(
        runtime.evaluate_text("(max 1 (* 2 3) 4)", &mut world)?,
        (Value::Real(6.0), ValueType::Real)
    );
    assert_eq!(
        runtime.evaluate_text("(and true (< 1 2))", &mut world)?,
        (Value::Boolean(true), ValueType::Boolean)
    );
    assert_eq!(
        runtime.evaluate_text("(if (= 1 2) \"yes\" \"no\")", &mut world)?,
        (Value::String("no".into()), ValueType::String)
    );
    Ok(())
}

#[test]
fn natives_take_their_parameters_types() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    // (a bare line is a call: the console's way)
    runtime.evaluate_text("set_flag on", &mut world)?;
    assert!(world.flag);
    assert_eq!(
        runtime.evaluate_text("flag", &mut world)?,
        (Value::Boolean(true), ValueType::Boolean)
    );
    // (an object's name, taken as its object through the world)
    runtime.evaluate_text("(unit_kill marine)", &mut world)?;
    assert_eq!(world.killed, [107]);
    runtime.evaluate_text("place \"weapons\\pistol\\pistol\"", &mut world)?;
    runtime.evaluate_text("(place weapons\\rifle)", &mut world)?;
    assert_eq!(world.tags, ["weapons\\pistol\\pistol", "weapons\\rifle"]);
    assert_eq!(
        runtime.evaluate_text("(volume landing)", &mut world)?,
        (Value::Short(2), ValueType::Short)
    );
    runtime.evaluate_text("(begin (set_flag false) (unit_kill marine))", &mut world)?;
    assert!(!world.flag);
    assert_eq!(world.killed, [107, 107]);
    Ok(())
}

#[test]
fn globals_and_scripts() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    runtime.evaluate_text("(set count 4)", &mut world)?;
    assert_eq!(runtime.global("count"), Some(&Value::Short(4)));
    assert_eq!(
        runtime.evaluate_text("(+ count 1)", &mut world)?,
        (Value::Real(5.0), ValueType::Real)
    );
    assert_eq!(
        runtime.evaluate_text("count", &mut world)?,
        (Value::Short(4), ValueType::Short)
    );
    runtime.evaluate_text("set game_speed 0.5", &mut world)?;
    assert_eq!(runtime.engine_global("game_speed"), Some(&Value::Real(0.5)));
    assert_eq!(
        runtime.evaluate_text("(five)", &mut world)?,
        (Value::Short(5), ValueType::Short)
    );
    assert_eq!(runtime.sleep_of("later"), Some(Sleep::Dormant));
    runtime.evaluate_text("(wake later)", &mut world)?;
    assert_ne!(runtime.sleep_of("later"), Some(Sleep::Dormant));
    runtime.evaluate_text("(inspect count)", &mut world)?;
    assert_eq!(world.printed.len(), 1);
    Ok(())
}

#[test]
fn what_is_not_there_is_an_error() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    let error = |runtime: &mut Runtime<Fake>, world: &mut Fake, text: &str| {
        runtime
            .evaluate_text(text, world)
            .err()
            .ok_or_else(|| format!("`{text}` gave no error"))
    };
    assert_eq!(
        error(&mut runtime, &mut world, "(ai_place x30)")?,
        ConsoleError::UnknownFunction("ai_place".into())
    );
    assert_eq!(
        error(&mut runtime, &mut world, "(unit_kill)")?,
        ConsoleError::Arguments {
            function: "unit_kill".into(),
            expected: 1,
            given: 0,
        }
    );
    assert_eq!(
        error(&mut runtime, &mut world, "(unit_kill nobody)")?,
        ConsoleError::UnknownName {
            value_type: ValueType::UnitName,
            name: "nobody".into(),
        }
    );
    assert_eq!(
        error(&mut runtime, &mut world, "(place vehicles\\warthog)")?,
        ConsoleError::UnknownName {
            value_type: ValueType::ObjectDefinition,
            name: "vehicles\\warthog".into(),
        }
    );
    assert_eq!(
        error(&mut runtime, &mut world, "(+ 1 two)")?,
        ConsoleError::NotAValue {
            value_type: ValueType::Real,
            given: "two".into(),
        }
    );
    assert_eq!(
        error(&mut runtime, &mut world, "(set nothing 1)")?,
        ConsoleError::UnknownGlobal("nothing".into())
    );
    for text in ["(+ 1 2", "(+ 1 2))", "\"open", "()", ""] {
        assert!(
            matches!(
                error(&mut runtime, &mut world, text)?,
                ConsoleError::Syntax(_)
            ),
            "{text}"
        );
    }
    let deep = format!("{}1{}", "(+ ".repeat(200), ")".repeat(200));
    assert!(matches!(
        error(&mut runtime, &mut world, &deep)?,
        ConsoleError::Syntax(_)
    ));
    Ok(())
}

#[test]
fn the_scripts_are_left_as_they_were() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    for _ in 0..3 {
        runtime.evaluate_text("(begin (unit_kill marine) (flag))", &mut world)?;
        let _ = runtime.evaluate_text("(ai_place x30)", &mut world);
    }
    assert!(runtime.missing().is_empty());
    // (a console expression may not sleep: it gives its default at once)
    assert_eq!(
        runtime.evaluate_text("(sleep 30)", &mut world)?,
        (Value::Void, ValueType::Void)
    );
    runtime.tick(&mut world);
    assert!(runtime.faults().is_empty());
    Ok(())
}

#[test]
fn a_line_is_taken_to_its_limits_and_no_further() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    let longest = format!("(+ 1 2){}", " ".repeat(opence_script::MAX_TEXT - 7));
    assert_eq!(
        runtime.evaluate_text(&longest, &mut world)?,
        (Value::Real(3.0), ValueType::Real)
    );
    let syntax = |text: &str| ConsoleError::Syntax(text.to_owned());
    for (text, expected) in [
        ("", syntax("nothing to evaluate")),
        ("   ", syntax("nothing to evaluate")),
        ("(+ 1 2) (+ 3 4)", syntax("more than one expression")),
        ("(+ 1 2) 3", syntax("more than one expression")),
    ] {
        assert_eq!(
            runtime.evaluate_text(text, &mut world),
            Err(expected),
            "{text:?}"
        );
    }
    // (a lone word naming a script is a call of it, as a function's is)
    assert_eq!(
        runtime.evaluate_text("five", &mut world)?,
        (Value::Short(5), ValueType::Short)
    );
    Ok(())
}

#[test]
fn a_wrong_count_names_the_bound_it_passed() {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    for (text, expected, given) in [("(sleep)", 1, 0), ("(sleep 1 later 3)", 2, 3)] {
        assert_eq!(
            runtime.evaluate_text(text, &mut world),
            Err(ConsoleError::Arguments {
                function: "sleep".into(),
                expected,
                given,
            }),
            "{text}"
        );
    }
}

#[test]
fn a_begin_gives_its_last_expressions_value() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    assert_eq!(
        runtime.evaluate_text("(begin (set_flag true) (five))", &mut world)?,
        (Value::Short(5), ValueType::Short)
    );
    assert!(world.flag);
    Ok(())
}

#[test]
fn the_special_forms_of_waiting_and_units_compile() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    // (a console expression may not sleep: it ends at once)
    assert_eq!(
        runtime.evaluate_text("(sleep_until true)", &mut world)?,
        (Value::Void, ValueType::Void)
    );
    assert_eq!(
        runtime.evaluate_text("(unit none)", &mut world)?.1,
        ValueType::Unit
    );
    assert_eq!(
        runtime
            .evaluate_text("(object_to_unit none)", &mut world)?
            .1,
        ValueType::Unit
    );
    Ok(())
}

#[test]
fn names_are_looked_up_as_the_parameter_takes_them() -> Result<(), Box<dyn Error>> {
    let mut world = Fake::default();
    let mut runtime = runtime(&mut world);
    // (`none` is no object; an object list takes an object's name or an
    // encounter)
    runtime.evaluate_text("(unit_kill none)", &mut world)?;
    assert_eq!(world.killed, [-1]);
    runtime.evaluate_text("(objects warthog)", &mut world)?;
    runtime.evaluate_text("(objects x30)", &mut world)?;
    assert_eq!(world.listed, [1103, 2004]);
    assert_eq!(
        runtime.evaluate_text("(unit_kill x30)", &mut world),
        Err(ConsoleError::UnknownName {
            value_type: ValueType::UnitName,
            name: "x30".into(),
        })
    );
    // (any other name: its index, `none` for -1, or an error)
    assert_eq!(
        runtime.evaluate_text("(volume start)", &mut world)?,
        (Value::Short(0), ValueType::Short)
    );
    assert_eq!(
        runtime.evaluate_text("(volume none)", &mut world)?,
        (Value::Short(-1), ValueType::Short)
    );
    assert_eq!(
        runtime.evaluate_text("(volume nowhere)", &mut world),
        Err(ConsoleError::UnknownName {
            value_type: ValueType::TriggerVolume,
            name: "nowhere".into(),
        })
    );
    Ok(())
}
