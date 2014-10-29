# tsexact/Makefile

MODULE_big = tsexact
OBJS = tsexact.o
EXTENSION = tsexact
DATA = tsexact--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

