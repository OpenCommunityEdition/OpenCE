//! Saved games in the engine's own format: a mission is its scenario, its
//! difficulty and the inputs of each tick played, since the simulation and
//! the scripts are deterministic; loading one plays its inputs again from
//! the mission's start (`game_save.c` writes memory, which this need not).

use opence_sim::input::{InputRecord, RecordError, TickInput};
use opence_sim::objects::Difficulty;
use thiserror::Error;

const MAGIC: &[u8; 8] = b"OCESAVED";
/// Raised with any change to the saved game's format.
pub const CHECKPOINT_VERSION: u32 = 1;
/// The longest scenario path a saved game holds.
pub const MAX_SCENARIO_PATH: usize = 256;
const HEADER_SIZE: usize = 8 + 4 + 4 + 4;

/// A saved game.
#[derive(Clone, Debug, PartialEq)]
pub struct Checkpoint {
    /// The scenario's path.
    pub scenario: String,
    /// The difficulty played.
    pub difficulty: Difficulty,
    /// The inputs of each tick from the mission's start to the save.
    pub inputs: Vec<TickInput>,
}

/// Why a saved game was refused.
#[derive(Debug, PartialEq, Error)]
pub enum CheckpointError {
    /// The file is shorter than its header or its scenario's path.
    #[error("the saved game ends at byte {0}, inside its {1}")]
    Truncated(usize, &'static str),
    /// A field holds a value it cannot.
    #[error("the saved game's {0} is {1}")]
    Invalid(&'static str, String),
    /// Its inputs were refused.
    #[error("the saved game's inputs: {0}")]
    Inputs(#[from] RecordError),
    /// It is of another scenario than the one it was loaded into.
    #[error("the saved game is of {0}, not {1}")]
    Scenario(String, String),
    /// The scenario's player could not be made.
    #[error("the mission could not start: {0}")]
    Start(String),
}

fn difficulty_byte(difficulty: Difficulty) -> u8 {
    match difficulty {
        Difficulty::Easy => 0,
        Difficulty::Normal => 1,
        Difficulty::Hard => 2,
        Difficulty::Legendary => 3,
    }
}

impl Checkpoint {
    /// The saved game as a file: a header (magic, version, difficulty and
    /// three zero bytes, the path's length), the scenario's path, then the
    /// inputs as an input record.
    pub fn to_bytes(&self) -> Vec<u8> {
        // (a longer path is cut, at a character's end)
        let mut path = self.scenario.as_str();
        while path.len() > MAX_SCENARIO_PATH {
            let mut end = path.len() - 1;
            while !path.is_char_boundary(end) {
                end -= 1;
            }
            path = &path[..end];
        }
        let record = InputRecord {
            ticks: self.inputs.clone(),
        }
        .to_bytes();
        let mut bytes = Vec::with_capacity(HEADER_SIZE + path.len() + record.len());
        bytes.extend_from_slice(MAGIC);
        bytes.extend_from_slice(&CHECKPOINT_VERSION.to_le_bytes());
        bytes.extend_from_slice(&[difficulty_byte(self.difficulty), 0, 0, 0]);
        bytes.extend_from_slice(&(path.len() as u32).to_le_bytes());
        bytes.extend_from_slice(path.as_bytes());
        bytes.extend_from_slice(&record);
        bytes
    }

    /// Reads a saved game, checking every field: the file is untrusted.
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, CheckpointError> {
        let header = bytes
            .get(..HEADER_SIZE)
            .ok_or(CheckpointError::Truncated(bytes.len(), "header"))?;
        if &header[..8] != MAGIC {
            return Err(CheckpointError::Invalid(
                "magic",
                format!("{:?}", &header[..8]),
            ));
        }
        let word = |at: usize| {
            u32::from_le_bytes([header[at], header[at + 1], header[at + 2], header[at + 3]])
        };
        if word(8) != CHECKPOINT_VERSION {
            return Err(CheckpointError::Invalid("version", word(8).to_string()));
        }
        let difficulty = match header[12] {
            0 => Difficulty::Easy,
            1 => Difficulty::Normal,
            2 => Difficulty::Hard,
            3 => Difficulty::Legendary,
            other => return Err(CheckpointError::Invalid("difficulty", other.to_string())),
        };
        if header[13..16] != [0, 0, 0] {
            return Err(CheckpointError::Invalid(
                "padding",
                format!("{:?}", &header[13..16]),
            ));
        }
        let length = word(16) as usize;
        if length > MAX_SCENARIO_PATH {
            return Err(CheckpointError::Invalid(
                "path's length",
                length.to_string(),
            ));
        }
        let path = bytes
            .get(HEADER_SIZE..HEADER_SIZE + length)
            .ok_or(CheckpointError::Truncated(bytes.len(), "scenario's path"))?;
        let scenario = std::str::from_utf8(path)
            .map_err(|_| CheckpointError::Invalid("scenario's path", format!("{path:?}")))?
            .to_owned();
        let record = InputRecord::from_bytes(&bytes[HEADER_SIZE + length..])?;
        Ok(Self {
            scenario,
            difficulty,
            inputs: record.ticks,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_sim::input::Buttons;
    use opence_sim::units::Radians;

    fn sample() -> Checkpoint {
        Checkpoint {
            scenario: "levels\\a10\\a10".into(),
            difficulty: Difficulty::Hard,
            inputs: vec![
                TickInput::default(),
                TickInput {
                    buttons: Buttons::FIRE,
                    forward: 127,
                    right: -5,
                    up: 0,
                    yaw: Radians(0.25),
                    pitch: Radians(-0.1),
                },
            ],
        }
    }

    #[test]
    fn a_saved_game_reads_back_as_it_was_written() -> Result<(), CheckpointError> {
        let checkpoint = sample();
        assert_eq!(Checkpoint::from_bytes(&checkpoint.to_bytes())?, checkpoint);
        Ok(())
    }

    #[test]
    fn every_cut_and_every_changed_header_byte_is_refused_or_read() {
        let bytes = sample().to_bytes();
        for end in 0..bytes.len() {
            assert!(
                Checkpoint::from_bytes(&bytes[..end]).is_err(),
                "cut at {end}"
            );
        }
        for at in 0..HEADER_SIZE {
            let mut changed = bytes.clone();
            changed[at] ^= 0x80;
            // (a changed byte is refused, or read as some other valid game)
            if let Ok(read) = Checkpoint::from_bytes(&changed) {
                assert_ne!(read, sample(), "byte {at}");
            }
        }
    }

    #[test]
    fn a_path_too_long_is_refused() {
        let mut bytes = sample().to_bytes();
        bytes[16..20].copy_from_slice(&(MAX_SCENARIO_PATH as u32 + 1).to_le_bytes());
        assert!(matches!(
            Checkpoint::from_bytes(&bytes),
            Err(CheckpointError::Invalid("path's length", _))
        ));
    }
}
