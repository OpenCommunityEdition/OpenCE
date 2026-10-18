#include <stdio.h>
int platform_clipboard_get(char *text,int size);
int main(void)
{
	char text[8]="x";
	int failures=0;
	if(platform_clipboard_get(NULL,sizeof(text))!=0)failures++;
	if(platform_clipboard_get(text,0)!=0)failures++;
	if(text[0]!='x')failures++;
	printf("mac clipboard boundary: 3 checks, %d failures\n",failures);
	return failures?1:0;
}
