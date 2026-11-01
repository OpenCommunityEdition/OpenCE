/* Android's optional gyro settings. Only input changes; player actions and
the network's simulation contract are unchanged. */
#include "touch_gyro.h"
#include "port_config.h"
#include <string.h>

int touch_gyro_mode(void)
{
#ifdef HALO_ANDROID
	const char *mode = config_string("input.gyro_aim");
	if (!strcmp(mode, "false")) return _touch_gyro_off;
	if (!strcmp(mode, "true")) return _touch_gyro_on;
	if (!strcmp(mode, "zoom")) return _touch_gyro_zoom;
	return _touch_gyro_legacy;
#else
	return _touch_gyro_legacy;
#endif
}

#ifdef HALO_ANDROID
static float gyro_scale(const char *setting, const char *invert)
{
	double value = config_real(setting);
	/* The comparisons reject NaN as well as out-of-range settings. */
	if (!(value >= 0.25 && value <= 10.0)) value = 1.0;
	return (float)(config_boolean(invert) ? -value : value);
}
#endif

void touch_gyro_apply(float *yaw, float *pitch)
{
#ifdef HALO_ANDROID
	int mode = touch_gyro_mode();
	if (mode == _touch_gyro_off)
	{
		*yaw = *pitch = 0.0f;
	}
	else if (mode != _touch_gyro_legacy)
	{
		*yaw *= gyro_scale("input.gyro_sensitivity_horizontal", "input.gyro_invert_horizontal");
		*pitch *= gyro_scale("input.gyro_sensitivity_vertical", "input.gyro_invert_vertical");
	}
#else
	(void)yaw;
	(void)pitch;
#endif
}

void halo_linux_gyro_filter(int zoomed, float *yaw, float *pitch)
{
	if (!zoomed && touch_gyro_mode() == _touch_gyro_zoom)
		*yaw = *pitch = 0.0f;
}
