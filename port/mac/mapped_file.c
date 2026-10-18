#include "mapped_file.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
int mac_mapped_file_open(const char *path,size_t limit,struct mac_mapped_file *out,const char **error)
{
    int fd;struct stat st;void *view;size_t size;
    if(error)*error="mapped file output";
    if(!out)return 0;out->data=NULL;out->size=0;
    if(!path||!limit){if(error)*error="mapped file arguments";return 0;}
    fd=open(path,O_RDONLY);if(fd<0){if(error)*error="mapped file open";return 0;}
    if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_size<=0||
       (uint64_t)st.st_size>(uint64_t)limit||(uint64_t)st.st_size>(uint64_t)(size_t)-1) {
        close(fd);if(error)*error="mapped file size or type";return 0;
    }
    size=(size_t)st.st_size;view=mmap(NULL,size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    if(view==MAP_FAILED){if(error)*error="mapped file address space";return 0;}
    out->data=view;out->size=size;if(error)*error=NULL;return 1;
}
const void *mac_mapped_file_span(const struct mac_mapped_file *file,size_t offset,size_t length)
{
    if(!file||!file->data||offset>file->size||length>file->size-offset)return NULL;
    return file->data+offset;
}
void mac_mapped_file_close(struct mac_mapped_file *file)
{
    if(!file)return;if(file->data)munmap((void*)file->data,file->size);
    file->data=NULL;file->size=0;
}
