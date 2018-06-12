#include <Std.h>
#include <Riva/Memory.h>
#include <Stat/Dist.h>
#include <Math/Random.h>

//#define BOOST_NO_EXCEPTIONS 1
#define BOOST_MATH_DOMAIN_ERROR_POLICY errno_on_error
#define BOOST_MATH_POLE_ERROR_POLICY errno_on_error
#define BOOST_MATH_OVERFLOW_ERROR_POLICY errno_on_error
#define BOOST_MATH_ROUNDING_ERROR_POLICY errno_on_error
#define BOOST_MATH_EVALUATION_ERROR_POLICY errno_on_error
#define BOOST_MATH_UNDERFLOW_ERROR_POLICY errno_on_error
#define BOOST_MATH_DENORM_ERROR_POLICY errno_on_error
#define BOOST_MATH_INDETERMINATE_RESULT_ERROR_POLICY errno_on_error

extern "C" {

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

void * __attribute__((__regparm__ (1))) ___tls_get_addr(tls_index *ti) {
};

};	
