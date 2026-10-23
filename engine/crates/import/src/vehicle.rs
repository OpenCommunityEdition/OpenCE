//! Vehicles' own parts of their definitions, their seats, and the physics
//! (`phys`) they move by.

use opence_content::{
    FrictionType, MassPoint, Physics, PoweredMassPoint, PoweredSeat, Vehicle, VehicleKind,
    VehicleSeat,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::tag_ref;
use crate::object::{optional_index, reference, vector, OBJECT_SIZE};
use crate::tags::{Element, GroupTag};

pub(crate) const VEHI: GroupTag = GroupTag::new(b"vehi");
pub(crate) const PHYS: GroupTag = GroupTag::new(b"phys");
const SND: GroupTag = GroupTag::new(b"snd!");
use crate::sound::FOOT;
const EFFE: GroupTag = GroupTag::new(b"effe");
const ACTV: GroupTag = GroupTag::new(b"actv");
const WEAP: GroupTag = GroupTag::new(b"weap");

pub(crate) const VEHICLE_SIZE: usize = 0x3F0;
const UNIT_PART: usize = OBJECT_SIZE;
const VEHICLE_PART: usize = 0x2F0;
pub(crate) const SEAT_SIZE: usize = 0x11C;
pub(crate) const POWERED_SEAT_SIZE: usize = 0x44;
pub(crate) const INITIAL_WEAPON_SIZE: usize = 0x24;
/// The most of each (the unit definition's blocks hold two powered seats
/// and four initial weapons).
const MAX_POWERED_SEATS: usize = 2;
const MAX_INITIAL_WEAPONS: usize = 4;
const MAX_SEATS: usize = 32;
const NAME_LONGEST: usize = 31;

pub(crate) const PHYSICS_SIZE: usize = 0x80;
pub(crate) const INERTIAL_MATRIX_SIZE: usize = 0x24;
pub(crate) const POWERED_MASS_POINT_SIZE: usize = 0x80;
pub(crate) const MASS_POINT_SIZE: usize = 0x80;
/// The most mass points and powered mass points the engine moves a
/// physics by (the C port's `MAXIMUM_MASS_POINTS_PER_PHYSICS`); the retail
/// tags have at most 22 and 2.
pub const MAX_MASS_POINTS: usize = 32;

/// A vehicle's own part of its definition and its seats.
pub fn import_vehicle(cache: &Cache, index: usize) -> Result<Vehicle, ImportError> {
    let vehicle = cache.tag(index, VEHI, VEHICLE_SIZE)?;
    let f = |at: usize, field: &str| vehicle.f32(VEHICLE_PART + at, field);
    let value = vehicle.i16(VEHICLE_PART + 0x04, "type")?;
    let kind = usize::try_from(value)
        .ok()
        .and_then(|value| VehicleKind::ALL.get(value).copied())
        .ok_or_else(|| vehicle.fault("type", format!("is {value}")))?;
    let mut function_modes = [0; 4];
    for (index, mode) in function_modes.iter_mut().enumerate() {
        *mode = vehicle.i16(VEHICLE_PART + 0x2C + index * 2, "function modes")?;
    }

    let block = vehicle.block(UNIT_PART + 0x168, "seats", SEAT_SIZE, MAX_SEATS)?;
    let mut seats = Vec::with_capacity(block.len());
    for seat in block.iter() {
        seats.push(import_seat(&seat)?);
    }
    let block = vehicle.block(
        UNIT_PART + 0x150,
        "powered seats",
        POWERED_SEAT_SIZE,
        MAX_POWERED_SEATS,
    )?;
    let mut powered_seats = Vec::with_capacity(block.len());
    for seat in block.iter() {
        powered_seats.push(PoweredSeat {
            powerup_time: seat.f32(0x04, "powerup time")?,
            powerdown_time: seat.f32(0x08, "powerdown time")?,
        });
    }
    let block = vehicle.block(
        UNIT_PART + 0x15C,
        "initial weapons",
        INITIAL_WEAPON_SIZE,
        MAX_INITIAL_WEAPONS,
    )?;
    let mut initial_weapons = Vec::with_capacity(block.len());
    for weapon in block.iter() {
        // (one of no weapon is passed over, as the game does)
        if let Some(weapon) = reference(&weapon, 0x00, "weapon", &[WEAP])? {
            initial_weapons.push(weapon);
        }
    }

    Ok(Vehicle {
        flags: vehicle.u32(VEHICLE_PART, "vehicle flags")?,
        kind,
        maximum_forward_speed: f(0x08, "maximum forward speed")?,
        maximum_reverse_speed: f(0x0C, "maximum reverse speed")?,
        speed_acceleration: f(0x10, "speed acceleration")?,
        speed_deceleration: f(0x14, "speed deceleration")?,
        maximum_left_turn: f(0x18, "maximum left turn")?,
        maximum_right_turn: f(0x1C, "maximum right turn")?,
        wheel_circumference: f(0x20, "wheel circumference")?,
        turn_rate: f(0x24, "turn rate")?,
        blur_speed: f(0x28, "blur speed")?,
        function_modes,
        maximum_left_slide: f(0x40, "maximum left slide")?,
        maximum_right_slide: f(0x44, "maximum right slide")?,
        slide_acceleration: f(0x48, "slide acceleration")?,
        slide_deceleration: f(0x4C, "slide deceleration")?,
        minimum_flipping_angular_velocity: f(0x50, "minimum flipping angular velocity")?,
        maximum_flipping_angular_velocity: f(0x54, "maximum flipping angular velocity")?,
        fixed_gun_yaw: f(0x70, "fixed gun yaw")?,
        fixed_gun_pitch: f(0x74, "fixed gun pitch")?,
        suspension_sound: reference(&vehicle, 0x3B0, "suspension sound", &[SND])?,
        crash_sound: reference(&vehicle, 0x3C0, "crash sound", &[SND])?,
        material_effects: reference(&vehicle, 0x3D0, "material effects", &[FOOT])?,
        effect: reference(&vehicle, 0x3E0, "effect", &[EFFE])?,
        seats,
        initial_weapons,
        powered_seats,
    })
}

fn import_seat(seat: &Element<'_>) -> Result<VehicleSeat, ImportError> {
    Ok(VehicleSeat {
        flags: seat.u32(0x00, "seat flags")?,
        label: seat.text(0x04, "label", NAME_LONGEST)?.to_owned(),
        marker: seat.text(0x24, "marker name", NAME_LONGEST)?.to_owned(),
        acceleration_scale: vector(seat, 0x64, "acceleration scale")?,
        yaw_rate: seat.f32(0x7C, "yaw rate")?,
        pitch_rate: seat.f32(0x80, "pitch rate")?,
        camera_marker: seat
            .text(0x84, "camera marker name", NAME_LONGEST)?
            .to_owned(),
        camera_submerged_marker: seat
            .text(0xA4, "camera submerged marker name", NAME_LONGEST)?
            .to_owned(),
        pitch_auto_level: seat.f32(0xC4, "pitch auto-level")?,
        pitch_range_minimum: seat.f32(0xC8, "pitch range minimum")?,
        pitch_range_maximum: seat.f32(0xCC, "pitch range maximum")?,
        yaw_minimum: seat.f32(0xF0, "yaw minimum")?,
        yaw_maximum: seat.f32(0xF4, "yaw maximum")?,
        built_in_actor: reference(seat, 0xF8, "built-in actor", &[ACTV])?,
    })
}

/// A physics tag: its mass points, each of whose powered mass point is
/// one of its own.
pub fn import_physics(cache: &Cache, index: usize) -> Result<Physics, ImportError> {
    let physics = cache.tag(index, PHYS, PHYSICS_SIZE)?;
    let f = |at: usize, field: &str| physics.f32(at, field);

    let matrices = physics.block(0x5C, "inertial matrix and inverse", INERTIAL_MATRIX_SIZE, 2)?;
    let inertial_matrix = match (matrices.get(0), matrices.get(1)) {
        (Some(matrix), Some(inverse)) => Some([rows(&matrix)?, rows(&inverse)?]),
        _ => None,
    };

    let block = physics.block(
        0x68,
        "powered mass points",
        POWERED_MASS_POINT_SIZE,
        MAX_MASS_POINTS,
    )?;
    let mut powered_mass_points = Vec::with_capacity(block.len());
    for point in block.iter() {
        powered_mass_points.push(PoweredMassPoint {
            name: point.text(0x00, "name", NAME_LONGEST)?.to_owned(),
            flags: point.u32(0x20, "flags")?,
            antigrav_strength: point.f32(0x24, "antigrav strength")?,
            antigrav_offset: point.f32(0x28, "antigrav offset")?,
            antigrav_height: point.f32(0x2C, "antigrav height")?,
            antigrav_damp_fraction: point.f32(0x30, "antigrav damp fraction")?,
            antigrav_normal_k1: point.f32(0x34, "antigrav normal k1")?,
            antigrav_normal_k0: point.f32(0x38, "antigrav normal k0")?,
        });
    }

    let block = physics.block(0x74, "mass points", MASS_POINT_SIZE, MAX_MASS_POINTS)?;
    let mut mass_points = Vec::with_capacity(block.len());
    for point in block.iter() {
        let value = point.i16(0x5C, "friction type")?;
        let friction_type = usize::try_from(value)
            .ok()
            .and_then(|value| FrictionType::ALL.get(value).copied())
            .ok_or_else(|| point.fault("friction type", format!("is {value}")))?;
        let radius = point.f32(0x68, "radius")?;
        if radius.is_nan() || radius < 0.0 {
            return Err(point.fault("radius", format!("is {radius}")));
        }
        mass_points.push(MassPoint {
            name: point.text(0x00, "name", NAME_LONGEST)?.to_owned(),
            powered_mass_point: optional_index(
                &point,
                0x20,
                "powered mass point",
                powered_mass_points.len(),
            )?,
            // (the model's nodes are checked against the model, which the
            // physics does not name)
            model_node: match point.i16(0x22, "model node")? {
                -1 => None,
                value => Some(
                    u16::try_from(value)
                        .map_err(|_| point.fault("model node", format!("is {value}")))?,
                ),
            },
            flags: point.u32(0x24, "flags")?,
            relative_mass: point.f32(0x28, "relative mass")?,
            mass: point.f32(0x2C, "mass")?,
            relative_density: point.f32(0x30, "relative density")?,
            density: point.f32(0x34, "density")?,
            position: vector(&point, 0x38, "position")?,
            forward: vector(&point, 0x44, "forward")?,
            up: vector(&point, 0x50, "up")?,
            friction_type,
            friction_parallel_scale: point.f32(0x60, "friction parallel scale")?,
            friction_perpendicular_scale: point.f32(0x64, "friction perpendicular scale")?,
            radius,
        });
    }

    Ok(Physics {
        tag: tag_ref(cache, index),
        radius: f(0x00, "radius")?,
        moment_scale: f(0x04, "moment scale")?,
        mass: f(0x08, "mass")?,
        center_of_mass: vector(&physics, 0x0C, "center of mass")?,
        density: f(0x18, "density")?,
        gravity_scale: f(0x1C, "gravity scale")?,
        ground_friction: f(0x20, "ground friction")?,
        ground_depth: f(0x24, "ground depth")?,
        ground_damp_fraction: f(0x28, "ground damp fraction")?,
        ground_normal_k1: f(0x2C, "ground normal k1")?,
        ground_normal_k0: f(0x30, "ground normal k0")?,
        water_friction: f(0x38, "water friction")?,
        water_depth: f(0x3C, "water depth")?,
        water_density: f(0x40, "water density")?,
        air_friction: f(0x48, "air friction")?,
        xx_moment: f(0x50, "xx moment")?,
        yy_moment: f(0x54, "yy moment")?,
        zz_moment: f(0x58, "zz moment")?,
        inertial_matrix,
        powered_mass_points,
        mass_points,
    })
}

fn rows(matrix: &Element<'_>) -> Result<[[f32; 3]; 3], ImportError> {
    Ok([
        vector(matrix, 0x00, "yy+zz -xy -zx")?,
        vector(matrix, 0x0C, "-xy zz+xx -yz")?,
        vector(matrix, 0x18, "-zx -yz xx+yy")?,
    ])
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, empty, put, stock_map};
    use crate::object::{import_object, OBJE};
    use opence_content::ObjectKind;
    use std::error::Error;

    type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

    fn floats(values: &[f32]) -> Vec<u8> {
        values
            .iter()
            .flat_map(|value| value.to_le_bytes())
            .collect()
    }

    fn vehicle_cache(kind: i16) -> Result<(Cache, usize)> {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let mut seat = vec![0u8; SEAT_SIZE];
        put(&mut seat, 0, &(VehicleSeat::DRIVER).to_le_bytes());
        put(&mut seat, 0x04, b"W-driver");
        put(&mut seat, 0x24, b"driver");
        put(&mut seat, 0x7C, &floats(&[2.0, 1.5]));
        put(&mut seat, 0x84, b"driver camera");
        put(&mut seat, 0xC8, &floats(&[-1.0, 1.0]));
        empty(&mut seat, 0xF8);
        let seats = fake.add_data(&seat);
        let mut powered = vec![0u8; POWERED_SEAT_SIZE];
        put(&mut powered, 0x04, &floats(&[0.5, 2.0]));
        let powered = fake.add_data(&powered);

        let mut vehicle = vec![0u8; VEHICLE_SIZE];
        put(&mut vehicle, 0, &1i16.to_le_bytes());
        for at in [0x28, 0x38, 0x70, 0x80, 0x90, 0xA0] {
            empty(&mut vehicle, at);
        }
        block(&mut vehicle, UNIT_PART + 0x168, 1, seats);
        block(&mut vehicle, UNIT_PART + 0x150, 1, powered);
        put(
            &mut vehicle,
            VEHICLE_PART,
            &Vehicle::CAUSES_COLLISION_DAMAGE.to_le_bytes(),
        );
        put(&mut vehicle, VEHICLE_PART + 0x04, &kind.to_le_bytes());
        put(
            &mut vehicle,
            VEHICLE_PART + 0x08,
            &floats(&[30.0, 10.0, 0.5, 0.25, 0.6, -0.6, 2.5]),
        );
        for at in [0x3B0, 0x3C0, 0x3D0, 0x3E0] {
            empty(&mut vehicle, at);
        }
        let index = fake.add_tag(VEHI, "vehicles\\warthog\\warthog", &vehicle);
        fake.set_parent(index, OBJE);
        Ok((Cache::open(fake.build())?, index))
    }

    #[test]
    fn a_vehicle_has_its_kind_speeds_and_seats() -> Result {
        let (cache, index) = vehicle_cache(1)?;
        let object = import_object(&cache, index)?;
        assert_eq!(object.kind, ObjectKind::Vehicle);
        let vehicle = object.vehicle.ok_or("no vehicle part")?;
        assert_eq!(vehicle.kind, VehicleKind::HumanJeep);
        assert_eq!(vehicle.flags, Vehicle::CAUSES_COLLISION_DAMAGE);
        assert_eq!(
            [
                vehicle.maximum_forward_speed,
                vehicle.maximum_reverse_speed,
                vehicle.maximum_right_turn,
                vehicle.wheel_circumference
            ],
            [30.0, 10.0, -0.6, 2.5]
        );
        let seat = &vehicle.seats[0];
        assert_eq!(seat.flags, VehicleSeat::DRIVER);
        assert_eq!(
            (seat.label.as_str(), seat.marker.as_str()),
            ("W-driver", "driver")
        );
        assert_eq!((seat.yaw_rate, seat.pitch_rate), (2.0, 1.5));
        assert_eq!(seat.camera_marker, "driver camera");
        assert_eq!(
            (seat.pitch_range_minimum, seat.pitch_range_maximum),
            (-1.0, 1.0)
        );
        assert_eq!(seat.built_in_actor, None);
        assert_eq!(
            vehicle.powered_seats,
            [PoweredSeat {
                powerup_time: 0.5,
                powerdown_time: 2.0
            }]
        );
        assert!(vehicle.initial_weapons.is_empty());
        Ok(())
    }

    #[test]
    fn a_vehicle_of_no_known_type_is_refused() -> Result {
        for kind in [7, -1] {
            let (cache, index) = vehicle_cache(kind)?;
            assert!(import_object(&cache, index).is_err(), "{kind}");
        }
        Ok(())
    }

    /// A physics with two powered mass points and three mass points, the
    /// last pushed by `powered`.
    fn physics_cache(powered: i16, friction_type: i16) -> Result<(Cache, usize)> {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let mut matrices = Vec::new();
        for scale in [2.0, 0.5] {
            let mut matrix = vec![0u8; INERTIAL_MATRIX_SIZE];
            put(&mut matrix, 0x00, &floats(&[scale, 0.0, 0.0]));
            put(&mut matrix, 0x10, &floats(&[scale]));
            put(&mut matrix, 0x20, &floats(&[scale]));
            matrices.extend(matrix);
        }
        let matrices = fake.add_data(&matrices);
        let mut powered_points = Vec::new();
        for name in [&b"front"[..], b"back"] {
            let mut point = vec![0u8; POWERED_MASS_POINT_SIZE];
            put(&mut point, 0, name);
            put(
                &mut point,
                0x20,
                &PoweredMassPoint::GROUND_FRICTION.to_le_bytes(),
            );
            put(&mut point, 0x24, &floats(&[1.0, 0.1, 0.4, 0.5, 0.7, 0.6]));
            powered_points.extend(point);
        }
        let powered_points = fake.add_data(&powered_points);
        let mut points = Vec::new();
        for (index, powered) in [-1, 0, powered].into_iter().enumerate() {
            let mut point = vec![0u8; MASS_POINT_SIZE];
            put(&mut point, 0, b"wheel");
            put(&mut point, 0x20, &powered.to_le_bytes());
            put(&mut point, 0x22, &(-1i16).to_le_bytes());
            put(&mut point, 0x28, &floats(&[1.0, 100.0, 1.0, 0.5]));
            put(&mut point, 0x38, &floats(&[index as f32, 0.0, 0.0]));
            put(&mut point, 0x44, &floats(&[1.0, 0.0, 0.0, 0.0, 0.0, 1.0]));
            put(&mut point, 0x5C, &friction_type.to_le_bytes());
            put(&mut point, 0x60, &floats(&[1.0, 0.25, 0.3]));
            points.extend(point);
        }
        let points = fake.add_data(&points);

        let mut physics = vec![0u8; PHYSICS_SIZE];
        put(&mut physics, 0, &floats(&[0.5, 1.0, 300.0]));
        put(&mut physics, 0x1C, &floats(&[1.0, 0.2]));
        put(&mut physics, 0x58, &floats(&[12.0]));
        block(&mut physics, 0x5C, 2, matrices);
        block(&mut physics, 0x68, 2, powered_points);
        block(&mut physics, 0x74, 3, points);
        let index = fake.add_tag(PHYS, "vehicles\\warthog\\warthog", &physics);
        Ok((Cache::open(fake.build())?, index))
    }

    #[test]
    fn a_physics_has_its_mass_points_and_matrices() -> Result {
        let (cache, index) = physics_cache(1, 1)?;
        let physics = import_physics(&cache, index)?;
        assert_eq!((physics.mass, physics.gravity_scale), (300.0, 1.0));
        assert_eq!((physics.ground_friction, physics.zz_moment), (0.2, 12.0));
        let [matrix, inverse] = physics.inertial_matrix.ok_or("no matrices")?;
        assert_eq!(
            [matrix[0][0], matrix[1][1], matrix[2][2], inverse[1][1]],
            [2.0, 2.0, 2.0, 0.5]
        );
        let powered = &physics.powered_mass_points[1];
        assert_eq!(powered.name, "back");
        assert_eq!(powered.flags, PoweredMassPoint::GROUND_FRICTION);
        assert_eq!(
            (powered.antigrav_height, powered.antigrav_normal_k0),
            (0.4, 0.6)
        );
        let points = &physics.mass_points;
        assert_eq!(
            points
                .iter()
                .map(|point| point.powered_mass_point)
                .collect::<Vec<_>>(),
            [None, Some(0), Some(1)]
        );
        assert_eq!(points[2].position, [2.0, 0.0, 0.0]);
        assert_eq!(
            (points[2].forward, points[2].up),
            ([1.0, 0.0, 0.0], [0.0, 0.0, 1.0])
        );
        assert_eq!(points[2].friction_type, FrictionType::Forward);
        assert_eq!((points[2].mass, points[2].radius), (100.0, 0.3));
        assert_eq!(points[2].model_node, None);
        Ok(())
    }

    #[test]
    fn a_mass_point_past_the_powered_ones_or_of_no_friction_type_is_refused() -> Result {
        let (cache, index) = physics_cache(2, 0)?;
        assert!(import_physics(&cache, index).is_err());
        let (cache, index) = physics_cache(1, 4)?;
        assert!(import_physics(&cache, index).is_err());
        Ok(())
    }

    #[test]
    fn bloodgulchs_vehicles_import_when_the_maps_are_here() -> Result {
        let Some(cache) = stock_map("bloodgulch") else {
            return Ok(());
        };
        let level = crate::level::import_level(&cache)?;
        let vehicle = |path: &str| {
            level
                .objects
                .iter()
                .find(|object| object.tag.path == path)
                .and_then(|object| Some((object.vehicle.clone()?, object.physics.clone()?)))
                .ok_or(format!("no {path}"))
        };
        let physics = |tag: &opence_content::TagRef| {
            level
                .physics
                .iter()
                .find(|physics| &physics.tag == tag)
                .ok_or(format!("no physics {}", tag.path))
        };

        let (warthog, warthog_physics) = vehicle("vehicles\\warthog\\warthog")?;
        assert_eq!(warthog.kind, VehicleKind::HumanJeep);
        assert!(warthog.maximum_forward_speed > 0.0 && warthog.wheel_circumference > 0.0);
        assert!(warthog
            .seats
            .iter()
            .any(|seat| seat.flags & VehicleSeat::DRIVER != 0));
        assert!(warthog
            .seats
            .iter()
            .any(|seat| seat.flags & VehicleSeat::GUNNER != 0));
        // (its gunner's chaingun, and its driver's power)
        assert_eq!(
            warthog
                .initial_weapons
                .iter()
                .map(|weapon| weapon.path.as_str())
                .collect::<Vec<_>>(),
            ["vehicles\\warthog\\warthog gun"]
        );
        assert_eq!(warthog.powered_seats.len(), 1);
        assert!(warthog
            .powered_seats
            .iter()
            .all(|seat| seat.powerup_time > 0.0 && seat.powerdown_time > 0.0));
        let warthog_physics = physics(&warthog_physics)?;
        assert_eq!(warthog_physics.powered_mass_points.len(), 2);
        assert!(warthog_physics.mass_points.len() >= 4);
        assert!(warthog_physics.mass > 0.0);

        let (ghost, ghost_physics) = vehicle("vehicles\\ghost\\ghost_mp")?;
        assert_eq!(ghost.kind, VehicleKind::AlienScout);
        let ghost_physics = physics(&ghost_physics)?;
        assert!(ghost_physics
            .powered_mass_points
            .iter()
            .any(|point| point.flags & PoweredMassPoint::ANTIGRAV != 0));
        let (scorpion, _) = vehicle("vehicles\\scorpion\\scorpion_mp")?;
        assert_eq!(scorpion.kind, VehicleKind::HumanTank);
        Ok(())
    }

    #[test]
    fn every_stock_vehicle_and_physics_imports_when_the_maps_are_here() -> Result {
        let mut kinds = std::collections::BTreeSet::new();
        for name in [
            "a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40",
        ] {
            let Some(cache) = stock_map(name) else {
                continue;
            };
            for (index, tag) in cache.tags().iter().enumerate() {
                let label = format!("{name}: '{}'", tag.name);
                if tag.is(VEHI) {
                    let vehicle =
                        import_vehicle(&cache, index).map_err(|e| format!("{label}: {e}"))?;
                    kinds.insert(format!("{:?}", vehicle.kind));
                } else if tag.is(PHYS) {
                    import_physics(&cache, index).map_err(|e| format!("{label}: {e}"))?;
                }
            }
        }
        Ok(())
    }
}
