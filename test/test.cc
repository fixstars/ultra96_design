#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define BUFSIZE (640*480*3)
#define FRAME_NUM 2
#define USE_USERPTR

#ifdef USE_USERPTR
unsigned char *userptr[FRAME_NUM][BUFSIZE];
#endif /* USE_USERPTR */

static int xioctl(int fd, int request, void *arg){
    int rc;

    do rc = ioctl(fd, request, arg);
    while (-1 == rc && EINTR == errno);
    return rc;
}

int main()
{
    int fd, rc;
    int w = 640, h = 480;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;
    void *user_frame[BUFSIZE];

    fd = open("/dev/video0", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open = %d, errno = %d\n", fd, errno);
        return -1;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    rc = xioctl(fd, VIDIOC_S_FMT, &fmt);
    printf("VIDIOC_S_FMT = %d\n", rc);

    memset(&buf, 0, sizeof(buf));
    #ifndef USE_USERPTR
    memset(&req, 0, sizeof(req));
    req.count = FRAME_NUM;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    rc = xioctl(fd, VIDIOC_REQBUFS, &req);
    printf("VIDIOC_REQBUFS = %d\n", rc);
    buf.memory = V4L2_MEMORY_MMAP;
    #else /* USE_USERPTR */
    buf.memory = V4L2_MEMORY_USERPTR;
    #endif /* USE_USERPTR */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for (int i = 0; i < FRAME_NUM; i++) {
        #ifndef USE_USERPTR
        buf.index = i;
        rc = xioctl(fd, VIDIOC_QUERYBUF, &buf);
        printf("VIDIOC_QUERYBUF = %d, i= %d, buf.m.offset = %d, buf.length = %d\n", rc, i, buf.m.offset, buf.length);
        user_frame[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        printf("user_frame[%d] = %p\n", i, user_frame[i]);
        rc = xioctl(fd, VIDIOC_QBUF, &buf);
        #else /* USE_USERPTR */
        buf.index = i;
        buf.m.userptr = (unsigned long)userptr[i];
        rc = xioctl(fd, VIDIOC_QBUF, &buf);
        #endif /* USE_USERPTR */
        printf("VIDIOC_QBUF = %d\n", rc);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rc = xioctl(fd, VIDIOC_STREAMON, &type);
    printf("VIDIOC_STREAMON = %d\n", rc);

    cv::Mat frame(h, w, CV_8UC3);
    for (int i = 0; i < 20; i++) {
        fd_set fds;
        struct timeval tv;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        rc = select(fd + 1, &fds, NULL, NULL, &tv);
        printf("select%d = %d\n", i, rc);
        if (FD_ISSET(fd, &fds)) {
            printf("FD_ISSET\n");
            rc = xioctl(fd, VIDIOC_DQBUF, &buf);
            printf("VIDIOC_DQBUF = %d, buf.index = %d\n", rc, buf.index);
            if (buf.index < FRAME_NUM) {
                #ifdef USE_USERPTR
                frame.data = (unsigned char *)userptr[buf.index];
                buf.m.userptr = (unsigned long)userptr[buf.index];
                #else /* !USE_USERPTR */
                frame.data = (unsigned char *)user_frame[buf.index];
                #endif /* !USE_USERPTR */
                std::stringstream ss;
                ss << "./" << i << ".png";
                cv::imwrite(ss.str(), frame);
                rc = xioctl(fd, VIDIOC_QBUF, &buf);
                printf("VIDIOC_QBUF = %d\n", rc);
            }
        } else {
            printf("!FD_ISSET\n");
        }
        printf("%d\n", i);
    }

    rc = xioctl(fd, VIDIOC_STREAMOFF, &type);
    printf("VIDIOC_STREAMOFF = %d\n", rc);
    close(fd);

    return 0;
}
