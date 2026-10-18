/* See launcher_branding.h. */
#include "launcher_branding.h"
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void lb_defaults(struct lb_branding *b)
{
	memset(b, 0, sizeof(*b));
	snprintf(b->feedback_app, sizeof(b->feedback_app), "powerspark");
}

static int dict_string(CFDictionaryRef d, const char *key, char *out, size_t size)
{
	CFStringRef k = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
	CFTypeRef v = k ? CFDictionaryGetValue(d, k) : NULL;
	int ok = 0;

	if (v && CFGetTypeID(v) == CFStringGetTypeID())
		ok = CFStringGetCString((CFStringRef)v, out, (CFIndex)size, kCFStringEncodingUTF8) != 0;
	if (k)
		CFRelease(k);
	if (!ok && size)
		out[0] = 0;
	return ok;
}

/* an address that a link may open */
static int web_address(const char *url)
{
	return strncmp(url, "https://", 8) == 0 || strncmp(url, "http://", 7) == 0;
}

/* a plain file name: no folder, nothing hidden */
static int plain_name(const char *name)
{
	return name[0] && name[0] != '.' && !strchr(name, '/') && !strchr(name, ':');
}

static void file_in(char *out, size_t size, const char *folder, const char *name)
{
	struct stat st;

	out[0] = 0;
	if (!plain_name(name) || snprintf(out, size, "%s/%s", folder, name) >= (int)size || stat(out, &st) != 0 ||
		!S_ISREG(st.st_mode))
		out[0] = 0;
}

int lb_load(struct lb_branding *b, const char *resources)
{
	char path[PATH_MAX], folder[PATH_MAX], text[PATH_MAX];
	const char *override = getenv("HALO_MAC_BRANDING");
	FILE *f;
	long length;
	unsigned char *bytes;
	CFDataRef data;
	CFPropertyListRef plist;
	CFTypeRef links;
	CFIndex i;

	lb_defaults(b);
	if (override && *override)
	{
		char *slash;

		snprintf(path, sizeof(path), "%s", override);
		snprintf(folder, sizeof(folder), "%s", override);
		slash = strrchr(folder, '/');
		if (slash)
			*slash = 0;
		else
			strcpy(folder, ".");
	}
	else
	{
		snprintf(folder, sizeof(folder), "%s", resources);
		snprintf(path, sizeof(path), "%s/branding.plist", resources);
	}
	f = fopen(path, "rb");
	if (!f)
		return 0;
	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (length <= 0 || length > 65536 || (bytes = (unsigned char *)malloc((size_t)length)) == NULL)
	{
		fclose(f);
		return 0;
	}
	if (fread(bytes, 1, (size_t)length, f) != (size_t)length)
	{
		fclose(f);
		free(bytes);
		return 0;
	}
	fclose(f);
	data = CFDataCreate(NULL, bytes, length);
	free(bytes);
	if (!data)
		return 0;
	plist = CFPropertyListCreateFromXMLData(NULL, data, kCFPropertyListImmutable, NULL);
	CFRelease(data);
	if (!plist || CFGetTypeID(plist) != CFDictionaryGetTypeID())
	{
		if (plist)
			CFRelease(plist);
		return 0;
	}
	dict_string((CFDictionaryRef)plist, "PublisherText", b->publisher, sizeof(b->publisher));
	{
		char app[64], token[64], url[256];

		if (dict_string((CFDictionaryRef)plist, "FeedbackApp", app, sizeof(app)) && app[0])
			snprintf(b->feedback_app, sizeof(b->feedback_app), "%s", app);
		if (dict_string((CFDictionaryRef)plist, "FeedbackToken", token, sizeof(token)))
			snprintf(b->feedback_token, sizeof(b->feedback_token), "%s", token);
		if (dict_string((CFDictionaryRef)plist, "FeedbackURL", url, sizeof(url)) && strncmp(url, "https://", 8) == 0)
			snprintf(b->feedback_url, sizeof(b->feedback_url), "%s", url);
	}
	if (dict_string((CFDictionaryRef)plist, "LogoFile", text, sizeof(text)))
		file_in(b->logo_path, sizeof(b->logo_path), folder, text);
	if (dict_string((CFDictionaryRef)plist, "CAFile", text, sizeof(text)))
		file_in(b->ca_path, sizeof(b->ca_path), folder, text);
	{
		CFStringRef k = CFSTR("Links");

		links = CFDictionaryGetValue((CFDictionaryRef)plist, k);
	}
	if (links && CFGetTypeID(links) == CFArrayGetTypeID())
		for (i = 0; i < CFArrayGetCount((CFArrayRef)links) && b->link_count < LB_LINKS; ++i)
		{
			CFTypeRef one = CFArrayGetValueAtIndex((CFArrayRef)links, i);
			struct lb_link *l = &b->links[b->link_count];

			if (one && CFGetTypeID(one) == CFDictionaryGetTypeID() &&
				dict_string((CFDictionaryRef)one, "Title", l->title, sizeof(l->title)) && l->title[0] &&
				dict_string((CFDictionaryRef)one, "URL", l->url, sizeof(l->url)) && web_address(l->url))
				++b->link_count;
		}
	CFRelease(plist);
	return 1;
}

int lb_feedback_ready(const struct lb_branding *b)
{
	struct stat st;

	return b->feedback_url[0] && b->ca_path[0] && b->sender_path[0] && stat(b->sender_path, &st) == 0;
}
