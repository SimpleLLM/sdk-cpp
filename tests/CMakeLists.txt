include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(simplellm_tests test_main.cpp)

target_link_libraries(simplellm_tests PRIVATE
    simplellm
    doctest::doctest
)

target_compile_features(simplellm_tests PRIVATE cxx_std_17)

# Use the doctest CMake integration for test discovery
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(simplellm_tests)
