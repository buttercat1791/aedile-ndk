cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0135 NEW)
project(NostrSDK VERSION 0.0.1)

include(ExternalProject)
include(FetchContent)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/_deps)

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    message(STATUS "Configuring as a subproject.")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/bin/)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/lib/)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/lib/)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../env/)

    set(Boost_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../env/boost/include)
else()
    message(STATUS "Configuring as a standalone project.")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/bin/)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/lib/)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/lib/)

    set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include/)
    set(OPENSSL_LIBRARIES ${OPENSSL_ROOT_DIR}/lib/)
endif()

if(DEFINED ENV{WORKSPACE})
    list(APPEND CMAKE_PREFIX_PATH $ENV{WORKSPACE}/env)
else()
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/../env)
endif()

#======== Find unmanaged dependencies ========#
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)

#======== Fetch header-only dependencies ========#
FetchContent_Declare(
    nlohmann_json
    URL      https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_Declare(
    plog
    URL https://github.com/SergiusTheBest/plog/archive/refs/tags/1.1.10.tar.gz
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY git@github.com:zaphoyd/websocketpp.git
    GIT_TAG 0.8.2
)
FetchContent_Declare(
    uuid_v4
    URL https://github.com/crashoz/uuid_v4/archive/refs/tags/v1.0.0.tar.gz
    USES_TERMINAL_DOWNLOAD TRUE
)

FetchContent_MakeAvailable(nlohmann_json plog uuid_v4 websocketpp)

#======== Build the project ========#
set(INCLUDE_DIR ./include)
set(CLIENT_INCLUDE_DIR ./include/client)
include_directories(${INCLUDE_DIR})
include_directories(${CLIENT_INCLUDE_DIR})

set(HEADERS
    ${INCLUDE_DIR}/nostr.hpp
    ${CLIENT_INCLUDE_DIR}/web_socket_client.hpp
)

set(SOURCE_DIR ./src)
set(CLIENT_SOURCE_DIR ./src/client)
set(SOURCES
    ${SOURCE_DIR}/event.cpp
    ${SOURCE_DIR}/filters.cpp
    ${SOURCE_DIR}/nostr_service.cpp
    ${CLIENT_SOURCE_DIR}/websocketpp_client.cpp
)

add_library(NostrSDK ${SOURCES} ${HEADERS})
target_link_libraries(NostrSDK PRIVATE
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    plog::plog
    uuid_v4::uuid_v4
    websocketpp::websocketpp
)
set_target_properties(NostrSDK PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

#======== Build the tests ========#
enable_testing()
include(GoogleTest)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_DIR ./test)
set(TEST_SOURCES
    ${TEST_DIR}/nostr_service_test.cpp
)

add_executable(NostrSDKTest ${TEST_SOURCES} ${HEADERS})
target_link_libraries(NostrSDKTest PRIVATE
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
    NostrSDK
    plog::plog
    uuid_v4::uuid_v4
    websocketpp::websocketpp
)
set_target_properties(NostrSDKTest PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

gtest_add_tests(TARGET NostrSDKTest)
