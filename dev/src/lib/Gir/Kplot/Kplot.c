#include <Std.h>
#include <Riva/Memory.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <cairo/cairo.h>
#include <gdk/gdk.h>
#include <kplot.h>
#include <Gir/GObject/Object.h>
#include <Gir/Cairo/Cairo.h>
#include <Gir/Cairo/Pattern.h>
#include <Gir/Cairo/LineJoin.h>
#include <Gir/Cairo/FontSlant.h>
#include <Gir/Cairo/FontWeight.h>
#include <Gir/Gdk/RGBA.h>

typedef struct data_t {
	const Std$Type$t *Type;
	struct kdata *Value;
} data_t;

TYPE(DataT, Gir$GObject$Object$T);

TYPE(ArrayDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(ArrayNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	size_t Size = Std$Integer$get_small(Args[0].Val);
	data_t *Data = new(data_t);
	Data->Type = ArrayDataT;
	Data->Value = kdata_array_alloc(0, Size);
	RETURN(Data);
}

METHOD("add", TYP, ArrayDataT, TYP, Std$Integer$SmallT, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	size_t X = Std$Integer$get_small(Args[1].Val);
	double Y = Std$Real$get_value(Args[2].Val);
	kdata_array_add(Data->Value, X, Y);
	RETURN0;
}

static void riva_array_fill(size_t X, struct kpair *Pair, Std$Object$t *Function) {
	Std$Function$result Result[1];
	// TODO: Create pair object that accepts :x and :y
	Std$Function$call(Function, 2, Result, Std$Integer$new_small(X), 0);
}

METHOD("fill", TYP, ArrayDataT, ANY) {
	data_t *Data = (data_t *)Args[0].Val;
	kdata_array_fill(Data->Value, Args[1].Val, riva_array_fill);
	RETURN0;
}

METHOD("set", TYP, ArrayDataT, TYP, Std$Integer$SmallT, TYP, Std$Real$T, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	size_t Pos = Std$Integer$get_small(Args[1].Val);
	double X = Std$Real$get_value(Args[2].Val);
	double Y = Std$Real$get_value(Args[3].Val);
	kdata_array_set(Data->Value, Pos, X, Y);
	RETURN0;
}

TYPE(VectorDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(VectorNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	size_t Size = Std$Integer$get_small(Args[0].Val);
	data_t *Data = new(data_t);
	Data->Type = VectorDataT;
	Data->Value = kdata_vector_alloc(Size);
	RETURN(Data);
}

METHOD("append", TYP, VectorDataT, TYP, Std$Real$T, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	double X = Std$Real$get_value(Args[1].Val);
	double Y = Std$Real$get_value(Args[2].Val);
	kdata_vector_append(Data->Value, X, Y);
	RETURN0;
}

METHOD("set", TYP, VectorDataT, TYP, Std$Integer$SmallT, TYP, Std$Real$T, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	size_t Pos = Std$Integer$get_small(Args[1].Val);
	double X = Std$Real$get_value(Args[2].Val);
	double Y = Std$Real$get_value(Args[3].Val);
	kdata_vector_set(Data->Value, Pos, X, Y);
	RETURN0;
}

TYPE(StddevDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(StddevNew, 1) {
	CHECK_ARG_TYPE(0, DataT);
	data_t *Source = (data_t *)Args[0].Val;
	data_t *Data = new(data_t);
	Data->Type = StddevDataT;
	Data->Value = kdata_stddev_alloc(Data->Value);
	RETURN(Data);
}

METHOD("attach", TYP, StddevDataT, TYP, DataT) {
	data_t *Data = (data_t *)Args[0].Val;
	data_t *Source = (data_t *)Args[1].Val;
	kdata_stddev_attach(Data->Value, Source->Value);
	RETURN0;
}

TYPE(MeanDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(MeanNew, 1) {
	CHECK_ARG_TYPE(0, DataT);
	data_t *Source = (data_t *)Args[0].Val;
	data_t *Data = new(data_t);
	Data->Type = MeanDataT;
	Data->Value = kdata_mean_alloc(Data->Value);
	RETURN(Data);
}

METHOD("attach", TYP, MeanDataT, TYP, DataT) {
	data_t *Data = (data_t *)Args[0].Val;
	data_t *Source = (data_t *)Args[1].Val;
	kdata_mean_attach(Data->Value, Source->Value);
	RETURN0;
}

TYPE(HistDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(HistNew, 3) {
	CHECK_EXACT_ARG_TYPE(0, Std$Real$T);
	CHECK_EXACT_ARG_TYPE(1, Std$Real$T);
	CHECK_EXACT_ARG_TYPE(2, Std$Integer$SmallT);
	double RMin = Std$Real$get_value(Args[0].Val);
	double RMax = Std$Real$get_value(Args[1].Val);
	size_t Bins = Std$Integer$get_small(Args[2].Val);
	data_t *Data = new(data_t);
	Data->Type = HistDataT;
	Data->Value = kdata_hist_alloc(RMin, RMax, Bins);
	RETURN(Data);
}

METHOD("add", TYP, HistDataT, TYP, Std$Real$T, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	double X = Std$Real$get_value(Args[1].Val);
	double Y = Std$Real$get_value(Args[2].Val);
	kdata_hist_add(Data->Value, X, Y);
	RETURN(Data);
}

METHOD("set", TYP, HistDataT, TYP, Std$Real$T, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	double X = Std$Real$get_value(Args[1].Val);
	double Y = Std$Real$get_value(Args[2].Val);
	kdata_hist_set(Data->Value, X, Y);
	RETURN(Data);
}

TYPE(BufferDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(BufferNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	size_t Size = Std$Integer$get_small(Args[0].Val);
	data_t *Data = new(data_t);
	Data->Type = BufferDataT;
	Data->Value = kdata_buffer_alloc(Size);
	RETURN(Data);
}

METHOD("copy", TYP, BufferDataT, TYP, DataT) {
	data_t *Data = (data_t *)Args[0].Val;
	data_t *Src = (data_t *)Args[1].Val;
	kdata_buffer_copy(Data->Value, Src->Value);
	RETURN0;
}

TYPE(BucketDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(BucketNew, 3) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	double RMin = Std$Real$get_value(Args[0].Val);
	double RMax = Std$Real$get_value(Args[1].Val);
	data_t *Data = new(data_t);
	Data->Type = BucketDataT;
	Data->Value = kdata_bucket_alloc(RMin, RMax);
	RETURN(Data);
}

METHOD("add", TYP, BucketDataT, TYP, Std$Integer$SmallT, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	size_t Bucket = Std$Integer$get_small(Args[1].Val);
	double Y = Std$Real$get_value(Args[2].Val);
	kdata_bucket_add(Data->Value, Bucket, Y);
	RETURN0;
}

METHOD("set", TYP, BucketDataT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Real$T) {
	data_t *Data = (data_t *)Args[0].Val;
	size_t Bucket = Std$Integer$get_small(Args[1].Val);
	size_t X = Std$Integer$get_small(Args[2].Val);
	double Y = Std$Real$get_value(Args[3].Val);
	kdata_bucket_set(Data->Value, Bucket, X, Y);
	RETURN0;
}

SYMBOL($xmin, "xmin");
SYMBOL($xmax, "xmax");
SYMBOL($ymin, "ymin");
SYMBOL($ymax, "ymax");
SYMBOL($margins, "margins");
SYMBOL($margin, "margin");
SYMBOL($tic_labels, "tic_labels");
SYMBOL($tic_label_font, "tic_label_font");
SYMBOL($xtic_label_pad, "xtic_label_pad");
SYMBOL($ytic_label_pad, "ytic_label_pad");
SYMBOL($xtic_label_rot, "xtic_label_rot");
SYMBOL($xtic_label_fmt, "xtic_label_fmt");
SYMBOL($ytic_label_fmt, "ytic_label_fmt");
SYMBOL($borders, "borders");
SYMBOL($border, "border");
SYMBOL($colours, "colours");
SYMBOL($xtics, "xtics");
SYMBOL($ytics, "ytics");
SYMBOL($tics, "tics");
SYMBOL($tic, "tic");
SYMBOL($xaxis_label, "x_axis_label");
SYMBOL($xaxis_label2, "x_axis_label_2");
SYMBOL($yaxis_label, "y_axis_label");
SYMBOL($yaxis_label2, "y_axis_label_2");
SYMBOL($xaxis_label_rot, "x_axis_label_rot");
SYMBOL($yaxis_label_rot, "y_axis_label_rot");
SYMBOL($xaxis_label_pad, "x_axis_label_pad");
SYMBOL($yaxis_label_pad, "y_axis_label_pad");
SYMBOL($axis_label_font, "axis_label_font");
SYMBOL($grids, "grids");
SYMBOL($grid, "grid");
SYMBOL($size, "size");
SYMBOL($dashes, "dashes");
SYMBOL($offset, "offset");
SYMBOL($join, "join");
SYMBOL($colour, "colour");
SYMBOL($radius, "radius");
SYMBOL($length, "length");
SYMBOL($family, "family");
SYMBOL($slant, "slant");
SYMBOL($weight, "weight");
SYMBOL($point, "point");
SYMBOL($line, "line");
SYMBOL($smooth, "smooth");

static Std$Function$status plotccfg_fill(Std$Object$t *Fun, struct kplotccfg *Config, Std$Object$t *Value, Std$Function$result *Result) {
	if (Value == Std$Object$Nil) {
		Config->type = KPLOTCTYPE_DEFAULT;
	} else if (Value->Type == Std$Integer$SmallT) {
		Config->type = KPLOTCTYPE_PALETTE;
		Config->palette = Std$Integer$get_small(Value);
	} else if (Value->Type == Gir$Cairo$Pattern$T) {
		Config->type = KPLOTCTYPE_PATTERN;
		Config->pattern = ((Gir$Cairo$Pattern$t *)Value)->Value;
	} else if (Value->Type == Std$String$T) {
		GdkRGBA Colour[1];
		if (!gdk_rgba_parse(Colour, Std$String$flatten(Value))) {
			SEND(Std$String$new("Error parsing colour"));
		}
		Config->rgba[0] = Colour->red;
		Config->rgba[1] = Colour->green;
		Config->rgba[2] = Colour->blue;
		Config->rgba[3] = Colour->alpha;
	} else if (Value->Type == Gir$Gdk$RGBA$T) {
		Gir$Gdk$RGBA$t *Colour = (Gir$Gdk$RGBA$t *)Value;
		Config->rgba[0] = Colour->Value->red;
		Config->rgba[1] = Colour->Value->green;
		Config->rgba[2] = Colour->Value->blue;
		Config->rgba[3] = Colour->Value->alpha;
	} else {
		SEND("Unknown type for colour");
	}
	return SUCCESS;
}

static Std$Function$status plotfont_fill(Std$Object$t *Fun, struct kplotfont *Font, Std$Object$t *Options, Std$Function$result *Result) {
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Options); Node; Node = Agg$Table$trav_next(Trav)) {
		const Std$Object$t *Key = Agg$Table$node_key(Node);
		const Std$Object$t *Value = Agg$Table$node_value(Node);
		if (Key == $size) {
			Font->sz = Std$Real$double(Value);
		} else if (Key == $family) {
			if (Value->Type != Std$String$T) SEND("Font family must be string");
			Font->family = Std$String$flatten(Value);
		} else if (Key == $slant) {
			if (Value->Type != Gir$Cairo$FontSlant$T) SEND(Std$String$new("Invalid type of font slant"));
			Font->slant = ((Gir$Cairo$FontSlant$t *)Value)->Value;
		} else if (Key == $weight) {
			if (Value->Type != Gir$Cairo$FontWeight$T) SEND(Std$String$new("Invalid type of font weight"));
			Font->weight = ((Gir$Cairo$FontWeight$t *)Value)->Value;
		} else if (Key == $colour) {
			if (plotccfg_fill(Fun, &Font->clr, Value, Result) == MESSAGE) return MESSAGE;
		}
	}
	return SUCCESS;
}

static Std$Function$status plotticln_fill(Std$Object$t *Fun, struct kplotticln *Line, Std$Object$t *Options, Std$Function$result *Result) {
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Options); Node; Node = Agg$Table$trav_next(Trav)) {
		const Std$Object$t *Key = Agg$Table$node_key(Node);
		const Std$Object$t *Value = Agg$Table$node_value(Node);
		if (Key == $size) {
			Line->sz = Std$Real$double(Value);
		} else if (Key == $dashes) {
			if (Value->Type != Agg$List$T) SEND(Std$String$new("Dashes must be list"));
			Line->dashesz = Agg$List$length(Value);
			if (Line->dashesz > KPLOT_DASH_MAX) SEND(Std$String$new("Too many dash values"));
			double *Dash = Line->dashes;
			for (Agg$List$node *Node = Agg$List$head(Value); Node; Node = Node->Next) {
				Dash[0] = Std$Real$double(Node->Value);
				++Dash;
			}
		} else if (Key == $offset) {
			Line->dashoff = Std$Real$double(Value);
		} else if (Key == $length) {
			Line->len = Std$Real$double(Value);
		} else if (Key == $colour) {
			if (plotccfg_fill(Fun, &Line->clr, Value, Result) == MESSAGE) return MESSAGE;
		}
	}
	return SUCCESS;
}

static Std$Function$status plotline_fill(Std$Object$t *Fun, struct kplotline *Line, Std$Object$t *Options, Std$Function$result *Result) {
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Options); Node; Node = Agg$Table$trav_next(Trav)) {
		const Std$Object$t *Key = Agg$Table$node_key(Node);
		const Std$Object$t *Value = Agg$Table$node_value(Node);
		if (Key == $size) {
			Line->sz = Std$Real$double(Value);
		} else if (Key == $dashes) {
			if (Value->Type != Agg$List$T) SEND(Std$String$new("Dashes must be list"));
			Line->dashesz = Agg$List$length(Value);
			if (Line->dashesz > KPLOT_DASH_MAX) SEND(Std$String$new("Too many dash values"));
			double *Dash = Line->dashes;
			for (Agg$List$node *Node = Agg$List$head(Value); Node; Node = Node->Next) {
				Dash[0] = Std$Real$double(Node->Value);
				++Dash;
			}
		} else if (Key == $offset) {
			Line->dashoff = Std$Real$double(Value);
		} else if (Key == $join) {
			if (Value->Type != Gir$Cairo$LineJoin$T) SEND(Std$String$new("Invalid type of line join"));
			Line->join = ((Gir$Cairo$LineJoin$t *)Value)->Value;
		} else if (Key == $colour) {
			if (plotccfg_fill(Fun, &Line->clr, Value, Result) == MESSAGE) return MESSAGE;
		}
	}
	return SUCCESS;
}

static Std$Function$status plotpoint_fill(Std$Object$t *Fun, struct kplotpoint *Point, Std$Object$t *Options, Std$Function$result *Result) {
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Options); Node; Node = Agg$Table$trav_next(Trav)) {
		const Std$Object$t *Key = Agg$Table$node_key(Node);
		const Std$Object$t *Value = Agg$Table$node_value(Node);
		if (Key == $size) {
			Point->sz = Std$Real$double(Value);
		} else if (Key == $dashes) {
			if (Value->Type != Agg$List$T) SEND(Std$String$new("Dashes must be list"));
			Point->dashesz = Agg$List$length(Value);
			if (Point->dashesz > KPLOT_DASH_MAX) SEND(Std$String$new("Too many dash values"));
			double *Dash = Point->dashes;
			for (Agg$List$node *Node = Agg$List$head(Value); Node; Node = Node->Next) {
				Dash[0] = Std$Real$double(Node->Value);
				++Dash;
			}
		} else if (Key == $offset) {
			Point->dashoff = Std$Real$double(Value);
		} else if (Key == $radius) {
			Point->radius = Std$Real$double(Value);
		} else if (Key == $colour) {
			if (plotccfg_fill(Fun, &Point->clr, Value, Result) == MESSAGE) return MESSAGE;
		}
	}
	return SUCCESS;
}

typedef struct plotcfg_t {
	const Std$Type$t *Type;
	struct kplotcfg Value[1];
} plotcfg_t;

TYPE(PlotCfgT);

// These values are common across the different direction types
Std$Integer$smallt Left[1] = {{Std$Integer$SmallT, MARGIN_LEFT}};
Std$Integer$smallt Right[1] = {{Std$Integer$SmallT, MARGIN_RIGHT}};
Std$Integer$smallt Top[1] = {{Std$Integer$SmallT, MARGIN_TOP}};
Std$Integer$smallt Bottom[1] = {{Std$Integer$SmallT, MARGIN_BOTTOM}};
Std$Integer$smallt All[1] = {{Std$Integer$SmallT, MARGIN_ALL}};

static Std$Function$status plotcfg_fill(Std$Object$t *Fun, struct kplotcfg *Config, Std$Symbol$array *Options, Std$Function$argument *Args, Std$Function$result *Result) {
	for (int I = 0; I < Options->Count; ++I) {
		Std$Object$t *Value = Args[I].Val;
		if (Options->Values[I] == $xmin) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->extrema |= EXTREMA_XMIN;
			Config->extrema_xmin = Std$Real$double(Value);
		} else if (Options->Values[I] == $xmax) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->extrema |= EXTREMA_XMAX;
			Config->extrema_xmax = Std$Real$double(Value);
		} else if (Options->Values[I] == $ymin) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->extrema |= EXTREMA_YMIN;
			Config->extrema_ymin = Std$Real$double(Value);
		} else if (Options->Values[I] == $ymax) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->extrema |= EXTREMA_YMAX;
			Config->extrema_ymax = Std$Real$double(Value);
		} else if (Options->Values[I] == $margins) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Config->margin = Std$Integer$get_small(Value);
		} else if (Options->Values[I] == $margin) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->marginsz = Std$Real$double(Value);
		} else if (Options->Values[I] == $tic_labels) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Config->ticlabel = Std$Integer$get_small(Value);
		} else if (Options->Values[I] == $tic_label_font) {
			if (plotfont_fill(Fun, &Config->ticlabelfont, Value, Result) == MESSAGE) return MESSAGE;
		} else if (Options->Values[I] == $xtic_label_pad) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->xticlabelpad = Std$Real$double(Value);
		} else if (Options->Values[I] == $ytic_label_pad) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->yticlabelpad = Std$Real$double(Value);
		} else if (Options->Values[I] == $xtic_label_rot) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->xticlabelrot = Std$Real$double(Value);
		} else if (Options->Values[I] == $xtic_label_fmt) {
			SEND(Std$String$new("Formatting functions not implemented yet"));
		} else if (Options->Values[I] == $ytic_label_fmt) {
			SEND(Std$String$new("Formatting functions not implemented yet"));
		} else if (Options->Values[I] == $borders) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Config->border = Std$Integer$get_small(Value);
		} else if (Options->Values[I] == $border) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->bordersz = Std$Real$double(Value);
		} else if (Options->Values[I] == $colours) {
			CHECK_EXACT_ARG_TYPE(I, Agg$List$T);
			Config->clrsz = Agg$List$length(Value);
			struct kplotccfg *Colour = Config->clrs = Riva$Memory$alloc(Agg$List$length(Value) * sizeof(struct kplotccfg));
			for (Agg$List$node *Node = Agg$List$head(Value); Node; Node = Node->Next) {
				if (plotccfg_fill(Fun, Colour, Node->Value, Result) == MESSAGE) return MESSAGE;
				++Colour;
			}
		} else if (Options->Values[I] == $xtics) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Config->xtics = Std$Integer$get_small(Value);
		} else if (Options->Values[I] == $ytics) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Config->ytics = Std$Integer$get_small(Value);
		} else if (Options->Values[I] == $tics) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Config->tic = Std$Integer$get_small(Value);
		} else if (Options->Values[I] == $tic) {
			if (plotticln_fill(Fun, &Config->ticline, Value, Result) == MESSAGE) return MESSAGE;
		} else if (Options->Values[I] == $xaxis_label) {
			CHECK_EXACT_ARG_TYPE(I, Std$String$T);
			Config->xaxislabel = Std$String$flatten(Value);
		} else if (Options->Values[I] == $xaxis_label2) {
			CHECK_EXACT_ARG_TYPE(I, Std$String$T);
			Config->x2axislabel = Std$String$flatten(Value);
		} else if (Options->Values[I] == $yaxis_label) {
			CHECK_EXACT_ARG_TYPE(I, Std$String$T);
			Config->yaxislabel = Std$String$flatten(Value);
		} else if (Options->Values[I] == $yaxis_label2) {
			CHECK_EXACT_ARG_TYPE(I, Std$String$T);
			Config->y2axislabel = Std$String$flatten(Value);
		} else if (Options->Values[I] == $xaxis_label_rot) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->xaxislabelrot = Std$Real$double(Value);
		} else if (Options->Values[I] == $yaxis_label_rot) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->yaxislabelrot = Std$Real$double(Value);
		} else if (Options->Values[I] == $xaxis_label_pad) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->xaxislabelpad = Std$Real$double(Value);
		} else if (Options->Values[I] == $yaxis_label_pad) {
			CHECK_ARG_TYPE(I, Std$Number$T);
			Config->yaxislabelpad = Std$Real$double(Value);
		} else if (Options->Values[I] == $axis_label_font) {
			if (plotfont_fill(Fun, &Config->axislabelfont, Value, Result) == MESSAGE) return MESSAGE;
		} else if (Options->Values[I] == $grids) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Config->grid = Std$Integer$get_small(Value);
		} else if (Options->Values[I] == $grid) {
			CHECK_EXACT_ARG_TYPE(I, Agg$Table$T);
			if (plotline_fill(Fun, &Config->gridline, Value, Result) == MESSAGE) return MESSAGE;
		}
	}
	return SUCCESS;
}

/*
void		 kdatacfg_defaults(struct kdatacfg *);
void		 ksmthcfg_defaults(struct ksmthcfg *);
 */

typedef struct plot_t {
	const Std$Type$t *Type;
	struct kplot *Value;
} plot_t;

TYPE(PlotT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(PlotNew, 0) {
	struct kplotcfg *Config = 0, LocalConfig[1];
	if (Count > 0) {
		if (Args[0].Val->Type == PlotCfgT) {
			Config = ((plotcfg_t *)Args[0].Val)->Value;
		} else if (Args[0].Val->Type == Std$Symbol$ArrayT) {
			kplotcfg_defaults(LocalConfig);
			Config = LocalConfig;
			if (plotcfg_fill(Fun, LocalConfig, (Std$Symbol$array *)Args[0].Val, Args + 1, Result) == MESSAGE) {
				return MESSAGE;
			}
		}
	}
	plot_t *Plot = new(plot_t);
	Plot->Type = PlotT;
	Plot->Value = kplot_alloc(Config);
	RETURN(Plot);
}

typedef struct plottype_t {
	const Std$Type$t *Type;
	enum kplottype Value;
} plottype_t;

TYPE(PlotTypeT);

plottype_t PlotTypePoints[1] = {{PlotTypeT, KPLOT_POINTS}};
plottype_t PlotTypeMarks[1] = {{PlotTypeT, KPLOT_MARKS}};
plottype_t PlotTypeLines[1] = {{PlotTypeT, KPLOT_LINES}};
plottype_t PlotTypeLinesPoints[1] = {{PlotTypeT, KPLOT_LINESPOINTS}};
plottype_t PlotTypeLinesMarks[1] = {{PlotTypeT, KPLOT_LINESMARKS}};

typedef struct datacfg_t {
	const Std$Type$t *Type;
	struct kdatacfg *Value;
} datacfg_t;

TYPE(DataCfgT);

typedef struct smoothtype_t {
	const Std$Type$t *Type;
	enum ksmthtype Value;
} smoothtype_t;

TYPE(SmoothTypeT);

smoothtype_t SmoothTypeNone[1] = {{SmoothTypeT, KSMOOTH_NONE}};
smoothtype_t SmoothTypeMovAvg[1] = {{SmoothTypeT, KSMOOTH_MOVAVG}};
smoothtype_t SmoothTypeCDF[1] = {{SmoothTypeT, KSMOOTH_CDF}};
smoothtype_t SmoothTypePMF[1] = {{SmoothTypeT, KSMOOTH_PMF}};

typedef struct smoothcfg_t {
	const Std$Type$t *Type;
	struct ksmthcfg *Value;
} smoothcfg_t;

static Std$Function$status datacfg_fill(Std$Object$t *Fun, struct kdatacfg *Config, struct ksmthcfg *Smooth, Std$Symbol$array *Options, Std$Function$argument *Args, Std$Function$result *Result) {
	for (int I = 0; I < Options->Count; ++I) {
		Std$Object$t *Value = Args[I].Val;
		if (Options->Values[I] == $point) {
			if (plotpoint_fill(Fun, &Config->point, Value, Result) == MESSAGE) return MESSAGE;
		} else if (Options->Values[I] == $line) {
			if (plotline_fill(Fun, &Config->line, Value, Result) == MESSAGE) return MESSAGE;
		} else if (Options->Values[I] == $smooth) {
			if (!Smooth) SEND(Std$String$new("Smooth option not available"));
			CHECK_ARG_TYPE(I, Std$Integer$SmallT);
			Smooth->movsamples = Std$Integer$get_small(Value);
		}
	}
	return SUCCESS;
}

METHOD("attach", TYP, PlotT, TYP, DataT, TYP, PlotTypeT) {
	plot_t *Plot = (plot_t *)Args[0].Val;
	data_t *Data = (data_t *)Args[1].Val;
	plottype_t *PlotType = (plottype_t *)Args[2].Val;
	struct kdatacfg *DataCfg = 0, LocalDataCfg[1];
	smoothtype_t *SmoothType = 0;
	struct ksmthcfg *SmoothCfg = 0, LocalSmoothCfg[1];
	for (int I = 3; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg->Type == DataCfgT) {
			DataCfg = ((datacfg_t *)Arg)->Value;
		} else if (Arg->Type == SmoothTypeT) {
			SmoothType = Arg;
		} else if (Arg->Type == SmoothCfg) {
			SmoothCfg = ((smoothcfg_t *)Arg)->Value;
		} else if (Arg->Type == Std$Symbol$ArrayT) {
			kdatacfg_defaults(LocalDataCfg);
			ksmthcfg_defaults(LocalSmoothCfg);
			DataCfg = LocalDataCfg;
			SmoothCfg = LocalSmoothCfg;
			if (datacfg_fill(Fun, LocalDataCfg, LocalSmoothCfg, Arg, Args + I + 1, Result) == MESSAGE) return MESSAGE;
			break;
		}
	}
	if (SmoothType) {
		kplot_attach_smooth(Plot->Value, Data->Value, PlotType->Value, DataCfg, SmoothType->Value, SmoothCfg);
	} else {
		kplot_attach_data(Plot->Value, Data->Value, PlotType->Value, DataCfg);
	}
	RETURN0;
}

typedef struct plotstype_t {
	const Std$Type$t *Type;
	enum kplotstype Value;
} plotstype_t;

TYPE(PlotsTypeT);

plotstype_t PlotsTypeYErrorLine[1] = {{PlotsTypeT, KPLOTS_YERRORLINE}};
plotstype_t PlotsTypeYErrorBar[1] = {{PlotsTypeT, KPLOTS_YERRORBAR}};

typedef struct plotset_node_t {
	struct plotset_node_t *Next;
	struct kdata *Data;
	enum kplottype Type;
	struct kdatacfg *Config, Local[1];
} plotset_node_t;

typedef struct plotset_t {
	const Std$Type$t *Type;
	plotset_node_t *Nodes;
	size_t Size;
} plotset_t;

TYPE(PlotSetT);

GLOBAL_FUNCTION(PlotSetNew, 0) {
	plotset_t *PlotSet = new(plotset_t);
	PlotSet->Type = PlotSetT;
	RETURN(PlotSet);
}

METHOD("add", TYP, PlotSetT, TYP, DataT, TYP, PlotTypeT) {
	plotset_t *PlotSet = (plotset_t *)Args[0].Val;
	data_t *Data = (data_t *)Args[1].Val;
	plottype_t *PlotType = (plottype_t *)Args[2].Val;
	plotset_node_t *Node = new(plotset_node_t);
	Node->Next = PlotSet->Nodes;
	Node->Data = Data->Value;
	Node->Type = PlotType->Value;
	for (int I = 3; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg->Type == DataCfgT) {
			Node->Config = ((datacfg_t *)Arg)->Value;
		} else if (Arg->Type == Std$Symbol$ArrayT) {
			kdatacfg_defaults(Node->Local);
			Node->Config = Node->Local;
			if (datacfg_fill(Fun, Node->Local, 0, Arg, Args + I + 1, Result) == MESSAGE) return MESSAGE;
			break;
		}
	}
	PlotSet->Nodes = Node;
	++PlotSet->Size;
	RETURN0;
}

METHOD("attach", TYP, PlotT, TYP, PlotSetT, TYP, PlotsTypeT) {
	plot_t *Plot = (plot_t *)Args[0].Val;
	plotset_t *PlotSet = (plotset_t *)Args[1].Val;
	size_t Size = PlotSet->Size;
	struct kdata **Data = Riva$Memory$alloc(Size * sizeof(struct kdata *));
	enum kplottype *Types = Riva$Memory$alloc(Size * sizeof(enum kplottype));
	struct kdatacfg **DataCfgs = Riva$Memory$alloc(Size * sizeof(struct kdatacfg *));
	plotstype_t *PlotsType = (plotstype_t *)Args[2].Val;
	plotset_node_t *Node = PlotSet->Nodes;
	for (int I = Size; --I >= 0;) {
		Data[I] = Node->Data;
		Types[I] = Node->Type;
		DataCfgs[I] = Node->Config;
		Node = Node->Next;
	}
	kplot_attach_datas(Plot->Value, Size, Data, Types, DataCfgs, PlotsType->Value);
	RETURN0;
}

METHOD("draw", TYP, PlotT, TYP, Std$Real$T, TYP, Std$Real$T, TYP, Gir$Cairo$Cairo$T) {
	plot_t *Plot = (plot_t *)Args[0].Val;
	double W = Std$Real$get_value(Args[1].Val);
	double H = Std$Real$get_value(Args[2].Val);
	Gir$Cairo$Cairo$t *Cairo = (Gir$Cairo$Cairo$t *)Args[3].Val;
	kplot_draw(Plot->Value, W, H, Cairo->Value);
	RETURN0;
}
