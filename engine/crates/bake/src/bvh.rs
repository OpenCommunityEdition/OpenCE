//! A bounding-volume hierarchy of the scene's triangles, built by the
//! surface area heuristic over binned centroids, and flattened as the
//! GPU's path tracer reads it: 32 bytes a node, 48 a triangle.

use crate::scene::SceneTriangle;

/// The most triangles a leaf holds when a split would cost more.
const MAX_LEAF: usize = 8;
/// The bins the centroids are counted into along the split axis.
const BINS: usize = 16;
/// The cost of visiting a node, against one triangle test.
const TRAVERSAL_COST: f32 = 1.0;
/// The deepest the hierarchy goes (and the traversal's stack).
const MAX_DEPTH: usize = 64;

/// A node: its bounds, then either its children (`count` 0: nodes `first`
/// and `first + 1`) or its triangles (`count` of them from `first`).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BvhNode {
    /// Its least corner.
    pub min: [f32; 3],
    /// Its first child or first triangle.
    pub first: u32,
    /// Its greatest corner.
    pub max: [f32; 3],
    /// How many triangles; 0 for a node with children.
    pub count: u32,
}

/// A triangle, as the intersection test takes it: a corner and the edges
/// from it to the other two.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BvhTriangle {
    /// Its first corner.
    pub a: [f32; 3],
    /// Its material, in the scene's.
    pub material: u32,
    /// From the first corner to the second.
    pub ab: [f32; 3],
    /// From the first corner to the third.
    pub ac: [f32; 3],
}

/// A ray, from `origin` along `direction` as far as `t_max` times it.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Ray {
    /// Where it starts.
    pub origin: [f32; 3],
    /// Where it goes (need not be a unit vector).
    pub direction: [f32; 3],
    /// How far it goes, in lengths of `direction`.
    pub t_max: f32,
}

/// Where a ray met a triangle.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Hit {
    /// How far along the ray.
    pub t: f32,
    /// The triangle, in [`Bvh::triangles`].
    pub triangle: u32,
    /// The barycentric weights of the second and third corners.
    pub uv: [f32; 2],
}

/// The hierarchy: nodes, the root first, and the triangles in leaf order.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Bvh {
    /// Its nodes.
    pub nodes: Vec<BvhNode>,
    /// Its triangles, each leaf's together.
    pub triangles: Vec<BvhTriangle>,
}

/// Bounds: least corner, then greatest.
type Bounds = [[f32; 3]; 2];

const EMPTY: Bounds = [[f32::INFINITY; 3], [f32::NEG_INFINITY; 3]];

fn grow(bounds: &mut Bounds, point: [f32; 3]) {
    for axis in 0..3 {
        bounds[0][axis] = bounds[0][axis].min(point[axis]);
        bounds[1][axis] = bounds[1][axis].max(point[axis]);
    }
}

fn merge(bounds: &mut Bounds, other: &Bounds) {
    grow(bounds, other[0]);
    grow(bounds, other[1]);
}

/// Half the surface area of `bounds` (0 when empty).
fn area(bounds: &Bounds) -> f32 {
    let [x, y, z] = std::array::from_fn(|axis| (bounds[1][axis] - bounds[0][axis]).max(0.0));
    x * y + y * z + z * x
}

impl Bvh {
    /// The hierarchy of `triangles`.
    pub fn build(triangles: &[SceneTriangle]) -> Self {
        let boxes: Vec<Bounds> = triangles
            .iter()
            .map(|triangle| {
                let mut bounds = EMPTY;
                triangle
                    .positions
                    .iter()
                    .for_each(|&corner| grow(&mut bounds, corner));
                bounds
            })
            .collect();
        let centroids: Vec<[f32; 3]> = boxes
            .iter()
            .map(|bounds| std::array::from_fn(|axis| (bounds[0][axis] + bounds[1][axis]) / 2.0))
            .collect();
        let mut order: Vec<u32> = (0..triangles.len() as u32).collect();
        let mut nodes = vec![BvhNode {
            min: [0.0; 3],
            first: 0,
            max: [0.0; 3],
            count: 0,
        }];
        // (node, first triangle in `order`, how many, depth)
        let mut pending = vec![(0usize, 0usize, order.len(), 0usize)];
        while let Some((node, first, count, depth)) = pending.pop() {
            let mut bounds = EMPTY;
            let mut centre = EMPTY;
            for &triangle in &order[first..first + count] {
                merge(&mut bounds, &boxes[triangle as usize]);
                grow(&mut centre, centroids[triangle as usize]);
            }
            if count == 0 {
                bounds = [[0.0; 3]; 2];
            }
            nodes[node].min = bounds[0];
            nodes[node].max = bounds[1];
            let split = (count > 1 && depth < MAX_DEPTH)
                .then(|| {
                    split(
                        &order[first..first + count],
                        &boxes,
                        &centroids,
                        &bounds,
                        &centre,
                    )
                })
                .flatten()
                .filter(|split| split.cost < count as f32 || count > MAX_LEAF);
            let slice = &mut order[first..first + count];
            let left = match split {
                Some(split) => partition(slice, |&triangle| {
                    split.goes_left(centroids[triangle as usize], &centre)
                }),
                // (centroids all at one point: halve the triangles as they are)
                None if count > MAX_LEAF && depth < MAX_DEPTH => count / 2,
                None => {
                    nodes[node].first = first as u32;
                    nodes[node].count = count as u32;
                    continue;
                }
            };
            let left = if left == 0 || left == count {
                count / 2
            } else {
                left
            };
            let child = nodes.len();
            nodes[node].first = child as u32;
            nodes[node].count = 0;
            nodes.extend([nodes[node]; 2]);
            pending.push((child, first, left, depth + 1));
            pending.push((child + 1, first + left, count - left, depth + 1));
        }
        let triangles = order
            .iter()
            .map(|&index| {
                let triangle = &triangles[index as usize];
                let [a, b, c] = triangle.positions;
                BvhTriangle {
                    a,
                    material: triangle.material,
                    ab: std::array::from_fn(|axis| b[axis] - a[axis]),
                    ac: std::array::from_fn(|axis| c[axis] - a[axis]),
                }
            })
            .collect();
        Self { nodes, triangles }
    }

    /// The nearest triangle `ray` meets, if any.
    pub fn nearest(&self, ray: &Ray) -> Option<Hit> {
        let mut nearest: Option<Hit> = None;
        self.traverse(ray, |triangle, hit| {
            if nearest.is_none_or(|nearest| hit.t < nearest.t) {
                nearest = Some(Hit { triangle, ..hit });
            }
            false
        });
        nearest
    }

    /// Whether `ray` meets any triangle.
    pub fn occluded(&self, ray: &Ray) -> bool {
        let mut met = false;
        self.traverse(ray, |_, _| {
            met = true;
            true
        });
        met
    }

    /// The nodes as the GPU reads them: each its least corner, first,
    /// greatest corner and count, little-endian.
    pub fn node_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(self.nodes.len() * 32);
        for node in &self.nodes {
            put_floats(&mut bytes, &node.min);
            bytes.extend_from_slice(&node.first.to_le_bytes());
            put_floats(&mut bytes, &node.max);
            bytes.extend_from_slice(&node.count.to_le_bytes());
        }
        bytes
    }

    /// The triangles as the GPU reads them: each its corner and material,
    /// then its two edges, each padded to four words.
    pub fn triangle_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(self.triangles.len() * 48);
        for triangle in &self.triangles {
            put_floats(&mut bytes, &triangle.a);
            bytes.extend_from_slice(&triangle.material.to_le_bytes());
            put_floats(&mut bytes, &triangle.ab);
            bytes.extend_from_slice(&[0; 4]);
            put_floats(&mut bytes, &triangle.ac);
            bytes.extend_from_slice(&[0; 4]);
        }
        bytes
    }

    /// Visits each triangle `ray` meets, nearer nodes first, until `met`
    /// says to stop.
    fn traverse(&self, ray: &Ray, mut met: impl FnMut(u32, Hit) -> bool) {
        if self.triangles.is_empty() {
            return;
        }
        let inverse = ray.direction.map(|axis| 1.0 / axis);
        let mut t_max = ray.t_max;
        let mut stack = [0u32; 2 * MAX_DEPTH + 2];
        let mut depth = 1;
        while depth > 0 {
            depth -= 1;
            let node = &self.nodes[stack[depth] as usize];
            if node.count > 0 {
                for index in node.first..node.first + node.count {
                    let triangle = &self.triangles[index as usize];
                    if let Some(hit) = intersect(triangle, ray, t_max) {
                        if met(index, hit) {
                            return;
                        }
                        t_max = t_max.min(hit.t);
                    }
                }
                continue;
            }
            let near = |child: u32| {
                let child = &self.nodes[child as usize];
                enter(&[child.min, child.max], ray.origin, inverse, t_max)
            };
            let (left, right) = (node.first, node.first + 1);
            let (a, b) = (near(left), near(right));
            // (the farther pushed first, so the nearer is visited first)
            let mut push = |child: u32| {
                stack[depth] = child;
                depth += 1;
            };
            match (a, b) {
                (Some(a), Some(b)) if a <= b => {
                    push(right);
                    push(left);
                }
                (Some(_), Some(_)) => {
                    push(left);
                    push(right);
                }
                (Some(_), None) => push(left),
                (None, Some(_)) => push(right),
                (None, None) => {}
            }
        }
    }
}

/// Where a ray enters `bounds` before `t_max`, if it does (slab test).
fn enter(bounds: &Bounds, origin: [f32; 3], inverse: [f32; 3], t_max: f32) -> Option<f32> {
    let mut near = 0.0f32;
    let mut far = t_max;
    for axis in 0..3 {
        let a = (bounds[0][axis] - origin[axis]) * inverse[axis];
        let b = (bounds[1][axis] - origin[axis]) * inverse[axis];
        // (NaN, from a ray in the slab's plane, leaves the bound as it was)
        near = near.max(a.min(b));
        far = far.min(a.max(b));
    }
    (near <= far).then_some(near)
}

/// Where `ray` meets `triangle`, from either side, before `t_max`
/// (Möller and Trumbore).
pub fn intersect(triangle: &BvhTriangle, ray: &Ray, t_max: f32) -> Option<Hit> {
    let p = cross(ray.direction, triangle.ac);
    let determinant = dot(triangle.ab, p);
    if determinant.abs() < 1e-12 {
        return None;
    }
    let inverse = 1.0 / determinant;
    let to_origin: [f32; 3] = std::array::from_fn(|axis| ray.origin[axis] - triangle.a[axis]);
    let u = dot(to_origin, p) * inverse;
    if !(0.0..=1.0).contains(&u) {
        return None;
    }
    let q = cross(to_origin, triangle.ab);
    let v = dot(ray.direction, q) * inverse;
    if v < 0.0 || u + v > 1.0 {
        return None;
    }
    let t = dot(triangle.ac, q) * inverse;
    (t > 0.0 && t < t_max).then_some(Hit {
        t,
        triangle: 0,
        uv: [u, v],
    })
}

/// The best split of `triangles`, by the surface area heuristic.
struct Split {
    axis: usize,
    bin: usize,
    cost: f32,
}

impl Split {
    /// Whether a centroid goes to the left child.
    fn goes_left(&self, centroid: [f32; 3], centre: &Bounds) -> bool {
        bin_of(
            centroid[self.axis],
            centre[0][self.axis],
            centre[1][self.axis],
        ) <= self.bin
    }
}

/// The bin a centroid at `at` falls in, between `low` and `high`.
fn bin_of(at: f32, low: f32, high: f32) -> usize {
    let extent = high - low;
    if extent <= 0.0 {
        return 0;
    }
    (((at - low) / extent * BINS as f32) as usize).min(BINS - 1)
}

fn split(
    triangles: &[u32],
    boxes: &[Bounds],
    centroids: &[[f32; 3]],
    bounds: &Bounds,
    centre: &Bounds,
) -> Option<Split> {
    let parent = area(bounds);
    let mut best: Option<Split> = None;
    for axis in 0..3 {
        let (low, high) = (centre[0][axis], centre[1][axis]);
        if high - low <= 0.0 {
            continue;
        }
        let mut bins = [(EMPTY, 0usize); BINS];
        for &triangle in triangles {
            let bin = &mut bins[bin_of(centroids[triangle as usize][axis], low, high)];
            merge(&mut bin.0, &boxes[triangle as usize]);
            bin.1 += 1;
        }
        // (the left side's area and count up to each bin, then the right's)
        let mut left = [(0.0f32, 0usize); BINS];
        let mut sweep = (EMPTY, 0usize);
        for (bin, (bounds, count)) in bins.iter().enumerate() {
            merge(&mut sweep.0, bounds);
            sweep.1 += count;
            left[bin] = (area(&sweep.0), sweep.1);
        }
        let mut sweep = (EMPTY, 0usize);
        for bin in (1..BINS).rev() {
            merge(&mut sweep.0, &bins[bin].0);
            sweep.1 += bins[bin].1;
            let (left_area, left_count) = left[bin - 1];
            if left_count == 0 || sweep.1 == 0 {
                continue;
            }
            let cost = TRAVERSAL_COST
                + (left_area * left_count as f32 + area(&sweep.0) * sweep.1 as f32)
                    / parent.max(f32::MIN_POSITIVE);
            if best.as_ref().is_none_or(|best| cost < best.cost) {
                best = Some(Split {
                    axis,
                    bin: bin - 1,
                    cost,
                });
            }
        }
    }
    best
}

/// Puts the elements `left` says go left before the others; how many go
/// left.
fn partition<T>(slice: &mut [T], mut left: impl FnMut(&T) -> bool) -> usize {
    let mut boundary = 0;
    for index in 0..slice.len() {
        if left(&slice[index]) {
            slice.swap(boundary, index);
            boundary += 1;
        }
    }
    boundary
}

fn put_floats(bytes: &mut Vec<u8>, values: &[f32; 3]) {
    for value in values {
        bytes.extend_from_slice(&value.to_le_bytes());
    }
}

fn dot(a: [f32; 3], b: [f32; 3]) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

fn cross(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A repeatable sequence of numbers from 0 to 1.
    struct Random(u64);

    impl Random {
        fn next(&mut self) -> f32 {
            self.0 = self
                .0
                .wrapping_mul(6364136223846793005)
                .wrapping_add(1442695040888963407);
            (self.0 >> 40) as f32 / (1u64 << 24) as f32
        }

        fn point(&mut self, scale: f32) -> [f32; 3] {
            [0; 3].map(|_| (self.next() - 0.5) * scale)
        }
    }

    fn scattered(count: usize) -> Vec<SceneTriangle> {
        let mut random = Random(7);
        (0..count)
            .map(|index| {
                let centre = random.point(100.0);
                let corner = |random: &mut Random| {
                    let offset = random.point(6.0);
                    std::array::from_fn(|axis| centre[axis] + offset[axis])
                };
                SceneTriangle {
                    positions: [
                        corner(&mut random),
                        corner(&mut random),
                        corner(&mut random),
                    ],
                    material: index as u32,
                }
            })
            .collect()
    }

    fn brute_force(bvh: &Bvh, ray: &Ray) -> Option<Hit> {
        let mut nearest: Option<Hit> = None;
        for (index, triangle) in bvh.triangles.iter().enumerate() {
            if let Some(hit) = intersect(triangle, ray, ray.t_max) {
                if nearest.is_none_or(|nearest| hit.t < nearest.t) {
                    nearest = Some(Hit {
                        triangle: index as u32,
                        ..hit
                    });
                }
            }
        }
        nearest
    }

    #[test]
    fn the_hierarchy_finds_what_testing_every_triangle_finds() {
        let bvh = Bvh::build(&scattered(2000));
        let mut random = Random(11);
        let mut hits = 0;
        for _ in 0..500 {
            let ray = Ray {
                origin: random.point(120.0),
                direction: random.point(2.0),
                t_max: 200.0,
            };
            let expected = brute_force(&bvh, &ray);
            assert_eq!(bvh.nearest(&ray), expected);
            assert_eq!(bvh.occluded(&ray), expected.is_some());
            hits += usize::from(expected.is_some());
        }
        assert!(hits > 50, "only {hits} rays met a triangle");
    }

    #[test]
    fn every_triangle_is_in_one_leaf_inside_its_bounds() {
        let triangles = scattered(1000);
        let bvh = Bvh::build(&triangles);
        let mut materials: Vec<u32> = bvh.triangles.iter().map(|t| t.material).collect();
        materials.sort_unstable();
        assert_eq!(materials, (0..1000).collect::<Vec<_>>());
        let mut covered = vec![0; bvh.triangles.len()];
        let mut stack = vec![(0usize, 0usize)];
        let mut deepest = 0;
        while let Some((node, depth)) = stack.pop() {
            deepest = deepest.max(depth);
            let node = bvh.nodes[node];
            if node.count == 0 {
                for child in [node.first, node.first + 1] {
                    let inner = bvh.nodes[child as usize];
                    for axis in 0..3 {
                        assert!(inner.min[axis] >= node.min[axis]);
                        assert!(inner.max[axis] <= node.max[axis]);
                    }
                    stack.push((child as usize, depth + 1));
                }
                continue;
            }
            assert!(node.count as usize <= MAX_LEAF);
            for index in node.first..node.first + node.count {
                let triangle = bvh.triangles[index as usize];
                covered[index as usize] += 1;
                for corner in [
                    triangle.a,
                    add(triangle.a, triangle.ab),
                    add(triangle.a, triangle.ac),
                ] {
                    for (axis, value) in corner.into_iter().enumerate() {
                        assert!(value >= node.min[axis] - 1e-4);
                        assert!(value <= node.max[axis] + 1e-4);
                    }
                }
            }
        }
        assert!(covered.iter().all(|&count| count == 1));
        assert!(deepest < 30, "{deepest} deep");
    }

    fn add(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
        std::array::from_fn(|axis| a[axis] + b[axis])
    }

    #[test]
    fn a_ray_meets_a_triangle_from_either_side_and_not_beyond_its_reach() {
        let triangle = SceneTriangle {
            positions: [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            material: 3,
        };
        let bvh = Bvh::build(&[triangle]);
        let down = Ray {
            origin: [0.25, 0.25, 2.0],
            direction: [0.0, 0.0, -1.0],
            t_max: 10.0,
        };
        let hit = bvh.nearest(&down).unwrap_or_else(|| panic!("missed"));
        assert_eq!((hit.t, hit.triangle, hit.uv), (2.0, 0, [0.25, 0.25]));
        let up = Ray {
            origin: [0.25, 0.25, -1.0],
            direction: [0.0, 0.0, 1.0],
            t_max: 10.0,
        };
        assert!(bvh.occluded(&up));
        assert!(!bvh.occluded(&Ray { t_max: 0.5, ..up }));
        assert!(!bvh.occluded(&Ray {
            origin: [0.75, 0.75, 2.0],
            ..down
        }));
        // (along its plane: no hit)
        assert!(!bvh.occluded(&Ray {
            origin: [-1.0, 0.25, 0.0],
            direction: [1.0, 0.0, 0.0],
            t_max: 10.0,
        }));
        assert_eq!(Bvh::build(&[]).nearest(&down), None);
    }

    #[test]
    fn identical_triangles_still_split_into_small_leaves() {
        let triangle = SceneTriangle {
            positions: [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            material: 0,
        };
        let bvh = Bvh::build(&vec![triangle; 100]);
        assert!(bvh.nodes.iter().all(|node| node.count as usize <= MAX_LEAF));
        assert_eq!(bvh.triangles.len(), 100);
    }

    #[test]
    fn the_gpu_s_layout() {
        let bvh = Bvh::build(&scattered(20));
        let nodes = bvh.node_bytes();
        let triangles = bvh.triangle_bytes();
        assert_eq!(nodes.len(), bvh.nodes.len() * 32);
        assert_eq!(triangles.len(), 20 * 48);
        let word = |bytes: &[u8], at: usize| {
            u32::from_le_bytes([bytes[at], bytes[at + 1], bytes[at + 2], bytes[at + 3]])
        };
        assert_eq!(word(&nodes, 12), bvh.nodes[0].first);
        assert_eq!(word(&nodes, 28), bvh.nodes[0].count);
        assert_eq!(f32::from_bits(word(&nodes, 16)), bvh.nodes[0].max[0]);
        assert_eq!(word(&triangles, 48 + 12), bvh.triangles[1].material);
        assert_eq!(f32::from_bits(word(&triangles, 32)), bvh.triangles[0].ac[0]);
    }
}
