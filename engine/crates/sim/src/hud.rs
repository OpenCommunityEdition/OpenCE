//! What the HUD shows of the world: the player's shield and health, its
//! weapon's rounds and heat, its grenades, and the motion sensor's blips
//! (`motion_sensor.c`).
//!
//! None of it is the world's state: the motion sensor is kept beside the
//! world, as the player is, and never hashed.

use glam::{Vec2, Vec3};
use opence_content::TagRef;

use crate::armory::GRENADE_KINDS;
use crate::weapon::MagazinePhase;

/// The scans the sensor remembers: its blips fade over the last few
/// (`MOTION_SENSOR_HISTORY_COUNT`).
pub const HISTORY: usize = 10;
/// The game's ticks between scans (`MOTION_SENSOR_UPDATE_PERIOD`, of its
/// 30 a second).
pub const UPDATE_PERIOD: u64 = 15;
/// The most blips a scan holds (`MAXIMUM_MOTION_SENSOR_BLIPS`).
pub const MAXIMUM_BLIPS: usize = 16;
/// How near a blip is drawn to the centre at least, in world units.
const MINIMUM_DISTANCE: f32 = 0.015625;
/// The HUD pixels of a blip's radius for each of its size's units (the
/// rasterizer's sixteenth of a 32-pixel half-field).
const PIXELS_PER_SIZE: f32 = 2.0;

/// Who a blip is.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BlipKind {
    /// The player.
    Own,
    /// A friend on foot.
    Friend,
    /// An enemy on foot.
    Enemy,
    /// A friendly vehicle.
    VehicleFriend,
    /// An enemy vehicle.
    VehicleEnemy,
}

impl BlipKind {
    /// Its colour (`blip_colors`).
    pub fn color(self) -> [f32; 3] {
        match self {
            BlipKind::Own => [1.0, 0.5, 0.0],
            BlipKind::Friend | BlipKind::VehicleFriend => [1.0, 1.0, 0.0],
            BlipKind::Enemy | BlipKind::VehicleEnemy => [1.0, 0.0, 0.0],
        }
    }
}

/// How large a unit's blip is (its unit's `blip type`).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum BlipSize {
    /// A biped's.
    #[default]
    Medium,
    /// A grunt's.
    Small,
    /// A vehicle's.
    Large,
}

impl BlipSize {
    /// What it adds to a blip's radius (`blip_size_get`).
    fn extra(self) -> f32 {
        match self {
            BlipSize::Medium => 0.0,
            BlipSize::Small => -0.75,
            BlipSize::Large => 1.0,
        }
    }
}

/// A unit the sensor might see.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Contact {
    /// Its centre, in world units.
    pub position: Vec3,
    /// Its speed squared, in world units a game tick (the game's 30 a
    /// second), as the sensor's sensitivity is.
    pub speed_squared: f32,
    /// Whether it is firing or throwing.
    pub firing: bool,
    /// Whether it is camouflaged.
    pub camouflaged: bool,
    /// Who it is.
    pub kind: BlipKind,
    /// How large its blip is.
    pub size: BlipSize,
}

/// A blip a scan saw: where it was, from where the viewer stood, in
/// 127ths of the sensor's range (the game's tiny points).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ScannedBlip {
    /// The x and y, -127 to 127.
    pub position: [i8; 2],
    /// Who it is.
    pub kind: BlipKind,
    /// How large it is.
    pub size: BlipSize,
}

/// What the sensor's settings are (the HUD globals').
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SensorSettings {
    /// How far it sees, in world units.
    pub range: f32,
    /// The speed squared, in world units a game tick, a unit must move at
    /// to show.
    pub velocity_sensitivity: f32,
    /// The HUD pixels from its centre to its edge.
    pub scale: f32,
    /// Whether the game is multiplayer: heights are ignored and the
    /// camouflaged show.
    pub multiplayer: bool,
}

/// One scan (`motion_sensor_update`): the contacts in range that move,
/// fire or throw, from `viewer`.
pub fn scan(viewer: Vec3, contacts: &[Contact], settings: &SensorSettings) -> Vec<ScannedBlip> {
    let range = settings.range;
    if range <= 0.0 {
        return Vec::new();
    }
    let mut blips = Vec::new();
    for contact in contacts {
        if blips.len() == MAXIMUM_BLIPS {
            break;
        }
        let mut offset = contact.position - viewer;
        if settings.multiplayer {
            offset.z = 0.0;
        }
        if offset.length_squared() > range * range {
            continue;
        }
        let visible = !contact.camouflaged || settings.multiplayer;
        let moving = contact.speed_squared >= settings.velocity_sensitivity && visible;
        if !(contact.firing || moving) {
            continue;
        }
        let tiny = |value: f32| {
            // (in range, so -127 to 127)
            libm::roundf((value / range * 127.0).clamp(-127.0, 127.0)) as i8
        };
        blips.push(ScannedBlip {
            position: [tiny(offset.x), tiny(offset.y)],
            kind: contact.kind,
            size: contact.size,
        });
    }
    blips
}

/// A blip as the HUD draws it.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Blip {
    /// From the sensor's centre, in HUD pixels: x to the right, y down
    /// (ahead is up).
    pub offset: [f32; 2],
    /// Its radius, in HUD pixels.
    pub radius: f32,
    /// Its colour, faded.
    pub color: [f32; 3],
}

/// Where a scanned blip is drawn (`render_blip`), facing `yaw` (the turn
/// to the left from +X), and its radius for `history` scans ago; `None`
/// if it is out of the sensor's range.
pub fn place_blip(
    blip: &ScannedBlip,
    yaw: f32,
    history: usize,
    settings: &SensorSettings,
) -> Option<Blip> {
    let range = settings.range;
    let world =
        Vec2::new(f32::from(blip.position[0]), f32::from(blip.position[1])) * (range / 127.0);
    // (ahead along +x, to the left along +y)
    let (sin, cos) = libm::sincosf(-yaw);
    let turned = Vec2::new(world.x * cos - world.y * sin, world.x * sin + world.y * cos);
    let distance_squared = turned.length_squared();
    if distance_squared >= range * range {
        return None;
    }
    let distance = libm::sqrtf(distance_squared).max(MINIMUM_DISTANCE);
    let mapped = range * libm::powf(distance / range, 0.7);
    let placed = turned / distance * mapped * (settings.scale / range);
    let weight = (HISTORY - history.min(HISTORY)) as f32 * 0.1;
    let fade = weight * weight;
    let pulse = libm::powf(1.0 - weight, 3.5) * 7.0 + 1.0;
    let color = blip.kind.color().map(|channel| channel * fade);
    Some(Blip {
        offset: [-placed.y, -placed.x],
        radius: (blip.size.extra() + pulse) * PIXELS_PER_SIZE,
        color,
    })
}

/// The motion sensor: its last scans, newest first.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct MotionSensor {
    scans: Vec<Vec<ScannedBlip>>,
}

impl MotionSensor {
    /// One game tick of 30 a second (`motion_sensor_update`): a scan
    /// every [`UPDATE_PERIOD`], the last one again between.
    pub fn update(&mut self, game_tick: u64, scanned: impl FnOnce() -> Vec<ScannedBlip>) {
        let scan = if game_tick.is_multiple_of(UPDATE_PERIOD) {
            scanned()
        } else {
            self.scans.first().cloned().unwrap_or_default()
        };
        self.scans.insert(0, scan);
        self.scans.truncate(HISTORY);
    }

    /// Its scans, newest first.
    pub fn scans(&self) -> &[Vec<ScannedBlip>] {
        &self.scans
    }

    /// Every blip as the HUD draws it, facing `yaw`: the oldest first, so
    /// the newest are drawn over them.
    pub fn blips(&self, yaw: f32, settings: &SensorSettings) -> Vec<Blip> {
        self.scans
            .iter()
            .enumerate()
            .rev()
            .flat_map(|(history, scan)| {
                scan.iter()
                    .filter_map(move |blip| place_blip(blip, yaw, history, settings))
            })
            .collect()
    }
}

/// A magazine as the HUD counts it.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct MagazineCount {
    /// The rounds in it.
    pub loaded: i16,
    /// The rounds carried for it.
    pub total: i16,
    /// Whether it is reloading.
    pub reloading: bool,
}

/// The weapon in the player's hands, as the HUD shows it.
#[derive(Clone, Debug, PartialEq)]
pub struct WeaponReadout {
    /// Its tag.
    pub weapon: TagRef,
    /// Its magazines.
    pub magazines: Vec<MagazineCount>,
    /// Its heat, 0 to 1.
    pub heat: f32,
    /// Its battery's age, 0 to 1.
    pub age: f32,
    /// Whether it is cooling from overheating.
    pub overheated: bool,
}

/// What the HUD shows this frame.
#[derive(Clone, Debug, PartialEq)]
pub struct HudState {
    /// The player's unit's tag.
    pub unit: TagRef,
    /// Its shield, 0 to 1 (more when overcharged).
    pub shield: f32,
    /// Its health, 0 to 1.
    pub body: f32,
    /// Whether its shield is down.
    pub shield_depleted: bool,
    /// Whether it is dead.
    pub dead: bool,
    /// The weapon in its hands.
    pub weapon: Option<WeaponReadout>,
    /// The grenades it carries, of each kind.
    pub grenades: [i16; GRENADE_KINDS],
    /// The kind it throws.
    pub grenade: usize,
    /// The motion sensor's blips.
    pub blips: Vec<Blip>,
    /// What the action button would do, as `hud_show_action_response`
    /// tells the player.
    pub prompt: Option<ActionPrompt>,
}

/// What the action button would do, the state message the HUD shows for
/// it (as `player_examine_nearby_objects` sets the action result; the
/// original never sets leaving a vehicle, so neither is it here).
#[derive(Clone, Debug, PartialEq)]
pub enum ActionPrompt {
    /// Enter seat `seat` of the vehicle `vehicle`.
    EnterVehicle {
        /// The vehicle's tag.
        vehicle: TagRef,
        /// The seat.
        seat: usize,
    },
    /// Turn the vehicle `vehicle` back over.
    FlipVehicle {
        /// The vehicle's tag.
        vehicle: TagRef,
    },
    /// Swap the weapon in its hands for `weapon`.
    SwapWeapon {
        /// The weapon's tag.
        weapon: TagRef,
    },
    /// Its weapon is empty: switch to `weapon`, the next it carries that
    /// is not.
    RemindToSwitch {
        /// The weapon's tag.
        weapon: TagRef,
    },
}

/// A magazine's count.
pub(crate) fn magazine_count(magazine: &crate::weapon::MagazineState) -> MagazineCount {
    MagazineCount {
        loaded: magazine.loaded,
        total: magazine.total,
        reloading: magazine.phase == MagazinePhase::Reloading,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn settings() -> SensorSettings {
        SensorSettings {
            range: 20.0,
            velocity_sensitivity: 0.0002,
            scale: 32.0,
            multiplayer: true,
        }
    }

    fn contact(position: Vec3, speed_squared: f32) -> Contact {
        Contact {
            position,
            speed_squared,
            firing: false,
            camouflaged: false,
            kind: BlipKind::Enemy,
            size: BlipSize::Medium,
        }
    }

    #[test]
    fn scans_only_moving_units_in_range() {
        let blips = scan(
            Vec3::ZERO,
            &[
                contact(Vec3::new(10.0, 0.0, 30.0), 0.01),
                contact(Vec3::new(30.0, 0.0, 0.0), 0.01),
                contact(Vec3::new(5.0, 0.0, 0.0), 0.0),
            ],
            &settings(),
        );
        // (heights are ignored in multiplayer; the still and the far are not seen)
        assert_eq!(blips.len(), 1);
        assert_eq!(blips[0].position, [64, 0]);
    }

    #[test]
    fn firing_units_show_though_still() {
        let mut firing = contact(Vec3::new(5.0, 0.0, 0.0), 0.0);
        firing.firing = true;
        assert_eq!(scan(Vec3::ZERO, &[firing], &settings()).len(), 1);
    }

    #[test]
    fn ahead_is_up_and_the_edge_is_the_scale() {
        let ahead = ScannedBlip {
            position: [127, 0],
            kind: BlipKind::Enemy,
            size: BlipSize::Medium,
        };
        // (just inside the edge: the mapping is range * (d / range)^0.7)
        let near_edge = ScannedBlip {
            position: [126, 0],
            ..ahead
        };
        assert!(place_blip(&ahead, 0.0, 0, &settings()).is_none());
        let Some(placed) = place_blip(&near_edge, 0.0, 0, &settings()) else {
            panic!("in range");
        };
        let expected = 32.0 * libm::powf(126.0 / 127.0, 0.7);
        assert!(placed.offset[0].abs() < 1e-4);
        assert!((placed.offset[1] + expected).abs() < 1e-3);
        // (the newest scan: full colour, the smallest pulse)
        assert_eq!(placed.color, [1.0, 0.0, 0.0]);
        assert!((placed.radius - 2.0).abs() < 1e-5);
    }

    #[test]
    fn turning_left_moves_blips_right() {
        let ahead = ScannedBlip {
            position: [64, 0],
            kind: BlipKind::Friend,
            size: BlipSize::Medium,
        };
        let Some(placed) = place_blip(&ahead, std::f32::consts::FRAC_PI_2, 0, &settings()) else {
            panic!("in range");
        };
        assert!(placed.offset[0] > 0.0);
        assert!(placed.offset[1].abs() < 1e-3);
    }

    #[test]
    fn near_blips_are_spread_out() {
        let near = ScannedBlip {
            position: [13, 0],
            kind: BlipKind::Enemy,
            size: BlipSize::Medium,
        };
        let Some(placed) = place_blip(&near, 0.0, 0, &settings()) else {
            panic!("in range");
        };
        // (a tenth of the range is drawn a fifth of the way out)
        assert!(-placed.offset[1] > 32.0 * 0.1 * 1.5);
    }

    #[test]
    fn older_scans_fade_and_grow() {
        let blip = ScannedBlip {
            position: [64, 0],
            kind: BlipKind::Enemy,
            size: BlipSize::Medium,
        };
        let (Some(new), Some(old)) = (
            place_blip(&blip, 0.0, 0, &settings()),
            place_blip(&blip, 0.0, 5, &settings()),
        ) else {
            panic!("in range");
        };
        assert!(old.color[0] < new.color[0]);
        assert!(old.radius > new.radius);
    }

    #[test]
    fn the_sensor_scans_every_period_and_remembers() {
        let mut sensor = MotionSensor::default();
        let blip = ScannedBlip {
            position: [10, 10],
            kind: BlipKind::Enemy,
            size: BlipSize::Medium,
        };
        let mut calls = 0;
        for tick in 0..UPDATE_PERIOD * 2 {
            sensor.update(tick, || {
                calls += 1;
                vec![blip]
            });
        }
        assert_eq!(calls, 2);
        assert_eq!(sensor.scans().len(), HISTORY);
        assert!(sensor.scans().iter().all(|scan| scan == &vec![blip]));
        assert_eq!(sensor.blips(0.0, &settings()).len(), HISTORY);
    }
}
