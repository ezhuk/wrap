add_executable(wrap-middleware)

target_sources(wrap-middleware
  PRIVATE
    main.cpp
)

target_link_libraries(wrap-middleware
  PRIVATE
    fmt::fmt
    wrap::wrap
)

set_target_properties(wrap-middleware PROPERTIES
  OUTPUT_NAME "wrap-middleware"
)
