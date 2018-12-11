OPENCV_DIR=./opencv-3.4.1/

all: rgbtest yuvtest uvc_camera_test

rgbtest: rgbtest.cc
	g++ -g -o $@ $< -I${OPENCV_DIR}/build -I${OPENCV_DIR}/modules/core/include -I${OPENCV_DIR}/modules/highgui/include -I${OPENCV_DIR}/modules/imgproc/include -I${OPENCV_DIR}/modules/imgcodecs/include -I${OPENCV_DIR}/modules/videoio/include -L . -L${OPENCV_DIR}/build/lib -lstdc++ -lopencv_core -lopencv_imgcodecs

yuvtest: yuvtest.cc
	g++ -g -o $@ $< -I${OPENCV_DIR}/build -I${OPENCV_DIR}/modules/core/include -I${OPENCV_DIR}/modules/highgui/include -I${OPENCV_DIR}/modules/imgproc/include -I${OPENCV_DIR}/modules/imgcodecs/include -I${OPENCV_DIR}/modules/videoio/include -L . -L${OPENCV_DIR}/build/lib -lstdc++ -lopencv_core -lopencv_imgcodecs

uvc_camera_test: uvc_camera_test.cc
	g++ -g -o $@ $< camera_umd/uvc_camera/src/uvc_cam.cpp -Icamera_umd/uvc_camera/include -I${OPENCV_DIR}/build -I${OPENCV_DIR}/modules/core/include -I${OPENCV_DIR}/modules/highgui/include -I${OPENCV_DIR}/modules/imgproc/include -I${OPENCV_DIR}/modules/imgcodecs/include -I${OPENCV_DIR}/modules/videoio/include -L . -L${OPENCV_DIR}/build/lib -lstdc++ -lopencv_core -lopencv_imgcodecs

clean:
	rm rgbtest yuvtest uvc_camera_test

