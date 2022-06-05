cmake_minimum_required(VERSION 3.10)
project(dummy)

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 11)
add_compile_options(-g)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories("src")

FILE(GLOB SRC "src/*.cc")

add_executable(
  get_device_test
  test/get_device_test.cc
)

target_link_libraries(
  get_device_test
  gtest_main
  ibverbs
  glog
)
  
add_executable(
  RDMA_RC_example
  test/raw_test.c
)

target_link_libraries(
  RDMA_RC_example
  ibverbs
)

add_executable(
  tcp_connection_test
  test/tcp_connection_test.cc
  ${SRC}
)

target_link_libraries(
  tcp_connection_test
  gtest_main
  glog
  ibverbs
)

add_executable(
  server
  test/server.cc
  ${SRC}
)

target_link_libraries(
  server
  gtest_main
  glog
  ibverbs
)

add_executable(
  client
  test/client.cc
  ${SRC}
)

target_link_libraries(
  client
  gtest_main
  glog
  ibverbs
)

include(GoogleTest)
gtest_discover_tests(get_device_test)
gtest_discover_tests(tcp_connection_test)