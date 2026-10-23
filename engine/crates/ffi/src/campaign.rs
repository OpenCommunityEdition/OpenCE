//! The campaign in the app: a mission started at a difficulty, its scripts
//! run with the world, its cinematics' camera seen, the objects its scripts
//! make dressed, and its saved games written and read again.

use std::sync::Arc;

use opence_campaign::{Checkpoint, MissionScripts};
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
        }
    }

    /// Runs a tick of the mission over `simulation`, which is the world it
    /// plays (a revert gives it another).
    pub(crate) fn step(&mut self, simulation: &mut Simulation, input: TickInput) {
        std::mem::swap(self.scripts.mission_mut().sim_mut(), simulation);
        self.scripts.tick(input);
        std::mem::swap(self.scripts.mission_mut().sim_mut(), simulation);
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
            lights: crate::lights::Lights::new(&geometry.content),
            lit: Vec::new(),
            decals: crate::dynamic_decals::DynamicDecals::new(&geometry.content),
            marks: Vec::new(),
            effects: crate::effects::LevelEffects::new(&geometry.content),
            particles: crate::particles::LevelParticles::new(&geometry.content),
            particle_rows: Vec::new(),
            running_effects: Vec::new(),
        };
        running.previous_camera = transform(&camera(&running).transform);
        let mut state = self.lock();
        state.running = Some(running);
        publish(&mut state, &self.snapshots, 0.0);
    }
}
