//! The registry's less common paths: `exec` files, help on variables, the
//! other kinds of arguments and variables, and the errors of the
//! registry's own commands.

use std::error::Error;

use opence_dev::{
    split_commands, ArgKind, Args, CommandError, Output, Param, Registry, VarKind, VarValue,
};

#[derive(Default)]
struct World {
    said: Vec<String>,
}

fn say(world: &mut World, args: &Args) -> Result<Output, CommandError> {
    let text = args.string_or("text", "nothing")?;
    let loud = args.bool_or("loud", false)?;
    let times = args.float_or("times", 1.0)?;
    world.said.push(format!("{text} {loud} {times}"));
    Ok(Output::text(text))
}

fn echo(world: &mut World, args: &Args) -> Result<Output, CommandError> {
    let text = args.string("words")?.to_owned();
    world.said.push(text.clone());
    Ok(Output::text(text))
}

fn registry() -> Registry<World> {
    let mut registry = Registry::new();
    registry.command(
        "say",
        "Says something.",
        &[
            Param::optional("text", ArgKind::String),
            Param::optional("loud", ArgKind::Bool),
            Param::optional("times", ArgKind::Float),
        ],
        say,
    );
    registry.command(
        "echo",
        "Says the rest of the line.",
        &[Param::required("words", ArgKind::Rest)],
        echo,
    );
    registry.variable(
        "ai.count",
        "How many.",
        VarKind::Int,
        VarValue::Int(4),
        false,
    );
    registry.variable(
        "player.name",
        "Who.",
        VarKind::String,
        VarValue::String("chief".into()),
        true,
    );
    registry
}

fn failure(result: Result<Output, CommandError>) -> Result<CommandError, Box<dyn Error>> {
    match result {
        Ok(output) => Err(format!("expected an error, got `{}`", output.text).into()),
        Err(error) => Ok(error),
    }
}

#[test]
fn optional_and_rest_arguments() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    registry.run(&mut world, "say")?;
    registry.run(&mut world, "say hi yes 2")?;
    let output = registry.run(&mut world, "echo the  rest of it")?;
    assert_eq!(output.text, "the rest of it");
    assert_eq!(
        world.said,
        ["nothing false 1", "hi true 2", "the rest of it"]
    );
    let error = failure(registry.run(&mut world, "echo"))?;
    assert!(matches!(error, CommandError::MissingArgument { .. }));
    let help = registry.run(&mut world, "help echo")?;
    assert_eq!(help.text, "echo <words...>\n  Says the rest of the line.");
    let help = registry.run(&mut world, "help say")?;
    assert!(help.text.starts_with("say [text] [loud] [times]"));
    Ok(())
}

#[test]
fn int_and_string_variables() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    registry.run(&mut world, "set ai.count 9")?;
    registry.run(&mut world, "player.name the arbiter")?;
    assert_eq!(registry.get("ai.count"), Some(&VarValue::Int(9)));
    assert_eq!(registry.get_float("ai.count", 0.0), 9.0);
    assert_eq!(registry.get_float("none", 1.5), 1.5);
    assert!(!registry.get_bool("ai.count"));
    assert_eq!(
        registry.get("player.name"),
        Some(&VarValue::String("the arbiter".into()))
    );
    let error = failure(registry.run(&mut world, "ai.count lots"))?;
    assert_eq!(
        error,
        CommandError::BadValue {
            variable: "ai.count".into(),
            expected: "an integer".into(),
            given: "lots".into(),
        }
    );
    let error = registry.set(&mut world, "ai.count", VarValue::Bool(true));
    assert!(matches!(error, Err(CommandError::BadValue { .. })));
    let error = registry.set(&mut world, "none", VarValue::Bool(true));
    assert_eq!(error, Err(CommandError::Unknown("none".into())));

    let help = registry.run(&mut world, "help ai.count")?;
    assert_eq!(help.text, "ai.count <integer> = 9 (default 4)\n  How many.");
    let help = registry.run(&mut world, "help player.name")?;
    assert!(help.text.starts_with("player.name <text>"));

    let listing = registry.run(&mut world, "vars")?;
    assert_eq!(listing.data["ai.count"], 9);
    assert_eq!(listing.data["player.name"], "the arbiter");
    let shown = registry.run(&mut world, "ai.count")?;
    assert_eq!(shown.data, 9);
    Ok(())
}

#[test]
fn the_registrys_own_commands_check_their_arguments() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    for line in [
        "set",
        "set ai.count",
        "reset",
        "bind",
        "bind f1",
        "unbind",
        "exec",
    ] {
        let error = failure(registry.run(&mut world, line))?;
        assert!(
            matches!(error, CommandError::MissingArgument { .. }),
            "{line}: {error:?}"
        );
    }
    let error = failure(registry.run(&mut world, "reset nothing"))?;
    assert_eq!(error, CommandError::Unknown("nothing".into()));
    let error = failure(registry.run(&mut world, "set nothing 1"))?;
    assert_eq!(error, CommandError::Unknown("nothing".into()));
    let error = failure(registry.run(&mut world, "(ai_place x)"))?;
    assert_eq!(error, CommandError::Unknown("(ai_place".into()));
    let error = failure(registry.run(&mut world, "say \"open"))?;
    assert!(matches!(error, CommandError::Syntax(_)));
    assert_eq!(registry.run(&mut world, " ; ;")?, Output::none());
    assert_eq!(registry.press(&mut world, "f9")?, Output::none());
    Ok(())
}

#[test]
fn help_lists_every_command() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    let help = registry.run(&mut world, "help")?;
    let names: Vec<&str> = help
        .text
        .lines()
        .filter_map(|l| l.split_whitespace().next())
        .collect();
    assert_eq!(names, registry.command_names());
    assert!(names.contains(&"exec") && names.contains(&"say"));
    assert!(registry.has_command("set") && registry.has_command("echo"));
    assert!(!registry.has_command("ai.count"));
    assert_eq!(
        registry.variable_names().collect::<Vec<_>>(),
        ["ai.count", "player.name"]
    );
    Ok(())
}

#[test]
fn exec_runs_a_files_lines() -> Result<(), Box<dyn Error>> {
    let folder = std::env::temp_dir().join(format!("opence_dev_exec_{}", std::process::id()));
    std::fs::create_dir_all(&folder)?;
    let inner = folder.join("inner.cfg");
    let outer = folder.join("outer.cfg");
    let bad = folder.join("bad.cfg");
    let looping = folder.join("loop.cfg");
    std::fs::write(&inner, "echo inside\n")?;
    std::fs::write(
        &outer,
        format!(
            "# a comment\n\nset ai.count 2\necho one; echo two\nexec {}\nbind f1 echo bound\n",
            inner.display()
        ),
    )?;
    std::fs::write(&bad, "echo fine\nai.count lots\necho never\n")?;
    std::fs::write(&looping, format!("exec {}\n", looping.display()))?;

    let mut registry = registry();
    let mut world = World::default();
    let output = registry.run(&mut world, &format!("exec {}", outer.display()))?;
    assert_eq!(output.text, "one\ntwo\ninside");
    assert_eq!(registry.get("ai.count"), Some(&VarValue::Int(2)));
    assert_eq!(registry.binding("f1"), Some("echo bound"));
    assert_eq!(registry.bindings().len(), 1);

    let error = failure(registry.run(&mut world, &format!("exec {}", bad.display())))?;
    assert!(
        matches!(&error, CommandError::Failed(text) if text.contains("bad.cfg:2:")),
        "{error}"
    );
    assert!(!world.said.contains(&"never".to_owned()));

    let error = failure(registry.run(&mut world, &format!("exec {}", looping.display())))?;
    assert!(matches!(&error, CommandError::Failed(text) if text.contains("inside one another")));
    // (the depth is back to nothing after the failure)
    registry.run(&mut world, &format!("exec {}", inner.display()))?;

    let error = failure(registry.run(&mut world, "exec /nowhere/at/all.cfg"))?;
    assert!(matches!(error, CommandError::Failed(_)));
    std::fs::remove_dir_all(&folder)?;
    Ok(())
}

#[test]
fn saved_values_that_are_refused() {
    let mut registry = registry();
    let saved = [
        ("player.name".to_owned(), "cortana".to_owned()),
        ("ai.count".to_owned(), "3".to_owned()),
        ("none".to_owned(), "1".to_owned()),
    ]
    .into_iter()
    .collect();
    assert_eq!(registry.load_saved(&saved), ["ai.count", "none"]);
    assert_eq!(
        registry.get("player.name"),
        Some(&VarValue::String("cortana".into()))
    );
}

#[test]
fn commands_split_outside_quotes_and_parentheses() {
    assert_eq!(
        split_commands(r#"a; (b; c) ;"d;e"; 'f;g' ;h\;i"#),
        ["a", "(b; c)", r#""d;e""#, "'f;g'", r"h\;i"]
    );
}

#[test]
fn completion_of_arguments_and_help() {
    let registry = registry();
    assert_eq!(registry.complete("help ec"), ["help echo"]);
    assert_eq!(registry.complete("reset ai"), ["reset ai.count"]);
    assert_eq!(
        registry.complete("say hi "),
        ["say hi false", "say hi true"]
    );
    assert_eq!(registry.complete("echo x"), Vec::<String>::new());
    assert_eq!(registry.complete("say hi; ec"), ["say hi; echo"]);
    assert!(registry.complete("").len() > 9);
}

#[test]
fn outputs_and_kinds_in_words() {
    assert_eq!(Output::with_data("x", 3.into()).data, 3);
    assert_eq!(Output::none().data, serde_json::Value::Null);
    assert_eq!(ArgKind::Enum(&["a", "b"]).expected(), "one of a, b");
    assert_eq!(ArgKind::Bool.expected(), "true or false");
    assert_eq!(ArgKind::Float.expected(), "a number");
    assert_eq!(ArgKind::String.expected(), "a word");
    assert_eq!(VarKind::Float.parse("inf"), None);
    assert_eq!(VarKind::Enum(&["a"]).parse("b"), None);
    assert_eq!(VarValue::Float(0.5).data(), 0.5);
    assert_eq!(VarValue::Bool(true).to_string(), "true");
}
