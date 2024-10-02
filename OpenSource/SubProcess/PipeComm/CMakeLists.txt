cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(PROJECT_NAME PipeComm)
project(${PROJECT_NAME})

################################################################################
# Source groups
################################################################################
set(Headers
    "PipeComm.h"
    "ProcessorComm.h"
    "TransportComm.h"
)
source_group("Headers" FILES ${Headers})

set(Sources
    "PipeComm.cpp"
    "ProcessorComm.cpp"
    "TransportComm.cpp"
)
source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Headers}
    ${Sources}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/../$<CONFIG>/"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../$<CONFIG>/"
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../$<CONFIG>/"
)
 
################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "_DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "NDEBUG"
    ">"
    "_LIB;"
    "UNICODE;"
    "_UNICODE"
)

