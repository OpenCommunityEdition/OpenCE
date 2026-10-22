//! `.ocpak`: a level in the engine's own format, as the importer writes it
//! to the cache folder and the engine reads it back.
//!
//! A pack is a 40-byte header and then an rkyv archive of the [`Level`]:
//!
//! | Offset | Size | Field |
//! | --- | --- | --- |
//! | 0x00 | 4 | `OCPK` |
//! | 0x04 | 4 | the format's version ([`FORMAT_VERSION`]) |
//! | 0x08 | 4 | the importer's version, which wrote it |
//! | 0x0C | 4 | zero |
//! | 0x10 | 8 | the source map's fingerprint ([`fingerprint`]) |
//! | 0x18 | 8 | the archive's length |
//! | 0x20 | 8 | the archive's fingerprint |
//!
//! Everything is little-endian. A pack is checked before any of it is
//! used: its header, its length, its fingerprint (any one byte changed
//! changes it), and then the archive itself, by bytecheck, so that reading
//! a damaged or hostile pack is an error, never a crash.

use rkyv::rancor;
use rkyv::util::AlignedVec;

use crate::Level;

/// The version of this format; a pack of another is not read.
pub const FORMAT_VERSION: u32 = 31;

const MAGIC: [u8; 4] = *b"OCPK";
const HEADER_SIZE: usize = 0x28;

/// What a pack says about where it came from.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PackHeader {
    /// The version of the importer that wrote it.
    pub importer_version: u32,
    /// The fingerprint of the map it was imported from.
    pub source: u64,
}

/// Why a pack was not read.
#[derive(Clone, Debug, PartialEq, Eq, thiserror::Error)]
pub enum PackError {
    /// It does not start with `OCPK`, or is shorter than its header.
    #[error("not a pack")]
    NotAPack,
    /// It is of another version of the format.
    #[error("a pack of format {0}, not {FORMAT_VERSION}")]
    FormatVersion(u32),
    /// Its length is not its header's and its archive's.
    #[error("{found} bytes long, not the {expected} its header gives")]
    Length {
        /// Its length.
        found: u64,
        /// Its header's and its archive's.
        expected: u64,
    },
    /// Its archive's fingerprint is not the one its header gives.
    #[error("damaged: its archive's fingerprint is not its header's")]
    Fingerprint,
    /// Its archive does not check, or could not be written.
    #[error("its archive: {0}")]
    Archive(String),
}

/// A fingerprint of some bytes (FNV-1a, 64-bit): changing any one byte
/// changes it.
pub fn fingerprint(bytes: &[u8]) -> u64 {
    bytes.iter().fold(0xCBF2_9CE4_8422_2325, |hash, &byte| {
        (hash ^ u64::from(byte)).wrapping_mul(0x0000_0100_0000_01B3)
    })
}

/// Writes a level as a pack.
pub fn write_pack(level: &Level, header: PackHeader) -> Result<Vec<u8>, PackError> {
    let archive = rkyv::to_bytes::<rancor::Error>(level)
        .map_err(|error| PackError::Archive(error.to_string()))?;
    let mut pack = Vec::with_capacity(HEADER_SIZE + archive.len());
    pack.extend_from_slice(&MAGIC);
    pack.extend_from_slice(&FORMAT_VERSION.to_le_bytes());
    pack.extend_from_slice(&header.importer_version.to_le_bytes());
    pack.extend_from_slice(&0u32.to_le_bytes());
    pack.extend_from_slice(&header.source.to_le_bytes());
    pack.extend_from_slice(&(archive.len() as u64).to_le_bytes());
    pack.extend_from_slice(&fingerprint(&archive).to_le_bytes());
    pack.extend_from_slice(&archive);
    Ok(pack)
}

fn u32_at(bytes: &[u8], at: usize) -> u32 {
    let mut word = [0; 4];
    word.copy_from_slice(&bytes[at..at + 4]);
    u32::from_le_bytes(word)
}

fn u64_at(bytes: &[u8], at: usize) -> u64 {
    let mut word = [0; 8];
    word.copy_from_slice(&bytes[at..at + 8]);
    u64::from_le_bytes(word)
}

/// Reads a pack's header, checking only that it is a pack of this format.
pub fn read_pack_header(pack: &[u8]) -> Result<PackHeader, PackError> {
    if pack.len() < HEADER_SIZE || pack[..4] != MAGIC {
        return Err(PackError::NotAPack);
    }
    match u32_at(pack, 4) {
        FORMAT_VERSION => Ok(PackHeader {
            importer_version: u32_at(pack, 8),
            source: u64_at(pack, 0x10),
        }),
        other => Err(PackError::FormatVersion(other)),
    }
}

/// Reads a pack, checking all of it.
pub fn read_pack(pack: &[u8]) -> Result<(PackHeader, Level), PackError> {
    let header = read_pack_header(pack)?;
    let length = u64_at(pack, 0x18);
    let expected = (HEADER_SIZE as u64).saturating_add(length);
    if pack.len() as u64 != expected {
        return Err(PackError::Length {
            found: pack.len() as u64,
            expected,
        });
    }
    let archive = &pack[HEADER_SIZE..];
    if fingerprint(archive) != u64_at(pack, 0x20) {
        return Err(PackError::Fingerprint);
    }
    // (an archive is read in place only at its alignment)
    let mut aligned = AlignedVec::<16>::with_capacity(archive.len());
    aligned.extend_from_slice(archive);
    let level = rkyv::from_bytes::<Level, rancor::Error>(&aligned)
        .map_err(|error| PackError::Archive(error.to_string()))?;
    Ok((header, level))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        Bitmap, BitmapImage, CollisionBsp, EnvironmentVertex, Group, Lightmap, LightmapVertex,
        Material, Pixels, Scenario, Shader, ShaderClass, StructureBsp, SurfaceMaterial, TagRef,
        Texture, TextureFormat,
    };

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn level() -> Level {
        let bsp = tag(b"sbsp", "levels\\test\\box\\box");
        let shader = tag(b"senv", "levels\\test\\box\\wall");
        let vertex = EnvironmentVertex {
            position: [1.0, 2.0, 3.0],
            normal: [0.0, 0.0, 1.0],
            binormal: [0.0, 1.0, 0.0],
            tangent: [1.0, 0.0, 0.0],
            texcoord: [0.5, 0.25],
        };
        Level {
            glows: Vec::new(),
            decals: Vec::new(),
            lens_flares: Vec::new(),
            light_volumes: Vec::new(),
            scenario: Scenario {
                path: "levels\\test\\box\\box".into(),
                structure_bsps: vec![bsp.clone()],
                player_starts: vec![crate::PlayerStart {
                    position: [1.0, 2.0, 3.0],
                    facing: 0.5,
                    team: 1,
                    structure_bsp: Some(0),
                    game_types: crate::GameTypes([12, 0, 0, 0]),
                }],
                placements: Vec::new(),
                netgame_flags: vec![crate::NetgameFlag {
                    kind: crate::NetgameFlagKind::CtfFlag,
                    team: 1,
                    position: [0.0, 1.0, 2.0],
                    facing: 0.0,
                }],
                netgame_equipment: Vec::new(),
                scripts: Default::default(),
                campaign: Default::default(),
                decal_palette: Vec::new(),
            },
            structure_bsps: vec![StructureBsp {
                cluster_skies: Vec::new(),
                decals: Vec::new(),
                lens_flare_markers: Vec::new(),
                lens_flares: Vec::new(),
                path: bsp.path,
                lightmap_bitmap: None,
                world_bounds: [[-1.0, 1.0]; 3],
                collision: CollisionBsp::default(),
                collision_materials: Vec::new(),
                surfaces: vec![[0, 1, 2]],
                lightmaps: vec![Lightmap {
                    bitmap_index: Some(0),
                    materials: vec![Material {
                        shader: Some(shader.clone()),
                        first_surface: 0,
                        surface_count: 1,
                        vertices: vec![vertex; 3],
                        lightmap_vertices: vec![
                            LightmapVertex {
                                incident_direction: [0.0, 0.0, -1.0],
                                texcoord: [0.0, 1.0],
                            };
                            3
                        ],
                    }],
                }],
            }],
            objects: Vec::new(),
            models: Vec::new(),
            animation_graphs: Vec::new(),
            shaders: vec![Shader::new(shader, 2, ShaderClass::Other(Group(*b"senv")))],
            bitmaps: vec![Bitmap {
                tag: tag(b"bitm", "levels\\test\\box\\bitmaps\\wall"),
                kind: 0,
                format: 0,
                usage: 0,
                flags: 0,
                bump_height: 0.0,
                sequences: Vec::new(),
                images: vec![BitmapImage {
                    width: 2,
                    height: 1,
                    depth: 1,
                    kind: 0,
                    format: 11,
                    flags: 0,
                    registration_point: [1, 0],
                    mipmap_count: 0,
                    pixels: Pixels::Texture(Texture {
                        format: TextureFormat::Rgba8,
                        width: 2,
                        height: 1,
                        depth: 1,
                        faces: 1,
                        levels: vec![vec![1, 2, 3, 4, 5, 6, 7, 8], vec![3, 4, 5, 6]],
                    }),
                }],
            }],
            resource_bitmaps: vec![tag(b"bitm", "ui\\shell\\bitmaps\\cursor")],
            globals: Default::default(),
            skies: vec![crate::Sky {
                tag: tag(b"sky ", "sky\\box\\box"),
                model: Some(tag(b"mod2", "sky\\box\\box")),
                outdoor_fog: crate::AtmosphericFog {
                    color: [0.5, 0.6, 0.7],
                    maximum_density: 0.8,
                    start_distance: 10.0,
                    opaque_distance: 300.0,
                },
                indoor_fog: crate::AtmosphericFog {
                    color: [0.0; 3],
                    maximum_density: 0.0,
                    start_distance: 0.0,
                    opaque_distance: 0.0,
                },
                indoor_ambient: crate::SkyAmbient {
                    color: [0.1, 0.1, 0.12],
                    power: 0.5,
                },
                outdoor_ambient: crate::SkyAmbient {
                    color: [0.4, 0.5, 0.6],
                    power: 1.0,
                },
                lights: vec![crate::SkyLight {
                    lens_flare: None,
                    flags: 3,
                    color: [1.0, 0.95, 0.8],
                    power: 2.0,
                    test_distance: 100.0,
                    direction: [0.5, 1.0],
                    diameter: 0.02,
                }],
            }],
            damage_effects: Vec::new(),
            collision_models: Vec::new(),
            physics: Vec::new(),
            effects: Vec::new(),
            lights: Vec::new(),
            unit_huds: Vec::new(),
            weapon_huds: Vec::new(),
            grenade_huds: Vec::new(),
            hud_digits: None,
            hud_globals: None,
        }
    }

    const HEADER: PackHeader = PackHeader {
        importer_version: 7,
        source: 0x0123_4567_89AB_CDEF,
    };

    fn pack() -> Vec<u8> {
        match write_pack(&level(), HEADER) {
            Ok(pack) => pack,
            Err(error) => panic!("{error}"),
        }
    }

    #[test]
    fn a_pack_round_trips() {
        assert_eq!(read_pack(&pack()), Ok((HEADER, level())));
        assert_eq!(read_pack_header(&pack()), Ok(HEADER));
    }

    #[test]
    fn a_shaders_material_is_in_the_pack() -> Result<(), PackError> {
        let mut level = level();
        assert_eq!(
            level.shaders[0].material,
            SurfaceMaterial::of(&level.shaders[0].class)
        );
        let mut material = SurfaceMaterial::diffuse(
            Some(tag(b"bitm", "base")),
            SurfaceMaterial::ALPHA_TESTED | SurfaceMaterial::TWO_SIDED,
        );
        material.albedo = [0.5, 0.25, 0.125];
        material.normal_map = Some(tag(b"bitm", "bump"));
        material.normal_map_scale = 4.0;
        material.roughness = 0.3;
        material.specular = [0.25, 0.5, 1.0];
        material.specular_grazing = [1.0; 3];
        material.reflection_map = Some(tag(b"bitm", "cube"));
        material.emission_map = Some(tag(b"bitm", "glow"));
        material.emission = [2.0, 0.0, 0.5];
        level.shaders[0].material = material.clone();
        let (_, read) = read_pack(&write_pack(&level, HEADER)?)?;
        assert_eq!(read.shaders[0].material, material);
        Ok(())
    }

    #[test]
    fn a_truncated_pack_is_refused() {
        let pack = pack();
        for length in 0..pack.len() {
            assert!(read_pack(&pack[..length]).is_err(), "{length} bytes read");
        }
        let mut longer = pack.clone();
        longer.push(0);
        assert!(matches!(read_pack(&longer), Err(PackError::Length { .. })));
    }

    #[test]
    fn any_flipped_byte_is_refused() {
        let pack = pack();
        for at in 0..pack.len() {
            if (0x0C..0x10).contains(&at) {
                continue; // (the unused word)
            }
            let mut flipped = pack.clone();
            flipped[at] ^= 0x40;
            if (0x08..0x0C).contains(&at) || (0x10..0x18).contains(&at) {
                // where it came from changes, which its reader checks
                assert_ne!(read_pack(&flipped).map(|(header, _)| header), Ok(HEADER));
            } else {
                assert!(read_pack(&flipped).is_err(), "byte {at} flipped was read");
            }
        }
    }

    #[test]
    fn an_archive_that_does_not_check_is_refused() {
        // a header that holds, over an archive of junk
        let junk = [0xA5u8; 64];
        let mut pack = pack()[..HEADER_SIZE].to_vec();
        pack[0x18..0x20].copy_from_slice(&(junk.len() as u64).to_le_bytes());
        pack[0x20..0x28].copy_from_slice(&fingerprint(&junk).to_le_bytes());
        pack.extend_from_slice(&junk);
        assert!(matches!(read_pack(&pack), Err(PackError::Archive(_))));
    }

    #[test]
    fn other_formats_are_refused() {
        let mut pack = pack();
        pack[4..8].copy_from_slice(&(FORMAT_VERSION + 1).to_le_bytes());
        assert_eq!(
            read_pack(&pack),
            Err(PackError::FormatVersion(FORMAT_VERSION + 1))
        );
        assert_eq!(read_pack(b"PNG"), Err(PackError::NotAPack));
    }
}
