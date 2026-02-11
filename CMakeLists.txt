cmake_minimum_required(VERSION 3.10)
project(static_radix_map VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost
find_package(Boost REQUIRED)

# Interface library (Header-only)
add_library(static_radix_map INTERFACE)

target_include_directories(static_radix_map INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(static_radix_map INTERFACE Boost::boost)

# Benchmark
add_executable(map_perf benchmark/map_perf.cpp)
target_link_libraries(map_perf PRIVATE static_radix_map Boost::boost)

# Install rules (Optional but good practice)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS static_radix_map EXPORT static_radix_mapTargets)
