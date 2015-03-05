# tsexact/Makefile

MODULE_big = tsexact
OBJS = tsexact.o tsheadline.o
EXTENSION = tsexact
DATA = tsexact--1.0.sql
REGRESS = tsexact

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

