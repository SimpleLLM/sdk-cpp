cmake_minimum_required(VERSION 3.16)
project(simplellm-sdk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

include(FetchContent)

# ---------------------------------------------------------------------------
# cpp-httplib (header-only with OpenSSL support)
# ---------------------------------------------------------------------------
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.5
)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cpp-httplib)

# ---------------------------------------------------------------------------
# nlohmann/json
# ---------------------------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# simplellm library
# ---------------------------------------------------------------------------
add_library(simplellm STATIC src/client.cpp)

target_include_directories(simplellm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# CPPHTTPLIB_OPENSSL_SUPPORT must be defined before including httplib.h
target_compile_definitions(simplellm PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

target_link_libraries(simplellm PUBLIC
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# cpp-httplib may expose itself as httplib::httplib or just add include dirs
if(TARGET httplib::httplib)
    target_link_libraries(simplellm PUBLIC httplib::httplib)
elseif(TARGET httplib)
    target_link_libraries(simplellm PUBLIC httplib)
else()
    # Fallback: manually add include dir
    FetchContent_GetProperties(cpp-httplib SOURCE_DIR cpp_httplib_SOURCE_DIR)
    if(NOT cpp_httplib_SOURCE_DIR)
        # Try the standard variable name
        FetchContent_GetProperties(cpp-httplib)
        set(cpp_httplib_SOURCE_DIR ${cpp-httplib_SOURCE_DIR})
    endif()
    target_include_directories(simplellm PUBLIC "${cpp_httplib_SOURCE_DIR}")
endif()

target_compile_features(simplellm PUBLIC cxx_std_17)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(SIMPLELLM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
