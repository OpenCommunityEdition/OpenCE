/* Android gyro settings; the default keeps the saved touch layout's behavior. */
#ifndef HALO_TOUCH_GYRO_H
#define HALO_TOUCH_GYRO_H

enum
{
	_touch_gyro_legacy = -1,
	_touch_gyro_off,
	_touch_gyro_on,
	_touch_gyro_zoom
};

int touch_gyro_mode(void);
void touch_gyro_apply(float *yaw, float *pitch);
void halo_linux_gyro_filter(int zoomed, float *yaw, float *pitch);

#endif
