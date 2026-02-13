cmake_minimum_required(VERSION 3.14)
project(static_radix_map VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Interface library (Header-only, no external dependencies)
add_library(static_radix_map INTERFACE)

target_include_directories(static_radix_map INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Benchmark
add_executable(map_perf benchmark/map_perf.cpp)
target_link_libraries(map_perf PRIVATE static_radix_map)
target_compile_options(map_perf PRIVATE -O2)

# Unit Tests
add_executable(unit_tests tests/unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE static_radix_map)

enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)

# Optional: Sanitizer support
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Install rules
install(DIRECTORY include/ DESTINATION include)
install(TARGETS static_radix_map EXPORT static_radix_mapTargets)
