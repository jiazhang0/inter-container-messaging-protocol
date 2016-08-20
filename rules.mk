include $(TOPDIR)/version.mk

LIB_NAME := libicmp

LDFLAGS +=
CFLAGS += -DVERSION=\"$(VERSION)\"

.DEFAULT_GOAL := all
.PHONE: all clean install
