#include <iostream>
#include <sys/time.h>
#include"V4L2CaptureVideo.h"

using namespace std;


V4L2CaptureVideo::V4L2CaptureVideo()
{
    dev_name = "/dev/video0";
    filename = "Test.yuv";
    count_frame = 100;
};

V4L2CaptureVideo::~V4L2CaptureVideo()
{
};

void V4L2CaptureVideo::open_device(){
    fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
};

void V4L2CaptureVideo::init_device(){
    struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

    if (-1 == my_ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        cout <<"查询V4L2属性失败！！"<<endl;
        exit(EXIT_FAILURE);
    }else
    {
        printf("driver:%s\n",cap.driver);
		printf("card:%s\n",cap.card);
		printf("bud_info:%s\n",cap.bus_info);
		printf("version:%u.%u.%u\n",(cap.version>>16)&0XFF,(cap.version>>8)&0XFF,(cap.version)&0XFF);
		printf("capabilities:%x\n",cap.capabilities);

        /* Values for 'capabilities' field */
        //#define V4L2_CAP_VIDEO_CAPTURE		0x00000001  /* Is a video capture device */
    }
    
    memset(&cropcap, 0, sizeof(cropcap));
    // VIDIOC_CROPCAP ：查询VIDIOC的Crop能力！! 但是注意：type = V4L2_BUF_TYPE_VIDEO_CAPTURE 要预先指定，否则查询失败！
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 == my_ioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = cropcap.type;
        crop.c    = cropcap.defrect; /* reset to default */
        //VIDIOC_S_CROP： 设置VIDIOC的Crop！
        if (-1 == my_ioctl(fd, VIDIOC_S_CROP, &crop)) {
			cout << "设置VIDIOC的Crop失败，忽略错误！！！" << endl;
		}
    }else{
        cout << "查询VIDIOC的Crop能力失败，但是这个失败是可以忽略的, 这段代码是多余的！!" << endl;
    }
    
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 320;
	fmt.fmt.pix.height = 240;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.field = V4L2_FIELD_NONE; //这个参数是设置场模式,经测验本机不支持其他的场格式！  /* this device has no fields ... */

	//VIDIOC_S_FMT : (Set)设置视频的格式
	if (-1 == my_ioctl(fd, VIDIOC_S_FMT, &fmt)){
		exit(EXIT_FAILURE);
	}

    struct v4l2_format fmt_get;
    memset(&fmt_get, 0, sizeof(fmt_get));
	fmt_get.type =  V4L2_BUF_TYPE_VIDEO_CAPTURE; //type 要提前设置，否则查询失败！！
	if (0 == my_ioctl(fd, VIDIOC_G_FMT, &fmt_get))
	{
		printf("fmt_get.fmt.pix.field:%u",fmt_get.fmt.pix.field);
	}
    init_mmap();
};

void V4L2CaptureVideo::init_mmap(){
    struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

    // 请求缓存(Request Buffers) 
    if(-1 == my_ioctl(fd, VIDIOC_REQBUFS, &req)){
        cout <<  "请求缓存失败！！" << endl;
        exit(EXIT_FAILURE);
    }

    //struct buffer * buffers ： buffers 是结构体指针变量
	buffers = (buffer *)calloc(req.count, sizeof(*buffers));
	n_buffers = req.count;
	for (size_t i = 0; i < n_buffers; i++) {//映射所有的缓存
		struct v4l2_buffer buf;
		memset(&buf , 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (-1 == my_ioctl(fd, VIDIOC_QUERYBUF, &buf)){
            cout << "VIDIOC_QUERYBUF失败！" << endl;
            exit(EXIT_FAILURE);
        }
		buffers[i].length = buf.length;
        //把每一个缓存buf的数据地址映射到buffers[i]的指向地点！
		buffers[i].start = mmap(NULL , buf.length,PROT_READ | PROT_WRITE ,MAP_SHARED, fd, buf.m.offset);
		if (MAP_FAILED == buffers[i].start){
            cout << "内存映射失败！！" <<endl;
            exit(EXIT_FAILURE);
        }
	}
}


void V4L2CaptureVideo::start_capturing(){
    uint32_t i = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (-1 == my_ioctl(fd, VIDIOC_QBUF, &buf)){//VIDIOC_QBUF：把帧放入队列！
            exit(EXIT_FAILURE);
        } 
	}
	
	if (-1 == my_ioctl(fd, VIDIOC_STREAMON, &type)){//启动或者停止数据流(VIDIOC_STREAMON,VIDIOC_STREAMOFF)
        exit(EXIT_FAILURE);
    } 
};

void V4L2CaptureVideo::file_open(){
    fp = fopen(filename, "wa+");
};

void V4L2CaptureVideo::mainloop(){
	for(int i = 0; i<count_frame; i++){
		while(true)
		{
			fd_set fds;
			struct timeval tv;
			int r;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			r = select(fd + 1, &fds, NULL, NULL, &tv);
            if (-1 == r) {
                if (EINTR == errno){
                    continue; //被其他信号打断，重新读取帧
                }else
                {
                    exit(EXIT_FAILURE);
                }
            }
            if (0 == r) {
                cout << "超时了，！"<< endl;
                exit(EXIT_FAILURE);
            }
			// fd是摄像头的文件描述符。		
			if (read_frame()){
                break;
            }
		}	
	}
};

int V4L2CaptureVideo::read_frame(){
    struct v4l2_buffer buf;
	uint32_t i;
	memset(&buf, 0 ,sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;				//前面Start函数已经把放进去buffer了！
	if (-1 == my_ioctl(fd, VIDIOC_DQBUF, &buf)) {  //VIDIOC_DQBUF ：把数据从缓存中读取出来！
        cout << "VIDIOC_DQBUF失败！！" << endl;
	}
	assert(buf.index < n_buffers);
	process_image(buffers[buf.index].start, buf.length);
	if (-1 == my_ioctl(fd, VIDIOC_QBUF, &buf)){//VIDIOC_QBUF ：把数据放回缓存队列中！
        exit(EXIT_FAILURE);
    }  
	return 1;
}

void  V4L2CaptureVideo::process_image(const void * p, int size){
    fwrite(p, size, 1, fp); //打开方式决定,这是向fp所指文件追加内容！
}


void V4L2CaptureVideo::file_close(){
    fclose(fp);
};

void V4L2CaptureVideo::stop_capturing(){
    enum v4l2_buf_type type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == my_ioctl(fd, VIDIOC_STREAMOFF, &type)){ //停止数据流：VIDIOC_STREAMOFF
		exit(EXIT_FAILURE);
	}
};

void V4L2CaptureVideo::uninit_device(){
    unsigned int i;
	for (i = 0; i < n_buffers; ++i){
		if (-1 == munmap(buffers[i].start, buffers[i].length)){//关闭内存映射！
            exit(EXIT_FAILURE);
        } 
	}
	free(buffers);
};

void V4L2CaptureVideo::close_device(){
    close(fd);
};


int V4L2CaptureVideo::my_ioctl(int fd, int request, void *arg){
    /**
     * 对于ioctl的调用，要注意对errno的判断，如何是被其他信号中断，
     * 即error等于EINTR的时候，要重新调用！
    ***/
    int rv=0;
    do{
        rv = ioctl(fd, request, arg);
    }while( -1 == rv && EINTR == errno);
    return rv;
};
void V4L2CaptureVideo::my_gettimeofday(long *sec, long *usec){
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) {
        *sec = time.tv_sec;
    } 
    if (usec) {
        *usec = time.tv_usec;
    }
}

int64_t V4L2CaptureVideo::gettimeofmsecond(){
    long int Seconds = 0; //秒
    long int Microseconds = 0; //微妙
    int64_t  nowmsecondtime = 0;
    my_gettimeofday(&Seconds, &Microseconds);
    nowmsecondtime = (Seconds) * 1000 + (Microseconds / 1000);
    return nowmsecondtime;
}

