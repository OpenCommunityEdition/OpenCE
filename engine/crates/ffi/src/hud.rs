//! The HUD as the app draws it: quads of the HUD's bitmaps, placed as the
//! game places them (`hud_draw.c`, `hud_unit.c`, `hud_weapon.c`) on a
//! screen 480 of the HUD's pixels high and as wide as the window is.
//!
//! Each quad names its bitmap by tag and image, which the app finds among
//! the high-resolution redraws (`port/assets/hud`); a meter's quad says
//! how much of it is filled.

use opence_content::hud::{
    CrosshairKind, HudAnchor, HudIcon, HudMeter, HudNumber, HudPlacement, HudStatic, WeaponHud,
    WeaponHudElement, WeaponHudState,
};
use opence_content::{Bitmap, Level, TagRef};
use opence_sim::hud::{HudState, WeaponReadout};

/// The game's ticks a second, as the icons' frame rates count them.
const TICKS_PER_SECOND: f32 = 30.0;

/// The height of the HUD's screen, in its pixels (the Xbox's 480).
pub const HUD_HEIGHT: f32 = 480.0;
/// The width of the HUD's screen at 4:3; a wider window is wider.
pub const HUD_WIDTH: f32 = 640.0;

/// How a meter's quad is filled: its texels whose meter value is at most
/// `threshold` are filled, from `min_color` to `max_color` along the
/// meter; the rest are `empty_color`.
#[derive(Clone, Copy, Debug, PartialEq, uniffi::Record)]
pub struct HudMeterFill {
    /// The meter value up to which it is filled, 0 to 1.
    pub threshold: f32,
    /// The colour of its filled part where the meter starts (`0xAARRGGBB`).
    pub min_color: u32,
    /// The colour of its filled part where it ends.
    pub max_color: u32,
    /// The colour of its empty part.
    pub empty_color: u32,
}

/// One of the HUD's bitmaps, drawn on the screen.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct HudQuad {
    /// Its bitmap's tag path.
    pub bitmap: String,
    /// The image of the bitmap.
    pub image: u32,
    /// Its left edge, in the window's pixels.
    pub left: f32,
    /// Its top edge.
    pub top: f32,
    /// Its right edge.
    pub right: f32,
    /// Its bottom edge.
    pub bottom: f32,
    /// The image's left texture coordinate, 0 to 1.
    pub u0: f32,
    /// Its top one.
    pub v0: f32,
    /// Its right one.
    pub u1: f32,
    /// Its bottom one.
    pub v1: f32,
    /// What it is tinted (`0xAARRGGBB`; the alpha is how opaque it is).
    pub color: u32,
    /// How it is filled, if it is a meter.
    pub meter: Option<HudMeterFill>,
}

/// How an element is drawn (`_hud_draw_flashing_bit` and the rest).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
struct DrawFlags {
    flashing: bool,
    disabled: bool,
}

impl DrawFlags {
    fn new(flashing: bool, disabled: bool) -> Self {
        Self { flashing, disabled }
    }
}

/// The overcharged shield's colours, after the first meter's own
/// (`hud_render_unit_interface`).
const OVERCHARGE_COLORS: [u32; 4] = [0x00FF_0000, 0x0000_FF00, 0x00FF_FF00, 0x007F_00FF];
/// How many units of a shield a meter shows, if its tag says none.
const SHIELD_VALUE_SCALE: i16 = 255;
/// How many of health's.
const HEALTH_VALUE_SCALE: i16 = 8;
/// The shield flashes below this.
const SHIELD_FLASH: f32 = 0.25;
/// How long a flash takes, and how long between flashes, in seconds (the
/// imported colours have no flash period of their own).
const FLASH_LENGTH: f32 = 0.5;
const FLASH_PERIOD: f32 = 1.0;
/// A bitmap group's flag: its digits are drawn at half size.
const HALF_HUD_SCALE: u16 = 1 << 4;
/// A placement's flag: drawn at half size, as Halo PC's high-resolution
/// digits are.
const HIGH_RESOLUTION_SCALE: u16 = 1 << 2;
/// A number's flag: its leading zeros are drawn.
const SHOW_LEADING_ZEROS: u8 = 1 << 0;
/// A weapon number's flag: counted in magazines.
const DIVIDE_BY_MAGAZINE: u16 = 1 << 0;
/// The kind of map elements are drawn on: any map, a single player's
/// view (`map_type_flags` of one player, not on the main menu).
const MAP_TYPE_FLAGS: i16 = 0b011;
/// The deepest chain of parent weapon HUDs drawn.
const MAXIMUM_PARENTS: usize = 4;

/// The HUD of `state`, drawn on a window `width` by `height` pixels at
/// `seconds` into the game.
pub fn hud_quads(
    level: &Level,
    state: &HudState,
    width: u32,
    height: u32,
    seconds: f32,
) -> Vec<HudQuad> {
    let Some(mut painter) = Painter::new(level, width, height, seconds) else {
        return Vec::new();
    };
    painter.unit(state);
    if let Some(weapon) = &state.weapon {
        painter.weapon(weapon);
    }
    painter.grenades(state);
    painter.quads
}

/// The HUD being drawn: its quads so far, in the window's pixels.
pub(crate) struct Painter<'a> {
    level: &'a Level,
    /// The window's pixels to the HUD's.
    pub(crate) scale: f32,
    /// The HUD screen's width, in its pixels.
    width: f32,
    seconds: f32,
    pub(crate) quads: Vec<HudQuad>,
}

impl<'a> Painter<'a> {
    /// The HUD of `level` on a window `width` by `height` pixels at
    /// `seconds` into the game: none on an empty window.
    pub(crate) fn new(level: &'a Level, width: u32, height: u32, seconds: f32) -> Option<Self> {
        if width == 0 || height == 0 {
            return None;
        }
        let scale = height as f32 / HUD_HEIGHT;
        Some(Self {
            level,
            scale,
            width: width as f32 / scale,
            seconds,
            quads: Vec::new(),
        })
    }
}

impl Painter<'_> {
    /// Where an element is, in the HUD's pixels (`hud_calculate_point`).
    pub(crate) fn point(&self, anchor: HudAnchor, placement: &HudPlacement) -> [f32; 2] {
        let [x, y] = placement.offset.map(f32::from);
        match anchor {
            HudAnchor::TopLeft => [x, y],
            HudAnchor::TopRight => [self.width - x, y],
            HudAnchor::BottomLeft => [x, HUD_HEIGHT - y],
            HudAnchor::BottomRight => [self.width - x, HUD_HEIGHT - y],
            HudAnchor::Center => [self.width * 0.5 + x, HUD_HEIGHT * 0.5 + y],
        }
    }

    /// A colour when flashing (`get_flash_color`): towards `flash` for
    /// the first part of each period.
    fn flash(&self, color: u32, flash: u32) -> u32 {
        let phase = self.seconds.rem_euclid(FLASH_PERIOD);
        if phase >= FLASH_LENGTH {
            return color;
        }
        let wave = (libm::cosf(phase / FLASH_LENGTH * std::f32::consts::TAU) + 1.0) * 0.5;
        let fraction = libm::sqrtf((1.0 - wave).clamp(0.0, 1.0));
        lerp_color(color, flash, fraction)
    }

    /// A static's colour as `flags` draw it.
    fn color(&self, color: u32, flash: u32, flags: DrawFlags) -> u32 {
        if flags.disabled {
            // (the imported tags keep no disabled colour: half as opaque)
            let alpha = (opacity(color) >> 1) << 24;
            alpha | (color & 0x00FF_FFFF)
        } else if flags.flashing {
            self.flash(color, flash)
        } else {
            color
        }
    }

    /// Draws frame `frame` of `sequence` of `tag` at `point` (in the HUD's
    /// pixels), kept to `anchor`'s side of it (`hud_calculate_bitmap_bounds`).
    #[allow(clippy::too_many_arguments)]
    fn bitmap(
        &mut self,
        tag: &TagRef,
        anchor: HudAnchor,
        point: [f32; 2],
        sequence: i16,
        frame: usize,
        scale: [f32; 2],
        color: u32,
        meter: Option<HudMeterFill>,
    ) {
        let Some(bitmap) = self.level.bitmap(tag) else {
            return;
        };
        let Some(Sprite { image, clip }) = sprite(bitmap, sequence, frame) else {
            return;
        };
        let Some(size) = bitmap.images.get(usize::from(image)) else {
            return;
        };
        let width = (clip[1] - clip[0]) * f32::from(size.width);
        let height = (clip[3] - clip[2]) * f32::from(size.height);
        let (x0, x1) = match anchor {
            HudAnchor::TopLeft | HudAnchor::BottomLeft => (0.0, width),
            HudAnchor::TopRight | HudAnchor::BottomRight => (-width, 0.0),
            HudAnchor::Center => (-width * 0.5, width * 0.5),
        };
        let (y0, y1) = match anchor {
            HudAnchor::TopLeft | HudAnchor::TopRight => (0.0, height),
            HudAnchor::BottomLeft | HudAnchor::BottomRight => (-height, 0.0),
            HudAnchor::Center => (-height * 0.5, height * 0.5),
        };
        let screen = |at: f32, bound: f32, scale: f32| (at + bound * scale) * self.scale;
        self.quads.push(HudQuad {
            bitmap: tag.path.clone(),
            image: u32::from(image),
            left: screen(point[0], x0, scale[0]),
            top: screen(point[1], y0, scale[1]),
            right: screen(point[0], x1, scale[0]),
            bottom: screen(point[1], y1, scale[1]),
            u0: clip[0],
            v0: clip[2],
            u1: clip[1],
            v1: clip[3],
            color,
            meter,
        });
    }

    /// A state message's picture `icon` of `icons`, its bottom left at
    /// `cursor` (`render_state_bitmap`): how far it moves the cursor on.
    pub(crate) fn icon(
        &mut self,
        icons: &TagRef,
        icon: &HudIcon,
        cursor: [f32; 2],
        color: u32,
    ) -> f32 {
        let point = [
            cursor[0] + f32::from(icon.offset[0]),
            cursor[1] - f32::from(icon.offset[1]),
        ];
        let frame = match icon.frame_rate {
            rate if rate > 0 => {
                libm::truncf(self.seconds * TICKS_PER_SECOND / f32::from(rate)) as usize
            }
            _ => 0,
        };
        let color = if icon.flags & HudIcon::USE_COLOR != 0 {
            icon.color
        } else {
            color
        };
        let drawn = self.quads.len();
        self.bitmap(
            icons,
            HudAnchor::BottomLeft,
            point,
            icon.sequence,
            frame,
            [1.0, 1.0],
            color,
            None,
        );
        let width = match self.quads.get(drawn) {
            _ if icon.flags & HudIcon::ABSOLUTE_WIDTH != 0 => 0.0,
            Some(quad) => (quad.right - quad.left) / self.scale,
            None => return 0.0,
        };
        point[0] + width + f32::from(icon.width_offset) - cursor[0]
    }

    /// A static element (`hud_draw_static_element`).
    fn draw_static(&mut self, anchor: HudAnchor, element: &HudStatic, flags: DrawFlags) {
        let Some(tag) = &element.bitmap else {
            return;
        };
        let point = self.point(anchor, &element.placement);
        let color = self.color(element.color, element.flash_color, flags);
        self.bitmap(
            tag,
            anchor,
            point,
            element.sequence,
            0,
            element.placement.scale,
            color,
            None,
        );
    }

    /// A meter showing `value` (`hud_draw_meter`, its colours simplified to
    /// the filled part's and the empty part's).
    fn draw_meter(&mut self, anchor: HudAnchor, meter: &HudMeter, value: u8, flags: DrawFlags) {
        let Some(tag) = &meter.bitmap else {
            return;
        };
        let alpha = (f32::from(meter.alpha_multiplier) * f32::from(value)
            + f32::from(meter.alpha_bias))
        .clamp(0.0, 255.0);
        let threshold = alpha.max(f32::from(meter.minimum_value)) / 255.0;
        let (min_color, max_color) = if flags.disabled {
            (0, 0)
        } else if flags.flashing && meter.flags & SWITCH_COLOR_ON_STATE_CHANGE != 0 {
            (meter.max_color, meter.max_color)
        } else if meter.flags & SWITCH_COLOR_ON_STATE_CHANGE != 0 {
            (meter.min_color, meter.min_color)
        } else {
            (meter.min_color, meter.max_color)
        };
        // (the tags' empty alpha is how clear it is; drawn opaque, the
        // meter's own opacity is how clear the whole of it is)
        let opaque = |color: u32| color | 0xFF00_0000;
        let empty = ((255 - (meter.empty_color >> 24)) << 24) | (meter.empty_color & 0x00FF_FFFF);
        let tint = libm::roundf((1.0 - meter.opacity).clamp(0.0, 1.0) * 255.0) as u32;
        let point = self.point(anchor, &meter.placement);
        self.bitmap(
            tag,
            anchor,
            point,
            meter.sequence,
            0,
            meter.placement.scale,
            (tint << 24) | 0x00FF_FFFF,
            Some(HudMeterFill {
                threshold,
                min_color: opaque(min_color),
                max_color: opaque(max_color),
                empty_color: empty,
            }),
        );
    }

    /// A number (`hud_draw_numbers`): its digits from the least, leftward
    /// from where its corner puts the last.
    fn draw_number(&mut self, anchor: HudAnchor, number: &HudNumber, value: i16, flags: DrawFlags) {
        let Some(digits) = &self.level.hud_digits else {
            return;
        };
        let Some(tag) = digits.bitmap.clone() else {
            return;
        };
        let Some(bitmap) = self.level.bitmap(&tag) else {
            return;
        };
        let half = number.placement.flags & HIGH_RESOLUTION_SCALE != 0
            || bitmap.flags & HALF_HUD_SCALE != 0;
        let digit_scale = if half { 0.5 } else { 1.0 };
        let count = f32::from(number.digits);
        let spacing = f32::from(digits.screen_width);
        let origin = self.point(anchor, &number.placement);
        let mut cursor = match anchor {
            HudAnchor::TopLeft | HudAnchor::BottomLeft => {
                libm::truncf((count - 2.0) * spacing + origin[0])
            }
            HudAnchor::Center => libm::truncf((count - 1.0) * spacing * 0.5 + origin[0]),
            HudAnchor::TopRight | HudAnchor::BottomRight => origin[0],
        };
        let color = self.color(number.color, number.color, flags);
        let mut value = value.unsigned_abs();
        for _ in 0..number.digits.max(0) {
            if value == 0 && number.flags & SHOW_LEADING_ZEROS == 0 {
                break;
            }
            let digit = usize::from(value % 10);
            self.bitmap(
                &tag,
                anchor,
                [cursor, origin[1]],
                0,
                digit,
                [digit_scale; 2],
                color,
                None,
            );
            cursor = libm::truncf(cursor - spacing);
            value /= 10;
        }
    }

    /// The unit's HUD (`hud_render_unit_interface`): its panel, shield and
    /// health, and the motion sensor in the bottom left.
    fn unit(&mut self, state: &HudState) {
        let level = self.level;
        let Some(hud) = level
            .object(&state.unit)
            .and_then(|unit| unit.unit_hud.as_ref())
            .and_then(|tag| level.unit_huds.iter().find(|hud| hud.tag == *tag))
        else {
            return;
        };
        let anchor = hud.anchor;
        self.draw_static(anchor, &hud.background, DrawFlags::new(false, state.dead));

        let shield_flags = DrawFlags::new(state.shield < SHIELD_FLASH, state.dead);
        let value_scale = match hud.shield_meter.value_scale {
            0 => SHIELD_VALUE_SCALE,
            scale => scale,
        };
        for (overcharge, color) in std::iter::once(None)
            .chain(OVERCHARGE_COLORS.iter().map(Some))
            .enumerate()
        {
            let shield = (state.shield - overcharge as f32).clamp(0.0, 1.0);
            if shield <= 0.0 {
                break;
            }
            let mut meter = hud.shield_meter.clone();
            if let Some(&color) = color {
                meter.min_color = color;
                meter.max_color = color;
            }
            self.draw_meter(
                anchor,
                &meter,
                meter_value(value_scale, shield),
                shield_flags,
            );
        }
        self.draw_static(anchor, &hud.shield_background, shield_flags);

        let health_flags = DrawFlags::new(state.shield_depleted, state.dead);
        let value_scale = match hud.health_meter.value_scale {
            0 => HEALTH_VALUE_SCALE,
            scale => scale,
        };
        let mut meter = hud.health_meter.clone();
        if state.body >= hud.health_max_cutoff {
            meter.min_color = meter.max_color;
        } else if state.body <= hud.health_min_cutoff {
            meter.max_color = meter.min_color;
        } else {
            meter.min_color = hud.health_mid_color;
            meter.max_color = hud.health_mid_color;
        }
        self.draw_meter(
            anchor,
            &meter,
            meter_value(value_scale, state.body),
            health_flags,
        );
        self.draw_static(anchor, &hud.health_background, health_flags);

        // (the motion sensor keeps to the bottom left, whatever the HUD's)
        let sensor = HudAnchor::BottomLeft;
        let plain = DrawFlags::default();
        self.draw_static(sensor, &hud.motion_sensor_background, plain);
        self.draw_static(sensor, &hud.motion_sensor_foreground, plain);
        let Some(blip) = level.globals.motion_sensor_blip.clone() else {
            return;
        };
        let centre = self.point(sensor, &hud.blip_placement);
        for placed in &state.blips {
            let [red, green, blue] = placed
                .color
                .map(|channel| libm::roundf(channel.clamp(0.0, 1.0) * 255.0) as u32);
            let at = [centre[0] + placed.offset[0], centre[1] + placed.offset[1]];
            let radius = placed.radius;
            self.quads.push(HudQuad {
                bitmap: blip.path.clone(),
                image: 0,
                left: (at[0] - radius) * self.scale,
                top: (at[1] - radius) * self.scale,
                right: (at[0] + radius) * self.scale,
                bottom: (at[1] + radius) * self.scale,
                u0: 0.0,
                v0: 0.0,
                u1: 1.0,
                v1: 1.0,
                color: 0xFF00_0000 | red << 16 | green << 8 | blue,
                meter: None,
            });
        }
    }

    /// The weapon's HUD (`weapon_hud_render`): its parent's first, then
    /// its statics, meters, numbers and crosshairs.
    fn weapon(&mut self, weapon: &WeaponReadout) {
        let level = self.level;
        let definition = level
            .object(&weapon.weapon)
            .and_then(|object| object.weapon.as_ref());
        let tag = definition
            .and_then(|weapon| weapon.hud_interface.as_ref())
            .or_else(|| {
                level
                    .hud_globals
                    .as_ref()
                    .and_then(|globals| globals.default_weapon_hud.as_ref())
            });
        let mut chain = Vec::new();
        let mut next = tag;
        while let Some(tag) = next {
            if chain.len() == MAXIMUM_PARENTS {
                break;
            }
            let Some(hud) = level.weapon_huds.iter().find(|hud| hud.tag == *tag) else {
                break;
            };
            chain.push(hud);
            next = hud.parent.as_ref();
        }
        let magazine_size = definition
            .and_then(|weapon| weapon.magazines.first())
            .map_or(0, |magazine| magazine.rounds_loaded_maximum);
        for hud in chain.iter().rev() {
            self.weapon_hud(hud, weapon, magazine_size);
        }
    }

    fn weapon_hud(&mut self, hud: &WeaponHud, weapon: &WeaponReadout, magazine_size: i16) {
        let anchor = hud.anchor;
        for element in &hud.statics {
            if shown(&element.element) {
                let flags = weapon_flags(weapon, element.element.state);
                self.draw_static(anchor, &element.draw, flags);
            }
        }
        for element in &hud.meters {
            if shown(&element.element) {
                let flags = weapon_flags(weapon, element.element.state);
                let value = weapon_value(weapon, element.element.state).unwrap_or(0);
                let value = u8::try_from(value.clamp(0, 255)).unwrap_or(u8::MAX);
                self.draw_meter(anchor, &element.draw, value, flags);
            }
        }
        for element in &hud.numbers {
            if !shown(&element.element) {
                continue;
            }
            let Some(mut value) = weapon_value(weapon, element.element.state) else {
                continue;
            };
            if element.weapon_flags & DIVIDE_BY_MAGAZINE != 0 && magazine_size > 0 {
                value /= magazine_size;
            }
            let flags = weapon_flags(weapon, element.element.state);
            self.draw_number(anchor, &element.draw, value, flags);
        }
        let reloading = weapon.magazines.iter().any(|magazine| magazine.reloading);
        for crosshair in &hud.crosshairs {
            let drawn = match crosshair.kind {
                CrosshairKind::Aim => true,
                CrosshairKind::Reload => reloading,
                _ => false,
            };
            let Some(tag) = &crosshair.bitmap else {
                continue;
            };
            if !drawn || !on_map(crosshair.use_on_map) {
                continue;
            }
            for item in &crosshair.items {
                let point = self.point(HudAnchor::Center, &item.placement);
                self.bitmap(
                    tag,
                    HudAnchor::Center,
                    point,
                    item.sequence,
                    0,
                    item.placement.scale,
                    item.color,
                    None,
                );
            }
        }
    }

    /// The grenades' HUD (`render_grenade_hud`): the kind thrown and how
    /// many of it are carried.
    fn grenades(&mut self, state: &HudState) {
        let level = self.level;
        let Some(hud) = level
            .globals
            .grenades
            .get(state.grenade)
            .and_then(|grenade| grenade.hud_interface.as_ref())
            .and_then(|tag| level.grenade_huds.iter().find(|hud| hud.tag == *tag))
        else {
            return;
        };
        let count = state.grenades.get(state.grenade).copied().unwrap_or(0);
        let flags = DrawFlags::new(false, count <= 0);
        self.draw_static(hud.anchor, &hud.background, flags);
        self.draw_static(hud.anchor, &hud.count_background, flags);
        if hud.count.digits > 0 {
            self.draw_number(hud.anchor, &hud.count, count, flags);
        }
    }
}

/// A meter's flag: its colour switches with its state.
const SWITCH_COLOR_ON_STATE_CHANGE: u8 = 1 << 3;

/// A frame of a sequence: its image and its part of it (`[left, right,
/// top, bottom]`, 0 to 1).
struct Sprite {
    image: u16,
    clip: [f32; 4],
}

/// Frame `frame` of `sequence` (`bitmap_group_get_bitmap_from_sequence`
/// and `get_sprite_clip_rect`): a sprite's part of its image, or a whole
/// image of the sequence; the first image for no sequence.
fn sprite(bitmap: &Bitmap, sequence: i16, frame: usize) -> Option<Sprite> {
    let whole = [0.0, 1.0, 0.0, 1.0];
    let Some(sequence) = usize::try_from(sequence)
        .ok()
        .and_then(|index| bitmap.sequences.get(index))
    else {
        return (!bitmap.images.is_empty()).then_some(Sprite {
            image: 0,
            clip: whole,
        });
    };
    if !sequence.sprites.is_empty() {
        let sprite = sequence.sprites.get(frame % sequence.sprites.len())?;
        return Some(Sprite {
            image: sprite.image,
            clip: sprite.bounds,
        });
    }
    let first = sequence.first_image?;
    let count = usize::from(sequence.image_count.max(1));
    let image = u16::try_from(usize::from(first) + frame % count).ok()?;
    Some(Sprite { image, clip: whole })
}

/// What a meter showing `vitality` in `value_scale` units is given.
fn meter_value(value_scale: i16, vitality: f32) -> u8 {
    libm::truncf((f32::from(value_scale) * vitality).clamp(0.0, 255.0)) as u8
}

/// Whether an element is drawn on this kind of map.
fn on_map(use_on_map: i16) -> bool {
    !(0..16).contains(&use_on_map) || MAP_TYPE_FLAGS & (1 << use_on_map) != 0
}

fn shown(element: &WeaponHudElement) -> bool {
    on_map(element.use_on_map)
}

/// How an element showing `state` is drawn: disabled when what it counts
/// is gone.
fn weapon_flags(weapon: &WeaponReadout, state: WeaponHudState) -> DrawFlags {
    let magazine = |index: usize| weapon.magazines.get(index).copied().unwrap_or_default();
    let disabled = match state {
        WeaponHudState::TotalAmmunition => {
            let magazine = magazine(0);
            magazine.loaded + magazine.total <= 0 && !weapon.magazines.is_empty()
        }
        WeaponHudState::SecondaryTotalAmmunition => {
            let magazine = magazine(1);
            magazine.loaded + magazine.total <= 0 && weapon.magazines.len() > 1
        }
        _ => false,
    };
    let flashing = state == WeaponHudState::Heat && weapon.overheated;
    DrawFlags::new(flashing, disabled)
}

/// What an element showing `state` shows (`weapon_hud_number_values`);
/// `None` for the distances, which are not drawn.
fn weapon_value(weapon: &WeaponReadout, state: WeaponHudState) -> Option<i16> {
    let magazine = |index: usize| weapon.magazines.get(index).copied().unwrap_or_default();
    let fraction = |value: f32, of: f32| libm::truncf((value * of).clamp(0.0, of)) as i16;
    Some(match state {
        WeaponHudState::TotalAmmunition => magazine(0).total,
        WeaponHudState::LoadedAmmunition => magazine(0).loaded,
        WeaponHudState::Heat => fraction(weapon.heat, 255.0),
        WeaponHudState::Age => fraction(1.0 - weapon.age, 100.0),
        WeaponHudState::SecondaryTotalAmmunition => magazine(1).total,
        WeaponHudState::SecondaryLoadedAmmunition => magazine(1).loaded,
        WeaponHudState::DistanceToTarget | WeaponHudState::ElevationToTarget => return None,
    })
}

/// A colour's alpha.
fn opacity(color: u32) -> u32 {
    color >> 24
}

/// The colour `fraction` of the way from `from` to `to`.
fn lerp_color(from: u32, to: u32, fraction: f32) -> u32 {
    let channel = |shift: u32| {
        let a = ((from >> shift) & 0xFF) as f32;
        let b = ((to >> shift) & 0xFF) as f32;
        (libm::roundf(a + (b - a) * fraction) as u32) << shift
    };
    channel(24) | channel(16) | channel(8) | channel(0)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn colours_lerp_by_channel() {
        assert_eq!(lerp_color(0xFF00_0000, 0xFFFF_FFFF, 0.5), 0xFF80_8080);
        assert_eq!(lerp_color(0x1020_3040, 0x5060_7080, 0.0), 0x1020_3040);
    }

    #[test]
    fn elements_are_drawn_on_this_kind_of_map() {
        assert!(on_map(0));
        assert!(on_map(1));
        assert!(!on_map(2));
    }
}
