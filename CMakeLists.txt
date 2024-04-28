cmake_minimum_required(VERSION 3.19)
cmake_policy(SET CMP0135 NEW)
project(NostrSDK VERSION 0.1.0)

include(ExternalProject)
include(FetchContent)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    message(STATUS "Configuring as a subproject.")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/bin/)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/lib/)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/lib/)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../env/)

    if(DEFINED ENV{WORKSPACE})
        list(APPEND CMAKE_PREFIX_PATH $ENV{WORKSPACE}/env)
    else()
        list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/../env)
    endif()
else()
    message(STATUS "Configuring as a standalone project.")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/bin/)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/lib/)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/lib/)
endif()

#======== Find dependencies ========#
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(plog CONFIG REQUIRED)
find_package(websocketpp CONFIG REQUIRED)

#======== Configure uuid_v4 ========#
FetchContent_Declare(
    uuid_v4
    GIT_REPOSITORY git@github.com:crashoz/uuid_v4.git
    GIT_TAG v1.0.0
)
FetchContent_Populate(uuid_v4)
set(uuid_v4_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/_deps/uuid_v4-src/)

find_path(uuid_v4_INCLUDE_DIR uuid_v4.h)
include_directories(${uuid_v4_INCLUDE_DIR})

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
    websocketpp::websocketpp
)
set_target_properties(NostrSDKTest PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

gtest_add_tests(TARGET NostrSDKTest)
