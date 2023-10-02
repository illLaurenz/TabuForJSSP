cmake_minimum_required(VERSION 3.5)
project(Hybrid_Evo_Algorithm)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

add_subdirectory(src)
add_executable(example examples.cpp)
target_link_libraries(example PRIVATE jssp tabu memetic heuristic)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE jssp tabu memetic heuristic)

#               ^^^^^^^^^ Standalone ^^^^^^^^^
# -------------------------------------------------------
#               vvvvvvvvv  PyBind11  vvvvvvvvv (uncomment below if you want to build a python interface with pybind11)

# add_subdirectory(pybind11)
# pybind11_add_module(python_interface pybind_main.cpp)

# target_compile_definitions(python_interface PRIVATE jssp tabu memetic)
