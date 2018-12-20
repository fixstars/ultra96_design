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
    int rc, key;
    unsigned char *buf;
    unsigned char rgb_buf[WIDTH * HEIGHT * 3];
    int w = WIDTH, h = HEIGHT;

    rc = v4l2init(w, h, V4L2_PIX_FMT_RGB24);
    if (rc < 0) {
        fprintf(stderr, "v4l2init = %d\n", rc);
        return -1;
    }

    cv::Mat img(cv::Size(w, h), CV_8UC3);
    const char window[16] = "camera";
    cv::namedWindow(window);

    while (1) {
        rc = v4l2grab(&buf);
        if (rc < 0) {
            fprintf(stderr, "v4l2grab = %d\n", rc);
            return -1;
        }
        memcpy(rgb_buf, buf, WIDTH * HEIGHT * 3);
        rc = v4l2release(rc);
        if (rc < 0) {
            fprintf(stderr, "v4l2release = %d\n", rc);
            return -1;
        }
        img.data = rgb_buf;
        cv::imshow(window, img);
        key = cv::waitKey(1);
        if (key == 'q') {
            break;
        }
    }
    rc = v4l2end();
    if (rc < 0) {
        fprintf(stderr, "v4l2end = %d\n", rc);
        return -1;
    }

    return 0;
}
