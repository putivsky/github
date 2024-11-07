cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(PROJECT_NAME MultiIndexTest)
project(${PROJECT_NAME})

################################################################################
# Source groups
################################################################################
set(Headers
    "../MultiIndexLib/MultiIndex.h"
    "../MultiIndexLib/OrderedMutliSet.h"
    "../MultiIndexLib/UnorderedMutliSet.h"
)

source_group("Headers" FILES ${Headers})

set(Sources
    "main.cpp"
)

source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Headers}
    ${Sources}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

################################################################################
# Output directory
################################################################################
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})


set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../MultiIndexLib"
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
    "_CRT_SECURE_NO_WARNINGS;"
    "_CONSOLE;"
    "UNICODE;"
    "_UNICODE"
)

################################################################################
# Dependencies
################################################################################
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

target_link_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../$<CONFIG>/"
)

