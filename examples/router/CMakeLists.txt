add_executable(wrap-router)

target_sources(wrap-router
  PRIVATE
    main.cpp
)

target_link_libraries(wrap-router
  PRIVATE
    fmt::fmt
    wrap::wrap
)

set_target_properties(wrap-router PROPERTIES
  OUTPUT_NAME "wrap-router"
)
