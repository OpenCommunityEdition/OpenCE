//! The campaign in the app: a mission started at a difficulty, its scripts
//! run with the world, its cinematics' camera seen, the objects its scripts
//! make dressed, its sounds and music played, and its saved games written
//! and read again.

use std::collections::BTreeMap;
use std::sync::Arc;

use opence_campaign::{Checkpoint, MissionScripts, SoundOrders};
use opence_sim::animation::AnimationPlayer;
use opence_sim::input::TickInput;
use opence_sim::objects::Appearance;
use opence_sim::units::Radians;
use opence_sim::{Camera, Simulation, Transform};

use crate::{
    difficulty, publish, transform, Clock, Engine, EngineError, LevelGeometry, Running, Wardrobe,
};

/// A mission being played.
pub(crate) struct Campaign {
    scripts: MissionScripts,
    wardrobe: Wardrobe,
    // (what the scripts did to the sounds since the last frame played it)
    orders: SoundOrders,
    // (the voices of the impulses the scripts started, by their paths in
    // lowercase, so the scripts can stop them)
    voices: BTreeMap<String, opence_audio::VoiceId>,
    // (whether the mission went back to its last save since the last frame,
    // which silences the impulses it started before)
    silenced: bool,
}

/// The bit a scripted loop's key carries, apart from the keys of the
/// objects' loops and the background's.
const SCRIPTED_LOOP: u64 = 1 << 62;

/// The key a looping sound the scripts play is refreshed under: its path's
/// FNV-1a hash, marked as scripted.
fn loop_key(path: &str) -> u64 {
    let hash = path.bytes().fold(0xcbf2_9ce4_8422_2325_u64, |hash, byte| {
        (hash ^ u64::from(byte)).wrapping_mul(0x0000_0100_0000_01b3)
    });
    SCRIPTED_LOOP | (hash & (SCRIPTED_LOOP - 1))
}

/// How the mission stands, for the app's screens.
#[derive(Clone, Copy, Debug, Default, PartialEq, uniffi::Record)]
pub struct CampaignState {
    /// The mission is won: its last cinematic has played.
    pub won: bool,
    /// The mission is lost.
    pub lost: bool,
    /// A cinematic plays.
    pub cinematic: bool,
    /// The letterbox shows.
    pub letterbox: bool,
    /// A save is being written ("checkpoint... done").
    pub saving: bool,
    /// The mission went back to its last save this tick.
    pub reverted: bool,
    /// The saves written.
    pub saves: u32,
    /// The structure the mission plays in, which the app draws.
    pub bsp: u16,
}

impl Campaign {
    fn new(scripts: MissionScripts) -> Self {
        Self {
            scripts,
            wardrobe: Wardrobe::default(),
            orders: SoundOrders::default(),
            voices: BTreeMap::new(),
            silenced: false,
        }
    }

    /// Runs a tick of the mission over `simulation`, which is the world it
    /// plays (a revert gives it another).
    pub(crate) fn step(&mut self, simulation: &mut Simulation, input: TickInput) {
        std::mem::swap(self.scripts.mission_mut().sim_mut(), simulation);
        self.scripts.tick(input);
        std::mem::swap(self.scripts.mission_mut().sim_mut(), simulation);
        let mission = self.scripts.mission_mut();
        if mission.reverted() {
            self.orders = SoundOrders::default();
            self.silenced = true;
        }
        let orders = mission.take_sounds();
        self.orders.started.extend(orders.started);
        self.orders.stopped.extend(orders.stopped);
        self.orders.gains.extend(orders.gains);
        // (the titles and nav points are not shown yet)
        mission.take_events();
    }

    /// Plays what the scripts did to the sounds since the last frame on
    /// `player`: the impulses started (from their objects in
    /// `simulation`) and stopped, the classes' gains, and the loops they
    /// play, refreshed so they play on.
    pub(crate) fn play_sounds(
        &mut self,
        player: &mut opence_audio::SoundPlayer,
        simulation: &Simulation,
    ) {
        let orders = std::mem::take(&mut self.orders);
        if std::mem::take(&mut self.silenced) {
            for (_, voice) in std::mem::take(&mut self.voices) {
                player.mixer().stop(voice);
            }
        }
        for key in &orders.stopped {
            if let Some(voice) = self.voices.remove(key) {
                player.mixer().stop(voice);
            }
        }
        let at = |object: Option<opence_sim::objects::ObjectHandle>| {
            object
                .and_then(|handle| simulation.transform(handle))
                .map(|transform| transform.position.to_array())
        };
        for sound in &orders.started {
            let Some(index) = player.sound_index(&sound.tag.path) else {
                continue;
            };
            let source = opence_audio::Source {
                position: at(sound.object),
                scale: sound.scale,
                ..opence_audio::Source::default()
            };
            if let Some(voice) = player.impulse(index, &source) {
                let key = sound.tag.path.to_ascii_lowercase();
                if let Some(earlier) = self.voices.insert(key, voice) {
                    player.mixer().stop(earlier);
                }
            }
        }
        for gain in &orders.gains {
            player
                .classes()
                .set(&gain.class, gain.gain, i32::from(gain.ticks));
        }
        let mixer = player.mixer();
        self.voices.retain(|_, voice| mixer.is_playing(*voice));
        for (path, looping) in self.scripts.mission().loops() {
            if let Some(index) = player.looping_index(&looping.tag.path) {
                let source = opence_audio::Source {
                    position: at(looping.object),
                    scale: looping.scale,
                    ..opence_audio::Source::default()
                };
                player.refresh_looping(loop_key(path), index, &source, looping.alternate);
            }
        }
    }

    /// Evaluates console text with the mission's scripts over
    /// `simulation`, the world it plays.
    pub(crate) fn evaluate(
        &mut self,
        simulation: &mut Simulation,
        text: &str,
    ) -> Result<(opence_script::Value, opence_content::ValueType), opence_script::ConsoleError>
    {
        std::mem::swap(self.scripts.mission_mut().sim_mut(), simulation);
        let result = self.scripts.evaluate(text);
        std::mem::swap(self.scripts.mission_mut().sim_mut(), simulation);
        result
    }

    /// Gives each object not yet drawn or posed its model and animation
    /// (those the scripts made, and all of a world a revert loaded).
    pub(crate) fn dress(&mut self, simulation: &mut Simulation) {
        let level = self.scripts.mission().level();
        for handle in simulation.objects() {
            if simulation.component::<Appearance>(handle).is_none()
                && simulation.component::<AnimationPlayer>(handle).is_none()
            {
                self.wardrobe.dress(simulation, level, handle);
            }
        }
    }

    /// The cinematic camera, if the scripts hold it: its point's angles and
    /// its horizontal field of view made the camera's vertical one at 4:3
    /// (Hor+ widens it again).
    pub(crate) fn camera(&self) -> Option<Camera> {
        let (position, rotation, horizontal) = self.scripts.mission().cinematic_camera()?;
        let vertical = 2.0 * libm::atanf(libm::tanf(horizontal * 0.5) * 0.75);
        Some(Camera {
            transform: Transform {
                position,
                rotation,
                scale: 1.0,
            },
            vertical_fov: Radians(if vertical.is_finite() && vertical > 0.0 {
                vertical
            } else {
                Camera::default().vertical_fov.0
            }),
        })
    }

    /// The structure the mission plays in.
    pub(crate) fn bsp(&self) -> usize {
        usize::try_from(self.scripts.mission().flow().bsp).unwrap_or(0)
    }

    fn state(&self) -> CampaignState {
        let mission = self.scripts.mission();
        let flow = mission.flow();
        CampaignState {
            won: flow.won,
            lost: flow.lost,
            cinematic: mission.cinematic().playing,
            letterbox: mission.cinematic().letterbox,
            saving: flow.saving > 0,
            reverted: mission.reverted(),
            saves: flow.saves,
            bsp: u16::try_from(flow.bsp).unwrap_or(0),
        }
    }
}

/// The camera a world is seen with: the cinematic's, else its own.
pub(crate) fn camera(running: &Running) -> Camera {
    running
        .campaign
        .as_ref()
        .and_then(Campaign::camera)
        .unwrap_or_else(|| running.simulation.camera())
}

fn load_error(error: impl std::fmt::Display) -> EngineError {
    EngineError::Load {
        message: error.to_string(),
    }
}

#[uniffi::export]
impl Engine {
    /// Starts `geometry`'s mission at `difficulty` (0 easy to 3 legendary):
    /// the player at the first start, the objects the mission begins with,
    /// and its scripts, which run with each tick; publishes its first
    /// snapshot.
    pub fn start_campaign(
        &self,
        geometry: Arc<LevelGeometry>,
        difficulty_level: u8,
    ) -> Result<(), EngineError> {
        let level = geometry.content.clone();
        let scripts =
            MissionScripts::start(level, difficulty(difficulty_level)).map_err(load_error)?;
        self.run_campaign(geometry, scripts);
        Ok(())
    }

    /// The running mission's last save as a file (its start, before the
    /// first save). Fails without a mission.
    pub fn campaign_save(&self) -> Result<Vec<u8>, EngineError> {
        let state = self.lock();
        let campaign = state
            .running
            .as_ref()
            .and_then(|running| running.campaign.as_ref())
            .ok_or(EngineError::NoWorld)?;
        Ok(campaign.scripts.mission().checkpoint().to_bytes())
    }

    /// Plays `geometry`'s mission from the save `bytes` (a file
    /// [`Engine::campaign_save`] wrote). Fails for a file that is not one,
    /// or that is of another level.
    pub fn load_campaign(
        &self,
        geometry: Arc<LevelGeometry>,
        bytes: Vec<u8>,
    ) -> Result<(), EngineError> {
        let checkpoint = Checkpoint::from_bytes(&bytes).map_err(load_error)?;
        let scripts =
            MissionScripts::load(geometry.content.clone(), &checkpoint).map_err(load_error)?;
        self.run_campaign(geometry, scripts);
        Ok(())
    }

    /// How the running mission stands; none without one.
    pub fn campaign_state(&self) -> Option<CampaignState> {
        let state = self.lock();
        let campaign = state.running.as_ref()?.campaign.as_ref()?;
        Some(campaign.state())
    }
}

impl Engine {
    fn run_campaign(&self, geometry: Arc<LevelGeometry>, mut scripts: MissionScripts) {
        let mut simulation = Simulation::empty();
        std::mem::swap(scripts.mission_mut().sim_mut(), &mut simulation);
        let mut campaign = Campaign::new(scripts);
        campaign.dress(&mut simulation);
        let mut running = Running {
            simulation,
            previous_camera: transform(&Transform::default()),
            clock: Clock::default(),
            level: Some(geometry.content.clone()),
            network: None,
            dressed: 0,
            campaign: Some(campaign),
            solo: None,
            lights: crate::lights::Lights::new(&geometry.content),
            lit: Vec::new(),
            decals: crate::dynamic_decals::DynamicDecals::new(&geometry.content),
            marks: Vec::new(),
            effects: crate::effects::LevelEffects::new(&geometry.content),
            particles: crate::particles::LevelParticles::new(&geometry.content),
            particle_rows: Vec::new(),
            running_effects: Vec::new(),
            heard: Vec::new(),
            stepped: Vec::new(),
            sound_world: crate::sound_world::SoundWorld::new(&geometry.content),
            loops: Vec::new(),
        };
        running.previous_camera = transform(&camera(&running).transform);
        let mut state = self.lock();
        state.running = Some(running);
        publish(&mut state, &self.snapshots, 0.0);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_scripted_loops_key_is_its_own_and_none_an_objects_or_the_backgrounds() {
        let music = loop_key("sound\\music\\a10\\a10_01");
        assert_eq!(music, loop_key("sound\\music\\a10\\a10_01"));
        assert_ne!(music, loop_key("sound\\music\\a10\\a10_02"));
        assert_ne!(music & SCRIPTED_LOOP, 0);
        assert_ne!(music, crate::sound_world::BACKGROUND_KEY);
        // (an object's loops are keyed by its index and slot, far below)
        assert!(music > u64::from(u32::MAX) << 8);
    }
}
