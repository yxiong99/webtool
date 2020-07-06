cmake_minimum_required(VERSION 3.10)

PROJECT( webalive
         VERSION 1.1.2
         DESCRIPTION "Echo alive from a web server"
         LANGUAGES C )

set( CMAKE_BUILD_TYPE Debug )
set( CMAKE_CXX_FLAGS "-Wall" )

ADD_EXECUTABLE( webalive
                src/main.c
                src/task.c
                src/cloud.c
                src/parse.c
                src/utils.c )

add_definitions( -DWEBALIVE )

target_link_libraries( webalive pthread fsm )

include_directories( ${PROJECT_BINARY_DIR} )
include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( include include/fsm )

