cmake_minimum_required(VERSION 3.15)
project(Server)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Source files
file(GLOB MAILSERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

# First executable for SMTPClient
add_executable(SMTPClient 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SMTPClient.cc
)
# Third executable for thunderbird
add_executable(thunderbird
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thunderbird.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/email.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Client.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ComposeService.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GRPCCalls.cc
)
# Second executable for smtp
add_executable(smtp_server 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/smtp.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/email.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Client.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ComposeService.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GRPCCalls.cc
)
target_link_libraries(SMTPClient 
    protolib
    pthread
    resolv
)
target_link_libraries(thunderbird 
    protolib
    pthread
    resolv
)

target_link_libraries(smtp_server
    protolib
    pthread
)


target_link_libraries(thunderbird 
    protolib
    pthread
    resolv
)


# Include directories for protolib (if needed)
target_include_directories(protolib 
    PUBLIC 
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
)