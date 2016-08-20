CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar
INSTALL ?= install

EXTRA_CFLAGS ?=
EXTRA_LDFLAGS ?=

DEBUG_BUILD ?=
DESTDIR ?=
prefix ?= /usr/local
libdir ?= $(prefix)/lib
bindir ?= $(prefix)/bin
sbindir ?= $(prefix)/sbin
includedir ?= $(prefix)/include

nanomsg_includedir ?= $(includedir)
nanomsg_libdir ?= $(libdir)

libyaml_includedir ?= $(includedir)
libyaml_libdir ?= $(libdir)

LDFLAGS := --warn-common --no-undefined --fatal-warnings \
	   $(patsubst $(join -Wl,,)%,%,$(EXTRA_LDFLAGS))
CFLAGS := -D_GNU_SOURCE -DEEE_IC -ggdb -DDEBUG \
	  -std=c99 -O2 -Wall -Werror \
	  $(addprefix -I, $(TOPDIR)/src/include \
	  $(nanomsg_includedir) $(libyaml_includedir)) \
	  -L$(nanomsg_libdir) -lnanomsg \
	  -L$(libyaml_libdir) -lyaml \
	  $(EXTRA_CFLAGS) $(addprefix $(join -Wl,,),$(LDFLAGS))

ifneq ($(DEBUG_BUILD),)
	CFLAGS += -ggdb -DDEBUG -DICMP_CHANNEL_PREFIX=\"/tmp/\" \
		  -DIC_DEFAULT_LXC_PATH=\"/tmp/lxc\"
endif
