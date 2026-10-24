//! The AI's conversations (`ai_communication.c`): a conversation's lines
//! spoken in turn, each when the dialogue before it has ended, waited
//! after by its delay, and held, when the line says so, until a script
//! advances the conversation.
//!
//! A participant speaks from the object its name places in the world, or
//! everywhere alike when it has no body or speaks over the radio (Halo also
//! finds an actor to speak a line; the scenarios' conversations name their
//! speakers).

use std::collections::BTreeMap;
use std::sync::Arc;

use opence_content::{Conversation, ConversationLine, ConversationParticipant, TagRef};
use opence_script::{Native, Value};

use crate::{index, Mission};

/// A conversation's status, as `ai_conversation_status` gives it.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ConversationStatus {
    /// Not begun since the mission started.
    None = 0,
    /// Begun, no line yet spoken.
    Begun = 2,
    /// A line is spoken, or waited after.
    Speaking = 3,
    /// Waiting for a script to advance it.
    Waiting = 4,
    /// It spoke its last line.
    Finished = 6,
    /// A script stopped it.
    Stopped = 7,
}

/// The line `ai_conversation_line` gives for a conversation not running.
const NO_LINE: i16 = 999;

/// The selections of participants heard everywhere alike: a voice without
/// a body, the radio and the sergeant's radio.
const UNSPATIALIZED_SELECTIONS: [i16; 3] = [1, 6, 7];

/// What a conversation speaks with: the mission, or a test's stand-in.
pub(crate) trait Stage {
    /// Whether dialogue plays, so the next line must wait.
    fn dialogue_playing(&self) -> bool;
    /// Plays `sound` spoken by `participant`; returns its key.
    fn speak(&mut self, sound: &TagRef, participant: &ConversationParticipant) -> String;
    /// Whether the sound of key `key` has ended.
    fn ended(&self, key: &str) -> bool;
    /// A random number.
    fn random(&mut self) -> u32;
}

/// A conversation running.
#[derive(Clone, Debug, Default, PartialEq)]
struct Conversing {
    // (the line spoken, -1 before the first)
    line: i32,
    // (the dialogue variant each participant speaks)
    variants: Vec<Option<usize>>,
    any_spoken: bool,
    waiting: bool,
    told: bool,
    // (the line's: its sound, playing once spoken; the game ticks left of
    // its delay; whether it is spoken, its sound ended, and done)
    sound: Option<TagRef>,
    playing: Option<String>,
    delay: u32,
    flags: u16,
    spoken: bool,
    finished: bool,
    advance: bool,
}

/// The conversations running and those that ended.
#[derive(Clone, Debug, Default)]
pub(crate) struct Conversations {
    running: BTreeMap<usize, Conversing>,
    // (each that ended since it last began, and whether it spoke its last
    // line)
    ended: BTreeMap<usize, bool>,
}

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("ai_conversation", ai_conversation),
    ("ai_conversation_stop", ai_conversation_stop),
    ("ai_conversation_advance", ai_conversation_advance),
    ("ai_conversation_status", ai_conversation_status),
    ("ai_conversation_line", ai_conversation_line),
];

/// The dialogue variant a participant speaks: the last for any unit, else
/// one at random of those for a unit's variant (Halo prefers the variant of
/// the unit speaking; the speakers here are taken as of none).
fn variant(participant: &ConversationParticipant, stage: &mut impl Stage) -> Option<usize> {
    let any = participant.variants.iter().rposition(|&v| v == 0);
    if any.is_some() {
        return any;
    }
    let candidates: Vec<usize> = participant
        .variants
        .iter()
        .enumerate()
        .filter(|&(_, &v)| (1..100).contains(&v))
        .map(|(at, _)| at)
        .collect();
    match candidates.len() {
        0 => None,
        1 => candidates.first().copied(),
        n => candidates.get(stage.random() as usize % n).copied(),
    }
}

impl Conversations {
    /// Begins `conversation` (number `at`) again from its first line.
    pub(crate) fn begin(&mut self, at: usize, conversation: &Conversation, stage: &mut impl Stage) {
        let variants = conversation
            .participants
            .iter()
            .map(|participant| variant(participant, stage))
            .collect();
        self.ended.remove(&at);
        self.running.insert(
            at,
            Conversing {
                line: -1,
                variants,
                ..Conversing::default()
            },
        );
    }

    /// Stops conversation `at`, if it runs.
    pub(crate) fn stop(&mut self, at: usize) {
        if self.running.remove(&at).is_some() {
            self.ended.insert(at, false);
        }
    }

    /// Tells conversation `at` to go on past a line it waits after.
    pub(crate) fn advance(&mut self, at: usize) {
        if let Some(conversing) = self.running.get_mut(&at) {
            conversing.told = true;
        }
    }

    /// Conversation `at`'s status.
    pub(crate) fn status(&self, at: usize) -> ConversationStatus {
        match (self.running.get(&at), self.ended.get(&at)) {
            (Some(c), _) if c.waiting => ConversationStatus::Waiting,
            (Some(c), _) if c.any_spoken => ConversationStatus::Speaking,
            (Some(_), _) => ConversationStatus::Begun,
            (None, Some(true)) => ConversationStatus::Finished,
            (None, Some(false)) => ConversationStatus::Stopped,
            (None, None) => ConversationStatus::None,
        }
    }

    /// The line conversation `at` speaks; [`NO_LINE`] when it does not run.
    pub(crate) fn line(&self, at: usize) -> i16 {
        self.running
            .get(&at)
            .map_or(NO_LINE, |c| i16::try_from(c.line).unwrap_or(NO_LINE))
    }

    /// Moves each conversation on by a game tick: the line it speaks, and
    /// the next once that is done, until one must wait or the last is.
    pub(crate) fn update(&mut self, conversations: &[Conversation], stage: &mut impl Stage) {
        let mut done = Vec::new();
        for (&at, conversing) in &mut self.running {
            let Some(conversation) = conversations.get(at) else {
                done.push((at, false));
                continue;
            };
            if conversing.update(conversation, stage) {
                done.push((at, true));
            }
        }
        for (at, finished) in done {
            self.running.remove(&at);
            self.ended.insert(at, finished);
        }
    }
}

impl Conversing {
    /// Moves on by a game tick; true once the last line is done.
    fn update(&mut self, conversation: &Conversation, stage: &mut impl Stage) -> bool {
        let mut perform = self.line >= 0;
        loop {
            if perform && !self.perform(conversation, stage) {
                return false;
            }
            self.line += 1;
            let Some(line) = usize::try_from(self.line)
                .ok()
                .and_then(|at| conversation.lines.get(at))
            else {
                return true;
            };
            perform = self.begin(line, conversation);
        }
    }

    /// Readies `line`; false when its participant is not the
    /// conversation's, so it is passed over.
    fn begin(&mut self, line: &ConversationLine, conversation: &Conversation) -> bool {
        let Some(participant) = usize::try_from(line.participant)
            .ok()
            .filter(|&p| p < conversation.participants.len())
        else {
            return false;
        };
        self.sound = self
            .variants
            .get(participant)
            .copied()
            .flatten()
            .and_then(|variant| line.dialogue.get(variant).cloned().flatten());
        self.playing = None;
        // (the delay's seconds as game ticks, as Halo truncates them)
        let delay = line.delay * opence_script::TICKS_PER_SECOND as f32;
        self.delay = if delay.is_finite() && delay > 0.0 {
            delay as u32
        } else {
            0
        };
        self.flags = line.flags;
        self.spoken = false;
        self.finished = false;
        self.advance = false;
        true
    }

    /// Speaks the line readied once the dialogue before it has ended, waits
    /// out its sound and delay, and, when it says so, a script's word to go
    /// on; true once it is done.
    fn perform(&mut self, conversation: &Conversation, stage: &mut impl Stage) -> bool {
        if self.advance {
            return true;
        }
        if !self.spoken {
            if stage.dialogue_playing() {
                return false;
            }
            let participant = self.participant(conversation);
            self.playing = self
                .sound
                .as_ref()
                .zip(participant)
                .map(|(sound, participant)| stage.speak(sound, participant));
            self.spoken = true;
            self.any_spoken = true;
        }
        if !self.finished {
            self.finished = self.playing.as_deref().is_none_or(|key| stage.ended(key));
        }
        if !self.finished {
            return false;
        }
        if self.delay > 0 {
            self.delay -= 1;
            return false;
        }
        self.advance = true;
        if self.flags & ConversationLine::WAIT_UNTIL_TOLD != 0 {
            if !self.waiting {
                self.waiting = true;
                self.told = false;
            }
            if self.told {
                self.waiting = false;
            } else {
                self.advance = false;
            }
        }
        self.advance
    }

    fn participant<'a>(
        &self,
        conversation: &'a Conversation,
    ) -> Option<&'a ConversationParticipant> {
        let line = conversation.lines.get(usize::try_from(self.line).ok()?)?;
        conversation
            .participants
            .get(usize::try_from(line.participant).ok()?)
    }
}

impl Stage for Mission {
    fn dialogue_playing(&self) -> bool {
        Mission::dialogue_playing(self)
    }

    fn speak(&mut self, sound: &TagRef, participant: &ConversationParticipant) -> String {
        let object = if UNSPATIALIZED_SELECTIONS.contains(&participant.selection) {
            None
        } else {
            usize::try_from(participant.object_name)
                .ok()
                .and_then(|name| self.named(name))
        };
        self.start_sound(sound, object, 1.0)
    }

    fn ended(&self, key: &str) -> bool {
        self.sound_ended(key)
    }

    fn random(&mut self) -> u32 {
        self.next_random()
    }
}

impl Mission {
    /// Moves the conversations on by a game tick.
    pub(crate) fn converse(&mut self) {
        let level = Arc::clone(&self.level);
        let mut conversations = std::mem::take(&mut self.conversations);
        conversations.update(&level.scenario.campaign.conversations, self);
        self.conversations = conversations;
    }
}

/// The conversation a script names, and its number.
fn named(mission: &Mission, arguments: &[Value]) -> Option<usize> {
    let at = usize::try_from(index(arguments, 0)).ok()?;
    (at < mission.level.scenario.campaign.conversations.len()).then_some(at)
}

fn ai_conversation(mission: &mut Mission, arguments: &[Value]) -> Value {
    let Some(at) = named(mission, arguments) else {
        return Value::Boolean(false);
    };
    let level = Arc::clone(&mission.level);
    let mut conversations = std::mem::take(&mut mission.conversations);
    if let Some(conversation) = level.scenario.campaign.conversations.get(at) {
        conversations.begin(at, conversation, mission);
    }
    mission.conversations = conversations;
    Value::Boolean(true)
}

fn ai_conversation_stop(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(at) = named(mission, arguments) {
        mission.conversations.stop(at);
    }
    Value::Void
}

fn ai_conversation_advance(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(at) = named(mission, arguments) {
        mission.conversations.advance(at);
    }
    Value::Void
}

fn ai_conversation_status(mission: &mut Mission, arguments: &[Value]) -> Value {
    let status = named(mission, arguments).map_or(ConversationStatus::None, |at| {
        mission.conversations.status(at)
    });
    Value::Short(status as i16)
}

fn ai_conversation_line(mission: &mut Mission, arguments: &[Value]) -> Value {
    Value::Short(named(mission, arguments).map_or(NO_LINE, |at| mission.conversations.line(at)))
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::Group;

    /// A stage whose sounds last `length` game ticks and whose clock the
    /// test moves.
    #[derive(Default)]
    struct Fake {
        tick: u64,
        length: u64,
        ends: BTreeMap<String, u64>,
        spoken: Vec<(String, i16)>,
    }

    impl Stage for Fake {
        fn dialogue_playing(&self) -> bool {
            self.ends.values().any(|&ends| self.tick < ends)
        }

        fn speak(&mut self, sound: &TagRef, participant: &ConversationParticipant) -> String {
            self.ends
                .insert(sound.path.clone(), self.tick + self.length);
            self.spoken
                .push((sound.path.clone(), participant.object_name));
            sound.path.clone()
        }

        fn ended(&self, key: &str) -> bool {
            self.ends.get(key).is_none_or(|&ends| ends <= self.tick)
        }

        fn random(&mut self) -> u32 {
            1
        }
    }

    fn sound(path: &str) -> Option<TagRef> {
        Some(TagRef {
            group: Group(*b"snd!"),
            path: path.into(),
        })
    }

    fn line(participant: i16, delay: f32, flags: u16, path: &str) -> ConversationLine {
        ConversationLine {
            flags,
            participant,
            delay,
            dialogue: vec![sound(path), None, None, None, None, None],
        }
    }

    fn participant(object_name: i16) -> ConversationParticipant {
        ConversationParticipant {
            selection: 0,
            object_name,
            variants: [0, -1, -1, -1, -1, -1],
        }
    }

    fn conversation(lines: Vec<ConversationLine>) -> Conversation {
        Conversation {
            name: "talk".into(),
            participants: vec![participant(3), participant(4)],
            lines,
        }
    }

    /// Runs `ticks` game ticks of `conversations` on `stage`.
    fn run(
        running: &mut Conversations,
        conversations: &[Conversation],
        stage: &mut Fake,
        ticks: u64,
    ) {
        for _ in 0..ticks {
            running.update(conversations, stage);
            stage.tick += 1;
        }
    }

    #[test]
    fn lines_are_spoken_in_turn_each_after_the_one_before() {
        let talk = [conversation(vec![
            line(0, 0.0, 0, "first"),
            line(1, 0.0, 0, "second"),
        ])];
        let mut stage = Fake {
            length: 5,
            ..Fake::default()
        };
        let mut running = Conversations::default();
        assert_eq!(running.status(0), ConversationStatus::None);
        running.begin(0, &talk[0], &mut stage);
        assert_eq!(running.status(0), ConversationStatus::Begun);
        run(&mut running, &talk, &mut stage, 1);
        assert_eq!(stage.spoken, [("first".to_owned(), 3)]);
        assert_eq!(
            (running.status(0), running.line(0)),
            (ConversationStatus::Speaking, 0)
        );
        run(&mut running, &talk, &mut stage, 4);
        assert_eq!(stage.spoken.len(), 1);
        run(&mut running, &talk, &mut stage, 1);
        assert_eq!(stage.spoken.last(), Some(&("second".to_owned(), 4)));
        assert_eq!(running.line(0), 1);
        run(&mut running, &talk, &mut stage, 6);
        assert_eq!(running.status(0), ConversationStatus::Finished);
        assert_eq!(running.line(0), NO_LINE);
    }

    #[test]
    fn a_line_waits_out_its_delay_and_for_the_dialogue_playing() {
        let talk = [conversation(vec![
            line(0, 0.1, 0, "first"),
            line(1, 0.0, 0, "second"),
        ])];
        let mut stage = Fake {
            length: 2,
            ..Fake::default()
        };
        // (another's dialogue plays for the first ten ticks)
        stage.ends.insert("other".into(), 10);
        let mut running = Conversations::default();
        running.begin(0, &talk[0], &mut stage);
        run(&mut running, &talk, &mut stage, 10);
        assert!(stage.spoken.is_empty());
        // (spoken at 10, ended at 12, then its three ticks of delay)
        run(&mut running, &talk, &mut stage, 5);
        assert_eq!(stage.spoken.len(), 1);
        run(&mut running, &talk, &mut stage, 1);
        assert_eq!(stage.spoken.len(), 2);
    }

    #[test]
    fn a_line_that_says_so_waits_until_told_and_a_stop_ends_it() {
        let talk = [conversation(vec![
            line(0, 0.0, ConversationLine::WAIT_UNTIL_TOLD, "first"),
            line(1, 0.0, 0, "second"),
        ])];
        let mut stage = Fake {
            length: 1,
            ..Fake::default()
        };
        let mut running = Conversations::default();
        running.begin(0, &talk[0], &mut stage);
        // (an advance before the wait does not count)
        running.advance(0);
        run(&mut running, &talk, &mut stage, 20);
        assert_eq!(running.status(0), ConversationStatus::Waiting);
        assert_eq!(stage.spoken.len(), 1);
        running.advance(0);
        run(&mut running, &talk, &mut stage, 1);
        assert_eq!(stage.spoken.len(), 2);
        assert_eq!(running.status(0), ConversationStatus::Speaking);
        running.stop(0);
        assert_eq!(running.status(0), ConversationStatus::Stopped);
        running.begin(0, &talk[0], &mut stage);
        assert_eq!(running.status(0), ConversationStatus::Begun);
    }

    #[test]
    fn a_line_of_no_participant_is_passed_over_and_one_without_sound_is_silent() {
        let mut silent = line(1, 0.0, 0, "x");
        silent.dialogue = vec![None; 6];
        let talk = [conversation(vec![
            line(5, 0.0, 0, "nobody"),
            silent,
            line(0, 0.0, 0, "last"),
        ])];
        let mut stage = Fake {
            length: 1,
            ..Fake::default()
        };
        let mut running = Conversations::default();
        running.begin(0, &talk[0], &mut stage);
        run(&mut running, &talk, &mut stage, 1);
        assert_eq!(stage.spoken, [("last".to_owned(), 3)]);
        run(&mut running, &talk, &mut stage, 2);
        assert_eq!(running.status(0), ConversationStatus::Finished);
    }

    #[test]
    fn a_participant_speaks_the_variant_for_any_unit_else_one_of_its_own() {
        let mut stage = Fake::default();
        let any = ConversationParticipant {
            selection: 0,
            object_name: 0,
            variants: [2, 0, -1, 0, 5, -1],
        };
        assert_eq!(variant(&any, &mut stage), Some(3));
        let own = ConversationParticipant {
            variants: [-1, 2, 100, 5, -1, -1],
            ..any
        };
        // (the fake's random number is 1: the second candidate)
        assert_eq!(variant(&own, &mut stage), Some(3));
        let none = ConversationParticipant {
            variants: [-1; 6],
            ..any
        };
        assert_eq!(variant(&none, &mut stage), None);
    }
}
