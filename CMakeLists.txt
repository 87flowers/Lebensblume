cmake_minimum_required(VERSION 3.30)

file(STRINGS src/lb_version.txt LB_VERSION LIMIT_COUNT 1)
project(Lebensblume VERSION ${LB_VERSION} LANGUAGES CXX)

include(CheckIPOSupported)
check_ipo_supported()

enable_testing()
set(CMAKE_SKIP_TEST_ALL_DEPENDENCY FALSE)
function(do_test name)
  add_executable(test_${name} tests/${name}.cpp $<TARGET_OBJECTS:lb>)
  add_test(${name} test_${name})
endfunction()

# Common settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
include_directories(src)
add_compile_options(-march=native)
add_link_options(-march=native)
add_compile_definitions(LB_VERSION="${CMAKE_PROJECT_VERSION}")

add_library(lb OBJECT
  src/lb/bitboard.cpp
  src/lb/bitboard.h
  src/lb/types.h
)

add_executable(lebensblume src/main.cpp $<TARGET_OBJECTS:lb>)

do_test(test)
