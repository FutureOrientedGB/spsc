cmake_minimum_required(VERSION 3.20)


set(PROJECT_NAME spsc)


project(${PROJECT_NAME})




# Defines
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
ADD_DEFINITIONS(-DVC_EXTRALEAN)
ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)



# Classify filters
FILE(GLOB_RECURSE HEADER_FILES
        "*.hpp"
)
FILE(GLOB_RECURSE SOURCE_FILES
        "*.cpp"
)
SOURCE_GROUP("Header Files" FILES ${HEADER_FILES})
SOURCE_GROUP("Source Files" FILES ${SRC_FILES})



# executable
add_executable(${PROJECT_NAME}
        WIN32
        ${HEADER_FILES}
        ${SOURCE_FILES}
)

# output dir
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY output/x64-windows-static
)


# Visual Studio - Properity - Linker - System - SubSystem > Console
if(MSVC)
set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
)
endif(MSVC)


# Visual Studio - Properity - C/C++ - Code Generation - Rutime Library > /MT
if(MSVC)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
endif(MSVC)

