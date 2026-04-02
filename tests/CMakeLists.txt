include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(ssrag_tests
    fingerprint_test.cpp
    document_store_test.cpp
    lambda_fold_test.cpp
)

target_link_libraries(ssrag_tests
    PRIVATE
        ssrag::ssrag
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ssrag_tests)
