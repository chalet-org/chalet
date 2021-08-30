cmake_minimum_required(VERSION 3.16)

project(chalet-debug VERSION 0.32)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(FATAL_ERROR "Build must be Debug")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET_NAME chalet-debug)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CLANG ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(GNU ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	set(INTEL ON)
endif()

if (MSVC)
	foreach(config DEBUG RELEASE MINSIZEREL RELWITHDEBINFO)
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} ${CMAKE_BINARY_DIR})
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config} ${CMAKE_BINARY_DIR})
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_BINARY_DIR})
	endforeach()
else()
	set(CMAKE_CXX_FLAGS_DEBUG "-g3")
endif()

file(GLOB_RECURSE SOURCES
	src/*.cpp
	src/*.rc
	vendor/json-schema-validator/src/*.cpp
	vendor/subprocess/src/cpp/*.cpp
)

add_executable(${TARGET_NAME} ${SOURCES})
add_definitions(-DJSON_NOEXCEPTION -DJSONSV_NOEXCEPTION)

target_precompile_headers(${TARGET_NAME} PRIVATE src/PCH.hpp)

target_include_directories(${TARGET_NAME}
	PRIVATE
		src/
		vendor/nlohmann/single_include/
		vendor/json-schema-validator/src/
		vendor/fmt/include/
		vendor/argparse/include/
		vendor/glob/single_include/
		vendor/subprocess/src/cpp/
		vendor/ust/
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
	if (MSVC)
		target_link_libraries(${TARGET_NAME} Threads::Threads DbgHelp)
	else()
		target_link_libraries(${TARGET_NAME} -static Threads::Threads stdc++fs imagehlp)
	endif()
elseif (CLANG)
	target_link_libraries(${TARGET_NAME} Threads::Threads)
else()
	target_link_libraries(${TARGET_NAME} Threads::Threads stdc++fs unwind)
endif()

if(MSVC)
	target_compile_options(${TARGET_NAME}
		PRIVATE
			-D_CRT_SECURE_NO_WARNINGS
			-D_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING
			-DNOMINMAX
	)

	target_compile_options(${TARGET_NAME}
		PRIVATE
			/W4 /WX /utf-8
	)
else()
	target_compile_options(${TARGET_NAME}
		PRIVATE
			-Wall -Werror -Wextra -Wpedantic -Wshadow -Wunreachable-code -Wunused -Wignored-qualifiers -Wcast-align -Wdouble-promotion -Wformat-nonliteral -Wformat=2 -Winvalid-pch -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wodr -fno-rtti -fdiagnostics-color=always
		)
endif()

