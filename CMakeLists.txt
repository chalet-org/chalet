cmake_minimum_required(VERSION 3.16)

project(chalet-debug VERSION 0.32)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(FATAL_ERROR "Build must be Debug")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG  "-g3")

set(TARGET_NAME chalet-debug)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CLANG ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(GNU ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	set(INTEL ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(MSVC ON)
endif()

file(GLOB_RECURSE SOURCES
	src/*.cpp
	src/*.rc
	vendor/json-schema-validator/*.cpp
)

add_executable(${TARGET_NAME} ${SOURCES})

target_precompile_headers(${TARGET_NAME} PRIVATE src/PCH.hpp)

target_include_directories(${TARGET_NAME}
	PRIVATE
		src/
		vendor/json-schema-validator/
		vendor/
)

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "_DEBUG")
else()
    set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "NDEBUG")
endif()

if (UNIX)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

find_package(Threads REQUIRED)

# imagehlp

if(WIN32)
	target_link_libraries(${TARGET_NAME} Threads::Threads stdc++fs imagehlp)
elseif (CLANG)
	target_link_libraries(${TARGET_NAME} PRIVATE Threads::Threads)
else()
	target_link_libraries(${TARGET_NAME} stdc++fs)
endif()

if(MSVC)
	target_compile_options(${TARGET_NAME}
		PRIVATE
			/W4 /WX
	)
else()
	target_compile_options(${TARGET_NAME}
		PRIVATE
			-Wall -Werror -Wextra -Wpedantic -Wshadow -Wunreachable-code -Wunused -Wignored-qualifiers -Wcast-align -Wdouble-promotion -Wformat-nonliteral -Wformat=2 -Winvalid-pch -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wodr -fdiagnostics-color=always
		)
endif()

