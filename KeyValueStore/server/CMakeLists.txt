cmake_minimum_required(VERSION 3.15)
project(Server)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dataModel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    )
file(GLOB DATA_MODEL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/dataModel/*.cc")
file(GLOB MANAGER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/managers/*.cc")
file(GLOB UTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cc")
# Add source files for the server executable, including ServiceLogic.cc

add_executable(Server 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ServiceLogic.cc  # Add ServiceLogic.cc for implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InterServerLogic.cc  # Add InterServerLogic.cc for implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CoordinatorLogic.cc  # Add CoordinatorLogic.cc for implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/checkpoint.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RecoveryUtils.cc
    ${DATA_MODEL_SOURCES}
    ${MANAGER_SOURCES} 
    ${UTILS_SOURCES}
)

# Link libraries (e.g., gRPC and protobuf)
target_link_libraries(Server protolib pthread)

# Keep the original include directories for protolib
target_include_directories(protolib 
    PUBLIC 
        ${CMAKE_CURRENT_BINARY_DIR}  
)