cmake_minimum_required(VERSION 3.18.0)
cmake_policy(VERSION 3.18.0)

# Output
project(image_blend VERSION 0.1.0 LANGUAGES CXX)
add_executable(image_blend main.cpp)

# Compilation options
set(CMAKE_CXX_STANDARD_REQUIRED "ON")
set_property(TARGET image_blend PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET image_blend PROPERTY CXX_STANDARD 20)
option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." True)

if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

find_library(libOEXR NAMES IlmImf)
find_library(libHalf NAMES Half)

if (UNIX)
	target_compile_options(image_blend PRIVATE 
		-march=native -pipe 
		-mavx2
	)

	add_definitions(-D_linux -D__AVX2__)

	target_link_libraries(image_blend PUBLIC
		stdc++fs atomic
		png
		${libOEXR}
		${libHalf}
	)
endif (UNIX)

# Include paths
target_include_directories(image_blend PRIVATE 
	include/
	/usr/include/Imath
)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

