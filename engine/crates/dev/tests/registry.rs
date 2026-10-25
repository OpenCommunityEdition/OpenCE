//! The registry: commands with typed arguments, variables, the
//! fall-through to HaloScript, and completion.

use insta::assert_snapshot;
use opence_dev::{Arg, ArgKind, CommandError, Output, Param, Registry, VarKind, VarValue};
use pretty_assertions::assert_eq;
use std::error::Error;

#[derive(Default)]
struct World {
    spawned: Vec<(String, i64)>,
    speed: f64,
    scripts: Vec<String>,
    changes: Vec<(String, VarValue)>,
}

fn spawn(world: &mut World, args: &opence_dev::Args) -> Result<Output, CommandError> {
    let tag = args.string("tag")?.to_owned();
    let count = args.int_or("count", 1)?;
    world.spawned.push((tag.clone(), count));
    Ok(Output::text(format!("spawned {count} {tag}")))
}

fn time(world: &mut World, args: &opence_dev::Args) -> Result<Output, CommandError> {
    world.speed = args.float("scale")?;
    Ok(Output::none())
}

fn script(world: &mut World, line: &str) -> Result<Output, CommandError> {
    world.scripts.push(line.to_owned());
    Ok(Output::text("ran"))
}

fn heard(world: &mut World, name: &str, value: &VarValue) {
    world.changes.push((name.to_owned(), value.clone()));
}

/// The error a line gives, or an error saying it gave none.
fn failure(result: Result<Output, CommandError>) -> Result<CommandError, Box<dyn Error>> {
    match result {
        Ok(output) => Err(format!("expected an error, got `{}`", output.text).into()),
        Err(error) => Ok(error),
    }
}

fn registry() -> Registry<World> {
    let mut registry = Registry::new();
    registry.command(
        "spawn",
        "Spawns objects of a tag in front of the player.",
        &[
            Param::required("tag", ArgKind::String),
            Param::optional("count", ArgKind::Int),
        ],
        spawn,
    );
    registry.command(
        "time",
        "Sets the game's speed.",
        &[Param::required("scale", ArgKind::Float)],
        time,
    );
    registry.variable(
        "render.wireframe",
        "Draws the level's triangles' edges.",
        VarKind::Bool,
        VarValue::Bool(false),
        false,
    );
    registry.variable(
        "render.view",
        "Shows one of the renderer's buffers alone.",
        VarKind::Enum(&["final", "albedo", "normal"]),
        VarValue::String("final".into()),
        true,
    );
    registry.variable(
        "sim.game_speed",
        "The game's speed.",
        VarKind::Float,
        VarValue::Float(1.0),
        false,
    );
    registry.listen(heard);
    registry.fallback(script);
    registry
}

#[test]
fn commands_run_with_typed_arguments() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    let output = registry.run(&mut world, "spawn warthog 3")?;
    assert_eq!(output.text, "spawned 3 warthog");
    registry.run(&mut world, "spawn \"cyborg mp\"")?;
    registry.run(&mut world, "time 0.25")?;
    assert_eq!(
        world.spawned,
        vec![("warthog".into(), 3), ("cyborg mp".into(), 1)]
    );
    assert!((world.speed - 0.25).abs() < 1e-9);
    Ok(())
}

#[test]
fn a_bad_argument_is_named() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    let error = failure(registry.run(&mut world, "spawn warthog many"))?;
    assert_eq!(
        error,
        CommandError::BadArgument {
            command: "spawn".into(),
            param: "count".into(),
            expected: "an integer".into(),
            given: "many".into(),
        }
    );
    let error = failure(registry.run(&mut world, "time"))?;
    assert!(matches!(error, CommandError::MissingArgument { ref param, .. } if param == "scale"));
    let error = failure(registry.run(&mut world, "time 1 2"))?;
    assert!(matches!(error, CommandError::TooManyArguments { .. }));
    assert!(world.spawned.is_empty());
    Ok(())
}

#[test]
fn variables_keep_their_type_and_default() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    registry.run(&mut world, "set render.wireframe true")?;
    assert_eq!(
        registry.get("render.wireframe"),
        Some(&VarValue::Bool(true))
    );
    // (a variable's name and a value set it, as `set` does)
    registry.run(&mut world, "sim.game_speed 0.5")?;
    assert_eq!(registry.get("sim.game_speed"), Some(&VarValue::Float(0.5)));
    let error = failure(registry.run(&mut world, "set render.wireframe maybe"))?;
    assert!(matches!(error, CommandError::BadValue { .. }));
    let error = failure(registry.run(&mut world, "set render.view depth"))?;
    assert!(matches!(error, CommandError::BadValue { .. }));
    // (a variable's name alone shows it)
    let output = registry.run(&mut world, "render.view")?;
    assert_eq!(output.text, "render.view = final (default final)");
    registry.run(&mut world, "reset render.wireframe")?;
    assert_eq!(
        registry.get("render.wireframe"),
        Some(&VarValue::Bool(false))
    );
    Ok(())
}

#[test]
fn saved_variables_are_kept() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    registry.run(&mut world, "set render.view normal")?;
    registry.run(&mut world, "set render.wireframe true")?;
    let saved = registry.saved();
    // (only the saved variables, and only those that are not their default)
    assert_eq!(saved.len(), 1);
    assert_eq!(saved.get("render.view").map(String::as_str), Some("normal"));

    let mut again = self::registry();
    let refused = again.load_saved(&saved);
    assert!(refused.is_empty());
    assert_eq!(
        again.get("render.view"),
        Some(&VarValue::String("normal".into()))
    );
    Ok(())
}

#[test]
fn listeners_hear_changes() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    registry.run(&mut world, "set render.wireframe true")?;
    registry.run(&mut world, "set render.wireframe true")?;
    registry.run(&mut world, "set sim.game_speed 2")?;
    // (a value set to what it was is no change)
    assert_eq!(
        world.changes,
        vec![
            ("render.wireframe".into(), VarValue::Bool(true)),
            ("sim.game_speed".into(), VarValue::Float(2.0)),
        ]
    );
    Ok(())
}

#[test]
fn anything_else_falls_through_to_haloscript() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    registry.run(&mut world, "(ai_place x30_marines)")?;
    registry.run(&mut world, "object_create warthog")?;
    assert_eq!(
        world.scripts,
        vec!["(ai_place x30_marines)", "object_create warthog"]
    );
    assert!(world.spawned.is_empty());
    Ok(())
}

#[test]
fn several_commands_on_a_line() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    let output = registry.run(&mut world, "spawn a; spawn \"b; c\" 2 ; (print \"x;y\")")?;
    assert_eq!(world.spawned, vec![("a".into(), 1), ("b; c".into(), 2)]);
    assert_eq!(world.scripts, vec!["(print \"x;y\")"]);
    assert_snapshot!(output.text, @r"
    spawned 1 a
    spawned 2 b; c
    ran
    ");
    Ok(())
}

#[test]
fn help_and_listing() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    // (the listing, every command's help and a variable's: their text is
    // the shell's, kept in tests/snapshots)
    assert_snapshot!("help", registry.run(&mut world, "help")?.text);
    assert_snapshot!("help_spawn", registry.run(&mut world, "help spawn")?.text);
    assert_snapshot!(
        "help_render_view",
        registry.run(&mut world, "help render.view")?.text
    );
    let output = registry.run(&mut world, "vars render.")?;
    assert_snapshot!("vars_render", output.text);
    assert_eq!(output.data["render.wireframe"], serde_json::json!(false));
    let error = failure(registry.run(&mut world, "help nothing"))?;
    assert!(matches!(error, CommandError::Unknown(ref name) if name == "nothing"));
    Ok(())
}

#[test]
fn bindings() -> Result<(), Box<dyn Error>> {
    let mut registry = registry();
    let mut world = World::default();
    registry.run(&mut world, "bind f5 \"spawn warthog; time 0.5\"")?;
    assert_eq!(registry.binding("f5"), Some("spawn warthog; time 0.5"));
    registry.press(&mut world, "f5")?;
    assert_eq!(world.spawned, vec![("warthog".into(), 1)]);
    registry.run(&mut world, "unbind f5")?;
    assert_eq!(registry.binding("f5"), None);
    Ok(())
}

#[test]
fn completion() -> Result<(), Box<dyn Error>> {
    let registry = registry();
    assert_eq!(registry.complete("sp"), vec!["spawn"]);
    assert_eq!(
        registry.complete("set render."),
        vec!["set render.view", "set render.wireframe"]
    );
    assert_eq!(
        registry.complete("set render.view n"),
        vec!["set render.view normal"]
    );
    assert_eq!(
        registry.complete("set render.wireframe "),
        vec!["set render.wireframe false", "set render.wireframe true"]
    );
    Ok(())
}

#[test]
fn arguments_parse_as_their_kind() -> Result<(), Box<dyn Error>> {
    let args = opence_dev::parse_words("a \"b c\" 'd e' f\\ g")?;
    assert_eq!(args, vec!["a", "b c", "d e", "f g"]);
    assert!(opence_dev::parse_words("\"open").is_err());
    assert_eq!(ArgKind::Bool.parse("on"), Some(Arg::Bool(true)));
    assert_eq!(ArgKind::Bool.parse("0"), Some(Arg::Bool(false)));
    assert_eq!(ArgKind::Int.parse("-3"), Some(Arg::Int(-3)));
    assert_eq!(ArgKind::Float.parse("1e2"), Some(Arg::Float(100.0)));
    assert_eq!(ArgKind::Float.parse("nan"), None);
    assert_eq!(
        ArgKind::Enum(&["x", "y"]).parse("y"),
        Some(Arg::String("y".into()))
    );
    assert_eq!(ArgKind::Enum(&["x", "y"]).parse("z"), None);
    Ok(())
}
