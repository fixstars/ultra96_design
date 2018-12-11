 #include "uvc_cam/uvc_cam.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

int main(int argc, char *argv[])
{
    unsigned char *mem;
    uint32_t bytes;
    int idx;
    uvc_cam::Cam cam("/dev/video0");

    cam.enumerate();
    cam.v4l2_query(0, "testquery");
    cam.set_v4l2_control(0, 0, "testset");
    //cam.set_control(0, 0);
    cam.set_motion_thresholds(0, 0);
    
    cv::Mat frame(480, 640, CV_8UC3);
    for (int i = 0; i < 20; i++) {
        idx = cam.grab(&mem, bytes);
        printf("i = %d, idx = %d\n", i, idx);
        frame.data = mem;
        std::stringstream ss;
        ss << "./" << i << ".png";
        cv::imwrite(ss.str(), frame);
        cam.release(idx);
    }
    
    return 0;
}
