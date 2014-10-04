#
# Makefile
# Copyright (C) 2014  Antonie Blom
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

CC = gcc
CFLAGS = -c -Iinclude -std=c99 -pedantic-errors -DBUILDFOR_LINUX \
	-DACC_VERSION=\"pre-alpha\"
LD = $(CC)

TARGET = acc

SOURCES ?= $(wildcard src/*.c) \
           $(wildcard src/itm/*.c) \
           $(wildcard src/parsing/*.c) \
           $(wildcard src/target/*.c) \
           $(wildcard src/target/cpu/*.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

release: CFLAGS += -DNDEBUG -O2
debug: CFLAGS += -g -DITM_COLORS=1
debug: LDFLAGS += -rdynamic

release debug: mksyms $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

mksyms:
	ln -sf cpus/$(shell uname -m) -T src/target/cpu
	ln -sf cpus/$(shell uname -m) -T include/acc/target/cpu

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm $(TARGET) $(OBJECTS) src/target/cpu include/acc/target/cpu
