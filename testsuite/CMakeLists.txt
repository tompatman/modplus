# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
#Set the project name
set(PROJ_NAME Test_modplus)

project (${PROJ_NAME})


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/${PROJ_NAME}/bin)

file(GLOB PROJ_SRC
        "./src/*.cpp"
        "*.cpp"
        )

include_directories( ../include
        /usr/local/include/modbus)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/output/lib)


set(SOURCE_FILES ${PROJ_SRC})
add_executable(${PROJ_NAME} ${SOURCE_FILES})


#Required libraries for link
target_link_libraries(${PROJ_NAME}
        PRIVATE modplus
        PRIVATE gtest
        PRIVATE gmock
        PUBLIC PocoUtil
        PUBLIC PocoFoundation
        PUBLIC PocoData
        PUBLIC PocoDataMySQL
        PUBLIC PocoNet
        PUBLIC PocoXML
        #        PUBLIC zmq
        PUBLIC modbus
        PUBLIC pthread
        PUBLIC z
        PUBLIC rt)

set(UNIT_TEST ${PROJ_NAME})

IF (CMAKE_BUILD_TYPE MATCHES Release)
    add_custom_command(
            TARGET ${UNIT_TEST}
            COMMENT "Run tests"
            POST_BUILD
            COMMAND ${UNIT_TEST}
    )
ENDIF (CMAKE_BUILD_TYPE MATCHES Release)

