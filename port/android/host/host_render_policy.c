#include "host.h"
#include "../include/android_render_policy.h"
#include <sys/system_properties.h>

int host_android_chip_family(void)
{
	char manufacturer[PROP_VALUE_MAX] = { 0 }, model[PROP_VALUE_MAX] = { 0 };
	char hardware[PROP_VALUE_MAX] = { 0 }, platform[PROP_VALUE_MAX] = { 0 };
	int family;

	/* The public SoC properties back Build.SOC_MANUFACTURER/SOC_MODEL on
	Android 12+. Board identifiers are a conservative older-device fallback. */
	__system_property_get("ro.soc.manufacturer", manufacturer);
	__system_property_get("ro.soc.model", model);
	__system_property_get("ro.hardware", hardware);
	__system_property_get("ro.board.platform", platform);
	family = android_chip_classify(manufacturer, model, hardware, platform);
	host_logf(HOST_LOG_INFO, "render chip: manufacturer=%s model=%s hardware=%s platform=%s family=%d",
		manufacturer, model, hardware, platform, family);
	return family;
}
