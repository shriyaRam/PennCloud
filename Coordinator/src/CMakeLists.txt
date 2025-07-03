cmake_minimum_required(VERSION 3.15)
project(Coordinator)

# Add source files for the server executable, including ServiceLogic.cc

add_executable(Coordinator 
    ${CMAKE_CURRENT_SOURCE_DIR}/Mapper.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Heartbeat.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Commons.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Client.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Communicator.cc
)

# Link libraries (e.g., gRPC and protobuf)
target_link_libraries(Coordinator protolib pthread)

# Keep the original include directories for protolib
target_include_directories(protolib 
    PUBLIC 
        ${CMAKE_CURRENT_BINARY_DIR}  
)