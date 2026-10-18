/* Carbon clipboard access for the native menu text-field callbacks. */
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#include <string.h>

int platform_clipboard_get(char *text,int size)
{
	PasteboardRef pasteboard=NULL;
	PasteboardItemID item=NULL;
	ItemCount count=0;
	CFDataRef data=NULL;
	const UInt8 *bytes;
	CFIndex length;
	size_t copied;
	int result=0;
	if(!text||size<=0)return 0;
	text[0]='\0';
	if(PasteboardCreate(kPasteboardClipboard,&pasteboard)!=noErr||!pasteboard)return 0;
	PasteboardSynchronize(pasteboard);
	if(PasteboardGetItemCount(pasteboard,&count)==noErr&&count>0&&
		PasteboardGetItemIdentifier(pasteboard,1,&item)==noErr&&item&&
		PasteboardCopyItemFlavorData(pasteboard,item,kUTTypeUTF8PlainText,&data)==noErr&&data) {
		bytes=CFDataGetBytePtr(data);length=CFDataGetLength(data);
		if(bytes&&length>0) {
			copied=(size_t)length<(size_t)size-1?(size_t)length:(size_t)size-1;
			/* Do not return a partial UTF-8 character when the engine buffer is
			   smaller than the clipboard text. */
			if(copied<(size_t)length)
				while(copied>0&&(bytes[copied]&0xc0)==0x80)copied--;
			memcpy(text,bytes,copied);text[copied]='\0';result=copied>0;
		}
	}
	if(data)CFRelease(data);
	CFRelease(pasteboard);
	return result;
}

void platform_clipboard_set(const char *text)
{
	PasteboardRef pasteboard=NULL;
	CFDataRef data=NULL;
	static char item_identity;
	const char *value=text?text:"";
	if(PasteboardCreate(kPasteboardClipboard,&pasteboard)!=noErr||!pasteboard)return;
	data=CFDataCreate(kCFAllocatorDefault,(const UInt8 *)value,(CFIndex)strlen(value));
	if(data) {
		if(PasteboardClear(pasteboard)==noErr) {
			PasteboardPutItemFlavor(pasteboard,(PasteboardItemID)&item_identity,
				kUTTypeUTF8PlainText,data,kPasteboardFlavorNoFlags);
			PasteboardSynchronize(pasteboard);
		}
		CFRelease(data);
	}
	CFRelease(pasteboard);
}
