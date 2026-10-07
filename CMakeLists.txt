cmake_minimum_required(VERSION 3.21)
project(metric_protocol VERSION 1.0.0 LANGUAGES CXX)

add_library(metric_protocol src/MetricProtocol.cpp)
add_library(metric_protocol::metric_protocol ALIAS metric_protocol)
target_include_directories(metric_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(metric_protocol PUBLIC cxx_std_17)

option(METRIC_PROTOCOL_BUILD_TESTS "Build metric_protocol tests" ${PROJECT_IS_TOP_LEVEL})

if(METRIC_PROTOCOL_BUILD_TESTS)
    target_compile_options(metric_protocol PRIVATE -Wall -Wextra -Wconversion -Werror)

    include(FetchContent)
    FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(metric_protocol_tests test/test_metric_protocol.cpp)
    target_link_libraries(metric_protocol_tests PRIVATE metric_protocol GTest::gtest_main)
    target_compile_options(metric_protocol_tests PRIVATE -Wall -Wextra -Wconversion -Werror)

    include(GoogleTest)
    gtest_discover_tests(metric_protocol_tests)
endif()