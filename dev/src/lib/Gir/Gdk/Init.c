#include <Riva.h>
#include <Std.h>
#include <Agg.h>
#include <Sys/Service.h>

INITIAL() {
	Sys$Service$t *Service = Sys$Service$new("gdk-threads");
	if (Service) {
		gdk_threads_init();
		Sys$Service$start(Service, Std$Object$Nil);
	};
};
