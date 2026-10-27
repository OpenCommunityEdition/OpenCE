/* Chip identity, not the phone brand or GPU family. In particular, Mali
and PowerVR do not identify the SoC vendor. No device permissions needed. */
#include "../include/android_render_policy.h"

static int chip_lower(int c)
{
	return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

static int chip_space(int c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static const char *chip_trim(const char *s)
{
	if (!s)
		return "";
	while (chip_space(*s))
		s++;
	return s;
}

static int chip_word(const char *s, const char *word)
{
	s = chip_trim(s);
	while (*word)
	{
		if (chip_lower((unsigned char)*s++) != *word++)
			return 0;
	}
	return !*s || chip_space(*s) || *s == '-' || *s == '(';
}

static int chip_number(const char *s, const char *prefix, int digits)
{
	s = chip_trim(s);
	while (*prefix)
	{
		if (chip_lower((unsigned char)*s++) != *prefix++)
			return 0;
	}
	while (digits--)
	{
		if (*s < '0' || *s > '9')
			return 0;
		s++;
	}
	/* Chip revisions often append letters or a dash. */
	return !*s || chip_space(*s) || *s == '-' ||
		(chip_lower(*s) >= 'a' && chip_lower(*s) <= 'z');
}

static int chip_identity(const char *s)
{
	if (chip_word(s, "qualcomm") || chip_word(s, "qcom") ||
		chip_word(s, "qti") || chip_word(s, "snapdragon") ||
		chip_number(s, "sm", 4) || chip_number(s, "sdm", 3) ||
		chip_number(s, "msm", 4) || chip_number(s, "apq", 4))
		return _android_chip_snapdragon;
	if (chip_word(s, "mediatek") || chip_number(s, "mt", 4))
		return _android_chip_mediatek;
	if (chip_word(s, "google") || chip_word(s, "tensor") ||
		chip_word(s, "samsung") || chip_word(s, "exynos") ||
		chip_word(s, "hisilicon") || chip_word(s, "kirin") ||
		chip_word(s, "unisoc") || chip_word(s, "spreadtrum") ||
		chip_word(s, "nvidia") || chip_word(s, "rockchip") ||
		chip_word(s, "allwinner") || chip_word(s, "amlogic") ||
		chip_word(s, "intel") || chip_word(s, "broadcom") ||
		chip_number(s, "exynos", 4) || chip_number(s, "samsungexynos", 4) ||
		chip_number(s, "gs", 3))
		return _android_chip_other;
	return _android_chip_unknown;
}

int android_chip_classify(const char *manufacturer, const char *model,
	const char *hardware, const char *platform)
{
	int vendor = chip_identity(manufacturer);
	int chip = chip_identity(model);

	/* Explicit SoC fields take precedence over older board properties.
	Conflicting identities retain the conservative unknown policy. */
	if (vendor && chip && vendor != chip)
		return _android_chip_unknown;
	if (vendor || chip)
		return vendor ? vendor : chip;
	vendor = chip_identity(hardware);
	chip = chip_identity(platform);
	if (vendor && chip && vendor != chip)
		return _android_chip_unknown;
	return vendor ? vendor : chip;
}
