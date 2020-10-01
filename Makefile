#      This file is part of the KoraOS project.
#  Copyright (C) 2018  <Fabien Bavent>
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
#  This makefile is more or less generic.
#  The configuration is on `sources.mk`.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST))))
gendir ?= $(shell pwd)

include $(topdir)/make/global.mk

all: bins

CFLAGS ?= -Wall -Wextra -Wno-unused-parameter -ggdb
CFLAGS += -fPIC
CFLAGS += -fno-builtin
ifeq ($(target_os),kora)
CFLAGS += -Dmain=_main -D_GNU_SOURCE
endif

include $(topdir)/make/build.mk


CFLAGS += $(shell $(PKC) --cflags lgfx freetype)
LFLAGS += $(shell $(PKC) --libs lgfx freetype) -lm

ifeq ($(DISTO),linux)
LFLAGS += -lpthread
else ifeq ($(DISTO),kora)
CFLAGS += -Dmain=_main
endif


# src_logon-y = $(wildcard $(srcdir)/logon/*.c)
# $(eval $(call link_bin,logon,src_logon))

src_winmgr-y = $(wildcard $(srcdir)/winmgr/*.c)
$(eval $(call link_bin,winmgr,src_winmgr,LFLAGS))


install: $(call fn_inst,$(BINS) $(LIBS))

bins: $(BINS)

SRCS-y += src_winmgr-y

ifeq ($(NODEPS),)
-include $(call fn_deps,SRCS-y)
endif

