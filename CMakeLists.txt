cmake_minimum_required(VERSION 3.5)
project(Hybrid_Evo_Algorithm)

set(CMAKE_CXX_STANDARD 17)

#profiler settings
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

add_executable(test test.cpp jssp.cpp jssp.h ts.cpp ts.h mem.cpp mem.h)

target_link_libraries(test PRIVATE)

#               ^^^^^^^^^ Standalone ^^^^^^^^^
# -------------------------------------------------------
#               vvvvvvvvv  PyBind11  vvvvvvvvv

#cmake_minimum_required(VERSION 3.5)
#project(Hybrid_Evo_Algorithm)

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#set(PYTHON_EXECUTABLE /home/laurenz/anaconda3/envs/jssp/bin/python)

add_subdirectory(pybind11)
pybind11_add_module(metaheuristic main.cpp jssp.cpp jssp.h ts.cpp ts.h mem.cpp mem.h)

target_compile_definitions(metaheuristic PRIVATE)
