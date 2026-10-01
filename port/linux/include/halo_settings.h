/*
HALO_SETTINGS.H

The settings of the menus' HALO SETTINGS (source/interface/ui_widget.c), each
one of config.toml's (port/linux/src/port_config.c), in the order of its
sections: display, audio, input. Changing one changes it in the running game
at once and writes it into the file (port/linux/src/halo_settings.c).
Desktop builds only: Android has no HALO SETTINGS.
*/

#ifndef HALO_SETTINGS_H
#define HALO_SETTINGS_H

enum
{
	_halo_setting_fullscreen,
	_halo_setting_vsync,
	_halo_setting_interpolation,
	_halo_setting_direct_camera,
	_halo_setting_window_scale,
	_halo_setting_audio,
	_halo_setting_master_volume,
	_halo_setting_mouse_sensitivity,
	_halo_setting_mouse_aim_assist,
	_halo_setting_invert_mouse,
	NUMBER_OF_HALO_SETTINGS
};

/* whether the menus offer HALO SETTINGS; 0 on Android */
int halo_settings_available(void);
/* the setting as it is in effect now (fullscreen as F11 last left it), as
the menu shows it: ON or OFF, a number or a percentage */
void halo_setting_text(int setting, char *text, int size);
/* whether the setting is on or off, rather than a number in a range */
int halo_setting_is_switch(int setting);
/* turns a switch on or off, or steps a number down (direction < 0) or up,
now and in config.toml; 0 if it could not change (at the end of its range,
or refused), and is as it was */
int halo_setting_step(int setting, int direction);

#endif
