//! Debug drawing: lines, boxes, spheres and labels in the world that any
//! system adds, each on a layer that can be switched on and off and each
//! drawn for a number of ticks. The renderer draws them after the world.
//!
//! A shape on a layer that is off is not kept: adding it costs a test of
//! the layer's bit, and nothing is allocated while every layer is off.

use std::f32::consts::TAU;

/// The most lines kept; more are not added.
pub const MAX_LINES: usize = 32768;

/// The most labels kept; more are not added.
pub const MAX_LABELS: usize = 256;

/// The longest label, in bytes (a longer one is cut at a character).
pub const MAX_LABEL_BYTES: usize = 47;

/// The segments of each of a sphere's three circles.
pub const SPHERE_SEGMENTS: usize = 16;

/// What a shape shows, which its layer's switch turns on and off.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Layer {
    /// Shapes the shell's commands and scripts add (on unless switched
    /// off).
    General,
    /// The level's collision.
    Collision,
    /// The objects' physical bodies.
    Bodies,
    /// The AI's paths and firing positions.
    Ai,
    /// The scenario's trigger volumes.
    Triggers,
    /// The objects' origins and markers.
    Markers,
    /// The lights.
    Lights,
    /// The sound emitters.
    Sounds,
    /// The test map's galleries' headings and objects' names (on unless
    /// switched off).
    Labels,
}

impl Layer {
    /// Every layer, in order.
    pub const ALL: [Layer; 9] = [
        Layer::General,
        Layer::Collision,
        Layer::Bodies,
        Layer::Ai,
        Layer::Triggers,
        Layer::Markers,
        Layer::Lights,
        Layer::Sounds,
        Layer::Labels,
    ];

    /// Its name, as its `debug.draw.<name>` variable has it.
    pub fn name(self) -> &'static str {
        match self {
            Layer::General => "general",
            Layer::Collision => "collision",
            Layer::Bodies => "bodies",
            Layer::Ai => "ai",
            Layer::Triggers => "triggers",
            Layer::Markers => "markers",
            Layer::Lights => "lights",
            Layer::Sounds => "sounds",
            Layer::Labels => "labels",
        }
    }

    /// The layer named `name`.
    pub fn named(name: &str) -> Option<Layer> {
        Self::ALL.into_iter().find(|layer| layer.name() == name)
    }

    /// The colour its shapes are drawn in when they are given none.
    pub fn color(self) -> Color {
        match self {
            Layer::General => Color::WHITE,
            Layer::Collision => Color(0x40c0ffc0),
            Layer::Bodies => Color(0xffa040ff),
            Layer::Ai => Color(0xff4060ff),
            Layer::Triggers => Color(0x60ff60ff),
            Layer::Markers => Color(0xffff40ff),
            Layer::Lights => Color(0xfff0c0ff),
            Layer::Sounds => Color(0xc080ffff),
            Layer::Labels => Color(0xf0f0e0ff),
        }
    }

    fn bit(self) -> u16 {
        1 << self as u16
    }
}

/// A colour, red, green, blue and alpha a byte each from the top.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
pub struct Color(pub u32);

impl Color {
    /// White.
    pub const WHITE: Color = Color(0xffffffff);
    /// Red.
    pub const RED: Color = Color(0xff0000ff);
    /// Green.
    pub const GREEN: Color = Color(0x00ff00ff);
    /// Blue.
    pub const BLUE: Color = Color(0x0000ffff);
    /// Yellow.
    pub const YELLOW: Color = Color(0xffff00ff);
}

/// A line to draw.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Line {
    /// Its start.
    pub from: [f32; 3],
    /// Its end.
    pub to: [f32; 3],
    /// Its colour.
    pub color: Color,
    /// Its layer.
    pub layer: Layer,
    /// The tick it is gone at.
    expires: u64,
}

/// A label to draw at a point.
#[derive(Clone, Debug, PartialEq)]
pub struct Label {
    /// Where it is.
    pub at: [f32; 3],
    /// Its text.
    pub text: String,
    /// Its colour.
    pub color: Color,
    /// Its layer.
    pub layer: Layer,
    expires: u64,
}

/// The shapes being drawn.
#[derive(Clone, Debug, PartialEq)]
pub struct DebugDraw {
    on: u16,
    now: u64,
    lines: Vec<Line>,
    labels: Vec<Label>,
}

impl Default for DebugDraw {
    fn default() -> Self {
        Self::new()
    }
}

impl DebugDraw {
    /// Nothing drawn, the general and labels layers on and the others off.
    pub fn new() -> Self {
        Self {
            on: Layer::General.bit() | Layer::Labels.bit(),
            now: 0,
            lines: Vec::new(),
            labels: Vec::new(),
        }
    }

    /// Whether `layer` is on.
    pub fn is_on(&self, layer: Layer) -> bool {
        self.on & layer.bit() != 0
    }

    /// Whether any layer but the general one is on.
    pub fn any_world_layer(&self) -> bool {
        self.on & !Layer::General.bit() != 0
    }

    /// Switches `layer` on or off; its shapes go when it is switched off.
    pub fn set_layer(&mut self, layer: Layer, on: bool) {
        if on {
            self.on |= layer.bit();
        } else {
            self.on &= !layer.bit();
            self.lines.retain(|line| line.layer != layer);
            self.labels.retain(|label| label.layer != layer);
        }
    }

    /// The tick it is at.
    pub fn now(&self) -> u64 {
        self.now
    }

    /// Moves to tick `now`: the shapes whose ticks are over go.
    pub fn advance(&mut self, now: u64) {
        self.now = now;
        self.lines.retain(|line| line.expires > now);
        self.labels.retain(|label| label.expires > now);
    }

    /// Removes every shape (`debug_clear`).
    pub fn clear(&mut self) {
        self.lines.clear();
        self.labels.clear();
    }

    // (a lifetime of 0 is the next tick, as 1 is)
    fn expires(&self, ticks: u32) -> u64 {
        self.now + u64::from(ticks.max(1))
    }

    /// A line from `from` to `to`, drawn for `ticks` ticks.
    pub fn line(&mut self, layer: Layer, from: [f32; 3], to: [f32; 3], color: Color, ticks: u32) {
        if !self.is_on(layer) || self.lines.len() >= MAX_LINES {
            return;
        }
        let expires = self.expires(ticks);
        self.lines.push(Line {
            from,
            to,
            color,
            layer,
            expires,
        });
    }

    /// A box at `center`, turned by its `axes` (forward, left, up, each a
    /// unit vector) and reaching `half` along each, drawn for `ticks`
    /// ticks: its twelve edges.
    pub fn oriented_box(
        &mut self,
        layer: Layer,
        center: [f32; 3],
        axes: [[f32; 3]; 3],
        half: [f32; 3],
        color: Color,
        ticks: u32,
    ) {
        if !self.is_on(layer) {
            return;
        }
        let corner = |signs: [f32; 3]| {
            let mut point = center;
            for (axis, (sign, reach)) in axes.iter().zip(signs.iter().zip(half)) {
                for (value, along) in point.iter_mut().zip(axis) {
                    *value += sign * reach * along;
                }
            }
            point
        };
        let corners: [[f32; 3]; 8] = std::array::from_fn(|at| {
            let sign = |bit: usize| if at & bit != 0 { 1.0 } else { -1.0 };
            corner([sign(1), sign(2), sign(4)])
        });
        // (each edge joins two corners one bit apart)
        for a in 0..8 {
            for bit in [1, 2, 4] {
                if a & bit == 0 {
                    self.line(layer, corners[a], corners[a | bit], color, ticks);
                }
            }
        }
    }

    /// A box along the world's axes from `min` to `max`, drawn for
    /// `ticks` ticks.
    pub fn bounds(&mut self, layer: Layer, min: [f32; 3], max: [f32; 3], color: Color, ticks: u32) {
        let center = std::array::from_fn(|at| (min[at] + max[at]) / 2.0);
        let half = std::array::from_fn(|at| (max[at] - min[at]).abs() / 2.0);
        let axes = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]];
        self.oriented_box(layer, center, axes, half, color, ticks);
    }

    /// A sphere at `center` of `radius`, drawn for `ticks` ticks: its
    /// three circles about the world's axes.
    pub fn sphere(
        &mut self,
        layer: Layer,
        center: [f32; 3],
        radius: f32,
        color: Color,
        ticks: u32,
    ) {
        if !self.is_on(layer) {
            return;
        }
        let point = |plane: usize, at: usize| {
            let angle = TAU * at as f32 / SPHERE_SEGMENTS as f32;
            let (sin, cos) = libm::sincosf(angle);
            let mut point = center;
            point[plane] += radius * cos;
            point[(plane + 1) % 3] += radius * sin;
            point
        };
        for plane in 0..3 {
            for at in 0..SPHERE_SEGMENTS {
                self.line(layer, point(plane, at), point(plane, at + 1), color, ticks);
            }
        }
    }

    /// Three lines from `origin` along `axes` (forward red, left green,
    /// up blue), `length` long, drawn for `ticks` ticks.
    pub fn axes(
        &mut self,
        layer: Layer,
        origin: [f32; 3],
        axes: [[f32; 3]; 3],
        length: f32,
        ticks: u32,
    ) {
        if !self.is_on(layer) {
            return;
        }
        for (axis, color) in axes.iter().zip([Color::RED, Color::GREEN, Color::BLUE]) {
            let end = std::array::from_fn(|at| origin[at] + axis[at] * length);
            self.line(layer, origin, end, color, ticks);
        }
    }

    /// A label of `text` at `at`, drawn for `ticks` ticks.
    pub fn text(&mut self, layer: Layer, at: [f32; 3], text: &str, color: Color, ticks: u32) {
        if !self.is_on(layer) || self.labels.len() >= MAX_LABELS {
            return;
        }
        let mut end = text.len().min(MAX_LABEL_BYTES);
        while !text.is_char_boundary(end) {
            end -= 1;
        }
        let expires = self.expires(ticks);
        self.labels.push(Label {
            at,
            text: text[..end].to_owned(),
            color,
            layer,
            expires,
        });
    }

    /// The lines being drawn.
    pub fn lines(&self) -> &[Line] {
        &self.lines
    }

    /// The labels being drawn.
    pub fn labels(&self) -> &[Label] {
        &self.labels
    }

    /// The room taken by its lines and labels (nothing until a shape is
    /// added on a layer that is on).
    pub fn capacity(&self) -> usize {
        self.lines.capacity() + self.labels.capacity()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const O: [f32; 3] = [0.0; 3];
    const X: [f32; 3] = [1.0, 0.0, 0.0];

    #[test]
    fn shapes_last_their_ticks() {
        let mut draw = DebugDraw::new();
        draw.advance(10);
        draw.line(Layer::General, O, X, Color::WHITE, 3);
        draw.line(Layer::General, O, X, Color::RED, 0);
        draw.text(Layer::General, O, "hello", Color::WHITE, 1);
        assert_eq!((draw.lines().len(), draw.labels().len()), (2, 1));
        draw.advance(11);
        assert_eq!((draw.lines().len(), draw.labels().len()), (1, 0));
        draw.advance(12);
        assert_eq!(draw.lines().len(), 1);
        draw.advance(13);
        assert!(draw.lines().is_empty());
    }

    #[test]
    fn layers_switch() {
        let mut draw = DebugDraw::new();
        assert!(draw.is_on(Layer::General) && draw.is_on(Layer::Labels));
        draw.set_layer(Layer::Labels, false);
        assert!(!draw.any_world_layer());
        draw.line(Layer::Collision, O, X, Color::WHITE, 5);
        assert!(draw.lines().is_empty());
        draw.set_layer(Layer::Collision, true);
        assert!(draw.any_world_layer());
        draw.line(Layer::Collision, O, X, Color::WHITE, 5);
        draw.line(Layer::General, O, X, Color::WHITE, 5);
        draw.text(Layer::Collision, O, "edge", Color::WHITE, 5);
        draw.set_layer(Layer::Collision, false);
        assert_eq!(draw.lines().len(), 1);
        assert_eq!(draw.lines()[0].layer, Layer::General);
        assert!(draw.labels().is_empty());
        for layer in Layer::ALL {
            assert_eq!(Layer::named(layer.name()), Some(layer));
            assert_ne!(layer.color(), Color::default());
        }
        assert_eq!(Layer::named("nothing"), None);
    }

    #[test]
    fn nothing_is_allocated_when_off() {
        let mut draw = DebugDraw::new();
        draw.set_layer(Layer::General, false);
        draw.set_layer(Layer::Labels, false);
        for layer in Layer::ALL {
            draw.line(layer, O, X, Color::WHITE, 5);
            draw.bounds(layer, O, X, Color::WHITE, 5);
            draw.oriented_box(layer, O, [X; 3], X, Color::WHITE, 5);
            draw.sphere(layer, O, 1.0, Color::WHITE, 5);
            draw.axes(layer, O, [X; 3], 1.0, 5);
            draw.text(layer, O, "x", Color::WHITE, 5);
        }
        draw.advance(1);
        assert_eq!(draw.capacity(), 0);
    }

    #[test]
    fn shapes_are_made_of_lines() -> Result<(), Box<dyn std::error::Error>> {
        let mut draw = DebugDraw::new();
        draw.bounds(Layer::General, [0.0; 3], [2.0, 4.0, 6.0], Color::WHITE, 1);
        assert_eq!(draw.lines().len(), 12);
        for line in draw.lines() {
            for point in [line.from, line.to] {
                assert!(point[0] == 0.0 || point[0] == 2.0);
                assert!(point[1] == 0.0 || point[1] == 4.0);
                assert!(point[2] == 0.0 || point[2] == 6.0);
            }
            // (an edge runs along one axis)
            let differ = (0..3).filter(|&at| line.from[at] != line.to[at]).count();
            assert_eq!(differ, 1);
        }
        draw.clear();
        draw.sphere(Layer::General, [1.0, 1.0, 1.0], 2.0, Color::WHITE, 1);
        assert_eq!(draw.lines().len(), 3 * SPHERE_SEGMENTS);
        for line in draw.lines() {
            let distance = libm::sqrtf(
                line.from
                    .iter()
                    .map(|value| (value - 1.0) * (value - 1.0))
                    .sum::<f32>(),
            );
            assert!((distance - 2.0).abs() < 1e-4);
        }
        draw.clear();
        draw.axes(
            Layer::General,
            O,
            [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]],
            2.0,
            1,
        );
        let ends: Vec<_> = draw
            .lines()
            .iter()
            .map(|line| (line.to, line.color))
            .collect();
        assert_eq!(
            ends,
            [
                ([2.0, 0.0, 0.0], Color::RED),
                ([0.0, 2.0, 0.0], Color::GREEN),
                ([0.0, 0.0, 2.0], Color::BLUE)
            ]
        );
        let first = draw.lines().first().ok_or("no line")?;
        assert_eq!(first.from, O);
        Ok(())
    }

    #[test]
    fn it_holds_no_more_than_its_capacity() {
        let mut draw = DebugDraw::new();
        for _ in 0..MAX_LINES + 10 {
            draw.line(Layer::General, O, X, Color::WHITE, 1);
        }
        for _ in 0..MAX_LABELS + 10 {
            draw.text(Layer::General, O, "x", Color::WHITE, 1);
        }
        assert_eq!(
            (draw.lines().len(), draw.labels().len()),
            (MAX_LINES, MAX_LABELS)
        );
    }

    #[test]
    fn long_labels_are_cut_at_a_character() {
        let mut draw = DebugDraw::default();
        let text = "é".repeat(40);
        draw.text(Layer::General, O, &text, Color::WHITE, 1);
        let label = &draw.labels()[0];
        assert!(label.text.len() <= MAX_LABEL_BYTES);
        assert_eq!(label.text.chars().count(), MAX_LABEL_BYTES / 2);
    }
}
