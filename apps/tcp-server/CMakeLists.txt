# set a minumum version of cmake
cmake_minimum_required(VERSION 3.31.2)

project(tcp-server
    VERSION 1.0.0
    DESCRIPTION "Simple TCP Server"
    LANGUAGES C
)

# make an executable target
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
    src/main.c
)

target_link_libraries(${PROJECT_NAME}
    # 
)
