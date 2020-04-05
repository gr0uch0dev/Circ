cmake_minimum_required(VERSION 3.5.1)
project(chirc C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "/build")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY {CMAKE_BINARY_DIR}/build)

include_directories(src
    # External libraries: Add lib/ directories here
    )

add_executable(chirc
    src/main.c
    src/log.c
        src/interfaces/utils.h
        src/modules/utils.c)

target_link_libraries(chirc pthread)

add_custom_target(link_tests ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../tests/ tests
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../tests/pytest.ini pytest.ini)

set(ASSIGNMENTS
    1 2 3 4 5)

foreach(AID ${ASSIGNMENTS})

    add_custom_target(categories-assignment-${AID}
                COMMAND ../venv/bin/python3 tests/print-categories.py tests/rubrics/assignment-${AID}.json
                DEPENDS link_tests)

    add_custom_target(tests-assignment-${AID}
            COMMAND ../venv/bin/python3 ../venv/bin/pytest --chirc-rubric tests/rubrics/assignment-${AID}.json
            DEPENDS chirc link_tests)

    add_custom_target(grade-assignment-${AID}
            COMMAND ../venv/bin/python3 tests/grade.py tests/rubrics/assignment-${AID}.json)

    add_custom_target(assignment-${AID}
            COMMAND echo "Building assignment ${AID}"

            COMMAND /bin/sh -c ' ../venv/bin/python3 ../venv/bin/pytest --chirc-rubric tests/rubrics/assignment-${AID}.json\; exit 0'
            COMMAND ../venv/bin/python3 tests/grade.py tests/rubrics/assignment-${AID}.json
            DEPENDS chirc link_tests)

endforeach()



add_executable(server Learning/Socket/server.c)
add_executable(client Learning/Socket/client.c)
add_executable(whois Learning/Socket/whois.c)

add_executable(showip Learning/Socket/showip.c)