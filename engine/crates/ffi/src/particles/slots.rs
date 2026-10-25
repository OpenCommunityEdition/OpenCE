//! A data array, as the original keeps its contrails in
//! (`source/memory/data.c`): each held in a slot, a new one in the lowest
//! free slot (`datum_new`), and walked in slot order
//! (`data_next_index`). The order its parts update in, and so the order
//! they draw from the game's random numbers, is the original's and the
//! same each run, where a hash map's would follow its seed.

use std::collections::BTreeMap;

/// Values by key, in slots.
#[derive(Clone, Debug)]
pub(super) struct Slots<K, V> {
    slots: Vec<Option<(K, V)>>,
    /// Each held key's slot.
    by_key: BTreeMap<K, usize>,
}

impl<K, V> Default for Slots<K, V> {
    fn default() -> Self {
        Self {
            slots: Vec::new(),
            by_key: BTreeMap::new(),
        }
    }
}

impl<K: Ord + Clone, V> Slots<K, V> {
    /// `key`'s value, made by `make` in the lowest free slot when there is
    /// none.
    pub(super) fn get_or_insert_with(&mut self, key: K, make: impl FnOnce() -> V) -> &mut V {
        let slot = match self.by_key.get(&key) {
            Some(&slot) => slot,
            None => {
                let slot = self
                    .slots
                    .iter()
                    .position(Option::is_none)
                    .unwrap_or(self.slots.len());
                if slot == self.slots.len() {
                    self.slots.push(None);
                }
                self.by_key.insert(key.clone(), slot);
                slot
            }
        };
        &mut self.slots[slot].get_or_insert_with(|| (key, make())).1
    }

    /// Keeps, in slot order, the values `keep` says to; the rest free
    /// their slots.
    pub(super) fn retain(&mut self, mut keep: impl FnMut(&mut V) -> bool) {
        for held in &mut self.slots {
            let Some((key, value)) = held else {
                continue;
            };
            if !keep(value) {
                self.by_key.remove(key);
                *held = None;
            }
        }
    }
}

impl<K, V> Slots<K, V> {
    /// The keys and values, in slot order.
    pub(super) fn iter(&self) -> impl Iterator<Item = (&K, &V)> {
        self.slots.iter().flatten().map(|(key, value)| (key, value))
    }

    /// The values, in slot order.
    pub(super) fn values(&self) -> impl Iterator<Item = &V> {
        self.iter().map(|(_, value)| value)
    }

    /// The values, in slot order.
    pub(super) fn values_mut(&mut self) -> impl Iterator<Item = &mut V> {
        self.slots.iter_mut().flatten().map(|(_, value)| value)
    }

    /// How many are held.
    #[cfg(test)]
    pub(super) fn len(&self) -> usize {
        self.by_key.len()
    }

    /// Whether none are held.
    pub(super) fn is_empty(&self) -> bool {
        self.by_key.is_empty()
    }
}
