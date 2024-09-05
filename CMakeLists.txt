cmake_minimum_required(VERSION 3.19)

if(CMAKE_VERSION VERSION_GREATER "3.24.0")
  # FIXME this policy isn't compatible with versions prior to 3.24
  cmake_policy(SET CMP0135 NEW)
endif()
project(aedile VERSION 0.0.2)

include(ExternalProject)
include(FetchContent)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

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
FetchContent_MakeAvailable(uuid_v4)

#======== Build the project ========#
list(APPEND INCLUDE_DIR ./include)
list(APPEND INCLUDE_DIR ./include/client)
list(APPEND INCLUDE_DIR ${CMAKE_SOURCE_DIR}/build/linux/_deps/uuid_v4-src/)

set(SOURCE_DIR ./src)
set(CLIENT_SOURCE_DIR ./src/client)
set(SOURCES
    ${SOURCE_DIR}/event.cpp
    ${SOURCE_DIR}/filters.cpp
    ${SOURCE_DIR}/bech32.cpp
    ${SOURCE_DIR}/nostr_service.cpp
    ${CLIENT_SOURCE_DIR}/websocketpp_client.cpp
)

add_library(aedile ${SOURCES})
target_link_libraries(aedile PRIVATE
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    plog::plog
    websocketpp::websocketpp
)
target_include_directories(aedile PUBLIC ${INCLUDE_DIR})
set_target_properties(aedile PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

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

add_executable(aedile_test ${TEST_SOURCES} ${HEADERS})
target_link_libraries(aedile_test PRIVATE
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
    aedile
    plog::plog
    websocketpp::websocketpp
)
target_include_directories(aedile_test PUBLIC ${INCLUDE_DIR})
set_target_properties(aedile_test PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

gtest_add_tests(TARGET aedile_test)
