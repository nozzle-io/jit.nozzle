cmake_minimum_required(VERSION 3.20)
project(bbb.nozzle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Universal binary (macOS) ---
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
endif()

 # --- Build nozzle from source (skip tests/examples) ---
set(NOZZLE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NOZZLE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# --- Match Max SDK CRT (static /MT on Windows) ---
if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
	set(CMAKE_C_FLAGS_DEBUG            "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1" CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_MINSIZEREL       "/MT /O1 /Ob1 /D NDEBUG" CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_RELEASE          "/MT /O2 /Ob2 /D NDEBUG" CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_RELWITHDEBINFO   "/MT /Zi /O2 /Ob1 /D NDEBUG" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_DEBUG          "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "/MT /O1 /Ob1 /D NDEBUG" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE        "/MT /O2 /Ob2 /D NDEBUG" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1 /D NDEBUG" CACHE STRING "" FORCE)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/nozzle)

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
