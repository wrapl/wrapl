#include <Std.h>
#include <Agg/List.h>
#include <Math/Vector.h>
#include <Riva/Memory.h>
#include "clipper/cpp/clipper.hpp"
#include "clipper/cpp/clipper.cpp"

TYPE(T);

struct region_t {
	const Std$Type_t *Type;
	clipper::TPolyPolygon Polygons;
	region_t() : Type(T), Polygons() {};
};

GLOBAL_FUNCTION(New, 0) {
	region_t *Region = new region_t;
	if (Count > 0) {
		Agg$List_t *List = (Agg$List_t *)Args[0].Val;
		clipper::TPolygon Polygon(List->Length);
		int Index = 0;
		for (Agg$List_node *Node = List->Head; Node; Node = Node->Next) {
			Math$Vector_t *Point = (Math$Vector_t *)Node->Value;
			if ((Point->Type != Math$Vector$T) || (Point->Entries[0]->Type != Std$Real$T) || (Point->Entries[1]->Type != Std$Real$T)) {
				Result->Val = Std$Function$new_arg_type_message(Fun, Index + 1, Math$Vector$T, Point->Type);
				return MESSAGE;
			};
			Polygon[Index].X = ((Std$Real_t *)Point->Entries[0])->Value;
			Polygon[Index].Y = ((Std$Real_t *)Point->Entries[1])->Value;
			++Index;
		};
		Region->Polygons.push_back(Polygon);
	};
	Result->Val = (Std$Object_t *)Region;
	return SUCCESS;
};

METHOD("add", TYP, T, TYP, Agg$List$T) {
	region_t *Region = (region_t *)Args[0].Val;
	Agg$List_t *List = (Agg$List_t *)Args[1].Val;
	clipper::TPolygon Polygon(List->Length);
	int Index = 0;
	for (Agg$List_node *Node = List->Head; Node; Node = Node->Next) {
		Math$Vector_t *Point = (Math$Vector_t *)Node->Value;
		if ((Point->Type != Math$Vector$T) || (Point->Entries[0]->Type != Std$Real$T) || (Point->Entries[1]->Type != Std$Real$T)) {
			Result->Val = Std$Function$new_arg_type_message(Fun, Index + 1, Math$Vector$T, Point->Type);
			return MESSAGE;
		};
		Polygon[Index].X = ((Std$Real_t *)Point->Entries[0])->Value;
		Polygon[Index].Y = ((Std$Real_t *)Point->Entries[1])->Value;
		++Index;
	};
	Region->Polygons.push_back(Polygon);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("points", TYP, T) {
	region_t *Region = (region_t *)Args[0].Val;
	Std$Object_t *Lists = Agg$List$new(0);
	for (int I = 0; I < Region->Polygons.size(); ++I) {
		clipper::TPolygon &Polygon = Region->Polygons[I];
		Std$Object_t *List = Agg$List$new(0);
		for (int J = 0; J < Polygon.size(); ++J) {
			Math$Vector_t *Point = (Math$Vector_t *)Riva$Memory$alloc(sizeof(Math$Vector_t) + 2 * sizeof(Std$Object_t *));
			Point->Type = Math$Vector$T;
			Point->Length.Type = Std$Integer$SmallT;
			Point->Length.Value = 2;
			Point->Entries[0] = Std$Real$new(Polygon[J].X);
			Point->Entries[1] = Std$Real$new(Polygon[J].Y);
			Agg$List$put(List, (Std$Object_t *)Point);
		};
		Agg$List$put(Lists, List);
	};
	Result->Val = Lists;
	return SUCCESS;
};

METHOD("area", TYP, T) {
	region_t *Region = (region_t *)Args[0].Val;
	double Area = 0;
	for (int I = 0; I < Region->Polygons.size(); ++I) {
		Area += clipper::Area(Region->Polygons[I]);
	};
	Result->Val = Std$Real$new(Area);
	return SUCCESS;
};

TYPE(ClipperT);

struct clipper_t {
	const Std$Type_t *Type;
	clipper::Clipper Clipper;
	clipper_t() : Type(T), Clipper() {};
};

GLOBAL_FUNCTION(ClipperNew, 0) {
	Result->Val = (Std$Object_t *)new clipper_t;
	return SUCCESS;
};

TYPE(PolyTypeT);

struct polytype_t {
	const Std$Type_t *Type;
	clipper::TPolyType Value;
};

polytype_t PolyTypeSubject[] = {{PolyTypeT, clipper::ptSubject}};
polytype_t PolyTypeClip[] = {{PolyTypeT, clipper::ptClip}};

METHOD("add", TYP, ClipperT, TYP, T, TYP, PolyTypeT) {
	clipper_t *Clipper = (clipper_t *)Args[0].Val;
	region_t *Region = (region_t *)Args[1].Val;
	polytype_t *PolyType = (polytype_t *)Args[2].Val;
	Clipper->Clipper.AddPolyPolygon(Region->Polygons, PolyType->Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

TYPE(PolyFillTypeT);

struct polyfilltype_t {
	const Std$Type_t *Type;
	clipper::TPolyFillType Value;
};

polyfilltype_t PolyFillTypeEvenOdd[] = {{PolyFillTypeT, clipper::pftEvenOdd}};
polyfilltype_t PolyFillTypeNonZero[] = {{PolyFillTypeT, clipper::pftNonZero}};

TYPE(ClipTypeT);

struct cliptype_t {
	const Std$Type_t *Type;
	clipper::TClipType Value;
};

cliptype_t ClipTypeIntersection[] = {{ClipTypeT, clipper::ctIntersection}};
cliptype_t ClipTypeUnion[] = {{ClipTypeT, clipper::ctUnion}};
cliptype_t ClipTypeDifference[] = {{ClipTypeT, clipper::ctDifference}};
cliptype_t ClipTypeXor[] = {{ClipTypeT, clipper::ctXor}};

METHOD("execute", TYP, ClipperT, TYP, ClipTypeT, TYP, PolyFillTypeT, TYP, PolyFillTypeT) {
	clipper_t *Clipper = (clipper_t *)Args[0].Val;
	cliptype_t *ClipType = (cliptype_t *)Args[1].Val;
	polyfilltype_t *SubjectFillType = (polyfilltype_t *)Args[2].Val;
	polyfilltype_t *ClipFillType = (polyfilltype_t *)Args[3].Val;
	region_t *Region = new region_t;
	if (Clipper->Clipper.Execute(ClipType->Value, Region->Polygons, SubjectFillType->Value, ClipFillType->Value)) {
		Result->Val = (Std$Object_t *)Region;
		return SUCCESS;
	} else {
		return FAILURE;
	};
};
