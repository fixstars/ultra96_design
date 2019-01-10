#include <stdio.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cam.h"

#define WIDTH  640
#define HEIGHT 480

int main()
{
    int rc;
    int w = WIDTH, h = HEIGHT;
    unsigned char *buf;

    rc = v4l2init(w, h, V4L2_PIX_FMT_RGB24);
    if (rc < 0) {
        fprintf(stderr, "v4l2init = %d\n", rc);
        return -1;
    }

    cv::Mat frame(h, w, CV_8UC3);
    for (int i = 0; i < 20; i++) {
        printf("frame %d\n", i);
        rc = v4l2grab(&buf);
        if (rc < 0) {
            fprintf(stderr, "v4l2grab = %d\n", rc);
            return -1;
        }
        frame.data = buf;
        std::stringstream ss;
        ss << "./" << i << ".png";
        cv::imwrite(ss.str(), frame);
        rc = v4l2release(rc);
        if (rc < 0) {
            fprintf(stderr, "v4l2release = %d\n", rc);
            return -1;
        }
    }

    rc = v4l2end();
    if (rc < 0) {
        fprintf(stderr, "v4l2release = %d\n", rc);
        return -1;
    }

    return 0;
}
