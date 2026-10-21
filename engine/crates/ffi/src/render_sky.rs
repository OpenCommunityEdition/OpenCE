//! The level's sky and its fog, ready for the GPU: the sky's model, drawn
//! behind everything around the camera, and the fog's constants as the
//! game's rasterizer makes them from the sky's atmospheric fog
//! (`_rasterizer_window_set_fog`).

use opence_content::{AtmosphericFog, Level};

/// The level's sky, as the app draws it.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderSky {
    /// Its model, among the level's render models.
    pub model: Option<u32>,
    /// The fog outside.
    pub fog: RenderFog,
}

/// Atmospheric fog: a point `distance` ahead of the camera is mixed
/// toward `color` by `maximum_density` times how far it is from
/// `start_distance` to `end_distance`, at most all the way.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderFog {
    /// Red, green and blue.
    pub color: Vec<f32>,
    /// The most it hides, 0 to 1 (0: there is no fog).
    pub maximum_density: f32,
    /// Where it starts, in world units.
    pub start_distance: f32,
    /// Where it is densest, in world units (past the start).
    pub end_distance: f32,
}

impl RenderFog {
    /// No fog at all.
    pub fn none() -> Self {
        Self {
            color: vec![0.0; 3],
            maximum_density: 0.0,
            start_distance: 0.0,
            end_distance: 1.0,
        }
    }

    /// A sky's fog, as the game sets it: a density of zero or less is
    /// full, the densest distance is a little past the start at least,
    /// and an opaque distance of zero is no fog.
    pub fn from_sky(fog: &AtmosphericFog) -> Self {
        let finite = |value: f32| if value.is_finite() { value } else { 0.0 };
        let start = finite(fog.start_distance);
        let opaque = finite(fog.opaque_distance);
        if opaque == 0.0 {
            return Self::none();
        }
        let density = finite(fog.maximum_density);
        let density = if density <= 0.0 { 1.0 } else { density };
        Self {
            color: fog
                .color
                .iter()
                .map(|&c| finite(c).clamp(0.0, 1.0))
                .collect(),
            maximum_density: density.clamp(0.0, 1.0),
            start_distance: start,
            end_distance: opaque.max(start + 0.0001),
        }
    }

    /// How much of a point `distance` ahead it hides, 0 to 1.
    pub fn amount(&self, distance: f32) -> f32 {
        let along = (distance - self.start_distance) / (self.end_distance - self.start_distance);
        self.maximum_density * along.clamp(0.0, 1.0)
    }
}

/// The level's sky: its first, the one outside (none if it has none).
pub fn render_sky(level: &Level) -> Option<RenderSky> {
    let sky = level.skies.first()?;
    Some(RenderSky {
        model: sky
            .model
            .as_ref()
            .and_then(|tag| level.model_index(tag))
            .and_then(|index| u32::try_from(index).ok()),
        fog: RenderFog::from_sky(&sky.outdoor_fog),
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn fog(maximum_density: f32, start_distance: f32, opaque_distance: f32) -> AtmosphericFog {
        AtmosphericFog {
            color: [0.25, 0.5, 2.0],
            maximum_density,
            start_distance,
            opaque_distance,
        }
    }

    #[test]
    fn an_opaque_distance_of_zero_is_no_fog() {
        let made = RenderFog::from_sky(&fog(0.5, 10.0, 0.0));
        assert_eq!(made, RenderFog::none());
        assert_eq!(made.amount(1.0e6), 0.0);
    }

    #[test]
    fn a_density_of_zero_is_full_fog() {
        let made = RenderFog::from_sky(&fog(0.0, 10.0, 110.0));
        assert_eq!(made.maximum_density, 1.0);
        assert_eq!(made.color, [0.25, 0.5, 1.0]);
        assert_eq!(made.amount(5.0), 0.0);
        assert!((made.amount(60.0) - 0.5).abs() < 1e-6);
        assert_eq!(made.amount(500.0), 1.0);
    }

    #[test]
    fn fog_densest_before_it_starts_is_densest_just_past_its_start() {
        let made = RenderFog::from_sky(&fog(0.5, 20.0, 10.0));
        assert!(made.end_distance > made.start_distance);
        assert_eq!(made.amount(19.0), 0.0);
        assert_eq!(made.amount(21.0), 0.5);
    }
}
