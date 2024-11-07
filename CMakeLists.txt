cmake_minimum_required(VERSION 3.19)

if(CMAKE_VERSION VERSION_GREATER "3.24.0")
  # FIXME this policy isn't compatible with versions prior to 3.24
  cmake_policy(SET CMP0135 NEW)
endif()
project(aedile VERSION 0.0.3)

include(ExternalProject)
include(FetchContent)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

FetchContent_GetProperties(uuid_v4)
include_directories(${uuid_v4_SOURCE_DIR})

#======== Configure noscrypt ========#
FetchContent_Declare(
    libnoscrypt
    GIT_REPOSITORY git@github.com:VnUgE/noscrypt.git
    GIT_TAG v0.1.5
)
FetchContent_MakeAvailable(libnoscrypt)

FetchContent_GetProperties(libnoscrypt)
include_directories(${libnoscrypt_SOURCE_DIR}/include)

set_target_properties(noscrypt PROPERTIES CRYPTO_LIB openssl)
set_target_properties(noscrypt PROPERTIES NC_ENABLE_UTILS ON)   #utilities library is required

#======== Build the project ========#
set(INCLUDE_DIR ./include)
set(CLIENT_INCLUDE_DIR ./include/client)
set(CRYPTOGRAPHY_INCLUDE_DIR ./include/cryptography)
set(DATA_INCLUDE_DIR ./include/data)
set(SERVICE_INCLUDE_DIR ./include/service)
set(SIGNER_INCLUDE_DIR ./include/signer)
include_directories(${INCLUDE_DIR})
include_directories(${CLIENT_INCLUDE_DIR})
include_directories(${CRYPTOGRAPHY_INCLUDE_DIR})
include_directories(${DATA_INCLUDE_DIR})
include_directories(${SIGNER_INCLUDE_DIR})

set(HEADERS
    ${INCLUDE_DIR}/nostr.hpp
    ${CLIENT_INCLUDE_DIR}/web_socket_client.hpp
    ${CLIENT_INCLUDE_DIR}/websocketpp_client.hpp
    ${DATA_INCLUDE_DIR}/data.hpp
    ${SERVICE_INCLUDE_DIR}/nostr_service_base.hpp
    ${SIGNER_INCLUDE_DIR}/signer.hpp
    ${SIGNER_INCLUDE_DIR}/noscrypt_signer.hpp
)

set(SOURCE_DIR ./src)
set(CLIENT_SOURCE_DIR ./src/client)
set(CRYPTOGRAPHY_SOURCE_DIR ./src/cryptography)
set(DATA_SOURCE_DIR ./src/data)
set(SERVICE_SOURCE_DIR ./src/service)
set(SIGNER_SOURCE_DIR ./src/signer)
set(SOURCES
    ${CLIENT_SOURCE_DIR}/websocketpp_client.cpp
    ${CRYPTOGRAPHY_SOURCE_DIR}/noscrypt_cipher.cpp
    ${CRYPTOGRAPHY_SOURCE_DIR}/nostr_secure_rng.cpp
    ${DATA_SOURCE_DIR}/event.cpp
    ${DATA_SOURCE_DIR}/filters.cpp
    ${SERVICE_SOURCE_DIR}/nostr_service_base.cpp
    ${SIGNER_SOURCE_DIR}/noscrypt_signer.cpp
)

add_library(aedile ${SOURCES})
target_link_libraries(aedile PRIVATE
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    plog::plog
    websocketpp::websocketpp
    noscrypt
)
target_include_directories(aedile PUBLIC ${INCLUDE_DIR})
set_target_properties(aedile PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

#======== Build the tests ========#
if(AEDILE_INCLUDE_TESTS)
    message(STATUS "Building unit tests.")

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
        ${TEST_DIR}/nostr_service_base_test.cpp
    )

    add_executable(aedile_test ${TEST_SOURCES} ${HEADERS})
    target_link_libraries(aedile_test PRIVATE
        GTest::gmock
        GTest::gtest
        GTest::gtest_main
        plog::plog
        websocketpp::websocketpp
        aedile
    )
    set_target_properties(aedile_test PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

    gtest_add_tests(TARGET aedile_test)
endif()
