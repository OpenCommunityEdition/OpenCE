#include <stdio.h>
#include <ApplicationServices/ApplicationServices.h>
#include <string.h>
#include "mac_display_modes.h"

int platform_display_resolutions(long *,long *,int);
int platform_window_sizes(long *,long *,int);
void platform_video_window_size(int *width,int *height){if(width)*width=1024;if(height)*height=768;}
void platform_display_apply(void);
int config_boolean(const char *name){(void)name;return 1;}
long config_integer(const char *name){(void)name;return 2;}
static const char *mode_value="windowed",*size_value="1280x720";
const char *config_string(const char *name){return !strcmp(name,"display.mode")?mode_value:size_value;}
static unsigned resized_width,resized_height;static int resize_calls;
int mac_window_resize_active(unsigned width,unsigned height)
{resized_width=width;resized_height=height;resize_calls++;return 1;}

/* the window's side of the display mode (mac_window.c), as a window or a borderless one */
int mac_window_display_plan(void){return !strcmp(mode_value,"windowed")?0:1;}
int mac_window_fullscreen_active(void){return 0;}
int mac_window_set_borderless_active(int borderless){(void)borderless;return 1;}
int mac_window_set_vsync_active(int on){(void)on;return 0;}
void mac_dsound_output_device_apply(void){}
void platform_show_message(const char *title,const char *message){(void)title;(void)message;}

int main(void)
{
	long widths[64],heights[64];int failures=0,checks=0,count,index;
	checks++;if(platform_display_resolutions(NULL,heights,64)!=0)failures++;
	checks++;if(platform_window_sizes(widths,NULL,64)!=0)failures++;
	count=platform_display_resolutions(widths,heights,64);
	checks++;if(count<0||count>64)failures++;
	for(index=0;index<count;index++) {
		checks++;
		if(widths[index]<640||heights[index]<480)failures++;
		if(index)checks++;
		if(index&&(widths[index-1]<widths[index]||
			(widths[index-1]==widths[index]&&heights[index-1]<heights[index])))failures++;
	}
	/* the modes Video Setup offers: no smaller than 640x480, no interlaced or unsafe mode, no 16-bit ones */
	checks++;if(!mac_display_mode_usable(1280,800,32,1,0))failures++;
	checks++;if(!mac_display_mode_usable(640,480,32,1,0))failures++;
	checks++;if(!mac_display_mode_usable(1024,768,0,1,0))failures++;
	checks++;if(mac_display_mode_usable(640,400,32,1,0))failures++;
	checks++;if(mac_display_mode_usable(512,384,32,1,0))failures++;
	checks++;if(mac_display_mode_usable(1920,1080,32,1,1))failures++;
	checks++;if(mac_display_mode_usable(1920,1080,32,0,0))failures++;
	checks++;if(mac_display_mode_usable(1024,768,16,1,0))failures++;
	/* widescreen is wider than 3:2 */
	checks++;if(mac_display_mode_is_wide(1024,768)||mac_display_mode_is_wide(1280,1024)||mac_display_mode_is_wide(1440,960))failures++;
	checks++;if(!mac_display_mode_is_wide(1280,800)||!mac_display_mode_is_wide(1920,1080)||!mac_display_mode_is_wide(2560,1080))failures++;
	/* the main display's own list: each size once, the desktop's among them, all usable */
	{
		long w[64],h[64];int found=0,n=platform_display_resolutions(w,h,64),a,b;
		long desktop_w=(long)CGDisplayPixelsWide(CGMainDisplayID()),desktop_h=(long)CGDisplayPixelsHigh(CGMainDisplayID());
		for(a=0;a<n;a++) {
			if(w[a]==desktop_w&&h[a]==desktop_h)found=1;
			for(b=a+1;b<n;b++){checks++;if(w[a]==w[b]&&h[a]==h[b])failures++;}
		}
		checks++;if(!found)failures++;
		printf("display modes offered (%d):",n);for(a=0;a<n;a++)printf(" %ldx%ld%s",w[a],h[a],mac_display_mode_is_wide(w[a],h[a])?"w":"");printf("\n");
	}
	count=platform_window_sizes(widths,heights,64);
	checks++;if(count<1||count>64)failures++;
	for(index=0;index<count;index++){checks++;if(widths[index]<=0||heights[index]<=0)failures++;}
	platform_display_apply();
	checks++;
	if(resize_calls!=1||resized_width!=1280||resized_height!=720)failures++;
	size_value="";platform_display_apply();
	checks++;
	if(resize_calls!=2||resized_width!=640||resized_height!=480)failures++;
	size_value="not-a-size";platform_display_apply();
	checks++;
	if(resize_calls!=3||resized_width!=640||resized_height!=480)failures++;
	mode_value="borderless";platform_display_apply();
	checks++;
	if(resize_calls!=3)failures++;
	printf("mac display providers: %d checks, %d failures (resolutions/window sizes bounded)\n",checks,failures);
	return failures?1:0;
}
