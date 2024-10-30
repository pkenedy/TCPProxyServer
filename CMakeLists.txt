cmake_minimum_required(VERSION 3.10)

project(TCPProxyServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(TCPProxyServer tcp_proxy_server.cpp)

# Link directly to Ws2_32 by name
target_link_libraries(TCPProxyServer Ws2_32)
