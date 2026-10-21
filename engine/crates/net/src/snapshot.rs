//! Snapshots of the world the host sends, and their delta compression: a
//! snapshot is sent as the objects that changed since one the client
//! acknowledged, each with only its changed fields.

use crate::wire::{Reader, Writer};
use crate::{DecodeError, EncodeError};

/// The most objects in a snapshot.
pub const MAX_SNAPSHOT_OBJECTS: usize = 1024;

/// One object's state in a snapshot.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ObjectState {
    /// Its object's identifier, the same on every machine.
    pub id: u32,
    /// Its object's definition: an index into the level's objects.
    pub definition: u16,
    /// Where it is.
    pub position: [f32; 3],
    /// Its orientation, a quaternion.
    pub rotation: [f32; 4],
    /// Its velocity.
    pub velocity: [f32; 3],
    /// Its body's vitality, from 0 to 1.
    pub body: f32,
    /// Its shield's vitality, from 0 to 1.
    pub shield: f32,
}

/// The world at a tick: its objects, ordered by identifier.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Snapshot {
    /// The host's tick.
    pub tick: u32,
    /// The objects.
    pub objects: Vec<ObjectState>,
}

/// A snapshot as the changes from an earlier one.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SnapshotDelta {
    /// The tick of the snapshot it changes (none: from nothing).
    pub base: Option<u32>,
    /// The tick it brings the base to.
    pub tick: u32,
    /// The objects gone since the base.
    pub removed: Vec<u32>,
    /// The objects new or changed, with the fields that changed.
    pub changed: Vec<ObjectChange>,
}

/// An object's changed fields; `None` is unchanged.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ObjectChange {
    /// Its identifier.
    pub id: u32,
    /// Its definition.
    pub definition: Option<u16>,
    /// Its position.
    pub position: Option<[f32; 3]>,
    /// Its rotation.
    pub rotation: Option<[f32; 4]>,
    /// Its velocity.
    pub velocity: Option<[f32; 3]>,
    /// Its body's vitality.
    pub body: Option<f32>,
    /// Its shield's vitality.
    pub shield: Option<f32>,
}

const DEFINITION: u8 = 1 << 0;
const POSITION: u8 = 1 << 1;
const ROTATION: u8 = 1 << 2;
const VELOCITY: u8 = 1 << 3;
const BODY: u8 = 1 << 4;
const SHIELD: u8 = 1 << 5;
const FIELDS: u8 = (1 << 6) - 1;

/// A field's new value, if it changed (bit for bit: delta compression is
/// exact, so both machines rebuild the same snapshot).
fn changed<T: PartialEq + Copy>(old: Option<T>, new: T, same: impl Fn(T, T) -> bool) -> Option<T> {
    match old {
        Some(old) if same(old, new) => None,
        _ => Some(new),
    }
}

fn same_bits<const N: usize>(a: [f32; N], b: [f32; N]) -> bool {
    a.iter().zip(&b).all(|(a, b)| a.to_bits() == b.to_bits())
}

impl Snapshot {
    /// The changes from `base` (from nothing: every object, whole).
    pub fn delta(&self, base: Option<&Snapshot>) -> SnapshotDelta {
        let empty = Snapshot::default();
        let old = base.unwrap_or(&empty);
        let find = |id: u32| {
            old.objects
                .binary_search_by_key(&id, |object| object.id)
                .ok()
                .and_then(|at| old.objects.get(at))
        };
        let removed = old
            .objects
            .iter()
            .filter(|object| {
                self.objects
                    .binary_search_by_key(&object.id, |o| o.id)
                    .is_err()
            })
            .map(|object| object.id)
            .collect();
        let changed = self
            .objects
            .iter()
            .filter_map(|new| {
                let was = find(new.id);
                let change = ObjectChange {
                    id: new.id,
                    definition: changed(was.map(|o| o.definition), new.definition, |a, b| a == b),
                    position: changed(was.map(|o| o.position), new.position, same_bits),
                    rotation: changed(was.map(|o| o.rotation), new.rotation, same_bits),
                    velocity: changed(was.map(|o| o.velocity), new.velocity, same_bits),
                    body: changed(was.map(|o| [o.body]), [new.body], same_bits).map(|v| v[0]),
                    shield: changed(was.map(|o| [o.shield]), [new.shield], same_bits).map(|v| v[0]),
                };
                (change.mask() != 0).then_some(change)
            })
            .collect();
        SnapshotDelta {
            base: base.map(|base| base.tick),
            tick: self.tick,
            removed,
            changed,
        }
    }
}

impl ObjectChange {
    fn mask(&self) -> u8 {
        let mut mask = 0;
        let fields = [
            (self.definition.is_some(), DEFINITION),
            (self.position.is_some(), POSITION),
            (self.rotation.is_some(), ROTATION),
            (self.velocity.is_some(), VELOCITY),
            (self.body.is_some(), BODY),
            (self.shield.is_some(), SHIELD),
        ];
        for (present, bit) in fields {
            if present {
                mask |= bit;
            }
        }
        mask
    }
}

impl SnapshotDelta {
    /// The snapshot it brings `base` to; `None` if `base` is not the one
    /// it was made against, or a new object lacks fields.
    pub fn apply(&self, base: Option<&Snapshot>) -> Option<Snapshot> {
        if self.base != base.map(|base| base.tick) {
            return None;
        }
        let mut objects: Vec<ObjectState> = base
            .map(|base| base.objects.clone())
            .unwrap_or_default()
            .into_iter()
            .filter(|object| !self.removed.contains(&object.id))
            .collect();
        for change in &self.changed {
            match objects.binary_search_by_key(&change.id, |object| object.id) {
                Ok(at) => {
                    let object = objects.get_mut(at)?;
                    object.definition = change.definition.unwrap_or(object.definition);
                    object.position = change.position.unwrap_or(object.position);
                    object.rotation = change.rotation.unwrap_or(object.rotation);
                    object.velocity = change.velocity.unwrap_or(object.velocity);
                    object.body = change.body.unwrap_or(object.body);
                    object.shield = change.shield.unwrap_or(object.shield);
                }
                Err(at) => objects.insert(
                    at,
                    ObjectState {
                        id: change.id,
                        definition: change.definition?,
                        position: change.position?,
                        rotation: change.rotation?,
                        velocity: change.velocity?,
                        body: change.body?,
                        shield: change.shield?,
                    },
                ),
            }
        }
        Some(Snapshot {
            tick: self.tick,
            objects,
        })
    }

    pub(crate) fn write(&self, out: &mut Writer) -> Result<(), EncodeError> {
        out.u32(self.base.unwrap_or(u32::MAX));
        out.u32(self.tick);
        out.count(self.removed.len(), MAX_SNAPSHOT_OBJECTS)?;
        for &id in &self.removed {
            out.u32(id);
        }
        out.count(self.changed.len(), MAX_SNAPSHOT_OBJECTS)?;
        for change in &self.changed {
            out.u32(change.id);
            out.u8(change.mask());
            if let Some(definition) = change.definition {
                out.u16(definition);
            }
            if let Some(position) = change.position {
                out.f32s(&position);
            }
            if let Some(rotation) = change.rotation {
                out.f32s(&rotation);
            }
            if let Some(velocity) = change.velocity {
                out.f32s(&velocity);
            }
            if let Some(body) = change.body {
                out.f32(body);
            }
            if let Some(shield) = change.shield {
                out.f32(shield);
            }
        }
        Ok(())
    }

    pub(crate) fn read(input: &mut Reader) -> Result<Self, DecodeError> {
        let base = input.u32()?;
        let base = (base != u32::MAX).then_some(base);
        let tick = input.u32()?;
        let removed_count = input.count(MAX_SNAPSHOT_OBJECTS)?;
        let mut removed = Vec::with_capacity(removed_count);
        for _ in 0..removed_count {
            removed.push(input.u32()?);
        }
        let changed_count = input.count(MAX_SNAPSHOT_OBJECTS)?;
        let mut changed = Vec::with_capacity(changed_count);
        let mut last = None;
        for _ in 0..changed_count {
            let id = input.u32()?;
            // (in order, each once: what apply's binary search relies on)
            if last.is_some_and(|last| id <= last) {
                return Err(DecodeError::BadIndex);
            }
            last = Some(id);
            let mask = input.u8()?;
            if mask & !FIELDS != 0 || mask == 0 {
                return Err(DecodeError::BadValue);
            }
            let unit = |value: f32| {
                if (0.0..=1.0).contains(&value) {
                    Ok(value)
                } else {
                    Err(DecodeError::BadValue)
                }
            };
            changed.push(ObjectChange {
                id,
                definition: (mask & DEFINITION != 0).then(|| input.u16()).transpose()?,
                position: (mask & POSITION != 0).then(|| input.f32s()).transpose()?,
                rotation: (mask & ROTATION != 0).then(|| input.f32s()).transpose()?,
                velocity: (mask & VELOCITY != 0).then(|| input.f32s()).transpose()?,
                body: (mask & BODY != 0)
                    .then(|| input.f32().and_then(unit))
                    .transpose()?,
                shield: (mask & SHIELD != 0)
                    .then(|| input.f32().and_then(unit))
                    .transpose()?,
            });
        }
        Ok(Self {
            base,
            tick,
            removed,
            changed,
        })
    }
}
