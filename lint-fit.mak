# Lint makefile for  ====> M-Fit <====
# Original author: Jonathan Grant
# Lint the M-Fit utilities with Gimple PC-Lint or FlexeLint
#
# Needs a GNU 'make' version 3.81 or higher to run
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


SRCS = \
	src/fit.c \
	src/datakeyFit.c \
	src/displayFit.c \
	src/displayLib.c \
	src/eepromFit.c \
	src/ethernetFit.c \
	src/fioMonitorFit.c \
	src/fsFit.c \
	src/memoryFit.c \
	src/powerdownFit.c \
	src/rtcFit.c \
	src/serialFit.c \
	src/serialEchoFit.c \
	src/serialPortFit.c \
	src/todFit.c

RDIR	 := lobs
ODIR	 := lint_out
OUTPUT	 := $(ODIR)/mfit_final.txt
XMLOUTPUT:= $(ODIR)/mfit_final.xml

LINTDIR  := lint
INCLUDES := -i$(LINTDIR)
LCOMMON	 := +v -zero $(INCLUDES) co-gcc.lnt mfit.lnt $(EXTRA) $(PFLAGS)

ifdef STDOUT
# allows all output to come out standard out, rather than to text files
LINT_STD_OUT=
LINT_FINAL_STD_OUT=
else
LINT_STD_OUT="-os($(ODIR)/$(notdir $(basename $<).txt))"
LINT_FINAL_STD_OUT="-os($@)"
endif

LINT	 := lint-nt
LINT_UNIT = $(LINT) -u "-oo($@)" $(LINT_STD_OUT) $(LCOMMON) $<
LINT_LOBS = $(LINT) $(LINT_FINAL_STD_OUT) $(LCOMMON) $^

# For XML out append env-xml.lnt file to the command line of everything
# Be sure to run clean in between when switching between TXT and XML output
ifeq "$(MAKECMDGOALS)" "pclintxml"
LINT_UNIT = $(LINT) -u "-oo($@)" "-os($(ODIR)/$(notdir $(basename $<).xml))" $(LCOMMON) env-xml.lnt $<
LINT_UNIT_CPP = $(LINT) -u "-oo($@)" "-os($(ODIR)/$(notdir $(basename $<).xml))" $(LCOMMONCPP) env-xml.lnt $<
XMLLINT_LOBS = $(LINT) "-os($@)" $(LCOMMONCPP) env-xml.lnt $^
endif

CFILES := $(SRCS)

TFILES   = $(addsuffix .txt, $(addprefix $(ODIR)/,$(notdir $(basename $(CFILES)))))
LFILES   = $(addsuffix .lob, $(addprefix $(RDIR)/,$(notdir $(basename $(CFILES)))))
XFILES   = $(addsuffix .xml, $(addprefix $(ODIR)/,$(notdir $(basename $(CFILES)))))

pclint : $(RDIR) $(ODIR) $(OUTPUT)

pclintxml : $(RDIR) $(ODIR) $(XMLOUTPUT)

$(RDIR):
	mkdir -p $(RDIR)

$(ODIR):
	mkdir -p $(ODIR)

$(OUTPUT): $(LFILES)
	$(LINT_LOBS)

$(XMLOUTPUT): $(LFILES)
	$(XMLLINT_LOBS)


.SECONDEXPANSION:
PERCENT = %

# Uses SECONDEXPANSION so we can have a generic build target for all sources
# The first '%' is expanded first, which gets the obj you are trying to build
# with a full path, then with that full path, make does a second pass and
# expands/executes the "$$" functions and the $$(PERCENT) becomes just a
# wildcard for the filter()
$(LFILES): %.lob : $$(filter $$(PERCENT)/$$(notdir %).c, $(CFILES))
	$(LINT_UNIT)


force: ;

clean:
	rm -f $(LFILES) $(TFILES) $(OUTPUT) $(XFILES) $(XMLOUTPUT)
