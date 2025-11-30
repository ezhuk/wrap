add_executable(wrap-hello)

target_sources(wrap-hello
  PRIVATE
    main.cpp
)

target_link_libraries(wrap-hello
  PRIVATE
    fmt::fmt
    wrap::wrap
)

set_target_properties(wrap-hello PROPERTIES
  OUTPUT_NAME "wrap-hello"
)
