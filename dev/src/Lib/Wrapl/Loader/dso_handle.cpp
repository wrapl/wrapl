extern "C" {

void *__dso_handle = &__dso_handle;

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

void * __attribute__((__regparm__ (1))) ___tls_get_addr(tls_index *ti) {
};

};
