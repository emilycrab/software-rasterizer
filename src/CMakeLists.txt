find_package(SDL2 REQUIRED)

add_executable(
  software_rasterizer
  main.cpp
)
set_property(TARGET software_rasterizer PROPERTY CXX_STANDARD 20)
target_link_libraries(software_rasterizer PRIVATE SDL2::SDL2)