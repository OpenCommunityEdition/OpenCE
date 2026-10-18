/* Native display-mode and window-size choices for the Mac settings menu. */
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void platform_video_window_size(int *width,int *height);
extern int mac_window_resize_active(unsigned width,unsigned height);
extern int mac_window_set_vsync_active(int on) __attribute__((weak));
extern int mac_window_fullscreen_active(void) __attribute__((weak));
extern int mac_window_display_plan(void) __attribute__((weak));
extern int mac_window_set_borderless_active(int borderless) __attribute__((weak));
/* engine_messages.c: a dialog on the window's thread */
extern void platform_show_message(const char *title,const char *message) __attribute__((weak));
/* audio.output_device (mac_dsound_native, from tools/mac_dsound_view.py) */
extern void mac_dsound_output_device_apply(void) __attribute__((weak));
extern int config_boolean(const char *name);
extern long config_integer(const char *name);
extern const char *config_string(const char *name);

static int display_add(long *widths,long *heights,int count,int maximum,long width,long height)
{
	int index;
	if(width<=0||height<=0)return count;
	for(index=0;index<count;index++)if(widths[index]==width&&heights[index]==height)return count;
	if(count<maximum){widths[count]=width;heights[count]=height;return count+1;}
	return count;
}

static void display_sort(long *widths,long *heights,int count)
{
	int index;
	for(index=1;index<count;index++) {
		long width=widths[index],height=heights[index];int place=index;
		while(place>0&&(widths[place-1]<width||
			(widths[place-1]==width&&heights[place-1]<height))) {
			widths[place]=widths[place-1];heights[place]=heights[place-1];place--;
		}
		widths[place]=width;heights[place]=height;
	}
}

int platform_display_resolutions(long *widths,long *heights,int maximum)
{
	CGDirectDisplayID display=CGMainDisplayID();CFArrayRef modes;CGRect bounds;int count=0,index;
	if(!widths||!heights||maximum<=0)return 0;
	bounds=CGDisplayBounds(display);modes=CGDisplayAvailableModes(display);
	for(index=0;modes&&index<CFArrayGetCount(modes);index++) {
		CFDictionaryRef mode=(CFDictionaryRef)CFArrayGetValueAtIndex(modes,index);
		CFNumberRef width_number=(CFNumberRef)CFDictionaryGetValue(mode,kCGDisplayWidth);
		CFNumberRef height_number=(CFNumberRef)CFDictionaryGetValue(mode,kCGDisplayHeight);
		int width=0,height=0;
		if(!width_number||!height_number||
			!CFNumberGetValue(width_number,kCFNumberIntType,&width)||
			!CFNumberGetValue(height_number,kCFNumberIntType,&height))continue;
		if(width<640||height<480||width>bounds.size.width||height>bounds.size.height)continue;
		count=display_add(widths,heights,count,maximum,width,height);
	}
	/* (the system's array: CGDisplayAvailableModes follows the Get rule, and a
	   release here left the second call with a freed one) */
	display_sort(widths,heights,count);
	return count;
}

int platform_window_sizes(long *widths,long *heights,int maximum)
{
	static const short offered[][2]={
		{640,480},{800,600},{1024,768},{1280,960},{1600,1200},{1920,1440},{2560,1920},
		{1280,800},{1440,900},{1680,1050},{1920,1200},{2560,1600},
		{1280,720},{1600,900},{1920,1080},{2560,1440},{3840,2160},
		{2560,1080},{3440,1440},{3840,1600},{5120,2160}
	};
	CGRect bounds;int count=0,index,width,height;
	if(!widths||!heights||maximum<=0)return 0;
	bounds=CGDisplayBounds(CGMainDisplayID());
	for(index=0;index<(int)(sizeof(offered)/sizeof(offered[0]));index++) {
		width=offered[index][0];height=offered[index][1];
		if((index==0)||(width<=bounds.size.width&&height<=bounds.size.height))
			count=display_add(widths,heights,count,maximum,width,height);
	}
	platform_video_window_size(&width,&height);
	count=display_add(widths,heights,count,maximum,width,height);
	return count;
}

static int display_parse_size(const char *value,unsigned *width,unsigned *height)
{
	char *end;long parsed_width,parsed_height;
	if(!value||!*value)return 0;
	parsed_width=strtol(value,&end,10);
	if(end==value||*end!='x')return 0;
	value=end+1;parsed_height=strtol(value,&end,10);
	if(end==value||*end||parsed_width<640||parsed_height<480||
		parsed_width>8192||parsed_height>8192)return 0;
	*width=(unsigned)parsed_width;*height=(unsigned)parsed_height;
	return 1;
}

void platform_display_apply(void)
{
	static int warned_unsupported_mode;
	const char *size=config_string("display.window_size");
	unsigned width,height;long scale;
	int plan,running_exclusive,notice=0;
	if(mac_dsound_output_device_apply)mac_dsound_output_device_apply();
	/* (the swap interval changes at once, in either mode) */
	if(mac_window_set_vsync_active&&mac_window_set_vsync_active(config_boolean("display.vsync")?1:0))
		fprintf(stderr,"Mac display apply: vsync %s\n",config_boolean("display.vsync")?"on":"off");
	/* port: what display.mode asks: 0 a window, 1 a borderless window over the
	   display (mac_window.c: "borderless", and "fullscreen" at the desktop's own
	   mode), 2 the display captured at a mode of its own (display.resolution, or
	   display.fullscreen_exclusive). The first two are the same context and a
	   window moved and resized, and apply at once. The third has its own
	   context, which cannot take a window's place (the game's frame buffers,
	   bound to the context, would not follow it), and applies when the game
	   starts: the player is told, once for each such choice, rather than seeing
	   nothing happen. */
	plan=mac_window_display_plan?mac_window_display_plan():0;
	running_exclusive=mac_window_fullscreen_active&&mac_window_fullscreen_active()!=0;
	if(running_exclusive) {
		unsigned full_width=0,full_height=0;
		const char *resolution=config_string("display.resolution");
		if(plan!=2)notice=1;
		else if(display_parse_size(resolution,&full_width,&full_height)&&
			(full_width!=(unsigned)CGDisplayPixelsWide(kCGDirectMainDisplay)||
			 full_height!=(unsigned)CGDisplayPixelsHigh(kCGDirectMainDisplay)))notice=3;
	} else if(plan==2)notice=2;
	if(notice!=warned_unsupported_mode) {
		warned_unsupported_mode=notice;
		if(notice) {
			fprintf(stderr,"Mac display apply: the display mode (%s) applies when the game starts\n",
				notice==1?"window or borderless":notice==2?"exclusive fullscreen":"fullscreen resolution");
			if(platform_show_message)
				platform_show_message("Restart PowerSpark",
					notice==1?"A window or a borderless window takes effect the next time the game starts.":
					notice==2?"This full screen mode (display.resolution or display.fullscreen_exclusive) takes effect the next time the game starts. Quit and open PowerSpark again.":
					"The full screen resolution takes effect the next time the game starts.");
		}
	}
	/* (an exclusive display has no window to move, and one that is asked for
	   waits for the start: the window stays as it is) */
	if(running_exclusive||plan==2)return;
	if(mac_window_set_borderless_active) {
		if(!mac_window_set_borderless_active(plan==1))
			fprintf(stderr,"Mac display apply: cannot switch the window to %s\n",plan==1?"borderless":"a window");
		else if(plan==1)return;
	} else if(plan==1)return;
	if(!display_parse_size(size,&width,&height)) {
		/* port: no size chosen: the game's own (display.window_scale is the
		   desktop builds', and its default of 2 is in the Mac's config.toml) */
		scale=1;
		width=(unsigned)(640*scale);height=(unsigned)(480*scale);
	}
	if(!mac_window_resize_active(width,height))
		fprintf(stderr,"Mac display apply: cannot resize the active window to %ux%u\n",width,height);
}
