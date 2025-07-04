cmake_minimum_required(VERSION 3.15)
project(daggle 
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(PATCH_VERSION 0)
set(ABI_VERSION 1)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/daggle)
configure_file(
    include/daggle.h.in
    ${CMAKE_BINARY_DIR}/include/daggle/daggle.h
)

################################################################################

set(DAGGLE_SRC
    src/api_data.c
    src/api_dynamic_plugin.c
    src/api_instance.c
    src/api_etc.c
    src/api_execution.c
    src/api_graph.c
    src/api_node.c
    src/api_port.c
    src/api_tasks.c
    src/closure.c
    src/data_container.c
    src/dynamic_array.c
    src/executor.c
    src/hash.c
    src/llist_queue.c
    src/node.c
    src/plugin_manager.c
    src/ports.c
    src/resource_container.c
    src/serialization.c
    src/thread_safe_llist_queue.c
)

add_library(daggle SHARED ${DAGGLE_SRC})

target_include_directories(daggle
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

set_target_properties(daggle PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_definitions(daggle PRIVATE 
    DAGGLE_EXPORTS
    DAGGLE_ENABLE_LOG
    DAGGLE_ENABLE_LOG_CONDITIONAL_DEBUG
    DAGGLE_ENABLE_LOG_LOCATION
    DAGGLE_ENABLE_RETURN_STATUS_ERROR_LOGS
    #DAGGLE_ENABLE_RETURN_STATUS_SUCCESS_LOGS
    DAGGLE_ENABLE_ASSERT
)

################################################################################

include(CMakePackageConfigHelpers)

message(STATUS "Generating daggleConfig.cmake")

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/daggleConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/daggleConfig.cmake"
    INSTALL_DESTINATION lib/cmake/daggle
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/daggleConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/daggleConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/daggleConfigVersion.cmake"
    DESTINATION lib/cmake/daggle
)

################################################################################

install(TARGETS daggle
    EXPORT daggleTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION include)

install(EXPORT daggleTargets
    FILE daggleTargets.cmake
    NAMESPACE daggle::
    DESTINATION lib/cmake/daggle
)

configure_file(daggle.pc.in daggle.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/daggle.pc DESTINATION lib/pkgconfig)

################################################################################

set(CORE_SRC
    plugins/core/src/types/int.c
    plugins/core/src/types/float.c
    plugins/core/src/types/double.c
    plugins/core/src/types/bool.c
    plugins/core/src/types/string.c
    plugins/core/src/types/bytes.c
    plugins/core/src/nodes/input.c
    plugins/core/src/nodes/math.c
    plugins/core/src/nodes/output.c
    plugins/core/src/main.c
)

add_library(core SHARED ${CORE_SRC})

target_include_directories(core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/core/private
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/core/include
)

target_link_libraries(core PRIVATE daggle)

################################################################################

set(GRAPH_SRC
    plugins/graph/main.c
)

add_library(graph SHARED ${GRAPH_SRC})

target_link_libraries(graph PRIVATE daggle core)

################################################################################

add_executable(example_sum_graph examples/sum_graph.c)

target_include_directories(example_sum_graph
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

target_link_libraries(example_sum_graph
    PRIVATE daggle core
)

################################################################################

add_executable(example_invoke_graph examples/invoke_graph.c)

target_include_directories(example_invoke_graph
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

target_link_libraries(example_invoke_graph
    PRIVATE daggle graph
)

################################################################################

find_package(Doxygen)

if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.out)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")
    
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
