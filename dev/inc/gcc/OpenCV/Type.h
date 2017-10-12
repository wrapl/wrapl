#ifndef OPENCV_TYPE_H
#define OPENCV_TYPE_H

#include <Std/Type.h>

#define RIVA_MODULE OpenCV$Type
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	const int Code;
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
