/* Clean engine HUD declarations with the color union explicitly represented
   as its two runtime objective tick fields; no pointers or copied SDK data. */
struct native_hud_color {
 unsigned long color; unsigned long flash_color; real flash_period; real flash_delay;
 short number_of_flashes; word flash_flags; real flash_length;
 unsigned long disabled_color; short up_ticks; short fade_ticks;
};
typedef char native_hud_color_size[sizeof(struct native_hud_color)==sizeof(struct hud_color_definition)?1:-1];
typedef char native_hud_color_union_offset[offsetof(struct native_hud_color,up_ticks)==offsetof(struct hud_color_definition,custom)?1:-1];
struct hud_waypoint_arrow
{
	char name[0x20];
	long unused0[2];
	unsigned long color;
	real opacity;
	real fade;
	short sequence_indices[3];
	short pad;
	long unused1[4];
	unsigned long flags;
	long unused2[6];
};
typedef char native_hud_arrow_size[sizeof(struct hud_waypoint_arrow)==104?1:-1];
typedef char native_hud_root_size[sizeof(struct hud_globals_definition)==1104?1:-1];
