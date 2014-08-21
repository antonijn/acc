#
# CMakeLists
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

cmake_minimum_required(VERSION 2.6)

project(acc C)

include_directories("include")

if (CMAKE_BUILD_TYPE MATCHES RELEASE)
	add_definitions(-DNDEBUG)
endif()

if (UNIX)
	if (APPLE)
		add_definitions(-DBUILDFOR_OSX)
	else()
		add_definitions(-DBUILDFOR_LINUX)
	endif()
	
	set(CMAKE_C_FLAGS "-std=c90 -pedantic-errors")
else()
	add_definitions(-DBUILDFOR_WINDOWS)
endif()

add_executable(acc
	src/emit/x86.c
	include/acc/emit/x86.h
	src/parsing/decl.c
	include/acc/parsing/decl.h
	src/parsing/expr.c
	include/acc/parsing/expr.h
	src/parsing/file.c
	include/acc/parsing/file.h
	src/parsing/stat.c
	include/acc/parsing/stat.h
	src/parsing/tools.c
	include/acc/parsing/tools.h
	src/ast.c
	include/acc/ast.h
	src/error.c
	include/acc/error.h
	src/ext.c
	include/acc/ext.h
	src/itm.c
	include/acc/itm.h
	src/list.c
	include/acc/list.h
	src/main.c
	src/options.c
	include/acc/options.h
	src/target.c
	include/acc/target.h
	src/token.c
	include/acc/token.h
)
