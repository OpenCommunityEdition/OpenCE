/* Thread-safe handoff for host messages; Carbon dialogs run on the window
   owner's event thread rather than a networking or cache worker. */
#include "engine_messages.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#if defined(__APPLE__) && !defined(__LP64__)
#include <Carbon/Carbon.h>
#endif

static pthread_mutex_t mac_message_lock=PTHREAD_MUTEX_INITIALIZER;
static char mac_message_title[160];
static char mac_message_body[768];
static int mac_message_pending;

static void mac_message_copy(char *destination,size_t capacity,const char *source)
{
	size_t length=0;int truncated;
	if(!source)source="";
	while(length+1<capacity&&source[length])length++;
	truncated=source[length]!='\0';
	memcpy(destination,source,length);destination[length]='\0';
	/* Do not leave a truncated UTF-8 continuation sequence at the boundary. */
	if(truncated&&length) {
		while(length&&((unsigned char)destination[length-1]&0xc0)==0x80)length--;
		if(length) {
			unsigned char lead=(unsigned char)destination[length-1];
			if((lead&0xe0)==0xc0)length--;
			else if((lead&0xf0)==0xe0)length--;
			else if((lead&0xf8)==0xf0)length--;
		}
		destination[length]='\0';
	}
}

void platform_show_message(const char *title,const char *message)
{
	pthread_mutex_lock(&mac_message_lock);
	mac_message_copy(mac_message_title,sizeof(mac_message_title),title);
	mac_message_copy(mac_message_body,sizeof(mac_message_body),message);
	mac_message_pending=1;
	pthread_mutex_unlock(&mac_message_lock);
}

void mac_engine_message_dispatch(int visible_window)
{
	char title[sizeof(mac_message_title)],body[sizeof(mac_message_body)];
	int pending;
	if(!visible_window)return;
	pthread_mutex_lock(&mac_message_lock);
	pending=mac_message_pending;
	if(pending) {
		memcpy(title,mac_message_title,sizeof(title));
		memcpy(body,mac_message_body,sizeof(body));
		mac_message_pending=0;
	}
	pthread_mutex_unlock(&mac_message_lock);
	if(!pending)return;
#if defined(__APPLE__) && !defined(__LP64__)
	{
		CFStringRef title_string=CFStringCreateWithCString(NULL,title,kCFStringEncodingUTF8);
		CFStringRef body_string=CFStringCreateWithCString(NULL,body,kCFStringEncodingUTF8);
		DialogRef alert=NULL;DialogItemIndex item=0;OSStatus status=noErr;
		if(title_string&&body_string)
			status=CreateStandardAlert(kAlertStopAlert,title_string,body_string,NULL,&alert);
		else status=paramErr;
		if(title_string)CFRelease(title_string);
		if(body_string)CFRelease(body_string);
		if(status==noErr&&alert)status=RunStandardAlert(alert,NULL,&item);
		if(status!=noErr)fprintf(stderr,"Mac message: %s: %s\n",title,body);
	}
#else
	fprintf(stderr,"Mac message: %s: %s\n",title,body);
#endif
}
