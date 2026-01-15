add_executable(wrap-static)

target_sources(wrap-static
  PRIVATE
    main.cpp
)

target_link_libraries(wrap-static
  PRIVATE
    fmt::fmt
    wrap::wrap
)

set_target_properties(wrap-static PROPERTIES
  OUTPUT_NAME "wrap-static"
)
