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
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fconstexpr-ops-limit=1000000000)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-fconstexpr-steps=1000000000)
endif()


add_library(lb OBJECT
  src/lb/assert.cpp
  src/lb/assert.h
  src/lb/attacks.cpp
  src/lb/attacks.h
  src/lb/board.cpp
  src/lb/board.h
  src/lb/common.h
  src/lb/geometry.cpp
  src/lb/geometry.h
  src/lb/movegen.cpp
  src/lb/movegen.h
  src/lb/perft.cpp
  src/lb/perft.h
  src/lb/tokenizer.h
  src/lb/types.h
  src/lb/util/pext.h
  src/lb/util/static_vector.h
)

add_executable(lebensblume src/main.cpp $<TARGET_OBJECTS:lb>)

do_test(board)
do_test(common)
do_test(movegen)
do_test(perft)
