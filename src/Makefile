# Makefile for Gnuk

# Define project name here
PROJECT = chopstx-template

CHOPSTX = ../chopstx

CSRC = main.c crc32.c \
	usb_desc.c usb_ctrl.c \
	usb-thread.c

INCDIR =

include config.mk

USE_SYS = yes
USE_USB = yes
#USE_ADC = yes
USE_EVENTFLAG = yes

ifeq ($(EMULATION),)
DEFS += -DFLASH_UPGRADE_SUPPORT
endif

ifneq ($(ENABLE_DEBUG),)
CSRC += debug.c
endif

ifeq ($(CHIP),stm32f103)
CSRC += mcu-stm32f103.c 
endif

###################################
CC   = $(CROSS)gcc
LD   = $(CROSS)gcc
OBJCOPY   = $(CROSS)objcopy

CWARN = -Wall -Wextra -Wstrict-prototypes
OPT   = -O3 -Os -g

#######################
include $(CHOPSTX)/rules.mk

board.h:
	@echo Please run configure to have a symbolic link \'board.h\'
	@exit 1

sys.c: board.h

build/bignum.o: OPT = -O3 -g

distclean: clean
	-rm -f $(LDSCRIPT) config.h board.h config.mk \
	       usb-strings.c.inc usb-vid-pid-ver.c.inc

ifneq ($(EMULATION),)
# By specifying DESTDIR on invocation of "make", you can install
# program to different ROOT.

# The variables prefix, exec_prefix, libexecdir are defined in
# config.mk.

install: build/$(PROJECT)
	test -d "$(DESTDIR)$(libexecdir)" || mkdir -p "$(DESTDIR)$(libexecdir)"
	install -c build/$(PROJECT) "$(DESTDIR)$(libexecdir)"

endif
