//! A query's state kept from one tick to the next. `World::query` builds
//! a new state each call, a few dozen allocations for its access sets and
//! archetype lists; a kept state only takes in the archetypes made since,
//! and iterates them in the same order.

use std::fmt;

use bevy_ecs::prelude::*;
use bevy_ecs::query::{QueryData, QueryFilter, QueryState};
use bevy_ecs::world::WorldId;

/// A [`QueryState`] built the first time it is asked for, and again for
/// another world. A copy starts empty (it is built again when used), and
/// two are always equal, so the types that keep one keep their derives.
pub struct Cached<D: QueryData + 'static, F: QueryFilter + 'static = ()>(
    Option<(WorldId, QueryState<D, F>)>,
);

impl<D: QueryData + 'static, F: QueryFilter + 'static> Cached<D, F> {
    /// The state, for `world`.
    pub fn get(&mut self, world: &mut World) -> &mut QueryState<D, F> {
        if self.0.as_ref().is_some_and(|(id, _)| *id != world.id()) {
            self.0 = None;
        }
        let (_, state) = self
            .0
            .get_or_insert_with(|| (world.id(), world.query_filtered::<D, F>()));
        state
    }
}

impl<D: QueryData + 'static, F: QueryFilter + 'static> Default for Cached<D, F> {
    fn default() -> Self {
        Self(None)
    }
}

impl<D: QueryData + 'static, F: QueryFilter + 'static> Clone for Cached<D, F> {
    fn clone(&self) -> Self {
        Self::default()
    }
}

impl<D: QueryData + 'static, F: QueryFilter + 'static> PartialEq for Cached<D, F> {
    fn eq(&self, _: &Self) -> bool {
        true
    }
}

impl<D: QueryData + 'static, F: QueryFilter + 'static> fmt::Debug for Cached<D, F> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("Cached")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Component)]
    struct Number(u32);

    #[derive(Component)]
    struct Marked;

    fn numbers(cached: &mut Cached<&'static Number>, world: &mut World) -> Vec<u32> {
        let state = cached.get(world);
        state.iter(world).map(|number| number.0).collect()
    }

    #[test]
    fn a_kept_state_sees_what_is_spawned_after_it_in_the_same_order() {
        let mut world = World::new();
        world.spawn(Number(1));
        let mut cached = Cached::default();
        assert_eq!(numbers(&mut cached, &mut world), [1]);
        world.spawn((Number(2), Marked));
        world.spawn(Number(3));
        let fresh: Vec<u32> = world
            .query::<&Number>()
            .iter(&world)
            .map(|number| number.0)
            .collect();
        assert_eq!(numbers(&mut cached, &mut world), fresh);
    }

    #[test]
    fn a_state_is_built_again_for_another_world() {
        let mut first = World::new();
        first.spawn(Number(1));
        let mut cached = Cached::default();
        assert_eq!(numbers(&mut cached, &mut first), [1]);
        let mut second = World::new();
        second.spawn(Number(7));
        assert_eq!(numbers(&mut cached, &mut second), [7]);
        assert_eq!(cached.clone(), cached);
    }
}
