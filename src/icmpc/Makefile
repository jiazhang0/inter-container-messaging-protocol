include $(TOPDIR)/env.mk
include $(TOPDIR)/rules.mk

BIN_NAME := icmpc

OBJS_$(BIN_NAME) := \
		    icmpc.o \
		    subcmd_help.o \
		    subcmd_echo.o \
		    subcmd_commandline.o

all: $(BIN_NAME) Makefile

$(BIN_NAME): $(OBJS_$(BIN_NAME)) $(TOPDIR)/src/lib/$(LIB_NAME).so
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	@$(RM) $(OBJS_$(BIN_NAME)) $(BIN_NAME)

install: all
	$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
	$(INSTALL) -m 700 $(BIN_NAME) $(DESTDIR)$(bindir)

include $(TOPDIR)/env.mk
include $(TOPDIR)/rules.mk
