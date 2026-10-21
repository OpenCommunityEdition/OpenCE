//! The tag index, and checked views of a tag's fields, blocks and data.

use std::fmt;

use crate::cache::{Cache, CacheKind};
use crate::error::ImportError;
use crate::reader::{self, Fault};

/// A tag group's four-character code (`scnr`), as the game writes it in
/// source: the first character in the high byte.
#[derive(Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct GroupTag(pub u32);

impl GroupTag {
    /// No group (a tag's missing parent, or an empty reference).
    pub const NONE: Self = Self(u32::MAX);

    /// The group of a code.
    pub const fn new(code: &[u8; 4]) -> Self {
        Self(u32::from_be_bytes(*code))
    }
}

impl fmt::Display for GroupTag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if *self == Self::NONE {
            return f.write_str("none");
        }
        let code = self.0.to_be_bytes();
        if code.iter().all(|&byte| (0x20..0x7f).contains(&byte)) {
            code.iter()
                .try_for_each(|&byte| write!(f, "{}", char::from(byte)))
        } else {
            write!(f, "{:#010x}", self.0)
        }
    }
}

impl fmt::Debug for GroupTag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "'{self}'")
    }
}

/// One tag of the index.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TagInstance {
    /// Its group.
    pub group: GroupTag,
    /// The groups its group derives from (`obje` for a `weap`).
    pub parents: [GroupTag; 2],
    /// Its handle: a salt above its index.
    pub handle: u32,
    /// Its path (`levels\test\bloodgulch\bloodgulch`).
    pub name: String,
    /// Where its definition is, in the tag data (or, for one in the
    /// resource maps, its index there).
    pub address: u32,
    /// Whether it is in Custom Edition's shared resource maps.
    pub in_resource_map: bool,
}

impl TagInstance {
    /// Whether the tag is of `group` or of one derived from it.
    pub fn is(&self, group: GroupTag) -> bool {
        self.group == group || self.parents.contains(&group)
    }
}

/// The tags of a cache.
#[derive(Clone, Debug, Default)]
pub(crate) struct TagIndex {
    pub(crate) instances: Vec<TagInstance>,
    pub(crate) scenario: usize,
}

const INSTANCES_OFFSET: usize = 0x00;
const SCENARIO_OFFSET: usize = 0x04;
const COUNT_OFFSET: usize = 0x0C;
const INSTANCE_SIZE: usize = 0x20;
const INSTANCE_PARENTS_OFFSET: usize = 0x04;
const INSTANCE_HANDLE_OFFSET: usize = 0x0C;
const INSTANCE_NAME_OFFSET: usize = 0x10;
const INSTANCE_ADDRESS_OFFSET: usize = 0x14;
const INSTANCE_IN_RESOURCE_MAP_OFFSET: usize = 0x18;
/// The longest tag path, terminator excluded.
pub const NAME_LONGEST: usize = 255;
/// The most tags a cache holds: a handle's index is 16 bits.
pub const MAX_TAGS: u32 = 0xFFFF;
const INDEX_MASK: u32 = 0xFFFF;
const NO_HANDLE: u32 = u32::MAX;

/// A tag block's size: count, address, definition.
pub const BLOCK_SIZE: usize = 12;
/// A tag data's size: size, flags, file offset, address, definition.
pub const DATA_SIZE: usize = 20;
/// A tag reference's size: group, name, name length, handle.
pub const REFERENCE_SIZE: usize = 16;

fn index_fault(field: impl Into<String>, reason: impl ToString) -> ImportError {
    ImportError::Index {
        field: field.into(),
        reason: reason.to_string(),
    }
}

impl TagIndex {
    /// Reads the index at the start of the tag data.
    pub(crate) fn read(kind: CacheKind, tag_data: &[u8]) -> Result<Self, ImportError> {
        let size = tag_data.len() as u32;
        let read = |at: usize, field: &str| {
            reader::u32(tag_data, at).map_err(|fault| index_fault(field, fault))
        };
        let signature = GroupTag(read(kind.tag_header_size() - 4, "signature")?);
        if signature != GroupTag::new(b"tags") {
            return Err(index_fault(
                "signature",
                format!("is {signature}, not tags"),
            ));
        }
        let count = read(COUNT_OFFSET, "tag count")?;
        if count > MAX_TAGS {
            return Err(index_fault(
                "tag count",
                format!("is {count}, more than {MAX_TAGS}"),
            ));
        }
        let count = count as usize;
        let instances_address = read(INSTANCES_OFFSET, "instances")?;
        let instances_at =
            crate::cache::tag_offset(kind, size, instances_address, count * INSTANCE_SIZE)
                .map_err(|fault| index_fault("instances", fault))?;

        let mut instances = Vec::with_capacity(count);
        for index in 0..count {
            let at = instances_at + index * INSTANCE_SIZE;
            let field = |name: &str| format!("tag {index}'s {name}");
            let word = |offset: usize, name: &str| {
                reader::u32(tag_data, at + offset).map_err(|fault| index_fault(field(name), fault))
            };
            let handle = word(INSTANCE_HANDLE_OFFSET, "handle")?;
            if handle & INDEX_MASK != index as u32 {
                return Err(index_fault(
                    field("handle"),
                    format!("is {handle:#x}, whose index is not {index}"),
                ));
            }
            let name_address = word(INSTANCE_NAME_OFFSET, "name")?;
            let name = crate::cache::tag_offset(kind, size, name_address, 1)
                .and_then(|name_at| reader::text(tag_data, name_at, NAME_LONGEST))
                .map_err(|fault| index_fault(field("name"), fault))?;
            instances.push(TagInstance {
                group: GroupTag(word(0, "group")?),
                parents: [
                    GroupTag(word(INSTANCE_PARENTS_OFFSET, "parent group")?),
                    GroupTag(word(INSTANCE_PARENTS_OFFSET + 4, "parent group")?),
                ],
                handle,
                name: name.to_owned(),
                address: word(INSTANCE_ADDRESS_OFFSET, "address")?,
                in_resource_map: kind == CacheKind::CustomEdition
                    && word(INSTANCE_IN_RESOURCE_MAP_OFFSET, "resource map flag")? != 0,
            });
        }

        let scenario_handle = read(SCENARIO_OFFSET, "scenario")?;
        let scenario = handle_index(&instances, scenario_handle)
            .filter(|&index| instances[index].group == GroupTag::new(b"scnr"))
            .ok_or_else(|| {
                index_fault(
                    "scenario",
                    format!("is {scenario_handle:#x}, which is not a scenario's handle"),
                )
            })?;
        Ok(Self {
            instances,
            scenario,
        })
    }
}

fn handle_index(instances: &[TagInstance], handle: u32) -> Option<usize> {
    let index = (handle & INDEX_MASK) as usize;
    instances
        .get(index)
        .filter(|instance| instance.handle == handle)
        .map(|_| index)
}

impl Cache {
    /// Every tag.
    pub fn tags(&self) -> &[TagInstance] {
        &self.index.instances
    }

    /// The index of the map's scenario.
    pub fn scenario_index(&self) -> usize {
        self.index.scenario
    }

    /// The index of the tag a handle names, if any does.
    pub fn handle_index(&self, handle: u32) -> Option<usize> {
        handle_index(&self.index.instances, handle)
    }

    /// The index of the tag of a group and path.
    pub fn find(&self, group: GroupTag, name: &str) -> Option<usize> {
        self.index
            .instances
            .iter()
            .position(|instance| instance.is(group) && instance.name.eq_ignore_ascii_case(name))
    }

    /// A tag's definition, `size` bytes, which must be of `group`.
    pub fn tag(
        &self,
        index: usize,
        group: GroupTag,
        size: usize,
    ) -> Result<Element<'_>, ImportError> {
        let instance = self.index.instances.get(index).ok_or_else(|| {
            index_fault(
                format!("tag {index}"),
                format!("is past the {} tags", self.index.instances.len()),
            )
        })?;
        let element = Element {
            cache: self,
            tag: index,
            path: String::new(),
            bytes: &[],
        };
        if !instance.is(group) {
            return Err(element.fault("group", format!("is not {group}")));
        }
        if instance.in_resource_map {
            return Err(element.fault("definition", "is in the resource maps"));
        }
        let at = self
            .tag_offset(instance.address, size)
            .map_err(|fault| element.fault("definition", fault))?;
        Ok(Element {
            bytes: &self.tag_data()[at..at + size],
            ..element
        })
    }
}

/// A tag's definition, or an element of one of its blocks: fields read
/// from it are checked, and a refusal names the tag and the field.
#[derive(Clone, Debug)]
pub struct Element<'a> {
    cache: &'a Cache,
    tag: usize,
    path: String,
    bytes: &'a [u8],
}

impl<'a> Element<'a> {
    /// The tag it is of.
    pub fn instance(&self) -> &'a TagInstance {
        &self.cache.index.instances[self.tag]
    }

    /// Its index in the cache's tags.
    pub fn tag_index(&self) -> usize {
        self.tag
    }

    /// The bytes it is made of.
    pub fn bytes(&self) -> &'a [u8] {
        self.bytes
    }

    /// A refusal of a field of it.
    pub fn fault(&self, field: &str, reason: impl ToString) -> ImportError {
        let instance = self.instance();
        ImportError::Tag {
            tag: instance.name.clone(),
            group: instance.group,
            field: if self.path.is_empty() {
                field.to_owned()
            } else {
                format!("{}.{field}", self.path)
            },
            reason: reason.to_string(),
        }
    }

    fn read<T>(&self, field: &str, read: Result<T, Fault>) -> Result<T, ImportError> {
        read.map_err(|fault| self.fault(field, fault))
    }

    /// A `u16` field.
    pub fn u16(&self, at: usize, field: &str) -> Result<u16, ImportError> {
        self.read(field, reader::u16(self.bytes, at))
    }

    /// An `i16` field.
    pub fn i16(&self, at: usize, field: &str) -> Result<i16, ImportError> {
        self.read(field, reader::i16(self.bytes, at))
    }

    /// A `u32` field.
    pub fn u32(&self, at: usize, field: &str) -> Result<u32, ImportError> {
        self.read(field, reader::u32(self.bytes, at))
    }

    /// An `i32` field.
    pub fn i32(&self, at: usize, field: &str) -> Result<i32, ImportError> {
        self.read(field, reader::i32(self.bytes, at))
    }

    /// An `f32` field, which must be finite.
    pub fn f32(&self, at: usize, field: &str) -> Result<f32, ImportError> {
        let value = self.read(field, reader::f32(self.bytes, at))?;
        if value.is_finite() {
            Ok(value)
        } else {
            Err(self.fault(field, format!("is {value}")))
        }
    }

    /// `size` bytes of it.
    pub fn slice(&self, at: usize, size: usize, field: &str) -> Result<&'a [u8], ImportError> {
        self.read(field, reader::slice(self.bytes, at, size))
    }

    /// A block at `at`, of elements of `element_size` bytes, of which
    /// there may be at most `longest`.
    pub fn block(
        &self,
        at: usize,
        field: &str,
        element_size: usize,
        longest: usize,
    ) -> Result<Block<'a>, ImportError> {
        let count = self.i32(at, field)?;
        let address = self.u32(at + 4, field)?;
        let count = usize::try_from(count)
            .ok()
            .filter(|&count| count <= longest)
            .ok_or_else(|| {
                self.fault(field, format!("has {count} elements, not 0 to {longest}"))
            })?;
        let start = if count == 0 {
            0
        } else {
            let size = count
                .checked_mul(element_size)
                .ok_or_else(|| self.fault(field, "is too large"))?;
            self.read(field, self.cache.tag_offset(address, size))?
        };
        Ok(Block {
            parent: self.clone(),
            field: field.to_owned(),
            count,
            start,
            element_size,
        })
    }

    /// The bytes of a tag data field at `at`, in the tag data, of at most
    /// `longest` bytes.
    pub fn data(&self, at: usize, field: &str, longest: usize) -> Result<&'a [u8], ImportError> {
        let (size, address) = self.data_size(at, field, longest)?;
        if size == 0 {
            return Ok(&[]);
        }
        let start = self.read(field, self.cache.tag_offset(address, size))?;
        Ok(&self.cache.tag_data()[start..start + size])
    }

    /// The bytes of a tag data field at `at` that are in the file, outside
    /// the tag data (a BSP's, a sound's or a bitmap's), of at most
    /// `longest` bytes.
    pub fn file_data(
        &self,
        at: usize,
        field: &str,
        longest: usize,
    ) -> Result<&'a [u8], ImportError> {
        let (size, _) = self.data_size(at, field, longest)?;
        let offset = self.u32(at + 8, field)? as usize;
        self.read(field, reader::slice(self.cache.file(), offset, size))
    }

    fn data_size(
        &self,
        at: usize,
        field: &str,
        longest: usize,
    ) -> Result<(usize, u32), ImportError> {
        let size = self.i32(at, field)?;
        let address = self.u32(at + 12, field)?;
        let size = usize::try_from(size)
            .ok()
            .filter(|&size| size <= longest)
            .ok_or_else(|| self.fault(field, format!("is {size} bytes, not 0 to {longest}")))?;
        Ok((size, address))
    }

    /// The index of the tag a reference at `at` names, which must be of
    /// one of `groups`; `None` for an empty reference.
    pub fn reference(
        &self,
        at: usize,
        field: &str,
        groups: &[GroupTag],
    ) -> Result<Option<usize>, ImportError> {
        let group = GroupTag(self.u32(at, field)?);
        let handle = self.u32(at + 12, field)?;
        if handle == NO_HANDLE {
            return Ok(None);
        }
        let index = self.cache.handle_index(handle).ok_or_else(|| {
            self.fault(
                field,
                format!("names {handle:#x}, which is not a tag's handle"),
            )
        })?;
        let target = &self.cache.index.instances[index];
        if target.group != group {
            return Err(self.fault(
                field,
                format!(
                    "is a {group} reference to '{}', a {}",
                    target.name, target.group
                ),
            ));
        }
        if !groups.is_empty() && !groups.iter().any(|&allowed| target.is(allowed)) {
            let allowed: Vec<String> = groups.iter().map(ToString::to_string).collect();
            return Err(self.fault(
                field,
                format!(
                    "names '{}', a {}, not {}",
                    target.name,
                    target.group,
                    allowed.join(" or ")
                ),
            ));
        }
        Ok(Some(index))
    }
}

/// A block of a tag: its elements, each checked to be in the tag data.
#[derive(Clone, Debug)]
pub struct Block<'a> {
    parent: Element<'a>,
    field: String,
    count: usize,
    start: usize,
    element_size: usize,
}

impl<'a> Block<'a> {
    /// How many elements it has.
    pub fn len(&self) -> usize {
        self.count
    }

    /// Whether it has none.
    pub fn is_empty(&self) -> bool {
        self.count == 0
    }

    /// Element `index`, if there is one.
    pub fn get(&self, index: usize) -> Option<Element<'a>> {
        (index < self.count).then(|| {
            let at = self.start + index * self.element_size;
            let path = if self.parent.path.is_empty() {
                format!("{}[{index}]", self.field)
            } else {
                format!("{}.{}[{index}]", self.parent.path, self.field)
            };
            Element {
                cache: self.parent.cache,
                tag: self.parent.tag,
                path,
                // (Element::block checked the whole block's range)
                bytes: &self.parent.cache.tag_data()[at..at + self.element_size],
            }
        })
    }

    /// Every element, in order.
    pub fn iter(&self) -> impl Iterator<Item = Element<'a>> + '_ {
        (0..self.count).filter_map(|index| self.get(index))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::fake::FakeCache;

    const SCNR: GroupTag = GroupTag::new(b"scnr");
    const SBSP: GroupTag = GroupTag::new(b"sbsp");

    fn message(error: ImportError) -> String {
        error.to_string()
    }

    #[test]
    fn group_tags_print_as_text() {
        assert_eq!(SCNR.to_string(), "scnr");
        assert_eq!(GroupTag::new(b"snd!").to_string(), "snd!");
        assert_eq!(GroupTag::NONE.to_string(), "none");
        assert_eq!(GroupTag(0x0102_0304).to_string(), "0x01020304");
    }

    /// A scenario with a block of two 8-byte elements at 0, a tag data of
    /// 4 bytes at 12 and a reference to a BSP at 32.
    fn level(kind: CacheKind) -> FakeCache {
        let mut fake = FakeCache::new(kind);
        let elements = fake.add_data(&[1, 0, 0, 0, 0, 0, 0x80, 0x3f, 2, 0, 0, 0, 0, 0, 0, 0x40]);
        let bytes = fake.add_data(&[9, 8, 7, 6]);
        let bsp = fake.add_tag(SBSP, "levels\\fake\\fake", &[0u8; 4]);
        let mut scenario = vec![0u8; 48];
        scenario[0..4].copy_from_slice(&2i32.to_le_bytes());
        scenario[4..8].copy_from_slice(&elements.to_le_bytes());
        scenario[12..16].copy_from_slice(&4i32.to_le_bytes());
        scenario[24..28].copy_from_slice(&bytes.to_le_bytes());
        scenario[32..36].copy_from_slice(&SBSP.0.to_le_bytes());
        scenario[44..48].copy_from_slice(&fake.handle(bsp).to_le_bytes());
        fake.set_scenario_data(&scenario);
        fake
    }

    #[test]
    fn fields_blocks_data_and_references_read() {
        for kind in [CacheKind::Xbox, CacheKind::CustomEdition] {
            let cache = match Cache::open(level(kind).build()) {
                Ok(cache) => cache,
                Err(error) => panic!("{kind:?}: {error}"),
            };
            assert_eq!(cache.tags().len(), 2);
            let scenario = cache.tag(cache.scenario_index(), SCNR, 48);
            let scenario = scenario.map_err(message);
            let Ok(scenario) = scenario else {
                panic!("{scenario:?}")
            };
            assert_eq!(scenario.instance().name, "levels\\fake\\fake");
            let block = scenario.block(0, "elements", 8, 4).map_err(message);
            let Ok(block) = block else {
                panic!("{block:?}")
            };
            assert_eq!(block.len(), 2);
            let values: Vec<_> = block
                .iter()
                .map(|element| (element.u32(0, "a").ok(), element.f32(4, "b").ok()))
                .collect();
            assert_eq!(values, [(Some(1), Some(1.0)), (Some(2), Some(2.0))]);
            assert_eq!(scenario.data(12, "bytes", 16).ok(), Some(&[9, 8, 7, 6][..]));
            assert_eq!(scenario.reference(32, "bsp", &[SBSP]).ok(), Some(Some(1)));
            assert_eq!(cache.find(SBSP, "LEVELS\\fake\\fake"), Some(1));
        }
    }

    #[test]
    fn out_of_range_blocks_and_data_name_the_tag_and_field() {
        let cache = Cache::open(level(CacheKind::CustomEdition).build());
        let Ok(cache) = cache else {
            panic!("{cache:?}")
        };
        let Ok(scenario) = cache.tag(0, SCNR, 48) else {
            panic!()
        };
        let error = scenario.block(0, "elements", 8, 1).err().map(message);
        assert_eq!(
            error.as_deref(),
            Some("tag 'levels\\fake\\fake' (scnr): elements has 2 elements, not 0 to 1")
        );
        // (an element far larger than the tag data)
        let error = scenario
            .block(0, "elements", 0x0100_0000, 4)
            .err()
            .map(message);
        assert!(
            error
                .as_deref()
                .is_some_and(|e| e
                    .starts_with("tag 'levels\\fake\\fake' (scnr): elements (0x2000000 bytes at")),
            "{error:?}"
        );
        let error = scenario.data(12, "bytes", 3).err().map(message);
        assert_eq!(
            error.as_deref(),
            Some("tag 'levels\\fake\\fake' (scnr): bytes is 4 bytes, not 0 to 3")
        );
        let Ok(block) = scenario.block(0, "elements", 8, 4) else {
            panic!()
        };
        let error = block
            .get(1)
            .and_then(|element| element.u32(6, "b").err())
            .map(message);
        assert_eq!(
            error.as_deref(),
            Some("tag 'levels\\fake\\fake' (scnr): elements[1].b (0x4 bytes at 0x6) lies outside the 0x8 bytes it is in")
        );
        assert!(block.get(2).is_none());
        let error = scenario.u32(46, "past the end").err().map(message);
        assert!(error.is_some_and(|e| e.contains("past the end (0x4 bytes at 0x2e)")));
    }

    #[test]
    fn bad_tag_groups_are_refused() {
        let cache = Cache::open(level(CacheKind::Xbox).build());
        let Ok(cache) = cache else {
            panic!("{cache:?}")
        };
        let error = cache.tag(1, SCNR, 4).err().map(message);
        assert_eq!(
            error.as_deref(),
            Some("tag 'levels\\fake\\fake' (sbsp): group is not scnr")
        );
        let Ok(scenario) = cache.tag(0, SCNR, 48) else {
            panic!()
        };
        let error = scenario
            .reference(32, "bsp", &[GroupTag::new(b"bitm")])
            .err()
            .map(message);
        assert_eq!(
            error.as_deref(),
            Some(
                "tag 'levels\\fake\\fake' (scnr): bsp names 'levels\\fake\\fake', a sbsp, not bitm"
            )
        );
        assert!(cache.tag(9, SCNR, 4).is_err());
    }

    #[test]
    fn a_bad_index_is_refused() {
        // (the instances' address, the count, a handle and a name)
        let mut fake = level(CacheKind::CustomEdition);
        fake.instances_address = Some(0x10);
        assert!(Cache::open(fake.build())
            .err()
            .map(message)
            .is_some_and(|e| e.starts_with("the tag index's instances")));
        let mut fake = level(CacheKind::CustomEdition);
        fake.count = Some(0x0100_0000);
        assert!(Cache::open(fake.build())
            .err()
            .map(message)
            .is_some_and(|e| e.starts_with("the tag index's tag count is 16777216")));
        let mut fake = level(CacheKind::Xbox);
        fake.count = Some(3);
        assert!(Cache::open(fake.build()).is_err());
        let mut fake = level(CacheKind::Xbox);
        fake.handle_salt = 0;
        fake.scenario_handle = Some(0xE174_0000);
        assert_eq!(
            Cache::open(fake.build()).err().map(message).as_deref(),
            Some("the tag index's scenario is 0xe1740000, which is not a scenario's handle")
        );
        let mut fake = level(CacheKind::Xbox);
        fake.bad_handle = Some(1);
        assert_eq!(
            Cache::open(fake.build()).err().map(message).as_deref(),
            Some("the tag index's tag 1's handle is 0xe1740000, whose index is not 1")
        );
        let mut fake = level(CacheKind::Xbox);
        fake.unterminated_names = true;
        assert!(Cache::open(fake.build())
            .err()
            .map(message)
            .is_some_and(|e| e.starts_with("the tag index's tag 0's name")));
    }

    #[test]
    fn every_single_byte_change_is_refused_or_read() {
        // (no panic, whatever the byte, in the header, the index or a tag)
        let file = level(CacheKind::CustomEdition).build();
        for at in 0..file.len() {
            if (0x80..0x7F0).contains(&at) && at != 0x76 {
                continue;
            }
            for value in [0x00, 0x01, 0x7f, 0x80, 0xff] {
                let mut changed = file.clone();
                changed[at] = value;
                if let Ok(cache) = Cache::open(changed) {
                    if let Ok(scenario) = cache.tag(cache.scenario_index(), SCNR, 48) {
                        if let Ok(block) = scenario.block(0, "elements", 8, 4) {
                            for element in block.iter() {
                                let _ = element.u32(4, "b");
                            }
                        }
                        let _ = scenario.data(12, "bytes", 64);
                        let _ = scenario.file_data(12, "bytes", 64);
                        let _ = scenario.reference(32, "bsp", &[SBSP]);
                    }
                }
            }
        }
    }

    #[test]
    fn bloodgulch_reads_when_the_maps_are_here() {
        let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../../../assets/maps/bloodgulch.map");
        let Ok(file) = std::fs::read(&path) else {
            eprintln!("skipped: {} is not here", path.display());
            return;
        };
        let cache = Cache::open(file).map_err(message);
        let Ok(cache) = cache else {
            panic!("{cache:?}")
        };
        assert_eq!(cache.header().kind, CacheKind::Xbox);
        assert_eq!(cache.header().name, "bloodgulch");
        assert_eq!(cache.tags().len(), 0x70e);
        let Ok(scenario) = cache.tag(cache.scenario_index(), SCNR, 0x5B0) else {
            panic!()
        };
        assert_eq!(
            scenario.instance().name,
            "levels\\test\\bloodgulch\\bloodgulch"
        );
        let bsps = scenario
            .block(0x5A4, "structure bsps", 0x20, 32)
            .map_err(message);
        let Ok(bsps) = bsps else { panic!("{bsps:?}") };
        assert_eq!(bsps.len(), 1);
        let Some(bsp) = bsps.get(0) else { panic!() };
        assert_eq!(
            bsp.reference(0x10, "structure bsp", &[SBSP])
                .ok()
                .flatten()
                .map(|index| cache.tags()[index].group),
            Some(SBSP)
        );
    }
}
