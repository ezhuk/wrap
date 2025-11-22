find_package(benchmark CONFIG REQUIRED)

add_executable(wrap_bench
  main.cpp
)

target_link_libraries(wrap_bench PRIVATE
  benchmark::benchmark
  wrap::wrap
)
