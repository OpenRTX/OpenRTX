TOPDIR=../..

#lib or target you want to build: eg. LIB=libmisc.a  or TARGET=wlan
LIB		=
TARGET	= ic_stream_ap

#libraries depend: eg. LIBS = -lbsp -ldrv -lrtos
LIBS	= -lbsp -ldbg -lrtos -lurpc
ifeq ($(IC_BOARD), 0)
LIBS	+= -ldrv_fpga
else
LIBS	+= -ldrv
endif

#exclude subdirs
exclude_dirs= fifo

#files want to build: eg. CSRCS=a.c  CPPSRCS=b.cpp  SSRCS=c.S
CSRCS	= $(wildcard *.c) $(wildcard mtrace/*.c)
CPPSRCS	=
SSRCS	=

#includings and flags
CFLAGS = -I./fifo \
		 -I./mtrace \
		 -I $(TOPDIR)/include/CMSIS \
         -I $(TOPDIR)/include/CMSIS/core \
		 -I$(TOPDIR)/include/rtos \
		 -I$(TOPDIR)/include/urpc \
		 -I$(TOPDIR)/include/bsp \
		 -I$(TOPDIR)/include/debug \
		 -I$(TOPDIR)/include/driver \
	     -I $(TOPDIR)/modules/driver/private_include

# submodule
include utility.mk

modules := $(subst /module.mk,,$(shell find . -name module.mk))
include $(addsuffix /module.mk,$(modules))

CSRCS	+= $(module_sources)
CFLAGS	+= $(module_cflags)
CFLAGS	+= $(module_includes)
LIBS	+= $(module_libs)

include $(TOPDIR)/rules.mk

LDSCRIPT  = ld/gcc_arm.ld

# OPTIM     = -O2
