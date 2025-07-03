cmake_minimum_required(VERSION 3.15)
project(Client)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Frontend source files
set(FRONTEND_SOURCES
    src/HttpRequest.cc
    src/HttpConnection.cc
    src/HttpResponse.cc
    src/RequestHandler.cc
    src/FrontendServer.cc
    src/frontend.cc
    src/utils.cc
    src/Session.cc
    src/FileUtils.cc # Add the new FileUtils.cc file
    src/BalancerClient.cc
    src/CoordinatorClient.cc
    src/EmailClient.cc
)

# Add client library
add_library(clientlib src/Client.cc)
add_library(balancerlib src/BalancerClient.cc) # For BalancerClient.cc
add_library(coordinatorlib src/CoordinatorClient.cc) # For CoordinatorClient.cc
add_library(emailclientlib src/EmailClient.cc)

# Link protolib to clientlib
target_link_libraries(clientlib PUBLIC protolib)
target_link_libraries(balancerlib PUBLIC protolib) # For BalancerClient.cc
target_link_libraries(coordinatorlib PUBLIC protolib) # For CoordinatorClient.cc
target_link_libraries(emailclientlib PUBLIC protolib)


# Include directories for clientlib (so that dependents can find Client.h)
target_include_directories(clientlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(balancerlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src) # For BalancerClient.cc
target_include_directories(coordinatorlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src) # For CoordinatorClient.cc
target_include_directories(emailclientlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add executable for frontend
add_executable(frontend ${FRONTEND_SOURCES})

# Link libraries to frontend executable
target_link_libraries(frontend PRIVATE clientlib pthread)

# Include directories for frontend (includes src for other headers and binary dir for generated headers)
target_include_directories(frontend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Compilation flags
target_compile_options(frontend PRIVATE -std=c++17 -g)
