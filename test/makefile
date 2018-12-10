OPENCV_DIR=./opencv-3.4.1/

all: test yuvtest

test: test.cc
	g++ -g -o $@ $< -I${OPENCV_DIR}/build -I${OPENCV_DIR}/modules/core/include -I${OPENCV_DIR}/modules/highgui/include -I${OPENCV_DIR}/modules/imgproc/include -I${OPENCV_DIR}/modules/imgcodecs/include -I${OPENCV_DIR}/modules/videoio/include -L . -L${OPENCV_DIR}/build/lib -lstdc++ -lopencv_core -lopencv_imgcodecs

yuvtest: yuvtest.cc
	g++ -g -o $@ $< -I${OPENCV_DIR}/build -I${OPENCV_DIR}/modules/core/include -I${OPENCV_DIR}/modules/highgui/include -I${OPENCV_DIR}/modules/imgproc/include -I${OPENCV_DIR}/modules/imgcodecs/include -I${OPENCV_DIR}/modules/videoio/include -L . -L${OPENCV_DIR}/build/lib -lstdc++ -lopencv_core -lopencv_imgcodecs

clean:
	rm test yuvtest

