
typedef unsigned short word;
typedef char boolean;
struct defn { char pad[0x44]; long index; };
struct seatblk { long count; void *addr; };
struct graph { char pad[0xc]; struct seatblk seats; };
struct entry { short animation_index; short pad; };
struct defn *defn_get(long index);
struct graph *graph_get(long index);
void *block_element(struct seatblk *b, long i, long size);
struct entry *table_get(struct seatblk *b);
void start_interp(long object_index, short frames);
short choose(long render, long graph_index, short animation_index);
void apply(long unit_index, long graph_index, short animation_index);
void tail(long unit_index);

void f(long unit_index, long target_index, short seat_index)
{
	struct defn *d;
	struct graph *g;
	struct seatblk *sb;
	long gi;

	d = defn_get(unit_index);
	g = graph_get(d->index);
	sb = (struct seatblk *)block_element(&g->seats, seat_index, 0x64);
	if (sb->count > 7)
	{
		short ai = table_get(sb)[0].animation_index;

		if (ai != -1)
		{
			start_interp(unit_index, 6);
			gi = d->index;
			ai = choose(1, gi, ai);
			gi = d->index;
			apply(unit_index, gi, ai);
		}
	}
	tail(unit_index);
}
