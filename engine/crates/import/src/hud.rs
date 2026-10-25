//! The HUD's tags: the unit's (`unhi`: its shield and health meters and
//! its motion sensor), each weapon's (`wphi`: its ammunition and heat,
//! and its crosshairs), the grenades' (`grhi`), the digits the counts are
//! drawn in (`hud#`) and the HUD's globals (`hudg`).
//!
//! The bitmaps they draw are reached, to import with the level's.

use std::collections::BTreeMap;

use opence_content::{
    CrosshairItem, CrosshairKind, Globals, GrenadeHud, HudAnchor, HudDigits, HudGlobals, HudIcon,
    HudMessageElement, HudMessaging, HudMeter, HudNumber, HudPlacement, HudStateMessage, HudStatic,
    ObjectDefinition, TagRef, UnitHud, WeaponHud, WeaponHudCrosshair, WeaponHudElement,
    WeaponHudMeter, WeaponHudNumber, WeaponHudStatic,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_index, tag_ref, Reached};
use crate::tags::{Element, GroupTag};

const UNHI: GroupTag = GroupTag::new(b"unhi");
const WPHI: GroupTag = GroupTag::new(b"wphi");
const GRHI: GroupTag = GroupTag::new(b"grhi");
const HUD_DIGITS: GroupTag = GroupTag::new(b"hud#");
const HUDG: GroupTag = GroupTag::new(b"hudg");
const HMT: GroupTag = GroupTag::new(b"hmt ");
const FONT: GroupTag = GroupTag::new(b"font");
const USTR: GroupTag = GroupTag::new(b"ustr");

const UNIT_HUD_SIZE: usize = 0x56C;
const WEAPON_HUD_SIZE: usize = 0x17C;
const GRENADE_HUD_SIZE: usize = 0x1F8;
const HUD_DIGITS_SIZE: usize = 0x64;
/// As much of the HUD globals as the engine reads (to the motion
/// sensor's scale).
const HUD_GLOBALS_SIZE: usize = 0x2DC;
const HUD_MESSAGE_TEXT_SIZE: usize = 0x80;
const HUD_ICON_SIZE: usize = 0x10;
const HUD_MESSAGE_ELEMENT_SIZE: usize = 2;
const HUD_MESSAGE_SIZE: usize = 0x40;
/// The most button icons, message pieces and messages there may be, and
/// the longest the messages' text may be, in bytes.
const MAX_BUTTON_ICONS: usize = 18;
const MAX_MESSAGE_ELEMENTS: usize = 8192;
const MAX_MESSAGES: usize = 1024;
const MAX_MESSAGE_TEXT: usize = 0x20000;

const WEAPON_STATIC_SIZE: usize = 0xB4;
const WEAPON_METER_SIZE: usize = 0xB4;
const WEAPON_NUMBER_SIZE: usize = 0xA0;
const CROSSHAIR_SIZE: usize = 0x68;
const CROSSHAIR_ITEM_SIZE: usize = 0x6C;
/// The most elements of each kind a weapon's HUD may have (the game's 16,
/// and its 19 crosshair kinds), and items a crosshair may have.
const MAX_WEAPON_ELEMENTS: usize = 16;
const MAX_CROSSHAIRS: usize = 19;
const MAX_CROSSHAIR_ITEMS: usize = 64;
/// How deep a weapon's HUD's parents may go before the chain is taken to
/// loop.
const MAX_PARENTS: usize = 8;

/// The HUD tags a level imports.
#[derive(Default)]
pub(crate) struct Huds {
    pub(crate) unit_huds: Vec<UnitHud>,
    pub(crate) weapon_huds: Vec<WeaponHud>,
    pub(crate) grenade_huds: Vec<GrenadeHud>,
    pub(crate) hud_digits: Option<HudDigits>,
    pub(crate) hud_globals: Option<HudGlobals>,
}

/// The HUD tags that `objects` and `globals` name: each unit's, each
/// weapon's (and each weapon HUD's parents, and the default), the
/// grenades', the digits and the HUD globals.
pub(crate) fn import_huds(
    cache: &Cache,
    objects: &[ObjectDefinition],
    globals: &Globals,
    reached: &mut Reached,
) -> Result<Huds, ImportError> {
    let mut huds = Huds::default();
    let present = |tag: &Option<TagRef>| {
        tag.as_ref()
            .and_then(|tag| tag_index(cache, tag).map(|index| (tag.clone(), index)))
    };

    if let Some((tag, index)) = present(&globals.hud_globals) {
        huds.hud_globals = Some(import_hud_globals(cache, index, tag, reached)?);
    }
    if let Some((tag, index)) = present(&globals.hud_digits) {
        huds.hud_digits = Some(import_hud_digits(cache, index, tag, reached)?);
    }

    let mut units = BTreeMap::new();
    for (tag, index) in objects
        .iter()
        .filter_map(|object| present(&object.unit_hud))
    {
        if !units.contains_key(&tag) {
            units.insert(tag.clone(), import_unit_hud(cache, index, tag, reached)?);
        }
    }
    huds.unit_huds = units.into_values().collect();

    let mut weapons = BTreeMap::new();
    let default = huds
        .hud_globals
        .as_ref()
        .and_then(|globals| globals.default_weapon_hud.clone());
    let mut queue: Vec<(Option<TagRef>, usize)> = objects
        .iter()
        .filter_map(|object| object.weapon.as_ref())
        .map(|weapon| (weapon.hud_interface.clone(), 0))
        .chain([(default, 0)])
        .collect();
    while let Some((tag, depth)) = queue.pop() {
        let Some((tag, index)) = present(&tag) else {
            continue;
        };
        if weapons.contains_key(&tag) || depth > MAX_PARENTS {
            continue;
        }
        let hud = import_weapon_hud(cache, index, tag.clone(), reached)?;
        queue.push((hud.parent.clone(), depth + 1));
        weapons.insert(tag, hud);
    }
    huds.weapon_huds = weapons.into_values().collect();

    let mut grenades = BTreeMap::new();
    for (tag, index) in globals
        .grenades
        .iter()
        .filter_map(|grenade| present(&grenade.hud_interface))
    {
        if !grenades.contains_key(&tag) {
            grenades.insert(tag.clone(), import_grenade_hud(cache, index, tag, reached)?);
        }
    }
    huds.grenade_huds = grenades.into_values().collect();
    Ok(huds)
}

fn placement(element: &Element<'_>, at: usize) -> Result<HudPlacement, ImportError> {
    Ok(HudPlacement {
        offset: [
            element.i16(at, "anchor offset")?,
            element.i16(at + 2, "anchor offset")?,
        ],
        scale: [
            element.f32(at + 4, "width scale")?,
            element.f32(at + 8, "height scale")?,
        ],
        flags: element.u16(at + 0xC, "scaling flags")?,
    })
}

fn hud_static(
    element: &Element<'_>,
    at: usize,
    reached: &mut Reached,
) -> Result<HudStatic, ImportError> {
    Ok(HudStatic {
        placement: placement(element, at)?,
        bitmap: reached.bitmap(element, at + 0x24, "interface bitmap")?,
        color: element.u32(at + 0x34, "default color")?,
        flash_color: element.u32(at + 0x38, "flashing color")?,
        sequence: element.i16(at + 0x54, "sequence index")?,
    })
}

fn hud_meter(
    element: &Element<'_>,
    at: usize,
    reached: &mut Reached,
) -> Result<HudMeter, ImportError> {
    Ok(HudMeter {
        placement: placement(element, at)?,
        bitmap: reached.bitmap(element, at + 0x24, "meter bitmap")?,
        min_color: element.u32(at + 0x34, "color at meter minimum")?,
        max_color: element.u32(at + 0x38, "color at meter maximum")?,
        flash_color: element.u32(at + 0x3C, "flash color")?,
        empty_color: element.u32(at + 0x40, "empty color")?,
        flags: element.u8(at + 0x44, "meter flags")?,
        minimum_value: element.u8(at + 0x45, "meter minimum value")?,
        sequence: element.i16(at + 0x46, "sequence index")?,
        alpha_multiplier: element.u8(at + 0x48, "alpha multiplier")?,
        alpha_bias: element.u8(at + 0x49, "alpha bias")?,
        value_scale: element.i16(at + 0x4A, "value scale")?,
        opacity: element.f32(at + 0x4C, "opacity")?,
        fade: element.f32(at + 0x50, "translucency")?,
    })
}

fn signed(element: &Element<'_>, at: usize, field: &str) -> Result<i8, ImportError> {
    Ok(i8::from_ne_bytes([element.u8(at, field)?]))
}

fn hud_number(element: &Element<'_>, at: usize) -> Result<HudNumber, ImportError> {
    Ok(HudNumber {
        placement: placement(element, at)?,
        color: element.u32(at + 0x24, "default color")?,
        digits: signed(element, at + 0x44, "maximum number of digits")?,
        flags: element.u8(at + 0x45, "flags")?,
        fractional_digits: signed(element, at + 0x46, "number of fractional digits")?,
    })
}

fn weapon_element(element: &Element<'_>) -> Result<WeaponHudElement, ImportError> {
    Ok(WeaponHudElement {
        state: opence_content::WeaponHudState::from_index(element.i16(0, "state attached to")?),
        use_on_map: element.i16(4, "can use on map type")?,
    })
}

fn import_unit_hud(
    cache: &Cache,
    index: usize,
    tag: TagRef,
    reached: &mut Reached,
) -> Result<UnitHud, ImportError> {
    let hud = cache.tag(index, UNHI, UNIT_HUD_SIZE)?;
    Ok(UnitHud {
        tag,
        anchor: HudAnchor::from_index(hud.i16(0, "anchor")?),
        background: hud_static(&hud, 0x24, reached)?,
        shield_background: hud_static(&hud, 0x8C, reached)?,
        shield_meter: hud_meter(&hud, 0x8C + 0x68, reached)?,
        health_background: hud_static(&hud, 0x17C, reached)?,
        health_meter: hud_meter(&hud, 0x17C + 0x68, reached)?,
        health_mid_color: hud.u32(0x17C + 0xD0, "medium health color")?,
        health_max_cutoff: hud.f32(0x17C + 0xD4, "max color health fraction cutoff")?,
        health_min_cutoff: hud.f32(0x17C + 0xD8, "min color health fraction cutoff")?,
        motion_sensor_background: hud_static(&hud, 0x26C, reached)?,
        motion_sensor_foreground: hud_static(&hud, 0x26C + 0x68, reached)?,
        blip_placement: placement(&hud, 0x35C)?,
    })
}

fn import_weapon_hud(
    cache: &Cache,
    index: usize,
    tag: TagRef,
    reached: &mut Reached,
) -> Result<WeaponHud, ImportError> {
    let hud = cache.tag(index, WPHI, WEAPON_HUD_SIZE)?;
    let parent = hud
        .reference(0, "child hud", &[WPHI])?
        .map(|index| tag_ref(cache, index));

    let mut statics = Vec::new();
    for element in hud
        .block(
            0x60,
            "static elements",
            WEAPON_STATIC_SIZE,
            MAX_WEAPON_ELEMENTS,
        )?
        .iter()
    {
        statics.push(WeaponHudStatic {
            element: weapon_element(&element)?,
            draw: hud_static(&element, 0x24, reached)?,
        });
    }
    let mut meters = Vec::new();
    for element in hud
        .block(
            0x6C,
            "meter elements",
            WEAPON_METER_SIZE,
            MAX_WEAPON_ELEMENTS,
        )?
        .iter()
    {
        meters.push(WeaponHudMeter {
            element: weapon_element(&element)?,
            draw: hud_meter(&element, 0x24, reached)?,
        });
    }
    let mut numbers = Vec::new();
    for element in hud
        .block(
            0x78,
            "number elements",
            WEAPON_NUMBER_SIZE,
            MAX_WEAPON_ELEMENTS,
        )?
        .iter()
    {
        numbers.push(WeaponHudNumber {
            element: weapon_element(&element)?,
            draw: hud_number(&element, 0x24)?,
            weapon_flags: element.u16(0x78, "weapon specific flags")?,
        });
    }
    let mut crosshairs = Vec::new();
    for element in hud
        .block(0x84, "crosshairs", CROSSHAIR_SIZE, MAX_CROSSHAIRS)?
        .iter()
    {
        let mut items = Vec::new();
        for item in element
            .block(
                0x34,
                "crosshair overlays",
                CROSSHAIR_ITEM_SIZE,
                MAX_CROSSHAIR_ITEMS,
            )?
            .iter()
        {
            items.push(CrosshairItem {
                placement: placement(&item, 0)?,
                color: item.u32(0x24, "default color")?,
                flash_color: item.u32(0x28, "flashing color")?,
                frame_rate: item.i16(0x44, "frame rate")?,
                sequence: item.i16(0x46, "sequence index")?,
                flags: item.u32(0x48, "flags")?,
            });
        }
        crosshairs.push(WeaponHudCrosshair {
            kind: CrosshairKind::from_index(element.i16(0, "crosshair type")?),
            use_on_map: element.i16(4, "can use on map type")?,
            bitmap: reached.bitmap(&element, 0x24, "crosshair bitmap")?,
            items,
        });
    }
    Ok(WeaponHud {
        tag,
        parent,
        anchor: HudAnchor::from_index(hud.i16(0x3C, "anchor")?),
        statics,
        meters,
        numbers,
        crosshairs,
        messaging_icon: Some(icon(&hud, 0x13C)?).filter(|icon| icon.sequence != -1),
    })
}

fn import_grenade_hud(
    cache: &Cache,
    index: usize,
    tag: TagRef,
    reached: &mut Reached,
) -> Result<GrenadeHud, ImportError> {
    let hud = cache.tag(index, GRHI, GRENADE_HUD_SIZE)?;
    Ok(GrenadeHud {
        tag,
        anchor: HudAnchor::from_index(hud.i16(0, "anchor")?),
        background: hud_static(&hud, 0x24, reached)?,
        count_background: hud_static(&hud, 0x8C, reached)?,
        count: hud_number(&hud, 0xF4)?,
    })
}

fn import_hud_digits(
    cache: &Cache,
    index: usize,
    tag: TagRef,
    reached: &mut Reached,
) -> Result<HudDigits, ImportError> {
    let digits = cache.tag(index, HUD_DIGITS, HUD_DIGITS_SIZE)?;
    Ok(HudDigits {
        tag,
        bitmap: reached.bitmap(&digits, 0, "digits bitmap")?,
        character_width: signed(&digits, 0x10, "bitmap digit width")?,
        screen_width: signed(&digits, 0x11, "screen digit width")?,
        x_offset: signed(&digits, 0x12, "x offset")?,
        y_offset: signed(&digits, 0x13, "y offset")?,
        decimal_point_width: signed(&digits, 0x14, "decimal point width")?,
        colon_width: signed(&digits, 0x15, "colon width")?,
    })
}

fn import_hud_globals(
    cache: &Cache,
    index: usize,
    tag: TagRef,
    reached: &mut Reached,
) -> Result<HudGlobals, ImportError> {
    let globals = cache.tag(index, HUDG, HUD_GLOBALS_SIZE)?;
    let reference = |at: usize, field: &str, group: GroupTag| -> Result<_, ImportError> {
        Ok(globals
            .reference(at, field, &[group])?
            .map(|index| tag_ref(cache, index)))
    };
    let button_icons = globals
        .block(0xC4, "button icons", HUD_ICON_SIZE, MAX_BUTTON_ICONS)?
        .iter()
        .map(|button| icon(&button, 0))
        .collect::<Result<_, _>>()?;
    let messages = match globals.reference(0xF0, "hud messages", &[HMT])? {
        Some(index) => import_hud_messages(cache, index)?,
        None => Vec::new(),
    };
    Ok(HudGlobals {
        tag,
        default_weapon_hud: reference(0x2C0, "default weapon hud", WPHI)?,
        motion_sensor_range: globals.f32(0x2D0, "motion sensor range")?,
        motion_sensor_velocity_sensitivity: globals
            .f32(0x2D4, "motion sensor velocity sensitivity")?,
        motion_sensor_scale: globals.f32(0x2D8, "motion sensor scale")?,
        multiplayer_scale: globals.f32(0x1BC, "hud scale in multiplayer")?,
        messaging: HudMessaging {
            anchor: HudAnchor::from_index(globals.i16(0, "messaging anchor")?),
            placement: placement(&globals, 0x24)?,
            font: reference(0x48, "single player font", FONT)?,
            color: packed_argb(&globals, 0x70, "state color")?,
            icons: reached.bitmap(&globals, 0xA4, "messaging icons")?,
            alternate_icon_text: reference(0xB4, "alternate icon text", USTR)?,
            button_icons,
            messages,
        },
    })
}

/// A picture of the HUD's messages (`icon_hud_element_definition`) at `at`.
fn icon(element: &Element<'_>, at: usize) -> Result<HudIcon, ImportError> {
    Ok(HudIcon {
        sequence: element.i16(at, "icon sequence index")?,
        width_offset: element.i16(at + 2, "icon width offset")?,
        offset: [
            element.i16(at + 4, "icon offset")?,
            element.i16(at + 6, "icon offset")?,
        ],
        color: element.u32(at + 8, "icon color")?,
        frame_rate: signed(element, at + 0xC, "icon frame rate")?,
        flags: element.u8(at + 0xD, "icon flags")?,
        text_index: element.i16(at + 0xE, "icon text index")?,
    })
}

/// A real ARGB colour at `at`, packed 0xAARRGGBB.
fn packed_argb(element: &Element<'_>, at: usize, field: &str) -> Result<u32, ImportError> {
    let mut packed = 0;
    for channel in 0..4 {
        let value = element.f32(at + channel * 4, field)?.clamp(0.0, 1.0);
        packed = packed << 8 | (value * 255.0).round() as u32;
    }
    Ok(packed)
}

/// The state messages of a `hmt ` tag: each message's pieces, its text cut
/// from the tag's text as the original cuts it, a piece's length at a time
/// from the message's first character. Text past the tag's end is dropped.
fn import_hud_messages(cache: &Cache, index: usize) -> Result<Vec<HudStateMessage>, ImportError> {
    let hmt = cache.tag(index, HMT, HUD_MESSAGE_TEXT_SIZE)?;
    let units: Vec<u16> = hmt
        .data(0, "text data", MAX_MESSAGE_TEXT)?
        .as_chunks::<2>()
        .0
        .iter()
        .map(|pair| u16::from_le_bytes(*pair))
        .collect();
    let elements: Vec<(u8, u8)> = hmt
        .block(
            0x14,
            "message elements",
            HUD_MESSAGE_ELEMENT_SIZE,
            MAX_MESSAGE_ELEMENTS,
        )?
        .iter()
        .map(|element| Ok((element.u8(0, "type")?, element.u8(1, "data")?)))
        .collect::<Result<_, ImportError>>()?;
    let mut messages = Vec::new();
    for message in hmt
        .block(0x20, "messages", HUD_MESSAGE_SIZE, MAX_MESSAGES)?
        .iter()
    {
        let mut position = usize::from(message.u16(0x20, "text start index")?);
        let first = usize::from(message.u16(0x22, "element start index")?);
        let count = usize::from(message.u8(0x24, "element count")?);
        let mut pieces = Vec::new();
        for &(kind, data) in elements.iter().skip(first).take(count) {
            if kind == 0 {
                let end = position.saturating_add(usize::from(data));
                // (A piece's length counts the end it is stored with.)
                let text = units
                    .get(position.min(units.len())..end.min(units.len()))
                    .unwrap_or_default();
                let text = text.split(|&unit| unit == 0).next().unwrap_or_default();
                pieces.push(HudMessageElement::Text(String::from_utf16_lossy(text)));
                position = end;
            } else {
                pieces.push(HudMessageElement::Icon(data));
            }
        }
        messages.push(HudStateMessage {
            name: message.text(0, "name", 31)?.to_owned(),
            elements: pieces,
        });
    }
    Ok(messages)
}
