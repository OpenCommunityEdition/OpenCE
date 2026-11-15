/* port: a Custom Edition map's mouse screen is Halo PC's own: created 121, OK 122, Defaults 133. Its
spinners' ten steps are these sensitivities (the port's own list has more) */
#define MOUSE_STEPS 10
static double const mouse_step_values[MOUSE_STEPS] = { 0.25, 0.4, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 2.5, 3.0 };

static struct
{
	short horizontal;
	short vertical;
	short invert;
} mouse_loaded;

static short mouse_step_nearest(double value)
{
	short index, best = 0;
	double best_gap = 1000.0;

	for (index = 0; index < MOUSE_STEPS; index++)
	{
		double gap = mouse_step_values[index] > value ? mouse_step_values[index] - value :
			value - mouse_step_values[index];

		if (gap < best_gap)
		{
			best_gap = gap;
			best = index;
		}
	}
	return best;
}

static struct widget_instance *mouse_list_of(struct widget_instance *widget)
{
	struct widget_instance *top;

	/* (its created event runs before the screen has the list among its children) */
	if (!strcmp(widget->name, "mouse_options_menu"))
		return widget;
	top = screen_of(widget);
	return !strcmp(top->name, "mouse_settings_screen") ? named(top, "mouse_options_menu", 0) : NULL;
}

extern struct widget_instance *ui_widget_port_inject_row(struct widget_instance *source,
	struct widget_instance *before, short vertical_offset, wchar_t const *label, wchar_t const *description,
	wchar_t const *const *values, short value_count);

/* port: the mouse screen's added rows: AIM ASSIST (a copy of the invert row: NO, YES) and MENU CURSOR (a copy
of the horizontal row, with its five values' names) */
static wchar_t const *const mouse_cursor_names[5] = { L"OFF", L"SMALL", L"MEDIUM", L"LARGE", L"EXTRA LARGE" };
static char const *const mouse_cursor_values[5] = { "off", "small", "medium", "large", "extra_large" };

static struct
{
	short aim;
	short cursor;
} mouse_extra_loaded;

static struct widget_instance *row_spinner(struct widget_instance *row)
{
	struct widget_instance *child;

	for (child = row ? row->child : NULL; child && child->type != 2; child = child->next)
		;
	return child;
}

static void mouse_inject(struct widget_instance *list)
{
	struct widget_instance *invert = named(list, "op_invert_vertical", 0);
	struct widget_instance *horizontal = named(list, "op_horizontal_sensitivity", 0);
	struct widget_instance *bar = named(list, "mouse_button_bar", 0);
	struct widget_instance *last_old;

	if (!invert || !horizontal || !bar || named(list, "op_invert_vertical", 1))
		return;
	last_old = bar->previous;
	if (!ui_widget_port_inject_row(invert, bar, (short)(invert->vertical_offset + 30), L"AIM ASSIST:",
		L"Slow and drag the view along with a target\r\nwhile aiming with the mouse.", NULL, 0))
		goto failed;
	if (!ui_widget_port_inject_row(horizontal, bar, (short)(invert->vertical_offset + 60), L"MENU CURSOR:",
		L"The menus' blue arrow for the mouse, and its\r\nsize. Off shows the system's pointer.",
		mouse_cursor_names, 5))
		goto failed;
	return;

failed:
	/* Leave the original controls and focus intact if any clone failed. */
	while (last_old ? last_old->next != bar : list->child != bar)
		ui_widget_delete(last_old ? last_old->next : list->child);
}


static void mouse_extra_show(struct widget_instance *list, boolean defaults)
{
	struct widget_instance *spinner;
	char text[64];
	short aim = 0, cursor = 0, index;

	if (setting_text("input.mouse_aim_assist", text, sizeof(text), defaults))
		aim = !strcmp(text, "true") ? 1 : 0;
	if (setting_text("display.menu_cursor", text, sizeof(text), defaults))
	{
		for (index = 0; index < 5; index++)
		{
			if (!strcmp(text, mouse_cursor_values[index]))
				cursor = index;
		}
	}
	spinner = row_spinner(named(list, "op_invert_vertical", 1));
	if (spinner)
		spinner->parameters.list.selected_index = aim;
	spinner = row_spinner(named(list, "op_horizontal_sensitivity", 1));
	if (spinner)
		spinner->parameters.list.selected_index = cursor;
	if (!defaults)
	{
		mouse_extra_loaded.aim = aim;
		mouse_extra_loaded.cursor = cursor;
	}
}

static void mouse_extra_save(struct widget_instance *list)
{
	struct widget_instance *spinner = row_spinner(named(list, "op_invert_vertical", 1));
	short cursor;

	if (spinner && spinner->parameters.list.selected_index != mouse_extra_loaded.aim)
	{
		mouse_extra_loaded.aim = spinner->parameters.list.selected_index;
		setting_write("input.mouse_aim_assist", mouse_extra_loaded.aim ? "true" : "false");
	}
	spinner = row_spinner(named(list, "op_horizontal_sensitivity", 1));
	if (spinner)
	{
		cursor = (short)PIN(spinner->parameters.list.selected_index, 0, 4);
		if (cursor != mouse_extra_loaded.cursor)
		{
			mouse_extra_loaded.cursor = cursor;
			setting_write("display.menu_cursor", mouse_cursor_values[cursor]);
			platform_display_apply();
		}
	}
}

static void mouse_show(struct widget_instance *list, boolean defaults)
{
	struct widget_instance *spinner;
	char text[64];
	double horizontal = 1.0, vertical = 0.0;
	short horizontal_step, vertical_step, invert = 0;

	if (setting_text("input.mouse_sensitivity", text, sizeof(text), defaults))
		horizontal = atof(text);
	if (setting_text("input.mouse_vertical_sensitivity", text, sizeof(text), defaults))
		vertical = atof(text);
	if (setting_text("input.invert_mouse", text, sizeof(text), defaults))
		invert = !strcmp(text, "true") ? 1 : 0;
	mouse_extra_show(list, defaults);
	horizontal_step = mouse_step_nearest(horizontal);
	vertical_step = vertical > 0.0 ? mouse_step_nearest(vertical) : horizontal_step;
	if ((spinner = named(list, "effects_volume_spinner", 0)) != NULL)
		spinner->parameters.list.selected_index = horizontal_step;
	if ((spinner = named(list, "vertical_sensitivity_spinner", 0)) != NULL)
		spinner->parameters.list.selected_index = vertical_step;
	if ((spinner = named(list, "invert_vertical_spinner", 0)) != NULL)
		spinner->parameters.list.selected_index = invert;
	if (!defaults)
	{
		mouse_loaded.horizontal = horizontal_step;
		mouse_loaded.vertical = vertical_step;
		mouse_loaded.invert = invert;
	}
}

static void mouse_save(struct widget_instance *list)
{
	struct widget_instance *horizontal = named(list, "effects_volume_spinner", 0);
	struct widget_instance *vertical = named(list, "vertical_sensitivity_spinner", 0);
	struct widget_instance *invert = named(list, "invert_vertical_spinner", 0);
	short horizontal_step, vertical_step;
	char text[32];

	if (!horizontal || !vertical || !invert)
		return;
	horizontal_step = (short)PIN(horizontal->parameters.list.selected_index, 0, MOUSE_STEPS - 1);
	vertical_step = (short)PIN(vertical->parameters.list.selected_index, 0, MOUSE_STEPS - 1);
	if (horizontal_step != mouse_loaded.horizontal)
	{
		snprintf(text, sizeof(text), "%g", mouse_step_values[horizontal_step]);
		setting_write("input.mouse_sensitivity", text);
	}
	if (vertical_step != mouse_loaded.vertical)
	{
		/* (the same as horizontal: 0, which the port reads as "same") */
		snprintf(text, sizeof(text), "%g", mouse_step_values[vertical_step]);
		setting_write("input.mouse_vertical_sensitivity", text);
	}
	if (invert->parameters.list.selected_index != mouse_loaded.invert)
		setting_write("input.invert_mouse", invert->parameters.list.selected_index ? "true" : "false");
	mouse_loaded.horizontal = horizontal_step;
	mouse_loaded.vertical = vertical_step;
	mouse_loaded.invert = invert->parameters.list.selected_index;
	mouse_extra_save(list);
}

/* NONE if the function is not this screen's, else its result */
short pc_menu_mouse_function(struct widget_instance *widget, word function)
{
	struct widget_instance *list = mouse_list_of(widget);

	if (!list)
		return NONE;
	if (function == 121)
		mouse_inject(list);
	if (function == 121)
		mouse_show(list, FALSE);
	else if (function == 122)
		mouse_save(list);
	else if (function == 133)
		mouse_show(list, TRUE);
	else
		return NONE;
	return TRUE;
}

/* port: a Custom Edition map's audio screen is Halo PC's own: created 123, OK 124, Defaults 134. Its volume
spinners' eleven steps are 0 to 10 tenths; its environmental sound (OFF, EAX) is the port's reverb */
static struct
{
	short master;
	short effects;
	short music;
	short reverb;
} audio_loaded;

static struct widget_instance *audio_list_of(struct widget_instance *widget)
{
	struct widget_instance *top;

	if (!strcmp(widget->name, "audio_options_menu"))
		return widget;
	top = screen_of(widget);
	return !strcmp(top->name, "audio_settings_screen") ? named(top, "audio_options_menu", 0) : NULL;
}

static short audio_step_read(char const *name, boolean defaults)
{
	char text[64];

	if (!setting_text(name, text, sizeof(text), defaults))
		return 10;
	return (short)PIN((long)(atof(text) * 10.0 + 0.5), 0, 10);
}

/* port: the audio screen's added rows: SOUND, VOICE CHAT, VOICE VOLUME and RESAMPLER */
static wchar_t const *const audio_on_off_names[2] = { L"OFF", L"ON" };
static wchar_t const *const audio_voice_names[3] = { L"PUSH TO TALK", L"OPEN MIC", L"OFF" };
static wchar_t const *const audio_resampler_names[2] = { L"SINC", L"LINEAR" };
static char const *const audio_voice_values[3] = { "push_to_talk", "open_mic", "off" };

static struct
{
	short sound;
	short voice_chat;
	short voice_volume;
	short resampler;
} audio_extra_loaded;

/* port: the audio screen's OUTPUT DEVICE and INPUT DEVICE rows: SYSTEM DEFAULT, then the devices SDL finds */
int platform_audio_devices(int recording, char (*names)[128], int maximum);

#define AUDIO_DEVICE_MAX 16

static char audio_device_real[2][AUDIO_DEVICE_MAX][128];
static wchar_t audio_device_shown[2][AUDIO_DEVICE_MAX][32];
static wchar_t const *audio_device_pointers[2][AUDIO_DEVICE_MAX];
static short audio_device_count[2];
static short audio_device_loaded[2];

static void audio_device_read(short recording)
{
	short added, index, at;

	strcpy(audio_device_real[recording][0], "default");
	ustrncpy(audio_device_shown[recording][0], L"SYSTEM DEFAULT", 31);
	audio_device_shown[recording][0][31] = 0;
	added = (short)platform_audio_devices(recording, audio_device_real[recording] + 1, AUDIO_DEVICE_MAX - 1);
	if (added < 0)
		added = 0;
	if (added > AUDIO_DEVICE_MAX - 1)
		added = AUDIO_DEVICE_MAX - 1;
	for (index = 1; index <= added; index++)
	{
		char const *name = audio_device_real[recording][index];

		for (at = 0; name[at] && at < 30; at++)
		{
			char character = name[at];

			audio_device_shown[recording][index][at] = (wchar_t)(unsigned char)(character >= 'a' &&
				character <= 'z' ? character - 'a' + 'A' : character);
		}
		audio_device_shown[recording][index][at] = 0;
		if (strlen(name) > 18)
			ustrncpy(audio_device_shown[recording][index] + 15, L"...", 4);
	}
	for (index = 0; index <= added; index++)
		audio_device_pointers[recording][index] = audio_device_shown[recording][index];
	audio_device_count[recording] = (short)(added + 1);
}

static void audio_device_show(struct widget_instance *list, boolean defaults)
{
	short recording;

	for (recording = 0; recording < 2; recording++)
	{
		struct widget_instance *spinner = row_spinner(named(list, "op_master_volume", 3 + recording));
		char text[128];
		short index, found = 0;

		if (!spinner)
			continue;
		if (setting_text(recording ? "audio.input_device" : "audio.output_device", text, sizeof(text), defaults))
		{
			for (index = 0; index < audio_device_count[recording]; index++)
			{
				if (!strcmp(text, audio_device_real[recording][index]))
					found = index;
			}
		}
		spinner->parameters.list.selected_index = found;
		if (!defaults)
			audio_device_loaded[recording] = found;
	}
}

static void audio_device_save(struct widget_instance *list)
{
	short recording;

	for (recording = 0; recording < 2; recording++)
	{
		struct widget_instance *spinner = row_spinner(named(list, "op_master_volume", 3 + recording));
		short index;

		if (!spinner)
			continue;
		index = spinner->parameters.list.selected_index;
		if (index < 0 || index >= audio_device_count[recording] || index == audio_device_loaded[recording])
			continue;
		audio_device_loaded[recording] = index;
		setting_write(recording ? "audio.input_device" : "audio.output_device",
			audio_device_real[recording][index]);
	}
}

static void audio_inject(struct widget_instance *list)
{
	struct widget_instance *master = named(list, "op_master_volume", 0);
	struct widget_instance *reverb = named(list, "op_environmental_sound", 0);
	struct widget_instance *bar = named(list, "audio_button_bar", 0);
	struct widget_instance *last_old;

	if (!master || !reverb || !bar || named(list, "op_master_volume", 1))
		return;
	last_old = bar->previous;
	if (!ui_widget_port_inject_row(reverb, bar, 193, L"SOUND:",
		L"Play sound at all; from the next time the\r\ngame starts.", audio_on_off_names, 2))
		goto failed;
	if (!ui_widget_port_inject_row(master, bar, 223, L"VOICE CHAT:",
		L"Talk in network games: while push to talk is\r\nheld, whenever you speak, or never.",
		audio_voice_names, 3))
		goto failed;
	if (!ui_widget_port_inject_row(master, bar, 253, L"VOICE VOLUME:",
		L"The volume of the other players' voices.", NULL, 0))
		goto failed;
	if (!ui_widget_port_inject_row(reverb, bar, 283, L"RESAMPLER:",
		L"How sounds are converted to the output's rate.\r\nSinc is cleaner, linear is brighter.",
		audio_resampler_names, 2))
		goto failed;
	audio_device_read(0);
	audio_device_read(1);
	if (!ui_widget_port_inject_row(master, bar, 313, L"OUTPUT DEVICE:",
		L"Where the game's sound and the voices play.", audio_device_pointers[0], audio_device_count[0]))
		goto failed;
	if (!ui_widget_port_inject_row(master, bar, 343, L"INPUT DEVICE:",
		L"The microphone voice chat listens to.", audio_device_pointers[1], audio_device_count[1]))
		goto failed;
	return;

failed:
	/* Leave the original controls and focus intact if any clone failed. */
	while (last_old ? last_old->next != bar : list->child != bar)
		ui_widget_delete(last_old ? last_old->next : list->child);
}


static void audio_extra_show(struct widget_instance *list, boolean defaults)
{
	struct widget_instance *spinner;
	char text[64];
	short sound = 1, chat = 0, resampler = 0, index;
	short voice = audio_step_read("audio.voice_volume", defaults);

	audio_device_show(list, defaults);
	if (setting_text("audio.enabled", text, sizeof(text), defaults))
		sound = !strcmp(text, "true") ? 1 : 0;
	if (setting_text("audio.voice_chat", text, sizeof(text), defaults))
	{
		for (index = 0; index < 3; index++)
		{
			if (!strcmp(text, audio_voice_values[index]))
				chat = index;
		}
	}
	if (setting_text("audio.resampler", text, sizeof(text), defaults))
		resampler = !strcmp(text, "linear") ? 1 : 0;
	if ((spinner = row_spinner(named(list, "op_environmental_sound", 1))) != NULL)
		spinner->parameters.list.selected_index = sound;
	if ((spinner = row_spinner(named(list, "op_master_volume", 1))) != NULL)
		spinner->parameters.list.selected_index = chat;
	if ((spinner = row_spinner(named(list, "op_master_volume", 2))) != NULL)
		spinner->parameters.list.selected_index = voice;
	if ((spinner = row_spinner(named(list, "op_environmental_sound", 2))) != NULL)
		spinner->parameters.list.selected_index = resampler;
	if (!defaults)
	{
		audio_extra_loaded.sound = sound;
		audio_extra_loaded.voice_chat = chat;
		audio_extra_loaded.voice_volume = voice;
		audio_extra_loaded.resampler = resampler;
	}
}

static void audio_extra_save(struct widget_instance *list)
{
	struct widget_instance *spinner;
	char text[16];
	short value;

	if ((spinner = row_spinner(named(list, "op_environmental_sound", 1))) != NULL &&
		spinner->parameters.list.selected_index != audio_extra_loaded.sound)
	{
		audio_extra_loaded.sound = spinner->parameters.list.selected_index;
		setting_write("audio.enabled", audio_extra_loaded.sound ? "true" : "false");
	}
	if ((spinner = row_spinner(named(list, "op_master_volume", 1))) != NULL)
	{
		value = (short)PIN(spinner->parameters.list.selected_index, 0, 2);
		if (value != audio_extra_loaded.voice_chat)
		{
			audio_extra_loaded.voice_chat = value;
			setting_write("audio.voice_chat", audio_voice_values[value]);
		}
	}
	if ((spinner = row_spinner(named(list, "op_master_volume", 2))) != NULL)
	{
		value = (short)PIN(spinner->parameters.list.selected_index, 0, 10);
		if (value != audio_extra_loaded.voice_volume)
		{
			audio_extra_loaded.voice_volume = value;
			snprintf(text, sizeof(text), "%g", value / 10.0);
			setting_write("audio.voice_volume", text);
		}
	}
	if ((spinner = row_spinner(named(list, "op_environmental_sound", 2))) != NULL &&
		spinner->parameters.list.selected_index != audio_extra_loaded.resampler)
	{
		audio_extra_loaded.resampler = spinner->parameters.list.selected_index;
		setting_write("audio.resampler", audio_extra_loaded.resampler ? "linear" : "sinc");
	}
}

static void audio_show(struct widget_instance *list, boolean defaults)
{
	struct widget_instance *spinner;
	char text[64];
	short master = audio_step_read("audio.volume", defaults);
	short effects = audio_step_read("audio.effects_volume", defaults);
	short music = audio_step_read("audio.music_volume", defaults);
	short reverb = 1;

	if (setting_text("audio.reverb", text, sizeof(text), defaults))
		reverb = !strcmp(text, "true") ? 1 : 0;
	audio_extra_show(list, defaults);
	if ((spinner = named(list, "master_volume_spinner", 0)) != NULL)
		spinner->parameters.list.selected_index = master;
	if ((spinner = named(list, "effects_volume_spinner", 0)) != NULL)
		spinner->parameters.list.selected_index = effects;
	if ((spinner = named(list, "music_volume_spinner", 0)) != NULL)
		spinner->parameters.list.selected_index = music;
	if ((spinner = named(list, "environmental_sound_spinner", 0)) != NULL)
		spinner->parameters.list.selected_index = reverb;
	if (!defaults)
	{
		audio_loaded.master = master;
		audio_loaded.effects = effects;
		audio_loaded.music = music;
		audio_loaded.reverb = reverb;
	}
}

static void audio_step_save(struct widget_instance *spinner, short *loaded, char const *name)
{
	short step;
	char text[16];

	if (!spinner)
		return;
	step = (short)PIN(spinner->parameters.list.selected_index, 0, 10);
	if (step == *loaded)
		return;
	snprintf(text, sizeof(text), "%g", step / 10.0);
	setting_write(name, text);
	*loaded = step;
}

/* port: the rows with no port setting behind them are removed (hardware acceleration, sound quality, sound
variety), and environmental sound takes the first free place */
static void audio_trim(struct widget_instance *list)
{
	static char const *const unused_rows[] =
	{
		"op_software_buffers", "op_supplementary_buffers", "op_sound_variety"
	};
	struct widget_instance *row;
	short index;

	for (index = 0; index < (short)NUMBEROF(unused_rows); index++)
	{
		row = named(list, unused_rows[index], 0);
		if (row)
			ui_widget_delete(row);
	}
	row = named(list, "op_environmental_sound", 0);
	if (row)
		row->vertical_offset = 163;
}

static void audio_save(struct widget_instance *list)
{
	struct widget_instance *reverb = named(list, "environmental_sound_spinner", 0);

	audio_extra_save(list);
	audio_device_save(list);
	audio_step_save(named(list, "master_volume_spinner", 0), &audio_loaded.master, "audio.volume");
	audio_step_save(named(list, "effects_volume_spinner", 0), &audio_loaded.effects, "audio.effects_volume");
	audio_step_save(named(list, "music_volume_spinner", 0), &audio_loaded.music, "audio.music_volume");
	if (reverb && reverb->parameters.list.selected_index != audio_loaded.reverb)
	{
		audio_loaded.reverb = reverb->parameters.list.selected_index;
		setting_write("audio.reverb", audio_loaded.reverb ? "true" : "false");
	}
}

/* NONE if the function is not this screen's, else its result */
short pc_menu_audio_function(struct widget_instance *widget, word function)
{
	struct widget_instance *list = audio_list_of(widget);

	if (!list)
		return NONE;
	if (function == 123)
	{
		audio_trim(list);
		audio_inject(list);
		audio_show(list, FALSE);
	}
	else if (function == 124)
		audio_save(list);
	else if (function == 134)
		audio_show(list, TRUE);
	else
		return NONE;
	return TRUE;
}

/* port: a Custom Edition map's video screen is Halo PC's own: created 114, OK 125, Defaults 135. Its rows
(resolution, refresh rate, frame rate, specular, shadows, decals, particles, texture quality, gamma) have no
port setting behind them, so they are replaced by the port's own: display mode, resolution, window size,
V-Sync and frame rate limit (copies of the specular row, which has a string list) */
int platform_display_resolutions(long *widths, long *heights, int maximum);
int platform_window_sizes(long *widths, long *heights, int maximum);

#define VIDEO_MAX 64
#define VIDEO_CHOICES 5

struct video_choice
{
	wchar_t shown[VIDEO_MAX][24];
	wchar_t const *pointers[VIDEO_MAX];
	char real[VIDEO_MAX][24];
	short count;
	short loaded;
};

static struct video_choice video_choices[VIDEO_CHOICES];
static char const *const video_keys[VIDEO_CHOICES] =
{
	"display.mode", "display.resolution", "display.window_size", "display.vsync", "display.max_fps"
};

static void video_choice_add(struct video_choice *choice, char const *shown, char const *real)
{
	short at;

	if (choice->count >= VIDEO_MAX)
		return;
	for (at = 0; shown[at] && at < 23; at++)
		choice->shown[choice->count][at] = (wchar_t)(unsigned char)shown[at];
	choice->shown[choice->count][at] = 0;
	snprintf(choice->real[choice->count], sizeof(choice->real[0]), "%s", real);
	choice->pointers[choice->count] = choice->shown[choice->count];
	choice->count++;
}

/* the sizes from the smallest to the largest */
static void video_sizes_sort(long *widths, long *heights, short count)
{
	short index, other;

	for (index = 1; index < count; index++)
	{
		long width = widths[index];
		long height = heights[index];

		for (other = index - 1; other >= 0 && (widths[other] * heights[other] > width * height ||
			(widths[other] * heights[other] == width * height && widths[other] > width)); other--)
		{
			widths[other + 1] = widths[other];
			heights[other + 1] = heights[other];
		}
		widths[other + 1] = width;
		heights[other + 1] = height;
	}
}

static void video_choices_read(void)
{
	long widths[VIDEO_MAX], heights[VIDEO_MAX];
	char shown[32], real[32];
	short index, added;

	memset(video_choices, 0, sizeof(video_choices));
	video_choice_add(&video_choices[0], "FULLSCREEN", "fullscreen");
	video_choice_add(&video_choices[0], "BORDERLESS", "borderless");
	video_choice_add(&video_choices[0], "WINDOWED", "windowed");
	added = (short)platform_display_resolutions(widths, heights, VIDEO_MAX - 2);
	if (added < 0)
		added = 0;
	if (added > VIDEO_MAX - 2)
		added = VIDEO_MAX - 2;
	video_sizes_sort(widths, heights, added);
	for (index = 0; index < added; index++)
	{
		snprintf(shown, sizeof(shown), "%ld x %ld", widths[index], heights[index]);
		snprintf(real, sizeof(real), "%ldx%ld", widths[index], heights[index]);
		video_choice_add(&video_choices[1], shown, real);
	}
	video_choice_add(&video_choices[1], "NATIVE", "native");
	added = (short)platform_window_sizes(widths, heights, VIDEO_MAX);
	if (added < 0)
		added = 0;
	if (added > VIDEO_MAX)
		added = VIDEO_MAX;
	video_sizes_sort(widths, heights, added);
	for (index = 0; index < added; index++)
	{
		snprintf(shown, sizeof(shown), "%ld x %ld", widths[index], heights[index]);
		snprintf(real, sizeof(real), "%ldx%ld", widths[index], heights[index]);
		video_choice_add(&video_choices[2], shown, real);
	}
	if (!video_choices[2].count)
		video_choice_add(&video_choices[2], "1280 x 960", "1280x960");
	video_choice_add(&video_choices[3], "OFF", "false");
	video_choice_add(&video_choices[3], "ON", "true");
	video_choice_add(&video_choices[4], "AUTO", "0");
	video_choice_add(&video_choices[4], "30", "30");
	video_choice_add(&video_choices[4], "60", "60");
	video_choice_add(&video_choices[4], "120", "120");
	video_choice_add(&video_choices[4], "144", "144");
	video_choice_add(&video_choices[4], "165", "165");
	video_choice_add(&video_choices[4], "240", "240");
	video_choice_add(&video_choices[4], "NONE", "-1");
}

static struct widget_instance *video_list_of(struct widget_instance *widget)
{
	struct widget_instance *top;

	if (!strcmp(widget->name, "video_options_menu"))
		return widget;
	top = screen_of(widget);
	return !strcmp(top->name, "video_settings_screen") ? named(top, "video_options_menu", 0) : NULL;
}

static struct widget_instance *ce_video_list;

static boolean video_inject(struct widget_instance *list)
{
	struct widget_instance *source = named(list, "op_specular", 0);
	struct widget_instance *bar = named(list, "video_button_bar", 0);
	struct widget_instance *last_old;
	struct widget_instance *row;
	struct widget_instance *next;

	if (!named(list, "op_resolution", 0) || !source || !bar || !bar->previous)
		return FALSE;
	last_old = bar->previous;
	video_choices_read();
	if (!ui_widget_port_inject_row(source, bar, 73, L"DISPLAY MODE:",
		L"Fullscreen takes the display; borderless covers\r\nit with a window. F11 switches to the window.",
		video_choices[0].pointers, video_choices[0].count))
		goto failed;
	if (!ui_widget_port_inject_row(source, bar, 99, L"RESOLUTION:",
		L"What fullscreen and borderless draw at. Native\r\nis the display's own.",
		video_choices[1].pointers, video_choices[1].count))
		goto failed;
	if (!ui_widget_port_inject_row(source, bar, 125, L"WINDOW SIZE:",
		L"The size of the window, for the windowed mode.",
		video_choices[2].pointers, video_choices[2].count))
		goto failed;
	if (!ui_widget_port_inject_row(source, bar, 151, L"V-SYNC:",
		L"Wait for the display between frames, so that\r\nthe picture never tears.",
		video_choices[3].pointers, video_choices[3].count))
		goto failed;
	if (!ui_widget_port_inject_row(source, bar, 177, L"FRAME RATE LIMIT:",
		L"With V-Sync off, the most frames a second.\r\nAuto: twice the display's refresh rate.",
		video_choices[4].pointers, video_choices[4].count))
		goto failed;
	/* the map's own rows go (the list's focus is on the first of them: it moves to the first new row,
	as deleting a widget leaves the focus pointing at it) */
	for (row = list->child; row; row = next)
	{
		next = row->next;
		ui_widget_delete(row);
		if (row == last_old)
			break;
	}
	list->focused_child = list->child;
	list->parameters.list.selected_index = 0;
	ce_video_list = list;
	return TRUE;

failed:
	/* Leave the original controls and focus intact if any clone failed. */
	while (last_old ? last_old->next != bar : list->child != bar)
		ui_widget_delete(last_old ? last_old->next : list->child);
	return FALSE;
}


static void video_show(struct widget_instance *list, boolean defaults)
{
	short choice, index;

	for (choice = 0; choice < VIDEO_CHOICES; choice++)
	{
		struct widget_instance *spinner = row_spinner(named(list, "op_specular", choice));
		char text[128];
		short found = 0;

		if (!spinner)
			continue;
		if (setting_text(video_keys[choice], text, sizeof(text), defaults))
		{
			for (index = 0; index < video_choices[choice].count; index++)
			{
				if (!strcmp(text, video_choices[choice].real[index]))
					found = index;
			}
		}
		spinner->parameters.list.selected_index = found;
		if (!defaults)
			video_choices[choice].loaded = found;
	}
}

static void video_save(struct widget_instance *list)
{
	short choice;
	boolean changed = FALSE;

	for (choice = 0; choice < VIDEO_CHOICES; choice++)
	{
		struct widget_instance *spinner = row_spinner(named(list, "op_specular", choice));
		short index;

		if (!spinner)
			continue;
		index = spinner->parameters.list.selected_index;
		if (index < 0 || index >= video_choices[choice].count || index == video_choices[choice].loaded)
			continue;
		video_choices[choice].loaded = index;
		setting_write(video_keys[choice], video_choices[choice].real[index]);
		changed = TRUE;
	}
	if (changed)
		platform_display_apply();
}

/* NONE if the function is not this screen's, else its result */
short pc_menu_video_function(struct widget_instance *widget, word function, boolean *widget_deleted)
{
	struct widget_instance *list = video_list_of(widget);

	if (!list)
		return NONE;
	if (function == 114)
	{
		ce_video_list = NULL;
		if (!video_inject(list))
			return FALSE;
		video_show(list, FALSE);
	}
	else if (function == 125)
	{
		if (ce_video_list != list)
			return FALSE;
		/* (saved, then back, as the audio screen's OK does: its own opens another widget) */
		video_save(list);
		ui_widget_port_go_back(widget);
		*widget_deleted = TRUE;
	}
	else if (function == 135 && ce_video_list == list)
		video_show(list, TRUE);
	else
		return NONE;
	return TRUE;
}

/* port: a Custom Edition map's network screen is Halo PC's own: created 166, OK 167, Defaults 187. Its rows
(connection type, server port, client port) are all removed, and the port's own are made as copies of the
connection type row (it has a string list): internet play, join from clipboard, the multiplayer HUD's four,
and an entry row that opens a second page of the same screen (streamer mode's four). A page is built by
deleting the rows of the other (a hidden row of a map's own list would still take the focus). */
struct net_row
{
	char const *key;
	wchar_t const *label;
	wchar_t const *help;
	wchar_t const *const *shown;
	char const *const *real;
	short count;
	short page;
	short value;
	short loaded;
};

static wchar_t const *const net_on_off_names[2] = { L"OFF", L"ON" };
static char const *const net_on_off_values[2] = { "false", "true" };
static wchar_t const *const net_players_names[4] = { L"ALL", L"ALLIES", L"ENEMIES", L"NONE" };
static char const *const net_players_values[4] = { "all", "allies", "enemies", "none" };
static wchar_t const *const net_size_names[6] = { L"0.5x", L"0.75x", L"1x", L"1.25x", L"1.5x", L"2x" };
static char const *const net_size_values[6] = { "0.5", "0.75", "1", "1.25", "1.5", "2" };
static wchar_t const *const net_layout_names[2] = { L"TEAMS", L"BY SCORE" };
static char const *const net_layout_values[2] = { "teams", "score" };
static wchar_t const *const net_open_names[2] = { L"OPEN", L"OPEN" };
static char const *const net_open_values[2] = { "", "" };
static wchar_t const *const net_level_names[4] = { L"OFF", L"SEVERE", L"STRONG+", L"MILD+" };
static char const *const net_level_values[4] = { "off", "severe", "strong", "mild" };

#define NET_ENTRY 6

static struct net_row net_rows[] =
{
	{ "network.online", L"INTERNET PLAY:",
		L"Host and join games over the internet by invite\r\nlinks; off keeps to the local network.",
		net_on_off_names, net_on_off_values, 2, 0, 0, 0 },
	{ "network.join_from_clipboard", L"JOIN FROM CLIPBOARD:",
		L"Join the game of an invite link copied before\r\nswitching to the game.",
		net_on_off_names, net_on_off_values, 2, 0, 0, 0 },
	{ "display.player_names", L"PLAYER NAMES:",
		L"In multiplayer, whose names are drawn above\r\ntheir heads.",
		net_players_names, net_players_values, 4, 0, 0, 0 },
	{ "display.player_name_scale", L"PLAYER NAME SIZE:", L"How large the players' names are drawn.",
		net_size_names, net_size_values, 6, 0, 0, 0 },
	{ "display.scoreboard_team_layout", L"SCOREBOARD LAYOUT:",
		L"A team game's scoreboard: a column for each\r\nteam, or every player in order of score.",
		net_layout_names, net_layout_values, 2, 0, 0, 0 },
	{ "display.scoreboard_background", L"SCOREBOARD PANEL:",
		L"Draw a panel behind the scoreboard, for clearer\r\ntext.",
		net_on_off_names, net_on_off_values, 2, 0, 0, 0 },
	/* (the entry row: stepping it opens the second page) */
	{ NULL, L"STREAMER MODE:",
		L"Hide offensive words in names and text from\r\nother players, maps and mods, as stars.\r\nStep it left or right to open.",
		net_open_names, net_open_values, 2, 0, 0, 0 },
	{ "streamer.enabled", L"STREAMER MODE:",
		L"Hide offensive words in names and text from\r\nother players, maps and mods, as stars.",
		net_on_off_names, net_on_off_values, 2, 1, 0, 0 },
	{ "streamer.slurs", L"CENSOR SLURS:",
		L"Slurs about race, sexuality, gender and\r\ndisability. STRONG+ and MILD+ include the\r\nlevels above them.",
		net_level_names, net_level_values, 4, 1, 0, 0 },
	{ "streamer.swearing", L"CENSOR SWEARING:",
		L"Swearing and sexual words (STRONG+ leaves the\r\nmildest showing).",
		net_level_names, net_level_values, 4, 1, 0, 0 },
	{ "streamer.insults", L"CENSOR INSULTS:",
		L"Name-calling and insults that are not slurs.",
		net_level_names, net_level_values, 4, 1, 0, 0 },
};

void pc_menu_video_description(struct widget_instance *list);

/* the row the injected rows are copies of, kept: the first page's rows are deleted when the second is built */
static struct widget_instance net_template;
static short net_page;

static struct widget_instance *net_list_of(struct widget_instance *widget)
{
	struct widget_instance *top = screen_of(widget);

	if (!strcmp(widget->name, "network_options_menu"))
		return widget;
	return !strcmp(top->name, "network_settings_screen") ? named(top, "network_options_menu", 0) : NULL;
}

/* a row's place among the rows of its page */
static short net_nth(short row)
{
	short index, nth = 0;

	for (index = 0; index < row; index++)
	{
		if (net_rows[index].page == net_rows[row].page)
			nth++;
	}
	return nth;
}

static struct widget_instance *ce_network_list;

static struct widget_instance *net_row_spinner(struct widget_instance *list, short row)
{
	return ce_network_list == list && net_rows[row].page == net_page ? row_spinner(named(list, "op_connection_type", net_nth(row))) : NULL;
}

/* the values the settings have (or, with defaults, their defaults), by the place of each among its row's */
static void net_values_read(boolean defaults)
{
	short row, index;

	for (row = 0; row < (short)NUMBEROF(net_rows); row++)
	{
		char text[64];
		short found = 0;

		if (!net_rows[row].key)
			continue;
		if (setting_text(net_rows[row].key, text, sizeof(text), defaults))
		{
			for (index = 0; index < net_rows[row].count; index++)
			{
				if (!strcmp(text, net_rows[row].real[index]) ||
					(atof(text) > 0.0 && atof(text) == atof(net_rows[row].real[index])))
				{
					found = index;
				}
			}
		}
		net_rows[row].value = found;
		if (!defaults)
			net_rows[row].loaded = found;
	}
}

/* the spinners of the page shown set to the values held */
static void net_apply(struct widget_instance *list)
{
	short row;

	for (row = 0; row < (short)NUMBEROF(net_rows); row++)
	{
		struct widget_instance *spinner = net_row_spinner(list, row);

		if (spinner && net_rows[row].key)
			spinner->parameters.list.selected_index = net_rows[row].value;
	}
}

/* the values the spinners of the page shown have, held (before the page changes, or OK) */
static void net_capture(struct widget_instance *list)
{
	short row;

	for (row = 0; row < (short)NUMBEROF(net_rows); row++)
	{
		struct widget_instance *spinner = net_row_spinner(list, row);

		if (spinner && net_rows[row].key)
			net_rows[row].value = (short)PIN(spinner->parameters.list.selected_index, 0, net_rows[row].count - 1);
	}
}

/* the rows of a page made (those there deleted, with the map's own), the focus on its row `focus` */
static boolean net_build(struct widget_instance *list, short page, short focus)
{
	struct widget_instance *source = named(list, "op_connection_type", 0);
	struct widget_instance *bar = named(list, "network_button_bar", 0);
	struct widget_instance *row;
	struct widget_instance *last_old;
	struct widget_instance *next;
	short index, nth = 0;

	if (!source || !bar)
		return FALSE;
	net_template = *source;
	last_old = bar->previous;
	for (index = 0; index < (short)NUMBEROF(net_rows); index++)
	{
		if (net_rows[index].page != page)
			continue;
		if (!ui_widget_port_inject_row(&net_template, bar, (short)(73 + 30 * nth), net_rows[index].label,
			net_rows[index].help, net_rows[index].shown, net_rows[index].count))
			goto failed;
		nth++;
	}
	for (row = list->child; row; row = next)
	{
		next = row->next;
		ui_widget_delete(row);
		if (row == last_old)
			break;
	}
	ce_network_list = list;
	net_page = page;
	/* (deleting a widget leaves the focus pointing at it) */
	row = list->child;
	for (index = 0; row && row->next && index < focus; index++)
		row = row->next;
	list->focused_child = row;
	list->parameters.list.selected_index = index;
	net_apply(list);
	return TRUE;

failed:
	while (last_old->next != bar)
		ui_widget_delete(last_old->next);
	return FALSE;
}


/* back (the button, and B: ui_widget.c): from the second page to the first; FALSE on the first, which the
screen then leaves as usual */
boolean pc_menu_network_back(struct widget_instance *widget)
{
	struct widget_instance *list;

	if (!widget || !widget->name || net_page == 0 || strcmp(screen_of(widget)->name, "network_settings_screen"))
		return FALSE;
	list = net_list_of(widget);
	if (!list)
		return FALSE;
	net_capture(list);
	net_build(list, 0, NET_ENTRY);
	return TRUE;
}

/* NONE if the function is not this screen's, else its result */
short pc_menu_network_function(struct widget_instance *widget, word function)
{
	struct widget_instance *list = net_list_of(widget);
	short row;

	if (!list)
		return NONE;
	if (function == 166)
	{
		ce_network_list = NULL;
		net_page = 0;
		net_values_read(FALSE);
		return net_build(list, 0, 0);
	}
	else if (function == 167)
	{
		if (ce_network_list != list)
			return FALSE;
		net_capture(list);
		for (row = 0; row < (short)NUMBEROF(net_rows); row++)
		{
			if (net_rows[row].key && net_rows[row].value != net_rows[row].loaded)
			{
				net_rows[row].loaded = net_rows[row].value;
				setting_write(net_rows[row].key, net_rows[row].real[net_rows[row].value]);
			}
		}
	}
	else if (function == 187)
	{
		net_values_read(TRUE);
		net_apply(list);
	}
	else
		return NONE;
	return TRUE;
}

/* port: A on the network screen's entry row (its spinner is hidden, ui_widget.c calls this): steps it, which
opens the second page. FALSE for any other widget. */
boolean pc_menu_network_open(struct widget_instance *widget)
{
	struct widget_instance *list;
	struct widget_instance *entry;

	if (!widget || !widget->name || net_page != 0)
		return FALSE;
	list = net_list_of(widget);
	if (!list)
		return FALSE;
	entry = net_row_spinner(list, NET_ENTRY);
	if (!entry || entry != widget)
		return FALSE;
	entry->parameters.list.selected_index = 1;
	return TRUE;
}

/* port: the network screen's side text (Halo PC's game data function 58, each frame): the entry row stepped
opens the second page; the text is the video screen's, moved below the rows added
(ui_widget_port_description_clear) */
void pc_menu_network_description(struct widget_instance *list)
{
	extern void ui_widget_port_description_clear(struct widget_instance *list);
	struct widget_instance *entry = net_row_spinner(list, NET_ENTRY);

	if (entry)
		entry->visible = FALSE;

	if (entry && entry->parameters.list.selected_index != 0)
	{
		net_capture(list);
		net_build(list, 1, 0);
	}
	ui_widget_port_description_clear(list);
	pc_menu_video_description(list);
}

/* port: the video screen's side text for the row with the focus (Halo PC's game data function 44) */
void pc_menu_video_description(struct widget_instance *list)
{
	extern short ui_widget_port_row_description(struct widget_instance *row);
	struct widget_instance *description = list->parameters.list.extended_description;
	short index;

	if (!description || description->type != 1 || !list->focused_child)
		return;
	index = ui_widget_port_row_description(list->focused_child);
	if (index != NONE)
		description->parameters.text_box.string_list_index = index;
}

/* port: the audio screen's side text for the row with the focus (Halo PC's game data function 49) */
void pc_menu_audio_description(struct widget_instance *list)
{
	extern short ui_widget_port_row_description(struct widget_instance *row);
	struct widget_instance *description = list->parameters.list.extended_description;
	struct widget_instance *row = list->focused_child;
	short index;

	{
		extern void ui_widget_port_description_clear(struct widget_instance *list);

		ui_widget_port_description_clear(list);
	}
	if (!description || description->type != 1 || !row)
		return;
	index = ui_widget_port_row_description(row);
	if (index == NONE)
	{
		if (!strcmp(row->name, "op_master_volume"))
			index = 0;
		else if (!strcmp(row->name, "op_effects_volume"))
			index = 1;
		else if (!strcmp(row->name, "op_music_volume"))
			index = 2;
		else if (!strcmp(row->name, "op_environmental_sound"))
			index = 5;
		else
			return;
	}
	description->parameters.text_box.string_list_index = index;
}
