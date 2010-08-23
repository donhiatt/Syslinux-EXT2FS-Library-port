#
# LGPL/GPL code library
#

# Include configuration rules
topdir = ../..
include ../lib/MCONFIG

EXT2LIBDIR    = e2fsprogs/lib
EXT2LIB       = $(EXT2LIBDIR)/ext2fs 
ETLIB         = $(EXT2LIBDIR)/et 
EXT2LIBFLAGS  = -DHAVE_SYS_STAT_H -DHAVE_SYS_TYPES_H
EXT2LIBFLAGS += -DHAVE_UNISTD_H -DHAVE_ERRNO_H
EXT2LIBFLAGS += -DHAVE_SYSLINUX_BUILD
EXT2LIBFLAGS += -g
###EXT2LIBFLAGS += -DSYSLINUX_EXT2FS_TDB

REQFLAGS += -I../gplinclude
REQFLAGS += -I../lib
REQFLAGS += -I../../core/include
REQFLAGS += -I../gpllib/e2fsprogs/lib
REQFLAGS += $(EXT2LIBFLAGS)

GPLDIRS := . disk dmi vpd $(EXT2LIB) $(ETLIB)
LIBOBJS := $(foreach dir,$(GPLDIRS),$(patsubst %.c,%.o,$(wildcard $(dir)/*.c)))

BINDIR   = /usr/bin
LIBDIR   = /usr/lib
DATADIR  = /usr/share
AUXDIR   = $(DATADIR)/syslinux
INCDIR   = /usr/include
COM32DIR = $(AUXDIR)/com32

all: libcom32gpl.a

libcom32gpl.a : $(LIBOBJS)
	rm -f $@
	$(AR) cq $@ $^
	$(RANLIB) $@

tidy dist clean:
	find . \( -name \*.o -o -name \*.a -o -name .\*.d -o -name \*.tmp \) -print0 | \
		xargs -0r rm -f

spotless: clean
	rm -f *.a
	rm -f *~ \#* */*~ */\#*

# Mixing in the GPL include files is suboptimal, but I'm not sure
# there is a better way to do it.
install: all
	mkdir -m 755 -p $(INSTALLROOT)$(COM32DIR)
	install -m 644 libcom32gpl.a $(INSTALLROOT)$(COM32DIR)
	mkdir -p $(INSTALLROOT)$(COM32DIR)/include/
	cp -r ../gplinclude $(INSTALLROOT)$(COM32DIR)/include/

-include .*.d */.*.d */*/.*.d
