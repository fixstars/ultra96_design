#include <iostream>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <unistd.h>
#include "camctrl.h"


int main()
{
    int32_t w = 640;
    int32_t h = 480;
    int32_t c = 3;

    if (caminit(SENSOR_OV5640, RESOLUTION_640_480, 3, w * h * c)) {
        return -1;
    }

    cv::Mat frame(h, w, CV_8UC3);
    for (int i=0; i<10; ++i) {
        std::cout << __LINE__ << std::endl;
        frame.data = (unsigned char *)get_frame_buffer();
        printf("frame.data = 0x%llx\n", frame.data);
        std::stringstream ss;
        ss << "./" << i << ".png";
        cv::imwrite(ss.str(), frame);
        sleep(1);
    }

    return 0;
}
