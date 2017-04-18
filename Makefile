# Linux Makefile for  ====> fit <====
# Original authors: Jack McCarthy and Jonathan Grant
#
# ------------------------------------------------------------------------
#              Copyright (c) 2015-2017 Siemens Industry, Inc.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the
#       Free Software Foundation, Inc.
#       51 Franklin Street, Fifth Floor
#       Boston MA  02110-1301 USA.
# ------------------------------------------------------------------------

#
# Macros
#

PROGRAM         = fit
DATAKEY         = datakeyFit
DISPLAY         = displayFit
DISPLAYLIB      = displayLib
EEPROM          = eepromFit
ETHERNET        = ethernetFit
FIO_MONITOR     = fioMonitorFit
FS              = fsFit
MEMORY          = memoryFit
POWERDOWN       = powerdownFit
RTC             = rtcFit
SERIAL          = serialFit
SERIAL_ECHO     = serialEchoFit
SERIAL_PORT     = serialPortFit
#TOD             = todFit
SDIR            = src
RDIR            = rels
ODIR            = bin
PACKAGE         = mfit
OUTPUT          = $(ODIR)/$(PROGRAM)
RFILES          = $(RDIR)/$(PROGRAM).o $(RDIR)/$(DISPLAY).o    \
                  $(RDIR)/$(DISPLAYLIB).o                      \
                  $(RDIR)/$(DATAKEY).o $(RDIR)/$(EEPROM).o     \
                  $(RDIR)/$(ETHERNET).o $(RDIR)/$(FS).o        \
                  $(RDIR)/$(FIO_MONITOR).o $(RDIR)/$(MEMORY).o \
                  $(RDIR)/$(POWERDOWN).o $(RDIR)/$(RTC).o      \
                  $(RDIR)/$(SERIAL).o $(RDIR)/$(SERIAL_ECHO).o \
                  $(RDIR)/$(SERIAL_PORT).o # $(RDIR)/$(TOD).o
EXTRA           =
OPT             = -O2
MFLAGS          = $(OPT) $(EXTRA) -Wall -Werror -g
CPPFLAGS        = $(MFLAGS)
LDFLAGS         = $(MFLAGS) -lpthread -lrt

#
# Rules
#

# remember the GNU make default pattern rules.
# %.o : %.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<
# % : %.o
#	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

# Somehow GNU make does not see the dependencies as mapping to the default
# rules when files are in other than the local directory.  Thus the rules
# must be explicitly defined and used.
COMPILE = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<
LINK    = $(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
STRIP   = $(STRIPTOOL) $@


#
# Link Command
#

$(OUTPUT) : $(RFILES)
	$(LINK)
	$(STRIP)


#
# Compile Commands
#

$(RDIR)/$(DATAKEY).o : $(SDIR)/$(DATAKEY).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)

$(RDIR)/$(DISPLAY).o : $(SDIR)/$(DISPLAY).c \
                       $(SDIR)/fit.h $(SDIR)/ftypes.h \
                       $(SDIR)/displayLib.h
	$(COMPILE)

$(RDIR)/$(DISPLAYLIB).o : $(SDIR)/$(DISPLAYLIB).c \
                          $(SDIR)/fit.h $(SDIR)/ftypes.h \
                          $(SDIR)/displayLib.h
	$(COMPILE)

$(RDIR)/$(EEPROM).o : $(SDIR)/$(EEPROM).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)

$(RDIR)/$(ETHERNET).o : $(SDIR)/$(ETHERNET).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)

$(RDIR)/$(FIO_MONITOR).o : $(SDIR)/$(FIO_MONITOR).c \
                           $(SDIR)/fit.h $(SDIR)/ftypes.h \
                           $(SDIR)/serialFit.h
	$(COMPILE)

$(RDIR)/$(FS).o : $(SDIR)/$(FS).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)

$(RDIR)/$(MEMORY).o : $(SDIR)/$(MEMORY).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)

$(RDIR)/$(POWERDOWN).o : $(SDIR)/$(POWERDOWN).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)

$(RDIR)/$(RTC).o : $(SDIR)/$(RTC).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)

$(RDIR)/$(SERIAL).o : $(SDIR)/$(SERIAL).c \
                      $(SDIR)/fit.h $(SDIR)/ftypes.h \
                      $(SDIR)/serialFit.h
	$(COMPILE)

$(RDIR)/$(SERIAL_ECHO).o : $(SDIR)/$(SERIAL_ECHO).c \
                           $(SDIR)/fit.h $(SDIR)/ftypes.h \
                           $(SDIR)/serialFit.h
	$(COMPILE)

$(RDIR)/$(SERIAL_PORT).o : $(SDIR)/$(SERIAL_PORT).c \
                           $(SDIR)/fit.h $(SDIR)/ftypes.h \
                           $(SDIR)/serialFit.h
	$(COMPILE)

#$(RDIR)/$(TOD).o : $(SDIR)/$(TOD).c
#	$(COMPILE)

$(RDIR)/$(PROGRAM).o : $(SDIR)/$(PROGRAM).c $(SDIR)/fit.h $(SDIR)/ftypes.h
	$(COMPILE)


#
# Utility Commands
#

clean:
	rm -f $(OUTPUT)
	rm -f $(RFILES)
	rm -f $(PACKAGE).tgz
	rm -rf $(ODIR)/$(PACKAGE)

package:  $(OUTPUT)
	cd $(ODIR); mkdir $(PACKAGE);mv $(PROGRAM) $(PACKAGE)
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) datakey
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) display
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) eeprom
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) ethernet
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) filesystem
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) fio_monitor
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) memory
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) powerdown
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) rtc
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) sd
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) serial
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) serial_echo
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) serial_port
	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) usb
#	cd $(ODIR)/$(PACKAGE);ln -s $(PROGRAM) tod
	cp -r $(SDIR)/conf $(ODIR)/$(PACKAGE)
	cp License.txt $(ODIR)/$(PACKAGE)
	cd $(ODIR);tar czvpf ../$(PACKAGE).tgz *

