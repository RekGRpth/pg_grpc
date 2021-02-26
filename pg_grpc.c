#include <grpc/grpc.h>
#include <postgres.h>

//#include <catalog/pg_type.h>
#include <fmgr.h>
//extern text *cstring_to_text(const char *s);
//extern text *cstring_to_text_with_len(const char *s, int len);
//extern char *text_to_cstring(const text *t);
//extern void text_to_cstring_buffer(const text *src, char *dst, size_t dst_len);
//#define CStringGetTextDatum(s) PointerGetDatum(cstring_to_text(s))
//#define TextDatumGetCString(d) text_to_cstring((text *) DatumGetPointer(d))

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

void _PG_init(void); void _PG_init(void) {
    grpc_init();
}

void _PG_fini(void); void _PG_fini(void) {
    grpc_shutdown();
}

EXTENSION(pg_grpc) {
    PG_RETURN_BOOL(true);
}
