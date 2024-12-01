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
    GIT_REPOSITORY https://github.com/crashoz/uuid_v4.git
    GIT_TAG v1.0.0
)
FetchContent_Populate(uuid_v4)

FetchContent_GetProperties(uuid_v4)
include_directories(${uuid_v4_SOURCE_DIR})

#======== Configure noscrypt ========#
set(CRYPTO_LIB openssl)
set(NC_ENABLE_UTILS ON)

FetchContent_Declare(
    libnoscrypt
    GIT_REPOSITORY https://github.com/VnUgE/noscrypt.git
    GIT_TAG v0.1.5
)
FetchContent_MakeAvailable(libnoscrypt)
FetchContent_GetProperties(libnoscrypt)

#======== Build the project ========#
set(AEDILE_HEADERS
    "include/nostr.hpp"
    "include/client/web_socket_client.hpp"
    "include/client/websocketpp_client.hpp"
    "include/data/data.hpp"
    "include/service/nostr_service_base.hpp"
    "include/service/nostr_service.hpp"
    "include/service/nostr_service_specializations.hpp"
    "include/signer/signer.hpp"
    "include/signer/noscrypt_signer.hpp"
    "src/cryptography/noscrypt_cipher.hpp"
    "src/cryptography/nostr_secure_rng.hpp"
    "src/internal/noscrypt_logger.hpp"
)

set(AEDILE_SOURCES
    "src/client/websocketpp_client.cpp"
    "src/cryptography/noscrypt_cipher.cpp"
    "src/cryptography/nostr_secure_rng.cpp"
    "src/data/event.cpp"
    "src/data/filters.cpp"
    "src/internal/noscrypt_logger.cpp"
    "src/service/nostr_service_base.cpp"
    "src/signer/noscrypt_signer.cpp"
)

add_library(aedile ${AEDILE_SOURCES} ${AEDILE_HEADERS})
target_link_libraries(aedile PRIVATE
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    plog::plog
    websocketpp::websocketpp
    noscrypt
)
target_include_directories(aedile PRIVATE include)
target_include_directories(aedile PRIVATE ${libnoscrypt_SOURCE_DIR}/include)
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
        "test/nostr_event_test.cpp"
        "test/nostr_service_base_test.cpp"
    )

    add_executable(aedile_test ${TEST_SOURCES})
    target_link_libraries(aedile_test PRIVATE
        GTest::gmock
        GTest::gtest
        GTest::gtest_main
        aedile
        nlohmann_json::nlohmann_json
    )
    target_include_directories(aedile_test PRIVATE include)
    target_include_directories(aedile_test PRIVATE src)
    target_include_directories(aedile_test PRIVATE ${libnoscrypt_SOURCE_DIR}/include)
    set_target_properties(aedile_test PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

    gtest_add_tests(TARGET aedile_test)
endif()
