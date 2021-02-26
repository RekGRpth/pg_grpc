#include <postgres.h>

#include <catalog/pg_type.h>
#include <fmgr.h>
#include <lib/stringinfo.h>
#include <utils/builtins.h>

#include <grpc/grpc.h>
#include <grpc/grpc_security.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

#define FORMAT_0(fmt, ...) "%s(%s:%d): %s", __func__, __FILE__, __LINE__, fmt
#define FORMAT_1(fmt, ...) "%s(%s:%d): " fmt,  __func__, __FILE__, __LINE__
#define GET_FORMAT(fmt, ...) GET_FORMAT_PRIVATE(fmt, 0, ##__VA_ARGS__, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define GET_FORMAT_PRIVATE(fmt, \
      _0,  _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, \
     _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, \
     _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, \
     _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, \
     _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, \
     _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, \
     _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, \
     _70, format, ...) FORMAT_ ## format(fmt)

#define D1(fmt, ...) ereport(DEBUG1, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D2(fmt, ...) ereport(DEBUG2, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D3(fmt, ...) ereport(DEBUG3, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D4(fmt, ...) ereport(DEBUG4, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D5(fmt, ...) ereport(DEBUG5, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define E(fmt, ...) ereport(ERROR, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define F(fmt, ...) ereport(FATAL, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define I(fmt, ...) ereport(INFO, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define L(fmt, ...) ereport(LOG, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define N(fmt, ...) ereport(NOTICE, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define W(fmt, ...) ereport(WARNING, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))

PG_MODULE_MAGIC;

/*struct grpc_c_event_s {
    grpc_c_event_type_t type;
    void *data;
    grpc_c_event_callback_t callback;
};*/

static char *target = NULL;
static grpc_call *call = NULL;
static grpc_channel *channel = NULL;
static grpc_completion_queue *completion_queue = NULL;

void _PG_init(void); void _PG_init(void) {
    grpc_init();
    if (!grpc_is_initialized()) E("!grpc_is_initialized");
}

void _PG_fini(void); void _PG_fini(void) {
    void *reserved = NULL;
    grpc_call_error error;
    if (target) pfree((void *)target);
    grpc_completion_queue_destroy(completion_queue);
    if (call && (error = grpc_call_cancel(call, reserved))) E("!grpc_call_cancel and %s", grpc_call_error_to_string(error));
    if (channel) grpc_channel_destroy(channel);
    grpc_shutdown();
}

EXTENSION(pg_grpc_insecure_channel_create) {
    const grpc_channel_args *args = NULL;
    void *reserved = NULL;
    if (PG_ARGISNULL(0)) E("target is null!");
    if (target) pfree((void *)target);
    target = TextDatumGetCString(PG_GETARG_DATUM(0));
    if (channel) grpc_channel_destroy(channel);
    if (!(channel = grpc_insecure_channel_create(target, args, reserved))) E("!grpc_insecure_channel_create");
    if (completion_queue) grpc_completion_queue_destroy(completion_queue);
    if (!(completion_queue = grpc_completion_queue_create_for_next(reserved))) E("!grpc_completion_queue_create_for_next");
    PG_RETURN_BOOL(true);
}

EXTENSION(pg_grpc_secure_channel_create) {
    grpc_channel_credentials *creds = NULL;
    const grpc_channel_args *args = NULL;
    void *reserved = NULL;
    if (PG_ARGISNULL(0)) E("target is null!");
    if (target) pfree((void *)target);
    target = TextDatumGetCString(PG_GETARG_DATUM(0));
    if (channel) grpc_channel_destroy(channel);
    if (!(channel = grpc_secure_channel_create(creds, target, args, reserved))) E("!grpc_insecure_channel_create");
    if (completion_queue) grpc_completion_queue_destroy(completion_queue);
    if (!(completion_queue = grpc_completion_queue_create_for_next(reserved))) E("!grpc_completion_queue_create_for_next");
    PG_RETURN_BOOL(true);
}

EXTENSION(pg_grpc_channel_create_call) {
    grpc_call *parent_call = NULL;
    uint32_t propagation_mask = 0;
    grpc_slice method;
    grpc_slice host;
    gpr_timespec deadline = gpr_inf_future(GPR_CLOCK_MONOTONIC);
    void *reserved = NULL;
    grpc_call_error error;
    const char *cmethod;
    if (!channel) E("!channel");
    if (PG_ARGISNULL(0)) E("method is null!");
    cmethod = TextDatumGetCString(PG_GETARG_DATUM(0));
    method = grpc_slice_from_copied_string(cmethod);
    host = grpc_slice_from_copied_string(target);
    if (call && (error = grpc_call_cancel(call, reserved))) E("!grpc_call_cancel and %s", grpc_call_error_to_string(error));
    if (!(call = grpc_channel_create_call(channel, parent_call, propagation_mask, completion_queue, method, &host, deadline, reserved))) E("!grpc_channel_create_call");
    pfree((void *)cmethod);
    grpc_slice_unref(method);
    grpc_slice_unref(host);
    PG_RETURN_BOOL(true);
}

EXTENSION(pg_grpc_call_start_batch) {
    grpc_op ops;
    size_t nops = 1;
    void *tag = NULL;
    void *reserved = NULL;
    grpc_metadata_array metadata;
    grpc_call_error error;
    if (!call) E("!call");
    grpc_metadata_array_init(&metadata);
    memset(&ops, 0, sizeof(ops));
    ops.op = GRPC_OP_SEND_INITIAL_METADATA;
    ops.data.send_initial_metadata.metadata = metadata.metadata;
    ops.data.send_initial_metadata.count = metadata.count;
    if ((error = grpc_call_start_batch(call, &ops, nops, tag, reserved))) E("!grpc_call_start_batch and %s", grpc_call_error_to_string(error));
    grpc_metadata_array_destroy(&metadata);
    PG_RETURN_BOOL(true);
}

EXTENSION(pg_grpc_completion_queue_next) {
    gpr_timespec deadline = gpr_inf_future(GPR_CLOCK_MONOTONIC);
    void *reserved = NULL;
    if (!completion_queue) E("!completion_queue");
    for (;;) {
        grpc_event event = grpc_completion_queue_next(completion_queue, deadline, reserved);
        if (event.type == GRPC_OP_COMPLETE) {
            W("GRPC_OP_COMPLETE");
        } else if (event.type == GRPC_QUEUE_SHUTDOWN) {
            W("GRPC_QUEUE_SHUTDOWN");
            break;
        }
    }
    PG_RETURN_BOOL(true);
}
