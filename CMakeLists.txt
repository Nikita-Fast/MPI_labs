cmake_minimum_required(VERSION 3.20)

project(MPI_labs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED COMPONENTS CXX)

add_executable(comp_pi Lab1/src/comp_pi.cpp)
target_link_libraries(comp_pi PRIVATE MPI::MPI_CXX)

add_executable(count_primes Lab1/src/count_primes.cpp)
target_link_libraries(count_primes PRIVATE MPI::MPI_CXX)

add_executable(comp_lu Lab2/src/comp_lu.cpp)
target_link_libraries(comp_lu PRIVATE MPI::MPI_CXX)
