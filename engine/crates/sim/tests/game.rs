//! Slayer and Capture the Flag: the scores, the respawn timers, the flags
//! taken, returned and captured, and a game on Blood Gulch with its flags
//! and equipment.

use std::error::Error;

use opence_content::{GameType, Level};
use opence_import::testing::stock_level;
use opence_sim::clock::TICKS_PER_SECOND;
use opence_sim::game::{
    FlagState, Game, GameEvent, Rules, Variant, FLAG_RETURN_TICKS, MINIMUM_RESPAWN_TICKS,
};
use opence_sim::input::TickInput;
use opence_sim::Simulation;

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

const RED_STAND: [f32; 3] = [0.0, 0.0, 0.0];
const BLUE_STAND: [f32; 3] = [50.0, 0.0, 0.0];

fn ctf() -> Rules {
    let mut rules = Rules::new(Variant::ctf(), &[(0, RED_STAND), (1, BLUE_STAND)]);
    rules.join(0);
    rules.join(1);
    rules
}

#[test]
fn slayer_scores_kills_and_takes_one_for_a_suicide() {
    let mut rules = Rules::new(Variant::slayer(), &[]);
    let a = rules.join(0);
    let b = rules.join(0);
    rules.player_killed(b, Some(a), [0.0; 3]);
    assert_eq!(rules.contestants()[a].score, 1);
    assert_eq!(rules.contestants()[a].kills, 1);
    assert!(!rules.contestants()[b].alive);
    // (players without teams are each their own team: not a betrayal)
    assert_eq!(rules.contestants()[b].respawn_ticks, MINIMUM_RESPAWN_TICKS);

    rules.player_killed(a, Some(a), [0.0; 3]);
    assert_eq!(rules.contestants()[a].score, 0);
    assert_eq!(rules.contestants()[a].suicides, 1);
    assert_eq!(
        rules.contestants()[a].respawn_ticks,
        Variant::slayer().suicide_penalty_ticks
    );
}

#[test]
fn team_slayer_sums_its_players_and_punishes_betrayal() {
    let mut rules = Rules::new(Variant::team_slayer(), &[]);
    let red = [rules.join(0), rules.join(0)];
    let blue = rules.join(1);
    rules.player_killed(blue, Some(red[0]), [0.0; 3]);
    let events = rules.step(&[None; 3]);
    assert!(events.contains(&GameEvent::Killed {
        victim: blue,
        killer: Some(red[0])
    }));
    rules.player_killed(red[1], Some(red[0]), [0.0; 3]);
    assert_eq!(rules.team_score(0), 0);
    assert_eq!(rules.contestants()[red[0]].betrayals, 1);
}

#[test]
fn the_dead_respawn_when_their_wait_is_over() {
    let mut rules = Rules::new(Variant::slayer(), &[]);
    let a = rules.join(0);
    let b = rules.join(0);
    rules.player_killed(b, Some(a), [0.0; 3]);
    for _ in 1..MINIMUM_RESPAWN_TICKS {
        assert!(!rules.step(&[None; 2]).contains(&GameEvent::Respawn(b)));
    }
    assert!(rules.step(&[None; 2]).contains(&GameEvent::Respawn(b)));
    assert!(rules.contestants()[b].alive);
}

#[test]
fn slayer_ends_at_the_score_to_win() {
    let mut rules = Rules::new(Variant::slayer(), &[]);
    let a = rules.join(0);
    let b = rules.join(0);
    for kill in 0..15 {
        assert!(!rules.is_over(), "over after {kill} kills");
        rules.player_killed(b, Some(a), [0.0; 3]);
        while !rules.contestants()[b].alive && !rules.is_over() {
            rules.step(&[None; 2]);
        }
    }
    assert!(rules.is_over());
    assert_eq!(rules.leader(), Some(0));
}

#[test]
fn a_time_limit_ends_the_game() {
    let variant = Variant {
        time_limit_ticks: Some(10),
        ..Variant::slayer()
    };
    let mut rules = Rules::new(variant, &[]);
    rules.join(0);
    for _ in 0..9 {
        assert!(!rules.step(&[None]).contains(&GameEvent::GameOver));
    }
    assert!(rules.step(&[None]).contains(&GameEvent::GameOver));
}

#[test]
fn ctf_kills_do_not_score() {
    let mut rules = ctf();
    rules.player_killed(1, Some(0), [0.0; 3]);
    assert_eq!(rules.team_score(0), 0);
    assert_eq!(
        rules.contestants()[1].respawn_ticks,
        Variant::ctf().respawn_ticks
    );
}

#[test]
fn a_flag_is_taken_carried_home_and_captured() {
    let mut rules = ctf();
    let events = rules.step(&[Some([50.5, 0.0, 0.0]), None]);
    assert!(events.contains(&GameEvent::FlagTaken { flag: 1, by: 0 }));
    assert_eq!(rules.flags()[1].state, FlagState::Carried(0));
    let events = rules.step(&[Some([0.5, 0.0, 0.0]), None]);
    assert!(events.contains(&GameEvent::Captured { flag: 1, by: 0 }));
    assert_eq!(rules.team_score(0), 1);
    assert_eq!(rules.flags()[1].state, FlagState::Home);
}

#[test]
fn a_dropped_flag_goes_home_when_its_team_touches_it() {
    let mut rules = ctf();
    rules.step(&[Some(BLUE_STAND), None]);
    rules.player_killed(0, Some(1), [25.0, 0.0, 0.0]);
    assert!(matches!(rules.flags()[1].state, FlagState::Dropped { .. }));
    let events = rules.step(&[None, Some([25.0, 0.5, 0.0])]);
    assert!(events.contains(&GameEvent::FlagReturned(1)));
}

#[test]
fn a_dropped_flag_goes_home_after_17_seconds() {
    let mut rules = ctf();
    rules.step(&[Some(BLUE_STAND), None]);
    rules.player_killed(0, None, [25.0, 0.0, 0.0]);
    let mut returned = None;
    for tick in 1..=FLAG_RETURN_TICKS + 1 {
        if rules
            .step(&[None, None])
            .contains(&GameEvent::FlagReturned(1))
        {
            returned = Some(tick);
            break;
        }
    }
    assert_eq!(returned, Some(FLAG_RETURN_TICKS));
    assert_eq!(FLAG_RETURN_TICKS, 17 * TICKS_PER_SECOND);
}

#[test]
fn the_flag_must_be_home_to_score_when_the_variant_says() {
    let variant = Variant {
        flag_at_home_to_score: true,
        ..Variant::ctf()
    };
    let mut rules = Rules::new(variant, &[(0, RED_STAND), (1, BLUE_STAND)]);
    rules.join(0);
    rules.join(1);
    // (both take the other's flag; red's carrier reaches home first)
    rules.step(&[Some(BLUE_STAND), Some(RED_STAND)]);
    let events = rules.step(&[Some(RED_STAND), Some([10.0, 0.0, 0.0])]);
    assert!(!events
        .iter()
        .any(|e| matches!(e, GameEvent::Captured { .. })));
}

fn bloodgulch() -> Result<Option<Level>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

#[test]
fn blood_gulch_plays_capture_the_flag() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    simulation.spawn_player(&level, 0)?;
    simulation.add_player(&level, 1, Some(1))?;
    let mut game = Game::start(&mut simulation, &level, Variant::ctf());
    assert_eq!(game.rules().flags().len(), 2);
    assert!(game.flag_objects().iter().all(Option::is_some));
    let equipment = game.equipment_objects();
    assert!(
        level
            .scenario
            .netgame_equipment
            .iter()
            .any(|e| e.game_types.matches(GameType::CaptureTheFlag))
            == !equipment.is_empty()
    );

    // (a player killed by the other stands again at a start after 10 s)
    game.rules_mut().player_killed(1, Some(0), [0.0; 3]);
    let before = simulation.players()[1].handle();
    let mut respawned = false;
    for _ in 0..Variant::ctf().respawn_ticks + 1 {
        simulation.step_all(&[TickInput::default(), TickInput::default()]);
        respawned |= game
            .step(&mut simulation, &level)
            .contains(&GameEvent::Respawn(1));
    }
    assert!(respawned);
    assert_ne!(simulation.players()[1].handle(), before);

    // (taken equipment comes back)
    if let Some(&taken) = equipment.first() {
        simulation.remove(taken);
        game.step(&mut simulation, &level);
        assert!(!game.equipment_objects().contains(&taken));
        for _ in 0..120 * TICKS_PER_SECOND {
            game.step(&mut simulation, &level);
        }
        assert_eq!(game.equipment_objects().len(), equipment.len());
    }
    Ok(())
}
