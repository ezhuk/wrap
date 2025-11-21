find_package(GTest CONFIG REQUIRED)

add_executable(wrap_tests
  wrap_test.cpp
)

target_link_libraries(wrap_tests PRIVATE
  GTest::gtest_main
  wrap::wrap
)

include(GoogleTest)
gtest_discover_tests(wrap_tests)
