project (acc C)

cmake_minimum_required (VERSION 2.6)

include_directories("include")

if (UNIX)
	if (APPLE)
		add_definitions(-DBUILDFOR_OSX)
	else()
		add_definitions(-DBUILDFOR_LINUX)
	endif()
	
	set(CMAKE_C_FLAGS "-std=c90 -pedantic-errors")
	
	set(CMAKE_C_FLAGS_RELEASE "-O3")
	set(CMAKE_C_FLAGS_DEBUG "-g")
else()
	# TODO: what do we do here?
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
