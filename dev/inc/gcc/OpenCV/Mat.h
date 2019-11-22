#ifndef OPENCV_MAT_H
#define OPENCV_MAT_H

#include <Std/Type.h>

#define RIVA_MODULE OpenCV$Mat
#include <Riva-Header.h>

#include <opencv2/opencv.hpp>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	cv::Mat Value;
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
