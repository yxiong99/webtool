cmake_minimum_required(VERSION 3.10)

PROJECT( webpoll
         VERSION 1.1.0
         DESCRIPTION "Poll a file from a web server"
         LANGUAGES C )

set( CMAKE_BUILD_TYPE Debug )
set( CMAKE_CXX_FLAGS "-Wall" )

ADD_EXECUTABLE( webpoll
                src/main.c
                src/task.c
                src/cloud.c
                src/parse.c
                src/utils.c
                src/fsm/config.c
                src/fsm/functs.c
                src/fsm/input.c
                src/fsm/rhapsody.c )

add_definitions( -DWEBPOLL -DDOWNLOAD )

target_link_libraries( webpoll pthread )

include_directories( ${PROJECT_BINARY_DIR} )
include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( include include/fsm )

