MODULE = extra

SRCS = logchan.c \
	pseudoclient.c \
	alias.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CFLAGS += -I../../include -I../../src

CFLAGS += $(MOWGLI_CFLAGS)
LIBS += $(LDFLAGS_RPATH)
LIBS += $(MOWGLI_LIBS)
