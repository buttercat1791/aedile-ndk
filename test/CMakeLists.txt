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

set(TEST_DIR ./)
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
