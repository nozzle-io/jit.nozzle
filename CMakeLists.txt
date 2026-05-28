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
	set(NOZZLE_DEBUG ON CACHE BOOL "Enable debug logging" FORCE)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/nozzle)

if(NOZZLE_DEBUG)
	target_compile_definitions(nozzle PRIVATE NOZZLE_DEBUG=1)
endif()

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

option(JIT_NOZZLE_BUILD_TESTS "Build format mapping unit tests" OFF)
if(JIT_NOZZLE_BUILD_TESTS)
    enable_testing()
    add_executable(test_format_mapping tests/test_format_mapping.cpp)
    target_include_directories(test_format_mapping PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source/common
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/nozzle/include)
    add_test(NAME test_format_mapping COMMAND test_format_mapping)

    add_executable(test_3plane_copy tests/test_3plane_copy.cpp)
    target_include_directories(test_3plane_copy PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source/common
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/nozzle/include)
    add_test(NAME test_3plane_copy COMMAND test_3plane_copy)

    add_executable(test_matrix_copy_dispatch tests/test_matrix_copy_dispatch.cpp)
    target_include_directories(test_matrix_copy_dispatch PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source/common
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/nozzle/include)
    add_test(NAME test_matrix_copy_dispatch COMMAND test_matrix_copy_dispatch)
endif()
