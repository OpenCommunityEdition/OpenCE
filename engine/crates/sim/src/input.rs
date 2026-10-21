//! A tick's input, and recordings of them.
//!
//! The simulation reads nothing from the keyboard or the mouse itself: the
//! app turns them into one `TickInput` per tick, so a recording of those
//! replays a run exactly.

use crate::units::Radians;

/// Buttons held during a tick.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub struct Buttons(pub u32);

impl Buttons {
    /// Jump (and, for the free camera, nothing).
    pub const JUMP: Self = Self(1 << 0);
    /// Crouch.
    pub const CROUCH: Self = Self(1 << 1);
    /// Fire the weapon.
    pub const FIRE: Self = Self(1 << 2);
    /// The free camera's fast move.
    pub const SPRINT: Self = Self(1 << 3);
    /// Reload the weapon.
    pub const RELOAD: Self = Self(1 << 4);
    /// Change to the other weapon carried.
    pub const SWAP: Self = Self(1 << 5);
    /// The action: pick up the weapon at the player's feet.
    pub const ACTION: Self = Self(1 << 6);
    /// Melee.
    pub const MELEE: Self = Self(1 << 7);
    /// Throw a grenade.
    pub const GRENADE: Self = Self(1 << 8);
    /// Zoom.
    pub const ZOOM: Self = Self(1 << 9);
    /// Change to the other kind of grenade.
    pub const SWITCH_GRENADE: Self = Self(1 << 10);
    /// Every button the engine knows; others in a recording are refused.
    pub const ALL: Self = Self(0x7FF);

    /// Whether any of `buttons` is pressed this tick, held now and not
    /// `earlier`.
    pub fn pressed(self, earlier: Self, buttons: Self) -> bool {
        self.0 & !earlier.0 & buttons.0 != 0
    }

    /// Whether every button of `buttons` is held.
    pub fn contains(self, buttons: Self) -> bool {
        self.0 & buttons.0 == buttons.0
    }
}

/// What the player did during one tick.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct TickInput {
    /// The buttons held.
    pub buttons: Buttons,
    /// Forward (127) or back (-127).
    pub forward: i8,
    /// Right (127) or left (-127).
    pub right: i8,
    /// Up (127) or down (-127), for the free camera.
    pub up: i8,
    /// The turn to the left this tick (from the mouse, already scaled by
    /// the sensitivity).
    pub yaw: Radians,
    /// The turn up this tick.
    pub pitch: Radians,
}

/// A recording of inputs, one per tick.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct InputRecord {
    /// The inputs, from the first tick.
    pub ticks: Vec<TickInput>,
}

const MAGIC: &[u8; 8] = b"OCEINPUT";
/// Raised with any change to the record's format.
pub const RECORD_VERSION: u32 = 1;
const HEADER_SIZE: usize = 8 + 4 + 4 + 8;
const ENTRY_SIZE: usize = 16;
/// The longest recording read: a day of ticks.
pub const MAX_RECORD_TICKS: u64 = 60 * 60 * 60 * 24;

/// Why a recording was refused.
#[derive(Debug, PartialEq, thiserror::Error)]
pub enum RecordError {
    /// The file is shorter than its header or than its entries.
    #[error("the input record ends at byte {0}, inside its {1}")]
    Truncated(usize, &'static str),
    /// A field holds a value it cannot.
    #[error("the input record's {field} is {value}")]
    Invalid {
        /// The field (and the tick it is in).
        field: String,
        /// What it holds.
        value: String,
    },
}

fn invalid(field: impl Into<String>, value: impl ToString) -> RecordError {
    RecordError::Invalid {
        field: field.into(),
        value: value.to_string(),
    }
}

impl InputRecord {
    /// The record as a file: a header (magic, version, tick rate, count),
    /// then 16 little-endian bytes for each tick.
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(HEADER_SIZE + ENTRY_SIZE * self.ticks.len());
        bytes.extend_from_slice(MAGIC);
        bytes.extend_from_slice(&RECORD_VERSION.to_le_bytes());
        bytes.extend_from_slice(&crate::clock::TICKS_PER_SECOND.to_le_bytes());
        bytes.extend_from_slice(&(self.ticks.len() as u64).to_le_bytes());
        for input in &self.ticks {
            bytes.extend_from_slice(&input.buttons.0.to_le_bytes());
            bytes.extend_from_slice(&[input.forward as u8, input.right as u8, input.up as u8, 0]);
            bytes.extend_from_slice(&input.yaw.0.to_le_bytes());
            bytes.extend_from_slice(&input.pitch.0.to_le_bytes());
        }
        bytes
    }

    /// Reads a record, checking every field: the file is untrusted.
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, RecordError> {
        let header = bytes
            .get(..HEADER_SIZE)
            .ok_or(RecordError::Truncated(bytes.len(), "header"))?;
        let (magic, rest) = header.split_at(8);
        if magic != MAGIC {
            return Err(invalid("magic", format!("{magic:?}")));
        }
        let word =
            |at: usize| u32::from_le_bytes([rest[at], rest[at + 1], rest[at + 2], rest[at + 3]]);
        if word(0) != RECORD_VERSION {
            return Err(invalid("version", word(0)));
        }
        if word(4) != crate::clock::TICKS_PER_SECOND {
            return Err(invalid("tick rate", word(4)));
        }
        let mut count = [0u8; 8];
        count.copy_from_slice(&rest[8..16]);
        let count = u64::from_le_bytes(count);
        if count > MAX_RECORD_TICKS {
            return Err(invalid("tick count", count));
        }
        // (count is at most a day of ticks, so this cannot overflow)
        let entries_size = count as usize * ENTRY_SIZE;
        let entries = bytes
            .get(HEADER_SIZE..HEADER_SIZE + entries_size)
            .ok_or(RecordError::Truncated(bytes.len(), "entries"))?;
        if bytes.len() != HEADER_SIZE + entries_size {
            return Err(invalid("length", bytes.len()));
        }
        let mut ticks = Vec::with_capacity(count as usize);
        for (tick, entry) in entries.as_chunks::<ENTRY_SIZE>().0.iter().enumerate() {
            let buttons = Buttons(u32::from_le_bytes([entry[0], entry[1], entry[2], entry[3]]));
            if buttons.0 & !Buttons::ALL.0 != 0 {
                return Err(invalid(format!("buttons of tick {tick}"), buttons.0));
            }
            let axis = |at: usize, name: &str| {
                let value = entry[at] as i8;
                if value == i8::MIN {
                    Err(invalid(format!("{name} of tick {tick}"), value))
                } else {
                    Ok(value)
                }
            };
            let angle = |at: usize, name: &str| {
                let value =
                    f32::from_le_bytes([entry[at], entry[at + 1], entry[at + 2], entry[at + 3]]);
                // (no tick turns more than a half turn)
                if value.is_finite() && value.abs() <= std::f32::consts::PI {
                    Ok(Radians(value))
                } else {
                    Err(invalid(format!("{name} of tick {tick}"), value))
                }
            };
            if entry[7] != 0 {
                return Err(invalid(format!("padding of tick {tick}"), entry[7]));
            }
            ticks.push(TickInput {
                buttons,
                forward: axis(4, "forward")?,
                right: axis(5, "right")?,
                up: axis(6, "up")?,
                yaw: angle(8, "yaw")?,
                pitch: angle(12, "pitch")?,
            });
        }
        Ok(Self { ticks })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sample() -> InputRecord {
        InputRecord {
            ticks: vec![
                TickInput::default(),
                TickInput {
                    buttons: Buttons::SPRINT,
                    forward: 127,
                    right: -3,
                    up: 5,
                    yaw: Radians(0.01),
                    pitch: Radians(-0.02),
                },
            ],
        }
    }

    #[test]
    fn a_record_reads_back_as_written() {
        assert_eq!(InputRecord::from_bytes(&sample().to_bytes()), Ok(sample()));
    }

    #[test]
    fn every_truncation_is_refused() {
        let bytes = sample().to_bytes();
        for length in 0..bytes.len() {
            assert!(
                InputRecord::from_bytes(&bytes[..length]).is_err(),
                "length {length}"
            );
        }
    }

    #[test]
    fn bad_fields_are_named() {
        let mut bytes = sample().to_bytes();
        bytes[HEADER_SIZE + ENTRY_SIZE + 8..HEADER_SIZE + ENTRY_SIZE + 12]
            .copy_from_slice(&f32::NAN.to_le_bytes());
        let error = InputRecord::from_bytes(&bytes).err().map(|e| e.to_string());
        assert_eq!(
            error.as_deref(),
            Some("the input record's yaw of tick 1 is NaN")
        );

        let mut bytes = sample().to_bytes();
        // (the first button past the last there is)
        bytes[HEADER_SIZE..HEADER_SIZE + 4].copy_from_slice(&(Buttons::ALL.0 + 1).to_le_bytes());
        assert!(
            matches!(InputRecord::from_bytes(&bytes), Err(RecordError::Invalid { field, .. }) if field == "buttons of tick 0")
        );

        let mut bytes = sample().to_bytes();
        bytes[16..24].copy_from_slice(&u64::MAX.to_le_bytes());
        assert!(
            matches!(InputRecord::from_bytes(&bytes), Err(RecordError::Invalid { field, .. }) if field == "tick count")
        );
    }

    #[test]
    fn every_single_byte_change_is_refused_or_read() {
        // (no panic, whatever the byte)
        let bytes = sample().to_bytes();
        for at in 0..bytes.len() {
            for value in [0x00, 0x7f, 0x80, 0xff] {
                let mut changed = bytes.clone();
                changed[at] = value;
                let _ = InputRecord::from_bytes(&changed);
            }
        }
    }
}
