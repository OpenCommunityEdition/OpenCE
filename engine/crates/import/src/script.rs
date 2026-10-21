//! A scenario's scripts: its compiled HaloScript nodes (a data array in a
//! tag data field), the strings they name, its scripts and its globals.
//!
//! A node's children are linked as a list: a call's data is its first
//! child, the node naming its function, and the arguments follow it by
//! their `next` links. Every link is checked, and an expression that
//! reaches a node twice (a loop among them) is refused, as the original's
//! port refuses it (`hs_expression_refusal`).

use std::collections::BTreeMap;

use opence_content::{
    Call, Constant, GlobalRef, Named, NodeKind, Script, ScriptGlobal, ScriptKind, ScriptNode,
    Scripts, ValueType,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_ref, SCENARIO_SIZE, SCNR};
use crate::reader;
use crate::tags::Element;

const SYNTAX_DATA_OFFSET: usize = 0x474;
const STRING_CONSTANTS_OFFSET: usize = 0x488;
const SCRIPTS_OFFSET: usize = 0x49C;
const GLOBALS_OFFSET: usize = 0x4A8;

/// The nodes a scenario has room for.
const MAX_NODES: usize = 19001;
const NODE_SIZE: usize = 0x14;
/// The data array's header, before its nodes.
const DATA_ARRAY_SIZE: usize = 0x38;
/// `d@t@`, a data array's signature.
const DATA_ARRAY_SIGNATURE: u32 = 0x6440_7440;
const MAX_STRING_BYTES: usize = 0x40000;
/// The longest string a node names.
const MAX_STRING: usize = 0x1000;
const MAX_SCRIPTS: usize = 512;
const MAX_GLOBALS: usize = 0x400;
const SCRIPT_SIZE: usize = 0x5C;
const GLOBAL_SIZE: usize = 0x5C;

const PRIMITIVE: i16 = 1 << 0;
const SCRIPT_CALL: i16 = 1 << 1;
const VARIABLE: i16 = 1 << 2;
/// An external (engine) global's index has this bit.
const EXTERNAL_GLOBAL: i32 = 0x8000;
const NONE: i32 = -1;

/// A node as the map holds it.
#[derive(Clone, Copy)]
struct Raw {
    salt: i16,
    index: i16,
    value_type: i16,
    flags: i16,
    next: i32,
    string: i32,
    data: i32,
}

struct Reader<'a, 'e> {
    scenario: &'e Element<'a>,
    nodes: &'a [u8],
    count: usize,
    strings: &'a [u8],
    globals: usize,
    scripts: usize,
    /// The nodes made so far, and by the map's index.
    made: Vec<Option<ScriptNode>>,
    by_index: BTreeMap<usize, u32>,
}

impl<'a> Reader<'a, '_> {
    fn fault(&self, reason: impl ToString) -> ImportError {
        self.scenario.fault("script syntax data", reason)
    }

    /// The node a link names, by its index in the array.
    fn index(&self, handle: i32) -> Result<usize, ImportError> {
        let index = usize::from(handle as u32 as u16);
        let raw = self.raw(index)?;
        // (a link's salt above its index is the node's)
        if raw.salt == 0 || raw.salt != (handle >> 16) as i16 {
            return Err(self.fault(format!("links to node {index}, which is free")));
        }
        Ok(index)
    }

    fn raw(&self, index: usize) -> Result<Raw, ImportError> {
        if index >= self.count {
            return Err(self.fault(format!("node {index} of {}", self.count)));
        }
        let at = index * NODE_SIZE;
        let read = |offset: usize| reader::i16(self.nodes, at + offset);
        let long = |offset: usize| reader::i32(self.nodes, at + offset);
        let raw = (|| {
            Ok::<_, reader::Fault>(Raw {
                salt: read(0)?,
                index: read(2)?,
                value_type: read(4)?,
                flags: read(6)?,
                next: long(8)?,
                string: long(0xC)?,
                data: long(0x10)?,
            })
        })();
        raw.map_err(|fault| self.fault(fault))
    }

    /// A string the nodes name, read as Latin-1 (c10's has bytes that are
    /// not UTF-8).
    fn string(&self, offset: i32) -> Result<String, ImportError> {
        let at = usize::try_from(offset).map_err(|_| self.fault("a string before the strings"))?;
        let rest = self.strings.get(at..).unwrap_or_default();
        let window = &rest[..rest.len().min(MAX_STRING + 1)];
        let end = window
            .iter()
            .position(|&byte| byte == 0)
            .ok_or_else(|| self.fault(format!("the string at {at} has no end")))?;
        Ok(window[..end].iter().map(|&byte| char::from(byte)).collect())
    }

    /// Makes the expression at `handle` and returns its root's index.
    fn expression(&mut self, handle: i32) -> Result<u32, ImportError> {
        // (each expression is walked alone: one reached from two roots is
        // made once, but one reached twice from one root is a loop)
        let mut walked = vec![false; self.count];
        let root = self.index(handle)?;
        let mut stack = vec![root];
        walked[root] = true;
        let root = self.place(root);
        while let Some(index) = stack.pop() {
            let Some(&at) = self.by_index.get(&index) else {
                continue;
            };
            if self.made.get(at as usize).is_some_and(Option::is_some) {
                continue;
            }
            let raw = self.raw(index)?;
            let node = if raw.flags & PRIMITIVE != 0 {
                self.primitive(&raw)?
            } else {
                let mut children = Vec::new();
                let mut link = raw.data;
                while link != NONE {
                    let child = self.index(link)?;
                    if std::mem::replace(&mut walked[child], true) {
                        return Err(self.fault(format!("node {child} is linked to twice")));
                    }
                    children.push(child);
                    link = self.raw(child)?.next;
                }
                let Some((&name, arguments)) = children.split_first() else {
                    return Err(self.fault(format!("call {index} names no function")));
                };
                let name = self.raw(name)?;
                let kind = if raw.flags & SCRIPT_CALL != 0 {
                    let script = u32::try_from(raw.index)
                        .ok()
                        .filter(|&script| (script as usize) < self.scripts)
                        .ok_or_else(|| self.fault(format!("calls script {}", raw.index)))?;
                    if !arguments.is_empty() {
                        return Err(self.fault(format!("passes script {script} arguments")));
                    }
                    NodeKind::ScriptCall(script)
                } else {
                    let mut placed = Vec::with_capacity(arguments.len());
                    for &argument in arguments {
                        placed.push(self.place(argument));
                        stack.push(argument);
                    }
                    NodeKind::Call(Call {
                        function: self.string(name.string)?,
                        arguments: placed,
                    })
                };
                ScriptNode {
                    value_type: self.value_type(raw.value_type)?,
                    kind,
                }
            };
            if let Some(slot) = self.made.get_mut(at as usize) {
                *slot = Some(node);
            }
        }
        Ok(root)
    }

    /// The index the node at `index` is made at.
    fn place(&mut self, index: usize) -> u32 {
        if let Some(&at) = self.by_index.get(&index) {
            return at;
        }
        let at = self.made.len() as u32;
        self.made.push(None);
        self.by_index.insert(index, at);
        at
    }

    fn value_type(&self, index: i16) -> Result<ValueType, ImportError> {
        ValueType::from_index(index).ok_or_else(|| self.fault(format!("a value of type {index}")))
    }

    fn primitive(&self, raw: &Raw) -> Result<ScriptNode, ImportError> {
        let value_type = self.value_type(raw.value_type)?;
        let bytes = raw.data.to_le_bytes();
        let short = i16::from_le_bytes([bytes[0], bytes[1]]);
        let kind = if raw.flags & VARIABLE != 0 {
            let external = raw.data & EXTERNAL_GLOBAL != 0;
            let index = u16::try_from(raw.data & 0x7FFF).unwrap_or(0);
            if !external && usize::from(index) >= self.globals {
                return Err(self.fault(format!("names global {index} of {}", self.globals)));
            }
            NodeKind::Global(GlobalRef {
                name: self.string(raw.string)?,
                external,
                index,
            })
        } else {
            // (a constant is decoded as the type it was written as, which
            // its index holds; the runtime casts it to the node's type)
            let written = self.value_type(raw.index)?;
            NodeKind::Constant(
                written,
                match written {
                    ValueType::Boolean => Constant::Boolean(bytes[0] != 0),
                    ValueType::Real => {
                        let real = f32::from_le_bytes(bytes);
                        if !real.is_finite() {
                            return Err(self.fault("a real that is not finite"));
                        }
                        Constant::Real(real)
                    }
                    ValueType::Short => Constant::Short(short),
                    ValueType::Long => Constant::Long(raw.data),
                    ValueType::String => Constant::String(self.string(raw.string)?),
                    tag if tag.is_tag() => Constant::Tag(if raw.data == NONE {
                        None
                    } else {
                        let cache = self.scenario.cache();
                        let index = cache
                            .handle_index(raw.data as u32)
                            .ok_or_else(|| self.fault(format!("names tag {:#x}", raw.data)))?;
                        Some(tag_ref(cache, index))
                    }),
                    // (the values below a string are the special forms' and
                    // functions' names, never constants)
                    ValueType::Unparsed
                    | ValueType::SpecialForm
                    | ValueType::FunctionName
                    | ValueType::Passthrough
                    | ValueType::Void => Constant::Long(raw.data),
                    other => Constant::Named(Named {
                        name: self.string(raw.string)?,
                        // (an encounter's or an object's is a long; the rest
                        // are shorts)
                        index: if matches!(other, ValueType::Ai | ValueType::ObjectList)
                            || other.is_object()
                        {
                            raw.data
                        } else {
                            i32::from(short)
                        },
                    }),
                },
            )
        };
        Ok(ScriptNode { value_type, kind })
    }
}

/// Imports the cache's scenario's scripts and globals and the nodes of
/// their expressions.
pub fn import_scripts(cache: &Cache) -> Result<Scripts, ImportError> {
    let scenario = cache.tag(cache.scenario_index(), SCNR, SCENARIO_SIZE)?;
    scenario_scripts(&scenario)
}

/// Imports a scenario's scripts and globals and the nodes of their
/// expressions.
pub(crate) fn scenario_scripts(scenario: &Element<'_>) -> Result<Scripts, ImportError> {
    let syntax = scenario.data(
        SYNTAX_DATA_OFFSET,
        "script syntax data",
        DATA_ARRAY_SIZE + MAX_NODES * NODE_SIZE,
    )?;
    let script_block = scenario.block(SCRIPTS_OFFSET, "scripts", SCRIPT_SIZE, MAX_SCRIPTS)?;
    let global_block = scenario.block(GLOBALS_OFFSET, "globals", GLOBAL_SIZE, MAX_GLOBALS)?;
    if syntax.is_empty() {
        if !script_block.is_empty() || !global_block.is_empty() {
            return Err(scenario.fault("script syntax data", "is empty"));
        }
        return Ok(Scripts::default());
    }
    let header =
        |at: usize| reader::i16(syntax, at).map_err(|f| scenario.fault("script syntax data", f));
    let signature =
        reader::u32(syntax, 0x28).map_err(|f| scenario.fault("script syntax data", f))?;
    let (max, size, count) = (header(0x20)?, header(0x22)?, header(0x2E)?);
    let count = usize::try_from(count).unwrap_or(usize::MAX);
    if signature != DATA_ARRAY_SIGNATURE
        || usize::try_from(size).ok() != Some(NODE_SIZE)
        || usize::try_from(max)
            .ok()
            .is_none_or(|max| count > max || max > MAX_NODES)
    {
        return Err(scenario.fault("script syntax data", "is not an array of script nodes"));
    }
    let strings = scenario.data(STRING_CONSTANTS_OFFSET, "script strings", MAX_STRING_BYTES)?;
    let mut reader = Reader {
        scenario,
        nodes: &syntax[DATA_ARRAY_SIZE..],
        count,
        strings,
        globals: global_block.len(),
        scripts: script_block.len(),
        made: Vec::new(),
        by_index: BTreeMap::new(),
    };

    let mut scripts = Vec::with_capacity(script_block.len());
    for element in script_block.iter() {
        let name = element.text(0, "name", 31)?.to_owned();
        let kind = element.i16(0x20, "type")?;
        let kind = ScriptKind::from_index(kind)
            .ok_or_else(|| element.fault("type", format!("is {kind}")))?;
        let return_type = reader.value_type(element.i16(0x22, "return type")?)?;
        let root = reader.expression(element.i32(0x24, "root expression")?)?;
        scripts.push(Script {
            name,
            kind,
            return_type,
            root,
        });
    }
    let mut globals = Vec::with_capacity(global_block.len());
    for element in global_block.iter() {
        let name = element.text(0, "name", 31)?.to_owned();
        let value_type = reader.value_type(element.i16(0x20, "type")?)?;
        let initial = element.i32(0x28, "initialization expression")?;
        let initial = if initial == NONE {
            None
        } else {
            Some(reader.expression(initial)?)
        };
        globals.push(ScriptGlobal {
            name,
            value_type,
            initial,
        });
    }
    let nodes = reader
        .made
        .into_iter()
        .collect::<Option<Vec<_>>>()
        .ok_or_else(|| scenario.fault("script syntax data", "a node was not made"))?;
    Ok(Scripts {
        nodes,
        scripts,
        globals,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, put};

    type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

    const SALT: i16 = 0x6000;

    /// A node: its type, flags, next link, string and data.
    struct Node(i16, i16, i16, i32, i32, i32);

    fn handle(index: usize) -> i32 {
        i32::from(SALT) << 16 | index as i32
    }

    /// A scenario of one global and one startup script, whose nodes are
    /// `nodes` (the script's root the first) and strings `strings`.
    fn scenario(nodes: &[Node], strings: &[u8]) -> Result<Scripts> {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let mut syntax = vec![0u8; DATA_ARRAY_SIZE + MAX_NODES * NODE_SIZE];
        put(&mut syntax, 0x20, &(MAX_NODES as i16).to_le_bytes());
        put(&mut syntax, 0x22, &(NODE_SIZE as i16).to_le_bytes());
        put(&mut syntax, 0x28, &DATA_ARRAY_SIGNATURE.to_le_bytes());
        put(&mut syntax, 0x2E, &(nodes.len() as i16).to_le_bytes());
        for (index, Node(value_type, function, flags, next, string, data)) in
            nodes.iter().enumerate()
        {
            let at = DATA_ARRAY_SIZE + index * NODE_SIZE;
            put(&mut syntax, at, &SALT.to_le_bytes());
            put(&mut syntax, at + 2, &function.to_le_bytes());
            put(&mut syntax, at + 4, &value_type.to_le_bytes());
            put(&mut syntax, at + 6, &flags.to_le_bytes());
            put(&mut syntax, at + 8, &next.to_le_bytes());
            put(&mut syntax, at + 0xC, &string.to_le_bytes());
            put(&mut syntax, at + 0x10, &data.to_le_bytes());
        }
        let syntax_size = syntax.len() as i32;
        let syntax = fake.add_data(&syntax);
        let strings_address = fake.add_data(strings);
        let mut script = vec![0u8; SCRIPT_SIZE];
        put(&mut script, 0, b"main");
        put(&mut script, 0x22, &4i16.to_le_bytes());
        put(&mut script, 0x24, &handle(0).to_le_bytes());
        let script = fake.add_data(&script);
        let mut global = vec![0u8; GLOBAL_SIZE];
        put(&mut global, 0, b"count");
        put(&mut global, 0x20, &7i16.to_le_bytes());
        put(&mut global, 0x28, &NONE.to_le_bytes());
        let global = fake.add_data(&global);

        let mut data = vec![0u8; SCENARIO_SIZE];
        put(&mut data, SYNTAX_DATA_OFFSET, &syntax_size.to_le_bytes());
        put(&mut data, SYNTAX_DATA_OFFSET + 12, &syntax.to_le_bytes());
        put(
            &mut data,
            STRING_CONSTANTS_OFFSET,
            &(strings.len() as i32).to_le_bytes(),
        );
        put(
            &mut data,
            STRING_CONSTANTS_OFFSET + 12,
            &strings_address.to_le_bytes(),
        );
        block(&mut data, SCRIPTS_OFFSET, 1, script);
        block(&mut data, GLOBALS_OFFSET, 1, global);
        fake.set_scenario_data(&data);
        Ok(import_scripts(&Cache::open(fake.build())?)?)
    }

    /// `(begin (set count 3))`: the strings are "begin\0set\0count\0".
    fn nodes() -> Vec<Node> {
        vec![
            // (begin ...), its name, (set ...), its name, count, 3
            Node(4, 0, 0, NONE, 0, handle(1)),
            Node(2, 0, PRIMITIVE, handle(2), 0, 0),
            Node(4, 4, 0, NONE, 0, handle(3)),
            Node(2, 4, PRIMITIVE, handle(4), 6, 0),
            Node(7, 0, PRIMITIVE | VARIABLE, handle(5), 10, 0),
            Node(7, 7, PRIMITIVE, NONE, 0, 3),
        ]
    }

    const STRINGS: &[u8] = b"begin\0set\0count\0";

    #[test]
    fn a_script_imports_as_a_tree() -> Result {
        let scripts = scenario(&nodes(), STRINGS)?;
        assert_eq!(scripts.scripts[0].name, "main");
        assert_eq!(scripts.scripts[0].kind, ScriptKind::Startup);
        assert_eq!(scripts.scripts[0].return_type, ValueType::Void);
        assert_eq!(scripts.globals[0].value_type, ValueType::Short);
        let NodeKind::Call(begin) = &scripts.nodes[scripts.scripts[0].root as usize].kind else {
            return Err("not a call".into());
        };
        assert_eq!(begin.function, "begin");
        let NodeKind::Call(set) = &scripts.nodes[begin.arguments[0] as usize].kind else {
            return Err("not a call".into());
        };
        assert_eq!(set.function, "set");
        let [global, value] = set.arguments[..] else {
            return Err("not two arguments".into());
        };
        assert!(matches!(
            &scripts.nodes[global as usize].kind,
            NodeKind::Global(GlobalRef { name, external: false, index: 0 }) if name == "count"
        ));
        assert_eq!(
            scripts.nodes[value as usize].kind,
            NodeKind::Constant(ValueType::Short, Constant::Short(3))
        );
        Ok(())
    }

    #[test]
    fn a_loop_among_the_nodes_is_refused() {
        // (3 links back to the call it is in)
        let mut looped = nodes();
        looped[5].3 = handle(2);
        assert!(scenario(&looped, STRINGS).is_err());
    }

    #[test]
    fn a_link_to_a_free_node_or_past_the_nodes_is_refused() {
        let mut past = nodes();
        past[5].3 = handle(40);
        assert!(scenario(&past, STRINGS).is_err());
        let mut salted = nodes();
        salted[1].3 = handle(2) ^ 0x0100_0000;
        assert!(scenario(&salted, STRINGS).is_err());
    }

    #[test]
    fn a_missing_global_or_string_is_refused() {
        let mut global = nodes();
        global[4].5 = 1;
        assert!(scenario(&global, STRINGS).is_err());
        let mut string = nodes();
        string[3].4 = 0x1000;
        assert!(scenario(&string, STRINGS).is_err());
        assert!(scenario(&nodes(), b"begin\0set\0count").is_err());
    }
}
