include $(TOPDIR)/version.mk

SUBDIRS := lib icmpd icmpc

.DEFAULT_GOAL := all
.PHONE: all clean install

all clean install:
	@for x in $(SUBDIRS); do $(MAKE) -C $$x $@; done
