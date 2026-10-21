//! The runtime on scripts built by hand: expressions and globals, `sleep`
//! and `sleep_until` to the tick, continuous and dormant scripts, static
//! scripts' values, casts through the world, and what the engine does not
//! have reported.

use opence_content::{
    Call, Constant, GlobalRef, Named, NodeKind, Script, ScriptGlobal, ScriptKind, ScriptNode,
    Scripts, ValueType,
};
use opence_script::{Missing, Registry, Runtime, Sleep, Value, World};

#[derive(Default)]
struct Fake {
    flag: bool,
    calls: u32,
    killed: Vec<i32>,
    printed: Vec<String>,
}

impl World for Fake {
    fn object_of_name(&mut self, name: i32) -> i32 {
        name + 100
    }

    fn print(&mut self, text: &str) {
        self.printed.push(text.to_owned());
    }
}

fn flag(world: &mut Fake, _: &[Value]) -> Value {
    world.calls += 1;
    Value::Boolean(world.flag)
}

fn kill(world: &mut Fake, arguments: &[Value]) -> Value {
    world.killed.push(arguments[0].as_index());
    Value::Void
}

fn registry() -> Registry<Fake> {
    let mut registry = Registry::new();
    registry.function("flag", ValueType::Boolean, &[], flag);
    registry.function("unit_kill", ValueType::Void, &[ValueType::Unit], kill);
    registry
}

/// A scenario's scripts, built a node at a time.
#[derive(Default)]
struct Tree(Scripts);

impl Tree {
    fn node(&mut self, value_type: ValueType, kind: NodeKind) -> u32 {
        self.0.nodes.push(ScriptNode { value_type, kind });
        u32::try_from(self.0.nodes.len() - 1).unwrap_or(0)
    }

    fn call(&mut self, value_type: ValueType, function: &str, arguments: &[u32]) -> u32 {
        let call = Call {
            function: function.to_owned(),
            arguments: arguments.to_vec(),
        };
        self.node(value_type, NodeKind::Call(call))
    }

    fn short(&mut self, value: i16) -> u32 {
        self.node(
            ValueType::Short,
            NodeKind::Constant(ValueType::Short, Constant::Short(value)),
        )
    }

    /// A short written where a real is taken.
    fn real(&mut self, value: i16) -> u32 {
        self.node(
            ValueType::Real,
            NodeKind::Constant(ValueType::Short, Constant::Short(value)),
        )
    }

    fn boolean(&mut self, value: bool) -> u32 {
        self.node(
            ValueType::Boolean,
            NodeKind::Constant(ValueType::Boolean, Constant::Boolean(value)),
        )
    }

    fn global(&mut self, name: &str, value_type: ValueType, initial: Option<u32>) -> u16 {
        self.0.globals.push(ScriptGlobal {
            name: name.to_owned(),
            value_type,
            initial,
        });
        u16::try_from(self.0.globals.len() - 1).unwrap_or(0)
    }

    fn read(&mut self, index: u16, value_type: ValueType) -> u32 {
        let name = self.0.globals[usize::from(index)].name.clone();
        let global = GlobalRef {
            name,
            external: false,
            index,
        };
        self.node(value_type, NodeKind::Global(global))
    }

    fn set(&mut self, index: u16, value: u32) -> u32 {
        let value_type = self.0.globals[usize::from(index)].value_type;
        let target = self.read(index, value_type);
        self.call(value_type, "set", &[target, value])
    }

    fn named(&mut self, value_type: ValueType, written: ValueType, index: i32) -> u32 {
        let named = Named {
            name: format!("named_{index}"),
            index,
        };
        self.node(
            value_type,
            NodeKind::Constant(written, Constant::Named(named)),
        )
    }

    fn script(&mut self, name: &str, kind: ScriptKind, return_type: ValueType, root: u32) -> u32 {
        self.0.scripts.push(Script {
            name: name.to_owned(),
            kind,
            return_type,
            root,
        });
        u32::try_from(self.0.scripts.len() - 1).unwrap_or(0)
    }

    fn run(self, world: &mut Fake) -> Runtime<Fake> {
        Runtime::new(self.0, registry(), world)
    }
}

fn ticks(runtime: &mut Runtime<Fake>, world: &mut Fake, count: u32) {
    for _ in 0..count {
        runtime.tick(world);
    }
}

#[test]
fn expressions_and_globals_take_their_values() {
    let mut tree = Tree::default();
    // (global short x (+ 1 2)): a real, cast to a short
    let (one, two) = (tree.real(1), tree.real(2));
    let sum = tree.call(ValueType::Short, "+", &[one, two]);
    let x = tree.global("x", ValueType::Short, Some(sum));
    // (global short y (if (> x 2) (* x 10) 0)): it sees x's value
    let (read, two) = (tree.read(x, ValueType::Real), tree.real(2));
    let above = tree.call(ValueType::Boolean, ">", &[read, two]);
    let (read, ten) = (tree.read(x, ValueType::Real), tree.real(10));
    let times = tree.call(ValueType::Short, "*", &[read, ten]);
    let zero = tree.short(0);
    let choice = tree.call(ValueType::Short, "if", &[above, times, zero]);
    let y = tree.global("y", ValueType::Short, Some(choice));
    // (global boolean z (or true (flag))): flag is never called
    let (yes, asked) = (
        tree.boolean(true),
        tree.call(ValueType::Boolean, "flag", &[]),
    );
    let either = tree.call(ValueType::Boolean, "or", &[yes, asked]);
    tree.global("z", ValueType::Boolean, Some(either));
    // (global boolean w (and (flag) false)): flag is called, once
    let (asked, no) = (
        tree.call(ValueType::Boolean, "flag", &[]),
        tree.boolean(false),
    );
    let both = tree.call(ValueType::Boolean, "and", &[asked, no]);
    tree.global("w", ValueType::Boolean, Some(both));
    let _ = (x, y);

    let mut world = Fake::default();
    let runtime = tree.run(&mut world);
    assert_eq!(runtime.global("x"), Some(&Value::Short(3)));
    assert_eq!(runtime.global("y"), Some(&Value::Short(30)));
    assert_eq!(runtime.global("z"), Some(&Value::Boolean(true)));
    assert_eq!(runtime.global("w"), Some(&Value::Boolean(false)));
    assert_eq!(world.calls, 1);
    assert!(runtime.missing().is_empty());
}

#[test]
fn sleep_waits_its_ticks() {
    let mut tree = Tree::default();
    let n = tree.global("n", ValueType::Short, None);
    let one = tree.short(1);
    let first = tree.set(n, one);
    let thirty = tree.short(30);
    let sleep = tree.call(ValueType::Void, "sleep", &[thirty]);
    let two = tree.short(2);
    let second = tree.set(n, two);
    let begin = tree.call(ValueType::Void, "begin", &[first, sleep, second]);
    tree.script("main", ScriptKind::Startup, ValueType::Void, begin);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    ticks(&mut runtime, &mut world, 1);
    assert_eq!(runtime.global("n"), Some(&Value::Short(1)));
    assert_eq!(runtime.sleep_of("main"), Some(Sleep::Until(30)));
    // (it runs again on the tick at time 30: the 31st)
    ticks(&mut runtime, &mut world, 29);
    assert_eq!(runtime.global("n"), Some(&Value::Short(1)));
    ticks(&mut runtime, &mut world, 1);
    assert_eq!(runtime.global("n"), Some(&Value::Short(2)));
    // (a startup script runs once)
    assert_eq!(runtime.sleep_of("main"), Some(Sleep::Done));
}

#[test]
fn sleep_until_tests_its_condition_every_period() {
    let mut tree = Tree::default();
    let n = tree.global("n", ValueType::Short, None);
    let asked = tree.call(ValueType::Boolean, "flag", &[]);
    let wait = tree.call(ValueType::Void, "sleep_until", &[asked]);
    let one = tree.short(1);
    let done = tree.set(n, one);
    let begin = tree.call(ValueType::Void, "begin", &[wait, done]);
    tree.script("main", ScriptKind::Startup, ValueType::Void, begin);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    // (it sleeps a period before it first tests: at 30, then 60)
    ticks(&mut runtime, &mut world, 31);
    assert_eq!(world.calls, 1);
    world.flag = true;
    ticks(&mut runtime, &mut world, 29);
    assert_eq!(runtime.global("n"), Some(&Value::Short(0)));
    ticks(&mut runtime, &mut world, 1);
    assert_eq!(world.calls, 2);
    assert_eq!(runtime.global("n"), Some(&Value::Short(1)));
}

#[test]
fn sleep_until_gives_up_at_its_timeout() {
    let mut tree = Tree::default();
    let n = tree.global("n", ValueType::Short, None);
    let never = tree.boolean(false);
    let (period, timeout) = (tree.short(10), tree.short(25));
    let wait = tree.call(ValueType::Void, "sleep_until", &[never, period, timeout]);
    let one = tree.short(1);
    let done = tree.set(n, one);
    let begin = tree.call(ValueType::Void, "begin", &[wait, done]);
    tree.script("main", ScriptKind::Startup, ValueType::Void, begin);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    // (tests at 10 and 20, and stops at 25)
    ticks(&mut runtime, &mut world, 25);
    assert_eq!(runtime.global("n"), Some(&Value::Short(0)));
    assert_eq!(runtime.sleep_of("main"), Some(Sleep::Until(25)));
    ticks(&mut runtime, &mut world, 1);
    assert_eq!(runtime.global("n"), Some(&Value::Short(1)));
}

#[test]
fn a_continuous_script_runs_again_after_it_ends() {
    let mut tree = Tree::default();
    let count = tree.global("count", ValueType::Short, None);
    let (read, one) = (tree.read(count, ValueType::Real), tree.real(1));
    let plus = tree.call(ValueType::Short, "+", &[read, one]);
    let more = tree.set(count, plus);
    let ten = tree.short(10);
    let sleep = tree.call(ValueType::Void, "sleep", &[ten]);
    let begin = tree.call(ValueType::Void, "begin", &[more, sleep]);
    tree.script("counting", ScriptKind::Continuous, ValueType::Void, begin);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    // (it counts at 0, wakes at 10 to end, and starts again at 11 and 22:
    // as Halo's does, a sleep of 10 at its end is a turn every 11 ticks)
    ticks(&mut runtime, &mut world, 22);
    assert_eq!(runtime.global("count"), Some(&Value::Short(2)));
    ticks(&mut runtime, &mut world, 1);
    assert_eq!(runtime.global("count"), Some(&Value::Short(3)));
    assert!(matches!(
        runtime.sleep_of("counting"),
        Some(Sleep::Until(_))
    ));
}

#[test]
fn a_dormant_script_runs_when_woken() {
    let mut tree = Tree::default();
    let n = tree.global("n", ValueType::Short, None);
    let seven = tree.short(7);
    let body = tree.set(n, seven);
    tree.script("later", ScriptKind::Dormant, ValueType::Void, body);
    let five = tree.short(5);
    let sleep = tree.call(ValueType::Void, "sleep", &[five]);
    let later = tree.named(ValueType::Script, ValueType::Script, 0);
    let wake = tree.call(ValueType::Void, "wake", &[later]);
    let begin = tree.call(ValueType::Void, "begin", &[sleep, wake]);
    tree.script("main", ScriptKind::Startup, ValueType::Void, begin);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    assert_eq!(runtime.sleep_of("later"), Some(Sleep::Dormant));
    ticks(&mut runtime, &mut world, 6);
    // (woken at 5 by a thread after it: it runs on the next tick)
    assert_eq!(runtime.global("n"), Some(&Value::Short(0)));
    assert_eq!(runtime.sleep_of("later"), Some(Sleep::Until(0)));
    ticks(&mut runtime, &mut world, 1);
    assert_eq!(runtime.global("n"), Some(&Value::Short(7)));
    // (and once it has run, waking it again does nothing)
    assert_eq!(runtime.sleep_of("later"), Some(Sleep::Done));
    runtime.wake("later");
    assert_eq!(runtime.sleep_of("later"), Some(Sleep::Done));
}

#[test]
fn a_static_script_returns_its_value() {
    let mut tree = Tree::default();
    let (two, three) = (tree.real(2), tree.real(3));
    let sum = tree.call(ValueType::Short, "+", &[two, three]);
    let five = tree.script("five", ScriptKind::Static, ValueType::Short, sum);
    let called = tree.node(ValueType::Real, NodeKind::ScriptCall(five));
    tree.global("y", ValueType::Real, Some(called));

    let mut world = Fake::default();
    let runtime = tree.run(&mut world);
    assert_eq!(runtime.global("y"), Some(&Value::Real(5.0)));
    assert_eq!(runtime.sleep_of("five"), None);
}

#[test]
fn an_objects_name_is_cast_to_its_object() {
    let mut tree = Tree::default();
    let name = tree.named(ValueType::Unit, ValueType::UnitName, 4);
    let kill = tree.call(ValueType::Void, "unit_kill", &[name]);
    let text = tree.node(
        ValueType::String,
        NodeKind::Constant(ValueType::String, Constant::String("hello".into())),
    );
    let inspect = tree.call(ValueType::Void, "inspect", &[text]);
    let begin = tree.call(ValueType::Void, "begin", &[kill, inspect]);
    tree.script("main", ScriptKind::Startup, ValueType::Void, begin);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    ticks(&mut runtime, &mut world, 1);
    assert_eq!(world.killed, vec![104]);
    assert_eq!(world.printed, vec!["hello".to_owned()]);
}

#[test]
fn what_the_engine_does_not_have_is_reported() {
    let mut tree = Tree::default();
    let unknown = tree.call(ValueType::Boolean, "ai_allegiance_broken", &[]);
    let wrong = tree.call(ValueType::Void, "unit_kill", &[]);
    let external = tree.node(
        ValueType::Boolean,
        NodeKind::Global(GlobalRef {
            name: "cheat_deathless_player".into(),
            external: true,
            index: 0,
        }),
    );
    let either = tree.call(ValueType::Boolean, "or", &[unknown, external]);
    tree.global("b", ValueType::Boolean, Some(either));
    tree.script("main", ScriptKind::Startup, ValueType::Void, wrong);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    ticks(&mut runtime, &mut world, 1);
    let missing: Vec<_> = runtime.missing().iter().cloned().collect();
    assert_eq!(
        missing,
        vec![
            Missing::Function("ai_allegiance_broken".into()),
            Missing::Global("cheat_deathless_player".into()),
            Missing::Arguments("unit_kill".into()),
        ]
    );
    // (each gives its type's default, and the scripts run on)
    assert_eq!(runtime.global("b"), Some(&Value::Boolean(false)));
    assert_eq!(runtime.sleep_of("main"), Some(Sleep::Done));
    assert!(world.killed.is_empty());
}

#[test]
fn a_script_that_calls_itself_without_end_is_ended() {
    let mut tree = Tree::default();
    // (static script forever: (forever)), called by a startup script
    let call = tree.node(ValueType::Void, NodeKind::ScriptCall(0));
    tree.script("forever", ScriptKind::Static, ValueType::Void, call);
    let start = tree.node(ValueType::Void, NodeKind::ScriptCall(0));
    tree.script("main", ScriptKind::Startup, ValueType::Void, start);

    let mut world = Fake::default();
    let mut runtime = tree.run(&mut world);
    ticks(&mut runtime, &mut world, 2);
    assert_eq!(runtime.sleep_of("main"), Some(Sleep::Done));
}
