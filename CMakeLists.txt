cmake_minimum_required(VERSION 3.16)
project(router_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(router_sim src/main.cpp src/Network.cpp)
add_executable(run_tests tests/test_network.cpp src/Network.cpp)

enable_testing()
add_test(NAME network_tests COMMAND run_tests)
