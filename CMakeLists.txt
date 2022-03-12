cmake_minimum_required(VERSION 3.13)

project(protei-test-task)

set(LIBRARY_NAME transport)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

function(SetDefaultValue VARIABLE DEFAULT_VALUE)
    if (DEFINED ${VARIABLE})
    else ()
        set(${VARIABLE} ${DEFAULT_VALUE} PARENT_SCOPE)
    endif ()
endfunction()


find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND AND USE_CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    message(STATUS "Use ccache")
endif(CCACHE_FOUND AND USE_CCACHE)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g -std=c++17 -DPUGIXML_HEADER_ONLY -DPUGIXML_NO_XPATH")

# Add warnings
set(ADDITIONAL_BINARY_COMPILE_FLAGS "-Wall                  \
                                    -Wextra                 \
                                    -Wshadow                \
                                    -Wnon-virtual-dtor      \
                                    -Wimplicit-fallthrough  \
                                    -Wreorder               \
                                    -Wreturn-type           \
                                    -Wstrict-aliasing       \
                                    -Wswitch                \
                                    -Wuninitialized         \
                                    -Wno-unused-parameter   \
                                    -Wno-unused-private-field   \
                                    -Wno-unused-function    \
                                    -Wparentheses           \
                                    -Werror")

# Allow warnings in
set(ADDITIONAL_BINARY_TEST_COMPILE_FLAGS "-Wall                 \
                                        -Wextra                 \
                                        -Wshadow                \
                                        -Wparentheses           \
                                        -Wnon-virtual-dtor      \
                                        -Wno-unused-parameter   \
                                        -Wno-unused-function    \
                                        -Wall")
SetDefaultValue(TOOLCHAIN GNU)

# Select toolchain
if (${TOOLCHAIN} STREQUAL "LLVM")
    message(STATUS "Select LLVM toolchain")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined,integer,alignment")
    set(LDFLAGS "${LDFLAGS} --coverage")
    set(LIBRARY_NAME "${LIBRARY_NAME}.llvm")
else ()
    message(STATUS "Use default c++ compiler ${CXX}")
endif ()


# Add optimization flags
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "Release build, optimization set to -O3")
    set(ADDITIONAL_BINARY_COMPILE_FLAGS "${ADDITIONAL_BINARY_COMPILE_FLAGS} -O2")
    add_definitions(-DRELEASE_BUILD)
else()
    message(STATUS "Not release build, optimization set to -O0")
    set(ADDITIONAL_BINARY_COMPILE_FLAGS "${ADDITIONAL_BINARY_COMPILE_FLAGS} -O0")
    add_definitions(-DDEBUG_BUILD)
endif ()


# Add local files
file(GLOB_RECURSE BINARY_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h ${CMAKE_SOURCE_DIR}/src/*.tpp)
file(GLOB_RECURSE BINARY_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/test/*.cpp
        ${CMAKE_SOURCE_DIR}/test/*.hpp)

message(STATUS "Build static lib")
add_library(Transport STATIC ${BINARY_HEADERS} ${BINARY_SOURCES})

# include local files
target_include_directories(Transport PRIVATE "./include")
target_link_libraries(Transport PUBLIC pthread)

set_target_properties(Transport PROPERTIES COMPILE_FLAGS "${ADDITIONAL_BINARY_COMPILE_FLAGS}")

set_target_properties(Transport PROPERTIES OUTPUT_NAME "${LIBRARY_NAME}")
set_target_properties(Transport PROPERTIES DEBUG_POSTFIX ".debug")

# Start gtest
enable_testing()
include(GoogleTest)

set(BINARY_TEST_NAME TEST.${CMAKE_BUILD_TYPE}.ns)

add_executable(Test
        ${BINARY_HEADERS}
        ${BINARY_SOURCES}
        ${TEST_SOURCES}
        ./test/test_main.cpp)


set_target_properties(Test PROPERTIES OUTPUT_NAME "${BINARY_TEST_NAME}")
set_target_properties(Test PROPERTIES COMPILE_FLAGS "${ADDITIONAL_BINARY_TEST_COMPILE_FLAGS}")

# googletest targets
add_library(googletest external/googletest/googletest/src/gtest-all.cc)
add_library(googlemock external/googletest/googlemock/src/gmock-all.cc)

target_include_directories(googletest PUBLIC external/googletest/googletest/include external/googletest/googletest)
target_include_directories(googlemock PUBLIC external/googletest/googletest/include external/googletest/googletest)
target_include_directories(googlemock PUBLIC external/googletest/googlemock/include external/googletest/googlemock)
target_include_directories(Test PUBLIC external/googletest/googletest/include external/googletest/googletest)
target_include_directories(Test PUBLIC external/googletest/googlemock/include external/googletest/googlemock)
target_include_directories(Test PUBLIC "./include")

target_link_libraries(Test
        googletest
        googlemock
        pthread)

add_dependencies(Test googletest googlemock)

add_executable(client app/client_main.cpp)
target_link_libraries(client Transport pthread)
target_include_directories(client PUBLIC "./include")

add_executable(server app/server_main.cpp)
target_link_libraries(server Transport pthread)
target_include_directories(server PUBLIC "./include")
