#include <iostream>
#include"V4L2CaptureVideo.h"

using namespace std;

int main(int argc, char const *argv[])
{
    V4L2CaptureVideo * VCL = new V4L2CaptureVideo();


    cout << VCL ->gettimeofmsecond() << endl;
    VCL->open_device();
    VCL->init_device();
    VCL->start_capturing();
    VCL->file_open();
    VCL->mainloop();
    VCL->file_close();
    VCL->stop_capturing();
    VCL->uninit_device();
    VCL->close_device();
    cout << VCL ->gettimeofmsecond() << endl;
    return 0;
}


