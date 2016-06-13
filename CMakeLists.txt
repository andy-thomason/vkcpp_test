cmake_minimum_required (VERSION 2.6)
project (vkcpp_test)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(include)
add_executable(vkpp_test main.cpp)
target_link_libraries(vkpp_test vulkan)

