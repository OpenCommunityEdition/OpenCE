//! Fake caches for the tests, of either kind, with fields to break.

use crate::cache::{CacheKind, HEADER_SIZE};
use crate::tags::GroupTag;

struct FakeTag {
    group: GroupTag,
    parent: GroupTag,
    name: String,
    data: Vec<u8>,
}

/// A cache of a scenario (tag 0) and whatever tags and data are added.
/// The tag data holds its header, the added data, the tags' definitions,
/// the instances and then the names.
pub struct FakeCache {
    kind: CacheKind,
    data: Vec<u8>,
    file_data: Vec<u8>,
    tags: Vec<FakeTag>,
    /// The instances' address, if not where they are.
    pub instances_address: Option<u32>,
    /// The tag count, if not the number of tags.
    pub count: Option<u32>,
    /// The salt above each handle's index.
    pub handle_salt: u32,
    /// The scenario's handle, if not tag 0's.
    pub scenario_handle: Option<u32>,
    /// A tag whose handle's index is 0, not its own.
    pub bad_handle: Option<usize>,
    /// Whether the names lack their terminators.
    pub unterminated_names: bool,
    /// Words written into the tag index header (Custom Edition's model
    /// data), by offset.
    pub header_fields: Vec<(usize, u32)>,
}

impl FakeCache {
    pub fn new(kind: CacheKind) -> Self {
        Self {
            kind,
            data: Vec::new(),
            file_data: Vec::new(),
            tags: vec![FakeTag {
                group: GroupTag::new(b"scnr"),
                parent: GroupTag::NONE,
                name: "levels\\fake\\fake".to_owned(),
                data: vec![0; 4],
            }],
            instances_address: None,
            count: None,
            handle_salt: 0xE174,
            scenario_handle: None,
            bad_handle: None,
            unterminated_names: false,
            header_fields: Vec::new(),
        }
    }

    fn address(&self, offset: usize) -> u32 {
        self.kind.tag_base() + offset as u32
    }

    /// Adds bytes to the tag data, and returns their address.
    pub fn add_data(&mut self, bytes: &[u8]) -> u32 {
        let address = self.address(self.kind.tag_header_size() + self.data.len());
        self.data.extend_from_slice(bytes);
        while !self.data.len().is_multiple_of(4) {
            self.data.push(0);
        }
        address
    }

    /// Adds bytes to the file between its header and the tag data (where
    /// BSPs and pixels are), and returns their offset in the file.
    pub fn add_file_data(&mut self, bytes: &[u8]) -> u32 {
        let offset = (HEADER_SIZE + self.file_data.len()) as u32;
        self.file_data.extend_from_slice(bytes);
        while !self.file_data.len().is_multiple_of(4) {
            self.file_data.push(0);
        }
        offset
    }

    /// Adds a tag, and returns its index.
    pub fn add_tag(&mut self, group: GroupTag, name: &str, data: &[u8]) -> usize {
        self.tags.push(FakeTag {
            group,
            parent: GroupTag::NONE,
            name: name.to_owned(),
            data: data.to_vec(),
        });
        self.tags.len() - 1
    }

    /// Gives a tag a parent group (a shader class's `shdr`).
    pub fn set_parent(&mut self, index: usize, parent: GroupTag) {
        self.tags[index].parent = parent;
    }

    /// A tag's handle.
    pub fn handle(&self, index: usize) -> u32 {
        self.handle_salt << 16 | index as u32
    }

    /// The scenario's definition.
    pub fn set_scenario_data(&mut self, data: &[u8]) {
        self.tags[0].data = data.to_vec();
    }

    /// The cache file.
    pub fn build(&self) -> Vec<u8> {
        let mut tag_data = vec![0u8; self.kind.tag_header_size()];
        tag_data.extend_from_slice(&self.data);
        let mut addresses = Vec::new();
        for tag in &self.tags {
            addresses.push(self.address(tag_data.len()));
            tag_data.extend_from_slice(&tag.data);
            while !tag_data.len().is_multiple_of(4) {
                tag_data.push(0);
            }
        }
        let instances_at = tag_data.len();
        tag_data.resize(instances_at + self.tags.len() * 0x20, 0);
        for (index, tag) in self.tags.iter().enumerate() {
            let name_address = self.address(tag_data.len());
            tag_data.extend_from_slice(tag.name.as_bytes());
            if !self.unterminated_names {
                tag_data.push(0);
            }
            let handle = if self.bad_handle == Some(index) {
                self.handle(0)
            } else {
                self.handle(index)
            };
            let at = instances_at + index * 0x20;
            let instance = &mut tag_data[at..at + 0x20];
            instance[0..4].copy_from_slice(&tag.group.0.to_le_bytes());
            instance[4..12].fill(0xff);
            instance[4..8].copy_from_slice(&tag.parent.0.to_le_bytes());
            instance[12..16].copy_from_slice(&handle.to_le_bytes());
            instance[16..20].copy_from_slice(&name_address.to_le_bytes());
            instance[20..24].copy_from_slice(&addresses[index].to_le_bytes());
        }
        let instances_address = self
            .instances_address
            .unwrap_or_else(|| self.address(instances_at));
        let count = self.count.unwrap_or(self.tags.len() as u32);
        let scenario = self.scenario_handle.unwrap_or_else(|| self.handle(0));
        tag_data[0..4].copy_from_slice(&instances_address.to_le_bytes());
        tag_data[4..8].copy_from_slice(&scenario.to_le_bytes());
        tag_data[12..16].copy_from_slice(&count.to_le_bytes());
        for &(at, value) in &self.header_fields {
            tag_data[at..at + 4].copy_from_slice(&value.to_le_bytes());
        }
        let signature = self.kind.tag_header_size() - 4;
        tag_data[signature..signature + 4].copy_from_slice(&GroupTag::new(b"tags").0.to_le_bytes());

        let mut body = self.file_data.clone();
        body.extend_from_slice(&tag_data);
        let mut file = vec![0u8; HEADER_SIZE];
        let file_length = (HEADER_SIZE + body.len()) as u32;
        let mut put = |at: usize, bytes: &[u8]| file[at..at + bytes.len()].copy_from_slice(bytes);
        put(0, &GroupTag::new(b"head").0.to_le_bytes());
        put(4, &self.kind.version().to_le_bytes());
        put(8, &file_length.to_le_bytes());
        put(
            0x10,
            &((HEADER_SIZE + self.file_data.len()) as u32).to_le_bytes(),
        );
        put(0x14, &(tag_data.len() as u32).to_le_bytes());
        put(0x20, b"fake");
        put(0x40, b"01.01.14.2342");
        put(0x60, &1i16.to_le_bytes());
        put(0x64, &0x1234_5678u32.to_le_bytes());
        put(0x7FC, &GroupTag::new(b"foot").0.to_le_bytes());
        match self.kind {
            CacheKind::CustomEdition => file.extend_from_slice(&body),
            CacheKind::Xbox => {
                file.extend_from_slice(&miniz_oxide::deflate::compress_to_vec_zlib(&body, 6));
                // (the stock caches end with padding after the stream)
                file.extend_from_slice(&[0; 16]);
            }
        }
        file
    }
}
