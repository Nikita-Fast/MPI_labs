cmake_minimum_required(VERSION 3.20)

project(Lab2_LU_MPI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED)

add_executable(comp_lu src/comp_lu.cpp)
target_link_libraries(comp_lu PRIVATE MPI::MPI_CXX)

set_target_properties(comp_lu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
)
