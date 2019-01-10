#include <stdio.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cam.h"

#define WIDTH  640
#define HEIGHT 480

// saturate input into [0, 255]
inline unsigned char sat(float f)
{
    return (unsigned char)( f >= 255 ? 255 : (f < 0 ? 0 : f));
}

static void yuyv_to_rgb(unsigned char *yuyv, unsigned char *rgb)
{
    for (int i = 0; i < WIDTH * HEIGHT * 2; i += 4) {
        *rgb++ = sat(yuyv[i]+1.402f  *(yuyv[i+3]-128));
        *rgb++ = sat(yuyv[i]-0.34414f*(yuyv[i+1]-128)-0.71414f*(yuyv[i+3]-128));
        *rgb++ = sat(yuyv[i]+1.772f  *(yuyv[i+1]-128));
        *rgb++ = sat(yuyv[i+2]+1.402f*(yuyv[i+3]-128));
        *rgb++ = sat(yuyv[i+2]-0.34414f*(yuyv[i+1]-128)-0.71414f*(yuyv[i+3]-128));
        *rgb++ = sat(yuyv[i+2]+1.772f*(yuyv[i+1]-128));
    }
}

int main()
{
    int rc;
    int w = WIDTH, h = HEIGHT;
    unsigned char *buf;
    unsigned char rgb_buf[WIDTH*HEIGHT*3];

    rc = v4l2init(w, h, V4L2_PIX_FMT_YUYV);
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
        yuyv_to_rgb(buf, rgb_buf);
        frame.data = (unsigned char *)rgb_buf;
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
