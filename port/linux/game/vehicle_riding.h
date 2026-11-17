/*
VEHICLE_RIDING.H

Players carried by the vehicle they stand on (vehicle_riding.c).
*/

#ifndef VEHICLE_RIDING_H
#define VEHICLE_RIDING_H

enum
{
	/* the window: the ticks a biped stays tied to a vehicle it stopped
	standing on, counted the tick it leaves and on the ground off the deck
	(in the air it holds) */
	RIDING_TICKS = 60,
	/* the touches by different vehicles a biped's own tick resolves */
	RIDING_CONTACT_RECORDS = 4,
};

/* how a vehicle's collision treats a biped (vehicle_riding_collision_window,
vehicle_riding_collision_contact) */
enum vehicle_riding_collision
{
	_vehicle_riding_collision_original,
	_vehicle_riding_collision_ignore,
	/* (judged on the contact: vehicle_riding_collision_contact) */
	_vehicle_riding_collision_judge,
	/* (recorded for the biped's own tick: no push, fix or damage during the
	vehicle's update) */
	_vehicle_riding_collision_resolved,
};

/* accumulated netcode writes, separate from a vehicle's own motion */
struct vehicle_riding_correction
{
	long vehicle_index;
	real_quaternion rotation;
	real_vector3d translation;
};

enum vehicle_riding_role
{
	_vehicle_riding_role_simulated,
	_vehicle_riding_role_followed,
};

struct vehicle_riding_mark
{
	short role;
	long time;
	struct vehicle_riding_correction correction;
};

void vehicle_riding_correction_identity(struct vehicle_riding_correction *correction, long vehicle_index);
void vehicle_riding_correction_compose(struct vehicle_riding_correction *correction,
	real_point3d const *before_position, real_vector3d const *before_forward, real_vector3d const *before_up,
	real_point3d const *after_position, real_vector3d const *after_forward, real_vector3d const *after_up);
void vehicle_riding_correction_since(struct vehicle_riding_correction const *now,
	struct vehicle_riding_correction const *then, real_matrix4x3 *since);
struct vehicle_riding_correction const *vehicle_riding_vehicle_correction(long vehicle_index);
void vehicle_riding_vehicle_corrected(long vehicle_index,
	real_point3d const *before_position, real_vector3d const *before_forward, real_vector3d const *before_up,
	real_point3d const *after_position, real_vector3d const *after_forward, real_vector3d const *after_up);
struct vehicle_riding_mark *vehicle_riding_player_mark(short player_index);

/* the host's claim, kept after its grace so a local node test cannot undo it */
struct vehicle_riding_claim
{
	long vehicle_index;
	short node_index;
	short disagreement;
	boolean taken;
};

struct vehicle_riding_claim *vehicle_riding_player_claim(short player_index);
boolean vehicle_riding_claim_grace(struct vehicle_riding_claim *claim, long own_vehicle,
	short own_node, long host_vehicle, short host_node, short grace);

/* a biped's riding fields (bipeds.h), copied in and out by vehicle_riding_bipeds.c */
struct vehicle_riding_state
{
	char ticks;
	char falling;
	long object_index;
	long vehicle_updates;
	real_point3d vehicle_position;
	real_vector3d vehicle_forward;
	real_vector3d vehicle_up;
	real_vector3d velocity;
	/* (where it last stood, in the frame of the vehicle's node it stood on:
	along the node's forward, its left and its up from its position) */
	real_point3d local_point;
	short node_index;
	/* (the vehicle's own pose then, to tell a node that moves with the body
	from one that moves on its own) */
	real_point3d body_position;
	real_vector3d body_forward;
	real_vector3d body_up;
	struct vehicle_riding_mark mark;
};

/* a vehicle as it is now: the pose of the node a biped stands on (the
vehicle's own with no node), and the vehicle's own motion */
struct vehicle_riding_vehicle
{
	long object_index;
	short node_index;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_point3d origin;
	/* (the body's orientation, at its origin) */
	real_vector3d body_forward;
	real_vector3d body_up;
	long updates;
	real_vector3d translational_velocity;
	real_vector3d angular_velocity;
	real_point3d bounding_sphere_center;
	real bounding_sphere_radius;
	boolean has_player;
	boolean node_kept;
	short role;
	long time;
	struct vehicle_riding_correction correction;
};

/* a tick's carry, from the snapshot to the vehicle now */
struct vehicle_riding_step
{
	struct vehicle_riding_vehicle vehicle;
	long updates;
	/* (the player ticks the vehicle's updates since the snapshot span: 0 on
	double speed's skipped tick) */
	real player_ticks;
	real_vector3d displacement;
	real dyaw;
	real_vector3d velocity_now;
	real_vector3d tracked_deck_velocity;
	/* (the body's own motion at the base, on whatever node it stands: the
	frame a biped keeps when it leaves the floor) */
	real_vector3d body_deck_velocity;
	/* (the window's ray this tick, set by vehicle_riding_bipeds.c before the biped's
	physics: the vehicle it left is under it within reach, upright) */
	boolean over_deck;
	/* (the node moved faster than the body carries a rider, or not as a
	turret does: this tick's carry is the body's) */
	boolean node_rejected;
	/* (the ridden node's motion on the body a player tick, when one was
	judged: its origin's shift and its up's tilt in radians; else 0) */
	real node_shift;
	real node_tilt;
};

/* whether the vehicle a biped left encloses it, decided once a jump
(vehicle_riding_air_enclosure) */
enum vehicle_riding_enclosure
{
	_vehicle_riding_enclosure_unknown,
	_vehicle_riding_enclosure_open,
	_vehicle_riding_enclosure_enclosed,
};

/* a player's air frame beyond its biped's riding fields, never
saved: the most the air frame's speed has been since the take-off, and the
enclosure verdict of this jump */
struct vehicle_riding_air
{
	boolean have_peak;
	/* an own correction holds this jump's host ceiling */
	boolean host_peak;
	boolean host_verdict;
	real peak;
	short enclosure;
};

enum
{
	/* debug.riding_enclosure_grace_ticks: the standing samples a cabin's
	verdict outlasts missing, by default and at most */
	RIDING_CABIN_GRACE = 4,
	RIDING_CABIN_GRACE_MAXIMUM = 30,
};

/* which sample a player's tick takes of its cabin (vehicle_riding_cabin_phase) */
enum vehicle_riding_cabin_sample
{
	_vehicle_riding_cabin_sample_none,
	_vehicle_riding_cabin_sample_standing,
	_vehicle_riding_cabin_sample_jump,
};

/* how a cabin rider's place settled (vehicle_riding_cabin_recover) */
enum vehicle_riding_cabin_settle
{
	_vehicle_riding_cabin_settle_none,
	_vehicle_riding_cabin_settle_anchor,
	/* (two widths out along the vehicle's touch) */
	_vehicle_riding_cabin_settle_candidate,
	/* (collision_fix_pill's offset of the place) */
	_vehicle_riding_cabin_settle_fixed,
	/* (collision_fix_pill's offset of the anchor, the anchor blocked) */
	_vehicle_riding_cabin_settle_around,
	/* (out through an opening: let go) */
	_vehicle_riding_cabin_settle_door,
	/* (no valid place: left where it was, logged) */
	_vehicle_riding_cabin_settle_failed,
};

/* the two-sided cabin test at a pill (vehicle_riding_bipeds.c's biped_riding_cabin_rays):
whether there was a model to test, the end nearer the ceiling inside it, the
ceiling ray's hit and distance, the floor ray's, and the verdict */
struct vehicle_riding_cabin_rays
{
	boolean valid;
	boolean inside;
	boolean ceiling;
	real ceiling_distance;
	boolean floor;
	real floor_distance;
	boolean hit;
};

/* a player's cabin beyond its biped's riding fields, never saved: the unit
and vehicle of the window it is of, the standing verdict and the misses
since its last hit, the protected jump's own raw sample, the rays' axis
(TRUE: the vehicle's up), a jump taken this tick, the last interior base in
the vehicle body's frame, this tick's protection; and on a client the host's
verdict for this unit, window and vehicle, for one phase (an enum
vehicle_riding_cabin_sample: standing or jump), which the local samples no
longer change */
struct vehicle_riding_cabin
{
	long unit_index;
	long vehicle_index;
	short standing;
	short misses;
	short jump;
	boolean host;
	boolean host_legacy;
	short host_verdict;
	short host_phase;
	boolean body_axis;
	boolean take_off;
	boolean have_anchor;
	real_point3d anchor;
	boolean exception;
	boolean failure_logged;
	long failure_log_tick;
	long failure_seen_tick;
};

/* the deck part's flags (struct vehicle_riding_deck): falling, the
enclosure verdict (an enum vehicle_riding_enclosure) in the two bits above
it, and above those whether a verdict in the air is cabin evidence (a
protected jump's sample, or the host's word for it) rather than the legacy
air verdict; bits 4 to 7 stay clear */
enum
{
	_vehicle_riding_deck_falling_bit = 0,
	VEHICLE_RIDING_DECK_VERDICT_SHIFT = 1,
	_vehicle_riding_deck_cabin_bit = 3,
};
#define VEHICLE_RIDING_DECK_VERDICT_MASK 3

/* a player's riding state on the wire (network_distributed.c's deck part):
the vehicle (the host's salted datum index) and node it is in the window
of, the pill's base now in that node's frame, its ticks and flags, and
riding (RIDING_TICKS) its velocity relative to the deck, in the air its air
frame (world, a player tick) and its take-off speed */
struct vehicle_riding_deck
{
	long vehicle_index;
	short node_index;
	char ticks;
	byte flags;
	real_point3d place;
	real_vector3d velocity;
	real peak;
};

enum vehicle_riding_host_path
{
	_vehicle_riding_host_world,
	_vehicle_riding_host_deck,
	_vehicle_riding_host_along,
};

short vehicle_riding_host_path(struct vehicle_riding_state const *host, boolean deck_present,
	struct vehicle_riding_deck const *deck, boolean transition);
real vehicle_riding_transition_leash(void);
boolean vehicle_riding_host_leash(real_point3d const *client_base, real_point3d const *host_base, real leash);
boolean vehicle_riding_transition_agreed(short host_before, long host_vehicle_before, short host_now, long host_vehicle,
	short client_ticks, long client_vehicle, boolean client_part, boolean word_taken);
boolean vehicle_riding_word_on_air_deck(short host_ticks, long host_vehicle, short client_ticks, long client_vehicle);
void vehicle_riding_host_target(short path, struct vehicle_riding_vehicle const *client_node,
	struct vehicle_riding_vehicle const *host_node, real_point3d const *host_base,
	struct vehicle_riding_deck const *deck, real_point3d const *client_base, real_point3d *base);
void vehicle_riding_host_unlifted(struct vehicle_riding_vehicle const *host_node, real_point3d const *host_base, real_point3d *base);
void vehicle_riding_host_velocity(real_vector3d const *relative, real_vector3d const *frame_velocity,
	real speed, real fall_speed, real_vector3d *world);

/* a vehicle's touch on a biped, recorded during the vehicle's update for
the biped's own tick: the way out of the vehicle there; deferred: the
vehicle's update left it to an enclosed rider's cabin (never the original
push or hurt, whatever the rider's own tick then finds) */
struct vehicle_riding_contact
{
	long vehicle_index;
	real_vector3d normal;
	boolean deferred;
};

/* how a biped a resolved touch moved settles (vehicle_riding_contact_settle) */
enum vehicle_riding_settle
{
	_vehicle_riding_settle_moved,
	_vehicle_riding_settle_candidate,
	_vehicle_riding_settle_fixed,
	_vehicle_riding_settle_crushed,
};

/* the settings, taken at a map's start, a game's start and every load;
whether riding is on */
boolean vehicle_riding_latch(void);
/* the map's choice where it made one, else the decision stored at the map's start (a local game's, or the host's
in a network game); never in film playback or the editor */
boolean vehicle_riding_enabled(void);
/* fixes the game's decision for the map that starts, once per map (vehicle_riding_objects_reset) */
void vehicle_riding_map_decide(void);
/* whether a game rides, below a map's own choice: decided on the host (or locally) at a map's start */
boolean vehicle_riding_decide(boolean multiplayer, boolean gametype_rule, boolean host_option, short entry_rule,
	boolean dedicated, boolean override_gametype);
/* the map's choice, saved with the game and authoritative on the host */
void vehicle_riding_map_initialize(void);
void vehicle_riding_map_reset(void);
void vehicle_riding_map_script(boolean on);
short vehicle_riding_map(void);
void vehicle_riding_map_received(short map);
/* the latched debug.riding_ values the host sends its clients (the leash
stays the host's own) */
struct distributed_riding_latched
{
	real damage_speed;
	real teleport_step;
	real moved_alone;
	real air_grip;
	word look_carry;
	word cabin_grace;
};
void vehicle_riding_latched_get(struct distributed_riding_latched *latched);
/* a client takes the host's values for this game, through the latch's checks */
void vehicle_riding_latched_received(struct distributed_riding_latched const *host);
void vehicle_riding_latched_forget(void);
void vehicle_riding_map_start(void);
/* whether a biped rides at all: a living player's, riding on */
boolean vehicle_riding_applies(boolean has_player, boolean dead);
/* whether a biped updates after the main loop (objects_update_passes) */
boolean vehicle_riding_deferred(struct vehicle_riding_state const *state, boolean applies);

void vehicle_riding_clear(struct vehicle_riding_state *state);
/* ends riding; a biped still in the air keeps the frame it fell from */
void vehicle_riding_drop(struct vehicle_riding_state *state);
boolean vehicle_riding_is_riding(struct vehicle_riding_state const *state);

/* a world point in a pose's own frame (along its forward, its left and its
up from its origin), and back */
void vehicle_riding_point_to_local(real_point3d const *origin, real_vector3d const *forward, real_vector3d const *up,
	real_point3d const *point, real_point3d *local);
void vehicle_riding_point_to_world(real_point3d const *origin, real_vector3d const *forward, real_vector3d const *up,
	real_point3d const *local, real_point3d *point);
/* the turn from one basis to another about global up, in radians */
real vehicle_riding_yaw(real_vector3d const *from_forward, real_vector3d const *from_up,
	real_vector3d const *to_forward, real_vector3d const *to_up);
/* a vector turned about global up */
void vehicle_riding_turn_vector(real_vector3d *vector, real angle);
/* the same turn for several vectors, its sine and cosine taken once */
void vehicle_riding_turn_vectors(real_vector3d *const *vectors, short count, real dyaw);
/* the vehicle body's motion at a point, per player tick */
void vehicle_riding_deck_point_velocity(struct vehicle_riding_vehicle const *vehicle, real_point3d const *point,
	boolean double_speed, real_vector3d *velocity);

/* the deck's motion since the biped's last tick (NULL vehicle: gone);
FALSE when riding was dropped, or a rider something else moved off its
spot was put in the window. A node is a deck only while it moves on the
body as a turret does (its origin and its up put, RIDING_NODE_SHIFT and
RIDING_NODE_TILT a tick) and carries the rider's spot on the body slower
than the speed limit a tick: else the carry is the body's */
boolean vehicle_riding_snapshot_step(struct vehicle_riding_state *state, struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base, real pill_height, real pill_width, boolean double_speed, real node_speed_limit,
	struct vehicle_riding_step *step);
/* where a biped in the window of a vehicle is over that vehicle's deck now:
its base carried by the deck's own motion since its last tick (the
snapshot's), netcode writes left out as the step leaves them out; the base
itself for another vehicle or none, and on a tick whose frame the step
holds (a change of role, a followed vehicle's tick with no time or with
the host's snap) */
void vehicle_riding_window_base(struct vehicle_riding_state const *state, struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base, real_point3d *origin);
/* retake the same vehicle and node at the current base without changing
the window or its frame velocity; FALSE leaves the state untouched */
boolean vehicle_riding_rebase(struct vehicle_riding_state *state, struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base);
/* a client's copy takes the host's window at its post-blend base, with
this machine's frame velocity; FALSE leaves the state untouched */
boolean vehicle_riding_take_deck(struct vehicle_riding_state *state, struct vehicle_riding_vehicle const *vehicle,
	struct vehicle_riding_deck const *deck, real_point3d const *base);

/* a local player's view turned by its vehicle this tick (the impulse given);
0 when it is not carried */
real vehicle_riding_look(short local_player_index, boolean carried, real dyaw);
/* that turn, for the direct camera */
real vehicle_riding_kept_dyaw(short local_player_index);

/* the velocity the physics starts from: in the deck's frame while riding
(the frame velocity taken off), else the world's */
void vehicle_riding_seed(struct vehicle_riding_state const *state, real_vector3d const *world_velocity,
	real_vector3d *velocity, real_vector3d *frame_velocity);
/* the velocity of what a biped lands on: the ridden deck's frame, the
tracked deck's, another vehicle's at the contact, or none */
void vehicle_riding_support_velocity(long support_object_index, long riding_object_index,
	real_vector3d const *riding_frame_velocity, long tracked_object_index, real_vector3d const *tracked_deck_velocity,
	real_vector3d const *other_velocity, real_vector3d *support_velocity);
/* the landing velocity against what is landed on */
real vehicle_riding_landing_velocity(real_vector3d const *velocity, real_vector3d const *frame_velocity,
	real_vector3d const *support_velocity, real_vector3d const *normal);
/* what the biped stands on now, after its physics (step NULL: in no window;
support NULL: not a vehicle); leaving the floor, its window is the vehicle
body's, counted down then and on the ground off the deck, held in the air.
TRUE: clear its elevator fields */
boolean vehicle_riding_record(struct vehicle_riding_state *state, struct vehicle_riding_step const *step,
	struct vehicle_riding_vehicle const *support, boolean supported, real_vector3d const *support_velocity,
	real_vector3d const *support_normal, real_point3d const *new_base, boolean suspended,
	real_vector3d *new_velocity);
/* the vertical velocity the falling kill judges */
real vehicle_riding_falling_velocity(struct vehicle_riding_state const *state, real world_velocity_k);
/* the velocity a jump is taken above */
void vehicle_riding_jump_frame(struct vehicle_riding_state const *state, real_vector3d *jump_frame);
/* how far down a biped in the window looks for the deck it left: its own
jump's height plus a pill height */
real vehicle_riding_reach(real jump_velocity, real gravity, real height);
/* whether a ray down from the biped's base found that deck: within reach,
the vehicle upright (within 45 degrees of world up) */
boolean vehicle_riding_over_deck(boolean hit, real distance, real reach, real_vector3d const *body_up);
/* a rider's velocity relative to its deck, turned with the deck this tick
(step NULL, or in the window: not turned) */
void vehicle_riding_turn_relative(struct vehicle_riding_state const *state, struct vehicle_riding_step const *step,
	real_vector3d *velocity);
/* the window's air frame (state->velocity) brought toward the deck's
velocity at the biped's spot while it is airborne over the deck: a slowing
along the way it goes followed at once, so it is never ahead of the deck;
a turn, and a speed-up back to the most it went since the take-off (any
speed-up when the vehicle encloses the biped), by at most
debug.riding_air_grip a player tick; the biped's velocity given the same
change. air_frame: what its air control is relative to (zero: none); air
is the unit's own (NULL: no frame) */
void vehicle_riding_air_frame(struct vehicle_riding_state *state, struct vehicle_riding_step const *step,
	struct vehicle_riding_air *air, boolean airborne, real_vector3d *velocity, real_vector3d *air_frame);
/* a unit's air, by its absolute index (NULL: no player, or past the
player table) */
struct vehicle_riding_air *vehicle_riding_player_air(short player_index);
/* a remote unit's air from its deck part: the peak and verdict of the jump
the host has it in (cleared while it rides) */
void vehicle_riding_air_take(struct vehicle_riding_air *air, struct vehicle_riding_deck const *deck);
boolean vehicle_riding_air_take_verdict(struct vehicle_riding_air *air, struct vehicle_riding_deck const *deck,
	boolean current_episode);
/* an own air frame keeps the speed-up since the echoed tick */
void vehicle_riding_air_correct(struct vehicle_riding_state *state, struct vehicle_riding_air *air,
	real_vector3d const *difference, real peak, short verdict, real_vector3d *world_velocity);
/* the air of this tick, after the step and the window's ray: cleared while
the biped rides or has no window, its verdict ended when it is not
airborne over the deck. TRUE: decide the verdict now
(vehicle_riding_air_enclosure), the first airborne tick over the deck of a
jump */
boolean vehicle_riding_air_begin(struct vehicle_riding_air *air, struct vehicle_riding_state const *state,
	struct vehicle_riding_step const *step, boolean airborne);
/* the verdict: a ray up from the pill's top hit the vehicle within the
biped's jump height, or the top is inside it */
void vehicle_riding_air_enclosure(struct vehicle_riding_air *air, boolean hit, real distance, real jump_height,
	boolean inside);
/* the node a deck part's place is made in: the vehicle as it is now, but for a rider on its deck its pose as of the
rider's snapshot, the one the rider's next carry keeps it in */
void vehicle_riding_deck_frame(struct vehicle_riding_state const *state, struct vehicle_riding_vehicle const *vehicle,
	struct vehicle_riding_vehicle *frame);
/* the deck part of a biped's riding state now (base: its pill's; vehicle:
the selected snapshot frame of the node it is in the window of; world_velocity: its own;
air: its player's, NULL for none; cabin: its player's, NULL for none,
the verdict the rider goes by, riding as well as in the air
(vehicle_riding_cabin_verdict)): FALSE outside the window of that
vehicle, or with a value the wire cannot carry, the part then left as it
was */
boolean vehicle_riding_deck_make(struct vehicle_riding_state const *state, struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base, real_vector3d const *world_velocity, struct vehicle_riding_air const *air,
	struct vehicle_riding_cabin const *cabin, struct vehicle_riding_deck *deck);
/* whether a vehicle index from the wire names a datum: not NONE, and its
identifier not 0 (a lookup by 0 takes any datum in the slot) */
boolean vehicle_riding_deck_vehicle_named(long vehicle_index);
/* whether a deck part's values are whole: its vehicle named, a window's
ticks (1 to RIDING_TICKS), no flag but falling and a verdict of at most
enclosed, its place, velocity and peak finite and the peak not below 0 */
boolean vehicle_riding_deck_values_valid(struct vehicle_riding_deck const *deck);
/* ... and its node NONE or one of the vehicle's node_count (NONE: the
vehicle is not there) */
boolean vehicle_riding_deck_valid(struct vehicle_riding_deck const *deck, short node_count);

/* how the vehicle's collision treats a biped: as the original, not at all
(it rides it), or judged on the contact (in its window, or riding beside
it; vehicle_riding_collision_contact) */
short vehicle_riding_collision_window(struct vehicle_riding_state const *state, boolean applies, long vehicle_index);
/* a contact the window left to judge, during the vehicle's update: closing
faster than the damage speed along the push from the vehicle's centre or
along the touch's normal, the original game's hit, never for the vehicle
under a biped in its window; else resolved, in the biped's own tick, along
normal (the contact plane's, contact_normal, else from the vehicle's
centre to the biped's) */
short vehicle_riding_collision_contact(struct vehicle_riding_state const *state, long vehicle_index, boolean over_deck,
	real_point3d const *vehicle_center, real_point3d const *biped_center, real_vector3d const *contact_normal,
	real_vector3d const *deck_velocity, real damage_speed, real_vector3d const *biped_velocity, real_vector3d *normal);
/* a resolved touch kept for a player's own tick, by the player's absolute
index: one a vehicle, up to RIDING_CONTACT_RECORDS; FALSE: not kept (no
such player, or a further vehicle, which waits for its next touch) */
boolean vehicle_riding_contact_record(short player_index, long vehicle_index, real_vector3d const *normal);
/* a player's recorded touch of this vehicle marked deferred to the cabin (kept
until the touch is taken); FALSE: none recorded */
boolean vehicle_riding_contact_defer(short player_index, long vehicle_index);
/* a player's touches of this tick, taken; how many */
short vehicle_riding_contact_take(short player_index, struct vehicle_riding_contact *contacts);
/* every player's touches forgotten: a game tick's start, and the latch */
void vehicle_riding_contacts_clear(void);
/* how far a touch moves a biped out along normal in its own tick (move,
and its length): the vehicle's motion at base over its update beyond the
biped's own (a rider's carry, else its velocity without its fall over
those ticks), never back toward it; ahead: the part of the biped's own
motion its physics has still to make */
real vehicle_riding_contact_move(struct vehicle_riding_step const *step, boolean carried,
	struct vehicle_riding_vehicle const *vehicle, real_point3d const *base, real_vector3d const *biped_velocity,
	real_vector3d const *normal, boolean double_speed, real_vector3d *move, real_vector3d *ahead);
/* where a biped a touch moved settles: where the move put it, while it no
longer touches that vehicle once its own motion ahead is made; else one
spot twice its width further along the touch; else where
collision_fix_pill puts it; pinned, the original crush */
short vehicle_riding_contact_settle(boolean touching, boolean candidate_touching, boolean fixed);
/* whether a vehicle's state correction carries the biped it newly touches (as a
deck rider is carried by every write): it is in no window or in this vehicle's,
the unwritten hull did not touch it, the written one does, and the write moved
its base no further than a move that is still continuous */
boolean vehicle_riding_write_carries(struct vehicle_riding_state const *state, long vehicle_index, boolean touched_before,
	boolean touching, real distance);
/* the biped's own run and jump speed a tick, shared with the host's bound;
a speed not a number is none */
real vehicle_riding_run_jump_speed(boolean have_information, real run_forward, real run_backward, real run_sideways,
	boolean double_speed, real double_speed_multiplier, real player_speed_multiplier, real jump_velocity);
/* debug.riding_damage_speed, or when 0, twice that speed */
real vehicle_riding_damage_speed(boolean have_information, real run_forward, real run_backward, real run_sideways,
	boolean double_speed, real double_speed_multiplier, real player_speed_multiplier, real jump_velocity);

/* which of a sweep's hits carries the biped: the vehicle it rides while
any hit on it is steep enough to stand on, else the physics' best */
short vehicle_riding_claim(long riding_object_index, short count, long const *objects, real const *normal_ks,
	real minimum_normal_k, short best);

/* a player's cabin, by its absolute index (NULL: past the player table) */
struct vehicle_riding_cabin *vehicle_riding_player_cabin(short player_index);
void vehicle_riding_cabin_clear(struct vehicle_riding_cabin *cabin);
/* whether a player's tables hold what a tick of a biped riding cannot apply to leaves them: a cleared state's
mark, no contact recorded, the air a new jump starts from and a cleared cabin (that tick then has nothing to
write there; an unknown player has none) */
boolean vehicle_riding_player_idle(short player_index);
/* whether two reals are the same to the bit: a -0 is not the +0 a clear writes, and an idle check may not take it
for one (a saved game holds the bits) */
boolean vehicle_riding_real_same(real a, real b);
/* a recovery log: 1 at failure's start and once a second, -1 when the final
check ends it, 0 otherwise; the window's reset clears this history too */
short vehicle_riding_cabin_log(struct vehicle_riding_cabin *cabin, short settle, long tick, boolean final);
/* the cabin at a tick's start: cleared outside a valid window, or for
another unit or vehicle, then keyed to this one's */
void vehicle_riding_cabin_begin(struct vehicle_riding_cabin *cabin, long unit_index,
	struct vehicle_riding_state const *state, boolean valid_window);
/* after a tick's recording: keyed to the vehicle it rides; TRUE: sample it
now, at its final base (its standing verdict unknown) */
boolean vehicle_riding_cabin_acquire(struct vehicle_riding_cabin *cabin, long unit_index,
	struct vehicle_riding_state const *state);
/* a jump from the deck it rides, for its next tick */
void vehicle_riding_cabin_take_off(struct vehicle_riding_cabin *cabin, struct vehicle_riding_state const *state,
	long unit_index);
/* debug.riding_enclosure_grace_ticks, as latched */
short vehicle_riding_cabin_grace(void);
/* the up a tick's cabin is judged by: the vehicle body's */
real_vector3d const *vehicle_riding_cabin_up(struct vehicle_riding_step const *step);
/* the total tilt's cosine of an up (1 for none or not a number) */
real vehicle_riding_cabin_cosine(real_vector3d const *body_up);
/* whether a vehicle is tilted past where protection starts: its up's
cosine under the biped's surface limit or 45 degrees, the steeper */
boolean vehicle_riding_cabin_tilted(real_vector3d const *body_up, real minimum_normal_k);
/* the rays' axis, its latch moved: world up, the vehicle's up past the
surface limit until it is back a band inside it */
void vehicle_riding_cabin_axis(struct vehicle_riding_cabin *cabin, real_vector3d const *body_up, real minimum_normal_k,
	real_vector3d *axis);
/* ... as the latch has it, unmoved (a recovery candidate's test) */
void vehicle_riding_cabin_current_axis(struct vehicle_riding_cabin const *cabin, real_vector3d const *body_up,
	real_vector3d *axis);
/* where the ceiling's ray (and the inside test) and the floor's start */
void vehicle_riding_cabin_ends(real_point3d const *base, real height, real_vector3d const *axis,
	real_point3d *ceiling_start, real_point3d *floor_start);
/* the two-sided verdict of a test's rays */
boolean vehicle_riding_cabin_hit(struct vehicle_riding_cabin_rays const *rays, real jump_height, real height);
/* whether a segment's hit crosses a surface: inside it, never at an end */
boolean vehicle_riding_cabin_crossing(boolean hit, real t);
/* which sample this tick takes (step NULL: no valid window) */
short vehicle_riding_cabin_phase(struct vehicle_riding_cabin const *cabin, struct vehicle_riding_state const *state,
	struct vehicle_riding_step const *step, boolean airborne, boolean tilted);
/* a standing sample, with its grace */
void vehicle_riding_cabin_standing(struct vehicle_riding_cabin *cabin, boolean hit, short grace);
/* a jump's raw sample, kept for the jump */
void vehicle_riding_cabin_jump(struct vehicle_riding_cabin *cabin, boolean hit);
/* the host's verdict for this unit's window of a vehicle, for the phase its
part was in (an enum vehicle_riding_cabin_sample; cabin_evidence: its part
carried the cabin bit): taken only for the window this machine is in now
(current_window: the caller says whether the word is of it), and in a jump
only as cabin evidence; FALSE with the host's verdict dropped when it says
none (unknown, or a jump's legacy verdict) and with it kept when the word
is not of this window */
boolean vehicle_riding_cabin_take_host(struct vehicle_riding_cabin *cabin, long unit_index,
	struct vehicle_riding_state const *state, long vehicle_index, short verdict, short phase, boolean cabin_evidence,
	boolean current_window);
/* no host verdict (its part absent or not whole) */
void vehicle_riding_cabin_drop_host(struct vehicle_riding_cabin *cabin);
/* the verdict a rider goes by (an enum vehicle_riding_enclosure): on the
deck the standing one, in a jump its own sample; else, unless only cabin
evidence counts, the legacy air verdict */
short vehicle_riding_cabin_verdict(struct vehicle_riding_cabin const *cabin, struct vehicle_riding_state const *state,
	struct vehicle_riding_air const *air, boolean cabin_evidence);
/* whether a rider's tilted vehicle protects it: eligible, in a valid
window of it, enclosed by cabin evidence, tilted */
boolean vehicle_riding_cabin_exception(boolean eligible, boolean valid_window, boolean enclosed,
	real_vector3d const *body_up, real minimum_normal_k);
/* the vehicle an enclosed rider's steep contacts carry it on this tick:
its own when protected, none on the tick a jump leaves it */
long vehicle_riding_cabin_carrier_vehicle(struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_state const *state, boolean exception);
/* whether an enclosed rider stays on its vehicle this tick with no touch of it (carried: the deck carried it into
the tick): never a rider in the window, which waits for a real touch */
boolean vehicle_riding_cabin_holds(long cabin_vehicle, boolean carried);
/* whether a rider's cabin protects it: the verdict it goes by, or on the deck this machine's own standing sample */
boolean vehicle_riding_cabin_encloses(struct vehicle_riding_cabin const *cabin, struct vehicle_riding_state const *state,
	struct vehicle_riding_air const *air);
/* which of a sweep's contacts is that vehicle's (NONE: none) */
short vehicle_riding_cabin_carrier(long vehicle_index, short count, long const *objects);

/* what a cabin rider's place shows against its vehicle
(vehicle_riding_cabin_recover): an anchor to recover to; the place raw
interior, overlapping the vehicle, and the way from the anchor to it
crossing the vehicle's surface; each candidate valid
(vehicle_riding_cabin_candidate), the anchor's distance and the pill's width */
struct vehicle_riding_cabin_check
{
	boolean have_anchor;
	boolean interior;
	boolean touching;
	boolean crosses;
	boolean anchor_valid;
	real anchor_distance;
	real width;
	boolean normal_valid;
	boolean fixed_valid;
	boolean around_valid;
};

/* whether a recovery candidate is a cabin place: raw interior, free (of
the vehicle, reached with the vehicle ignored), no surface of the vehicle
between the anchor and it */
boolean vehicle_riding_cabin_candidate(boolean interior, boolean free, boolean crosses);
/* where a cabin rider settles (an enum vehicle_riding_cabin_settle) */
short vehicle_riding_cabin_recover(struct vehicle_riding_cabin_check const *check);
/* a settle kept: the anchor forgotten at a door, else kept at a settled
place raw interior and free (local: in the vehicle body's frame) */
void vehicle_riding_cabin_settled(struct vehicle_riding_cabin *cabin, short settle, boolean interior,
	real_point3d const *local);
/* whether a rider's cabin has evidence worth its recovery's casts (its
protection, its history, an enclosed verdict), keyed to this unit and window */
boolean vehicle_riding_cabin_watched(struct vehicle_riding_cabin const *cabin, struct vehicle_riding_state const *state,
	long unit_index);
/* whether a vehicle's touches of a rider wait for the rider's own tick:
its window's vehicle, protecting it or with its interior history */
boolean vehicle_riding_cabin_defers(struct vehicle_riding_cabin const *cabin, struct vehicle_riding_state const *state,
	long unit_index, long vehicle_index, boolean exception);
/* a vehicle's collision decision with its touches deferred: resolved
(recorded, never pushed or hurt), the original kept */
short vehicle_riding_cabin_collision(short decision, boolean defers);

#endif
