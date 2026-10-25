//! The heads-up display: the unit's shield, health and motion sensor
//! (`unhi`), the weapon's ammunition, heat and crosshairs (`wphi`), the
//! grenades (`grhi`), the digits numbers are drawn with (`hud#`) and the
//! HUD's globals (`hudg`).
//!
//! Every element is placed as the game places it: an offset from one of
//! the screen's corners (or its centre), in the pixels of a 640 by 480
//! screen, and a bitmap's sprite drawn from there. Colours are the tags'
//! (`0xAARRGGBB`).

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// The corner of the screen a HUD keeps to.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub enum HudAnchor {
    /// The top left.
    #[default]
    TopLeft,
    /// The top right.
    TopRight,
    /// The bottom left.
    BottomLeft,
    /// The bottom right.
    BottomRight,
    /// The centre.
    Center,
}

impl HudAnchor {
    /// The tag's corner (0 to 4), or the top left for any other.
    pub fn from_index(index: i16) -> Self {
        match index {
            1 => Self::TopRight,
            2 => Self::BottomLeft,
            3 => Self::BottomRight,
            4 => Self::Center,
            _ => Self::TopLeft,
        }
    }
}

/// Where an element is, from its HUD's corner.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudPlacement {
    /// Its offset, in pixels away from the corner (right and down from the
    /// top left; left from a right corner and up from a bottom one).
    pub offset: [i16; 2],
    /// How much its bitmap is scaled, across and down.
    pub scale: [f32; 2],
    /// Its multiplayer scaling flags.
    pub flags: u16,
}

/// A bitmap drawn in one colour.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudStatic {
    /// Where it is.
    pub placement: HudPlacement,
    /// Its bitmap.
    pub bitmap: Option<TagRef>,
    /// Its colour.
    pub color: u32,
    /// Its colour when it flashes.
    pub flash_color: u32,
    /// The sequence of the bitmap it draws (its first sprite).
    pub sequence: i16,
}

/// A bitmap drawn as a meter: each texel's alpha is the value at which it
/// fills, so a meter fills along whatever its bitmap's gradient is.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudMeter {
    /// Where it is.
    pub placement: HudPlacement,
    /// Its bitmap.
    pub bitmap: Option<TagRef>,
    /// The colour of its filled part, when the value is least.
    pub min_color: u32,
    /// The colour of its filled part, when the value is most.
    pub max_color: u32,
    /// The colour it flashes.
    pub flash_color: u32,
    /// The colour of its empty part.
    pub empty_color: u32,
    /// Its flags (bit 0: interpolates its colour between the two; bit 1:
    /// inverts that; bit 3: switches colour when its state changes).
    pub flags: u8,
    /// The least alpha it draws its filled part with.
    pub minimum_value: u8,
    /// The sequence of the bitmap it draws.
    pub sequence: i16,
    /// What the value is multiplied by, to be a texel's alpha.
    pub alpha_multiplier: u8,
    /// What is added to that.
    pub alpha_bias: u8,
    /// How many of the value's units the meter shows (0 for all of them).
    pub value_scale: i16,
    /// How opaque it is.
    pub opacity: f32,
    /// How faded it is.
    pub fade: f32,
}

/// A number, drawn with the HUD's digits.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudNumber {
    /// Where it is.
    pub placement: HudPlacement,
    /// Its colour.
    pub color: u32,
    /// How many digits it has room for.
    pub digits: i8,
    /// Its flags (bit 0: shows its leading zeros; bit 1: only when the view
    /// is zoomed; bit 2: a trailing m).
    pub flags: u8,
    /// How many digits after the point.
    pub fractional_digits: i8,
}

/// The unit's HUD: its shield and health, and its motion sensor (which is
/// always in the bottom left corner, whatever the HUD's anchor).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct UnitHud {
    /// Its tag.
    pub tag: TagRef,
    /// The corner it keeps to.
    pub anchor: HudAnchor,
    /// The panel behind it all.
    pub background: HudStatic,
    /// The shield meter's background.
    pub shield_background: HudStatic,
    /// The shield meter.
    pub shield_meter: HudMeter,
    /// The health meter's background.
    pub health_background: HudStatic,
    /// The health meter.
    pub health_meter: HudMeter,
    /// The health meter's colour between its cutoffs.
    pub health_mid_color: u32,
    /// The health at and above which the meter is its maximum colour.
    pub health_max_cutoff: f32,
    /// The health at and below which the meter is its minimum colour.
    pub health_min_cutoff: f32,
    /// The motion sensor's background.
    pub motion_sensor_background: HudStatic,
    /// What is drawn over the motion sensor.
    pub motion_sensor_foreground: HudStatic,
    /// Where the motion sensor's centre is, to draw its blips about.
    pub blip_placement: HudPlacement,
}

/// What a weapon HUD element shows or is drawn for.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub enum WeaponHudState {
    /// The rounds left, in the magazine and out of it.
    #[default]
    TotalAmmunition,
    /// The rounds in the magazine.
    LoadedAmmunition,
    /// The weapon's heat.
    Heat,
    /// The weapon's age (a plasma weapon's battery).
    Age,
    /// The second magazine's rounds left.
    SecondaryTotalAmmunition,
    /// The second magazine's loaded rounds.
    SecondaryLoadedAmmunition,
    /// How far away the target is.
    DistanceToTarget,
    /// How far above the view the target is.
    ElevationToTarget,
}

impl WeaponHudState {
    /// The tag's state (0 to 7), or the rounds left for any other.
    pub fn from_index(index: i16) -> Self {
        match index {
            1 => Self::LoadedAmmunition,
            2 => Self::Heat,
            3 => Self::Age,
            4 => Self::SecondaryTotalAmmunition,
            5 => Self::SecondaryLoadedAmmunition,
            6 => Self::DistanceToTarget,
            7 => Self::ElevationToTarget,
            _ => Self::TotalAmmunition,
        }
    }
}

/// When a weapon HUD element is drawn, and what it shows.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponHudElement {
    /// What it shows.
    pub state: WeaponHudState,
    /// The kind of map it is drawn on (bit `use_on_map` of: any map, a
    /// single player's view, a split screen).
    pub use_on_map: i16,
}

/// A weapon HUD's static element.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponHudStatic {
    /// When it is drawn.
    pub element: WeaponHudElement,
    /// What it draws.
    pub draw: HudStatic,
}

/// A weapon HUD's meter.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponHudMeter {
    /// When it is drawn, and what it shows.
    pub element: WeaponHudElement,
    /// What it draws.
    pub draw: HudMeter,
}

/// A weapon HUD's number.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponHudNumber {
    /// When it is drawn, and what it shows.
    pub element: WeaponHudElement,
    /// What it draws.
    pub draw: HudNumber,
    /// Its weapon flags (bit 0: shows the value in magazines, divided by
    /// the magazine's most rounds).
    pub weapon_flags: u16,
}

/// What a crosshair is for (the game's crosshair states, in its order).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub enum CrosshairKind {
    /// Where the weapon aims.
    #[default]
    Aim,
    /// The zoomed view's.
    Zoom,
    /// How charged the weapon is.
    Charge,
    /// Flashed when the magazine is low.
    FlashAmmunition,
    /// Flashed when the weapon is hot.
    FlashHeat,
    /// Flashed when the rounds left are low.
    FlashTotalAmmunition,
    /// Flashed when the battery is low.
    FlashTotalBattery,
    /// Shown while the weapon reloads.
    Reload,
    /// Flashed when it is fired empty.
    FiredWithNoAmmunition,
    /// Flashed when a grenade is thrown and none are left.
    ThrewWithNoGrenade,
    /// Flashed when the magazine is low and none are left to load.
    FlashAmmunitionNoneForReload,
    /// The second magazine's low flash.
    FlashSecondaryAmmunition,
    /// The second magazine's rounds-left flash.
    FlashSecondaryTotalAmmunition,
    /// Shown while the second magazine reloads.
    SecondaryReload,
    /// Flashed when the second trigger is fired empty.
    FiredSecondaryWithNoAmmunition,
    /// The second magazine's none-to-load flash.
    FlashSecondaryAmmunitionNoneForReload,
    /// The first trigger is ready.
    PrimaryTriggerReady,
    /// The second trigger is ready.
    SecondaryTriggerReady,
    /// Flashed when it is fired with a flat battery.
    FlashFiredBatteryDepleted,
}

impl CrosshairKind {
    /// The tag's type (0 to 18), or the aim for any other.
    pub fn from_index(index: i16) -> Self {
        const KINDS: [CrosshairKind; 19] = [
            CrosshairKind::Aim,
            CrosshairKind::Zoom,
            CrosshairKind::Charge,
            CrosshairKind::FlashAmmunition,
            CrosshairKind::FlashHeat,
            CrosshairKind::FlashTotalAmmunition,
            CrosshairKind::FlashTotalBattery,
            CrosshairKind::Reload,
            CrosshairKind::FiredWithNoAmmunition,
            CrosshairKind::ThrewWithNoGrenade,
            CrosshairKind::FlashAmmunitionNoneForReload,
            CrosshairKind::FlashSecondaryAmmunition,
            CrosshairKind::FlashSecondaryTotalAmmunition,
            CrosshairKind::SecondaryReload,
            CrosshairKind::FiredSecondaryWithNoAmmunition,
            CrosshairKind::FlashSecondaryAmmunitionNoneForReload,
            CrosshairKind::PrimaryTriggerReady,
            CrosshairKind::SecondaryTriggerReady,
            CrosshairKind::FlashFiredBatteryDepleted,
        ];
        usize::try_from(index)
            .ok()
            .and_then(|index| KINDS.get(index).copied())
            .unwrap_or_default()
    }
}

/// One of a crosshair's pictures.
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct CrosshairItem {
    /// Where it is.
    pub placement: HudPlacement,
    /// Its colour.
    pub color: u32,
    /// Its colour when it flashes.
    pub flash_color: u32,
    /// How many of its frames a second it plays (0 for one frame).
    pub frame_rate: i16,
    /// The sequence of the bitmap it draws.
    pub sequence: i16,
    /// Its flags (bit 1: its sequence is whole images, not sprites).
    pub flags: u32,
}

/// A weapon HUD's crosshair.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponHudCrosshair {
    /// What it is for.
    pub kind: CrosshairKind,
    /// The kind of map it is drawn on.
    pub use_on_map: i16,
    /// Its bitmap.
    pub bitmap: Option<TagRef>,
    /// Its pictures.
    pub items: Vec<CrosshairItem>,
}

/// A weapon's HUD.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct WeaponHud {
    /// Its tag.
    pub tag: TagRef,
    /// The HUD drawn before it, whose elements it adds to.
    pub parent: Option<TagRef>,
    /// The corner it keeps to.
    pub anchor: HudAnchor,
    /// Its static elements.
    pub statics: Vec<WeaponHudStatic>,
    /// Its meters.
    pub meters: Vec<WeaponHudMeter>,
    /// Its numbers.
    pub numbers: Vec<WeaponHudNumber>,
    /// Its crosshairs.
    pub crosshairs: Vec<WeaponHudCrosshair>,
    /// Its picture in the HUD's messages ("swap for the ..."); none when
    /// its weapon's name is written instead.
    pub messaging_icon: Option<HudIcon>,
}

/// A grenade's HUD: its picture and how many the player has.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct GrenadeHud {
    /// Its tag.
    pub tag: TagRef,
    /// The corner it keeps to.
    pub anchor: HudAnchor,
    /// The panel behind it.
    pub background: HudStatic,
    /// The panel behind the count.
    pub count_background: HudStatic,
    /// The count.
    pub count: HudNumber,
}

/// The digits numbers are drawn with (`hud#`): sprites of its bitmap's
/// first sequence, 0 to 9, then the point, the colon, the minus sign, the
/// m and the km.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudDigits {
    /// Its tag.
    pub tag: TagRef,
    /// Its bitmap.
    pub bitmap: Option<TagRef>,
    /// A digit's width in its bitmap.
    pub character_width: i8,
    /// How far apart digits are drawn.
    pub screen_width: i8,
    /// Where a digit is drawn from, across.
    pub x_offset: i8,
    /// Where a digit is drawn from, down.
    pub y_offset: i8,
    /// How wide the point is.
    pub decimal_point_width: i8,
    /// How wide the colon is.
    pub colon_width: i8,
}

/// The HUD's globals (`hudg`) that the engine reads.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudGlobals {
    /// Its tag.
    pub tag: TagRef,
    /// The HUD a weapon without one draws.
    pub default_weapon_hud: Option<TagRef>,
    /// How far the motion sensor sees, in world units.
    pub motion_sensor_range: f32,
    /// How fast a unit must move, squared, to show on it.
    pub motion_sensor_velocity_sensitivity: f32,
    /// How large its blips' field is: the distance of the edge of the
    /// sensor, in its bitmap's pixels.
    pub motion_sensor_scale: f32,
    /// How large the HUD is drawn in multiplayer.
    pub multiplayer_scale: f32,
    /// The state messages ("hold E to enter the Warthog").
    pub messaging: HudMessaging,
}

/// A picture in the HUD's messages: a sequence of the messaging icons'
/// bitmap, or a line of the alternate icon text.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudIcon {
    /// Its sequence of the messaging icons (none: -1).
    pub sequence: i16,
    /// How much wider than its picture it is, in HUD pixels.
    pub width_offset: i16,
    /// Where it is drawn from the text's cursor, in HUD pixels.
    pub offset: [i16; 2],
    /// Its colour, 0xAARRGGBB.
    pub color: u32,
    /// How fast its frames turn.
    pub frame_rate: i8,
    /// Its flags ([`HudIcon::USE_TEXT`] and the rest).
    pub flags: u8,
    /// Its line of the alternate icon text, for [`HudIcon::USE_TEXT`].
    pub text_index: i16,
}

impl HudIcon {
    /// It is written as its line of the alternate icon text.
    pub const USE_TEXT: u8 = 1 << 0;
    /// It is drawn in its own colour, not the message's.
    pub const USE_COLOR: u8 = 1 << 1;
    /// Its width is its width offset alone.
    pub const ABSOLUTE_WIDTH: u8 = 1 << 2;
}

/// How the HUD's state messages are drawn, and what they say.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudMessaging {
    /// The corner they keep to.
    pub anchor: HudAnchor,
    /// Where they are drawn from it.
    pub placement: HudPlacement,
    /// Their font.
    pub font: Option<TagRef>,
    /// Their colour, 0xAARRGGBB.
    pub color: u32,
    /// The bitmap their pictures are drawn from.
    pub icons: Option<TagRef>,
    /// The names their pictures are written as (`ustr`).
    pub alternate_icon_text: Option<TagRef>,
    /// The buttons' pictures, by button (the original's 18).
    pub button_icons: Vec<HudIcon>,
    /// The messages (`hmt `), by the original's message index.
    pub messages: Vec<HudStateMessage>,
}

/// One of the HUD's state messages.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct HudStateMessage {
    /// Its name ("enter_vehicle").
    pub name: String,
    /// Its pieces, in order.
    pub elements: Vec<HudMessageElement>,
}

/// A piece of a state message.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub enum HudMessageElement {
    /// Text, as it is written.
    Text(String),
    /// A picture: a button (0 to 17), a control (18 to 31: action, throw
    /// grenade, fire, flashlight, jump, equipment, swap weapons, swap
    /// grenades, crouch, zoom, accept, back, move, look) or one of the
    /// message's own (32 to 39).
    Icon(u8),
}
