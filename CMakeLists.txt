cmake_minimum_required(VERSION 3.20)
project(nozzle.max)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Universal binary (macOS) ---
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
endif()

# --- MSVC: Max SDK requires /MT (static CRT) ---
if(MSVC)
	cmake_policy(SET CMP0091 NEW)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

 # --- Build nozzle from source (skip tests/examples) ---
set(NOZZLE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NOZZLE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

option(JIT_NOZZLE_DEBUG "Enable debug logging for jit.nozzle externals" OFF)
option(NOZZLE_DEBUG "Enable debug logging in nozzle library" OFF)
if(JIT_NOZZLE_DEBUG)
	add_compile_definitions(JIT_NOZZLE_DEBUG=1)
endif()
if(NOZZLE_DEBUG)
	add_compile_definitions(NOZZLE_DEBUG=1)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/nozzle)

# Prevent nozzle's own option() from adding duplicate definitions
set(NOZZLE_DEBUG OFF CACHE BOOL "" FORCE)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/bbb_external.cmake)

set(C74_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/externals)

MACRO(SUBDIRLIST result curdir)
	FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
	SET(dirlist "")
	FOREACH(child ${children})
		IF(IS_DIRECTORY ${curdir}/${child})
			LIST(APPEND dirlist ${child})
		ENDIF()
	ENDFOREACH()
	SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach(project_dir ${PROJECT_DIRS})
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
		message("Generating: ${project_dir}")
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})
	endif()
endforeach()

set(CMAKE_SUPPRESS_REGENERATION true)
