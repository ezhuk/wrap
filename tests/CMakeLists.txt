find_package(GTest CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

add_executable(wrap_tests
  wrap_test.cpp
)

target_link_libraries(wrap_tests PRIVATE
  GTest::gtest_main
  httplib::httplib
  wrap::wrap
)

include(GoogleTest)
gtest_discover_tests(wrap_tests)
