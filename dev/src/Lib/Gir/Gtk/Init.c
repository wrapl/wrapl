#include <Riva.h>
#include <Std.h>
#include <Agg.h>
#include <Sys/Service.h>

INITIAL() {
	Sys$Service_t *Service = Sys$Service$new("gtk");
	if (Service) {
		gtk_init(0, 0);
		Sys$Service$start(Service, Std$Object$Nil);
	};
};
