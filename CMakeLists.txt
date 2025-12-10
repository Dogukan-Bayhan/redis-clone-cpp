cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

enable_testing()

set(SERVER_SOURCES ${SOURCE_FILES})
list(FILTER SERVER_SOURCES EXCLUDE REGEX "src/main\\.cpp")

add_library(redis_lib ${SERVER_SOURCES})
target_link_libraries(redis_lib PRIVATE asio asio::asio Threads::Threads)

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
