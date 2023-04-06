cmake_minimum_required(VERSION 3.16)

project(chalet-debug
	VERSION 0.6.0
	DESCRIPTION "A JSON-based project & build tool"
	HOMEPAGE_URL "https://www.chalet-work.space"
	LANGUAGES CXX
)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(FATAL_ERROR "Build must be Debug")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET_NAME chalet-debug)

configure_file(src/ChaletConfig.hpp.in ${PROJECT_BINARY_DIR}/ChaletConfig.hpp)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CLANG ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(GNU ON)
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

file(GLOB_RECURSE JSONSV_SOURCES
	external/json-schema-validator/src/*.cpp
)

add_library(json-schema-validator STATIC ${JSONSV_SOURCES})
target_compile_definitions(json-schema-validator
	PRIVATE
		JSON_NOEXCEPTION
		JSONSV_NOEXCEPTION
)
target_include_directories(json-schema-validator PRIVATE
	external/json-schema-validator/src/
	external/nlohmann/single_include/
)

file(GLOB_RECURSE TARGET_SOURCES
	src/*.cpp
	src/*.rc
)

add_executable(${TARGET_NAME} ${TARGET_SOURCES})
target_compile_definitions(${TARGET_NAME}
	PRIVATE
		JSON_NOEXCEPTION
		JSONSV_NOEXCEPTION
)

target_precompile_headers(${TARGET_NAME} PRIVATE src/PCH.hpp)

target_include_directories(${TARGET_NAME}
	PRIVATE
		src/
		external/fmt/include/
		external/json-schema-validator/src/
		external/nlohmann/single_include/
		external/stduuid/include/
		external/stduuid/gsl/
		external/
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

set(STATIC_LINKS
	json-schema-validator
)
# imagehlp

if(WIN32)
	if (MSVC)
		target_link_libraries(${TARGET_NAME} Threads::Threads ${STATIC_LINKS} DbgHelp)
	else()
		target_link_libraries(${TARGET_NAME} -static Threads::Threads ${STATIC_LINKS} stdc++fs imagehlp)
	endif()
elseif (CLANG)
	target_link_libraries(${TARGET_NAME} Threads::Threads ${STATIC_LINKS})
else()
	target_link_libraries(${TARGET_NAME} Threads::Threads ${STATIC_LINKS} stdc++fs uuid)
endif()

if(MSVC)
	target_compile_definitions(${TARGET_NAME}
		PRIVATE
			_CRT_SECURE_NO_WARNINGS
			_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING
			NOMINMAX
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

if (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
	target_link_libraries(${TARGET_NAME} "-framework CoreFoundation")
	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
endif()

