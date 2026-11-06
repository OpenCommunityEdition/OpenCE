/*
MENDICANT_INFER.C

MENDICANT AI's models without Python or PyTorch (mendicant_infer.h). Each
function follows policy.py (Strategy.body and forward, Brain.decide and
choose_stances, decision_mask, Strategy.sample) and brain.py (assign)
operation for operation, and was checked bit for bit against PyTorch's
answers (make_golden.py).

The file (tools/export_models.py), all little-endian:
  0  magic 'AIBW'   4 version (1)   8 tensor count   12 doctrine hash
  16 decisions      20 chains       24 step choices  28 data offset
  32 the CRC-32 of the data         36..63 zero
  64 the tensor table, 96 bytes each: name[64] (NUL padded), type (0 f32,
     1 f16), dimensions, the four sizes, offset (from the file's start),
     bytes
  then the data. Names are "<model>.<PyTorch name>", the models
  strategy.covenant, strategy.marines, squad.covenant and squad.marines; a
  file's other tensors (the AI Chief's: chief.*) are ignored.
*/

/* ---------- headers */

#include "mendicant_infer.h"

#include <stdlib.h>
#include <string.h>

/* ---------- constants */

#define MENDICANT_INFER_MAGIC 0x57424941UL
#define MENDICANT_INFER_VERSION 1
#define MENDICANT_INFER_HEADER 64
#define MENDICANT_INFER_ENTRY 96
#define MENDICANT_INFER_MAXIMUM_TENSORS 128

/* ---------- structures */

struct mendicant_infer_index
{
	unsigned char const *table;
	unsigned long count;
	float **at;
};

/* ---------- private variables */

/* mendicant_infer_assign's order of scores (qsort's comparison reads it) */
static float const *mendicant_infer_sort_scores;

/* ---------- maths: only + - * /, so every target rounds alike (Cephes'
single-precision forms) */

static float mendicant_infer_ldexp(float y, int n)
{
	union { float f; unsigned int u; } scale;

	while (n > 127)
	{
		scale.u = (unsigned int)(127 + 127) << 23;
		y *= scale.f;
		n -= 127;
	}
	while (n < -126)
	{
		scale.u = (unsigned int)(127 - 126) << 23;
		y *= scale.f;
		n += 126;
	}
	scale.u = (unsigned int)(n + 127) << 23;
	return y * scale.f;
}

float mendicant_infer_expf(float x)
{
	float fx, z, y;
	int n;

	if (x > 88.72f)
		return 3.4028235e38f;
	if (x < -103.0f)
		return 0.0f;
	fx = x * 1.44269504088896341f + 0.5f;
	n = (int)fx;
	if ((float)n > fx)
		n--;
	x = x - (float)n * 0.693359375f;
	x = x - (float)n * -2.12194440e-4f;
	z = x * x;
	y = ((((((1.9875691500e-4f * x + 1.3981999507e-3f) * x + 8.3334519073e-3f) * x + 4.1665795894e-2f) * x +
		1.6666665459e-1f) * x + 5.0000001201e-1f) * z) + x + 1.0f;
	return mendicant_infer_ldexp(y, n);
}

float mendicant_infer_tanhf(float x)
{
	float z = x < 0.0f ? -x : x, s;

	if (z > 0.625f)
	{
		if (z > 10.0f)
			return x < 0.0f ? -1.0f : 1.0f;
		s = mendicant_infer_expf(z + z);
		z = 1.0f - 2.0f / (s + 1.0f);
		return x < 0.0f ? -z : z;
	}
	s = x * x;
	return ((((-5.70498872745e-3f * s + 2.06390887954e-2f) * s - 5.37397155531e-2f) * s + 1.33314422036e-1f) * s -
		3.33332819422e-1f) * s * x + x;
}

/* eight running sums in a fixed order: SSE2, NEON and wasm's SIMD128 all
vectorise this alike (two registers of four), and with no reassociation the
result is the same on every target */
static float mendicant_infer_dot(float const *a, float const *b, long n)
{
	float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f, s5 = 0.0f, s6 = 0.0f, s7 = 0.0f;
	long i = 0;

	for (; i + 8 <= n; i += 8)
	{
		s0 += a[i] * b[i];
		s1 += a[i + 1] * b[i + 1];
		s2 += a[i + 2] * b[i + 2];
		s3 += a[i + 3] * b[i + 3];
		s4 += a[i + 4] * b[i + 4];
		s5 += a[i + 5] * b[i + 5];
		s6 += a[i + 6] * b[i + 6];
		s7 += a[i + 7] * b[i + 7];
	}
	for (; i < n; i++)
		s0 += a[i] * b[i];
	return ((s0 + s4) + (s1 + s5)) + ((s2 + s6) + (s3 + s7));
}

static void mendicant_infer_linear_apply(struct mendicant_infer_linear const *layer, float const *x, float *y,
	int squash)
{
	long o;

	for (o = 0; o < layer->out; o++)
	{
		float v = mendicant_infer_dot(layer->w + o * layer->in, x, layer->in) + layer->b[o];

		y[o] = squash ? mendicant_infer_tanhf(v) : v;
	}
}

/* rows [first, first + count) of a weight matrix 128 wide */
static void mendicant_infer_project(float const *w, float const *b, long first, long count, float const *x, float *y)
{
	long o;

	for (o = 0; o < count; o++)
		y[o] = mendicant_infer_dot(w + (first + o) * 128, x, 128) + b[first + o];
}

/* ---------- the strategic model (policy.Strategy) */

void mendicant_infer_strategy_logits(struct mendicant_infer_strategy const *model,
	struct mendicant_infer_workspace *work, float const *features, float *logits)
{
	float const *table = features + MENDICANT_INFER_SUMMARY_FEATURES;
	float friends_weight = 0.0f, foes_weight = 0.0f;
	float *friends = work->joined + 384, *foes = work->joined + 512;
	/* sqrt(1 / head size), with 4 heads of 32 */
	float const scaling = 0.17677669529663687f;
	int j, d, h, any = 0;

	mendicant_infer_linear_apply(&model->summary, features, work->summary, 1);
	for (j = 0; j < MENDICANT_INFER_ENTITIES; j++)
	{
		float total = 0.0f;

		for (d = 0; d < MENDICANT_INFER_ENTITY_FEATURES; d++)
		{
			float v = table[j * MENDICANT_INFER_ENTITY_FEATURES + d];

			total += v < 0.0f ? -v : v;
		}
		work->present[j] = total > 0.0f;
		any |= work->present[j];
	}

	/* an absent entity is masked from the attention and weighs nothing in the
	means, so it is not computed, except the first when none is present
	(the attention needs one place to look) */
	memset(friends, 0, sizeof(float) * 256);
	for (j = 0; j < MENDICANT_INFER_ENTITIES; j++)
	{
		float const *row = table + j * MENDICANT_INFER_ENTITY_FEATURES;

		if (!work->present[j] && !(j == 0 && !any))
			continue;
		mendicant_infer_linear_apply(&model->entity0, row, work->entity_hidden, 1);
		mendicant_infer_linear_apply(&model->entity2, work->entity_hidden, work->entities[j], 1);
		mendicant_infer_project(model->in_w, model->in_b, 128, 128, work->entities[j], work->k[j]);
		mendicant_infer_project(model->in_w, model->in_b, 256, 128, work->entities[j], work->v[j]);
		if (work->present[j])
		{
			float enemy = row[6], friend_share = 1.0f - enemy;

			for (d = 0; d < 128; d++)
			{
				friends[d] += work->entities[j][d] * friend_share;
				foes[d] += work->entities[j][d] * enemy;
			}
			friends_weight += friend_share;
			foes_weight += enemy;
		}
	}
	for (d = 0; d < 128; d++)
	{
		friends[d] /= friends_weight + 1e-6f;
		foes[d] /= foes_weight + 1e-6f;
	}

	mendicant_infer_linear_apply(&model->query, work->summary, work->query, 0);
	mendicant_infer_project(model->in_w, model->in_b, 0, 128, work->query, work->q);
	for (d = 0; d < 128; d++)
		work->q[d] *= scaling;
	for (h = 0; h < 4; h++)
	{
		float score[MENDICANT_INFER_ENTITIES], top = -3.4e38f, sum = 0.0f;

		for (j = 0; j < MENDICANT_INFER_ENTITIES; j++)
		{
			if (!work->present[j] && !(j == 0 && !any))
				continue;
			score[j] = mendicant_infer_dot(work->q + h * 32, work->k[j] + h * 32, 32);
			if (score[j] > top)
				top = score[j];
		}
		for (j = 0; j < MENDICANT_INFER_ENTITIES; j++)
		{
			if (!work->present[j] && !(j == 0 && !any))
				continue;
			score[j] = mendicant_infer_expf(score[j] - top);
			sum += score[j];
		}
		for (d = 0; d < 32; d++)
			work->attended[h * 32 + d] = 0.0f;
		for (j = 0; j < MENDICANT_INFER_ENTITIES; j++)
		{
			float a;

			if (!work->present[j] && !(j == 0 && !any))
				continue;
			a = score[j] / sum;
			for (d = 0; d < 32; d++)
				work->attended[h * 32 + d] += a * work->v[j][h * 32 + d];
		}
	}
	memcpy(work->joined, work->summary, sizeof(float) * 256);
	mendicant_infer_linear_apply(&model->out_proj, work->attended, work->joined + 256, 0);
	mendicant_infer_linear_apply(&model->trunk0, work->joined, work->hidden0, 1);
	mendicant_infer_linear_apply(&model->trunk2, work->hidden0, work->hidden1, 1);
	mendicant_infer_linear_apply(&model->decision, work->hidden1, logits, 0);
}

/* ---------- the per-enemy model (policy.Brain.decide) */

void mendicant_infer_squad_scores(struct mendicant_infer_squad const *model, float const *team,
	float const *features, int count, float *scores, float *side_out)
{
	float hidden[16], out[3], plan[2], side, x[12], a[32], b[32];
	int member, candidate, f;

	mendicant_infer_linear_apply(&model->team0, team, hidden, 1);
	mendicant_infer_linear_apply(&model->team2, hidden, out, 0);
	side = (out[0] >= 0.0f || team[7] < 0.5f) ? 1.0f : -1.0f;
	plan[0] = mendicant_infer_tanhf(out[1]) * team[7];
	plan[1] = mendicant_infer_tanhf(out[2]) * team[7];
	for (member = 0; member < count; member++)
	{
		for (candidate = 0; candidate < MENDICANT_INFER_CANDIDATES; candidate++)
		{
			float const *row = features + (member * MENDICANT_INFER_CANDIDATES + candidate) * MENDICANT_INFER_FEATURES;

			for (f = 0; f < MENDICANT_INFER_FEATURES; f++)
				x[f] = row[f];
			x[0] = x[0] * side;
			x[10] = plan[0];
			x[11] = plan[1];
			mendicant_infer_linear_apply(&model->enemy0, x, a, 1);
			mendicant_infer_linear_apply(&model->enemy2, a, b, 1);
			mendicant_infer_linear_apply(&model->enemy4, b, scores + member * MENDICANT_INFER_CANDIDATES + candidate, 0);
		}
	}
	*side_out = side;
}

/* policy.StanceModel.forward: the member's own view, then his group's */
void mendicant_infer_stance_logits(struct mendicant_infer_squad const *model, float const *own,
	float const *team, float *logits)
{
	float x[MENDICANT_INFER_FEATURES + MENDICANT_INFER_TEAM_FEATURES], hidden[32];
	int f;

	for (f = 0; f < MENDICANT_INFER_FEATURES; f++)
		x[f] = own[f];
	for (f = 0; f < MENDICANT_INFER_TEAM_FEATURES; f++)
		x[MENDICANT_INFER_FEATURES + f] = team[f];
	mendicant_infer_linear_apply(&model->stance.l0, x, hidden, 1);
	mendicant_infer_linear_apply(&model->stance.l2, hidden, logits, 0);
}

/* best first; ties go to the lowest index (numpy's argsort leaves them
unspecified) */
static int mendicant_infer_by_score(void const *a, void const *b)
{
	int i = *(short const *)a, j = *(short const *)b;

	if (mendicant_infer_sort_scores[i] != mendicant_infer_sort_scores[j])
		return mendicant_infer_sort_scores[i] > mendicant_infer_sort_scores[j] ? -1 : 1;
	return i - j;
}

void mendicant_infer_assign(float const *scores, short const *counts, short const *positions, int count,
	short *choice)
{
	static short order[MENDICANT_INFER_SQUAD * MENDICANT_INFER_CANDIDATES];
	static float masked[MENDICANT_INFER_SQUAD * MENDICANT_INFER_CANDIDATES];
	unsigned char done[MENDICANT_INFER_SQUAD];
	short taken[MENDICANT_INFER_SQUAD];
	int n, i, t, taken_count = 0;

	if (count > MENDICANT_INFER_SQUAD)
		count = MENDICANT_INFER_SQUAD;
	n = count * MENDICANT_INFER_CANDIDATES;
	for (i = 0; i < n; i++)
	{
		masked[i] = i % MENDICANT_INFER_CANDIDATES < counts[i / MENDICANT_INFER_CANDIDATES] ? scores[i] : -1e9f;
		order[i] = (short)i;
	}
	mendicant_infer_sort_scores = masked;
	qsort(order, (size_t)n, sizeof(short), mendicant_infer_by_score);
	memset(done, 0, sizeof(done));
	for (i = 0; i < count; i++)
		choice[i] = 0;
	for (i = 0; i < n; i++)
	{
		int member = order[i] / MENDICANT_INFER_CANDIDATES, candidate = order[i] % MENDICANT_INFER_CANDIDATES;
		int position, clash = 0;

		if (done[member] || masked[order[i]] <= -1e8f)
			continue;
		position = positions[order[i]];
		for (t = 0; t < taken_count; t++)
			clash |= taken[t] == position;
		if (position >= 0 && clash)
			continue;
		choice[member] = (short)candidate;
		done[member] = 1;
		if (position >= 0)
			taken[taken_count++] = (short)position;
	}
}

/* ---------- choosing */

void mendicant_infer_decision_mask(unsigned char *allowed, int chains, int step_choices,
	unsigned char const *chain_covenant, int kind, unsigned long long step_mask, int covenant)
{
	int plans = chains * 4, i, any = 0, pass;

	memset(allowed, 0, (size_t)(plans + step_choices));
	if (kind == 0)
	{
		/* step_mask is the chains that suit the moment (a bit each), when given;
		none of them allowed: any of the side's */
		for (pass = 0; pass < 2 && !any; pass++, step_mask = 0)
		{
			for (i = 0; i < chains; i++)
			{
				if ((covenant < 0 || chain_covenant[i] == (covenant != 0)) && (!step_mask || (step_mask >> i & 1)))
				{
					allowed[i * 4] = allowed[i * 4 + 1] = allowed[i * 4 + 2] = allowed[i * 4 + 3] = 1;
					any = 1;
				}
			}
		}
		return;
	}
	/* the next step: those the doctrine allows (the last is "done"), else done */
	for (i = 0; i < step_choices && i < 64; i++)
		any |= allowed[plans + i] = (unsigned char)(step_mask >> i & 1);
	if (!any)
		allowed[plans + step_choices - 1] = 1;
}

int mendicant_infer_sample(float const *logits, unsigned char const *allowed, int n, float temperature, float u,
	float *probabilities)
{
	float t = temperature > 0.05f ? temperature : 0.05f, top = -3.4e38f, sum = 0.0f, target;
	float p[MENDICANT_INFER_MAX_DECISIONS];
	int i, last = -1, chosen = -1, finite = 1;

	if (n > MENDICANT_INFER_MAX_DECISIONS)
		n = MENDICANT_INFER_MAX_DECISIONS;
	for (i = 0; i < n; i++)
	{
		if (allowed[i])
		{
			float v = logits[i] / t;

			if (!(v == v) || v > 3.0e38f || v < -3.0e38f)
				finite = 0;
			else if (v > top)
				top = v;
		}
	}
	for (i = 0; i < n; i++)
	{
		/* a broken model (not finite) chooses at random among the allowed, as
		the trainer does */
		p[i] = !allowed[i] ? 0.0f : finite ? mendicant_infer_expf(logits[i] / t - top) : 1.0f;
		sum += p[i];
	}
	target = u * sum;
	for (i = 0; i < n; i++)
	{
		if (probabilities)
			probabilities[i] = p[i] / sum;
		if (!allowed[i])
			continue;
		last = i;
		target -= p[i];
		if (target < 0.0f && chosen < 0)
			chosen = i;
	}
	/* u rounding past the sum: the last allowed */
	return chosen >= 0 ? chosen : last;
}

float mendicant_infer_random(unsigned long long *state)
{
	unsigned long long z = (*state += 0x9E3779B97F4A7C15ULL);

	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	z ^= z >> 31;
	return (float)(unsigned long)(z >> 40) * (1.0f / 16777216.0f);
}

/* ---------- loading */

static unsigned long mendicant_infer_read32(unsigned char const *p)
{
	return ((unsigned long)p[0] | (unsigned long)p[1] << 8 | (unsigned long)p[2] << 16 |
		(unsigned long)p[3] << 24) & 0xFFFFFFFFUL;
}

static unsigned long mendicant_infer_crc32(unsigned char const *p, unsigned long n)
{
	unsigned long c = 0xFFFFFFFFUL;
	int k;

	while (n--)
	{
		c ^= *p++;
		for (k = 0; k < 8; k++)
			c = (c >> 1) ^ (0xEDB88320UL & (0UL - (c & 1)));
	}
	return ~c & 0xFFFFFFFFUL;
}

static float mendicant_infer_half(unsigned short h)
{
	union { float f; unsigned int u; } r;
	unsigned int sign = (unsigned int)(h & 0x8000) << 16, exponent = (h >> 10) & 31, mantissa = h & 1023;

	if (exponent == 0)
	{
		/* subnormal: m x 2^-24 */
		r.f = (float)mantissa * (1.0f / 16777216.0f);
		r.u |= sign;
		return r.f;
	}
	r.u = sign | (exponent == 31 ? 255U << 23 : (exponent + 112) << 23) | mantissa << 13;
	return r.f;
}

/* the tensor of that name, if its shape is [d0] (d1 0) or [d0][d1] */
static float const *mendicant_infer_find(struct mendicant_infer_index const *index, char const *name, long d0,
	long d1)
{
	unsigned long i;

	for (i = 0; i < index->count; i++)
	{
		unsigned char const *entry = index->table + i * MENDICANT_INFER_ENTRY;

		if (strncmp((char const *)entry, name, 64))
			continue;
		if ((long)mendicant_infer_read32(entry + 72) != d0 || (long)mendicant_infer_read32(entry + 76) != d1 ||
			(long)mendicant_infer_read32(entry + 68) != (d1 ? 2 : 1))
		{
			return NULL;
		}
		return index->at[i];
	}
	return NULL;
}

static int mendicant_infer_bind(struct mendicant_infer_linear *layer, struct mendicant_infer_index const *index,
	char const *prefix, char const *name, long out, long in)
{
	char tensor[96];

	strcpy(tensor, prefix);
	strcat(tensor, name);
	strcat(tensor, ".weight");
	layer->w = mendicant_infer_find(index, tensor, out, in);
	strcpy(tensor, prefix);
	strcat(tensor, name);
	strcat(tensor, ".bias");
	layer->b = mendicant_infer_find(index, tensor, out, 0);
	layer->out = out;
	layer->in = in;
	return layer->w && layer->b;
}

int mendicant_infer_load(struct mendicant_infer_weights *weights, void const *data, unsigned long size,
	unsigned long expected_doctrine)
{
	static char const *const sides[NUMBER_OF_MENDICANT_INFER_SIDES] = { "covenant.", "marines." };
	unsigned char const *bytes = (unsigned char const *)data;
	struct mendicant_infer_index index;
	float *at[MENDICANT_INFER_MAXIMUM_TENSORS], *cursor;
	unsigned long count, start, i, total = 0;
	long decisions;
	int side;
	char prefix[48];

	memset(weights, 0, sizeof(*weights));
	if (size < MENDICANT_INFER_HEADER || mendicant_infer_read32(bytes) != MENDICANT_INFER_MAGIC)
		return _mendicant_infer_error_format;
	if (mendicant_infer_read32(bytes + 4) != MENDICANT_INFER_VERSION)
		return mendicant_infer_read32(bytes + 4) > MENDICANT_INFER_VERSION ? _mendicant_infer_error_version :
			_mendicant_infer_error_format;
	count = mendicant_infer_read32(bytes + 8);
	start = mendicant_infer_read32(bytes + 28);
	if (count > MENDICANT_INFER_MAXIMUM_TENSORS || start > size ||
		MENDICANT_INFER_HEADER + count * MENDICANT_INFER_ENTRY > start)
	{
		return _mendicant_infer_error_table;
	}
	if (mendicant_infer_crc32(bytes + start, size - start) != mendicant_infer_read32(bytes + 32))
		return _mendicant_infer_error_crc;
	weights->doctrine_hash = mendicant_infer_read32(bytes + 12);
	weights->decisions = mendicant_infer_read32(bytes + 16);
	weights->chains = mendicant_infer_read32(bytes + 20);
	weights->step_choices = mendicant_infer_read32(bytes + 24);
	if ((expected_doctrine && weights->doctrine_hash != (expected_doctrine & 0xFFFFFFFFUL)) ||
		weights->decisions > MENDICANT_INFER_MAX_DECISIONS || weights->chains > 64 || weights->step_choices > 64 ||
		weights->decisions != weights->chains * 4 + weights->step_choices)
	{
		return _mendicant_infer_error_doctrine;
	}
	for (i = 0; i < count; i++)
	{
		unsigned char const *entry = bytes + MENDICANT_INFER_HEADER + i * MENDICANT_INFER_ENTRY;
		unsigned long dimensions = mendicant_infer_read32(entry + 68), elements = 1, k;
		unsigned long type = mendicant_infer_read32(entry + 64), offset = mendicant_infer_read32(entry + 88);
		unsigned long length = mendicant_infer_read32(entry + 92);

		if (dimensions > 4 || type > 1)
			return _mendicant_infer_error_tensor;
		for (k = 0; k < dimensions; k++)
		{
			unsigned long dimension = mendicant_infer_read32(entry + 72 + k * 4);

			if (dimension > 0x100000UL || elements * dimension > 0x1000000UL)
				return _mendicant_infer_error_tensor;
			elements *= dimension;
		}
		if (length != elements * (type ? 2 : 4) || offset < start || offset > size || size - offset < length)
			return _mendicant_infer_error_tensor;
		total += elements;
		if (total > 0x4000000UL)
			return _mendicant_infer_error_tensor;
	}
	weights->storage = (float *)malloc(sizeof(float) * (total ? total : 1));
	if (!weights->storage)
		return _mendicant_infer_error_memory;
	cursor = weights->storage;
	for (i = 0; i < count; i++)
	{
		unsigned char const *entry = bytes + MENDICANT_INFER_HEADER + i * MENDICANT_INFER_ENTRY;
		unsigned char const *p = bytes + mendicant_infer_read32(entry + 88);
		unsigned long type = mendicant_infer_read32(entry + 64), k;
		unsigned long elements = type ? mendicant_infer_read32(entry + 92) / 2 : mendicant_infer_read32(entry + 92) / 4;

		at[i] = cursor;
		for (k = 0; k < elements; k++)
		{
			union { float f; unsigned int u; } v;

			if (type)
				v.f = mendicant_infer_half((unsigned short)(p[k * 2] | p[k * 2 + 1] << 8));
			else
				v.u = (unsigned int)mendicant_infer_read32(p + k * 4);
			cursor[k] = v.f;
		}
		cursor += elements;
	}
	index.table = bytes + MENDICANT_INFER_HEADER;
	index.count = count;
	index.at = at;

	decisions = (long)weights->decisions;
	for (side = 0; side < NUMBER_OF_MENDICANT_INFER_SIDES; side++)
	{
		struct mendicant_infer_strategy *strategy = &weights->strategy[side];
		struct mendicant_infer_squad *squad = &weights->squad[side];

		strcpy(prefix, "strategy.");
		strcat(prefix, sides[side]);
		weights->has_strategy[side] =
			mendicant_infer_bind(&strategy->summary, &index, prefix, "summary.0", 256, MENDICANT_INFER_SUMMARY_FEATURES) &&
			mendicant_infer_bind(&strategy->entity0, &index, prefix, "entity.0", 128, MENDICANT_INFER_ENTITY_FEATURES) &&
			mendicant_infer_bind(&strategy->entity2, &index, prefix, "entity.2", 128, 128) &&
			mendicant_infer_bind(&strategy->query, &index, prefix, "query", 128, 256) &&
			mendicant_infer_bind(&strategy->out_proj, &index, prefix, "attention.out_proj", 128, 128) &&
			mendicant_infer_bind(&strategy->trunk0, &index, prefix, "trunk.0", 512, 640) &&
			mendicant_infer_bind(&strategy->trunk2, &index, prefix, "trunk.2", 512, 512) &&
			mendicant_infer_bind(&strategy->decision, &index, prefix, "decision", decisions, 512);
		if (weights->has_strategy[side])
		{
			char tensor[96];

			strcpy(tensor, prefix);
			strcat(tensor, "attention.in_proj_weight");
			strategy->in_w = mendicant_infer_find(&index, tensor, 384, 128);
			strcpy(tensor, prefix);
			strcat(tensor, "attention.in_proj_bias");
			strategy->in_b = mendicant_infer_find(&index, tensor, 384, 0);
			weights->has_strategy[side] = strategy->in_w && strategy->in_b;
		}

		strcpy(prefix, "squad.");
		strcat(prefix, sides[side]);
		weights->has_squad[side] =
			mendicant_infer_bind(&squad->team0, &index, prefix, "team.layers.0", 16, MENDICANT_INFER_TEAM_FEATURES) &&
			mendicant_infer_bind(&squad->team2, &index, prefix, "team.layers.2", 3, 16) &&
			mendicant_infer_bind(&squad->enemy0, &index, prefix, "enemy.layers.0", 32, MENDICANT_INFER_FEATURES + 2) &&
			mendicant_infer_bind(&squad->enemy2, &index, prefix, "enemy.layers.2", 32, 32) &&
			mendicant_infer_bind(&squad->enemy4, &index, prefix, "enemy.layers.4", 1, 32);
		/* the stance model is newer than the rest: weights without it play
		without stances */
		weights->has_stance[side] = weights->has_squad[side] &&
			mendicant_infer_bind(&squad->stance.l0, &index, prefix, "stance.layers.0", 32,
				MENDICANT_INFER_FEATURES + MENDICANT_INFER_TEAM_FEATURES) &&
			mendicant_infer_bind(&squad->stance.l2, &index, prefix, "stance.layers.2", MENDICANT_INFER_STANCES, 32);
	}
	return _mendicant_infer_ok;
}

void mendicant_infer_free(struct mendicant_infer_weights *weights)
{
	free(weights->storage);
	memset(weights, 0, sizeof(*weights));
}

char const *mendicant_infer_error_string(int error)
{
	switch (error)
	{
	case _mendicant_infer_ok:
		return "loaded";
	case _mendicant_infer_error_format:
		return "not a MENDICANT AI models file";
	case _mendicant_infer_error_table:
		return "its tensor table is broken";
	case _mendicant_infer_error_crc:
		return "its data is damaged (CRC)";
	case _mendicant_infer_error_doctrine:
		return "made for another doctrine";
	case _mendicant_infer_error_tensor:
		return "a tensor is broken";
	case _mendicant_infer_error_memory:
		return "no memory for the weights";
	case _mendicant_infer_error_version:
		return "a newer format than this build reads";
	}
	return "unknown error";
}
