cmake_minimum_required(VERSION 3.15)
project(LoadBalancer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Frontend source files
set(LOADBALANCER_SOURCES
    src/HttpRequest.cc
    src/HttpConnection.cc
    src/HttpResponse.cc
    src/LoadBalancer.cc
    src/balancer.cc
    src/utils.cc
    src/GrpcServer.cc
)

# # Add client library
add_library(serverlib src/GrpcServer.cc)

# # Link protolib to clientlib
target_link_libraries(serverlib PUBLIC protolib)

# # Include directories for clientlib (so that dependents can find Client.h)
target_include_directories(serverlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add executable for frontend
add_executable(balancer ${LOADBALANCER_SOURCES})

# # Link libraries to frontend executable
target_link_libraries(balancer PRIVATE serverlib pthread)

# Include directories for frontend (includes src for other headers and binary dir for generated headers)
target_include_directories(balancer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Compilation flags
target_compile_options(balancer PRIVATE -std=c++17 -g)
