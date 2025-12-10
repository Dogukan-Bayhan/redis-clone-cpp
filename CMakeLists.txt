cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-28-0
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
endif()

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Threads REQUIRED)

enable_testing()

set(SERVER_SOURCES ${SOURCE_FILES})
list(FILTER SERVER_SOURCES EXCLUDE REGEX "src/main\\.cpp")

add_library(redis_lib ${SERVER_SOURCES})
target_link_libraries(redis_lib PRIVATE asio Threads::Threads)

add_executable(redis src/main.cpp)
target_link_libraries(redis PRIVATE redis_lib)

file(GLOB TEST_SOURCES tests/*.cpp)
if(TEST_SOURCES)
    add_executable(redis_tests ${TEST_SOURCES})
    target_link_libraries(redis_tests PRIVATE redis_lib GTest::gtest_main)
    add_test(NAME redis_unit_tests COMMAND redis_tests)
endif()

file(GLOB BENCH_SOURCES benchmarks/*.cpp)
if(BENCH_SOURCES)
    add_executable(redis_bench ${BENCH_SOURCES})
    target_link_libraries(redis_bench PRIVATE redis_lib)
endif()
