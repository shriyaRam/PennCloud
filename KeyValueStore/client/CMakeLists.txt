cmake_minimum_required(VERSION 3.15)
project(Client)

# Add source files for the client executable
add_executable(Client 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Client.cc  # Client implementation
)

# Link libraries (e.g., protolib, gRPC, and protobuf)
target_link_libraries(Client 
    protolib        # Link the protolib library (your own or pre-built)
    gRPC::grpc++    # Link gRPC C++ library
    protobuf::libprotobuf  # Link protobuf C++ library
)

# Keep the original include directories for protolib
target_include_directories(protolib 
    PUBLIC 
        ${CMAKE_CURRENT_BINARY_DIR}  # This keeps the protolib include directory
)
