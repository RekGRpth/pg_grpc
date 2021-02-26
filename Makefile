EXTENSION = pg_grpc
MODULE_big = $(EXTENSION)

PG_CONFIG = pg_config
OBJS = $(EXTENSION).o
DATA = pg_grpc--1.0.sql

LIBS += -lgrpc -laddress_sorting -lcares -lupb -lgpr
SHLIB_LINK := $(LIBS)

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

$(OBJS): Makefile