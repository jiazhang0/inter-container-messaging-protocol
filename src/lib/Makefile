include $(TOPDIR)/env.mk
include $(TOPDIR)/rules.mk

LIB_NAME := libicmp

LIB_TARGETS := $(LIB_NAME).so $(LIB_NAME).a

OBJS_$(LIB_NAME) = \
		   ic_build_info.o \
		   ic.o \
		   icmp.o \
		   subcommand.o \
		   conf_file.o \
		   transport.o \
		   nanomsg.o \
		   yaml.o \
		   lxc.o \
		   string_tree.o \
		   vector.o \
		   buffer_stream.o \
		   garbage.o \
		   init.o \
		   linux.o \
		   util.o

CFLAGS += -fpic

all: $(LIB_TARGETS) Makefile

clean:
	@$(RM) $(LIB_TARGETS) $(OBJS_$(LIB_NAME)) $(addsuffix .*, $(LIB_TARGETS)) ic_build_info.c

install: all
	$(INSTALL) -d -m 755 $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 $(LIB_NAME).a $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 $(LIB_NAME).so $(DESTDIR)$(libdir)/$(LIB_NAME).so.$(VERSION)
	$(foreach x, $(LIB_NAME).so, ln -sfn $(x).$(VERSION) $(DESTDIR)$(libdir)/$(patsubst %,%.$(MAJOR_VERSION).$(MINOR_VERSION),$(x)); \
		ln -sfn $(x).$(MAJOR_VERSION).$(MINOR_VERSION) $(DESTDIR)$(libdir)/$(patsubst %,%.$(MAJOR_VERSION),$(x));)

$(LIB_NAME).so: $(OBJS_$(LIB_NAME))
	$(CC) $^ -o $@ $(CFLAGS) -shared -Wl,-soname,$(patsubst %.$(VERSION),%,$@)

$(LIB_NAME).a: $(OBJS_$(LIB_NAME))
	$(AR) rcs $@ $^

ic_build_info.c: ic_build_info.c.in
	sed -e "s~@@IC_GIT_COMMIT@@~$(shell if [ -d $(TOPDIR)/.git ]; then git log -1 --pretty=format:%H | tr -d '\n'; elif [ -f $(TOPDIR)/commit ]; then cat $(TOPDIR)/commit | tr -d '\n'; else echo -n ???????; fi)~" \
		-e "s~@@IC_BUILD_MACHINE@@~$(shell whoami | tr -d '\n'; echo -n @; uname -a | tr -d '\n')~" < $^ > $@
