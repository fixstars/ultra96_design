OPENCV_DIR=./opencv-3.4.1
INCLUDE=-I${OPENCV_DIR}/build -I${OPENCV_DIR}/modules/core/include -I${OPENCV_DIR}/modules/highgui/include -I${OPENCV_DIR}/modules/imgproc/include -I${OPENCV_DIR}/modules/imgcodecs/include -I${OPENCV_DIR}/modules/videoio/include
LIB=-L${OPENCV_DIR}/build/lib -lstdc++ -lopencv_core -lopencv_imgcodecs -lopencv_highgui

all: rgbtest yuvtest rgbvideo uvc_camera_test

rgbtest: rgbtest.cc cam.cc
	g++ -g -o $@ $^ ${INCLUDE} ${LIB}

yuvtest: yuvtest.cc cam.cc
	g++ -g -o $@ $^ ${INCLUDE} ${LIB}

rgbvideo: rgbvideo.cc cam.cc
	g++ -g -o $@ $^ ${INCLUDE} ${LIB}

uvc_camera_test: uvc_camera_test.cc
	g++ -g -o $@ $< camera_umd/uvc_camera/src/uvc_cam.cpp ${INCLUDE} -I./camera_umd/uvc_camera/include/ ${LIB}

clean:
	rm rgbtest yuvtest rgbvideo uvc_camera_test

