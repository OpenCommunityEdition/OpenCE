/*
MENDICANT_INFER.H

MENDICANT AI's models without Python or PyTorch (mendicant_infer.c): the
strategic model (policy.Strategy), the per-enemy model (policy.Brain: the
team model, the enemy model and the stance model), brain.py's assignment and
the decision mask and sampling of policy.py, read from one models.aibw file
(tools/export_models.py).

Plain C, with no allocation after the load, no system maths (only + - * /,
so every build rounds alike: -ffp-contract=off) and the same order of sums
on every target, so a decision is bit-identical on i686, Windows, the
Android guest (arm64_32) and wasm32.
*/

#ifndef HALO_MENDICANT_INFER_H
#define HALO_MENDICANT_INFER_H

/* ---------- constants */

#define MENDICANT_INFER_SUMMARY_FEATURES 48
#define MENDICANT_INFER_ENTITIES 32
#define MENDICANT_INFER_ENTITY_FEATURES 12
/* the strategic model's input: the summary (plan_features, plan_extra),
then the entities (432) */
#define MENDICANT_INFER_PLAN_FEATURES (MENDICANT_INFER_SUMMARY_FEATURES + MENDICANT_INFER_ENTITIES * MENDICANT_INFER_ENTITY_FEATURES)
#define MENDICANT_INFER_MAX_DECISIONS 160
#define MENDICANT_INFER_SQUAD 32
#define MENDICANT_INFER_CANDIDATES 24
#define MENDICANT_INFER_FEATURES 10
#define MENDICANT_INFER_TEAM_FEATURES 8
#define MENDICANT_INFER_STANCES 5

/* the models' sides */
enum
{
	_mendicant_infer_covenant = 0,
	_mendicant_infer_marines,
	NUMBER_OF_MENDICANT_INFER_SIDES
};

/* mendicant_infer_load's results */
enum
{
	_mendicant_infer_ok = 0,
	/* not a models file */
	_mendicant_infer_error_format = -1,
	/* its tensor table is broken */
	_mendicant_infer_error_table = -2,
	/* its data's CRC is wrong */
	_mendicant_infer_error_crc = -3,
	/* made for another doctrine (another set of decisions) */
	_mendicant_infer_error_doctrine = -4,
	/* a tensor's type, shape or place is broken */
	_mendicant_infer_error_tensor = -5,
	/* no memory for the weights */
	_mendicant_infer_error_memory = -6,
	/* a newer format than this build reads */
	_mendicant_infer_error_version = -7
};

/* ---------- structures */

/* a linear layer: w is [out][in], PyTorch's layout */
struct mendicant_infer_linear
{
	float const *w;
	float const *b;
	long out;
	long in;
};

/* policy.Strategy (its training-only heads are not exported) */
struct mendicant_infer_strategy
{
	struct mendicant_infer_linear summary, entity0, entity2, query, out_proj, trunk0, trunk2, decision;
	/* MultiheadAttention's in_proj: rows q, k, v (3 x 128) */
	float const *in_w;
	float const *in_b;
};

/* policy.StanceModel: Linear(18, 32), tanh, Linear(32, 5) */
struct mendicant_infer_stance
{
	struct mendicant_infer_linear l0, l2;
};

/* policy.Brain: the team model, the enemy model and the stance model */
struct mendicant_infer_squad
{
	struct mendicant_infer_linear team0, team2, enemy0, enemy2, enemy4;
	struct mendicant_infer_stance stance;
};

struct mendicant_infer_weights
{
	float *storage;
	unsigned long doctrine_hash, decisions, chains, step_choices;
	int has_strategy[NUMBER_OF_MENDICANT_INFER_SIDES];
	int has_squad[NUMBER_OF_MENDICANT_INFER_SIDES];
	int has_stance[NUMBER_OF_MENDICANT_INFER_SIDES];
	struct mendicant_infer_strategy strategy[NUMBER_OF_MENDICANT_INFER_SIDES];
	struct mendicant_infer_squad squad[NUMBER_OF_MENDICANT_INFER_SIDES];
};

/* the scratch of one strategic decision (about 66 KB): keep it static, not
on a stack */
struct mendicant_infer_workspace
{
	float summary[256], query[128], q[128], entity_hidden[128];
	float entities[MENDICANT_INFER_ENTITIES][128];
	float k[MENDICANT_INFER_ENTITIES][128];
	float v[MENDICANT_INFER_ENTITIES][128];
	float joined[256 + 128 * 3], attended[128], hidden0[512], hidden1[512];
	int present[MENDICANT_INFER_ENTITIES];
};

/* ---------- prototypes/MENDICANT_INFER.C */

/* data: the file's bytes, which may be freed after the load. Returns
_mendicant_infer_ok, or an error. expected_doctrine: the hash of the game's
doctrine (mendicant_doctrine_hash), 0 to skip the check. */
int mendicant_infer_load(struct mendicant_infer_weights *weights, void const *data, unsigned long size,
	unsigned long expected_doctrine);
void mendicant_infer_free(struct mendicant_infer_weights *weights);
char const *mendicant_infer_error_string(int error);

/* features: MENDICANT_INFER_PLAN_FEATURES; logits: weights->decisions */
void mendicant_infer_strategy_logits(struct mendicant_infer_strategy const *model,
	struct mendicant_infer_workspace *work, float const *features, float *logits);
/* policy.Brain.decide: scores is count x MENDICANT_INFER_CANDIDATES;
features count x candidates x MENDICANT_INFER_FEATURES; side: +1 or -1 */
void mendicant_infer_squad_scores(struct mendicant_infer_squad const *model, float const *team,
	float const *features, int count, float *scores, float *side);
/* policy.StanceModel: own (10, the member's staying-put candidate), team (8),
logits (5) */
void mendicant_infer_stance_logits(struct mendicant_infer_squad const *model, float const *own,
	float const *team, float *logits);
/* brain.assign: best first, each member its best position no one else has
(a position below 0 is anyone's) */
void mendicant_infer_assign(float const *scores, short const *counts, short const *positions, int count,
	short *choice);

/* policy.decision_mask: chain_covenant[c] is 1 for a Covenant chain;
covenant below 0 is either side */
void mendicant_infer_decision_mask(unsigned char *allowed, int chains, int step_choices,
	unsigned char const *chain_covenant, int kind, unsigned long long step_mask, int covenant);
/* softmax(logits / max(temperature, 0.05)) over the allowed, then the index
where the sum passes u (in [0, 1)); a distribution that is not finite is
uniform over the allowed; probabilities, if given, gets the distribution */
int mendicant_infer_sample(float const *logits, unsigned char const *allowed, int n, float temperature, float u,
	float *probabilities);

/* splitmix64: [0, 1) */
float mendicant_infer_random(unsigned long long *state);
float mendicant_infer_expf(float x);
float mendicant_infer_tanhf(float x);

#endif
