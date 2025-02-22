cmake_minimum_required(VERSION 3.30)

file(STRINGS src/lb_version.txt LB_VERSION LIMIT_COUNT 1)
project(Lebensblume VERSION ${LB_VERSION} LANGUAGES CXX)

include(CheckIPOSupported)
check_ipo_supported()

# Common settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
include_directories(src)
add_compile_options(-march=native)
add_link_options(-march=native)
add_compile_definitions(LB_VERSION="${CMAKE_PROJECT_VERSION}")

add_executable(lebensblume src/main.cpp)
