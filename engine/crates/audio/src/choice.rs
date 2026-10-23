//! What a sound plays each time it is played (`sound_definitions.c` and
//! `sound_manager.c`): the pitch range nearest its pitch, a permutation of
//! that range chosen at random without repeating one until every other has
//! played, the values its scale moves between, and its promotion when it is
//! played too often.

use opence_content::{Sound, SoundPitchRange};

const RANDOM_A: u32 = 1_664_525;
const RANDOM_C: u32 = 1_013_904_223;

/// The random numbers the sounds draw: the game's local seed, which no
/// other machine shares, drawn as the game's seeded numbers are.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct LocalRandom(pub u32);

impl Default for LocalRandom {
    fn default() -> Self {
        Self(0xDEAD_BEEF)
    }
}

impl LocalRandom {
    /// The next number, from 0 to 65535 (`seed_random`).
    pub fn next_u16(&mut self) -> u16 {
        self.0 = self.0.wrapping_mul(RANDOM_A).wrapping_add(RANDOM_C);
        (self.0 >> 16) as u16
    }

    /// The next number, from 0 to 1 (`real_seed_random`).
    pub fn next_real(&mut self) -> f32 {
        f32::from(self.next_u16()) / 65535.0
    }

    /// The next number from `lower` up to but not including `upper`
    /// (`seed_random_range`).
    pub fn next_range(&mut self, lower: i16, upper: i16) -> i16 {
        let span = (i32::from(upper) - i32::from(lower)) as u32;
        let offset = span.wrapping_mul(u32::from(self.next_u16())) >> 16;
        (i32::from(lower) + offset as i32) as i16
    }

    /// A number between the bounds (`real_random_range`).
    pub fn between(&mut self, bounds: [f32; 2]) -> f32 {
        bounds[0] + (bounds[1] - bounds[0]) * self.next_real()
    }
}

/// A value moved by a sound's scale between its modifiers at 0 and at 1
/// (`sound_scale_value`).
pub fn scale_value(base: f32, zero: f32, one: f32, scale: f32) -> f32 {
    ((one - zero) * scale + zero) * base
}

/// A random value between the bounds, moved by the scale
/// (`sound_scale_random_value`).
pub fn scale_random_value(
    random: &mut LocalRandom,
    bounds: [f32; 2],
    zero: f32,
    one: f32,
    scale: f32,
) -> f32 {
    scale_value(random.between(bounds), zero, one, scale)
}

/// The pitch a playing sound bends to from `old`, no faster than `bend`
/// allows (`limit_pitch`): 0 lets it jump.
pub fn limit_pitch(desired: f32, old: f32, bend: f32) -> f32 {
    if bend == 0.0 || desired == old {
        desired
    } else if desired > old {
        desired.min(old * bend)
    } else {
        desired.max(old / bend)
    }
}

/// The pitch range a sound plays `pitch` with
/// (`sound_definition_find_pitch_range_by_pitch`): the one it played if the
/// pitch is still within its bounds, else the first whose bounds hold the
/// pitch, else the nearest. Only ranges with permutations are chosen;
/// `None` if no range has any.
pub fn pitch_range(sound: &Sound, pitch: f32, old: Option<usize>) -> Option<usize> {
    let within =
        |range: &SoundPitchRange| range.bend_bounds[0] <= pitch && pitch <= range.bend_bounds[1];
    if let Some(index) = old {
        if let Some(range) = sound.pitch_ranges.get(index) {
            if within(range) && !range.permutations.is_empty() {
                return Some(index);
            }
        }
    }
    let mut result = None;
    let mut closest = f32::MAX;
    for (index, range) in sound.pitch_ranges.iter().enumerate() {
        if range.permutations.is_empty() {
            continue;
        }
        if result.is_none() {
            result = Some(index);
        }
        if within(range) {
            return Some(index);
        }
        let ratio = if range.bend_bounds[1] < pitch {
            pitch / range.bend_bounds[1]
        } else {
            range.bend_bounds[0] / pitch
        };
        if ratio < closest {
            result = Some(index);
            closest = ratio;
        }
    }
    result
}

/// What a pitch range remembers between plays: the permutations played
/// since they were last all played, the last played, and one a script has
/// forced.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct RangeState {
    /// A bit for each permutation played.
    pub played: u32,
    /// The permutation played last.
    pub previous: Option<u16>,
    /// The permutation played next, once.
    pub forced: Option<u16>,
}

impl RangeState {
    /// Forgets the played permutations once all have played, but the last
    /// (`try_to_reset_permutations`), so that none plays twice running.
    fn try_to_reset(&mut self, count: u16) {
        let all = if count >= 32 {
            u32::MAX
        } else {
            (1u32 << count) - 1
        };
        if !self.played & all == 0 {
            self.played = 0;
            if count > 1 {
                if let Some(previous) = self.previous.filter(|p| *p < 32) {
                    self.played = 1 << previous;
                }
            }
        }
    }

    fn has_played(&self, index: u16) -> bool {
        index < 32 && self.played & (1 << index) != 0
    }

    fn mark_played(&mut self, index: u16) {
        if index < 32 {
            self.played |= 1 << index;
        }
    }
}

/// The permutation of `range` the sound plays next
/// (`sound_definition_next_permutation`): the forced one, else the one
/// `current` links to for a sound of linked permutations, else one chosen
/// at random from those not yet played, each skipped by its skip fraction.
/// `None` when no permutation plays.
pub fn next_permutation(
    sound: &Sound,
    range: usize,
    state: &mut RangeState,
    current: Option<u16>,
    random: &mut LocalRandom,
) -> Option<u16> {
    let pitch_range = sound.pitch_ranges.get(range)?;
    let count = u16::try_from(pitch_range.permutations.len()).ok()?;
    if count == 0 {
        return None;
    }
    let actual = pitch_range.actual_permutation_count.clamp(1, count);
    if let Some(forced) = state.forced.take().filter(|f| *f < count) {
        state.previous = Some(forced);
        return Some(forced);
    }
    if sound.flags & Sound::LINKED_PERMUTATIONS != 0 {
        if let Some(permutation) =
            current.and_then(|c| pitch_range.permutations.get(usize::from(c)))
        {
            return permutation.next.filter(|n| *n < count);
        }
    }
    let mut selected = random.next_range(0, actual as i16).max(0) as u16;
    let mut attempts = 0;
    loop {
        state.try_to_reset(actual);
        if !state.has_played(selected) {
            state.mark_played(selected);
            if attempts == 16 {
                break;
            }
            attempts += 1;
            let skip = pitch_range
                .permutations
                .get(usize::from(selected))
                .map_or(0.0, |p| p.skip_fraction);
            if random.next_real() >= skip {
                break;
            }
        }
        selected += 1;
        if selected >= actual {
            selected = 0;
        }
    }
    state.previous = Some(selected);
    Some(selected)
}

/// A sound's promotion (`sound_promotion`): played more than its count at
/// once, measured by its longest permutation, it plays its promotion sound
/// instead, or does not play.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Promotion {
    /// The milliseconds of play it has counted.
    pub counter: f32,
    /// When it last counted, in milliseconds.
    pub time: f32,
}

/// What a play of a sound with a promotion does.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Promoted {
    /// It plays.
    Play,
    /// Its promotion sound plays in its place.
    Promote,
    /// Nothing plays.
    Skip,
}

impl Promotion {
    /// Counts a play at `now` (in milliseconds) of a sound whose longest
    /// permutation lasts `longest`, `count` of which promote it; whether it
    /// has a promotion sound to play.
    pub fn play(&mut self, now: f32, longest: f32, count: u16, has_promotion: bool) -> Promoted {
        self.counter = (self.counter - (now - self.time)).max(0.0);
        self.time = now;
        self.counter += longest;
        if self.counter > f32::from(count) * longest {
            if has_promotion {
                self.counter = 0.0;
                Promoted::Promote
            } else {
                self.counter -= longest;
                Promoted::Skip
            }
        } else {
            Promoted::Play
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Group, SoundCompression, SoundModifiers, SoundPermutation, TagRef};

    fn permutation(skip: f32, next: Option<u16>) -> SoundPermutation {
        SoundPermutation {
            name: String::new(),
            skip_fraction: skip,
            gain: 1.0,
            compression: SoundCompression::XboxAdpcm,
            next,
            samples: Vec::new(),
            mouth: Vec::new(),
        }
    }

    fn range(bounds: [f32; 2], permutations: Vec<SoundPermutation>) -> SoundPitchRange {
        SoundPitchRange {
            name: String::new(),
            natural_pitch: 1.0,
            bend_bounds: bounds,
            actual_permutation_count: u16::try_from(permutations.len()).unwrap_or(0),
            playback_rate: 1.0,
            permutations,
        }
    }

    fn make_sound(flags: u32, pitch_ranges: Vec<SoundPitchRange>) -> Sound {
        Sound {
            tag: TagRef {
                group: Group(*b"snd!"),
                path: String::new(),
            },
            flags,
            class: 0,
            sample_rate: 22050,
            channels: 1,
            distance: [1.0, 10.0],
            skip_fraction: 0.0,
            random_pitch_bounds: [1.0, 1.0],
            cone_angles: [0.0, 0.0],
            outer_cone_gain: 1.0,
            gain_modifier: 1.0,
            maximum_bend_per_second: 0.0,
            zero: SoundModifiers::default(),
            one: SoundModifiers::default(),
            promotion: None,
            promotion_count: 0,
            longest_permutation_ms: 0,
            pitch_ranges,
        }
    }

    #[test]
    fn the_numbers_are_the_games() {
        let mut random = LocalRandom::default();
        let first = 0xDEAD_BEEFu32
            .wrapping_mul(1_664_525)
            .wrapping_add(1_013_904_223);
        let expected = (first >> 16) as u16;
        assert_eq!(random.next_u16(), expected);
        for _ in 0..100 {
            assert!((2..5).contains(&random.next_range(2, 5)));
            let value = random.between([0.5, 1.5]);
            assert!((0.5..=1.5).contains(&value));
        }
    }

    #[test]
    fn values_scale_and_pitches_bend() {
        assert!((scale_value(2.0, 0.5, 1.0, 0.5) - 1.5).abs() < 1e-6);
        assert!((scale_value(2.0, 0.5, 1.0, 0.0) - 1.0).abs() < 1e-6);
        let mut random = LocalRandom::default();
        let value = scale_random_value(&mut random, [1.0, 1.0], 1.0, 2.0, 1.0);
        assert!((value - 2.0).abs() < 1e-6);
        assert!((limit_pitch(2.0, 1.0, 0.0) - 2.0).abs() < 1e-6);
        assert!((limit_pitch(2.0, 1.0, 1.25) - 1.25).abs() < 1e-6);
        assert!((limit_pitch(0.5, 1.0, 1.25) - 0.8).abs() < 1e-6);
        assert!((limit_pitch(1.1, 1.0, 1.25) - 1.1).abs() < 1e-6);
        assert!((limit_pitch(1.0, 1.0, 1.25) - 1.0).abs() < 1e-6);
    }

    #[test]
    fn a_pitch_plays_with_the_nearest_range() {
        let low = range([0.5, 0.9], vec![permutation(0.0, None)]);
        let empty = range([0.9, 1.1], Vec::new());
        let high = range([1.2, 2.0], vec![permutation(0.0, None)]);
        let sound = make_sound(0, vec![low, empty, high]);
        assert_eq!(pitch_range(&sound, 0.7, None), Some(0));
        assert_eq!(pitch_range(&sound, 1.5, None), Some(2));
        // 1.0 is 1.11 above the low range's top, 1.2 below the high's floor.
        assert_eq!(pitch_range(&sound, 1.0, None), Some(0));
        assert_eq!(pitch_range(&sound, 1.15, None), Some(2));
        assert_eq!(pitch_range(&sound, 3.0, None), Some(2));
        // the old range is kept while it holds the pitch
        assert_eq!(pitch_range(&sound, 0.7, Some(2)), Some(0));
        assert_eq!(pitch_range(&sound, 1.5, Some(2)), Some(2));
        // the empty range is never chosen, and no range at all is none
        assert_eq!(pitch_range(&sound, 1.0, Some(1)), Some(0));
        let silent = make_sound(0, vec![range([0.0, 1.0], Vec::new())]);
        assert_eq!(pitch_range(&silent, 1.0, None), None);
        // a NaN pitch takes the first range with permutations
        assert_eq!(pitch_range(&sound, f32::NAN, None), Some(0));
    }

    #[test]
    fn every_permutation_plays_before_one_repeats() -> Result<(), Box<dyn std::error::Error>> {
        let permutations = (0..5).map(|_| permutation(0.0, None)).collect();
        let sound = make_sound(0, vec![range([0.0, 2.0], permutations)]);
        let mut state = RangeState::default();
        let mut random = LocalRandom(7);
        let mut plays = Vec::new();
        for _ in 0..45 {
            let chosen = next_permutation(&sound, 0, &mut state, None, &mut random);
            plays.push(chosen.ok_or("no permutation")?);
        }
        // the first five are each permutation once; after them, each four
        // are the four other than the one played before them
        let distinct = |group: &[u16]| {
            let mut sorted = group.to_vec();
            sorted.sort_unstable();
            sorted.dedup();
            sorted.len() == group.len()
        };
        assert!(plays.get(..5).is_some_and(distinct), "{plays:?}");
        for start in (5..45).step_by(4) {
            let group = plays.get(start..start + 4).ok_or("short")?;
            let before = plays.get(start - 1).ok_or("short")?;
            assert!(distinct(group) && !group.contains(before), "{plays:?}");
        }
        Ok(())
    }

    #[test]
    fn skipped_forced_and_linked_permutations() {
        // the first is always skipped while another can play
        let sound_skip = make_sound(
            0,
            vec![range(
                [0.0, 2.0],
                vec![permutation(1.0, None), permutation(0.0, None)],
            )],
        );
        let mut state = RangeState::default();
        let mut random = LocalRandom(3);
        let mut ones = 0;
        for _ in 0..10 {
            if next_permutation(&sound_skip, 0, &mut state, None, &mut random) == Some(1) {
                ones += 1;
            }
        }
        assert!(ones >= 5);

        let mut state = RangeState {
            forced: Some(1),
            ..RangeState::default()
        };
        assert_eq!(
            next_permutation(&sound_skip, 0, &mut state, None, &mut random),
            Some(1)
        );
        assert_eq!(state.forced, None);
        assert_eq!(state.previous, Some(1));

        let linked = make_sound(
            Sound::LINKED_PERMUTATIONS,
            vec![range(
                [0.0, 2.0],
                vec![
                    permutation(0.0, Some(1)),
                    permutation(0.0, Some(9)),
                    permutation(0.0, None),
                ],
            )],
        );
        let mut state = RangeState::default();
        assert_eq!(
            next_permutation(&linked, 0, &mut state, Some(0), &mut random),
            Some(1)
        );
        assert_eq!(
            next_permutation(&linked, 0, &mut state, Some(1), &mut random),
            None,
            "a link past the range ends the chain"
        );
        assert_eq!(
            next_permutation(&linked, 0, &mut state, Some(2), &mut random),
            None
        );
        assert!(next_permutation(&linked, 0, &mut state, None, &mut random).is_some());
        assert_eq!(
            next_permutation(&linked, 4, &mut state, None, &mut random),
            None
        );
        let empty = make_sound(0, vec![range([0.0, 2.0], Vec::new())]);
        assert_eq!(
            next_permutation(&empty, 0, &mut state, None, &mut random),
            None
        );
    }

    #[test]
    fn a_sound_played_too_often_is_promoted() {
        let mut promotion = Promotion::default();
        // three plays of a 1000 ms sound at once with a count of 2
        assert_eq!(promotion.play(0.0, 1000.0, 2, true), Promoted::Play);
        assert_eq!(promotion.play(0.0, 1000.0, 2, true), Promoted::Play);
        assert_eq!(promotion.play(0.0, 1000.0, 2, true), Promoted::Promote);
        assert_eq!(promotion.play(0.0, 1000.0, 2, true), Promoted::Play);

        let mut promotion = Promotion::default();
        assert_eq!(promotion.play(0.0, 1000.0, 1, false), Promoted::Play);
        assert_eq!(promotion.play(0.0, 1000.0, 1, false), Promoted::Skip);
        // a second later the first play is over
        assert_eq!(promotion.play(1000.0, 1000.0, 1, false), Promoted::Play);
    }
}
