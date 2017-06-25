#include <Std.h>
#include <Riva.h>
#include <OpenCV/Type.h>
#include <OpenCV/Mat.h>
#include <opencv2/opencv.hpp>

TYPE(T);

ASYMBOL(New);

AMETHOD(New, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, OpenCV$Type$T) {
	OpenCV$Mat$t *Matrix = (OpenCV$Mat$t *)Riva$Memory$alloc(sizeof(OpenCV$Mat$t));
	Matrix->Type = T;
	int Rows = Std$Integer$get_small(Args[0].Val);
	int Cols = Std$Integer$get_small(Args[1].Val);
	OpenCV$Type$t *Type = (OpenCV$Type$t *)Args[2].Val;
	new (&Matrix->Value) cv::Mat(Rows, Cols, Type->Code);
	Result->Val = (Std$Object$t *)Matrix;
	return SUCCESS;
}

METHOD("rows", TYP, T) {
	OpenCV$Mat$t *Matrix = (OpenCV$Mat$t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Matrix->Value.rows);
	return SUCCESS;
}

METHOD("cols", TYP, T) {
	OpenCV$Mat$t *Matrix = (OpenCV$Mat$t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Matrix->Value.cols);
	return SUCCESS;
}
