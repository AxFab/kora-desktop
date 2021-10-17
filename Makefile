#      This file is part of the KoraOS project.
#  Copyright (C) 2015-2021  <Fabien Bavent>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST))))
gendir ?= $(shell pwd)

include $(topdir)/make/global.mk

all: bins

include $(topdir)/make/build.mk
include $(topdir)/make/check.mk
include $(topdir)/make/targets.mk


CFLAGS ?= -Wall -Wextra -Wno-unused-parameter -ggdb

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CFLAGS_g += $(CFLAGS) -fPIC -fno-builtin

ifneq ($(sysdir),)
CFLAGS_g += -I$(sysdir)/include
LFLAGS_g += -L$(sysdir)/lib
endif

LFLAGS_lg = $(LFLAGS_g) -lgfx -lpng -lz -lm
LFLAGS_wm = $(LFLAGS_g) -lgfx -lpng -lz -lm

# CFLAGS += $(shell $(PKC) --cflags lgfx freetype)
# LFLAGS += $(shell $(PKC) --libs lgfx freetype) -lm


ifeq ($(DISTO),linux)
LFLAGS_wm += -lpthread
endif


# SRCS_lg = $(wildcard $(srcdir)/logon/*.c)
# $(eval $(call comp_source,lg,CFLAGS_g))
# $(eval $(call link_bin,logon,SRCS_lg,LFLAGS_lg,lg))

SRCS_wm = $(wildcard $(srcdir)/winmgr/*.c)
$(eval $(call comp_source,wm,CFLAGS_g))
$(eval $(call link_bin,winmgr,SRCS_wm,LFLAGS_wm,wm))


install: $(call fn_inst,$(BINS) $(LIBS))

bins: $(BINS)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ifeq ($(NODEPS),)
-include $(call fn_deps,SRCS_wm,wm)
-include $(call fn_deps,SRCS_lg,lg)
endif
