#include <Std.h>
#include <Riva/Memory.h>
#include <Agg/List.h>
#include <Gir/GObject/Object.h>
#include <Gir/Cairo/Cairo.h>
#include <cairo/cairo.h>
#include <kplot.h>

/*
int		 kdata_get(const struct kdata *, size_t, struct kpair *);

int		 kdata_array_add(struct kdata *, size_t, double);
struct kdata	*kdata_array_alloc(const struct kpair *, size_t);
int		 kdata_array_fill(struct kdata *, void *,
			void (*)(size_t, struct kpair *, void *));
int		 kdata_array_fill_ydoubles(struct kdata *, const double *);
int		 kdata_array_fill_ysizes(struct kdata *, const size_t *);
int		 kdata_array_set(struct kdata *, size_t, double, double);

int		 kdata_bucket_add(struct kdata *, size_t, double);
struct kdata	*kdata_bucket_alloc(size_t, size_t);
int		 kdata_bucket_set(struct kdata *, size_t, double, double);

struct kdata	*kdata_buffer_alloc(size_t);
int		 kdata_buffer_copy(struct kdata *, const struct kdata *);

int		 kdata_hist_add(struct kdata *, double, double);
struct kdata	*kdata_hist_alloc(double, double, size_t);
int		 kdata_hist_set(struct kdata *, double, double);

struct kdata	*kdata_mean_alloc(struct kdata *);
int		 kdata_mean_attach(struct kdata *, struct kdata *);

struct kdata	*kdata_stddev_alloc(struct kdata *);
int		 kdata_stddev_attach(struct kdata *, struct kdata *);

struct kdata	*kdata_vector_alloc(size_t);
int		 kdata_vector_append(struct kdata *, double, double);
int		 kdata_vector_set(struct kdata *, size_t, double, double);

double		 kdata_pmfmean(const struct kdata *);
double		 kdata_pmfvar(const struct kdata *);
double		 kdata_pmfstddev(const struct kdata *);

ssize_t		 kdata_xmax(const struct kdata *, struct kpair *);
double		 kdata_xmean(const struct kdata *);
ssize_t		 kdata_xmin(const struct kdata *, struct kpair *);

double		 kdata_xstddev(const struct kdata *);
ssize_t		 kdata_ymax(const struct kdata *, struct kpair *);
double		 kdata_ymean(const struct kdata *);
double		 kdata_ystddev(const struct kdata *);
ssize_t		 kdata_ymin(const struct kdata *, struct kpair *);

void		 kdatacfg_defaults(struct kdatacfg *);
void		 kplotcfg_defaults(struct kplotcfg *);
int		 kplotcfg_default_palette(struct kplotccfg **, size_t *);
void		 ksmthcfg_defaults(struct ksmthcfg *);

struct kplot	*kplot_alloc(const struct kplotcfg *);
int		 kplot_detach(struct kplot *, const struct kdata *);
int		 kplot_attach_data(struct kplot *, struct kdata *,
			enum kplottype, const struct kdatacfg *);
int		 kplot_attach_smooth(struct kplot *, struct kdata *,
			enum kplottype, const struct kdatacfg *,
			enum ksmthtype, const struct ksmthcfg *);
int		 kplot_attach_datas(struct kplot *, size_t,
			struct kdata **, const enum kplottype *,
			const struct kdatacfg *const *, enum kplotstype);
void		 kplot_draw(struct kplot *, double, double, cairo_t *);
void		 kplot_free(struct kplot *);
int		 kplot_get_datacfg(struct kplot *, size_t,
			struct kdatacfg **, size_t *);
struct kplotcfg	*kplot_get_plotcfg(struct kplot *);
 */

typedef struct data_t {
	const Std$Type$t *Type;
	struct kdata *Value;
} data_t;

TYPE(DataT, Gir$GObject$Object$T);

TYPE(ArrayDataT, DataT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(ArrayAlloc, 1) {
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

GLOBAL_FUNCTION(VectorAlloc, 1) {
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

GLOBAL_FUNCTION(StddevAlloc, 1) {
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

GLOBAL_FUNCTION(MeanAlloc, 1) {
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

GLOBAL_FUNCTION(HistAlloc, 3) {
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

GLOBAL_FUNCTION(BufferAlloc, 1) {
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

GLOBAL_FUNCTION(BucketAlloc, 3) {
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

typedef struct plotcfg_t {
	const Std$Type$t *Type;
	struct kplotcfg Value[1];
} plotcfg_t;

TYPE(PlotcfgT);

typedef struct plot_t {
	const Std$Type$t *Type;
	struct kplot *Value;
} plot_t;

TYPE(PlotT, Gir$GObject$Object$T);

GLOBAL_FUNCTION(PlotAlloc, 0) {
	struct kplotcfg *Config = 0;
	if (Count > 0) {
		CHECK_ARG_TYPE(0, PlotcfgT);
		Config = ((plotcfg_t *)Args[0].Val)->Value;
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

METHOD("attach", TYP, PlotT, TYP, DataT, TYP, PlotTypeT) {
	plot_t *Plot = (plot_t *)Args[0].Val;
	data_t *Data = (data_t *)Args[1].Val;
	plottype_t *PlotType = (plottype_t *)Args[2].Val;
	kplot_attach_data(Plot->Value, Data->Value, PlotType->Value, 0);
	RETURN0;
}

METHOD("attach", TYP, PlotT, TYP, DataT, TYP, PlotTypeT, TYP, DataCfgT) {
	plot_t *Plot = (plot_t *)Args[0].Val;
	data_t *Data = (data_t *)Args[1].Val;
	plottype_t *PlotType = (plottype_t *)Args[2].Val;
	datacfg_t *DataCfg = (datacfg_t *)Args[3].Val;
	kplot_attach_data(Plot->Value, Data->Value, PlotType->Value, DataCfg->Value);
	RETURN0;
}

typedef struct plotstype_t {
	const Std$Type$t *Type;
	enum kplotstype Value;
} plotstype_t;

TYPE(PlotsTypeT);

plotstype_t PlotsTypeYErrorLine[1] = {{PlotTypeT, KPLOTS_YERRORLINE}};
plotstype_t PlotsTypeYErrorBar[1] = {{PlotTypeT, KPLOTS_YERRORBAR}};

METHOD("attach", TYP, PlotT, TYP, Agg$List$T, TYP, PlotsTypeT) {
	plot_t *Plot = (plot_t *)Args[0].Val;
	size_t Size = Agg$List$length(Args[1].Val);
	struct kdata **Data = Riva$Memory$alloc(Size * sizeof(struct kdata *));
	enum kplottype *Types = Riva$Memory$alloc(Size * sizeof(enum kplottype));
	struct kdatacfg **DataCfgs = Riva$Memory$alloc(Size * sizeof(struct kdatacfg *));
	plotstype_t *PlotsType = (plotstype_t *)Args[2].Val;
	for (Agg$List$node *Node = Agg$List$head(Args[1].Val); Node; Node = Node->Next) {

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














