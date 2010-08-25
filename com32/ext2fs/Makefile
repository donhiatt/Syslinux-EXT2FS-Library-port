## -----------------------------------------------------------------------
##
##   Copyright 2010 Don Hiatt - All Rights Reserved
##
##   Permission is hereby granted, free of charge, to any person
##   obtaining a copy of this software and associated documentation
##   files (the "Software"), to deal in the Software without
##   restriction, including without limitation the rights to use,
##   copy, modify, merge, publish, distribute, sublicense, and/or
##   sell copies of the Software, and to permit persons to whom
##   the Software is furnished to do so, subject to the following
##   conditions:
##
##   The above copyright notice and this permission notice shall
##   be included in all copies or substantial portions of the Software.
##
##   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
##   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
##   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
##   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
##   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
##   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
##   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
##   OTHER DEALINGS IN THE SOFTWARE.
##
## -----------------------------------------------------------------------

##
## ext2fs
## Makefile for ext2fs module
##

topdir = ../..
include ../MCONFIG

EXT2FLAGS += -I../gpllib/e2fsprogs/lib
EXT2FLAGS += -I../../core/include
EXT2FLAGS += -DHAVE_SYSLINUX_BUILD

CFLAGS += $(EXT2FLAGS)
CFLAGS += -g

MODULES = ext2fs.c32
OBJS = ext2fs_main.o ext2fs_utils.o

all: $(MODULES)

ext2fs.elf : $(OBJS) $(LIBS) $(C_LIBS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o *.lo *.a *.lst *.elf *.c32 .*.d *.tmp

-include .*.d
