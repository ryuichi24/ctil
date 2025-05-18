set(EXAMPLE_NAME libstr_example)

project(${EXAMPLE_NAME}
    VERSION 1.0.0
    DESCRIPTION "An example of ${EXAMPLE_NAME} executable project"
    LANGUAGES C
)

# make an executable target
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
    main.c
)

target_link_libraries(${PROJECT_NAME}
    libstr
)

# make a custom target/task
add_custom_target(build_${PROJECT_NAME})
add_dependencies(build_${PROJECT_NAME}
   ${PROJECT_NAME}
)
