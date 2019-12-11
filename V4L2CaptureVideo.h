#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             
#include <fcntl.h>             
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          
#include <linux/videodev2.h>



struct buffer{
    void * start;
    size_t length;
};


class V4L2CaptureVideo
{
private:
    const char * dev_name;
    const char * filename;
    int fd;
    buffer *buffers;
    uint8_t n_buffers;
    FILE * fp;
    int count_frame;

public:
    V4L2CaptureVideo();
    ~V4L2CaptureVideo();
    void open_device();
    void init_device();
    void init_mmap();
    void start_capturing();
    void file_open();
    void mainloop();
    int  read_frame();
    void process_image(const void * p, int size);
    void file_close();
    void stop_capturing();
    void uninit_device();
    void close_device();
    int  my_ioctl(int fd, int request, void *arg);
    void my_gettimeofday(long int *sec, long int *usec);
    int64_t gettimeofmsecond();
};
